// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// netwm.cc for Blackbox - an X11 Window manager
// Copyright (c) 2002 Sean 'Shaleh' Perry <shaleh@debian.org>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include "Netwm.hh"


Netwm::Netwm(Display* display) {
  char* atoms[] = {
    "UTF8_STRING",
    "_NET_SUPPORTED",
    "_NET_NUMBER_OF_DESKTOPS",
    "_NET_CURRENT_DESKTOP",
    "_NET_SUPPORTING_WM_CHECK",
    "_NET_WM_NAME"
  };
  Atom atoms_return[6];
  XInternAtoms(display, atoms, 6, False, atoms_return);

  utf8_string = atoms_return[0];
  net_supported = atoms_return[1];
  net_number_of_desktops = atoms_return[2];
  net_current_desktop = atoms_return[3];
  net_supporting_wm_check = atoms_return[4];
  net_wm_name = atoms_return[5];
}


#define uchar unsigned char
// root window properties

void Netwm::setSupported(Atom* supported, unsigned int count,
                         Display *display,Window target) {
  XChangeProperty(display, target, net_supported, XA_ATOM,
                  32, PropModeReplace,
                  reinterpret_cast<unsigned char*>(supported), count);
}


void Netwm::setNumberOfDesktops(unsigned int number, Display* display,
                                Window target) {
  XChangeProperty(display, target, net_number_of_desktops, XA_CARDINAL,
                  32, PropModeReplace,
                  reinterpret_cast<unsigned char*>(&number), 1);
}


void Netwm::setCurrentDesktop(unsigned int number, Display* display,
                              Window target) {
  XChangeProperty(display, target, net_current_desktop, XA_CARDINAL,
                  32, PropModeReplace,
                  reinterpret_cast<unsigned char*>(&number), 1);
}


void Netwm::setSupportingWMCheck(Window target, Window data,
                                 Display* display) {
  XChangeProperty(display, target, net_supporting_wm_check, XA_WINDOW,
                  sizeof(Window), PropModeReplace,
                  reinterpret_cast<unsigned char*>(&data), 1);
}


// application properties

void Netwm::setWMName(const string& name, Window w, Display *display) {
  XChangeProperty(display, w, net_wm_name, utf8_string,
                  sizeof(char), PropModeReplace,
                  reinterpret_cast<uchar*>(const_cast<char*>(name.c_str())),
                  name.length());
}
#undef uchar


void Netwm::removeProperty(Atom atom, Display *display, Window target) const {
  XDeleteProperty(display, target, atom);
}
