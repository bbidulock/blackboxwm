// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Netwm.hh for Blackbox - an X11 Window manager
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

#ifndef __Netwm_hh
#define __Netwm_hh

#include "Util.hh"

#include <string>
#include <vector>

namespace bt {

  class Netwm: public NoCopy {
  public:
    explicit Netwm(::Display *_display);

    typedef std::vector<Atom> AtomList;
    typedef std::vector<Window> WindowList;
    typedef std::vector<std::string> UTF8StringList;

    struct Strut {
      unsigned int left, right, top, bottom;

      Strut(void): left(0), right(0), top(0), bottom(0) {}
    };

    inline Atom utf8String(void) const { return utf8_string; }

    // root window properties
    inline Atom supported(void) const { return net_supported; }
    inline Atom clientList(void) const { return net_client_list; }
    inline Atom clientListStacking(void) const
    { return net_client_list_stacking; }
    inline Atom numberOfDesktops(void) const { return net_number_of_desktops; }
    inline Atom desktopGeometry(void) const { return net_desktop_geometry; }
    inline Atom currentDesktop(void) const { return net_current_desktop; }
    inline Atom desktopNames(void) const { return net_desktop_names; }
    inline Atom activeWindow(void) const { return net_active_window; }
    inline Atom workarea(void) const { return net_workarea; }
    inline Atom supportingWMCheck(void) const
    { return net_supporting_wm_check; }

    void setSupported(Window target, Atom atoms[], unsigned int count) const;
    bool readSupported(Window target, AtomList& atoms) const;
    void setClientList(Window target, WindowList& windows) const;
    bool readClientList(Window target, WindowList& windows) const;
    void setClientListStacking(Window target, WindowList& windows) const;
    bool readClientListStacking(Window target, WindowList& windows) const;
    void setNumberOfDesktops(Window target, unsigned int count) const;
    bool readNumberOfDesktops(Window target, unsigned int* number) const;
    void setDesktopGeometry(Window target,
                            unsigned int width, unsigned int height) const;
    bool readDesktopGeometry(Window target,
                             unsigned int* width, unsigned int* height) const;
    void setCurrentDesktop(Window target, unsigned int number) const;
    bool readCurrentDesktop(Window target, unsigned int* number) const;
    void setDesktopNames(Window target, const std::string& names) const;
    bool readDesktopNames(Window target, UTF8StringList& names) const;
    void setActiveWindow(Window target, Window data) const;
    void setWorkarea(Window target, unsigned long workareas[],
                     unsigned int count) const;
    void setSupportingWMCheck(Window target, Window data) const;
    bool readSupportingWMCheck(Window target, Window* window) const;

    // other root messages
    inline Atom closeWindow(void) const { return net_close_window; }
    inline Atom moveresizeWindow(void) const { return net_moveresize_window; }

