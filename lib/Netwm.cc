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

typedef unsigned char uchar;

Netwm::Netwm(Display* _display)
  : display(_display)
{
  char* atoms[] = {
    "UTF8_STRING",
    "_NET_SUPPORTED",
    "_NET_NUMBER_OF_DESKTOPS",
    "_NET_DESKTOP_GEOMETRY",
    "_NET_DESKTOP_VIEWPORT",
    "_NET_CURRENT_DESKTOP",
    "_NET_ACTIVE_WINDOW",
    "_NET_WORKAREA",
    "_NET_SUPPORTING_WM_CHECK",
    "_NET_WM_NAME"
  };
  Atom atoms_return[10];
  XInternAtoms(display, atoms, 10, False, atoms_return);

  utf8_string = atoms_return[0];
  net_supported = atoms_return[1];
  net_number_of_desktops = atoms_return[2];
  net_desktop_geometry = atoms_return[3];
  net_desktop_viewport = atoms_return[4];
  net_current_desktop = atoms_return[5];
  net_active_window = atoms_return[6];
  net_workarea = atoms_return[7];
  net_supporting_wm_check = atoms_return[8];
  net_wm_name = atoms_return[9];
}


// root window properties

void Netwm::setSupported(Window target, Atom* supported,
                         unsigned int count) const {
  XChangeProperty(display, target, net_supported, XA_ATOM,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(supported), count);
}


void Netwm::setNumberOfDesktops(Window target, unsigned int number) const {
  XChangeProperty(display, target, net_number_of_desktops, XA_CARDINAL,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(&number), 1);
}


void Netwm::setDesktopGeometry(Window target,
                               unsigned int width, unsigned int height) const {
  unsigned int geometry[] = {width, height};
  XChangeProperty(display, target, net_desktop_geometry, XA_CARDINAL,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(geometry), 2);
}


void Netwm::setWorkarea(Window target, unsigned int x, unsigned y,
                        unsigned int width, unsigned int height) const {
  unsigned int area[] = {x, y, width, height};
  XChangeProperty(display, target, net_workarea, XA_CARDINAL,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(area), 4);
}


void Netwm::setDesktopViewport(Window target,
                               unsigned int x, unsigned int y) const {
  unsigned int coords[] = {x, y};
  XChangeProperty(display, target, net_desktop_viewport, XA_CARDINAL,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(coords), 2);
}


void Netwm::setCurrentDesktop(Window target, unsigned int number) const {
  XChangeProperty(display, target, net_current_desktop, XA_CARDINAL,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(&number), 1);
}


void Netwm::setActiveWindow(Window target, Window data) const {
  XChangeProperty(display, target, net_active_window, XA_WINDOW,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(&data), 1);
}


void Netwm::setSupportingWMCheck(Window target, Window data) const {
  XChangeProperty(display, target, net_supporting_wm_check, XA_WINDOW,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(&data), 1);
}


// application properties

void Netwm::setWMName(Window target, const std::string &name) const {
  XChangeProperty(display, target, net_wm_name, utf8_string,
                  8, PropModeReplace,
                  reinterpret_cast<uchar*>(const_cast<char*>(name.c_str())),
                  name.length());
}


void Netwm::removeProperty(Window target, Atom atom) const {
  XDeleteProperty(display, target, atom);
}
