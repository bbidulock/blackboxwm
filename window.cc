//
// window.cc for Blackbox - an X11 Window manager
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
#include "Windowmenu.hh"
#include "Workspace.hh"

#include "blackbox.hh"
#include "icon.hh"
#include "window.hh"

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <string.h>


// *************************************************************************
// Window class code
// *************************************************************************

BlackboxWindow::BlackboxWindow(Blackbox *ctrl, Window window, Bool internal)
{
  // set control members for operation
  moving = resizing = shaded = maximized = visible = iconic = transient =
    focused = False;

  window_number = -1;
  internal_window = internal;
  
  blackbox = ctrl;
  display = ctrl->control();
  client.window = window;

  frame.window = frame.title = frame.handle = frame.close_button =
    frame.iconify_button = frame.maximize_button = client.icon_window = 
    frame.button = frame.pbutton = frame.base = frame.fbase =
    frame.ubase = None;

  client.transient_for = client.transient = 0;
  client.title = client.app_class = client.app_name = 0;
  icon = 0;
  
  // fetch client size and placement
  XWindowAttributes wattrib;
  XGetWindowAttributes(display, client.window, &wattrib);
  if (wattrib.override_redirect) return;
  client.x = wattrib.x;
  client.y = wattrib.y;
  client.width = wattrib.width;
  client.height = wattrib.height;

  // get size, aspect, minimum/maximum size and other hints set by the
  // client
  getWMHints();

  // determine if this is a transient or an internal window
  if (! internal_window) {
    Window win;
    if (XGetTransientForHint(display, client.window, &win))
      if (win && (win != client.window_group))
	if ((client.transient_for = blackbox->searchWindow(win)) != NULL) {
	  client.transient_for->client.transient = this;	  
	  transient = True;
	}
  }

  // find out what type of decorations we need to create
  XSizeHints sizehint;
  getWMNormalHints(&sizehint);

  if ((! transient) && (! internal_window)) {
    resizable = True;

    if ((client.hint_flags & PMinSize) && (client.hint_flags & PMaxSize))
      if (client.max_w == client.min_w && client.max_h == client.min_h)
	resizable = False;
  } else
    resizable = False;
  
  frame.base_w = ((transient) ? 0 : client.width + 8);
  frame.base_h = ((transient) ? 0 : client.height + 8);
  frame.title_h = blackbox->titleFont()->ascent +
    blackbox->titleFont()->descent + 8;

  frame.button_h = frame.title_h - 8;
  frame.button_w = frame.button_h * 2 / 3;

  frame.rh_w = ((resizable) ? 8 : 0);
  frame.rh_h = ((resizable) ? frame.button_h : 0);
  frame.handle_w = ((resizable) ? 8 : 0);
  frame.handle_h = ((resizable) ? frame.base_h - frame.rh_h - 1 : 0);

  frame.title_w = ((transient) ? client.width :
		   frame.base_w + ((resizable) ? frame.handle_w + 1: 0));
  
  frame.width = frame.title_w;
  frame.height = ((transient) ? client.height + frame.title_h + 1 :
		  frame.base_h + frame.title_h + 1);
  
  if ((blackbox->Startup()) || client.hint_flags & (PPosition|USPosition)
      || transient) { 
    frame.x = client.x - ((transient) ? 1 : 5);
    frame.y = client.y - frame.title_h - ((transient) ? 1 : 5);
  } else {
    static unsigned int cx = 32, cy = 32;

    if (cx > (blackbox->XResolution() / 2)) { cx = 32; cy = 32; }
    if (cy > (blackbox->YResolution() / 2)) { cx = 32; cy = 32; }

    frame.x = cx;
    frame.y = cy;

    cy += frame.title_h;
    cx += frame.title_h;
  }

  frame.window = createToplevelWindow(frame.x, frame.y, frame.width,
				      frame.height, 1);
  blackbox->saveWindowSearch(frame.window, this);

  frame.title = createChildWindow(frame.window, 0, 0, frame.title_w,
				  frame.title_h, 0l);
  blackbox->saveWindowSearch(frame.title, this);

  if (! transient) {
    frame.base = createChildWindow(frame.window, 0, frame.title_h + 1,
				   frame.base_w, frame.base_h, 0l);
    blackbox->saveWindowSearch(frame.base, this);
    
    if (resizable) {
      frame.handle = createChildWindow(frame.window, frame.base_w + 1,
				       frame.title_h + 1, frame.handle_w,
				       frame.handle_h, 0l);
      blackbox->saveWindowSearch(frame.handle, this);
      
      frame.resize_handle = createChildWindow(frame.window, frame.base_w + 1,
					      frame.title_h +
					      frame.handle_h + 2,
					      frame.rh_w, frame.rh_h, 0l);
      blackbox->saveWindowSearch(frame.resize_handle, this);
    }
  }
  
  client.WMProtocols = blackbox->ProtocolsAtom();
  getWMProtocols();
  createDecorations();
  associateClientWindow();
  positionButtons();

  XGrabKey(display, XKeysymToKeycode(display, XK_Tab), Mod1Mask,
           frame.window, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(display, XKeysymToKeycode(display, XK_Left), ControlMask,
           frame.window, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(display, XKeysymToKeycode(display, XK_Right), ControlMask,
	   frame.window, True, GrabModeAsync, GrabModeAsync);

  XLowerWindow(display, client.window);
  XMapSubwindows(display, frame.title);
  if (! transient)
    XMapSubwindows(display, frame.base);
  XMapSubwindows(display, frame.window);
  
  if (! internal_window)
    windowmenu = new Windowmenu(this, blackbox);
  else
    windowmenu = 0;

  blackbox->addWindow(this);
  configureWindow(frame.x, frame.y, frame.width, frame.height);
  setFocusFlag(False);
  
  if (iconic && ! visible)
    iconifyWindow();
}  


BlackboxWindow::~BlackboxWindow(void) {
  XGrabServer(display);
  
  if (! internal_window) {
    if (windowmenu)
      delete windowmenu;
    if (icon)
      delete icon;

    blackbox->removeWindow(this);
  }

  if (transient && client.transient_for)
    client.transient_for->client.transient = 0;
  
  if (frame.close_button != None) {
    blackbox->removeWindowSearch(frame.close_button);
    XDestroyWindow(display, frame.close_button);
  }

  if (frame.iconify_button != None) {
    blackbox->removeWindowSearch(frame.iconify_button);
    XDestroyWindow(display, frame.iconify_button);
  }

  if (frame.maximize_button != None) {
    blackbox->removeWindowSearch(frame.maximize_button);
    XDestroyWindow(display, frame.maximize_button);
  }

  if (frame.title != None) {
    if (frame.ftitle) XFreePixmap(display, frame.ftitle);
    if (frame.utitle) XFreePixmap(display, frame.utitle);
    blackbox->removeWindowSearch(frame.title);
    XDestroyWindow(display, frame.title);
  }

  if (frame.handle != None) {
    if (frame.fhandle) XFreePixmap(display, frame.fhandle);
    if (frame.uhandle) XFreePixmap(display, frame.uhandle);
    if (frame.rhandle) XFreePixmap(display, frame.rhandle);
    blackbox->removeWindowSearch(frame.handle);
    blackbox->removeWindowSearch(frame.resize_handle);
    XDestroyWindow(display, frame.resize_handle);
    XDestroyWindow(display, frame.handle);
  }

  if (frame.base != None) {
    if (frame.fbase != None) XFreePixmap(display, frame.fbase);
    if (frame.ubase != None) XFreePixmap(display, frame.ubase);
    blackbox->removeWindowSearch(frame.base);
    XDestroyWindow(display, frame.base);
  }

  if (frame.button != None) XFreePixmap(display, frame.button);
  if (frame.pbutton != None) XFreePixmap(display, frame.pbutton);
  blackbox->removeWindowSearch(client.window);

  blackbox->removeWindowSearch(frame.window);  
  XDestroyWindow(display, frame.window);  

  if (client.app_name) XFree(client.app_name);
  if (client.app_class) XFree(client.app_class);
  if (client.title)
    if (strcmp(client.title, "Unnamed"))
      XFree(client.title);

  XFreeGC(display, frame.ftextGC);
  XFreeGC(display, frame.utextGC);

  XUngrabServer(display);
}


// *************************************************************************
// Window decoration code
// *************************************************************************

Window BlackboxWindow::createToplevelWindow(int x, int y, unsigned int width,
					    unsigned int height,
					    unsigned int borderwidth)
{
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWOverrideRedirect |CWCursor|CWEventMask; 
  
  attrib_create.background_pixmap = None;
  attrib_create.background_pixel = blackbox->frameColor().pixel;
  attrib_create.border_pixel = blackbox->frameColor().pixel;
  attrib_create.override_redirect = True;
  attrib_create.cursor = blackbox->sessionCursor();
  attrib_create.event_mask = ButtonPressMask|ButtonReleaseMask|
    ButtonMotionMask|ExposureMask|EnterWindowMask;
  
  return (XCreateWindow(display, blackbox->Root(), x, y, width, height,
			borderwidth, blackbox->Depth(), InputOutput,
			blackbox->visual(), create_mask,
			&attrib_create));
}


Window BlackboxWindow::createChildWindow(Window parent, int x, int y,
					 unsigned int width,
					 unsigned int height,
					 unsigned int borderwidth)
{
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|CWCursor|
    CWEventMask;
  
  attrib_create.background_pixmap = None;
  attrib_create.background_pixel = attrib_create.border_pixel = 
    blackbox->frameColor().pixel;
  attrib_create.cursor = blackbox->sessionCursor();
  attrib_create.event_mask = KeyPressMask|KeyReleaseMask|ButtonPressMask|
    ButtonReleaseMask|ButtonMotionMask|ExposureMask|EnterWindowMask|
    LeaveWindowMask;
  
  return (XCreateWindow(display, parent, x, y, width, height, borderwidth,
			blackbox->Depth(), InputOutput, blackbox->visual(),
			create_mask, &attrib_create));
}


void BlackboxWindow::associateClientWindow(void) {
  Protocols.WM_DELETE_WINDOW = False;
  Protocols.WM_TAKE_FOCUS = True;

  const unsigned long event_mask = StructureNotifyMask|PropertyChangeMask|
    EnterWindowMask|LeaveWindowMask|ColormapChangeMask|SubstructureNotifyMask|
    FocusChangeMask;

  XSetWindowAttributes attrib_set;
  attrib_set.event_mask = event_mask;
  attrib_set.do_not_propagate_mask = ButtonPressMask|ButtonReleaseMask;
  attrib_set.save_under = False;
  XChangeWindowAttributes(display, client.window, CWEventMask|CWDontPropagate|
			  CWSaveUnder, &attrib_set);
  XSetWindowBorderWidth(display, client.window, 0);

  if (! XFetchName(display, client.window, &client.title))
    client.title = "Unnamed";

  XEvent foo;
  if (! XCheckTypedWindowEvent(display, client.window, DestroyNotify,
                               &foo)) {
    attrib_set.event_mask = SubstructureRedirectMask|ButtonPressMask|
      ButtonReleaseMask|ButtonMotionMask|ExposureMask|EnterWindowMask;

    if (transient) {
      XChangeWindowAttributes(display, frame.window, CWEventMask, &attrib_set);
      XReparentWindow(display, client.window, frame.window, 0,
		      frame.title_h + 1);
    } else {
      XChangeWindowAttributes(display, frame.base, CWEventMask, &attrib_set);
      XReparentWindow(display, client.window, frame.base, 4, 4);
    }
  }

#ifdef SHAPE
  if (blackbox->shapeExtensions()) {
    XShapeSelectInput(display, client.window, ShapeNotifyMask);

    int foo, bShaped;
    unsigned int ufoo;
    XShapeQueryExtents(display, client.window, &bShaped, &foo, &foo, &ufoo,
		       &ufoo, &foo, &foo, &foo, &ufoo, &ufoo);
    frame.shaped = bShaped;

    if (frame.shaped) {
      XShapeCombineShape(display, frame.window, ShapeBounding, 4,
			 frame.title_h + 5, client.window, ShapeBounding,
			 ShapeSet);

      int num = 1;
      XRectangle xrect[2];
      xrect[0].x = xrect[0].y = 0;
      xrect[0].width = frame.title_w + 2;
      xrect[0].height = frame.title_h + 1;

      if (resizable) {
	xrect[1].x = 0;
	xrect[1].y = frame.title_h + frame.base_h + 1;
	xrect[1].width = frame.title_w;
	xrect[1].height = frame.handle_h;
	num++;
      }

      XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
			      xrect, num, ShapeUnion, Unsorted);
    }
  }
#endif

  blackbox->saveWindowSearch(client.window, this);
  createIconifyButton();
  createMaximizeButton();

  if (frame.button && frame.close_button)
    XSetWindowBackgroundPixmap(display, frame.close_button, frame.button);
  if (frame.button && frame.maximize_button)
    XSetWindowBackgroundPixmap(display, frame.maximize_button, frame.button);
  if (frame.button && frame.iconify_button)
    XSetWindowBackgroundPixmap(display, frame.iconify_button, frame.button);
}


void BlackboxWindow::createDecorations(void) {
  BImage *t_image = new BImage(blackbox, frame.title_w, frame.title_h,
			     blackbox->Depth());
  frame.ftitle = t_image->renderImage(blackbox->windowTexture(),
				      blackbox->focusColor(),
				      blackbox->focusToColor());
  frame.utitle = t_image->renderImage(blackbox->windowTexture(),
				      blackbox->unfocusColor(),
				      blackbox->unfocusToColor());
  delete t_image;

  XSetWindowBackgroundPixmap(display, frame.title, frame.utitle);
  XClearWindow(display, frame.title);

  if (! transient) {
    BImage *b_image = new BImage(blackbox, frame.base_w, frame.base_h,
				 blackbox->Depth());
    frame.fbase = b_image->renderSolidImage(blackbox->windowTexture(),
					    blackbox->focusColor());
    frame.ubase = b_image->renderSolidImage(blackbox->windowTexture(),
					    blackbox->unfocusColor());
    delete b_image;
    
    if (resizable) {
      BImage *h_image = new BImage(blackbox, frame.handle_w, frame.handle_h,
				   blackbox->Depth());
      frame.fhandle = h_image->renderImage(blackbox->windowTexture(),
					   blackbox->focusColor(),
					   blackbox->focusToColor());
      frame.uhandle = h_image->renderImage(blackbox->windowTexture(),
					   blackbox->unfocusColor(),
					   blackbox->unfocusToColor());
      delete h_image;

      XSetWindowBackground(display, frame.handle, frame.uhandle);
      XClearWindow(display, frame.handle);
      
      BImage *rh_image = new BImage(blackbox, frame.rh_w, frame.rh_h,
				    blackbox->Depth());
      frame.rhandle = rh_image->renderImage(blackbox->buttonTexture(),
					    blackbox->buttonColor(),
					    blackbox->buttonToColor());
      delete rh_image;

      XSetWindowBackgroundPixmap(display, frame.resize_handle, frame.rhandle);
      XClearWindow(display, frame.resize_handle);
    }
  }
    
  BImage *b_image = new BImage(blackbox, frame.button_w, frame.button_h,
			       blackbox->Depth());
  frame.button = b_image->renderImage(blackbox->buttonTexture(),
				      blackbox->buttonColor(),
				      blackbox->buttonToColor());
  frame.pbutton = b_image->renderInvertedImage(blackbox->buttonTexture(),
					       blackbox->buttonColor(),
					       blackbox->buttonToColor());
  delete b_image;

  XGCValues gcv;
  gcv.foreground = blackbox->unfocusTextColor().pixel;
  gcv.font = blackbox->titleFont()->fid;
  frame.utextGC = XCreateGC(display, frame.window, GCForeground|GCBackground|
			    GCFont, &gcv);

  gcv.foreground = blackbox->focusTextColor().pixel;
  gcv.font = blackbox->titleFont()->fid;
  frame.ftextGC = XCreateGC(display, frame.window, GCForeground|GCBackground|
			    GCFont, &gcv);
}


void BlackboxWindow::positionButtons(void) {
  int mx = frame.title_w - frame.button_w - 5, my = 3;
  
  if (do_close && frame.close_button != None) {
    XMoveResizeWindow(display, frame.close_button, mx, my, frame.button_w,
		      frame.button_h);
    XMapWindow(display, frame.close_button);
    XClearWindow(display, frame.close_button);
    mx = mx - frame.button_w - 4;
  }
  
  if (resizable && frame.maximize_button != None) {
    XMoveResizeWindow(display, frame.maximize_button, mx, my,
		      frame.button_w, frame.button_h);
    XMapWindow(display, frame.maximize_button);
    XClearWindow(display, frame.maximize_button);
    mx = mx - frame.button_w - 4;
  }
  
  if (do_iconify && frame.iconify_button != None) {
    XMoveResizeWindow(display, frame.iconify_button, mx, my, frame.button_w,
		      frame.button_h);
    XMapWindow(display, frame.iconify_button);
    XClearWindow(display, frame.iconify_button);
  }
}


void BlackboxWindow::createCloseButton(void) {
  if (do_close && frame.title != None && frame.close_button == None) {
    frame.close_button = createChildWindow(frame.title, 0, 0, frame.button_w,
					   frame.button_h, 1);
    if (frame.button != None)
      XSetWindowBackgroundPixmap(display, frame.close_button, frame.button);

    blackbox->saveWindowSearch(frame.close_button, this);
  }
}


void BlackboxWindow::createIconifyButton(void) {
  if (do_iconify && frame.title != None) {
    frame.iconify_button =
      createChildWindow(frame.title, 0, 0, frame.button_w,
		    frame.button_h, 1);
    blackbox->saveWindowSearch(frame.iconify_button, this);
  }
}


void BlackboxWindow::createMaximizeButton(void) {
  if (resizable && frame.title != None) {
    frame.maximize_button =
      createChildWindow(frame.title, 0, 0, frame.button_w, frame.button_h, 1);
    blackbox->saveWindowSearch(frame.maximize_button, this);
  }
}


void BlackboxWindow::Reconfigure(void) {
  XGrabServer(display);

  XGCValues gcv;
  gcv.foreground = blackbox->unfocusTextColor().pixel;
  gcv.font = blackbox->titleFont()->fid;
  XChangeGC(display, frame.utextGC, GCForeground|GCBackground|GCFont, &gcv);

  gcv.foreground = blackbox->focusTextColor().pixel;
  gcv.font = blackbox->titleFont()->fid;
  XChangeGC(display, frame.ftextGC, GCForeground|GCBackground|GCFont, &gcv);

  frame.base_w = ((transient) ? 0 : client.width + 8);
  frame.base_h = ((transient) ? 0 : client.height + 8);
  frame.title_h = blackbox->titleFont()->ascent +
    blackbox->titleFont()->descent + 8;

  frame.button_h = frame.title_h - 8;
  frame.button_w = frame.button_h * 2 / 3;

  frame.rh_w = ((resizable) ? 8 : 0);
  frame.rh_h = ((resizable) ? frame.button_h : 0);
  frame.handle_w = ((resizable) ? 8 : 0);
  frame.handle_h = ((resizable) ? frame.base_h - frame.rh_h - 1 : 0);

  frame.title_w = ((transient) ? client.width :
		   frame.base_w + ((resizable) ? frame.handle_w + 1: 0));
  
  frame.width = frame.title_w;
  frame.height = ((transient) ? client.height + frame.title_h + 1 :
		  frame.base_h + frame.title_h + 1);

  if (windowmenu) {
    windowmenu->Reconfigure();
  } 
  
  client.x = frame.x + ((transient) ? 1 : 5);
  client.y = frame.y + frame.title_h + ((transient) ? 1 : 5);
  
  XGrabServer(display);
  XResizeWindow(display, frame.window, frame.width, frame.height);
  XResizeWindow(display, frame.title, frame.title_w, frame.title_h);
  
  if (! transient) {
    XResizeWindow(display, frame.base, frame.base_w, frame.base_h);

    if (resizable) {
      XMoveResizeWindow(display, frame.handle, frame.base_w + 1,
			frame.title_h + 1,
			frame.handle_w, frame.handle_h);
      
      XMoveResizeWindow(display, frame.resize_handle, frame.base_w + 1,
			frame.handle_h + frame.title_h + 2,
			frame.rh_w, frame.rh_h);
    }
    
    XMoveResizeWindow(display, client.window, 4, 4,
		      client.width, client.height);
  } else
    XMoveResizeWindow(display, client.window, 0, frame.title_h + 1,
		      client.width, client.height);
  
#ifdef SHAPE
  if (blackbox->shapeExtensions()) {
    if (frame.shaped) {
      XShapeCombineShape(display, frame.window, ShapeBounding, 0,
			 frame.title_h + 1, client.window, ShapeBounding,
			 ShapeSet);
      
      int num = 1;
      XRectangle xrect[2];
      xrect[0].x = xrect[0].y = 0;
      xrect[0].width = frame.title_w;
      xrect[0].height = frame.title_h;
      
      if (resizable) {
	xrect[1].x = 4;
	xrect[1].y = frame.title_h;
	xrect[1].width = frame.handle_w;
	xrect[1].height = frame.handle_h + frame.button_h + 1;
	num++;
      }
      
      XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
			      xrect, num, ShapeUnion, Unsorted);
    }
  }
#endif

  if (frame.button) XFreePixmap(display, frame.button);
  if (frame.pbutton) XFreePixmap(display, frame.pbutton);

  BImage *b_image = new BImage(blackbox, frame.button_w, frame.button_h,
			       blackbox->Depth());
  frame.button = b_image->renderImage(blackbox->buttonTexture(),
					  blackbox->buttonColor(),
					  blackbox->buttonToColor());
  frame.pbutton = b_image->renderInvertedImage(blackbox->buttonTexture(),
					       blackbox->buttonColor(),
					       blackbox->buttonToColor());
  delete b_image;

  if (frame.iconify_button) XSetWindowBorder(display, frame.iconify_button,
					     blackbox->frameColor().pixel);
  if (frame.maximize_button) XSetWindowBorder(display, frame.maximize_button,
					     blackbox->frameColor().pixel);
  if (frame.close_button) XSetWindowBorder(display, frame.close_button,
					   blackbox->frameColor().pixel);

  positionButtons();
  
  if (frame.ftitle) XFreePixmap(display, frame.ftitle);
  if (frame.utitle) XFreePixmap(display, frame.utitle);
  
  BImage *t_image = new BImage(blackbox, frame.title_w, frame.title_h,
			      blackbox->Depth());
  frame.ftitle = t_image->renderImage(blackbox->windowTexture(),
				   blackbox->focusColor(),
				   blackbox->focusToColor());
  frame.utitle = t_image->renderImage(blackbox->windowTexture(),
				   blackbox->unfocusColor(),
				   blackbox->unfocusToColor());
  delete t_image;

  if (! transient) {
    if (frame.fbase) XFreePixmap(display, frame.fbase);
    if (frame.ubase) XFreePixmap(display, frame.ubase);

    BImage *b_image = new BImage(blackbox, frame.base_w, frame.base_h,
				 blackbox->Depth());
    frame.fbase = b_image->renderSolidImage(blackbox->windowTexture(),
					    blackbox->focusColor());
    frame.ubase = b_image->renderSolidImage(blackbox->windowTexture(),
					    blackbox->unfocusColor());
    delete b_image;

    if (resizable) {
      if (frame.fhandle) XFreePixmap(display, frame.fhandle);
      if (frame.uhandle) XFreePixmap(display, frame.uhandle);
      
      BImage *h_image = new BImage(blackbox, frame.handle_w, frame.handle_h,
				   blackbox->Depth());
      frame.fhandle = h_image->renderImage(blackbox->windowTexture(),
					   blackbox->focusColor(),
					   blackbox->focusToColor());
      frame.uhandle = h_image->renderImage(blackbox->windowTexture(),
					   blackbox->unfocusColor(),
					   blackbox->unfocusToColor());
      delete h_image;

      if (frame.rhandle) XFreePixmap(display, frame.rhandle);
      BImage *rh_image = new BImage(blackbox, frame.rh_w, frame.rh_h,
				    blackbox->Depth());
      frame.rhandle = rh_image->renderImage(blackbox->buttonTexture(),
					    blackbox->buttonColor(),
					    blackbox->buttonToColor());
      delete rh_image;

      XSetWindowBackgroundPixmap(display, frame.resize_handle, frame.rhandle);
      XClearWindow(display, frame.resize_handle);
    }
  }
    
  XSetWindowBorder(display, frame.window, blackbox->frameColor().pixel);
  XSetWindowBackground(display, frame.window, blackbox->frameColor().pixel);
  XClearWindow(display, frame.window);
  XClearWindow(display, frame.base);
  setFocusFlag(focused);
  drawTitleWin();
  drawAllButtons();
  if (icon) icon->Reconfigure();
  
  XEvent event;
  event.type = ConfigureNotify;  
  event.xconfigure.display = display;
  event.xconfigure.event = client.window;
  event.xconfigure.window = client.window;
  event.xconfigure.x = client.x;
  event.xconfigure.y = client.y;
  event.xconfigure.width = client.width;
  event.xconfigure.height = client.height;
  event.xconfigure.border_width = 0;
  event.xconfigure.above = frame.window;
  event.xconfigure.override_redirect = False;
  
  XSendEvent(display, client.window, False, StructureNotifyMask, &event);

  XUngrabServer(display);
}