    // application properties
    inline Atom wmName(void) const { return net_wm_name; }
    inline Atom wmVisibleName(void) const { return net_wm_visible_name; }
    inline Atom wmIconName(void) const { return net_wm_icon_name; }
    inline Atom wmVisibleIconName(void) const
    { return net_wm_visible_icon_name; }
    inline Atom wmDesktop(void) const { return net_wm_desktop; }
    inline Atom wmWindowType(void) const { return net_wm_window_type; }
    inline Atom wmWindowTypeDesktop(void) const
    { return net_wm_window_type_desktop; }
    inline Atom wmWindowTypeDock(void) const
    { return net_wm_window_type_dock; }
    inline Atom wmWindowTypeToolbar(void) const
    { return net_wm_window_type_toolbar; }
    inline Atom wmWindowTypeMenu(void) const
    { return net_wm_window_type_menu; }
    inline Atom wmWindowTypeUtility(void) const
    { return net_wm_window_type_utility; }
    inline Atom wmWindowTypeSplash(void) const
    { return net_wm_window_type_splash; }
    inline Atom wmWindowTypeDialog(void) const
    { return net_wm_window_type_dialog; }
    inline Atom wmWindowTypeNormal(void) const
    { return net_wm_window_type_normal; }
    inline Atom wmState(void) const { return net_wm_state; }
    inline Atom wmStateModal(void) const { return net_wm_state_modal; }
    inline Atom wmStateSticky(void) const { return net_wm_state_sticky; }
    inline Atom wmStateMaximizedVert(void) const
    { return net_wm_state_maximized_vert; }
    inline Atom wmStateMaximizedHorz(void) const
    { return net_wm_state_maximized_horz; }
    inline Atom wmStateShaded(void) const { return net_wm_state_shaded; }
    inline Atom wmStateSkipTaskbar(void) const
    { return net_wm_state_skip_taskbar; }
    inline Atom wmStateSkipPager(void) const
    { return net_wm_state_skip_pager; }
    inline Atom wmStateHidden(void) const { return net_wm_state_hidden; }
    inline Atom wmStateFullscreen(void) const
    { return net_wm_state_fullscreen; }
    inline Atom wmStateAbove(void) const { return net_wm_state_above; }
    inline Atom wmStateBelow(void) const { return net_wm_state_below; }
    inline Atom wmStateRemove(void) const { return 0; }
    inline Atom wmStateAdd(void) const { return 1; }
    inline Atom wmStateToggle(void) const { return 2; }
    inline Atom wmAllowedActions(void) const { return net_wm_allowed_actions; }
    inline Atom wmActionMove(void) const { return net_wm_action_move; }
    inline Atom wmActionResize(void) const
    { return net_wm_action_resize; }
    inline Atom wmActionMinimize(void) const
    { return net_wm_action_minimize; }
    inline Atom wmActionShade(void) const { return net_wm_action_shade; }
    inline Atom wmActionStick(void) const { return net_wm_action_stick; }
    inline Atom wmActionMaximizeHorz(void) const
    { return net_wm_action_maximize_horz; }
    inline Atom wmActionMaximizeVert(void) const
    { return net_wm_action_maximize_vert; }
    inline Atom wmActionFullscreen(void) const
    { return net_wm_action_fullscreen; }
    inline Atom wmActionChangeDesktop(void) const
    { return net_wm_action_change_desktop; }
    inline Atom wmActionClose(void) const { return net_wm_action_close; }
    inline Atom wmStrut(void) const { return net_wm_strut; }

    void setWMName(Window target, const std::string& name) const;
    bool readWMName(Window target, std::string& name) const;
    void setWMVisibleName(Window target, const std::string& name) const;
    bool readWMIconName(Window target, std::string& name) const;
    void setWMVisibleIconName(Window target, const std::string& name) const;
    void setWMDesktop(Window target, unsigned int desktop) const;
    bool readWMDesktop(Window target, unsigned int& desktop) const;
    bool readWMWindowType(Window target, AtomList& types) const;
    void setWMState(Window target, AtomList& atoms) const;
    bool readWMState(Window target, AtomList& states) const;
    void setWMAllowedActions(Window target, AtomList& atoms) const;
    bool readWMStrut(Window target, Strut* strut) const;

    // utility
    void removeProperty(Window target, Atom atom) const;
    bool isSupportedWMWindowType(Atom atom) const;

    bool getProperty(Window target, Atom type, Atom property,
                     unsigned char** data) const;
    bool getListProperty(Window target, Atom type, Atom property,
                         unsigned char** data, unsigned long* count) const;

  private:
    ::Display *display;
    Atom utf8_string,
      net_supported, net_client_list, net_client_list_stacking,
      net_number_of_desktops, net_desktop_geometry, net_current_desktop,
      net_desktop_names, net_active_window, net_workarea,
      net_supporting_wm_check, net_close_window, net_moveresize_window,
      net_wm_name, net_wm_visible_name,
      net_wm_icon_name, net_wm_visible_icon_name,
      net_wm_desktop, net_wm_window_type,
      net_wm_window_type_desktop, net_wm_window_type_dock,
      net_wm_window_type_toolbar, net_wm_window_type_menu,
      net_wm_window_type_utility, net_wm_window_type_splash,
      net_wm_window_type_dialog, net_wm_window_type_normal, net_wm_state,
      net_wm_state_modal, net_wm_state_sticky, net_wm_state_maximized_vert,
      net_wm_state_maximized_horz, net_wm_state_shaded,
      net_wm_state_skip_taskbar, net_wm_state_skip_pager, net_wm_state_hidden,
      net_wm_state_fullscreen, net_wm_state_above, net_wm_state_below,
      net_wm_state_remove, net_wm_state_add, net_wm_state_toggle,
      net_wm_allowed_actions, net_wm_action_move, net_wm_action_resize,
      net_wm_action_minimize, net_wm_action_shade, net_wm_action_stick,
      net_wm_action_maximize_horz, net_wm_action_maximize_vert,
      net_wm_action_fullscreen, net_wm_action_change_desktop,
      net_wm_action_close, net_wm_strut;
  };

} // namespace bt

#endif // __Netwm_hh
