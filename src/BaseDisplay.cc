// BaseDisplay.cc for Blackbox - an X11 Window manager
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

#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#ifdef    SHAPE
#  include <X11/extensions/shape.h>
#endif // SHAPE

#include "BaseDisplay.hh"

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    HAVE_FCNTL_H
#  include <fcntl.h>
#endif // HAVE_FCNTL_H


BaseDisplay::BaseDisplay(char *dpy_name) {
  _startup = True;
  _shutdown = False;
  server_grabs = 0;

  if (! (display = XOpenDisplay(dpy_name))) {
    fprintf(stderr,
            "BaseDisplay::BaseDisplay: connection to X server failed.\n");
    ::exit(2);
  } else if (fcntl(ConnectionNumber(display), F_SETFD, 1) == -1) {
    fprintf(stderr,
            "BaseDisplay::BaseDisplay: couldn't mark display connection "
            "as close-on-exec\n");
    ::exit(2);
  }

  number_of_screens = ScreenCount(display);
  display_name = XDisplayName(dpy_name);

#ifdef    SHAPE
  shape.extensions = XShapeQueryExtension(display, &shape.event_basep,
                                          &shape.error_basep);
#else // !SHAPE
  shape.extensions = False;
#endif // SHAPE
  
  xa_wm_colormap_windows =
    XInternAtom(display, "WM_COLORMAP_WINDOWS", False);
  xa_wm_protocols = XInternAtom(display, "WM_PROTOCOLS", False);
  xa_wm_state = XInternAtom(display, "WM_STATE", False);
  xa_wm_change_state = XInternAtom(display, "WM_CHANGE_STATE", False);
  xa_wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
  xa_wm_take_focus = XInternAtom(display, "WM_TAKE_FOCUS", False);
  motif_wm_hints = XInternAtom(display, "_MOTIF_WM_HINTS", False);
  
  cursor.session = XCreateFontCursor(display, XC_left_ptr);
  cursor.move = XCreateFontCursor(display, XC_fleur);

  screenInfoList = new LinkedList<ScreenInfo>;
  int i;
  for (i = 0; i < number_of_screens; i++) {
    ScreenInfo *screeninfo = new ScreenInfo(this, i);
    screenInfoList->insert(screeninfo);
  } 
}


BaseDisplay::~BaseDisplay(void) {
  while (screenInfoList->count())
    delete screenInfoList->remove(0);

  delete screenInfoList;

  XCloseDisplay(display);
}


void BaseDisplay::eventLoop(void) {
  run();

  while (! _shutdown) {
    XEvent e;
    XNextEvent(display, &e);
    process_event(&e);
  }
}


Bool BaseDisplay::validateWindow(Window window) {  
  XEvent event;
  if (XCheckTypedWindowEvent(display, window, DestroyNotify, &event)) {
    XPutBackEvent(display, &event);

    return False;
  } 

  return True;
}


void BaseDisplay::grab(void) {
  if (! server_grabs++)
    XGrabServer(display);

  XSync(display, False);
}


void BaseDisplay::ungrab(void) {
  if (! --server_grabs)
    XUngrabServer(display);

  if (server_grabs < 0) server_grabs = 0;
}


ScreenInfo::ScreenInfo(BaseDisplay *d, int num) {
  display = d;
  screen_number = num;

  root_window = RootWindow(display->getDisplay(), screen_number);
  visual = DefaultVisual(display->getDisplay(), screen_number);
  depth = DefaultDepth(display->getDisplay(), screen_number);

  width =
    WidthOfScreen(ScreenOfDisplay(display->getDisplay(), screen_number));
  height =
    HeightOfScreen(ScreenOfDisplay(display->getDisplay(), screen_number));
}