// *************************************************************************
// Window protocol and ICCCM code
// *************************************************************************

Bool BlackboxWindow::getWMProtocols(void) {
  Atom *protocols;
  int num_return = 0;
  if (! XGetWMProtocols(display, client.window, &protocols, &num_return))
    return False;

  do_close = False;
  for (int i = 0; i < num_return; ++i) {
    if (protocols[i] == blackbox->DeleteAtom()) {
      do_close = True;
      Protocols.WM_DELETE_WINDOW = True;
      client.WMDelete = blackbox->DeleteAtom();
      createCloseButton();
      positionButtons();
    } else if (protocols[i] ==  blackbox->FocusAtom()) {
      Protocols.WM_TAKE_FOCUS = True;
    } else if (protocols[i] ==  blackbox->StateAtom()) {
      Atom atom;
      int foo;
      unsigned long ulfoo, nitems;
      unsigned char *state;
      XGetWindowProperty(display, client.window, blackbox->StateAtom(),
			 0, 3, False, blackbox->StateAtom(), &atom, &foo,
			 &nitems, &ulfoo, &state);

      if (state != NULL) {
	switch (*((unsigned long *) state)) {
	case WithdrawnState:
	  withdrawWindow();
	  setFocusFlag(False);
	  break;
	  
	case IconicState:
	  iconifyWindow();
	  break;
	    
	case NormalState:
	default:
	  deiconifyWindow();
	  setFocusFlag(False);
	  break;
	}
      }
    } else  if (protocols[i] ==  blackbox->ColormapAtom()) {
    }
  }

  XFree(protocols);
  return True;
}


