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
#include "Workspace.hh"
#include "WorkspaceManager.hh"

#include "blackbox.hh"
#include "graphics.hh"
#include "window.hh"
#include "icon.hh"


// *************************************************************************
// Icon class code
// *************************************************************************

BlackboxIcon::BlackboxIcon(Blackbox *bb, BlackboxWindow *win) {
  blackbox = bb;
  display = blackbox->control();
  window = win;
  wsManager = blackbox->workspaceManager();
  icon.client = window->clientWindow();
  icon.height = icon.width = icon.pixmap_w = icon.pixmap_h = icon.label_w =
    icon.label_h = 0;
  focus = False;

  if (! XGetIconName(display, icon.client, &icon.name))
    icon.name = 0;

  unsigned int depth_of_client_pixmap = 0;
  if (window->clientIconPixmap()) {
    Window rootReturn;
    int foo;
    unsigned int ufoo;
    
    XGetGeometry(display, window->clientIconPixmap(), &rootReturn, &foo, &foo,
		 &icon.pixmap_w, &icon.pixmap_h, &ufoo, &depth_of_client_pixmap);
  }
  
  icon.label_w = XTextWidth(blackbox->iconFont(),
                            ((icon.name) ? icon.name : "Unnamed"),
                            strlen(((icon.name) ? icon.name : "Unnamed")));
  icon.width = icon.pixmap_w + icon.label_w + 4;
  
  icon.label_h = blackbox->iconFont()->ascent +
    blackbox->iconFont()->descent + 4;
  icon.height = ((icon.pixmap_h < icon.label_h) ? icon.label_h : icon.pixmap_h);
  
  unsigned long attrib_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWCursor|CWEventMask; 
  XSetWindowAttributes attrib;
  attrib.background_pixmap = None;
  attrib.background_pixel = blackbox->toolboxColor().pixel;
  attrib.border_pixel = blackbox->frameColor().pixel;
  attrib.cursor = blackbox->sessionCursor();
  attrib.event_mask = StructureNotifyMask|SubstructureNotifyMask|
    ExposureMask|SubstructureRedirectMask|ButtonPressMask|
    ButtonReleaseMask|ButtonMotionMask|ExposureMask;
  icon.window =
    XCreateWindow(display, blackbox->Root(), 0, 0,
		  icon.width + 4, icon.height + 4, 0,
		  blackbox->Depth(), InputOutput, blackbox->visual(),
		  attrib_mask, &attrib);
  blackbox->saveIconSearch(icon.window, this);

  BImage *i_image = new BImage(blackbox, icon.width + 4, icon.height + 4,
			       blackbox->Depth());
  Pixmap p = i_image->renderImage(blackbox->toolboxTexture(),
				  blackbox->toolboxColor(),
				  blackbox->toolboxToColor());
  delete i_image;
  XSetWindowBackgroundPixmap(display, icon.window, p);
  if (p) XFreePixmap(display, p);
  
  XGCValues gcv;
  gcv.foreground = blackbox->iconTextColor().pixel;
  gcv.background = blackbox->toolboxColor().pixel;
  gcv.font = blackbox->iconFont()->fid;
  iconGC = XCreateGC(display, icon.window, GCForeground|GCBackground|GCFont,
    &gcv);
  
  if (window->clientIconPixmap()) {
    attrib.event_mask |= EnterWindowMask|LeaveWindowMask;
    icon.subwindow =
      XCreateWindow(display, icon.window, 1, 1, icon.pixmap_w,
		    icon.pixmap_h, 1, blackbox->Depth(), InputOutput,
		    blackbox->visual(), attrib_mask, &attrib);
    blackbox->saveIconSearch(icon.subwindow, this);
    
    Pixmap p = XCreatePixmap(display, icon.window, icon.width, icon.height,
			     blackbox->Depth());
    if ((signed) depth_of_client_pixmap == blackbox->Depth())
      XCopyArea(display, window->clientIconPixmap(), p, iconGC, 0, 0,
		icon.width, icon.height, 0, 0);
    else
      XCopyPlane(display, window->clientIconPixmap(), p, iconGC, 0, 0,
		 icon.width, icon.height, 0, 0, 1);
    
    XSetWindowBackgroundPixmap(display, icon.subwindow, p);
    if (p) XFreePixmap(display, p);
    XClearWindow(display, icon.subwindow);
  } else
    icon.subwindow = None;

  wsManager->addIcon(this);
  XMapSubwindows(display, icon.window);
  XMapWindow(display, icon.window);
  XDrawString(display, icon.window, iconGC, icon.pixmap_w + 5,
	      (icon.height + blackbox->iconFont()->ascent -
	       blackbox->iconFont()->descent) / 2,
              ((icon.name) ? icon.name : "Unnamed"),
              strlen(((icon.name) ? icon.name : "Unnamed")));
}


