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
    "_NET_CLIENT_LIST_STACKING",
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
    "_NET_WM_WINDOW_TYPE",
    "_NET_WM_WINDOW_TYPE_DESKTOP",
    "_NET_WM_WINDOW_TYPE_DOCK",
    "_NET_WM_WINDOW_TYPE_TOOLBAR",
    "_NET_WM_WINDOW_TYPE_MENU",
    "_NET_WM_WINDOW_TYPE_UTILITY",
    "_NET_WM_WINDOW_TYPE_SPLASH",
    "_NET_WM_WINDOW_TYPE_DIALOG",
    "_NET_WM_WINDOW_TYPE_NORMAL",
    "_NET_WM_STATE",
    "_NET_WM_STATE_MODAL",
    "_NET_WM_STATE_STICKY",
    "_NET_WM_STATE_MAXIMIZED_VERT",
    "_NET_WM_STATE_MAXIMIZED_HORZ",
    "_NET_WM_STATE_SHADED",
    "_NET_WM_STATE_SKIP_TASKBAR",
    "_NET_WM_STATE_SKIP_PAGER",
    "_NET_WM_STATE_HIDDEN",
    "_NET_WM_STATE_FULLSCREEN",
    "_NET_WM_STATE_ABOVE",
    "_NET_WM_STATE_BELOW",
    "_NET_WM_STATE_REMOVE",
    "_NET_WM_STATE_ADD",
    "_NET_WM_STATE_TOGGLE"
  };
  Atom atoms_return[40];
  XInternAtoms(display, atoms, 40, False, atoms_return);

  utf8_string = atoms_return[0];
  net_supported = atoms_return[1];
  net_client_list = atoms_return[2];
  net_client_list_stacking = atoms_return[3];
  net_number_of_desktops = atoms_return[4];
  net_desktop_geometry = atoms_return[5];
  net_current_desktop = atoms_return[6];
  net_desktop_names = atoms_return[7];
  net_active_window = atoms_return[8];
  net_workarea = atoms_return[9];
  net_supporting_wm_check = atoms_return[10];
  net_close_window = atoms_return[11];
  net_moveresize_window = atoms_return[12];
  net_wm_name = atoms_return[13];
  net_wm_icon_name = atoms_return[14];
  net_wm_desktop = atoms_return[15];
  net_wm_window_type = atoms_return[16];
  net_wm_window_type_desktop = atoms_return[17];
  net_wm_window_type_dock = atoms_return[18];
  net_wm_window_type_toolbar = atoms_return[19];
  net_wm_window_type_menu = atoms_return[20];
  net_wm_window_type_utility = atoms_return[21];
  net_wm_window_type_splash = atoms_return[22];
  net_wm_window_type_dialog = atoms_return[23];
  net_wm_window_type_normal = atoms_return[24];
  net_wm_state = atoms_return[25];
  net_wm_state_modal = atoms_return[26];
  net_wm_state_sticky = atoms_return[27];
  net_wm_state_maximized_vert = atoms_return[28];
  net_wm_state_maximized_horz = atoms_return[29];
  net_wm_state_shaded = atoms_return[30];
  net_wm_state_skip_taskbar = atoms_return[31];
  net_wm_state_skip_pager = atoms_return[32];
  net_wm_state_hidden = atoms_return[33];
  net_wm_state_fullscreen = atoms_return[34];
  net_wm_state_above = atoms_return[35];
  net_wm_state_below = atoms_return[36];
  net_wm_state_remove = atoms_return[37];
  net_wm_state_add = atoms_return[38];
  net_wm_state_toggle = atoms_return[39];
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


void Netwm::setClientListStacking(Window target, const Window windows[],
                                  unsigned int count) const {
  XChangeProperty(display, target, net_client_list_stacking, XA_WINDOW,
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


void Netwm::setWMDesktop(Window target, unsigned int desktop) const {
  XChangeProperty(display, target, net_wm_desktop, XA_CARDINAL,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(&desktop), 1);
}


bool Netwm::readWMDesktop(Window target, unsigned int& desktop) const {
  return getCardinalProperty(target, net_wm_desktop, desktop);
}


bool Netwm::readWMWindowType(Window target, AtomList& types) const {
  return (getAtomListProperty(target, net_wm_window_type, types) &&
          ! types.empty());
}


bool Netwm::readWMState(Window target, AtomList& states) const {
  return (getAtomListProperty(target, net_wm_state, states) &&
          ! states.empty());
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


bool Netwm::getAtomListProperty(Window target, Atom property,
                                AtomList& atoms) const {
  Atom atom_return;
  int size;
  unsigned long nitems, bytes_left;
  unsigned char *data;

  int ret = XGetWindowProperty(display, target, property,
                               0l, 1l, False,
                               XA_ATOM, &atom_return, &size,
                               &nitems, &bytes_left, &data);
  if (ret != Success || nitems < 1)
    return False;

  if (bytes_left != 0) {
    XFree(data);
    unsigned long remain = ((size / 8) * nitems) + bytes_left;
    ret = XGetWindowProperty(display, target,
                             property, 0l, remain, False,
                             XA_ATOM, &atom_return, &size,
                             &nitems, &bytes_left, &data);
    if (ret != Success)
      return False;
  }

  Atom* values = reinterpret_cast<Atom*>(data);

  std::copy(values, values + (nitems * (size/8)), std::back_inserter(atoms));

  XFree(data);

  return True;
}


bool Netwm::isSupportedWMWindowType(Atom atom) const {
  /*
    the implementation looks silly I know.  You are probably thinking
    "why not just do:
    return (atom > net_wm_window_type && atom <= net_wm_window_type_normal)?".
    Well the problem is it assumes the atoms were created in a contiguous
    range. This happens to be true if we created them but could be false if we
    were started after some netwm app which allocated the windows in an odd
    order.  So the following is guaranteed to work even if it looks silly and
    requires more effort to maintain.
  */
  return (atom == net_wm_window_type_desktop ||
          atom == net_wm_window_type_dock ||
          atom == net_wm_window_type_toolbar ||
          atom == net_wm_window_type_menu ||
          atom == net_wm_window_type_utility ||
          atom == net_wm_window_type_splash ||
          atom == net_wm_window_type_dialog ||
          atom == net_wm_window_type_normal);
}

