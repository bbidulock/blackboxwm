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
#include "window.hh"
#include "session.hh"
#include "icon.hh"
#include "workspace.hh"

#include <X11/keysym.h>
#include <string.h>


// *************************************************************************
// Window class code
// *************************************************************************


BlackboxWindow::BlackboxWindow(BlackboxSession *ctrl, Window window) {
  debug = new Debugger('%');
#ifdef DEBUG
  debug->enable();
#endif
  debug->msg("%s: BlackboxWindow::BlackboxWindow\n", __FILE__);

  moving = False;
  resizing = False;
  shaded = False;
  maximized = False;
  visible = False;
  iconic = False;
  transient = False;
  focused = False;
  menu_visible = False;
  window_number = -1;
  
  session = ctrl;
  display = ctrl->control();
  client.window = window;
  frame.window = frame.title = frame.handle = frame.close_button =
    frame.iconify_button = frame.maximize_button = client.icon_window = 
    frame.button = frame.pbutton = None;
  client.transient_for = client.transient = 0;
  client.title = client.app_class = client.app_name = 0;
  icon = 0;
  
  XWindowAttributes wattrib;
  XGetWindowAttributes(display, client.window, &wattrib);
  if (wattrib.override_redirect) return;
  do_close = True;
  client.x = wattrib.x;
  client.y = wattrib.y;
  client.width = wattrib.width;
  client.height = wattrib.height;

  Window win;
  getWMHints();

  transient = False;
  do_handle = True;
  do_iconify = True;
  do_maximize = True;
  
  if (XGetTransientForHint(display, client.window, &win)) {
    if (win != client.window_group) {
      if ((client.transient_for = session->getWindow(win)) != NULL)
	client.transient_for->client.transient = this;
      
      transient = True;
      do_iconify = True;
      do_handle = False;
      do_maximize = False; 
    }
  }
    
  XSizeHints sizehint;
  getWMNormalHints(&sizehint);
  if ((client.hint_flags & PMinSize) && (client.hint_flags & PMaxSize))
    if (client.max_w == client.min_w && client.max_h == client.min_h) {
      do_handle = False;
      do_maximize = False;
    }
  
  frame.handle_h = ((do_handle) ? client.height : 0);
  frame.handle_w = ((do_handle) ? 8 : 0);
  frame.title_h = session->titleFont()->ascent +
    session->titleFont()->descent + 8;
  frame.title_w = client.width + ((do_handle) ? frame.handle_w + 1 : 0);
  frame.button_h = frame.title_h - 8;
  frame.button_w = frame.button_h * 2 / 3;

  if ((session->Startup()) || client.hint_flags & (PPosition|USPosition)
      || transient) { 
    frame.x = client.x;
    frame.y = client.y;
  } else {
    static unsigned int cx = 32, cy = 32;

    if (cx > (session->XResolution() / 2)) { cx = 32; cy = 32; }
    if (cy > (session->YResolution() / 2)) { cx = 32; cy = 32; }

    frame.x = cx;
    frame.y = cy;

    cy += frame.title_h;
    cx += frame.title_h;
  }

  frame.width = frame.title_w;
  frame.height = client.height + frame.title_h + 1;

  frame.window = createToplevelWindow(frame.x, frame.y, frame.width,
				      frame.height, 1);
  XSaveContext(display, frame.window, session->winContext(), (XPointer) this);

  frame.title = createChildWindow(frame.window, 0, 0, frame.title_w,
				  frame.title_h, 0L);
  XSaveContext(display, frame.title, session->winContext(), (XPointer) this);

  if (do_handle) {
    if (session->Orientation() == BlackboxSession::B_LeftHandedUser)
      frame.handle = createChildWindow(frame.window, 0, frame.title_h + 1,
				       frame.handle_w, frame.handle_h, 0l);
    else
      frame.handle = createChildWindow(frame.window, client.width + 1,
				       frame.title_h + 1, frame.handle_w,
				       frame.handle_h, 0l);
    
    XSaveContext(display, frame.handle, session->winContext(),
		 (XPointer) this);
    
    frame.resize_handle = createChildWindow(frame.handle, 0, frame.handle_h -
					    frame.button_h,
					    frame.handle_w,
					    frame.button_h, 0);
					    
    frame.handle_h -= frame.button_h;
    XSaveContext(display, frame.resize_handle, session->winContext(),
    		 (XPointer) this);
  }

  client.WMProtocols = session->ProtocolsAtom();
  getWMProtocols();
  createDecorations();
  associateClientWindow();
  positionButtons();

  XGrabKey(display, XKeysymToKeycode(display, XK_Tab), ControlMask,
           frame.window, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(display, XKeysymToKeycode(display, XK_Left), ControlMask,
           frame.window, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(display, XKeysymToKeycode(display, XK_Right), ControlMask,
	   frame.window, True, GrabModeAsync, GrabModeAsync);

  XLowerWindow(display, client.window);
  XMapSubwindows(display, frame.title);
  if (do_handle) XMapSubwindows(display, frame.handle);
  XMapSubwindows(display, frame.window);
  
  window_menu = new BlackboxWindowMenu(this, session);
  session->addWindow(this);
  configureWindow(frame.x, frame.y, frame.width, frame.height);

  if (iconic && ! visible)
    iconifyWindow();

  debug->msg("%s: leaving BlackboxWindow::BlackboxWindow\n", __FILE__);
}


/*
  Resources deallocated for each window:
  * Debugger - debug
  * Window - frame window, title, handle, resize handle, close button,
      iconify button, maximize button, input window,
      client window (context only)
  * Icon - icon
  * Menu - window menu
  * XPointer - client title
  * Pixmaps - ftitle, utitle, fhandle, uhandle, button, pbutton
  * GC - lGC, dGC, textGC
*/

BlackboxWindow::~BlackboxWindow(void) {
  debug->msg("%s: BlackboxWindow::~BlackboxWindow\n", __FILE__);

  XGrabServer(display);

  delete window_menu;
  if (icon)
    delete icon;

  session->removeWindow(this);
  if (transient && client.transient_for)
    client.transient_for->client.transient = 0;
  
  if (frame.close_button != None) {
    XDeleteContext(display, frame.close_button, session->winContext());
    XDestroyWindow(display, frame.close_button);
  }
  if (frame.iconify_button != None) {
    XDeleteContext(display, frame.iconify_button, session->winContext());
    XDestroyWindow(display, frame.iconify_button);
  }
  if (frame.maximize_button != None) {
    XDeleteContext(display, frame.maximize_button, session->winContext());
    XDestroyWindow(display, frame.maximize_button);
  }

  if (frame.title != None) {
    if (frame.ftitle) XFreePixmap(display, frame.ftitle);
    if (frame.utitle) XFreePixmap(display, frame.utitle);
    XDeleteContext(display, frame.title, session->winContext());
    XDestroyWindow(display, frame.title);
  }
  if (frame.handle != None) {
    if (frame.fhandle) XFreePixmap(display, frame.fhandle);
    if (frame.uhandle) XFreePixmap(display, frame.uhandle);
    if (frame.rhandle) XFreePixmap(display, frame.rhandle);
    XDeleteContext(display, frame.handle, session->winContext());
    XDeleteContext(display, frame.resize_handle, session->winContext());
    XDestroyWindow(display, frame.resize_handle);
    XDestroyWindow(display, frame.handle);
  }

  if (frame.button != None) XFreePixmap(display, frame.button);
  if (frame.pbutton != None) XFreePixmap(display, frame.pbutton);

  XDeleteContext(display, client.window, session->winContext());
  XDeleteContext(display, frame.window, session->winContext());
  XDestroyWindow(display, frame.window);  

  if (client.app_name) XFree(client.app_name);
  if (client.app_class) XFree(client.app_class);
  if (client.title)
    if (strcmp(client.title, "Unnamed"))
      XFree(client.title);

  XFreeGC(display, frame.ftextGC);
  XFreeGC(display, frame.utextGC);

  XUngrabServer(display);

  debug->msg("%s: leaving BlackboxWindow::~BlackboxWindow\n", __FILE__);
  delete debug;
}


// *************************************************************************
// Window decoration code
// *************************************************************************

Window BlackboxWindow::createToplevelWindow(int x, int y, unsigned int width,
					    unsigned int height,
					    unsigned int borderwidth)
{
  debug->msg("%s: BlackboxWindow::createTopLevelWindow\n", __FILE__);

  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWOverrideRedirect |CWCursor|CWEventMask; 
  
  attrib_create.background_pixmap = None;
  attrib_create.background_pixel = session->frameColor().pixel;
  attrib_create.border_pixel = session->frameColor().pixel;
  attrib_create.override_redirect = True;
  attrib_create.cursor = session->sessionCursor();
  attrib_create.event_mask = SubstructureRedirectMask|ButtonPressMask|
    ButtonReleaseMask|ButtonMotionMask|ExposureMask|EnterWindowMask;
  
  return (XCreateWindow(display, session->Root(), x, y, width, height,
			borderwidth, session->Depth(), InputOutput,
			session->visual(), create_mask,
			&attrib_create));
}


Window BlackboxWindow::createChildWindow(Window parent, int x, int y,
					 unsigned int width,
					 unsigned int height,
					 unsigned int borderwidth)
{
  debug->msg("%s: BlackboxWindow::createChildWindow\n", __FILE__);

  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|CWCursor|
    CWEventMask;
  
  attrib_create.background_pixmap = None;
  attrib_create.background_pixel = 0l;
  attrib_create.border_pixel = 0l;
  attrib_create.cursor = session->sessionCursor();
  attrib_create.event_mask = KeyPressMask|KeyReleaseMask|ButtonPressMask|
    ButtonReleaseMask|ButtonMotionMask|ExposureMask|EnterWindowMask|
    LeaveWindowMask;
  
  return (XCreateWindow(display, parent, x, y, width, height, borderwidth,
			session->Depth(), InputOutput, session->visual(),
			create_mask, &attrib_create));
}


void BlackboxWindow::associateClientWindow(void) {
  debug->msg("%s: BlackboxWindow::associeateClientWindow\n", __FILE__);

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
    if (session->Orientation() == BlackboxSession::B_LeftHandedUser)
      XReparentWindow(display, client.window, frame.window,
		      ((do_handle) ? frame.handle_w + 1 : 0),
		      frame.title_h + 1);
    else
      XReparentWindow(display, client.window, frame.window, 0,
		      frame.title_h + 1);
  }

#ifdef SHAPE
  if (session->shapeExtensions()) {
    XShapeSelectInput(display, client.window, ShapeNotifyMask);

    int foo, bShaped;
    unsigned int ufoo;
    XShapeQueryExtents(display, client.window, &bShaped, &foo, &foo, &ufoo,
		       &ufoo, &foo, &foo, &foo, &ufoo, &ufoo);
    frame.shaped = bShaped;

    if (frame.shaped) {
      if (session->Orientation() == BlackboxSession::B_RightHandedUser)
        XShapeCombineShape(display, frame.window, ShapeBounding, 0,
  			   frame.title_h + 1, client.window, ShapeBounding,
			   ShapeSet);
      else
        XShapeCombineShape(display, frame.window, ShapeBounding,
			   frame.handle_w + 1, frame.title_h + 1,
			   client.window, ShapeBounding, ShapeSet);

      int num = 1;
      XRectangle xrect[2];
      xrect[0].x = xrect[0].y = 0;
      xrect[0].width = frame.title_w;
      xrect[0].height = frame.title_h + 1;

      if (do_handle) {
	if (session->Orientation() == BlackboxSession::B_RightHandedUser)
	  xrect[1].x = client.width + 1;
	else
	  xrect[1].x = 0;
	xrect[1].y = frame.title_h;
	xrect[1].width = frame.handle_w + 1;
	xrect[1].height = frame.handle_h + frame.button_h + 1;
	num++;
      }

      XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
			      xrect, num, ShapeUnion, Unsorted);
    }
  }
#endif

  XSaveContext(display, client.window, session->winContext(), (XPointer) this);
  createIconifyButton();
  createMaximizeButton();

  if (frame.button && frame.close_button)
    XSetWindowBackgroundPixmap(display, frame.close_button, frame.button);
  if (frame.button && frame.maximize_button)
    XSetWindowBackgroundPixmap(display, frame.maximize_button, frame.button);
  if (frame.button && frame.iconify_button)
    XSetWindowBackgroundPixmap(display, frame.iconify_button, frame.button);

  debug->msg("%s: leaving BlackboxWindow::associateClientWindow\n", __FILE__);
}


