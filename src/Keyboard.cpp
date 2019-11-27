//
// Created by fred.nicolson on 25/11/2019.
//

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <cstdlib>
#include <stdexcept>
#include <iostream>
#include "Keyboard.h"

Keyboard::Keyboard()
{
    //Open up the display and get its root
    display = XOpenDisplay(nullptr);
    root_window = RootWindow(display, DefaultScreen(display));
}

Keyboard::~Keyboard()
{
    XCloseDisplay(display);
}

void Keyboard::bind_key(const std::string &key, Keyboard::KeyModifier modifier)
{
    Bool owner_events = False;
    int pointer_mode = GrabModeAsync;
    int keyboard_mode = GrabModeAsync;
    auto modifiers = modifier_to_mask(modifier);

    XGrabKey(display, XKeysymToKeycode(display, XStringToKeysym(key.c_str())), modifiers, root_window, owner_events, pointer_mode, keyboard_mode);
}

void Keyboard::unbind_key(const std::string &key, Keyboard::KeyModifier modifier)
{
    auto modifiers = modifier_to_mask(modifier);
    XUngrabKey(display, XKeysymToKeycode(display, XStringToKeysym(key.c_str())), modifiers, root_window);
}

void Keyboard::wait_for_keys(std::string &key, Keyboard::KeyModifier &modifier)
{
    XEvent event;
    XNextEvent(display, &event);
    modifier = mask_to_modifier(event.xbutton.state);
    key = XKeysymToString(XLookupKeysym(&event.xkey, 0));
    modifier.key_pressed = event.type == KeyPress;
    modifier.key_released = event.type == KeyRelease;
}

unsigned int Keyboard::modifier_to_mask(Keyboard::KeyModifier modifier)
{
    unsigned int mask = 0;
    if(modifier.ctrl_pressed) mask |= ControlMask;
    if(modifier.shift_pressed) mask |= ShiftMask;
    if(modifier.key_pressed) mask |= KeyPressMask;
    if(modifier.key_released) mask |= KeyReleaseMask;
    return mask;
}

Keyboard::KeyModifier Keyboard::mask_to_modifier(unsigned int mask)
{
    KeyModifier modifier = {0};
    modifier.ctrl_pressed = mask & ControlMask;
    modifier.shift_pressed = mask & ShiftMask;
    modifier.key_pressed = mask & KeyPressMask;
    modifier.key_released = mask & KeyReleaseMask;
    return modifier;
}