Bool BlackboxWindow::getWMHints(void) {
  XWMHints *wmhints;
  if ((wmhints = XGetWMHints(display, client.window)) == NULL) {
    visible = True;
    iconic = False;
    focus_mode = F_Passive;

    return False;
  }

  if (wmhints->flags & InputHint) {
    if (wmhints->input == True) {
      if (Protocols.WM_TAKE_FOCUS)
	focus_mode = F_LocallyActive;
      else
	focus_mode = F_Passive;
    } else {
      if (Protocols.WM_TAKE_FOCUS)
	focus_mode = F_GloballyActive;
      else
	focus_mode = F_NoInput;
    }
  } else
    focus_mode = F_Passive;
  
  if (wmhints->flags & StateHint)
    if (! blackbox->Startup())
      switch(wmhints->initial_state) {
      case WithdrawnState:
	visible = False;
	iconic = False;
	break;
	
      case NormalState:
	visible = True;
	iconic = False;
	break;
	
      case IconicState:
	visible = False;
	iconic = True;
	break;
      }
    else {
      visible = True;
      iconic = False;
    }
  else {
    visible = True;
    iconic = False;
  } 
  
  if (wmhints->flags & IconPixmapHint) {
    //    client.icon_pixmap = wmhints->icon_pixmap;
  }
  
  if (wmhints->flags & IconWindowHint) {
    //    client.icon_window = wmhints->icon_window;
  }
  
  /* icon position hint would be next, but as the ICCCM says, we can ignore
     this hint, as it is the window managers function to organize and place
     icons
  */
  
  if (wmhints->flags & IconMaskHint) {
    //    client.icon_mask = wmhints->icon_mask;
  }
  
  if (wmhints->flags & WindowGroupHint) {
    client.window_group = wmhints->window_group;
  }

  XFree(wmhints);
  return True;
}