void BlackboxWindow::createDecorations(void) {
  debug->msg("%s: BlackboxWindow::createDecorations\n", __FILE__);
  
  BImage image(session, frame.title_w, frame.title_h, session->Depth(),
	       session->focusColor());
  frame.ftitle = image.renderImage(session->windowTexture(), 1,
				   session->focusColor(),
				   session->focusToColor());
  frame.utitle = image.renderImage(session->windowTexture(), 1,
				   session->unfocusColor(),
				   session->unfocusToColor());
  
  XSetWindowBackgroundPixmap(display, frame.title, frame.utitle);
  XClearWindow(display, frame.title);
  
  if (do_handle) {
    BImage h_image(session, frame.handle_w, frame.handle_h,
		   session->Depth(), session->focusColor());
    frame.fhandle = h_image.renderImage(session->windowTexture(), 1,
					session->focusColor(),
					session->focusToColor());
    frame.uhandle = h_image.renderImage(session->windowTexture(), 1,
					session->unfocusColor(),
					session->unfocusToColor());
    
    XSetWindowBackground(display, frame.handle, frame.uhandle);
    XClearWindow(display, frame.handle);
    
    BImage rh_image(session, frame.handle_w, frame.button_h,
		    session->Depth(), session->buttonColor());
    frame.rhandle = rh_image.renderImage(session->buttonTexture(), 0,
					 session->buttonColor(),
					 session->buttonToColor());
     
    XSetWindowBackgroundPixmap(display, frame.resize_handle, frame.rhandle);
    XClearWindow(display, frame.resize_handle);
  }
  
  BImage button_image(session, frame.button_w, frame.button_h,
		      session->Depth(), session->buttonColor());
  frame.button = button_image.renderImage(session->buttonTexture(), 0,
					  session->buttonColor(),
					  session->buttonToColor());
  frame.pbutton = button_image.renderInvertedImage(session->buttonTexture(), 0,
						   session->buttonColor(),
						   session->buttonToColor());

  XGCValues gcv;
  gcv.foreground = session->unfocusTextColor().pixel;
  gcv.font = session->titleFont()->fid;
  frame.utextGC = XCreateGC(display, frame.window, GCForeground|GCBackground|
			    GCFont, &gcv);

  gcv.foreground = session->focusTextColor().pixel;
  gcv.font = session->titleFont()->fid;
  frame.ftextGC = XCreateGC(display, frame.window, GCForeground|GCBackground|
			    GCFont, &gcv);

  debug->msg("%s: leaving BlackboxWindow::createDecorations\n", __FILE__);
}


