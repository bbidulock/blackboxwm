// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// netwm.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 Bradley T Hughes <bhughes at trolltech.com>
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

bt::Netwm::Netwm(::Display* _display): display(_display) {
  char* atoms[51] = {
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
    "_NET_WM_VISIBLE_NAME",
    "_NET_WM_ICON_NAME",
    "_NET_WM_VISIBLE_ICON_NAME",
    "_NET_WM_DESKTOP",
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
    "_NET_WM_ALLOWED_ACTIONS",
    "_NET_WM_ALLOWED_ACTION_MOVE",
    "_NET_WM_ALLOWED_ACTION_RESIZE",
    "_NET_WM_ALLOWED_ACTION_MINIMIZE",
    "_NET_WM_ALLOWED_ACTION_SHADE",
    "_NET_WM_ALLOWED_ACTION_STICK",
    "_NET_WM_ALLOWED_ACTION_MAXIMIZE_HORZ",
    "_NET_WM_ALLOWED_ACTION_MAXIMIZE_VERT",
    "_NET_WM_ALLOWED_ACTION_FULLSCREEN",
    "_NET_WM_ALLOWED_ACTION_CHANGE_DESKTOP",
    "_NET_WM_ALLOWED_ACTION_CLOSE",
    "_NET_WM_STRUT"
  };
  Atom atoms_return[51];
  XInternAtoms(display, atoms, 51, False, atoms_return);

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
  net_wm_visible_name = atoms_return[14];
  net_wm_icon_name = atoms_return[15];
  net_wm_visible_icon_name = atoms_return[16];
  net_wm_desktop = atoms_return[17];
  net_wm_window_type = atoms_return[18];
  net_wm_window_type_desktop = atoms_return[19];
  net_wm_window_type_dock = atoms_return[20];
  net_wm_window_type_toolbar = atoms_return[21];
  net_wm_window_type_menu = atoms_return[22];
  net_wm_window_type_utility = atoms_return[23];
  net_wm_window_type_splash = atoms_return[24];
  net_wm_window_type_dialog = atoms_return[25];
  net_wm_window_type_normal = atoms_return[26];
  net_wm_state = atoms_return[27];
  net_wm_state_modal = atoms_return[28];
  net_wm_state_sticky = atoms_return[29];
  net_wm_state_maximized_vert = atoms_return[30];
  net_wm_state_maximized_horz = atoms_return[31];
  net_wm_state_shaded = atoms_return[32];
  net_wm_state_skip_taskbar = atoms_return[33];
  net_wm_state_skip_pager = atoms_return[34];
  net_wm_state_hidden = atoms_return[35];
  net_wm_state_fullscreen = atoms_return[36];
  net_wm_state_above = atoms_return[37];
  net_wm_state_below = atoms_return[38];
  net_wm_allowed_actions = atoms_return[39];
  net_wm_action_move = atoms_return[40];
  net_wm_action_resize = atoms_return[41];
  net_wm_action_minimize = atoms_return[42];
  net_wm_action_shade = atoms_return[43];
  net_wm_action_stick = atoms_return[44];
  net_wm_action_maximize_horz = atoms_return[45];
  net_wm_action_maximize_vert = atoms_return[46];
  net_wm_action_fullscreen = atoms_return[47];
  net_wm_action_change_desktop = atoms_return[48];
  net_wm_action_close = atoms_return[49];
  net_wm_strut = atoms_return[50];
}


// root window properties

void bt::Netwm::setSupported(Window target, Atom atoms[],
                             unsigned int count) const {
  XChangeProperty(display, target, net_supported, XA_ATOM,
                  32, PropModeReplace,
                  static_cast<uchar*>(atoms), count);
}


bool bt::Netwm::readSupported(Window target, AtomList& atoms) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, XA_ATOM, net_supported, &data, &nitems)) {
    Atom* values = static_cast<Atom*>(data);

    atoms.reserve(nitems);
    atoms.assign(values, values + nitems);

    XFree(data);
  }

  return (! atoms.empty());
}


void bt::Netwm::setClientList(Window target, WindowList& windows) const {
  XChangeProperty(display, target, net_client_list, XA_WINDOW,
                  32, PropModeReplace,
                  static_cast<uchar*>(&windows[0]), windows.size());
}


bool bt::Netwm::readClientList(Window target, WindowList& windows) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, XA_WINDOW, net_client_list, &data, &nitems)) {
    Window* values = static_cast<Window*>(data);

    windows.reserve(nitems);
    windows.assign(values, values + nitems);

    XFree(data);
  }

  return (! windows.empty());
}


void bt::Netwm::setClientListStacking(Window target,
                                      WindowList& windows) const {
  XChangeProperty(display, target, net_client_list_stacking, XA_WINDOW,
                  32, PropModeReplace,
                  static_cast<uchar*>(&windows[0]), windows.size());
}


