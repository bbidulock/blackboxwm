// -*- mode: C++; indent-tabs-mode: nil; -*-
// blackbox.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#ifdef    HAVE_STDIO_H
# include <stdio.h>
#endif // HAVE_STDIO_H

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

#include <map>

#include "i18n.hh"
#include "BaseDisplay.hh"
#include "Timer.hh"

//forward declaration
class BScreen;
class Blackbox;
class BImageControl;
class BlackboxWindow;
class Basemenu;
class Toolbar;
class Slit;

extern I18n i18n;

class Blackbox : public BaseDisplay, public TimeoutHandler {
private:
  struct MenuTimestamp {
    char *filename;
    time_t timestamp;

    MenuTimestamp(void): filename(0) {}
    ~MenuTimestamp(void) { delete [] filename; }
  };

  struct resource {
    Time double_click_interval;

    char *menu_file, *style_file;
    int colors_per_channel;
    timeval auto_raise_delay;
    unsigned long cache_life, cache_max;
  } resource;

  typedef std::map<Window, BlackboxWindow*> WindowLookup;
  WindowLookup windowSearchList, groupSearchList;

  typedef std::map<Window, Basemenu*> MenuLookup;
  MenuLookup menuSearchList;

  typedef std::map<Window, Toolbar*> ToolbarLookup;
  ToolbarLookup toolbarSearchList;

  typedef std::map<Window, Slit*> SlitLookup;
  SlitLookup slitSearchList;

  typedef std::list<MenuTimestamp*> MenuTimestampList;
  MenuTimestampList menuTimestamps;

  typedef std::list<BScreen*> ScreenList;
  ScreenList screenList;

  BlackboxWindow *focused_window, *masked_window;
  BTimer *timer;

#ifdef    HAVE_GETPID
  Atom blackbox_pid;
#endif // HAVE_GETPID

  Bool no_focus, reconfigure_wait, reread_menu_wait;
  Time last_time;
  Window masked;
  char *rc_file, **argv;
  int argc;


protected:
  void load_rc(void);
  void save_rc(void);
  void reload_rc(void);
  void real_rereadMenu(void);
  void real_reconfigure(void);

  virtual void process_event(XEvent *);


public:
  Blackbox(int, char **, char * = 0, char * = 0);
  virtual ~Blackbox(void);

#ifdef    HAVE_GETPID
  inline const Atom getBlackboxPidAtom(void) const { return blackbox_pid; }
#endif // HAVE_GETPID

  Basemenu *searchMenu(Window);

  BlackboxWindow *searchGroup(Window, BlackboxWindow *);
  BlackboxWindow *searchWindow(Window);
  inline BlackboxWindow *getFocusedWindow(void) { return focused_window; }

  BScreen *getScreen(int);
  BScreen *searchScreen(Window);

  inline const Time &getDoubleClickInterval(void) const
    { return resource.double_click_interval; }
  inline const Time &getLastTime(void) const { return last_time; }

  Toolbar *searchToolbar(Window);
  Slit *searchSlit(Window);

  inline const char *getStyleFilename(void) const
    { return resource.style_file; }
  inline const char *getMenuFilename(void) const
    { return resource.menu_file; }

  inline const int getColorsPerChannel(void) const
    { return resource.colors_per_channel; }

  inline const timeval &getAutoRaiseDelay(void) const
    { return resource.auto_raise_delay; }

  inline const unsigned long getCacheLife(void) const
    { return resource.cache_life; }
  inline const unsigned long getCacheMax(void) const
    { return resource.cache_max; }

  inline void maskWindowEvents(Window w, BlackboxWindow *bw)
    { masked = w; masked_window = bw; }
  inline void setNoFocus(Bool f) { no_focus = f; }

  void setFocusedWindow(BlackboxWindow *w);
  void shutdown(void);
  void load_rc(BScreen *);
  void saveStyleFilename(const char *);
  void saveMenuFilename(const char *);
  void saveMenuSearch(Window, Basemenu *);
  void saveWindowSearch(Window, BlackboxWindow *);
  void saveGroupSearch(Window, BlackboxWindow *);
  void saveToolbarSearch(Window, Toolbar *);
  void saveSlitSearch(Window, Slit *);
  void removeMenuSearch(Window);
  void removeWindowSearch(Window);
  void removeGroupSearch(Window);
  void removeToolbarSearch(Window);
  void removeSlitSearch(Window);
  void restart(const char * = 0);
  void reconfigure(void);
  void rereadMenu(void);
  void checkMenu(void);

  virtual Bool handleSignal(int);

  virtual void timeout(void);

#ifndef   HAVE_STRFTIME
  enum { B_AmericanDate = 1, B_EuropeanDate };
#endif // HAVE_STRFTIME
};


#endif // __blackbox_hh
