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
#include "window.hh"
#include "icon.hh"
#include "workspace.hh"


BlackboxIcon::BlackboxIcon(BlackboxSession *s, Window c) {
  debug = new Debugger('$');
#ifdef DEBUG
  debug->enable();
#endif
  debug->msg("%s:%d: BlackboxIcon::BlackboxIcon\n", __FILE__, __LINE__);

  session = s;
  display = session->control();
  ws_manager = session->WSManager();
  icon.client = c;
  focus = False;

  if (! XGetIconName(display, icon.client, &icon.name))
    icon.name = 0;

  icon.height = session->iconFont()->ascent + session->iconFont()->descent + 4;

  unsigned long attrib_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWCursor|CWEventMask; 
  XSetWindowAttributes attrib;
  attrib.background_pixmap = None;
  attrib.background_pixel = session->frameColor().pixel;
  attrib.border_pixel = 0l;
  attrib.cursor = session->sessionCursor();
  attrib.event_mask = StructureNotifyMask|SubstructureNotifyMask|
    ExposureMask|SubstructureRedirectMask|ButtonPressMask|
    ButtonReleaseMask|ButtonMotionMask|ExposureMask;
  
  icon.window =
    XCreateWindow(display, session->WSManager()->iconWindowID(), 1, 1,
		  ws_manager->iconWidth() - 2, icon.height, 0,
		  session->Depth(), InputOutput, session->visual(),
		  attrib_mask, &attrib);
  XSaveContext(display, icon.window, session->iconContext(),
	       (XPointer) this);

  XGCValues gcv;
  gcv.foreground = session->iconTextColor().pixel;
  gcv.font = session->iconFont()->fid;
  iconGC = XCreateGC(display, icon.window, GCForeground|GCFont, &gcv);

  attrib.event_mask |= EnterWindowMask|LeaveWindowMask;
  attrib.background_pixel = session->frameColor().pixel;
  icon.subwindow =
    XCreateWindow(display, icon.window, 0, 0, ws_manager->iconWidth() - 2,
		  icon.height, 0, session->Depth(), InputOutput,
		  session->visual(), attrib_mask, &attrib);
  XSaveContext(display, icon.subwindow, session->iconContext(),
	       (XPointer) this);
  
  BImage image(session, ws_manager->iconWidth() - 2, icon.height,
	       session->Depth(), session->frameColor());
  Pixmap p = image.renderImage(session->toolboxTexture(), 0,
			       session->toolboxColor(),
			       session->toolboxToColor());

  XSetWindowBackgroundPixmap(display, icon.subwindow, p);
  XClearWindow(display, icon.subwindow);
  XFreePixmap(display, p);

  XMapSubwindows(display, icon.window);
  XMapRaised(display, icon.window);
  ws_manager->addIcon(this);
}


BlackboxIcon::~BlackboxIcon(void) {
  debug->msg("%s:%d: BlackboxIcon::~BlackboxIcon\n", __FILE__, __LINE__);
  XUnmapWindow(display, icon.window);

  ws_manager->removeIcon(this);
  ws_manager->arrangeIcons();

  XFreeGC(display, iconGC);
  if (icon.name) XFree(icon.name);

  XDeleteContext(display, icon.window, session->iconContext());
  XDeleteContext(display, icon.subwindow, session->iconContext());
  XDestroyWindow(display, icon.subwindow);
  XDestroyWindow(display, icon.window);

  delete debug;
}


void BlackboxIcon::buttonPressEvent(XButtonEvent *) {
  debug->msg("%s:%d: BlackboxIcon::buttonPressEvent\n", __FILE__, __LINE__);
  if (session->button1Pressed())
    XRaiseWindow(display, icon.window);
}


void BlackboxIcon::buttonReleaseEvent(XButtonEvent *be) {
  debug->msg("%s:%d: BlackboxIcon::buttonReleaseEvent\n", __FILE__, __LINE__);
  if (be->button == 1 &&
      be->x >= 0 && be->x <= (signed) (ws_manager->iconWidth() - 2) &&
      be->y >= 0 && be->y <= (signed) icon.height) {
    XGrabServer(display);
    BlackboxWindow *win = session->getWindow(icon.client);
    XUnmapWindow(display, icon.window);
    XDrawString(display, session->Root(), session->GCOperations(), icon.x + 2, 
                icon.y + 2 + session->iconFont()->ascent,
                ((icon.name != NULL) ? icon.name : "Unnamed"),
                ((icon.name != NULL) ? strlen(icon.name) : strlen("Unnamed")));
    XClearWindow(display, session->WSManager()->iconWindowID());
    XUngrabServer(display);
    win->deiconifyWindow();
    win->setFocusFlag(False);
  }
}