bool bt::Netwm::readClientListStacking(Window target,
                                       WindowList& windows) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, XA_WINDOW, net_client_list_stacking,
                      &data, &nitems)) {
    Window* values = static_cast<Window*>(data);

    windows.reserve(nitems);
    windows.assign(values, values + nitems);

    XFree(data);
  }

  return (! windows.empty());
}


void bt::Netwm::setNumberOfDesktops(Window target, unsigned int number) const {
  XChangeProperty(display, target, net_number_of_desktops, XA_CARDINAL,
                  32, PropModeReplace,
                  static_cast<uchar*>(&number), 1);
}


bool bt::Netwm::readNumberOfDesktops(Window target,
                                     unsigned int* number) const {
  unsigned char* data = NULL;
  if (getProperty(target, XA_CARDINAL, net_number_of_desktops, &data)) {
    *number = * (static_cast<unsigned int*>(data));

    XFree(data);
    return True;
  }
  return False;
}


void bt::Netwm::setDesktopGeometry(Window target,
                                   unsigned int width,
                                   unsigned int height) const {
  unsigned int geometry[] = {width, height};
  XChangeProperty(display, target, net_desktop_geometry, XA_CARDINAL,
                  32, PropModeReplace,
                  static_cast<uchar*>(geometry), 2);
}


bool bt::Netwm::readDesktopGeometry(Window target,
                                    unsigned int* width,
                                    unsigned int* height) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, XA_CARDINAL, net_desktop_geometry,
                      &data, &nitems) && nitems == 2) {
    unsigned int* values = static_cast<unsigned int*>(data);

    *width = values[0];
    *height = values[1];

    XFree(data);
    return True;
  }

  return False;
}


void bt::Netwm::setWorkarea(Window target, unsigned long workareas[],
                            unsigned int count) const {
  XChangeProperty(display, target, net_workarea,
                  XA_CARDINAL, 32, PropModeReplace,
                  static_cast<uchar*>(workareas), count * 4);
}


void bt::Netwm::setCurrentDesktop(Window target, unsigned int number) const {
  XChangeProperty(display, target, net_current_desktop, XA_CARDINAL,
                  32, PropModeReplace,
                  static_cast<uchar*>(&number), 1);
}


bool bt::Netwm::readCurrentDesktop(Window target, unsigned int* number) const {
  unsigned char* data = NULL;
  if (getProperty(target, XA_CARDINAL, net_current_desktop, &data)) {
    *number = * (static_cast<unsigned int*>(data));

    XFree(data);
    return True;
  }
  return False;
}


void bt::Netwm::setDesktopNames(Window target,
                                const std::string& names) const {
  XChangeProperty(display, target, net_desktop_names,
                  utf8_string, 8, PropModeReplace,
                  static_cast<uchar*>(const_cast<char*>(names.c_str())),
                  names.length());
}


bool bt::Netwm::readDesktopNames(Window target, UTF8StringList& names) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, utf8_string, net_desktop_names,
                      &data, &nitems) && nitems > 0) {
    unsigned char* tmp = data;
    for (unsigned int i = 0; i < nitems; ++i) {
      if (data[i] == '\0') {
        names.push_back(std::string(tmp, data + i));
        tmp = data + i;
      }
    }
    XFree(data);
  }

  return (! names.empty());
}


void bt::Netwm::setActiveWindow(Window target, Window data) const {
  XChangeProperty(display, target, net_active_window, XA_WINDOW,
                  32, PropModeReplace,
                  static_cast<uchar*>(&data), 1);
}


void bt::Netwm::setSupportingWMCheck(Window target, Window data) const {
  XChangeProperty(display, target, net_supporting_wm_check, XA_WINDOW,
                  32, PropModeReplace,
                  static_cast<uchar*>(&data), 1);
}


bool bt::Netwm::readSupportingWMCheck(Window target, Window* window) const {
  unsigned char* data = NULL;
  if (getProperty(target, XA_WINDOW, net_supporting_wm_check, &data)) {
    *window = * (static_cast<Window*>(data));

    XFree(data);
    return True;
  }
  return False;
}


// application properties

void bt::Netwm::setWMName(Window target, const std::string &name) const {
  XChangeProperty(display, target, net_wm_name, utf8_string,
                  8, PropModeReplace,
                  static_cast<uchar*>(const_cast<char*>(name.c_str())),
                  name.length());
}


bool bt::Netwm::readWMName(Window target, std::string& name) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, utf8_string, net_wm_name,
                      &data, &nitems) && nitems > 0) {
    name = static_cast<char*>(data);
    XFree(data);
  }

  return (! name.empty());
}


