// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// EWMH.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2005
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

#include "EWMH.hh"

#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <stdio.h>


bt::EWMH::EWMH(const Display &_display)
  : display(_display)
{
  const struct AtomRef {
    const char *name;
    Atom *atom;
  } refs[] = {
    { "UTF8_STRING", &utf8_string },
    { "_NET_SUPPORTED", &net_supported },
    { "_NET_CLIENT_LIST", &net_client_list },
    { "_NET_CLIENT_LIST_STACKING", &net_client_list_stacking },
    { "_NET_NUMBER_OF_DESKTOPS", &net_number_of_desktops },
    { "_NET_DESKTOP_GEOMETRY", &net_desktop_geometry },
    { "_NET_DESKTOP_VIEWPORT", &net_desktop_viewport },
    { "_NET_CURRENT_DESKTOP", &net_current_desktop },
    { "_NET_DESKTOP_NAMES", &net_desktop_names },
    { "_NET_ACTIVE_WINDOW", &net_active_window },
    { "_NET_WORKAREA", &net_workarea },
    { "_NET_SUPPORTING_WM_CHECK", &net_supporting_wm_check },
    { "_NET_VIRTUAL_ROOTS", &net_virtual_roots },
    // { "_NET_DESKTOP_LAYOUT", &net_desktop_layout },
    { "_NET_SHOWING_DESKTOP", &net_showing_desktop },
    { "_NET_CLOSE_WINDOW", &net_close_window },
    { "_NET_MOVERESIZE_WINDOW", &net_moveresize_window },
    { "_NET_WM_MOVERESIZE", &net_wm_moveresize },
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
    { "_NET_WM_STRUT", &net_wm_strut },
    { "_NET_WM_STRUT_PARTIAL", &net_wm_strut_partial },
    { "_NET_WM_ICON_GEOMETRY", &net_wm_icon_geometry },
    // { "_NET_WM_ICON", &net_wm_icon },
    { "_NET_WM_PID", &net_wm_pid },
    // { "_NET_WM_HANDLED_ICONS", &net_wm_handled_icons },
    { "_NET_WM_USER_TIME", &net_wm_user_time },
    { "_NET_WM_PING", &net_wm_ping }
  };

  static const int AtomCount =
    sizeof(refs) / (sizeof(const char *) + sizeof(Atom *));
  char *names[AtomCount];
  Atom atoms[AtomCount];

  for (int i = 0; i < AtomCount; ++i)
    names[i] = const_cast<char *>(refs[i].name);

  XInternAtoms(display.XDisplay(), names, AtomCount, false, atoms);

  for (int i = 0; i < AtomCount; ++i)
    *refs[i].atom = atoms[i];
}


// root window properties

void bt::EWMH::setSupported(Window target, Atom atoms[],
                             unsigned int count) const {
  setProperty(target, XA_ATOM, net_supported,
              reinterpret_cast<unsigned char *>(atoms), count);
}


bool bt::EWMH::readSupported(Window target, AtomList& atoms) const {
  unsigned char* data = 0;
  unsigned long nitems;
  if (getListProperty(target, XA_ATOM, net_supported, &data, &nitems)) {
    Atom* values = reinterpret_cast<Atom*>(data);

    atoms.reserve(nitems);
    atoms.assign(values, values + nitems);

    XFree(data);
  }

  return (!atoms.empty());
}


void bt::EWMH::setClientList(Window target, WindowList& windows) const {
  setProperty(target, XA_WINDOW, net_client_list,
              reinterpret_cast<unsigned char *>(&windows[0]), windows.size());
}


bool bt::EWMH::readClientList(Window target, WindowList& windows) const {
  unsigned char* data = 0;
  unsigned long nitems;
  if (getListProperty(target, XA_WINDOW, net_client_list, &data, &nitems)) {
    Window* values = reinterpret_cast<Window*>(data);

    windows.reserve(nitems);
    windows.assign(values, values + nitems);

    XFree(data);
  }

  return (!windows.empty());
}


void bt::EWMH::setClientListStacking(Window target,
                                      WindowList& windows) const {
  setProperty(target, XA_WINDOW, net_client_list_stacking,
              reinterpret_cast<unsigned char *>(&windows[0]), windows.size());
}


