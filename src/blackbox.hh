// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// blackbox.hh for Blackbox - an X11 Window manager
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

#ifndef   __blackbox_hh
#define   __blackbox_hh

#include <Application.hh>
#include <Util.hh>

extern "C" {
#include <X11/Xatom.h>
};

#include "BlackboxResource.hh"

#include <list>

// forward declarations
class BlackboxWindow;
class BWindowGroup;

namespace bt {
  class EWMH;
}

class Blackbox : public bt::Application, public bt::TimeoutHandler {
private:
  unsigned int grab_count;

  Window wm_selection_owner;

  struct MenuTimestamp {
    std::string filename;
    long timestamp;
  };

  BlackboxResource _resource;

  BScreen** screen_list;
  size_t screen_list_count;
  BScreen *active_screen;

  typedef std::map<Window, BlackboxWindow*> WindowLookup;
  typedef WindowLookup::value_type WindowLookupPair;
  WindowLookup windowSearchList;

  typedef std::map<Window, BWindowGroup*> GroupLookup;
  typedef GroupLookup::value_type GroupLookupPair;
  GroupLookup groupSearchList;

  bt::EWMH* _ewmh;

  BlackboxWindow *focused_window;

  bt::Timer *timer;

  typedef std::list<MenuTimestamp*> MenuTimestampList;
  MenuTimestampList menuTimestamps;

  char **argv;
  int argc;

  Atom xa_manager, motif_wm_hints, xa_sm_client_id, xa_wm_change_state,
       xa_wm_client_leader, xa_wm_colormap_notify, xa_wm_colormap_windows,
       xa_wm_delete_window, xa_wm_protocols, xa_wm_save_yourself,
       xa_wm_state, xa_wm_take_focus, xa_wm_window_role;

  void load_rc(void);
  void save_rc(void);
  void reload_rc(void);

  void init_icccm(void);

  void updateActiveWindow() const;

  // reimplemented virtual functions
  void shutdown(void);

  void process_event(XEvent *e);
  bool process_signal(int sig);

  void timeout(bt::Timer *);

public:
  Blackbox(char **m_argv, int argc, const char *dpy_name, const std::string& rc,
           bool multi_head);
  ~Blackbox(void);

  void XGrabServer(void);
  void XUngrabServer(void);

  char **argV(void) const { return argv; }
  int argC(void) const { return argc; }

  inline BlackboxResource &resource(void)
  { return _resource; }

  // screen functions
  BScreen *findScreen(Window window) const;

  inline BScreen *activeScreen(void) const
  { return active_screen; }
  void setActiveScreen(BScreen *screen);

  inline unsigned int screenCount(void) const
  { return screen_list_count; }
  BScreen* screenNumber(unsigned int n);

  // window functions
  BlackboxWindow *findWindow(Window window) const;

  void insertWindow(Window window, BlackboxWindow *data);
  void removeWindow(Window window);

  // window group functions
  BWindowGroup *findWindowGroup(Window window) const;

  void insertWindowGroup(Window window, BWindowGroup *data);
  void removeWindowGroup(Window window);

  inline const bt::EWMH &ewmh(void) const
  { return *_ewmh; }

  void setFocusedWindow(BlackboxWindow *win);
  inline void forgetFocusedWindow(void)
  { focused_window = 0; }
  inline BlackboxWindow *focusedWindow(void) const
  { return focused_window; }

  void saveMenuFilename(const std::string& filename);
  void restart(const std::string &prog = std::string());
  void reconfigure(void);

  void checkMenu(void);
  void rereadMenu(void);

  // predefined by the X-Server
  inline Atom wmCommandAtom(void) const
  { return XA_WM_COMMAND; }
  inline Atom wmHintsAtom(void) const
  { return XA_WM_HINTS; }
  inline Atom wmClientMachineAtom(void) const
  { return XA_WM_CLIENT_MACHINE; }
  inline Atom wmIconNameAtom(void) const
  { return XA_WM_ICON_NAME; }
  inline Atom wmIconSizeAtom(void) const
  { return XA_WM_ICON_SIZE; }
  inline Atom wmNameAtom(void) const
  { return XA_WM_NAME; }
  inline Atom wmNormalHintsAtom(void) const
  { return XA_WM_NORMAL_HINTS; }
  inline Atom wmSizeHintsAtom(void) const
  { return XA_WM_SIZE_HINTS; }
  inline Atom wmZoomHintsAtom(void) const
  { return XA_WM_ZOOM_HINTS; }
  inline Atom wmClassAtom(void) const
  { return XA_WM_CLASS; }
  inline Atom wmTransientForAtom(void) const
  { return XA_WM_TRANSIENT_FOR; }

  // interned by us
  inline Atom managerAtom(void) const
  { return xa_manager; }
  inline Atom motifWmHintsAtom(void) const
  { return motif_wm_hints; }
  inline Atom smClientID(void) const
  { return xa_sm_client_id; }
  inline Atom wmChangeStateAtom(void) const
  { return xa_wm_change_state; }
  inline Atom wmClientLeaderAtom(void) const
  { return xa_wm_client_leader; }
  inline Atom wmColormapNotifyAtom(void) const
  { return xa_wm_colormap_notify; }
  inline Atom wmColormapAtom(void) const
  { return xa_wm_colormap_windows; }
  inline Atom wmDeleteWindowAtom(void) const
  { return xa_wm_delete_window; }
  inline Atom wmProtocolsAtom(void) const
  { return xa_wm_protocols; }
  inline Atom wmSaveYourselfAtom(void) const
  { return xa_wm_save_yourself; }
  inline Atom wmStateAtom(void) const
  { return xa_wm_state; }
  inline Atom wmTakeFocusAtom(void) const
  { return xa_wm_take_focus; }
  inline Atom wmWindowRoleAtom(void) const
  { return xa_wm_window_role; }
};

#endif // __blackbox_hh
