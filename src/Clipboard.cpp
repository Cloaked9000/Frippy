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

struct ConversionTarget
{
    Atom atom;
    std::string type;
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

ConversionTarget choose_conversion_target(Display *disp, const Property &prop, Atom xa_target, const std::vector<std::pair<std::string, std::string>> &xa_priority)
{
    //The list of targets is a list of atoms, so it should have type XA_ATOM
    //but it may have the type TARGETS instead.
    if((prop.type != XA_ATOM && prop.type != xa_target) || prop.format != 32)
    {
        return {None, "None"};
    }

    if(prop.nitems == 0)
    {
        return {None, "None"};
    }

    //We basically need to loop over the list of possible clipboard conversions, and then pick the one (if any)
    //that's furthest up our priority list.
    auto score = std::numeric_limits<size_t>::max();
    size_t chosen_index = 0;
    std::string chosen_name;

    Atom *atom_list = (Atom*)prop.data;
    for(size_t i = 0; i < prop.nitems; i++)
    {
        std::string atom_name = GetAtomName(disp, atom_list[i]);
        std::cout << "Available conversion: " << atom_name << std::endl;
        if(auto iter = std::find_if(xa_priority.begin(), xa_priority.end(), [&](const std::pair<std::string, std::string> &elem) {return elem.first == atom_name;}); iter != xa_priority.end())
        {
            size_t this_score = std::distance(xa_priority.begin(), iter);
            if(this_score < score)
            {
                score = this_score;
                chosen_index = i;
                chosen_name = std::move(atom_name);
            }
        }
    }
    std::cout << "Requesting as: " << chosen_name << "(" << chosen_index << ")" << std::endl;
    return {atom_list[chosen_index], chosen_name};
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

Clipboard::ClipboardRead Clipboard::read_clipboard()
{
    //Get the target list
    std::atomic_flag conversions_read(false);
    XConvertSelection(display, clipboard_atom, xa_targets_atom, clipboard_atom, our_window, CurrentTime);

    //Enter into an event loop to process the clipboard responses
    XEvent event;
    ClipboardRead clipboard_read = {};
    ConversionTarget conversionTarget{None, "None"};
    while(true)
    {
        XNextEvent(display, &event);

        //If the clipboard data is being sent in batches, and we've got a new batch
        if(event.type == PropertyNotify && event.xproperty.state == PropertyNewValue && event.xproperty.atom == clipboard_atom)
        {
            Property prop = read_property(display, our_window, clipboard_atom, False);
            auto prop_len = prop.nitems * prop.format / 8;
            clipboard_read.data.append((char*)prop.data, prop_len);
            XDeleteProperty(event.xproperty.display, our_window, clipboard_atom); //indicate that we've read the data
            if(prop_len == 0) //if the length is 0 then there's nothing left to read
            {
                break;
            }
        }

        if(event.type != SelectionNotify)
        {
            continue;
        }

        Atom target = event.xselection.target;
        if(event.xselection.property == None)
            throw std::runtime_error("No possible conversions");

        //Read the clipboard property
        Property prop = read_property(display, our_window, clipboard_atom, False);

        //If this is a list of possible conversions, parse it, so we can request data in the right format
        if(target == xa_targets_atom && !conversions_read.test_and_set())
        {
            //Figure out which conversion we want to use
            conversionTarget = choose_conversion_target(display, prop, xa_targets_atom, xa_priority);
            if(conversionTarget.atom == None)
                throw std::runtime_error("No viable conversion of target available");

            //Now request that specific conversion
            XConvertSelection(display, clipboard_atom, conversionTarget.atom, clipboard_atom, our_window, CurrentTime);
            clipboard_read.type = conversionTarget.type;
            continue;
        }

        //If this is the data itself, receive it
        if(target == conversionTarget.atom)
        {
            //incr indicates the data will be sent in batches, setup listening for future events and start it off
            if(prop.type == incr_atom)
            {
                XSelectInput(event.xselection.display, event.xselection.requestor, PropertyChangeMask); //we need to know when the property changes
                XDeleteProperty(event.xselection.display, event.xselection.requestor, event.xselection.property); //indicate start of transfer
                continue;
            }

            //else we have the data, store it and exit
            auto prop_len = prop.nitems * prop.format / 8;
            clipboard_read.data.append((char*)prop.data, prop_len);
            break;
        }
    }

    return clipboard_read;
}