// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Netwm.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2003
//         Bradley T Hughes <bhughes at trolltech.com>
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

#include <X11/Xatom.h>
#include <X11/Xlib.h>

static const int AtomCount = 51;

typedef unsigned char uchar;


bt::Netwm::Netwm(::Display* _display): display(_display) {
  const struct AtomRef {
    const char *name;
    Atom *atom;
  } refs[AtomCount] = {
    { "UTF8_STRING", &utf8_string },
    { "_NET_SUPPORTED", &net_supported },
    { "_NET_CLIENT_LIST", &net_client_list },
    { "_NET_CLIENT_LIST_STACKING", &net_client_list_stacking },
    { "_NET_NUMBER_OF_DESKTOPS", &net_number_of_desktops },
    { "_NET_DESKTOP_GEOMETRY", &net_desktop_geometry },
    { "_NET_CURRENT_DESKTOP", &net_current_desktop },
    { "_NET_DESKTOP_NAMES", &net_desktop_names },
    { "_NET_ACTIVE_WINDOW", &net_active_window },
    { "_NET_WORKAREA", &net_workarea },
    { "_NET_SUPPORTING_WM_CHECK", &net_supporting_wm_check },
    { "_NET_CLOSE_WINDOW", &net_close_window },
    { "_NET_MOVERESIZE_WINDOW", &net_moveresize_window },
    { "_NET_WM_NAME", &net_wm_name },
    { "_NET_WM_VISIBLE_NAME", &net_wm_visible_name },
    { "_NET_WM_ICON_NAME", &net_wm_icon_name },
    { "_NET_WM_VISIBLE_ICON_NAME", &net_wm_visible_icon_name },
    { "_NET_WM_DESKTOP", &net_wm_desktop },
    { "_NET_WM_WINDOW_TYPE", &net_wm_window_type },
    { "_NET_WM_WINDOW_TYPE_DESKTOP", &net_wm_window_type_desktop },
    { "_NET_WM_WINDOW_TYPE_DOCK", &net_wm_window_type_dock },
    { "_NET_WM_WINDOW_TYPE_TOOLBAR", &net_wm_window_type_toolbar },
    { "_NET_WM_WINDOW_TYPE_MENU", &net_wm_window_type_menu },
    { "_NET_WM_WINDOW_TYPE_UTILITY", &net_wm_window_type_utility },
    { "_NET_WM_WINDOW_TYPE_SPLASH", &net_wm_window_type_splash },
    { "_NET_WM_WINDOW_TYPE_DIALOG", &net_wm_window_type_dialog },
    { "_NET_WM_WINDOW_TYPE_NORMAL", &net_wm_window_type_normal },
    { "_NET_WM_STATE", &net_wm_state },
    { "_NET_WM_STATE_MODAL", &net_wm_state_modal },
    { "_NET_WM_STATE_STICKY", &net_wm_state_sticky },
    { "_NET_WM_STATE_MAXIMIZED_VERT", &net_wm_state_maximized_vert },
    { "_NET_WM_STATE_MAXIMIZED_HORZ", &net_wm_state_maximized_horz },
    { "_NET_WM_STATE_SHADED", &net_wm_state_shaded },
    { "_NET_WM_STATE_SKIP_TASKBAR", &net_wm_state_skip_taskbar },
    { "_NET_WM_STATE_SKIP_PAGER", &net_wm_state_skip_pager },
    { "_NET_WM_STATE_HIDDEN", &net_wm_state_hidden },
    { "_NET_WM_STATE_FULLSCREEN", &net_wm_state_fullscreen },
    { "_NET_WM_STATE_ABOVE", &net_wm_state_above },
    { "_NET_WM_STATE_BELOW", &net_wm_state_below },
    { "_NET_WM_ALLOWED_ACTIONS", &net_wm_allowed_actions },
    { "_NET_WM_ACTION_MOVE", &net_wm_action_move },
    { "_NET_WM_ACTION_RESIZE", &net_wm_action_resize },
    { "_NET_WM_ACTION_MINIMIZE", &net_wm_action_minimize },
    { "_NET_WM_ACTION_SHADE", &net_wm_action_shade },
    { "_NET_WM_ACTION_STICK", &net_wm_action_stick },
    { "_NET_WM_ACTION_MAXIMIZE_HORZ", &net_wm_action_maximize_horz },
    { "_NET_WM_ACTION_MAXIMIZE_VERT", &net_wm_action_maximize_vert },
    { "_NET_WM_ACTION_FULLSCREEN", &net_wm_action_fullscreen },
    { "_NET_WM_ACTION_CHANGE_DESKTOP", &net_wm_action_change_desktop },
    { "_NET_WM_ACTION_CLOSE", &net_wm_action_close },
    { "_NET_WM_STRUT", &net_wm_strut }
  };

  char *names[AtomCount];
  Atom atoms[AtomCount];

  for (int i = 0; i < AtomCount; ++i)
    names[i] = const_cast<char *>(refs[i].name);

  XInternAtoms(display, names, AtomCount, False, atoms);

  for (int i = 0; i < AtomCount; ++i)
    *refs[i].atom = atoms[i];
}


