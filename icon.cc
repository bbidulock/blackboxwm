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

/*

  Resources allocated for each icon:

  Debugger - debug
  Window - icon window, label (no context), subwindow
  GC - iconGC
  XPointer - icon name,
*/

BlackboxIcon::BlackboxIcon(BlackboxSession *ctrl, Window c) {
  session = ctrl;
  display = session->control();

  debug = new Debugger('$');
#ifdef DEBUG
  debug->enable();
#endif
  debug->enter("BlackboxIcon::BlackboxIcon\n");

  icon.client = c;
  focus = False;

  if (! XGetIconName(display, icon.client, &icon.name))
    icon.name = 0;

  int namewidth = (icon.name != NULL) ? 
    XTextWidth(session->iconFont(), icon.name, strlen(icon.name)) :
    XTextWidth(session->iconFont(), "Unnamed", strlen("Unnamed"));
  icon.width = (namewidth + 4 < 90) ? 90 : namewidth + 4;
  icon.height = session->iconFont()->ascent +
    session->iconFont()->descent + 4;

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
    XCreateWindow(display, session->WSManager()->iconWindowID(), 1, 1, 92,
		  icon.height, 0, session->Depth(), InputOutput,
		  session->visual(), attrib_mask, &attrib);
  XSaveContext(display, icon.window, session->iconContext(),
	       (XPointer) this);
  debug->msg("creating icon window %lx %lx\n", icon.window, icon.client);

  XGCValues gcv;
  gcv.foreground = session->iconTextColor().pixel;
  gcv.font = session->iconFont()->fid;
  iconGC = XCreateGC(display, icon.window, GCForeground|GCFont, &gcv);
  icon.subwindow = None;

  attrib.event_mask |= EnterWindowMask|LeaveWindowMask;
  attrib.background_pixel = session->frameColor().pixel;
  icon.subwindow =
    XCreateWindow(display, icon.window, 0, 0, 92, icon.height, 0,
		  session->Depth(), InputOutput, session->visual(),
		  attrib_mask, &attrib);
  XSaveContext(display, icon.subwindow, session->iconContext(),
	       (XPointer) this);
  
  BImage image(session, 92, icon.height, session->Depth(),
	       session->frameColor());
  Pixmap p = image.renderImage(session->frameTexture(), 0,
			       session->frameColor(),
			       session->frameToColor());

  XSetWindowBackgroundPixmap(display, icon.subwindow, p);
  XClearWindow(display, icon.subwindow);
  XFreePixmap(display, p);

  debug->msg("mapping icon\n");
  XMapSubwindows(display, icon.window);
  XMapRaised(display, icon.window);
  session->addIcon(this);
}


/*

  Resources deallocated for each icon:

  Debugger - debug
  Window - icon window, label (no context), subwindow
  GC - iconGC
  XPointer - icon name,
*/

BlackboxIcon::~BlackboxIcon(void) {
  /*
    remove icon from root window
  */
  XUnmapWindow(display, icon.window);

  /*
    dissociate this icon from the controlling session
  */
  debug->msg("deleting icon\n");
  session->removeIcon(this);
  session->arrangeIcons();

  XFreeGC(display, iconGC);
  XDeleteContext(display, icon.window, session->iconContext());
  XDeleteContext(display, icon.subwindow, session->iconContext());
  if (icon.name) XFree(icon.name);
  if (icon.subwindow != None) XDestroyWindow(display, icon.subwindow);
  XDestroyWindow(display, icon.window);

  delete debug;
}


void BlackboxIcon::buttonPressEvent(XButtonEvent *) {
  debug->msg("icon pressed\n");
  if (session->button1Pressed())
    XRaiseWindow(display, icon.window);
}


void BlackboxIcon::buttonReleaseEvent(XButtonEvent *be) {
  debug->msg("icon released\n");
  if (be->button == 1 &&
      be->x >= 0 && be->x <= (signed) icon.width &&
      be->y >= 0 && be->y <= (signed) icon.height) {
    XGrabServer(display);
    debug->msg("deiconifying app\n");
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
  focus = True;
  XClearWindow(display, icon.subwindow);
  XDrawString(display, session->Root(), session->GCOperations(), icon.x + 2,
              icon.y + 2 + session->iconFont()->ascent,
	      ((icon.name != NULL) ? icon.name : "Unnamed"),
	      ((icon.name != NULL) ? strlen(icon.name) : strlen("Unnamed")));
}


void BlackboxIcon::leaveNotifyEvent(XCrossingEvent *) {
  focus = False;
  XDrawString(display, session->Root(), session->GCOperations(), icon.x + 2, 
              icon.y + 2 + session->iconFont()->ascent,
              ((icon.name != NULL) ? icon.name : "Unnamed"),
              ((icon.name != NULL) ? strlen(icon.name) : strlen("Unnamed")));
  XClearWindow(display, icon.subwindow);
  exposeEvent(NULL);
}


void BlackboxIcon::exposeEvent(XExposeEvent *) {
  if (! focus) {
    int w = ((icon.name != NULL) ?
      XTextWidth(session->iconFont(), icon.name, strlen(icon.name)) :
      XTextWidth(session->iconFont(), "Unnamed", strlen("Unnamed"))) + 4;

    if (w > 90) {
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
  if (! XGetIconName(display, icon.client, &icon.name))
    icon.name = 0;
  XClearWindow(display, icon.subwindow);
  exposeEvent(NULL);
}


void BlackboxIcon::Reconfigure(void) {
  XGCValues gcv;
  gcv.foreground = session->iconTextColor().pixel;
  gcv.font = session->iconFont()->fid;
  XChangeGC(display, iconGC, GCForeground|GCFont, &gcv);

  BImage image(session, 92, icon.height, session->Depth(),
	       session->frameColor());
  Pixmap p = image.renderImage(session->frameTexture(), 0,
			       session->frameColor(),
			       session->frameToColor());

  XSetWindowBackgroundPixmap(display, icon.subwindow, p);
  XClearWindow(display, icon.subwindow);
  XFreePixmap(display, p);
  exposeEvent(NULL);
}