Bool BlackboxWindow::getWMNormalHints(XSizeHints *hint) {
  long icccm_mask;
  if (! XGetWMNormalHints(display, client.window, hint, &icccm_mask))
    return False;
  
  /* check to see if we're dealing with an up to date client */

  client.hint_flags = hint->flags;
  if (icccm_mask == (USPosition|USSize|PPosition|PSize|PMinSize|PMaxSize|
		     PResizeInc|PAspect))
    icccm_compliant = False;
  else
    icccm_compliant = True;
  
  if (client.hint_flags & PResizeInc) {
    client.inc_w = hint->width_inc;
    client.inc_h = hint->height_inc;
  } else
    client.inc_h = client.inc_w = 1;
  
  if (client.hint_flags & PMinSize) {
    client.min_w = hint->min_width;
    client.min_h = hint->min_height;
  } else
    client.min_h = client.min_w = 1;

  if (client.hint_flags & PBaseSize) {
    client.base_w = hint->base_width;
    client.base_h = hint->base_height;
  } else
    client.base_w = client.base_h = 0;

  if (client.hint_flags & PMaxSize) {
    client.max_w = hint->max_width;
    client.max_h = hint->max_height;
  } else {
    client.max_w = blackbox->XResolution();
    client.max_h = blackbox->YResolution();
  }
  
  if (client.hint_flags & PAspect) {
    client.min_ax = hint->min_aspect.x;
    client.min_ay = hint->min_aspect.y;
    client.max_ax = hint->max_aspect.x;
    client.max_ay = hint->max_aspect.y;
  }

  return True;
}


// *************************************************************************
// Window utility function code
// *************************************************************************