// root window properties

void bt::Netwm::setSupported(Window target, Atom atoms[],
                             unsigned int count) const {
  XChangeProperty(display, target, net_supported, XA_ATOM,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(atoms), count);
}


bool bt::Netwm::readSupported(Window target, AtomList& atoms) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, XA_ATOM, net_supported, &data, &nitems)) {
    Atom* values = reinterpret_cast<Atom*>(data);

    atoms.reserve(nitems);
    atoms.assign(values, values + nitems);

    XFree(data);
  }

  return (! atoms.empty());
}


void bt::Netwm::setClientList(Window target, WindowList& windows) const {
  XChangeProperty(display, target, net_client_list, XA_WINDOW,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(&windows[0]), windows.size());
}


bool bt::Netwm::readClientList(Window target, WindowList& windows) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, XA_WINDOW, net_client_list, &data, &nitems)) {
    Window* values = reinterpret_cast<Window*>(data);

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
                  reinterpret_cast<uchar*>(&windows[0]), windows.size());
}


bool bt::Netwm::readClientListStacking(Window target,
                                       WindowList& windows) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, XA_WINDOW, net_client_list_stacking,
                      &data, &nitems)) {
    Window* values = reinterpret_cast<Window*>(data);

    windows.reserve(nitems);
    windows.assign(values, values + nitems);

    XFree(data);
  }

  return (! windows.empty());
}


void bt::Netwm::setNumberOfDesktops(Window target, unsigned int number) const {
  XChangeProperty(display, target, net_number_of_desktops, XA_CARDINAL,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(&number), 1);
}


bool bt::Netwm::readNumberOfDesktops(Window target,
                                     unsigned int* number) const {
  unsigned char* data = NULL;
  if (getProperty(target, XA_CARDINAL, net_number_of_desktops, &data)) {
    *number = * (reinterpret_cast<unsigned int*>(data));

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
                  reinterpret_cast<uchar*>(geometry), 2);
}


bool bt::Netwm::readDesktopGeometry(Window target,
                                    unsigned int* width,
                                    unsigned int* height) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, XA_CARDINAL, net_desktop_geometry,
                      &data, &nitems) && nitems == 2) {
    unsigned int* values = reinterpret_cast<unsigned int*>(data);

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
                  reinterpret_cast<uchar*>(workareas), count * 4);
}


void bt::Netwm::setCurrentDesktop(Window target, unsigned int number) const {
  XChangeProperty(display, target, net_current_desktop, XA_CARDINAL,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(&number), 1);
}


bool bt::Netwm::readCurrentDesktop(Window target, unsigned int* number) const {
  unsigned char* data = NULL;
  if (getProperty(target, XA_CARDINAL, net_current_desktop, &data)) {
    *number = * (reinterpret_cast<unsigned int*>(data));

    XFree(data);
    return True;
  }
  return False;
}