bool bt::EWMH::readClientListStacking(Window target,
                                       WindowList& windows) const {
  unsigned char* data = 0;
  unsigned long nitems;
  if (getListProperty(target, XA_WINDOW, net_client_list_stacking,
                      &data, &nitems)) {
    Window* values = reinterpret_cast<Window*>(data);

    windows.reserve(nitems);
    windows.assign(values, values + nitems);

    XFree(data);
  }

  return (!windows.empty());
}


void bt::EWMH::setNumberOfDesktops(Window target, unsigned int number) const {
  const unsigned long x = number;
  setProperty(target, XA_CARDINAL, net_number_of_desktops,
              reinterpret_cast<const unsigned char *>(&x), 1);
}


bool bt::EWMH::readNumberOfDesktops(Window target,
                                     unsigned int* number) const {
  unsigned char* data = 0;
  if (getProperty(target, XA_CARDINAL, net_number_of_desktops, &data)) {
    *number =
      static_cast<unsigned int>(*(reinterpret_cast<unsigned long *>(data)));

    XFree(data);
    return true;
  }
  return false;
}


void bt::EWMH::setDesktopGeometry(Window target,
                                   unsigned int width,
                                   unsigned int height) const {
  const unsigned long geometry[] =
    { static_cast<unsigned long>(width), static_cast<unsigned long> (height) };
  setProperty(target, XA_CARDINAL, net_desktop_geometry,
              reinterpret_cast<const unsigned char *>(geometry), 2);
}


bool bt::EWMH::readDesktopGeometry(Window target,
                                    unsigned int* width,
                                    unsigned int* height) const {
  unsigned char* data = 0;
  unsigned long nitems;
  if (getListProperty(target, XA_CARDINAL, net_desktop_geometry,
                      &data, &nitems) && nitems == 2) {
    unsigned long *values = reinterpret_cast<unsigned long *>(data);

    *width  = static_cast<unsigned int>(values[0]);
    *height = static_cast<unsigned int>(values[1]);

    XFree(data);
    return true;
  }

  return false;
}


void bt::EWMH::setDesktopViewport(Window target, int x, int y) const {
  const unsigned long viewport[] =
    { static_cast<long>(x), static_cast<long>(y) };
  setProperty(target, XA_CARDINAL, net_desktop_viewport,
              reinterpret_cast<const unsigned char *>(viewport), 2);
}


bool bt::EWMH::readDesktopViewport(Window target, int *x, int *y) const {
  unsigned char* data = 0;
  unsigned long nitems;
  if (getListProperty(target, XA_CARDINAL, net_desktop_viewport,
                      &data, &nitems) && nitems == 2) {
    const long * const values = reinterpret_cast<long *>(data);

    *x = static_cast<int>(values[0]);
    *y = static_cast<int>(values[1]);

    XFree(data);
    return true;
  }

  return false;
}


void bt::EWMH::setWorkarea(Window target, unsigned long workareas[],
                            unsigned int count) const {
  setProperty(target, XA_CARDINAL, net_workarea,
              reinterpret_cast<unsigned char *>(workareas), count * 4);
}


void bt::EWMH::setCurrentDesktop(Window target, unsigned int number) const {
  const unsigned long x = static_cast<unsigned long>(number);
  setProperty(target, XA_CARDINAL, net_current_desktop,
              reinterpret_cast<const unsigned char *>(&x), 1);
}


bool bt::EWMH::readCurrentDesktop(Window target, unsigned int* number) const {
  unsigned char* data = 0;
  if (getProperty(target, XA_CARDINAL, net_current_desktop, &data)) {
    *number =
      static_cast<unsigned int>(*(reinterpret_cast<unsigned long *>(data)));

    XFree(data);
    return true;
  }
  return false;
}


void bt::EWMH::setDesktopNames(Window target,
                                const std::vector<bt::ustring> &names) const {
  if (!hasUnicode())
    return; // cannot convert UTF-32 to UTF-8

  std::string s;
  std::vector<bt::ustring>::const_iterator it = names.begin();
  const std::vector<bt::ustring>::const_iterator end = names.end();
  for (; it != end; ++it)
    s += toUtf8(*it) + '\0';

  XChangeProperty(display.XDisplay(), target, net_desktop_names, utf8_string,
                  8, PropModeReplace,
                  reinterpret_cast<const unsigned char *>(s.c_str()),
                  s.length());
}