void BlackboxWindow::configureWindow(int dx, int dy, unsigned int dw,
				     unsigned int dh) {
  //
  // configure our client and decoration windows according to the frame size
  // changes passed as the arguments
  //

  Bool resize, send_event;
  if (dw > (client.max_w + ((transient) ? 0 :
			    8 + ((resizable) ? frame.handle_w + 1 : 0))))
    dw = client.max_w + ((transient) ? 0:
			 8 + ((resizable) ? frame.handle_w + 1 : 0));
  if (dh > (client.max_h + frame.title_h + ((transient) ? 1 : 9)))
    dh = client.max_h + frame.title_h + ((transient) ? 1 : 9);
  
  if ((((signed) frame.width) + dx) < 0) dx = 0;
  if ((((signed) frame.title_h) + dy) < 0) dy = 0;
  
  resize = ((dw != frame.width) || (dh != frame.height));
  send_event = ((dx != frame.x || dy != frame.y) && (! resize));

  if (resize) {
    frame.x = dx;
    frame.y = dy;
    frame.width = dw;
    frame.height = dh;

    frame.base_w = ((transient) ? 0 :
		    frame.width - ((resizable) ? (frame.handle_w + 1) : 0));
    frame.base_h = ((transient) ? 0 :
		    frame.height - frame.title_h - 1);
    frame.title_w = frame.width;
    frame.handle_h = ((resizable) ? dh - frame.title_h - frame.rh_h - 2 : 0);

    client.x = dx + ((transient) ? 1 : 5);
    client.y = dy + frame.title_h + ((transient) ? 1 : 5);
    client.width = ((transient) ? frame.width : frame.base_w - 8);
    client.height = ((transient) ? frame.height - frame.title_h - 1 :
		     frame.base_h - 8);
    
    XWindowChanges xwc;
    xwc.x = dx;
    xwc.y = dy;
    xwc.width = dw;
    xwc.height = dh;
    XGrabServer(display);
    XConfigureWindow(display, frame.window, CWX|CWY|CWWidth|CWHeight, &xwc);
    XResizeWindow(display, frame.title, frame.title_w, frame.title_h);

    if (! transient) {
      XResizeWindow(display, frame.base, frame.base_w, frame.base_h);
      
      if (resizable) {
	XMoveResizeWindow(display, frame.handle, frame.base_w + 1,
			  frame.title_h + 1,
			  frame.handle_w, frame.handle_h);
	
	XMoveWindow(display, frame.resize_handle, frame.base_w + 1,
		    frame.title_h + frame.handle_h + 2);
      }
      
      XMoveResizeWindow(display, client.window, 4, 4, client.width,
			client.height);
    } else
      XMoveResizeWindow(display, client.window, 0, frame.title_h + 1,
			client.width, client.height);
    
    positionButtons();

    if (frame.ftitle) XFreePixmap(display, frame.ftitle);
    if (frame.utitle) XFreePixmap(display, frame.utitle);

    BImage *t_image = new BImage(blackbox, frame.title_w, frame.title_h,
				blackbox->Depth());
    frame.ftitle = t_image->renderImage(blackbox->windowTexture(),
					blackbox->focusColor(),
					blackbox->focusToColor());
    frame.utitle = t_image->renderImage(blackbox->windowTexture(),
					blackbox->unfocusColor(),
					blackbox->unfocusToColor());
    delete t_image;
    
    if (! transient) {
      if (frame.fbase) XFreePixmap(display, frame.fbase);
      if (frame.ubase) XFreePixmap(display, frame.ubase);

      BImage *b_image = new BImage(blackbox, frame.base_w, frame.base_h,
				   blackbox->Depth());
      frame.fbase = b_image->renderSolidImage(blackbox->windowTexture(),
					      blackbox->focusColor());
      frame.ubase = b_image->renderSolidImage(blackbox->windowTexture(),
					      blackbox->unfocusColor());
      delete b_image;
      
      if (resizable) {
	if (frame.fhandle) XFreePixmap(display, frame.fhandle);
	if (frame.uhandle) XFreePixmap(display, frame.uhandle);
	
	BImage *h_image = new BImage(blackbox, frame.handle_w, frame.handle_h,
				     blackbox->Depth());
	frame.fhandle = h_image->renderImage(blackbox->windowTexture(),
					    blackbox->focusColor(),
					    blackbox->focusToColor());
	frame.uhandle = h_image->renderImage(blackbox->windowTexture(),
					    blackbox->unfocusColor(),
					    blackbox->unfocusToColor());
	delete h_image;
      }
    }
    
    XClearWindow(display, frame.base);
    setFocusFlag(focused);
    drawTitleWin();
    drawAllButtons();
    XUngrabServer(display);
  } else {
    frame.x = dx;
    frame.y = dy;
    
    client.x = dx + ((transient) ? 1 : 5);
    client.y = dy + frame.title_h + ((transient) ? 1 : 5);
    
    XWindowChanges xwc;
    xwc.x = dx;
    xwc.y = dy;
    XConfigureWindow(display, frame.window, CWX|CWY, &xwc);
  }

  if (send_event) {
    XEvent event;
    event.type = ConfigureNotify;

    event.xconfigure.display = display;
    event.xconfigure.event = client.window;
    event.xconfigure.window = client.window;
    event.xconfigure.x = client.x;
    event.xconfigure.y = client.y;
    event.xconfigure.width = client.width;
    event.xconfigure.height = client.height;
    event.xconfigure.border_width = 0;
    event.xconfigure.above = frame.window;
    event.xconfigure.override_redirect = False;

    XSendEvent(display, client.window, False, StructureNotifyMask, &event);
  }
}


Bool BlackboxWindow::setInputFocus(void) {
  if (((signed) (frame.x + frame.width)) < 0) {
    if (((signed) (frame.y + frame.title_h)) < 0)
      configureWindow(0, 0, frame.width, frame.height);
    else if (frame.y > (signed) blackbox->YResolution())
      configureWindow(0, blackbox->YResolution() - frame.height, frame.width,
		      frame.height);
    else
      configureWindow(0, frame.y, frame.width, frame.height);
  } else if (frame.x > (signed) blackbox->XResolution()) {
    if (((signed) (frame.y + frame.title_h)) < 0)
      configureWindow(blackbox->XResolution() - frame.width, 0, frame.width,
		      frame.height);
    else if (frame.y > (signed) blackbox->YResolution())
      configureWindow(blackbox->XResolution() - frame.width,
		      blackbox->YResolution() - frame.height, frame.width,
		      frame.height);
    else
      configureWindow(blackbox->XResolution() - frame.width, frame.y,
		      frame.width, frame.height);
  }


  switch (focus_mode) {
  case F_NoInput:
  case F_GloballyActive:
    drawTitleWin();
    break;
    
  case F_LocallyActive:
  case F_Passive:
    XSetWindowBackgroundPixmap(display, frame.title, frame.ftitle);
    XClearWindow(display, frame.title);

    if (! transient) {
      XSetWindowBackgroundPixmap(display, frame.base, frame.fbase);
      XClearWindow(display, frame.base);

      if (resizable) {
	XSetWindowBackgroundPixmap(display, frame.handle, frame.fhandle);
	XClearWindow(display, frame.handle);
      }
    }
      
    XSetInputFocus(display, client.window, RevertToParent,
		   CurrentTime);

    drawTitleWin();
    return True;
    break;
  }

  return False;
}


void BlackboxWindow::iconifyWindow(void) {
  if (windowmenu) windowmenu->Hide();
  
#ifdef ANIMATIONS
  if (! shaded) {
    int wx = frame.x + (frame.width / 2), sx = frame.width / 16,
      sy = frame.height / 16;
    
    int fx = frame.width, fy = frame.height;
    for (int i = 0; i < 16; i++, fx -= sx, fy -= sy)
      XMoveResizeWindow(display, frame.window, wx - (fx / 2), frame.y, fx, fy);
  } else {
    int wx = frame.x + (frame.width / 2), sx = frame.width / 16;
    int fx = frame.width;
    for (int i = 0; i < 16; i++, fx -= sx)
      XMoveResizeWindow(display, frame.window, wx - (fx / 2), frame.y, fx,
			frame.title_h);
  }
#endif

  XUnmapWindow(display, frame.window);
  visible = False;
  iconic = True;
  focused = False;    
  
  if (transient) {
    if (! client.transient_for->iconic)
      client.transient_for->iconifyWindow();
  } else
    icon = new BlackboxIcon(blackbox, client.window);

  if (client.transient)
    if (! client.transient->iconic)
      client.transient->iconifyWindow();

  unsigned long state[2];
  state[0] = (unsigned long) IconicState;
  state[1] = (unsigned long) None;
  XChangeProperty(display, client.window, blackbox->StateAtom(),
                  blackbox->StateAtom(), 32, PropModeReplace,
		  (unsigned char *) state, 2);
}


void BlackboxWindow::deiconifyWindow(void) {
#ifdef ANIMATIONS
  int wx = frame.x + (frame.width / 2), sx = frame.width / 16,
    sy = frame.height / 16;
  XMoveResizeWindow(display, frame.window, wx, frame.y, sx, sy);
#endif

  XMapWindow(display, frame.window);

#ifdef ANIMATIONS
  if (! shaded) {
    int fx = sx, fy = sy;
    for (int i = 0; i < 16; i++, fx += sx, fy += sy)
      XMoveResizeWindow(display, frame.window, wx - (fx / 2), frame.y, fx,
			fy);
    XMoveResizeWindow(display, frame.window, frame.x, frame.y, frame.width,
		      frame.height);
  } else {
    int fx = sx;
    for (int i = 0; i < 16; i++, fx += sx)
      XMoveResizeWindow(display, frame.window, wx - (fx / 2), frame.y, fx,
			frame.title_h);
    XMoveResizeWindow(display, frame.window, frame.x, frame.y, frame.width,
		      frame.title_h);
  }
#endif
      
  XMapSubwindows(display, frame.window);
  visible = True;
  iconic = False;

  if (client.transient) client.transient->deiconifyWindow();
  
  unsigned long state[2];
  state[0] = (unsigned long) NormalState;
  state[1] = (unsigned long) None;
  XChangeProperty(display, client.window, blackbox->StateAtom(),
		  blackbox->StateAtom(), 32, PropModeReplace,
		  (unsigned char *) state, 2);
  blackbox->reassociateWindow(this);

  if (icon) {
    delete icon;
    removeIcon();
  }
}


void BlackboxWindow::closeWindow(void) {
  XEvent ce;
  ce.xclient.type = ClientMessage;
  ce.xclient.message_type = blackbox->ProtocolsAtom();
  ce.xclient.display = display;
  ce.xclient.window = client.window;
  ce.xclient.format = 32;
  
  ce.xclient.data.l[0] = blackbox->DeleteAtom();
  ce.xclient.data.l[1] = CurrentTime;
  ce.xclient.data.l[2] = 0L;
  ce.xclient.data.l[3] = 0L;
  
  XSendEvent(display, client.window, False, NoEventMask, &ce);
  XFlush(display);
}