void bt::Netwm::setDesktopNames(Window target,
                                const std::string& names) const {
  XChangeProperty(display, target, net_desktop_names,
                  utf8_string, 8, PropModeReplace,
                  reinterpret_cast<uchar*>(const_cast<char*>(names.c_str())),
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
                  reinterpret_cast<uchar*>(&data), 1);
}


void bt::Netwm::setSupportingWMCheck(Window target, Window data) const {
  XChangeProperty(display, target, net_supporting_wm_check, XA_WINDOW,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(&data), 1);
}


bool bt::Netwm::readSupportingWMCheck(Window target, Window* window) const {
  unsigned char* data = NULL;
  if (getProperty(target, XA_WINDOW, net_supporting_wm_check, &data)) {
    *window = * (reinterpret_cast<Window*>(data));

    XFree(data);
    return True;
  }
  return False;
}


// application properties

void bt::Netwm::setWMName(Window target, const std::string &name) const {
  XChangeProperty(display, target, net_wm_name, utf8_string,
                  8, PropModeReplace,
                  reinterpret_cast<uchar*>(const_cast<char*>(name.c_str())),
                  name.length());
}


bool bt::Netwm::readWMName(Window target, std::string& name) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, utf8_string, net_wm_name,
                      &data, &nitems) && nitems > 0) {
    name = reinterpret_cast<char*>(data);
    XFree(data);
  }

  return (! name.empty());
}


void bt::Netwm::setWMVisibleName(Window target,
                                 const std::string &name) const {
  XChangeProperty(display, target, net_wm_visible_name, utf8_string,
                  8, PropModeReplace,
                  reinterpret_cast<uchar*>(const_cast<char*>(name.c_str())),
                  name.length());
}


bool bt::Netwm::readWMIconName(Window target, std::string& name) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, utf8_string, net_wm_icon_name,
                      &data, &nitems) && nitems > 0) {
    name = reinterpret_cast<char*>(data);
    XFree(data);
  }

  return (! name.empty());
}


void bt::Netwm::setWMVisibleIconName(Window target,
                                     const std::string &name) const {
  XChangeProperty(display, target, net_wm_visible_icon_name, utf8_string,
                  8, PropModeReplace,
                  reinterpret_cast<uchar*>(const_cast<char*>(name.c_str())),
                  name.length());
}


void bt::Netwm::setWMDesktop(Window target, unsigned int desktop) const {
  XChangeProperty(display, target, net_wm_desktop, XA_CARDINAL,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(&desktop), 1);
}


bool bt::Netwm::readWMDesktop(Window target, unsigned int& desktop) const {
  unsigned char* data = NULL;
  if (getProperty(target, XA_CARDINAL, net_wm_desktop, &data)) {
    desktop = * (reinterpret_cast<int*>(data));

    XFree(data);
    return True;
  }
  return False;
}


bool bt::Netwm::readWMWindowType(Window target, AtomList& types) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, XA_ATOM, net_wm_window_type, &data, &nitems)) {
    Atom* values = reinterpret_cast<Atom*>(data);

    types.reserve(nitems);
    types.assign(values, values + nitems);

    XFree(data);
  }

  return (! types.empty());
}


void bt::Netwm::setWMState(Window target, AtomList& atoms) const {
  XChangeProperty(display, target, net_wm_state, XA_ATOM,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(&(atoms[0])), atoms.size());
}


bool bt::Netwm::readWMState(Window target, AtomList& states) const {
  unsigned char* data = NULL;
  unsigned long nitems;
  if (getListProperty(target, XA_ATOM, net_wm_state, &data, &nitems)) {
    Atom* values = reinterpret_cast<Atom*>(data);

    states.reserve(nitems);
    states.assign(values, values + nitems);

    XFree(data);
  }

  return (! states.empty());
}


void bt::Netwm::setWMAllowedActions(Window target, AtomList& atoms) const {
  XChangeProperty(display, target, net_wm_allowed_actions, XA_ATOM,
                  32, PropModeReplace,
                  reinterpret_cast<uchar*>(&(atoms[0])), atoms.size());
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

  unsigned int* const values = reinterpret_cast<unsigned int*>(data);
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
