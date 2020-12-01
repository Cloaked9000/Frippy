//
// Created by fred.nicolson on 12/11/2019.
//

#include "Clipboard.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <atomic>
#include <iostream>

//Convert an atom name in to a std::string
std::string GetAtomName(Display* disp, Atom a)
{
    if(a == None)
        return "None";

    char *name = XGetAtomName(disp, a);
    std::string ret(name);
    XFree(name);
    return ret;
}


class Property
{
public:
    Property(unsigned char *data, int format, unsigned long nitems, Atom type)
    : data(data), format(format), nitems(nitems), type(type)
    {

    }

    ~Property()
    {
        XFree(data);
    }

    Property(Property&&)=delete;
    Property(const Property&)=delete;
    void operator=(const Property&)=delete;
    void operator=(Property&&)=delete;


    unsigned char *data;
    int format;
    unsigned long nitems;
    Atom type;
};

Property read_property(Display* display, Window window, Atom property, Bool delete_old)
{
    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *ret = nullptr;

    int read_bytes = 1024;

    do
    {
        XFree(ret);
        XGetWindowProperty(display, window, property, 0, read_bytes, delete_old, AnyPropertyType,
                           &actual_type, &actual_format, &nitems, &bytes_after,
                           &ret);


        read_bytes *= 2;
    } while(bytes_after);

    return {ret, actual_format, nitems, actual_type};
}

Clipboard::Clipboard(std::vector<std::pair<std::string, std::string>> xa_priority_)
{
    //Copy dependencies
    xa_priority = std::move(xa_priority_);

    //Open up the display and get its root
    display = XOpenDisplay(nullptr);
    root_window = RootWindow(display, DefaultScreen(display));

    //Open up the atoms we need
    clipboard_atom = XInternAtom(display, "CLIPBOARD", False);
    if(!clipboard_atom)
        throw std::runtime_error("Failed to find CLIPBOARD atom");

    xa_targets_atom = XInternAtom(display, "TARGETS", False);
    if(!xa_targets_atom)
        throw std::runtime_error("Failed to fetch TARGETS atom");

    clipboard_type_atom = XInternAtom(display, "UTF8_STRING", False);
    if(!clipboard_type_atom)
        throw std::runtime_error("Failed to fetch UTF8_STRING atom!");

    incr_atom = XInternAtom(display, "INCR", False);
    if(!incr_atom)
        throw std::runtime_error("Failed to fetch INCR atom!");

    //Create an unmapped window which we can use for property transfer
    our_window = XCreateSimpleWindow(display, root_window, 0, 0, 1, 1, 0, 0, 0);
    if(!our_window)
        throw std::runtime_error("Failed to create unmapped window to transfer properties");
}

Clipboard::~Clipboard()
{
    XDestroyWindow(display, our_window);
    XCloseDisplay(display);
}

bool Clipboard::read_clipboard(const Target &conversion_target, const std::function<bool(const std::string &data)> &handler)
{
    //Request conversion
    XConvertSelection(display, clipboard_atom, conversion_target.atom, clipboard_atom, our_window, CurrentTime);

    //Enter into an event loop to process the clipboard responses
    XEvent event;
    while(true)
    {
        XNextEvent(display, &event);
        Property prop = read_property(display, our_window, clipboard_atom, False);

        //If the clipboard data is being sent in batches, and we've got a new batch
        if(event.type == PropertyNotify && event.xproperty.state == PropertyNewValue)
        {
            auto prop_len = prop.nitems * prop.format / 8;
            handler(std::string((char*)prop.data, prop_len));
            XDeleteProperty(event.xproperty.display, our_window, clipboard_atom); //indicate that we've read the data
            if(prop_len == 0) //if the length is 0 then there's nothing left to read
            {
                break;
            }
        }

        //Else this is the data itself
        if(event.type != SelectionNotify)
        {
            continue;
        }

        //incr indicates the data will be sent in batches, setup listening for future events and start it off
        if(prop.type == incr_atom)
        {
            XSelectInput(event.xselection.display, event.xselection.requestor, PropertyChangeMask); //we need to know when the property changes
            XDeleteProperty(event.xselection.display, event.xselection.requestor, event.xselection.property); //indicate start of transfer
            continue;
        }

        //else we have the data, store it and exit
        auto prop_len = prop.nitems * prop.format / 8;
        handler(std::string((char*)prop.data, prop_len));
        break;
    }

    return true;
}

std::vector<Clipboard::Target> Clipboard::list_available_conversions()
{
    std::vector<Target> available_targets;

    //Get the target list
    XConvertSelection(display, clipboard_atom, xa_targets_atom, clipboard_atom, our_window, CurrentTime);

    //Enter into an event loop to process the clipboard responses
    XEvent event;
    while(true)
    {
        XNextEvent(display, &event);

        if(event.type != SelectionNotify)
        {
            continue;
        }

        if(event.xselection.property == None)
        {
            return {}; // no targets
        }

        Property prop = read_property(display, our_window, clipboard_atom, False);
        Atom *atom_list = (Atom*)prop.data;
        for(size_t i = 0; i < prop.nitems; i++)
        {
            std::string atom_name = GetAtomName(display, atom_list[i]);
            available_targets.emplace_back(Target({std::move(atom_name), atom_list[i]}));
        }

        break;
    }

    return available_targets;
}