void BlackboxWindow::withdrawWindow(void) {
  XGrabServer(display);
  XSync(display, False);
  
  unsigned long state[2];
  state[0] = (unsigned long) WithdrawnState;
  state[1] = (unsigned long) None;

  focused = False;
  visible = False;

#ifdef ANIMATIONS
  int wx = frame.x + (frame.width / 2), sx = frame.width / 16,
    sy = frame.height / 16;
  
  if (! shaded) {
    int fx = frame.width, fy = frame.height;
    for (int i = 0; i < 16; i++, fx -= sx, fy -= sy)
      XMoveResizeWindow(display, frame.window, wx - (fx / 2), frame.y, fx, fy);
  } else {
    int fx = frame.width;
    for (int i = 0; i < 16; i++, fx -= sx)
      XMoveResizeWindow(display, frame.window, wx - (fx / 2), frame.y, fx,
			frame.title_h);
  }
#endif

  XUnmapWindow(display, frame.window);
  if (windowmenu) windowmenu->Hide();
  
  XEvent foo;
  if (! XCheckTypedWindowEvent(display, client.window, DestroyNotify,
			       &foo)) {
    XChangeProperty(display, client.window, blackbox->StateAtom(),
		    blackbox->StateAtom(), 32, PropModeReplace,
		    (unsigned char *) state, 2);
  }
  
  XUngrabServer(display);
}


int BlackboxWindow::setWindowNumber(int n) {
  window_number = n;
  return window_number;
}


int BlackboxWindow::setWorkspace(int n) {
  workspace_number = n;
  return workspace_number;
}


void BlackboxWindow::maximizeWindow(void) {
  XGrabServer(display);

  static int px, py;
  static unsigned int pw, ph;

  if (! maximized) {
    int dx, dy;
    unsigned int dw, dh;

    px = frame.x;
    py = frame.y;
    pw = frame.width;
    ph = frame.height;

    dw = blackbox->XResolution() - ((resizable) ? frame.handle_w + 1 : 0) - 1;
    dw -= client.base_w;
    dw -= (dw % client.inc_w);
    dw -= client.inc_w;
    dw += ((resizable) ? frame.handle_w + 1 : 0) + 1 +
      client.base_w;
    
    dx = ((blackbox->XResolution() - dw) / 2);

    dh = blackbox->YResolution() - frame.title_h - 2 -
      blackbox->workspaceManager()->Height();
    dh -= client.base_h;
    dh -= (dh % client.inc_h);
    dh -= client.inc_h;
    dh += frame.title_h + 2 + client.base_h;
    
    dy = ((blackbox->YResolution() - blackbox->workspaceManager()->Height())
	  - dh) / 2;

    maximized = True;
    shaded = False;
    configureWindow(dx, dy, dw, dh);
    blackbox->raiseWindow(this);
  } else {
    configureWindow(px, py, pw, ph);
    maximized = False;
  }

  XUngrabServer(display);
}


void BlackboxWindow::shadeWindow(void) {
  if (shaded) {
#ifdef ANIMATIONS
    int sy = frame.height / 16, fy = sy;
    for (int i = 0; i < 16; i++, fy += sy)
      XResizeWindow(display, frame.window, frame.width, fy);
#endif

    XResizeWindow(display, frame.window, frame.width, frame.height);
    shaded = False;
  } else {
#ifdef ANIMATIONS
    int sy = frame.height / 16, fy = frame.height;
    for (int i = 0; i < 16; i++, fy -= sy)
      XResizeWindow(display, frame.window, frame.width, fy);
#endif
    
    XResizeWindow(display, frame.window, frame.width, frame.title_h);
    shaded = True;
  }
}


void BlackboxWindow::setFocusFlag(Bool focus) {
  focused = focus;
  XSetWindowBackgroundPixmap(display, frame.title,
			     (focused) ? frame.ftitle : frame.utitle);
  XClearWindow(display, frame.title);

  if (! transient) {
    XSetWindowBackgroundPixmap(display, frame.base,
			       (focused) ? frame.fbase : frame.ubase);
    XClearWindow(display, frame.base);

    if (resizable) {
      XSetWindowBackgroundPixmap(display, frame.handle,
				 (focused) ? frame.fhandle : frame.uhandle);
      XClearWindow(display, frame.handle);
    }
  }
  
  drawTitleWin();
  drawAllButtons();
}


// *************************************************************************
// Window drawing code
// *************************************************************************

void BlackboxWindow::drawTitleWin(void) {  
  switch (blackbox->Justification()) {
  case Blackbox::B_LeftJustify: {
    int dx = frame.button_w + 4;
    
    XDrawString(display, frame.title,
		((focused) ? frame.ftextGC : frame.utextGC), dx,
		blackbox->titleFont()->ascent + 4, client.title,
		strlen(client.title));
    break; }

  case Blackbox::B_RightJustify: {
    int dx = (frame.button_w + 4) * 4;

    int off = XTextWidth(blackbox->titleFont(), client.title,
			 strlen(client.title)) + dx;
    
    XDrawString(display, frame.title,
		((focused) ? frame.ftextGC : frame.utextGC),
		frame.title_w - off, blackbox->titleFont()->ascent + 4,
		client.title, strlen(client.title));
    break;  }

  case Blackbox::B_CenterJustify: {
    int dx = (frame.button_w + 4) * 4;

    int ins = ((frame.width - dx) -
	       (XTextWidth(blackbox->titleFont(), client.title,
			   strlen(client.title)))) / 2;

    XDrawString(display, frame.title,
		((focused) ? frame.ftextGC : frame.utextGC),
		ins, blackbox->titleFont()->ascent + 4,
		client.title, strlen(client.title));
    break; }
  }
}


void BlackboxWindow::drawAllButtons(void) {
  if (frame.iconify_button) drawIconifyButton(False);
  if (frame.maximize_button) drawMaximizeButton(False);
  if (frame.close_button) drawCloseButton(False);
}


void BlackboxWindow::drawIconifyButton(Bool pressed) {
  if (! pressed && frame.button) {
    XSetWindowBackgroundPixmap(display, frame.iconify_button, frame.button);
    XClearWindow(display, frame.iconify_button);
  } else if (frame.pbutton) {
    XSetWindowBackgroundPixmap(display, frame.iconify_button, frame.pbutton);
    XClearWindow(display, frame.iconify_button);
  }

  XDrawRectangle(display, frame.iconify_button,
		 ((focused) ? frame.ftextGC : frame.utextGC),
		 2, frame.button_h - 5, frame.button_w - 5, 2);
}


void BlackboxWindow::drawMaximizeButton(Bool pressed) {
  if (! pressed && frame.button) {
    XSetWindowBackgroundPixmap(display, frame.maximize_button, frame.button);
    XClearWindow(display, frame.maximize_button);
  } else if (frame.pbutton) {
    XSetWindowBackgroundPixmap(display, frame.maximize_button, frame.pbutton);
    XClearWindow(display, frame.maximize_button);
  }

  XDrawRectangle(display, frame.maximize_button,
		 ((focused) ? frame.ftextGC : frame.utextGC),
		 2, 2, frame.button_w - 5, frame.button_h - 5);
  XDrawLine(display, frame.maximize_button,
	    ((focused) ? frame.ftextGC : frame.utextGC),
	    2, 3, frame.button_w - 3, 3);
}


void BlackboxWindow::drawCloseButton(Bool pressed) {
  if (! pressed && frame.button) {
    XSetWindowBackgroundPixmap(display, frame.close_button, frame.button);
    XClearWindow(display, frame.close_button);
  } else if (frame.pbutton) {
    XSetWindowBackgroundPixmap(display, frame.close_button, frame.pbutton);
    XClearWindow(display, frame.close_button);
  }

  XDrawLine(display, frame.close_button,
	    ((focused) ? frame.ftextGC : frame.utextGC), 2, 2,
            frame.button_w - 3, frame.button_h - 3);
  XDrawLine(display, frame.close_button,
	    ((focused) ? frame.ftextGC : frame.utextGC), 2, frame.button_h - 3,
            frame.button_w - 3, 2);
}


// *************************************************************************
// Window event code
// *************************************************************************