BlackboxIcon::~BlackboxIcon(void) {
  XUnmapWindow(display, icon.window);

  wsManager->removeIcon(this);
  wsManager->arrangeIcons();

  XFreeGC(display, iconGC);
  if (icon.name) XFree(icon.name);

  if (icon.subwindow != None) {
    blackbox->removeIconSearch(icon.subwindow);
    XDestroyWindow(display, icon.subwindow);
  }
  
  blackbox->removeIconSearch(icon.window);
  XDestroyWindow(display, icon.window);
}


void BlackboxIcon::buttonPressEvent(XButtonEvent *) { }


void BlackboxIcon::buttonReleaseEvent(XButtonEvent *be) {
  if (be->button == 1 &&
      be->x >= 0 && be->x <= (signed) icon.width + 4 &&
      be->y >= 0 && be->y <= (signed) icon.height + 4) {
    XUnmapWindow(display, icon.window);
    window->deiconifyWindow();
    window->setFocusFlag(False);
  }
}


void BlackboxIcon::exposeEvent(XExposeEvent *ee) {
  if (ee->window == icon.window)
    XDrawString(display, icon.window, iconGC, icon.pixmap_w + 5,
	       	(icon.height + blackbox->iconFont()->ascent -
		 blackbox->iconFont()->descent) / 2,
                ((icon.name) ? icon.name : "Unnamed"),
                strlen(((icon.name) ? icon.name : "Unnamed")));
}


void BlackboxIcon::rereadLabel(void) {
  if (! XGetIconName(display, icon.client, &icon.name))
    icon.name = 0;

  icon.label_w = XTextWidth(blackbox->iconFont(),
                            ((icon.name) ? icon.name : "Unnamed"),
                            strlen(((icon.name) ? icon.name : "Unnamed")));
  icon.width = icon.pixmap_w + icon.label_w + 4;
  icon.height = ((icon.pixmap_h < icon.label_h) ? icon.label_h : icon.pixmap_h);
  
  BImage *i_image = new BImage(blackbox, icon.width + 4, icon.height + 4,
			       blackbox->Depth());
  Pixmap p = i_image->renderImage(blackbox->toolboxTexture(),
				  blackbox->toolboxColor(),
				  blackbox->toolboxToColor());
  delete i_image;
  XSetWindowBackgroundPixmap(display, icon.window, p);
  if (p) XFreePixmap(display, p);
  XResizeWindow(display, icon.window, icon.width + 4, icon.height + 4);
  XClearWindow(display, icon.window);
}


void BlackboxIcon::Reconfigure(void) {
  XGCValues gcv;
  gcv.foreground = blackbox->iconTextColor().pixel;
  gcv.font = blackbox->iconFont()->fid;
  XChangeGC(display, iconGC, GCForeground|GCFont, &gcv);

  icon.label_w = XTextWidth(blackbox->iconFont(),
                            ((icon.name) ? icon.name : "Unnamed"),
                            strlen(((icon.name) ? icon.name : "Unnamed")));
  icon.width = icon.pixmap_w + icon.label_w + 4;
  
  icon.label_h = blackbox->iconFont()->ascent + blackbox->iconFont()->descent + 4;
  icon.height = ((icon.pixmap_h < icon.label_h) ? icon.label_h : icon.pixmap_h);
  
  BImage *i_image = new BImage(blackbox, icon.width + 4, icon.height + 4,
			       blackbox->Depth());
  Pixmap p = i_image->renderImage(blackbox->toolboxTexture(),
				  blackbox->toolboxColor(),
				  blackbox->toolboxToColor());
  delete i_image;
  XSetWindowBackgroundPixmap(display, icon.window, p);
  if (p) XFreePixmap(display, p);
  XResizeWindow(display, icon.window, icon.width + 4, icon.height + 4);
  XClearWindow(display, icon.window);
  XDrawString(display, icon.window, iconGC, icon.pixmap_w + 5,
	      (icon.height + blackbox->iconFont()->ascent -
	       blackbox->iconFont()->descent) / 2,
              ((icon.name) ? icon.name : "Unnamed"),
              strlen(((icon.name) ? icon.name : "Unnamed")));
}
