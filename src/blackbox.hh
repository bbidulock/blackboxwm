// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// blackbox.hh for Blackbox - an X11 Window manager
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

#ifndef   __blackbox_hh
#define   __blackbox_hh

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include <stdio.h>

#ifdef    TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else // !TIME_WITH_SYS_TIME
#  ifdef    HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else // !HAVE_SYS_TIME_H
#    include <time.h>
#  endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME
}

#include <list>
#include <map>
#include <string>

#include "Application.hh"

//forward declaration
class BScreen;
class BlackboxWindow;
class BWindowGroup;
class Basemenu;
class Toolbar;
class Slit;

namespace bt {
  class Netwm;
}

class Blackbox : public bt::Application, public bt::TimeoutHandler {
private:
  struct BCursor {
    Cursor session, move, ll_angle, lr_angle;
  };
  BCursor cursor;

  struct MenuTimestamp {
    std::string filename;
    time_t timestamp;
  };

  struct BResource {
    Time double_click_interval;

    std::string menu_file, style_file;
    timeval auto_raise_delay;
    unsigned long cache_life, cache_max;
  } resource;

  typedef std::map<Window, BlackboxWindow*> WindowLookup;
  typedef WindowLookup::value_type WindowLookupPair;
  WindowLookup windowSearchList;

  typedef std::map<Window, BWindowGroup*> GroupLookup;
  typedef GroupLookup::value_type GroupLookupPair;
  GroupLookup groupSearchList;

  typedef std::list<MenuTimestamp*> MenuTimestampList;
  MenuTimestampList menuTimestamps;

  BScreen** screen_list;
  size_t screen_list_count;
  BScreen *active_screen;

  BlackboxWindow *focused_window;
  bt::Timer *timer;

  bool no_focus, reconfigure_wait, reread_menu_wait;
  Time last_time;
  char **argv;
  std::string rc_file;

  Atom xa_wm_colormap_windows, xa_wm_protocols, xa_wm_state,
    xa_wm_delete_window, xa_wm_take_focus, xa_wm_change_state,
    motif_wm_hints;

  bt::Netwm* _netwm;

  Blackbox(const Blackbox&);
  Blackbox& operator=(const Blackbox&);

  void load_rc(void);
  void save_rc(void);
  void reload_rc(void);
  void real_rereadMenu(void);
  void real_reconfigure(void);

  void init_icccm(void);

  virtual void process_event(XEvent *e);


public:
  Blackbox(char **m_argv, const char *dpy_name, const char *rc,
           bool multi_head);
  virtual ~Blackbox(void);

  // screen functions
  BScreen *findScreen(Window window);
  BScreen *activeScreen(void) const { return active_screen; }
  void setActiveScreen(BScreen *screen);

  BlackboxWindow *findWindow(Window window);
  void insertWindow(Window window, BlackboxWindow *data);
  void removeWindow(Window window);

  BWindowGroup *findWindowGroup(Window window);
  void insertWindowGroup(Window window, BWindowGroup *data);
  void removeWindowGroup(Window window);

  inline const bt::Netwm& netwm(void) { return *_netwm; }

  inline BlackboxWindow *getFocusedWindow(void) { return focused_window; }

  inline const Time &doubleClickInterval(void) const
  { return resource.double_click_interval; }
  inline const Time &getLastTime(void) const { return last_time; }

  const std::string& getStyleFilename(void) const
  { return resource.style_file; }
  inline const char *getMenuFilename(void) const
    { return resource.menu_file.c_str(); }

  inline const timeval &getAutoRaiseDelay(void) const
    { return resource.auto_raise_delay; }

  inline unsigned long getCacheLife(void) const
    { return resource.cache_life; }
  inline unsigned long getCacheMax(void) const
    { return resource.cache_max; }

  inline void setNoFocus(bool f) { no_focus = f; }

  inline Cursor getSessionCursor(void) const
    { return cursor.session; }
  inline Cursor getMoveCursor(void) const
    { return cursor.move; }
  inline Cursor getLowerLeftAngleCursor(void) const
    { return cursor.ll_angle; }
  inline Cursor getLowerRightAngleCursor(void) const
    { return cursor.lr_angle; }

  void setFocusedWindow(BlackboxWindow *w);
  void shutdown(void);
  void load_rc(BScreen *screen);
  void saveStyleFilename(const std::string& filename);
  void saveMenuFilename(const std::string& filename);
  void restart(const std::string &prog = std::string());
  void reconfigure(void);
  void rereadMenu(void);
  void checkMenu(void);

  bool validateWindow(Window window);

  virtual bool handleSignal(int sig);

  virtual void timeout(bt::Timer *);

  inline Atom getWMChangeStateAtom(void) const
    { return xa_wm_change_state; }
  inline Atom getWMStateAtom(void) const
    { return xa_wm_state; }
  inline Atom getWMDeleteAtom(void) const
    { return xa_wm_delete_window; }
  inline Atom getWMProtocolsAtom(void) const
    { return xa_wm_protocols; }
  inline Atom getWMTakeFocusAtom(void) const
    { return xa_wm_take_focus; }
  inline Atom getWMColormapAtom(void) const
    { return xa_wm_colormap_windows; }
  inline Atom getMotifWMHintsAtom(void) const
    { return motif_wm_hints; }
};


#endif // __blackbox_hh