void bt::Netwm::setWMVisibleName(Window target,
                                 const std::string &name) const {
  XChangeProperty(display, target, net_wm_visible_name, utf8_string,
                  8, PropModeReplace,
                  static_cast<uchar*>(const_cast<char*>(name.c_str())),
                  name.length());
}


bool bt::Netwm::readWMIconName(Window target, std::string& name) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, utf8_string, net_wm_icon_name,
                      &data, &nitems) && nitems > 0) {
    name = static_cast<char*>(data);
    XFree(data);
  }

  return (! name.empty());
}


void bt::Netwm::setWMVisibleIconName(Window target,
                                     const std::string &name) const {
  XChangeProperty(display, target, net_wm_visible_icon_name, utf8_string,
                  8, PropModeReplace,
                  static_cast<uchar*>(const_cast<char*>(name.c_str())),
                  name.length());
}


void bt::Netwm::setWMDesktop(Window target, unsigned int desktop) const {
  XChangeProperty(display, target, net_wm_desktop, XA_CARDINAL,
                  32, PropModeReplace,
                  static_cast<uchar*>(&desktop), 1);
}


bool bt::Netwm::readWMDesktop(Window target, unsigned int& desktop) const {
  unsigned char* data = NULL;
  if (getProperty(target, XA_CARDINAL, net_wm_desktop, &data)) {
    desktop = * (static_cast<int*>(data));

    XFree(data);
    return True;
  }
  return False;
}


bool bt::Netwm::readWMWindowType(Window target, AtomList& types) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, XA_ATOM, net_wm_window_type, &data, &nitems)) {
    Atom* values = static_cast<Atom*>(data);

    types.reserve(nitems);
    types.assign(values, values + nitems);

    XFree(data);
  }

  return (! types.empty());
}


void bt::Netwm::setWMState(Window target, AtomList& atoms) const {
  XChangeProperty(display, target, net_wm_state, XA_ATOM,
                  32, PropModeReplace,
                  static_cast<uchar*>(&(atoms[0])), atoms.size());
}


bool bt::Netwm::readWMState(Window target, AtomList& states) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, XA_ATOM, net_wm_state, &data, &nitems)) {
    Atom* values = static_cast<Atom*>(data);

    states.reserve(nitems);
    states.assign(values, values + nitems);

    XFree(data);
  }

  return (! states.empty());
}


void bt::Netwm::setWMAllowedActions(Window target, AtomList& atoms) const {
  XChangeProperty(display, target, net_wm_allowed_actions, XA_ATOM,
                  32, PropModeReplace,
                  static_cast<uchar*>(&(atoms[0])), atoms.size());
}


bool bt::Netwm::readWMStrut(Window target, Strut* strut) const {
  Atom atom_return;
  int size;
  unsigned long nitems, bytes_left;
  unsigned char *data;

  int ret = XGetWindowProperty(display, target, net_wm_strut,
                               0l, 4l, False,
                               XA_CARDINAL, &atom_return, &size,
                               &nitems, &bytes_left, &data);
  if (ret != Success || nitems < 4)
    return False;

  unsigned int* const values = static_cast<unsigned int*>(data);
  strut->left   = values[0];
  strut->right  = values[1];
  strut->top    = values[2];
  strut->bottom = values[3];

  XFree(data);

  return True;
}


// utility

void bt::Netwm::removeProperty(Window target, Atom atom) const {
  XDeleteProperty(display, target, atom);
}


bool bt::Netwm::getProperty(Window target, Atom type, Atom property,
                            unsigned char** data) const {
  Atom atom_return;
  int size;
  unsigned long nitems, bytes_left;

  int ret = XGetWindowProperty(display, target, property,
                               0l, 1l, False,
                               type, &atom_return, &size,
                               &nitems, &bytes_left, data);
  if (ret != Success || nitems != 1)
    return False;

  return True;
}


bool bt::Netwm::getListProperty(Window target, Atom type, Atom property,
                                unsigned char** data,
                                unsigned long* count) const {
  Atom atom_return;
  int size;
  unsigned long nitems, bytes_left;

  int ret = XGetWindowProperty(display, target, property,
                               0l, 1l, False,
                               type, &atom_return, &size,
                               &nitems, &bytes_left, data);
  if (ret != Success || nitems < 1)
    return False;

  if (bytes_left != 0) {
    XFree(*data);
    unsigned long remain = ((size / 8) * nitems) + bytes_left;
    ret = XGetWindowProperty(display, target,
                             property, 0l, remain, False,
                             type, &atom_return, &size,
                             &nitems, &bytes_left, data);
    if (ret != Success)
      return False;
  }

  *count = nitems;
  return True;
}


bool bt::Netwm::isSupportedWMWindowType(Atom atom) const {
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