void BlackboxWindow::positionButtons(void) {
  if (session->Orientation() == BlackboxSession::B_LeftHandedUser) {
    int mx = 3, my = 3;
    
    if (do_iconify && frame.iconify_button != None) {
      XMoveWindow(display, frame.iconify_button, mx, my);
      XMapWindow(display, frame.iconify_button);
      XClearWindow(display, frame.iconify_button);
      mx = mx + frame.button_w + 4;
    }
    
    if (do_maximize && frame.maximize_button != None) {
      
      XMoveWindow(display, frame.maximize_button, mx, my);
      XMapWindow(display, frame.maximize_button);
      XClearWindow(display, frame.maximize_button);
      mx = mx + frame.button_w + 4;
    }
    
    if (do_close && frame.close_button != None) {
      XMoveWindow(display, frame.close_button, mx, my);
      XMapWindow(display, frame.close_button);
      XClearWindow(display, frame.close_button);
    }
  } else {
    int mx = frame.title_w - frame.button_w - 5, my = 3;
    
    if (do_close && frame.close_button != None) {
      XMoveWindow(display, frame.close_button, mx, my);
      XMapWindow(display, frame.close_button);
      XClearWindow(display, frame.close_button);
      mx = mx - frame.button_w - 4;
    }

    if (do_maximize && frame.maximize_button != None) {
      
      XMoveWindow(display, frame.maximize_button, mx, my);
      XMapWindow(display, frame.maximize_button);
      XClearWindow(display, frame.maximize_button);
      mx = mx - frame.button_w - 4;
    }

    if (do_iconify && frame.iconify_button != None) {
      XMoveWindow(display, frame.iconify_button, mx, my);
      XMapWindow(display, frame.iconify_button);
      XClearWindow(display, frame.iconify_button);
    }
  }

  XClearWindow(display, frame.title);
}


void BlackboxWindow::createCloseButton(void) {
  if (do_close && frame.title != None && frame.close_button == None) {
    frame.close_button = createChildWindow(frame.title, 0, 0, frame.button_w,
					   frame.button_h, 1);
    if (frame.button != None)
      XSetWindowBackgroundPixmap(display, frame.close_button, frame.button);

    XSaveContext(display, frame.close_button, session->winContext(),
		 (XPointer) this);
  }
}


void BlackboxWindow::createIconifyButton(void) {
  if (do_iconify && frame.title != None) {
    frame.iconify_button =
      createChildWindow(frame.title, 0, 0, frame.button_w,
		    frame.button_h, 1);
    XSaveContext(display, frame.iconify_button, session->winContext(),
		 (XPointer) this);
  }
}


void BlackboxWindow::createMaximizeButton(void) {
  if (do_maximize && frame.title != None) {
    frame.maximize_button =
      createChildWindow(frame.title, 0, 0, frame.button_w, frame.button_h, 1);
    XSaveContext(display, frame.maximize_button, session->winContext(),
		 (XPointer) this);
  }
}