bool bt::EWMH::readDesktopNames(Window target,
                                 std::vector<bt::ustring> &names) const {
  if (!hasUnicode())
    return false; // cannot convert UTF-8 to UTF-32

  unsigned char *data = 0;
  unsigned long nitems;
  if (getListProperty(target, utf8_string, net_desktop_names,
                      &data, &nitems) && nitems > 0) {
    char *tmp = reinterpret_cast<char *>(data);
    for (unsigned int i = 0; i < nitems; ++i) {
      if (data[i] == '\0') {
        const std::string str(tmp, reinterpret_cast<char *>(data) + i);
        names.push_back(toUtf32(str));
        tmp = reinterpret_cast<char *>(data) + i + 1;
      }
    }
    XFree(data);
  }

  return (!names.empty());
}


void bt::EWMH::setActiveWindow(Window target, Window data) const {
  setProperty(target, XA_WINDOW, net_active_window,
              reinterpret_cast<unsigned char *>(&data), 1);
}


void bt::EWMH::setSupportingWMCheck(Window target, Window data) const {
  setProperty(target, XA_WINDOW, net_supporting_wm_check,
              reinterpret_cast<unsigned char *>(&data), 1);
}


bool bt::EWMH::readSupportingWMCheck(Window target, Window* window) const {
  unsigned char* data = 0;
  if (getProperty(target, XA_WINDOW, net_supporting_wm_check, &data)) {
    *window = * (reinterpret_cast<Window*>(data));

    XFree(data);
    return true;
  }
  return false;
}


void bt::EWMH::setVirtualRoots(Window target, WindowList &windows) const {
  setProperty(target, XA_WINDOW, net_virtual_roots,
              reinterpret_cast<unsigned char *>(&windows[0]), windows.size());
}


bool bt::EWMH::readVirtualRoots(Window target, WindowList &windows) const {
  unsigned char* data = 0;
  unsigned long nitems;
  if (getListProperty(target, XA_WINDOW, net_virtual_roots, &data, &nitems)) {
    Window* values = reinterpret_cast<Window*>(data);

    windows.reserve(nitems);
    windows.assign(values, values + nitems);

    XFree(data);
  }

  return (!windows.empty());
}


// application properties

void bt::EWMH::setWMName(Window target, const bt::ustring &name) const {
  if (!hasUnicode())
    return; // cannot convert UTF-32 to UTF-8

  const std::string utf8 = toUtf8(name);
  XChangeProperty(display.XDisplay(), target, net_wm_name, utf8_string,
                  8, PropModeReplace,
                  reinterpret_cast<const unsigned char *>(utf8.c_str()),
                  utf8.length());
}


bool bt::EWMH::readWMName(Window target, bt::ustring &name) const {
  if (!hasUnicode())
    return false; // cannot convert UTF-8 to UTF-32

  unsigned char* data = 0;
  unsigned long nitems;
  if (getListProperty(target, utf8_string, net_wm_name,
                      &data, &nitems) && nitems > 0) {
    name = toUtf32(reinterpret_cast<char*>(data));
    XFree(data);
  }

  return (!name.empty());
}


void bt::EWMH::setWMVisibleName(Window target,
                                 const bt::ustring &name) const {
  if (!hasUnicode())
    return; // cannot convert UTF-32 to UTF-8

  const std::string utf8 = toUtf8(name);
  XChangeProperty(display.XDisplay(), target, net_wm_visible_name, utf8_string,
                  8, PropModeReplace,
                  reinterpret_cast<const unsigned char *>(utf8.c_str()),
                  utf8.length());
}


bool bt::EWMH::readWMIconName(Window target, bt::ustring &name) const {
  if (!hasUnicode())
    return false; // cannot convert UTF-8 to UTF-32

  unsigned char* data = 0;
  unsigned long nitems;
  if (getListProperty(target, utf8_string, net_wm_icon_name,
                      &data, &nitems) && nitems > 0) {
    name = toUtf32(reinterpret_cast<char*>(data));
    XFree(data);
  }

  return (!name.empty());
}


