// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// netwm.cc for Blackbox - an X11 Window manager
// Copyright (c) 2002 Brad Hughes (bhughes@tcac.net),
//                    Sean 'Shaleh' Perry <shaleh@debian.org>
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

Netwm::Netwm(Display* _display): display(_display) {
  char* atoms[] = {
    "UTF8_STRING",
    "_NET_SUPPORTED",
    "_NET_CLIENT_LIST",
    "_NET_NUMBER_OF_DESKTOPS",
    "_NET_DESKTOP_GEOMETRY",
    "_NET_CURRENT_DESKTOP",
    "_NET_DESKTOP_NAMES",
    "_NET_ACTIVE_WINDOW",
    "_NET_WORKAREA",
    "_NET_SUPPORTING_WM_CHECK",
    "_NET_CLOSE_WINDOW",
    "_NET_MOVERESIZE_WINDOW",
    "_NET_WM_NAME",
    "_NET_WM_ICON_NAME",
    "_NET_WM_DESKTOP"
  };
  Atom atoms_return[15];
  XInternAtoms(display, atoms, 15, False, atoms_return);

  utf8_string = atoms_return[0];
  net_supported = atoms_return[1];
  net_client_list = atoms_return[2];
  net_number_of_desktops = atoms_return[3];
  net_desktop_geometry = atoms_return[4];
  net_current_desktop = atoms_return[5];
  net_desktop_names = atoms_return[6];
  net_active_window = atoms_return[7];
  net_workarea = atoms_return[8];
  net_supporting_wm_check = atoms_return[9];
  net_close_window = atoms_return[10];
  net_moveresize_window = atoms_return[11];
  net_wm_name = atoms_return[12];
  net_wm_icon_name = atoms_return[13];
  net_wm_desktop = atoms_return[14];
}


// root window properties

void Netwm::setSupported(Window target, Atom atoms[],
                         unsigned int count) const {
  XChangeProperty(display, target, net_supported, XA_ATOM,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(atoms), count);
}


void Netwm::setClientList(Window target, const Window windows[],
                          unsigned int count) const {
  XChangeProperty(display, target, net_client_list, XA_WINDOW,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(&windows), count);
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


void Netwm::setWorkarea(Window target, unsigned long workareas[],
                        unsigned int count) const {
  XChangeProperty(display, target, net_workarea,
                  XA_CARDINAL, 32, PropModeReplace,
                  reinterpret_cast<uchar*>(workareas), count * 4);
}


void Netwm::setCurrentDesktop(Window target, unsigned int number) const {
  XChangeProperty(display, target, net_current_desktop, XA_CARDINAL,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(&number), 1);
}


void Netwm::setDesktopNames(Window target, const std::string& names) const {
  XChangeProperty(display, target, net_desktop_names,
                  utf8_string, 8, PropModeReplace,
                  reinterpret_cast<uchar*>(const_cast<char*>(names.c_str())),
                  names.length());
}


std::vector<std::string> Netwm::readDesktopNames(Window target) const {
  std::string names;
  std::vector<std::string> ret;

  if (getUTF8StringProperty(target, net_desktop_names, names) &&
      ! names.empty()) {
    std::string::iterator it = names.begin(), end = names.end(), tmp = it;
    for (; it != end; ++it) {
      if (*it == '\0') {
        ret.push_back(std::string(tmp, it));
        tmp = it;
      }
    }
  }

  return ret;
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


bool Netwm::readWMName(Window target, std::string& name) const {
  return getUTF8StringProperty(target, net_wm_name, name);
}


bool Netwm::readWMIconName(Window target, std::string& name) const {
  return getUTF8StringProperty(target, net_wm_icon_name, name);
}


bool Netwm::readWMDesktop(Window target, unsigned int& desktop) const {
  return getCardinalProperty(target, net_wm_desktop, desktop);
}


// utility

void Netwm::removeProperty(Window target, Atom atom) const {
  XDeleteProperty(display, target, atom);
}


bool Netwm::getUTF8StringProperty(Window target, Atom property,
                                  std::string& value) const {
  Atom atom_return;
  int size;
  unsigned long nitems, bytes_left;
  unsigned char *data;

  int ret = XGetWindowProperty(display, target, property,
                               0l, 1l, False,
                               utf8_string, &atom_return, &size,
                               &nitems, &bytes_left, &data);
  if (ret != Success || nitems < 1)
    return False;

  if (bytes_left != 0) {
    XFree(data);
    unsigned long remain = ((size / 8) * nitems) + bytes_left;
    ret = XGetWindowProperty(display, target,
                             property, 0l, remain, False,
                             utf8_string, &atom_return, &size,
                             &nitems, &bytes_left, &data);
    if (ret != Success)
      return False;
  }

  value = reinterpret_cast<char*>(data);

  XFree(data);

  return True;
}


bool Netwm::getCardinalProperty(Window target, Atom property,
                                unsigned int& value) const {
  Atom atom_return;
  int size;
  unsigned long nitems, bytes_left;
  unsigned char *data;

  int ret = XGetWindowProperty(display, target, property,
                               0l, 1l, False,
                               XA_CARDINAL, &atom_return, &size,
                               &nitems, &bytes_left, &data);
  if (ret != Success || nitems < 1)
    return False;

  value = * (reinterpret_cast<int*>(data));

  return True;
}