void BlackboxWindow::Reconfigure(void) {
  debug->msg("%s: BlackboxWindow::Reconfigure\n", __FILE__);

  XGrabServer(display);

  XGCValues gcv;
  gcv.foreground = session->unfocusTextColor().pixel;
  gcv.font = session->titleFont()->fid;
  XChangeGC(display, frame.utextGC, GCForeground|GCBackground|GCFont, &gcv);

  gcv.foreground = session->focusTextColor().pixel;
  gcv.font = session->titleFont()->fid;
  XChangeGC(display, frame.ftextGC, GCForeground|GCBackground|GCFont, &gcv);

  window_menu->Reconfigure();
  window_menu->updateMenu();

  if (session->Orientation() == BlackboxSession::B_RightHandedUser)
    client.x = frame.x + 1;
  else 
    client.x = frame.x + ((do_handle) ? frame.handle_w + 1 : 0) + 1;
  
  client.y = frame.y + frame.title_h + 2;
  client.width = frame.width - ((do_handle) ? (frame.handle_w + 1) : 0);
  frame.handle_h = client.height = frame.height - frame.title_h - 1;
  
  XGrabServer(display);
  XResizeWindow(display, frame.title, frame.title_w, frame.title_h);
  
  if (do_handle) {
    if (session->Orientation() == BlackboxSession::B_LeftHandedUser)
      XMoveResizeWindow(display, frame.handle, 0, frame.title_h + 1,
			frame.handle_w, frame.handle_h);
    else
      XMoveResizeWindow(display, frame.handle, client.width + 1,
			frame.title_h + 1, frame.handle_w, frame.handle_h);
    
    frame.handle_h -= frame.button_h;
    if (frame.handle_h <= 0) frame.handle_h = 1;
    XMoveWindow(display, frame.resize_handle, 0, frame.handle_h);
  }
  
  if (session->Orientation() == BlackboxSession::B_LeftHandedUser)
    XMoveResizeWindow(display, client.window,
		      ((do_handle) ? frame.handle_w + 1 : 0),
		      frame.title_h + 1, client.width, client.height);
  else
    XMoveResizeWindow(display, client.window, 0, frame.title_h + 1,
		      client.width, client.height);
  
#ifdef SHAPE
  if (session->shapeExtensions()) {
    if (frame.shaped) {
      XShapeCombineShape(display, frame.window, ShapeBounding, 0,
			 frame.title_h + 1, client.window, ShapeBounding,
			 ShapeSet);
      
      int num = 1;
      XRectangle xrect[2];
      xrect[0].x = xrect[0].y = 0;
      xrect[0].width = frame.title_w;
      xrect[0].height = frame.title_h;
      
      if (do_handle) {
	if (session->Orientation() == BlackboxSession::B_RightHandedUser)
	  xrect[1].x = client.width + 1;
	else
	  xrect[1].x = 0;
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

  BImage button_image(session, frame.button_w, frame.button_h,
		      session->Depth(), session->buttonColor());
  frame.button = button_image.renderImage(session->buttonTexture(), 0,
					  session->buttonColor(),
					  session->buttonToColor());
  frame.pbutton = button_image.renderInvertedImage(session->buttonTexture(), 0,
						   session->buttonColor(),
						   session->buttonToColor());
  positionButtons();
  
  if (frame.ftitle) XFreePixmap(display, frame.ftitle);
  if (frame.utitle) XFreePixmap(display, frame.utitle);
  
  BImage image(session, frame.title_w, frame.title_h, session->Depth(),
	       session->focusColor());
  frame.ftitle = image.renderImage(session->windowTexture(), 1,
				   session->focusColor(),
				   session->focusToColor());
  frame.utitle = image.renderImage(session->windowTexture(), 1,
				   session->unfocusColor(),
				   session->unfocusToColor());
  
  if (do_handle) {
    if (frame.fhandle) XFreePixmap(display, frame.fhandle);
    if (frame.uhandle) XFreePixmap(display, frame.uhandle);
    
    BImage h_image(session, frame.handle_w, frame.handle_h,
		   session->Depth(), session->focusColor());
    frame.fhandle = h_image.renderImage(session->windowTexture(), 1,
					session->focusColor(),
					session->focusToColor());
    frame.uhandle = h_image.renderImage(session->windowTexture(), 1,
					session->unfocusColor(),
					session->unfocusToColor());

    if (frame.rhandle) XFreePixmap(display, frame.rhandle);
    BImage rh_image(session, frame.handle_w, frame.button_h,
		    session->Depth(), session->buttonColor());
    frame.rhandle = rh_image.renderImage(session->buttonTexture(), 0,
					 session->buttonColor(),
					 session->buttonToColor());
     
    XSetWindowBackgroundPixmap(display, frame.resize_handle, frame.rhandle);
    XClearWindow(display, frame.resize_handle);
  }
    
  XSetWindowBorder(display, frame.window, session->frameColor().pixel);
  XSetWindowBackground(display, frame.window, session->frameColor().pixel);
  XClearWindow(display, frame.window);
  setFocusFlag(focused);
  drawTitleWin(0, 0, 0, 0);
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

  debug->msg("%s: leaving BlackboxWindow::Reconfigure\n", __FILE__);
}


// *************************************************************************
// Window protocol and ICCCM code
// *************************************************************************

Bool BlackboxWindow::getWMProtocols(void) {
  debug->msg("%s: BlackboxWindow::getWMProtocols\n", __FILE__);

  Atom *protocols;
  int num_return = 0;
  XGetWMProtocols(display, client.window, &protocols, &num_return);
  
  do_close = False;
  for (int i = 0; i < num_return; ++i) {
    if (protocols[i] == session->DeleteAtom()) {
      do_close = True;
      Protocols.WM_DELETE_WINDOW = True;
      client.WMDelete = session->DeleteAtom();
      createCloseButton();
      positionButtons();
    } else if (protocols[i] ==  session->FocusAtom()) {
      Protocols.WM_TAKE_FOCUS = True;
    } else if (protocols[i] ==  session->StateAtom()) {
      Atom atom;
      int foo;
      unsigned long ulfoo, nitems;
      unsigned char *state;
      XGetWindowProperty(display, client.window, session->StateAtom(),
			 0, 3, False, session->StateAtom(), &atom, &foo,
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
    } else  if (protocols[i] ==  session->ColormapAtom()) {
      debug->msg("\t[ wm colormap windows - unsupported ]\n");
    }
  }
    
  debug->msg("%s: BlackboxWindow::getWMProtocols\n", __FILE__);
  return True;
}


Bool BlackboxWindow::getWMHints(void) {
  debug->msg("%s: BlackboxWindow::getWMHints\n", __FILE__);

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
    if (! session->Startup())
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
  
  debug->msg("%s: leaving BlackboxWindow::getWMHints\n", __FILE__);
  return True;
}


Bool BlackboxWindow::getWMNormalHints(XSizeHints *hint) {
  debug->msg("%s: BlackboxWindow::getWMNormalHints\n", __FILE__);

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
    client.max_w = session->XResolution();
    client.max_h = session->YResolution();
  }
  
  if (client.hint_flags & PAspect) {
    client.min_ax = hint->min_aspect.x;
    client.min_ay = hint->min_aspect.y;
    client.max_ax = hint->max_aspect.x;
    client.max_ay = hint->max_aspect.y;
  }

  debug->msg("%s: leaving BlackboxWindow::getWMNormalHints\n");
  return True;
}


// *************************************************************************
// Window utility function code
// *************************************************************************

void BlackboxWindow::configureWindow(int dx, int dy, unsigned int dw,
				     unsigned int dh) {
  debug->msg("%s: BlackboxWindow::configureWindow\n", __FILE__);

  //
  // configure our client and decoration windows according to the frame size
  // changes passed as the arguments
  //

  Bool resize, synth_event;
  if (dw > (client.max_w + ((do_handle) ? frame.handle_w + 1 : 0)))
    dw = client.max_w + ((do_handle) ? frame.handle_w + 1 : 0);
  if (dh > (client.max_h + frame.title_h + 1))
    dh = client.max_h + frame.title_h + 1;
  
  if ((((int) frame.width) + dx) < 0) dx = 0;
  if ((((int) frame.title_h) + dy) < 0) dy = 0;
  
  resize = ((dw != frame.width) || (dh != frame.height));
  synth_event = ((dx != frame.x || dy != frame.y) && (! resize));

  if (resize) {
    frame.x = dx;
    frame.y = dy;
    frame.width = dw;
    frame.height = dh;
    frame.title_w = frame.width;
    frame.handle_h = dh - frame.title_h - 1;
    if (session->Orientation() == BlackboxSession::B_RightHandedUser)
      client.x = dx + 1;
    else 
      client.x = dx + ((do_handle) ? frame.handle_w + 1 : 0) + 1;

    client.y = dy + frame.title_h + 2;
    client.width = dw - ((do_handle) ? (frame.handle_w + 1) : 0);
    client.height = dh - frame.title_h - 1;

    XWindowChanges xwc;
    xwc.x = dx;
    xwc.y = dy;
    xwc.width = dw;
    xwc.height = dh;
    XGrabServer(display);
    XConfigureWindow(display, frame.window, CWX|CWY|CWWidth|CWHeight, &xwc);
    XResizeWindow(display, frame.title, frame.title_w, frame.title_h);

    if (do_handle) {
      if (session->Orientation() == BlackboxSession::B_LeftHandedUser)
	XMoveResizeWindow(display, frame.handle, 0, frame.title_h + 1,
			  frame.handle_w, frame.handle_h);
      else
	XMoveResizeWindow(display, frame.handle, client.width + 1,
			  frame.title_h + 1, frame.handle_w, frame.handle_h);

      frame.handle_h -= frame.button_h;
      if (frame.handle_h <= 0) frame.handle_h = 1;
      XMoveWindow(display, frame.resize_handle, 0, frame.handle_h);
    }

    if (session->Orientation() == BlackboxSession::B_LeftHandedUser)
      XMoveResizeWindow(display, client.window,
			((do_handle) ? frame.handle_w + 1 : 0),
			frame.title_h + 1, client.width, client.height);
    else
      XMoveResizeWindow(display, client.window, 0, frame.title_h + 1,
			client.width, client.height);

    positionButtons();

    if (frame.ftitle) XFreePixmap(display, frame.ftitle);
    if (frame.utitle) XFreePixmap(display, frame.utitle);

    BImage image(session, frame.title_w, frame.title_h,
		 session->Depth(), session->focusColor());
    frame.ftitle = image.renderImage(session->windowTexture(), 1,
				     session->focusColor(),
				     session->focusToColor());
    frame.utitle = image.renderImage(session->windowTexture(), 1,
				     session->unfocusColor(),
				     session->unfocusToColor());
    
    if (do_handle) {
      if (frame.fhandle) XFreePixmap(display, frame.fhandle);
      if (frame.uhandle) XFreePixmap(display, frame.uhandle);
      
      BImage h_image(session, frame.handle_w, frame.handle_h,
		     session->Depth(), session->focusColor());
      frame.fhandle = h_image.renderImage(session->windowTexture(), 1,
					  session->focusColor(),
					  session->focusToColor());
      frame.uhandle = h_image.renderImage(session->windowTexture(), 1,
					  session->unfocusColor(),
					  session->unfocusToColor());
    }
    
    setFocusFlag(focused);
    drawTitleWin(0, 0, 0, 0);
    drawAllButtons();
    XUngrabServer(display);
  } else {
    frame.x = dx;
    frame.y = dy;
    if (session->Orientation() == BlackboxSession::B_RightHandedUser)
      client.x = dx + 1;
    else 
      client.x = dx + ((do_handle) ? frame.handle_w + 1 : 0) + 1;

    client.y = dy + frame.title_h + 2;
   
    XWindowChanges xwc;
    xwc.x = dx;
    xwc.y = dy;
    XConfigureWindow(display, frame.window, CWX|CWY, &xwc);
  }

  if (synth_event) {
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

  debug->msg("%s: leaving BlackboxWindow::configureWindow\n", __FILE__);
}


Bool BlackboxWindow::setInputFocus(void) {
  debug->msg("%s: BlackboxWindow::setInputFocus\n", __FILE__);

  if (((signed) (frame.x + frame.width)) < 0) {
    if (((signed) (frame.y + frame.title_h)) < 0)
      configureWindow(0, 0, frame.width, frame.height);
    else if (frame.y > (signed) session->YResolution())
      configureWindow(0, session->YResolution() - frame.height, frame.width,
		      frame.height);
    else
      configureWindow(0, frame.y, frame.width, frame.height);
  } else if (frame.x > (signed) session->XResolution()) {
    if (((signed) (frame.y + frame.title_h)) < 0)
      configureWindow(session->XResolution() - frame.width, 0, frame.width,
		      frame.height);
    else if (frame.y > (signed) session->YResolution())
      configureWindow(session->XResolution() - frame.width,
		      session->YResolution() - frame.height, frame.width,
		      frame.height);
    else
      configureWindow(session->XResolution() - frame.width, frame.y,
		      frame.width, frame.height);
  }


  switch (focus_mode) {
  case F_NoInput:
  case F_GloballyActive:
    break;
    
  case F_LocallyActive:
  case F_Passive:
    XSetWindowBackgroundPixmap(display, frame.title, frame.ftitle);
    XClearWindow(display, frame.title);

    if (do_handle) {
      XSetWindowBackgroundPixmap(display, frame.handle, frame.fhandle);
      XClearWindow(display, frame.handle);
    }

    XSetInputFocus(display, client.window, RevertToParent,
		   CurrentTime);

    drawTitleWin(0,0,0,0);
    return True;
    break;
  }

  return False;
}


void BlackboxWindow::iconifyWindow(void) {
  debug->msg("%s: BlackboxWindow::iconifyWindow\n", __FILE__);

  window_menu->hideMenu();
  menu_visible = False;

  XUnmapWindow(display, frame.window);
  visible = False;
  iconic = True;
    
  if (transient) {
    if (! client.transient_for->iconic)
      client.transient_for->iconifyWindow();
  } else
    icon = new BlackboxIcon(session, client.window);

  if (client.transient)
    if (! client.transient->iconic)
      client.transient->iconifyWindow();

  unsigned long state[2];
  state[0] = (unsigned long) IconicState;
  state[1] = (unsigned long) None;
  XChangeProperty(display, client.window, session->StateAtom(),
                  session->StateAtom(), 32, PropModeReplace,
		  (unsigned char *) state, 2);
}


void BlackboxWindow::deiconifyWindow(void) {
  debug->msg("%s: BlackboxWindow::deiconifyWindow\n", __FILE__);

  XMapWindow(display, frame.window);
  visible = True;
  iconic = False;

  if (client.transient) client.transient->deiconifyWindow();
  
  unsigned long state[2];
  state[0] = (unsigned long) NormalState;
  state[1] = (unsigned long) None;
  XChangeProperty(display, client.window, session->StateAtom(),
		  session->StateAtom(), 32, PropModeReplace,
		  (unsigned char *) state, 2);
  session->reassociateWindow(this);

  if (icon) {
    delete icon;
    removeIcon();
  }
}


void BlackboxWindow::closeWindow(void) {
  debug->msg("%s: BlackboxWindow::closeWindow\n", __FILE__);

  XEvent ce;
  ce.xclient.type = ClientMessage;
  ce.xclient.message_type = session->ProtocolsAtom();
  ce.xclient.display = display;
  ce.xclient.window = client.window;
  ce.xclient.format = 32;
  
  ce.xclient.data.l[0] = session->DeleteAtom();
  ce.xclient.data.l[1] = CurrentTime;
  ce.xclient.data.l[2] = 0L;
  ce.xclient.data.l[3] = 0L;
  
  XSendEvent(display, client.window, False, NoEventMask, &ce);
  XFlush(display);
}


void BlackboxWindow::withdrawWindow(void) {
  debug->msg("%s: BlackboxWindow::withdrawWindow\n", __FILE__);

  XGrabServer(display);
  XSync(display, False);
  
  unsigned long state[2];
  state[0] = (unsigned long) WithdrawnState;
  state[1] = (unsigned long) None;

  visible = False;
  XUnmapWindow(display, frame.window);
  window_menu->hideMenu();
  
  XEvent foo;
  if (! XCheckTypedWindowEvent(display, client.window, DestroyNotify,
			       &foo)) {
    XChangeProperty(display, client.window, session->StateAtom(),
		    session->StateAtom(), 32, PropModeReplace,
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
  debug->msg("%s: BlackboxWindow::maximizeWindow\n", __FILE__);

  XGrabServer(display);

  static int px, py;
  static unsigned int pw, ph;

  if (maximized) {
    int dx, dy;
    unsigned int dw, dh;

    px = frame.x;
    py = frame.y;
    pw = frame.width;
    ph = frame.height;

    dw = session->XResolution() - session->WSManager()->Width() -
      ((do_handle) ? frame.handle_w + 1 : 0) - 1;
    dw -= client.base_w;
    dw -= (dw % client.inc_w);
    dw -= client.inc_w;
    dw += ((do_handle) ? frame.handle_w + 1 : 0) + 1 +
      client.base_w;
    
    dx = (((session->XResolution() - session->WSManager()->Width()) - dw) / 2)
      + session->WSManager()->Width();

    dh = session->YResolution() - frame.title_h - 2;
    dh -= client.base_h;
    dh -= (dh % client.inc_h);
    dh -= client.inc_h;
    dh += frame.title_h + 2 + client.base_h;
    
    dy = ((session->YResolution()) - dh) / 2;

    maximized = True;
    configureWindow(dx, dy, dw, dh);
    session->raiseWindow(this);
  } else {
    configureWindow(px, py, pw, ph);
    maximized = False;
  }

  XUngrabServer(display);
}


void BlackboxWindow::shadeWindow(void) {
  if (shaded) {
    XResizeWindow(display, frame.window, frame.width, frame.height);
    shaded = False;
  } else {
    XResizeWindow(display, frame.window, frame.width, frame.title_h);
    shaded = True;
  }
}


void BlackboxWindow::setFocusFlag(Bool focus) {
  debug->msg("%s: BlackboxWindow::setFocusFlag\n", __FILE__);

  focused = focus;
  XSetWindowBackgroundPixmap(display, frame.title,
			     (focused) ? frame.ftitle : frame.utitle);
  XClearWindow(display, frame.title);
  
  if (do_handle) {
    XSetWindowBackgroundPixmap(display, frame.handle,
			       (focused) ? frame.fhandle : frame.uhandle);
    XClearWindow(display, frame.handle);
  }
  
  drawTitleWin(0, 0, 0, 0);
  drawAllButtons();
}


// *************************************************************************
// Window drawing code
// *************************************************************************

void BlackboxWindow::drawTitleWin(int /* ax */, int /* ay */, int /* aw */,
				  int /* ah */) {  
  int dx = frame.button_w + 4;
  
  if (session->Orientation() == BlackboxSession::B_LeftHandedUser) {
    if (do_close) dx += frame.button_w + 4;
    if (do_maximize) dx += frame.button_w + 4;
    if (do_iconify) dx += frame.button_w + 4;
  }
  
  XDrawString(display, frame.title,
	      ((focused) ? frame.ftextGC : frame.utextGC), dx,
	      session->titleFont()->ascent + 4, client.title,
	      strlen(client.title));
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

  XPoint pts[4];
  pts[0].x = (frame.button_w - 5) / 2;
  pts[0].y = frame.button_h - ((frame.button_h - 6) / 2) - 1;

  pts[1].x = 5; pts[1].y = 0;
  pts[2].x = 0; pts[2].y = -1;
  pts[3].x = -5; pts[3].y = 0;
  
  XDrawLines(display, frame.iconify_button,
	     ((focused) ? frame.ftextGC : frame.utextGC), pts, 4,
	     CoordModePrevious);
}


void BlackboxWindow::drawMaximizeButton(Bool pressed) {
  if (! pressed && frame.button) {
    XSetWindowBackgroundPixmap(display, frame.maximize_button, frame.button);
    XClearWindow(display, frame.maximize_button);
  } else if (frame.pbutton) {
    XSetWindowBackgroundPixmap(display, frame.maximize_button, frame.pbutton);
    XClearWindow(display, frame.maximize_button);
  }

  XPoint pts[7];
  pts[0].x = (frame.button_w - 5) / 2;
  pts[0].y = (frame.button_h - 6) / 2;

  pts[1].x = 5; pts[1].y = 0;
  pts[2].x = 0; pts[2].y = 1;
  pts[3].x = -5; pts[3].y = 0;
  pts[4].x = 0; pts[4].y = 5;
  pts[5].x = 5; pts[5].y = 0;
  pts[6].x = 0; pts[6].y = -4;

  XDrawLines(display, frame.maximize_button,
	     ((focused) ? frame.ftextGC : frame.utextGC), pts, 7,
	     CoordModePrevious);
}


void BlackboxWindow::drawCloseButton(Bool pressed) {
  if (! pressed && frame.button) {
    XSetWindowBackgroundPixmap(display, frame.close_button, frame.button);
    XClearWindow(display, frame.close_button);
  } else if (frame.pbutton) {
    XSetWindowBackgroundPixmap(display, frame.close_button, frame.pbutton);
    XClearWindow(display, frame.close_button);
  }

  XPoint pts[18];
  pts[0].x = (frame.button_w - 5) / 2;
  pts[0].y = (frame.button_h - 6) / 2;
  pts[1].x = 1; pts[1].y = 0;
  pts[2].x = 3; pts[2].y = 0;
  pts[3].x = 1; pts[3].y = 0;
  pts[4].x = -4; pts[4].y = 1;
  pts[5].x = 3; pts[5].y = 0;
  pts[6].x = -2; pts[6].y = 1;
  pts[7].x = 1; pts[7].y = 0;
  pts[8].x = -1; pts[8].y = 1;
  pts[9].x = 1; pts[9].y = 0;
  pts[10].x = -1; pts[10].y = 1;
  pts[11].x = 1; pts[11].y = 0;
  pts[12].x = -2; pts[12].y = 1;
  pts[13].x = 3; pts[13].y = 0;
  pts[14].x = -4; pts[14].y = 1;
  pts[15].x = 1; pts[15].y = 0;
  pts[16].x = 3; pts[16].y = 0;
  pts[17].x = 1; pts[17].y = 0;

  XDrawPoints(display, frame.close_button,
	      ((focused) ? frame.ftextGC : frame.utextGC), pts, 18,
	      CoordModePrevious);
}


// *************************************************************************
// Window event code
// *************************************************************************

void BlackboxWindow::mapRequestEvent(XMapRequestEvent *re) {
  debug->msg("%s: BlackboxWindow::mapRequestEvent\n", __FILE__);

  if (re->window == client.window) {
    if (visible && ! iconic) {
      XGrabServer(display);
      
      unsigned long state[2];
      state[0] = (unsigned long) NormalState;
      state[1] = (unsigned long) None;
      XChangeProperty(display, client.window, session->StateAtom(),
		      session->StateAtom(), 32, PropModeReplace,
		      (unsigned char *) state, 2);

      XMapSubwindows(display, frame.window);
      XMapWindow(display, frame.window);
      setFocusFlag(False);
      drawTitleWin(0, 0, 0, 0);
      drawAllButtons();
      XUngrabServer(display);
    }
  }

  debug->msg("%s: leaving BlackboxWindow::mapRequestEvent\n", __FILE__);
}


void BlackboxWindow::mapNotifyEvent(XMapEvent *ne) {
  debug->msg("%s: BlackboxWindow::mapNotifyEvent\n", __FILE__);

  if (ne->window == client.window && (! ne->override_redirect)) {
    if (visible && ! iconic) {
      XGrabServer(display);
      
      getWMProtocols();
      positionButtons();

      unsigned long state[2];
      state[0] = (unsigned long) NormalState;
      state[1] = (unsigned long) None;
      XChangeProperty(display, client.window, session->StateAtom(),
		      session->StateAtom(), 32, PropModeReplace,
		      (unsigned char *) state, 2);
      
      visible = True;
      iconic = False;
      XMapSubwindows(display, frame.window);
      XMapWindow(display, frame.window);
      drawTitleWin(0, 0, 0, 0);
      drawAllButtons();
      XUngrabServer(display);
    }
  }

  debug->msg("%s: leaving BlackboxWindow::mapNotifyEvent\n", __FILE__);
}


void BlackboxWindow::unmapNotifyEvent(XUnmapEvent *ue) {
  debug->msg("%s: BlackboxWindow::unmapNotifyEvent\n", __FILE__);

  if (ue->window == client.window) {
    XGrabServer(display);
    XSync(display, False);

    unsigned long state[2];
    state[0] = (unsigned long) ((iconic) ? IconicState : WithdrawnState);
    state[1] = (unsigned long) None;

    visible = False;
    XUnmapWindow(display, frame.window);

    XEvent foo;
    if (! XCheckTypedWindowEvent(display, client.window, DestroyNotify,
				 &foo)) {
      XReparentWindow(display, client.window, session->Root(), client.x,
		      client.y);
      
      XChangeProperty(display, client.window, session->StateAtom(),
		      session->StateAtom(), 32, PropModeReplace,
		      (unsigned char *) state, 2);
    }
    
    XUngrabServer(display);
    delete this;
  }

  debug->msg("%s: leaving BlackboxWindow::unmapNotifyEvent\n", __FILE__);
}


void BlackboxWindow::destroyNotifyEvent(XDestroyWindowEvent *de) {
  debug->msg("%s: BlackboxWindow::destroyNotifyEvent\n", __FILE__);

  if (de->window == client.window)
    delete this;
}


void BlackboxWindow::propertyNotifyEvent(Atom atom) {
  debug->msg("%s: BlackboxWindow::propertyNotifyEvent\n", __FILE__);

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
    drawTitleWin(0, 0, frame.title_w, frame.title_h);
    session->updateWorkspace(workspace_number);
    break;
    
  case XA_WM_NORMAL_HINTS: {
    XSizeHints sizehint;
    getWMNormalHints(&sizehint);
    break;
  }
    
  case XA_WM_TRANSIENT_FOR:
    break;
    
  default:
    if (atom == session->ProtocolsAtom())
      getWMProtocols();
  }

  debug->msg("%s: leaving BlackboxWindow::propertyNotifyEvent\n", __FILE__);
}


void BlackboxWindow::exposeEvent(XExposeEvent *ee) {
  if (ee->window == frame.title)
    drawTitleWin(ee->x, ee->y, ee->width, ee->height);
  else if (do_close && frame.close_button == ee->window)
    drawCloseButton(False);
  else if (do_maximize && frame.maximize_button == ee->window)
    drawMaximizeButton(False);
  else if (do_iconify && frame.iconify_button == ee->window)
    drawIconifyButton(False);
}


void BlackboxWindow::configureRequestEvent(XConfigureRequestEvent *cr) {  
  debug->msg("%s: BlackboxWindow::configureRequestEvent\n", __FILE__);

  if (cr->window == client.window) {
    int cx, cy;
    unsigned int cw, ch;
    
    if (cr->value_mask & CWX) cx = cr->x - 1;
    else cx = frame.x;
    if (cr->value_mask & CWY) cy = cr->y - 1;
    else cy = frame.y;
    if (cr->value_mask & CWWidth)
      cw = cr->width + ((do_handle) ? frame.handle_w + 1 : 0);
    else cw = frame.width;
    if (cr->value_mask & CWHeight) ch = cr->height + frame.title_h + 1;
    else ch = frame.height;
    
    configureWindow(cx, cy, cw, ch);
  }

  debug->msg("%s: leaving BlackboxWindow::configureRequestEvent\n", __FILE__);
}


void BlackboxWindow::buttonPressEvent(XButtonEvent *be) {
  debug->msg("%s: BlackboxWindow::buttonPressEvent\n", __FILE__);

  if (session->button1Pressed()) {
    if (frame.title == be->window || frame.handle == be->window)
      session->raiseWindow(this);
    else if (frame.iconify_button == be->window)
      drawIconifyButton(True);
    else if (frame.maximize_button == be->window)
      drawMaximizeButton(True);
    else if (frame.close_button == be->window)
      drawCloseButton(True);
    else if (frame.resize_handle == be->window)
      session->raiseWindow(this);
  } else if (session->button2Pressed()) {
    if (frame.title == be->window || frame.handle == be->window) {
      session->lowerWindow(this);
    }
  } else if (session->button3Pressed()) {
    if (frame.title == be->window) {
      if (! menu_visible) {
	window_menu->moveMenu(be->x_root - (window_menu->Width() / 2),
			      frame.y + frame.title_h);
	XRaiseWindow(display, window_menu->windowID());
	window_menu->showMenu();
      } else
	window_menu->hideMenu();
    }
  }

  debug->msg("%s: leaving BlackboxWindow::buttonPressEvent\n", __FILE__);
}


void BlackboxWindow::buttonReleaseEvent(XButtonEvent *re) {
  debug->msg("%s: BlackboxWindow::buttonReleaseEvent\n", __FILE__);

  if (re->button == 1) {
    if (re->window == frame.title) {
      if (moving) {
	configureWindow(frame.x, frame.y, frame.width, frame.height);
	moving = False;
	XUngrabPointer(display, CurrentTime);
      } else if ((re->state&ControlMask))
	shadeWindow();
    } else if (resizing) {
      XDrawLine(display, session->Root(), session->GCOperations(),
		frame.x + 1, frame.y + frame.title_h, frame.x_resize + 1,
		frame.y + frame.title_h);
      XDrawLine(display, session->Root(), session->GCOperations(),
		frame.x_resize - frame.handle_w, frame.y + frame.title_h + 1,
		frame.x_resize - frame.handle_w, frame.y_resize + 1);
      XDrawLine(display, session->Root(), session->GCOperations(),
		frame.x + 1, frame.y + 1, frame.x_resize + 1, frame.y + 1);
      XDrawLine(display, session->Root(), session->GCOperations(),
		frame.x_resize + 1, frame.y + 1, frame.x_resize + 1,
		frame.y_resize + 1);
      XDrawLine(display, session->Root(), session->GCOperations(),
		frame.x + 1, frame.y_resize + 1, frame.x_resize + 1,
		frame.y_resize + 1);
      XDrawLine(display, session->Root(), session->GCOperations(),
		frame.x + 1, frame.y + 1, frame.x + 1, frame.y_resize + 1);
      
      int dw = frame.x_resize - frame.x , dh = frame.y_resize - frame.y;
      configureWindow(frame.x, frame.y, dw, dh);
      
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

  debug->msg("%s: leaving BlackboxWindow::buttonReleaseEvent\n", __FILE__);
}


void BlackboxWindow::motionNotifyEvent(XMotionEvent *me) {
  if (frame.title == me->window) {
    if (session->button1Pressed()) {
      if (! moving) {
	if (menu_visible) {
	  window_menu->hideMenu();
	}

	if (XGrabPointer(display, frame.title, False, PointerMotionMask|
			 ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
			 None, session->moveCursor(), CurrentTime)
	    == GrabSuccess) {
	  moving = True;
	  client.x_move = me->x;
	  client.y_move = me->y;
	} else
	  moving = False;
      } else {
	int dx = me->x_root - client.x_move,
	  dy = me->y_root - client.y_move;

	configureWindow(dx, dy, frame.width, frame.height);
      }
    }
  } else if (me->window == frame.resize_handle) {
    if (session->button1Pressed()) {
      if (! resizing) {
	if (XGrabPointer(display, frame.resize_handle, False,
			 PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
			 GrabModeAsync, None, None, CurrentTime)
	    == GrabSuccess) {
	  resizing = True;

	  frame.x_resize = frame.x + frame.width;
	  frame.y_resize = frame.y + frame.height;

	  XDrawLine(display, session->Root(), session->GCOperations(),
		    frame.x + 1, frame.y + frame.title_h, frame.x_resize + 1,
		    frame.y + frame.title_h);
	  XDrawLine(display, session->Root(), session->GCOperations(),
		    frame.x_resize - frame.handle_w,
		    frame.y + frame.title_h + 1,
		    frame.x_resize - frame.handle_w, frame.y_resize + 1);
	  XDrawLine(display, session->Root(), session->GCOperations(),
		    frame.x + 1, frame.y + 1, frame.x_resize + 1, frame.y + 1);
	  XDrawLine(display, session->Root(), session->GCOperations(),
		    frame.x_resize + 1, frame.y + 1, frame.x_resize + 1,
		    frame.y_resize + 1);
	  XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y_resize + 1, frame.x_resize + 1,
		    frame.y_resize + 1);
	  XDrawLine(display, session->Root(), session->GCOperations(),
		    frame.x + 1, frame.y + 1, frame.x + 1, frame.y_resize + 1);
	} else
	  resizing = False;
      } else if (resizing) {
	int dx, dy;

	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y + frame.title_h, frame.x_resize + 1,
		  frame.y + frame.title_h);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x_resize - frame.handle_w, frame.y + frame.title_h + 1,
		  frame.x_resize - frame.handle_w, frame.y_resize + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y + 1, frame.x_resize + 1, frame.y + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x_resize + 1, frame.y + 1, frame.x_resize + 1,
		  frame.y_resize + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y_resize + 1, frame.x_resize + 1,
		  frame.y_resize + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y + 1, frame.x + 1, frame.y_resize + 1);

	
	if (! client.hint_flags & PResizeInc) {
	  frame.x_resize = me->x_root;
	  frame.y_resize = me->y_root;
	} else {
	  if (session->Orientation() == BlackboxSession::B_LeftHandedUser) {
	    // calculate new xposition from pointer position
	    int nx;
	    nx = me->x_root - frame.x;

	    dx = frame.x_resize - (frame.x + nx) - frame.handle_w - 1;
	    dy = me->y_root - frame.y + 1;
	    dx -= ((dx % client.inc_h) - client.base_w);
	    dy -= ((dy % client.inc_h) - client.base_h);

	    if (dx < (signed) client.min_w)
	      dx = client.min_w + frame.handle_w + 1;
	    if (dy < (signed) client.min_h) dy = client.min_h;

	    frame.x = frame.x_resize - dx + 3;
	    frame.y_resize = dy + frame.title_h + frame.y + 1;
	  } else {
	    // calculate the width of the client window...
	    dx = me->x_root - frame.x + 1;
	    dy = me->y_root - frame.y + 1;
	    
	    // conform the width to the size specified by the size hints of the
	    //   client window...
	    dx -= ((dx % client.inc_w) - client.base_w);
	    dy -= ((dy % client.inc_h) - client.base_h);
	    
	    if (dx < (signed) client.min_w) dx = client.min_w;
	    if (dy < (signed) client.min_h) dy = client.min_h;
	    
	    // add the appropriate frame decoration sizes before drawing the
	    //   resizing lines...
	    frame.x_resize = dx + frame.handle_w + 1;
	    frame.y_resize = dy + frame.title_h + 1;
	    frame.x_resize += frame.x;
	    frame.y_resize += frame.y;
	  }
	}
	
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y + frame.title_h, frame.x_resize + 1,
		  frame.y + frame.title_h);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x_resize - frame.handle_w, frame.y + frame.title_h + 1,
		  frame.x_resize - frame.handle_w, frame.y_resize + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y + 1, frame.x_resize + 1, frame.y + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x_resize + 1, frame.y + 1, frame.x_resize + 1,
		  frame.y_resize + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y_resize + 1, frame.x_resize + 1,
		  frame.y_resize + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y + 1, frame.x + 1, frame.y_resize + 1);
      }
    }
  }
}


#ifdef SHAPE

void BlackboxWindow::shapeEvent(XShapeEvent *) {
  if (session->shapeExtensions()) {
    if (frame.shaped) {
      XShapeCombineShape(display, frame.window, ShapeBounding, 0,
			 frame.title_h + 1, client.window, ShapeBounding,
			 ShapeSet);
      
      int num = 1;
      XRectangle xrect[2];
      xrect[0].x = xrect[0].y = 0;
      xrect[0].width = frame.title_w;
      xrect[0].height = frame.title_h;
      
      if (do_handle) {
	if (session->Orientation() == BlackboxSession::B_RightHandedUser)
	  xrect[1].x = client.width + 1;
	else
	  xrect[1].x = 0;
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