void bt::EWMH::setWMVisibleIconName(Window target,
                                     const bt::ustring &name) const {
  if (!hasUnicode())
    return; // cannot convert UTF-32 to UTF-8

  const std::string utf8 = toUtf8(name);
  XChangeProperty(display.XDisplay(), target, net_wm_visible_icon_name, utf8_string,
                  8, PropModeReplace,
                  reinterpret_cast<const unsigned char *>(utf8.c_str()),
                  utf8.length());
}


void bt::EWMH::setWMDesktop(Window target, unsigned int desktop) const {
  const unsigned long x = desktop;
  setProperty(target, XA_CARDINAL, net_wm_desktop,
              reinterpret_cast<const unsigned char *>(&x), 1);
}


bool bt::EWMH::readWMDesktop(Window target, unsigned int& desktop) const {
  unsigned char* data = 0;
  if (getProperty(target, XA_CARDINAL, net_wm_desktop, &data)) {
    desktop =
      static_cast<unsigned int>(*(reinterpret_cast<unsigned long *>(data)));

    XFree(data);
    return true;
  }
  return false;
}


bool bt::EWMH::readWMWindowType(Window target, AtomList& types) const {
  unsigned char* data = 0;
  unsigned long nitems;
  if (getListProperty(target, XA_ATOM, net_wm_window_type, &data, &nitems)) {
    Atom* values = reinterpret_cast<Atom*>(data);

    types.reserve(nitems);
    types.assign(values, values + nitems);

    XFree(data);
  }

  return (!types.empty());
}


void bt::EWMH::setWMState(Window target, AtomList& atoms) const {
  setProperty(target, XA_ATOM, net_wm_state,
              reinterpret_cast<unsigned char *>(&(atoms[0])), atoms.size());
}


bool bt::EWMH::readWMState(Window target, AtomList& states) const {
  unsigned char* data = 0;
  unsigned long nitems;
  if (getListProperty(target, XA_ATOM, net_wm_state, &data, &nitems)) {
    Atom* values = reinterpret_cast<Atom*>(data);

    states.reserve(nitems);
    states.assign(values, values + nitems);

    XFree(data);
  }

  return (!states.empty());
}


void bt::EWMH::setWMAllowedActions(Window target, AtomList& atoms) const {
  setProperty(target, XA_ATOM, net_wm_allowed_actions,
              reinterpret_cast<unsigned char *>(&(atoms[0])), atoms.size());
}


bool bt::EWMH::readWMStrut(Window target, Strut* strut) const {
  Atom atom_return;
  int size;
  unsigned long nitems, bytes_left;
  unsigned char *data;

  int ret = XGetWindowProperty(display.XDisplay(), target, net_wm_strut,
                               0l, 4l, false,
                               XA_CARDINAL, &atom_return, &size,
                               &nitems, &bytes_left, &data);
  if (ret != Success || nitems < 4)
    return false;

  unsigned long* const values = reinterpret_cast<unsigned long*>(data);

  strut->left   = static_cast<unsigned int>(values[0]);
  strut->right  = static_cast<unsigned int>(values[1]);
  strut->top    = static_cast<unsigned int>(values[2]);
  strut->bottom = static_cast<unsigned int>(values[3]);

  XFree(data);

  return true;
}


bool bt::EWMH::readWMStrutPartial(Window target, StrutPartial* strut) const {
  Atom atom_return;
  int size;
  unsigned long nitems, bytes_left;
  unsigned char *data;

  int ret = XGetWindowProperty(display.XDisplay(), target, net_wm_strut_partial,
                               0l, 12l, false,
                               XA_CARDINAL, &atom_return, &size,
                               &nitems, &bytes_left, &data);
  if (ret != Success || nitems < 4)
    return false;

  unsigned long * const values = reinterpret_cast<unsigned long *>(data);

  strut->left         = static_cast<unsigned int>(values[0]);
  strut->right        = static_cast<unsigned int>(values[1]);
  strut->top          = static_cast<unsigned int>(values[2]);
  strut->bottom       = static_cast<unsigned int>(values[3]);
  strut->left_start   = static_cast<unsigned int>(values[4]);
  strut->left_end     = static_cast<unsigned int>(values[5]);
  strut->right_start  = static_cast<unsigned int>(values[6]);
  strut->right_end    = static_cast<unsigned int>(values[7]);
  strut->top_start    = static_cast<unsigned int>(values[8]);
  strut->top_end      = static_cast<unsigned int>(values[9]);
  strut->bottom_start = static_cast<unsigned int>(values[10]);
  strut->bottom_end   = static_cast<unsigned int>(values[11]);

  XFree(data);

  return true;
}