void BlackboxIcon::enterNotifyEvent(XCrossingEvent *) {
  debug->msg("%s:%d: BlackboxIcon::enterNotifyEvent\n", __FILE__, __LINE__);
  focus = True;
  XClearWindow(display, icon.subwindow);
  XDrawString(display, session->Root(), session->GCOperations(), icon.x + 2,
              icon.y + 2 + session->iconFont()->ascent,
	      ((icon.name != NULL) ? icon.name : "Unnamed"),
	      ((icon.name != NULL) ? strlen(icon.name) : strlen("Unnamed")));
}


void BlackboxIcon::leaveNotifyEvent(XCrossingEvent *) {
  debug->msg("%s:%d: BlackboxIcon::leaveNotifyEvent\n", __FILE__, __LINE__);
  focus = False;
  XDrawString(display, session->Root(), session->GCOperations(), icon.x + 2, 
              icon.y + 2 + session->iconFont()->ascent,
              ((icon.name != NULL) ? icon.name : "Unnamed"),
              ((icon.name != NULL) ? strlen(icon.name) : strlen("Unnamed")));
  XClearWindow(display, icon.subwindow);
  exposeEvent(NULL);
}


void BlackboxIcon::exposeEvent(XExposeEvent *) {
  debug->msg("%s:%d: BlackboxIcon::exposeEvent\n", __FILE__, __LINE__);
  if (! focus) {
    int w = ((icon.name != NULL) ?
      XTextWidth(session->iconFont(), icon.name, strlen(icon.name)) :
      XTextWidth(session->iconFont(), "Unnamed", strlen("Unnamed"))) + 4;

    if (w > (signed) (ws_manager->iconWidth() - 2)) {
      int il;
      for (il = ((icon.name != NULL) ? strlen(icon.name) : strlen("Unnamed"));
       	   il > 0; --il) {
        if ((w = ((icon.name != NULL) ?
                  XTextWidth(session->iconFont(), icon.name, il) :
                  XTextWidth(session->iconFont(), "Unnamed", il)) + 4) < 90)
          break;
      }

      XDrawString(display, icon.subwindow, iconGC, 2, 2 +
                  session->iconFont()->ascent,
                  ((icon.name != NULL) ? icon.name : "Unnamed"), il);
    } else {
      XDrawString(display, icon.subwindow, iconGC, 2, 2 +
		  session->iconFont()->ascent,
		  ((icon.name != NULL) ? icon.name : "Unnamed"),
		  ((icon.name != NULL) ? strlen(icon.name) :
                   strlen("Unnamed")));
    }
  }
}


void BlackboxIcon::rereadLabel(void) {
  debug->msg("%s:%d: BlackboxIcon::rereadLabel\n", __FILE__, __LINE__);
  if (! XGetIconName(display, icon.client, &icon.name))
    icon.name = 0;
  XClearWindow(display, icon.subwindow);
  exposeEvent(NULL);
}


void BlackboxIcon::Reconfigure(void) {
  debug->msg("%s:%d: BlackboxIcon::Reconfigure\n", __FILE__, __LINE__);
  XGCValues gcv;
  gcv.foreground = session->iconTextColor().pixel;
  gcv.font = session->iconFont()->fid;
  XChangeGC(display, iconGC, GCForeground|GCFont, &gcv);

  icon.height = session->iconFont()->ascent + session->iconFont()->descent + 4;
  XResizeWindow(display, icon.window, ws_manager->iconWidth() - 2,
		icon.height);
  XResizeWindow(display, icon.subwindow, ws_manager->iconWidth() - 2,
		icon.height);
  BImage image(session, ws_manager->iconWidth() - 2, icon.height,
	       session->Depth(), session->frameColor());
  Pixmap p = image.renderImage(session->toolboxTexture(), 0,
			       session->toolboxColor(),
			       session->toolboxToColor());

  XSetWindowBackgroundPixmap(display, icon.subwindow, p);
  XClearWindow(display, icon.subwindow);
  XFreePixmap(display, p);
  exposeEvent(NULL);
}