void BlackboxWindow::mapRequestEvent(XMapRequestEvent *re) {
  if (re->window == client.window) {
    if (visible && ! iconic) {
      XGrabServer(display);
      
      unsigned long state[2];
      state[0] = (unsigned long) NormalState;
      state[1] = (unsigned long) None;
      XChangeProperty(display, client.window, blackbox->StateAtom(),
		      blackbox->StateAtom(), 32, PropModeReplace,
		      (unsigned char *) state, 2);

#ifdef ANIMATIONS
      int wx = frame.x + (frame.width / 2), sx = frame.width / 16,
	sy = frame.height / 16;
      XMoveResizeWindow(display, frame.window, wx, frame.y, sx, sy);
#endif

      XMapSubwindows(display, frame.window);
      XMapWindow(display, frame.window);

#ifdef ANIMATIONS
      int fx = sx, fy = sy;
      for (unsigned int i = 0; i < 16; i++, fx += sx, fy += sy)
	XMoveResizeWindow(display, frame.window, wx - (fx / 2), frame.y, fx,
			  fy);
      XMoveResizeWindow(display, frame.window, frame.x, frame.y, frame.width,
			frame.height);
#endif
      
      setFocusFlag(False);
      XUngrabServer(display);
    }
  }
}


void BlackboxWindow::mapNotifyEvent(XMapEvent *ne) {
  if (ne->window == client.window && (! ne->override_redirect)) {
    if (visible && ! iconic) {
      XGrabServer(display);
      
      getWMProtocols();
      positionButtons();

      unsigned long state[2];
      state[0] = (unsigned long) NormalState;
      state[1] = (unsigned long) None;
      XChangeProperty(display, client.window, blackbox->StateAtom(),
		      blackbox->StateAtom(), 32, PropModeReplace,
		      (unsigned char *) state, 2);
      
      visible = True;
      iconic = False;
      XUngrabServer(display);
    }
  }
}


void BlackboxWindow::unmapNotifyEvent(XUnmapEvent *ue) {
  if (ue->window == client.window) {
    XGrabServer(display);
    XSync(display, False);

    unsigned long state[2];
    state[0] = (unsigned long) ((iconic) ? IconicState : WithdrawnState);
    state[1] = (unsigned long) None;

    visible = False;
      
#ifdef ANIMATIONS
    int wx = frame.x + (frame.width / 2), sx = frame.width / 16,
      sy = frame.height / 16;

    if (! shaded) {
      int fx = frame.width, fy = frame.height;
      for (int i = 0; i < 16; i++, fx -= sx, fy -= sy)
	XMoveResizeWindow(display, frame.window, wx - (fx / 2), frame.y, fx,
			  fy);
    } else {
      int fx = frame.width;
      for (int i = 0; i < 16; i++, fx -= sx)
	XMoveResizeWindow(display, frame.window, wx - (fx / 2), frame.y, fx,
			  frame.title_h);
    }
#endif

    XUnmapWindow(display, frame.window);

    XEvent event;
    if (! XCheckTypedWindowEvent(display, client.window, DestroyNotify,
				 &event)) {
      XReparentWindow(display, client.window, blackbox->Root(), client.x,
		      client.y);
      
      XChangeProperty(display, client.window, blackbox->StateAtom(),
		      blackbox->StateAtom(), 32, PropModeReplace,
		      (unsigned char *) state, 2);
    }
    
    XUngrabServer(display);
    delete this;
  }
}


void BlackboxWindow::destroyNotifyEvent(XDestroyWindowEvent *de) {
  if (de->window == client.window) {
#ifdef ANIMATIONS
    int wx = frame.x + (frame.width / 2), sx = frame.width / 16,
      sy = frame.height / 16;

    if (! shaded) {
      int fx = frame.width, fy = frame.height;
      for (int i = 0; i < 16; i++, fx -= sx, fy -= sy)
	XMoveResizeWindow(display, frame.window, wx - (fx / 2), frame.y, fx,
			  fy);
    } else {
      int fx = frame.width;
      for (int i = 0; i < 16; i++, fx -= sx)
	XMoveResizeWindow(display, frame.window, wx - (fx / 2), frame.y, fx,
			  frame.title_h);
    }
#endif
  
    XUnmapWindow(display, frame.window);
    delete this;
  }
}


void BlackboxWindow::propertyNotifyEvent(Atom atom) {
  switch(atom) {
  case XA_WM_CLASS:
    if (client.app_name) XFree(client.app_name);
    if (client.app_class) XFree(client.app_class);
    XClassHint classhint;
    if (XGetClassHint(display, client.window, &classhint)) {
      client.app_name = classhint.res_name;
      client.app_class = classhint.res_class;
    }
    
    break;
  
  case XA_WM_CLIENT_MACHINE:
  case XA_WM_COMMAND:   
    break;
    
  case XA_WM_HINTS:
    getWMHints();
    break;
	  
  case XA_WM_ICON_NAME:
    if (icon) icon->rereadLabel();
    break;
    
  case XA_WM_NAME:
    if (client.title)
      if (strcmp(client.title, "Unnamed"))
	XFree(client.title);
    if (! XFetchName(display, client.window, &client.title))
      client.title = "Unnamed";
    XClearWindow(display, frame.title);
    drawTitleWin();
    blackbox->workspaceManager()->workspace(workspace_number)->Update();
    break;
    
  case XA_WM_NORMAL_HINTS: {
    XSizeHints sizehint;
    getWMNormalHints(&sizehint);
    break;
  }
    
  case XA_WM_TRANSIENT_FOR:
    break;
    
  default:
    if (atom == blackbox->ProtocolsAtom())
      getWMProtocols();
  }
}


void BlackboxWindow::exposeEvent(XExposeEvent *ee) {
  if (frame.title == ee->window)
    drawTitleWin();
  else if (do_close && frame.close_button == ee->window)
    drawCloseButton(False);
  else if (resizable && frame.maximize_button == ee->window)
    drawMaximizeButton(False);
  else if (do_iconify && frame.iconify_button == ee->window)
    drawIconifyButton(False);
}


void BlackboxWindow::configureRequestEvent(XConfigureRequestEvent *cr) {  
  if (cr->window == client.window) {
    int cx, cy;
    unsigned int cw, ch;
    
    if (cr->value_mask & CWX)
      cx = cr->x - 1;
    else
      cx = frame.x;

    if (cr->value_mask & CWY)
      cy = cr->y - frame.title_h - 2;
    else
      cy = frame.y;

    if (cr->value_mask & CWWidth)
      cw = cr->width + 8 + ((resizable) ? frame.handle_w + 1 : 0);
    else
      cw = frame.width;

    if (cr->value_mask & CWHeight)
      ch = cr->height + frame.title_h + 9;
    else
      ch = frame.height;
    
    configureWindow(cx, cy, cw, ch);
  }
}


void BlackboxWindow::buttonPressEvent(XButtonEvent *be) {
  if (be->button == 1) {
    if (frame.title == be->window || frame.handle == be->window)
      blackbox->raiseWindow(this);
    else if (frame.iconify_button == be->window)
      drawIconifyButton(True);
    else if (frame.maximize_button == be->window)
      drawMaximizeButton(True);
    else if (frame.close_button == be->window)
      drawCloseButton(True);
    else if (frame.resize_handle == be->window)
      blackbox->raiseWindow(this);
  } else if (be->button == 2) {
    if (frame.title == be->window || frame.handle == be->window) {
      blackbox->lowerWindow(this);
    }
  } else if (be->button == 3) {
    if (frame.title == be->window) {
      if (windowmenu)
	if (! windowmenu->Visible()) {
	  windowmenu->Move(be->x_root - (windowmenu->Width() / 2),
			   frame.y + frame.title_h);
	  XRaiseWindow(display, windowmenu->WindowID());
	  windowmenu->Show();
	} else
	  windowmenu->Hide();
    }
  }
}


