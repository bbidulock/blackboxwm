//
// icon.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@arn.net
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

#define _GNU_SOURCE
#include "blackbox.hh"
#include "window.hh"
#include "icon.hh"
#include "Workspace.hh"


// *************************************************************************
// Icon class code
// *************************************************************************
//
// allocations:
// XPointer - icon.name
// Window icon.window, icon.subwindow
// GC iconGC
//
// *************************************************************************

BlackboxIcon::BlackboxIcon(Blackbox *bb, Window c) {
  blackbox = bb;
  display = blackbox->control();
  wsManager = blackbox->workspaceManager();
  icon.client = c;
  focus = False;

  if (! XGetIconName(display, icon.client, &icon.name))
    icon.name = 0;

  icon.height = blackbox->iconFont()->ascent + blackbox->iconFont()->descent + 4;

  unsigned long attrib_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWCursor|CWEventMask; 
  XSetWindowAttributes attrib;
  attrib.background_pixmap = None;
  attrib.background_pixel = blackbox->frameColor().pixel;
  attrib.border_pixel = 0l;
  attrib.cursor = blackbox->sessionCursor();
  attrib.event_mask = StructureNotifyMask|SubstructureNotifyMask|
    ExposureMask|SubstructureRedirectMask|ButtonPressMask|
    ButtonReleaseMask|ButtonMotionMask|ExposureMask;
  
  icon.window =
    XCreateWindow(display, blackbox->Root(), 1, 1,
		  128, icon.height, 0,
		  blackbox->Depth(), InputOutput, blackbox->visual(),
		  attrib_mask, &attrib);
  blackbox->saveIconSearch(icon.window, this);

  XGCValues gcv;
  gcv.foreground = blackbox->iconTextColor().pixel;
  gcv.font = blackbox->iconFont()->fid;
  iconGC = XCreateGC(display, icon.window, GCForeground|GCFont, &gcv);

  attrib.event_mask |= EnterWindowMask|LeaveWindowMask;
  attrib.background_pixel = blackbox->frameColor().pixel;
  icon.subwindow =
    XCreateWindow(display, icon.window, 0, 0, 128,
		  icon.height, 0, blackbox->Depth(), InputOutput,
		  blackbox->visual(), attrib_mask, &attrib);
  blackbox->saveIconSearch(icon.subwindow, this);
  
  BImage *i_image = new BImage(blackbox, 128, icon.height, blackbox->Depth());
  Pixmap p = i_image->renderImage(blackbox->toolboxTexture(),
				  blackbox->toolboxColor(),
				  blackbox->toolboxToColor());
  delete i_image;
  XSetWindowBackgroundPixmap(display, icon.subwindow, p);
  XClearWindow(display, icon.subwindow);
  XFreePixmap(display, p);

  XMapSubwindows(display, icon.window);
  XMapRaised(display, icon.window);
  wsManager->addIcon(this);
}


BlackboxIcon::~BlackboxIcon(void) {
  XUnmapWindow(display, icon.window);

  wsManager->removeIcon(this);
  wsManager->arrangeIcons();

  XFreeGC(display, iconGC);
  if (icon.name) XFree(icon.name);

  blackbox->removeIconSearch(icon.window);
  blackbox->removeIconSearch(icon.subwindow);
  XDestroyWindow(display, icon.subwindow);
  XDestroyWindow(display, icon.window);
}


void BlackboxIcon::buttonPressEvent(XButtonEvent *) { }


void BlackboxIcon::buttonReleaseEvent(XButtonEvent *be) {
  if (be->button == 1 &&
      be->x >= 0 && be->x <= (signed) (128) &&
      be->y >= 0 && be->y <= (signed) icon.height) {
    BlackboxWindow *win = blackbox->searchWindow(icon.client);
    XUnmapWindow(display, icon.window);
    XDrawString(display, blackbox->Root(), blackbox->GCOperations(), icon.x + 2, 
                icon.y + 2 + blackbox->iconFont()->ascent,
                ((icon.name != NULL) ? icon.name : "Unnamed"),
                ((icon.name != NULL) ? strlen(icon.name) :
		 strlen("Unnamed")));
    win->deiconifyWindow();
    win->setFocusFlag(False);
  }
}


void BlackboxIcon::enterNotifyEvent(XCrossingEvent *) {
  focus = True;
  XClearWindow(display, icon.subwindow);
  XDrawString(display, blackbox->Root(), blackbox->GCOperations(), icon.x + 2,
              icon.y + 2 + blackbox->iconFont()->ascent,
	      ((icon.name != NULL) ? icon.name : "Unnamed"),
	      ((icon.name != NULL) ? strlen(icon.name) : strlen("Unnamed")));
}


void BlackboxIcon::leaveNotifyEvent(XCrossingEvent *) {
  focus = False;
  XDrawString(display, blackbox->Root(), blackbox->GCOperations(), icon.x + 2, 
              icon.y + 2 + blackbox->iconFont()->ascent,
              ((icon.name != NULL) ? icon.name : "Unnamed"),
              ((icon.name != NULL) ? strlen(icon.name) : strlen("Unnamed")));
  XClearWindow(display, icon.subwindow);
  exposeEvent(NULL);
}


void BlackboxIcon::exposeEvent(XExposeEvent *) {
  if (! focus) {
    int w = ((icon.name != NULL) ?
      XTextWidth(blackbox->iconFont(), icon.name, strlen(icon.name)) :
      XTextWidth(blackbox->iconFont(), "Unnamed", strlen("Unnamed"))) + 4;

    if (w > (signed) (128)) {
      int il;
      for (il = ((icon.name != NULL) ? strlen(icon.name) : strlen("Unnamed"));
       	   il > 0; --il) {
        if ((w = ((icon.name != NULL) ?
                  XTextWidth(blackbox->iconFont(), icon.name, il) :
                  XTextWidth(blackbox->iconFont(), "Unnamed", il)) + 4) < 90)
          break;
      }

      XDrawString(display, icon.subwindow, iconGC, 2, 2 +
                  blackbox->iconFont()->ascent,
                  ((icon.name != NULL) ? icon.name : "Unnamed"), il);
    } else {
      XDrawString(display, icon.subwindow, iconGC, 2, 2 +
		  blackbox->iconFont()->ascent,
		  ((icon.name != NULL) ? icon.name : "Unnamed"),
		  ((icon.name != NULL) ? strlen(icon.name) :
                   strlen("Unnamed")));
    }
  }
}


void BlackboxIcon::rereadLabel(void) {
  if (! XGetIconName(display, icon.client, &icon.name))
    icon.name = 0;
  XClearWindow(display, icon.subwindow);
  exposeEvent(NULL);
}


void BlackboxIcon::Reconfigure(void) {
  XGCValues gcv;
  gcv.foreground = blackbox->iconTextColor().pixel;
  gcv.font = blackbox->iconFont()->fid;
  XChangeGC(display, iconGC, GCForeground|GCFont, &gcv);

  icon.height = blackbox->iconFont()->ascent +
    blackbox->iconFont()->descent + 4;
  XResizeWindow(display, icon.window, 128,
		icon.height);
  XResizeWindow(display, icon.subwindow, 128,
		icon.height);
  BImage *i_image = new BImage(blackbox, 128, icon.height, blackbox->Depth());
  Pixmap p = i_image->renderImage(blackbox->toolboxTexture(),
				  blackbox->toolboxColor(),
				  blackbox->toolboxToColor());
  delete i_image;
  XSetWindowBackgroundPixmap(display, icon.subwindow, p);
  XClearWindow(display, icon.subwindow);
  XFreePixmap(display, p);
  exposeEvent(NULL);
}