bool bt::EWMH::readWMIconGeometry(Window target, int &x, int &y,
                                   unsigned int &width,
                                   unsigned int &height) const {
  unsigned char *data = 0;
  unsigned long nitems;
  if (getListProperty(target, XA_CARDINAL, net_wm_icon_geometry,
                      &data, &nitems) && nitems == 4) {
    unsigned long *values = reinterpret_cast<unsigned long *>(data);

    x = static_cast<int>(values[0]);
    y = static_cast<int>(values[1]);
    width  = static_cast<unsigned int>(values[2]);
    height = static_cast<unsigned int>(values[3]);

    XFree(data);
    return true;
  }

  return false;
}


bool bt::EWMH::readWMPid(Window target, unsigned int &pid) const {
  unsigned char* data = 0;
  if (getProperty(target, XA_CARDINAL, net_wm_pid, &data)) {
    pid =
      static_cast<unsigned int>(*(reinterpret_cast<unsigned long *>(data)));

    XFree(data);
    return true;
  }
  return false;
}


bool bt::EWMH::readWMUserTime(Window target, Time &user_time) const {
  unsigned char* data = 0;
  if (getProperty(target, XA_CARDINAL, net_wm_user_time, &data)) {
    user_time = *(reinterpret_cast<unsigned long *>(data));

    XFree(data);
    return true;
  }
  return false;
}


// utility

void bt::EWMH::removeProperty(Window target, Atom atom) const {
  XDeleteProperty(display.XDisplay(), target, atom);
}


void bt::EWMH::setProperty(Window target, Atom type, Atom property,
                            const unsigned char *data,
                            unsigned long count) const {
  XChangeProperty(display.XDisplay(), target, property, type,
                  32, PropModeReplace, data, count);
}


bool bt::EWMH::getProperty(Window target, Atom type, Atom property,
                            unsigned char** data) const {
  Atom atom_return;
  int size;
  unsigned long nitems, bytes_left;

  int ret = XGetWindowProperty(display.XDisplay(), target, property,
                               0l, 1l, false,
                               type, &atom_return, &size,
                               &nitems, &bytes_left, data);
  if (ret != Success || nitems != 1)
    return false;

  return true;
}


bool bt::EWMH::getListProperty(Window target, Atom type, Atom property,
                                unsigned char** data,
                                unsigned long* count) const {
  Atom atom_return;
  int size;
  unsigned long nitems, bytes_left;

  int ret = XGetWindowProperty(display.XDisplay(), target, property,
                               0l, 1l, false,
                               type, &atom_return, &size,
                               &nitems, &bytes_left, data);
  if (ret != Success || nitems < 1)
    return false;

  if (bytes_left != 0) {
    XFree(*data);
    unsigned long remain = ((size / 8) * nitems) + bytes_left;
    ret = XGetWindowProperty(display.XDisplay(), target,
                             property, 0l, remain, false,
                             type, &atom_return, &size,
                             &nitems, &bytes_left, data);
    if (ret != Success)
      return false;
  }

  *count = nitems;
  return true;
}


bool bt::EWMH::isSupportedWMWindowType(Atom atom) const {
  /*
    the implementation looks silly I know.  You are probably thinking
    "why not just do:
    return (atom > net_wm_window_type && atom <= net_wm_window_type_normal)?".
    Well the problem is it assumes the atoms were created in a contiguous
    range. This happens to be true if we created them but could be false if we
    were started after some EWMH app which allocated the windows in an odd
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