void BlackboxWindow::buttonReleaseEvent(XButtonEvent *re) {
  if (re->button == 1) {
    if (re->window == frame.title) {
      if (moving) {
#ifdef WIREMOVE
	XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
		       frame.x, frame.y, frame.width, frame.height);
	XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
		       frame.x +
		       ((blackbox->Orientation() ==
			 Blackbox::B_LeftHandedUser) ?
			frame.handle_w : 1), frame.y + frame.title_h,
		       frame.width - frame.handle_w - 1, frame.height -
		       frame.title_h - 1);
#endif
	configureWindow(frame.x, frame.y, frame.width, frame.height);
	moving = False;
	XUngrabPointer(display, CurrentTime);
      } else if ((re->state&ControlMask))
	shadeWindow();
    } else if (resizing) {  
      int dx, dy;
      
      XDrawString(display, blackbox->Root(), blackbox->GCOperations(),
		  frame.x + frame.x_resize + 5, frame.y +
		  frame.y_resize - 5, resizeLabel, strlen(resizeLabel));
      XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
		     frame.x, frame.y, frame.x_resize, frame.y_resize);
      XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
		     frame.x + 1, frame.y + frame.title_h + 1,
		     frame.x_resize - frame.handle_w - 1,
		     frame.y_resize - frame.title_h - 2);

      // calculate the size of the client window and conform it to the
      // size specified by the size hints of the client window...
      dx = frame.x_resize - frame.handle_w - 9;
      dy = frame.y_resize - frame.title_h - 9;
      dx -= ((dx % client.inc_w) - client.base_w);
      dy -= ((dy % client.inc_h) - client.base_h);
      
      if (dx < (signed) client.min_w) dx = client.min_w;
      if (dy < (signed) client.min_h) dy = client.min_h;
      
      // add the appropriate frame decoration sizes before configuring
      dx += frame.handle_w + 9;
      dy += frame.title_h + 9;

      delete [] resizeLabel;
      configureWindow(frame.x, frame.y, dx, dy);
      resizing = False;
      XUngrabPointer(display, CurrentTime);
    } else if (re->window == frame.iconify_button) {
      if ((re->x >= 0) && ((unsigned int) re->x <= frame.button_w) &&
	  (re->y >= 0) && ((unsigned int) re->y <= frame.button_h)) {
	iconifyWindow();
      }	else
	drawIconifyButton(False);
    } else if (re->window == frame.maximize_button) {
      if ((re->x >= 0) && ((unsigned int) re->x <= frame.button_w) &&
	  (re->y >= 0) && ((unsigned int) re->y <= frame.button_h)) {
	maximizeWindow();
      } else
	drawMaximizeButton(False);
    } else if (re->window == frame.close_button) {
      if ((re->x >= 0) && ((unsigned int) re->x <= frame.button_w) &&
	  (re->y >= 0) && ((unsigned int) re->y <= frame.button_h)) {
	closeWindow();
      } else
	drawCloseButton(False);
    }
  }
}


void BlackboxWindow::motionNotifyEvent(XMotionEvent *me) {
  if (frame.title == me->window) {
    if (me->state & Button1Mask) {
      if (! moving) {
	if (windowmenu->Visible())
	  windowmenu->Hide();

	if (XGrabPointer(display, frame.title, False, PointerMotionMask|
			 ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
			 None, blackbox->moveCursor(), CurrentTime)
	    == GrabSuccess) {
	  moving = True;
	  client.x_move = me->x;
	  client.y_move = me->y;

#ifdef WIREMOVE
	  XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			 frame.x, frame.y, frame.width, frame.height);
	  XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			 frame.x +
			 ((blackbox->Orientation() ==
			   Blackbox::B_LeftHandedUser) ?
			  frame.handle_w : 1), frame.y + frame.title_h,
			 frame.width - frame.handle_w - 1, frame.height -
			 frame.title_h - 1);
#endif
	} else
	  moving = False;
      } else {
	int dx = me->x_root - client.x_move,
	  dy = me->y_root - client.y_move;
	
#ifdef WIREMOVE
	XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
		       frame.x, frame.y, frame.width, frame.height);
	XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
		       frame.x +
		       ((blackbox->Orientation() ==
			 Blackbox::B_LeftHandedUser) ?
			frame.handle_w : 1), frame.y + frame.title_h,
		       frame.width - frame.handle_w - 1, frame.height -
		       frame.title_h - 1);
	frame.x = dx;
	frame.y = dy;
	XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
		       frame.x, frame.y, frame.width, frame.height);
	XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
		       frame.x +
		       ((blackbox->Orientation() ==
			 Blackbox::B_LeftHandedUser) ?
			frame.handle_w : 1), frame.y + frame.title_h,
		       frame.width - frame.handle_w - 1, frame.height -
		       frame.title_h - 1);
#else
	configureWindow(dx, dy, frame.width, frame.height);
#endif
      }
    }
  } else if (me->window == frame.resize_handle) {
    if (me->state & Button1Mask) {
      if (! resizing) {
	if (XGrabPointer(display, frame.resize_handle, False,
			 PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
			 GrabModeAsync, None, None, CurrentTime)
	    == GrabSuccess) {
	  int dx, dy;
	  resizing = True;

	  frame.x_resize = frame.width;
	  frame.y_resize = frame.height;

	  resizeLabel = new char[strlen("00000 x 00000") + 1];
	  dx = frame.x_resize - frame.handle_w - 9;
	  dy = frame.y_resize - frame.title_h - 1;
	  dx -= ((dx % client.inc_w) - client.base_w);
	  dy -= ((dy % client.inc_h) - client.base_h);
	  
	  if (dx < (signed) client.min_w) dx = client.min_w;
	  if (dy < (signed) client.min_h) dy = client.min_h;
	
	  sprintf(resizeLabel, "%d x %d", dx / client.inc_w,
		  dy / client.inc_h);

	  frame.x_resize = dx + frame.handle_w + 9;
	  frame.y_resize = dy + frame.title_h + 9;

	  XDrawString(display, blackbox->Root(), blackbox->GCOperations(),
		      frame.x + frame.x_resize + 5, frame.y +
		      frame.y_resize - 5, resizeLabel, strlen(resizeLabel));
	  XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			 frame.x, frame.y, frame.x_resize, frame.y_resize);
	  XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			 frame.x + 1, frame.y + frame.title_h + 1,
			 frame.x_resize - frame.handle_w - 1,
			 frame.y_resize - frame.title_h - 2);
	} else
	  resizing = False;
      } else if (resizing) {
	int dx, dy;

	XDrawString(display, blackbox->Root(), blackbox->GCOperations(),
		    frame.x + frame.x_resize + 5, frame.y +
		    frame.y_resize - 5, resizeLabel, strlen(resizeLabel));
	XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
		       frame.x, frame.y, frame.x_resize, frame.y_resize);
	XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
		       frame.x + 1, frame.y + frame.title_h + 1,
		       frame.x_resize - frame.handle_w - 1,
		       frame.y_resize - frame.title_h - 2);

	frame.x_resize = me->x_root - frame.x;
	if (frame.x_resize < 1) frame.x_resize = 1;
	frame.y_resize = me->y_root - frame.y;
	if (frame.y_resize < 1) frame.y_resize = 1;

	// calculate the size of the client window and conform it to the
	// size specified by the size hints of the client window...
	dx = frame.x_resize - frame.handle_w - 9;
	dy = frame.y_resize - frame.title_h - 1;
	dx -= ((dx % client.inc_w) - client.base_w);
	dy -= ((dy % client.inc_h) - client.base_h);
	
	if (dx < (signed) client.min_w) dx = client.min_w;
	if (dy < (signed) client.min_h) dy = client.min_h;
	
	sprintf(resizeLabel, "%d x %d", dx / client.inc_w,
		dy / client.inc_h);

	frame.x_resize = dx + frame.handle_w + 9;
	frame.y_resize = dy + frame.title_h + 9;

	XDrawString(display, blackbox->Root(), blackbox->GCOperations(),
		    frame.x + frame.x_resize + 5, frame.y +
		    frame.y_resize - 5, resizeLabel, strlen(resizeLabel));
	XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
		       frame.x, frame.y, frame.x_resize, frame.y_resize);
	XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
		       frame.x + 1, frame.y + frame.title_h + 1,
		       frame.x_resize - frame.handle_w - 1,
		       frame.y_resize - frame.title_h - 2);
      }
    }
  }
}


#ifdef SHAPE

void BlackboxWindow::shapeEvent(XShapeEvent *) {
  if (blackbox->shapeExtensions()) {
    if (frame.shaped) {
      XShapeCombineShape(display, frame.window, ShapeBounding, 0,
			 frame.title_h + 1, client.window, ShapeBounding,
			 ShapeSet);
      
      int num = 1;
      XRectangle xrect[2];
      xrect[0].x = xrect[0].y = 0;
      xrect[0].width = frame.title_w;
      xrect[0].height = frame.title_h;
      
      if (resizable) {
	xrect[1].x = ((resizable) ? 5 : 1);
	xrect[1].y = frame.title_h;
	xrect[1].width = frame.handle_w;
	xrect[1].height = frame.handle_h + frame.button_h + 1;
	num++;
      }
      
      XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
			      xrect, num, ShapeUnion, Unsorted);
    }
  }
}

#endif
