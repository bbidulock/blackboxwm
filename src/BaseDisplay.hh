// BaseDisplay.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 1999 by Brad Hughes, bhughes@tcac.net
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//  
// (See the included file COPYING / GPL-2.0)
//

#ifndef   __BaseDisplay_hh
#define   __BaseDisplay_hh

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "LinkedList.hh"

// forward declaration
class ScreenInfo;


class BaseDisplay {
private:
  struct cursor {
    Cursor session, move;
  } cursor;

  struct shape {
    Bool extensions;
    int event_basep, error_basep;
  } shape;

  Atom xa_wm_colormap_windows, xa_wm_protocols, xa_wm_state,
    xa_wm_delete_window, xa_wm_take_focus, xa_wm_change_state,
    motif_wm_hints;
  Bool _startup, _shutdown;
  Display *display;

  LinkedList<ScreenInfo> *screenInfoList;

  char *display_name;
  int number_of_screens, server_grabs, colors_per_channel;


protected:
  // pure virtual function... you must override this
  virtual void process_event(XEvent *) = 0;


public:
  BaseDisplay(char * = 0);
  virtual ~BaseDisplay(void);

  Atom getWMChangeStateAtom(void) { return xa_wm_change_state; }
  Atom getWMStateAtom(void) { return xa_wm_state; }
  Atom getWMDeleteAtom(void) { return xa_wm_delete_window; }
  Atom getWMProtocolsAtom(void) { return xa_wm_protocols; }
  Atom getWMFocusAtom(void) { return xa_wm_take_focus; }
  Atom getWMColormapAtom(void) { return xa_wm_colormap_windows; }
  Atom getMotifWMHintsAtom(void) { return motif_wm_hints; }

  ScreenInfo *getScreenInfo(int);

  Bool hasShapeExtensions(void) { return shape.extensions; }
  Bool doShutdown(void) { return _shutdown; }
  Bool isStartup(void) { return _startup; }
  Bool validateWindow(Window);

  Cursor getSessionCursor(void) { return cursor.session; }
  Cursor getMoveCursor(void) { return cursor.move; }

  Display *getDisplay(void) { return display; }

  const char *getDisplayName(void) const { return (const char *) display_name; }

  int getNumberOfScreens(void) { return number_of_screens; }
  int getShapeEventBase(void) { return shape.event_basep; }

  void shutdown(void) { _shutdown = True; }
  void grab(void);
  void ungrab(void);
  void eventLoop(void);
  void run(void) { _startup = _shutdown = False; }
};


class ScreenInfo {
private:
  BaseDisplay *display;
  Visual *visual;
  Window root_window;

  int depth, screen_number;
  unsigned int width, height;


protected:


public:
  ScreenInfo(BaseDisplay *, int);

  BaseDisplay *getDisplay(void) { return display; }
  Visual *getVisual(void) { return visual; }
  Window getRootWindow(void) { return root_window; }

  int getDepth(void) { return depth; }
  int getScreenNumber(void) { return screen_number; }

  unsigned int getWidth(void) { return width; }
  unsigned int getHeight(void) { return height; }
};


#endif // __BaseDisplay_hh

