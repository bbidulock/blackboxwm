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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "Windowmenu.hh"
#include "Workspace.hh"

#include "blackbox.hh"
#include "icon.hh"
#include "Window.hh"

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <string.h>


// *************************************************************************
// Window class code
// *************************************************************************

BlackboxWindow::BlackboxWindow(Blackbox *ctrl, Window window) {
  // set control members for operation
  protocols.WM_TAKE_FOCUS = True;
  protocols.WM_DELETE_WINDOW = False;
  moving = resizing = shaded = maximized = visible = iconic = transient =
    focused = stuck = False;

  window_number = -1;
  
  blackbox = ctrl;
  display = ctrl->control();
  client.window = window;

  frame.window = frame.title = frame.handle = frame.border =
    frame.close_button = frame.iconify_button = frame.maximize_button =
    frame.fbutton = frame.ubutton = frame.pbutton = frame.resize_handle = None;
  
  client.normal_hint_flags = client.wmhint_flags = client.initial_state = 0;
  client.transient_for = client.transient = 0;
  client.title = 0;
  icon = 0;

  // fetch client size and placement
  XWindowAttributes wattrib;
  if (! validateClient()) return;
  if (! XGetWindowAttributes(display, client.window, &wattrib)) return;
  if (wattrib.override_redirect) return;
  client.x = wattrib.x;
  client.y = wattrib.y;
  client.width = wattrib.width;
  client.height = wattrib.height;

  if (! validateClient()) return;
  blackbox->saveWindowSearch(client.window, this);

  // get size, aspect, minimum/maximum size and other hints set by the
  // client
  if (! getWMHints()) return;
  XSizeHints sizehint;
  if (! getWMNormalHints(&sizehint)) return;

  // determine if this is a transient window
  Window win;
  if (! validateClient()) return;
  if (XGetTransientForHint(display, client.window, &win))
    if (win && (win != client.window))
      if ((client.transient_for = blackbox->searchWindow(win))
	  != NULL) {
	client.transient_for->client.transient = this;	  
	transient = True;
      } else if (win == client.window_group) {
	if ((client.transient_for = blackbox->searchGroup(win, this))
	    != NULL) {
	  client.transient_for->client.transient = this;
	  transient = True;
	}
      }
  
  if (! transient) {
    resizable = True;
    
    if ((client.normal_hint_flags & PMinSize) &&
	(client.normal_hint_flags & PMaxSize))
      if (client.max_w == client.min_w && client.max_h == client.min_h)
	resizable = False;
  } else
    resizable = False;

#ifdef MWM_HINTS

  // fixme

#endif // MWM_HINTS
  
  frame.bevel_w = blackbox->bevelWidth();
  frame.border_w = ((transient) ? 0 : client.width + (frame.bevel_w * 2));
  frame.border_h = ((transient) ? 0 : client.height + (frame.bevel_w * 2));
  frame.title_h = blackbox->titleFont()->ascent +
    blackbox->titleFont()->descent + (frame.bevel_w * 2);

  frame.button_h = frame.title_h - 6;
  frame.button_w = frame.button_h;

  frame.rh_w = ((resizable) ? blackbox->handleWidth() : 0);
  frame.rh_h = ((resizable) ? frame.button_h : 0);
  frame.handle_w = ((resizable) ? blackbox->handleWidth() : 0);
  frame.handle_h = ((resizable) ? frame.border_h - frame.rh_h - 1 : 0);

  frame.title_w = ((transient) ? client.width :
		   frame.border_w + ((resizable) ? frame.handle_w + 1: 0));
  
  frame.width = frame.title_w;
  frame.height = ((transient) ? client.height + frame.title_h + 1 :
		  frame.border_h + frame.title_h + 1);
  
  
  static unsigned int cx = 32, cy = 32;
  Bool cascade = True;

  if (blackbox->Startup() || transient ||
      (client.normal_hint_flags & (PPosition|USPosition))) { 
    frame.x = client.x - ((transient) ? 1 : (frame.bevel_w + 2));
    frame.y = client.y - frame.title_h - ((transient) ? 1 :
					  (frame.bevel_w + 2));

    if (frame.x >= 0 && frame.y >= 0)
      cascade = False;
  }

  if (cascade) {
    if (cx > (blackbox->XResolution() / 2)) { cx = 32; cy = 32; }
    if (cy > (blackbox->YResolution() / 2)) { cx = 32; cy = 32; }
    
    frame.x = cx;
    frame.y = cy;
    
    cy += frame.title_h;
    cx += frame.title_h;
  }

  if (! validateClient()) return;

  frame.window = createToplevelWindow(frame.x, frame.y, frame.width,
				      frame.height, 1);
  blackbox->saveWindowSearch(frame.window, this);

  frame.title = createChildWindow(frame.window, 0, 0, frame.title_w,
				  frame.title_h, 0l);
  blackbox->saveWindowSearch(frame.title, this);

  if (! transient) {
    frame.border = createChildWindow(frame.window, 0, frame.title_h + 1,
				     frame.border_w, frame.border_h, 0l);
    blackbox->saveWindowSearch(frame.border, this);

    if (resizable) {
      frame.handle = createChildWindow(frame.window, frame.border_w + 1,
				       frame.title_h + 1, frame.handle_w,
				       frame.handle_h, 0l);
      blackbox->saveWindowSearch(frame.handle, this);
      
      frame.resize_handle = createChildWindow(frame.window, frame.border_w + 1,
					      frame.title_h +
					      frame.handle_h + 2,
					      frame.rh_w, frame.rh_h, 0l);
      blackbox->saveWindowSearch(frame.resize_handle, this);
    }
  }

  protocols.WMProtocols = blackbox->ProtocolsAtom();
  if (! getWMProtocols()) return;
  associateClientWindow();
  positionButtons();

  XGrabKey(display, XKeysymToKeycode(display, XK_Tab), Mod1Mask,
           frame.window, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(display, XKeysymToKeycode(display, XK_Left), ControlMask,
           frame.window, True, GrabModeAsync, GrabModeAsync);
  XGrabKey(display, XKeysymToKeycode(display, XK_Right), ControlMask,
	   frame.window, True, GrabModeAsync, GrabModeAsync);

  if (! validateClient()) return;
  XLowerWindow(display, client.window);
  XMapSubwindows(display, frame.title);
  if (! transient)
    XMapSubwindows(display, frame.border);
  XMapSubwindows(display, frame.window);
  
  windowmenu = new Windowmenu(this, blackbox);

  createDecorations();
  if (! validateClient()) return;
  blackbox->workspaceManager()->currentWorkspace()->addWindow(this);

  setFocusFlag(False);
  configureWindow(frame.x, frame.y, frame.width, frame.height);
}  


BlackboxWindow::~BlackboxWindow(void) {
  blackbox->workspaceManager()->workspace(workspace_number)->
    removeWindow(this);
  
  if (windowmenu)
    delete windowmenu;
  if (icon)
    delete icon;
  
  if (client.window_group)
    blackbox->removeGroupSearch(client.window_group);

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
    if (frame.ftitle) {
      XFreePixmap(display, frame.ftitle);
      frame.ftitle = None;
    }

    if (frame.utitle) {
      XFreePixmap(display, frame.utitle);
      frame.utitle = None;
    }

    blackbox->removeWindowSearch(frame.title);
    XDestroyWindow(display, frame.title);
  }

  if (frame.handle != None) {
    if (frame.fhandle) {
      XFreePixmap(display, frame.fhandle);
      frame.fhandle = None;
    }

    if (frame.uhandle) {
      XFreePixmap(display, frame.uhandle);
      frame.uhandle = None;
    }

    blackbox->removeWindowSearch(frame.handle);
    blackbox->removeWindowSearch(frame.resize_handle);
    XDestroyWindow(display, frame.resize_handle);
    XDestroyWindow(display, frame.handle);
  }

  if (frame.border != None) {
    blackbox->removeWindowSearch(frame.border);
    XDestroyWindow(display, frame.border);
  }

  if (frame.fbutton != None) {
    XFreePixmap(display, frame.fbutton);
    frame.fbutton = None;
  }

  if (frame.ubutton != None) {
    XFreePixmap(display, frame.ubutton);
    frame.ubutton = None;
  }

  if (frame.pbutton != None) {
    XFreePixmap(display, frame.pbutton);
    frame.pbutton = None;
  }

  blackbox->removeWindowSearch(client.window);

  blackbox->removeWindowSearch(frame.window);  
  XDestroyWindow(display, frame.window);  

  if (client.title)
    if (strcmp(client.title, "Unnamed"))
      XFree(client.title);

  XFreeGC(display, frame.ftextGC);
  XFreeGC(display, frame.utextGC);
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
  attrib_create.background_pixel = attrib_create.border_pixel =
    blackbox->borderColor().pixel;
  attrib_create.override_redirect = True;
  attrib_create.cursor = blackbox->sessionCursor();
  attrib_create.event_mask = ButtonPressMask | EnterWindowMask;
  
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
    blackbox->borderColor().pixel;
  attrib_create.cursor = blackbox->sessionCursor();
  attrib_create.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask |
    ButtonReleaseMask | ExposureMask | EnterWindowMask | LeaveWindowMask |
    ButtonMotionMask;
  
  return (XCreateWindow(display, parent, x, y, width, height, borderwidth,
			blackbox->Depth(), InputOutput, blackbox->visual(),
			create_mask, &attrib_create));
}


void BlackboxWindow::associateClientWindow(void) {  
  XGrabServer(display);

  if (! validateClient()) return;
  XSetWindowBorderWidth(display, client.window, 0);
  if (! XFetchName(display, client.window, &client.title))
    client.title = "Unnamed";
  
  XSelectInput(display, client.window, PropertyChangeMask);

  if (! validateClient()) return;
  XChangeSaveSet(display, client.window, SetModeInsert);
  XSetWindowAttributes attrib_set;
  if (transient) {
    XSelectInput(display, frame.window, NoEventMask);
    XReparentWindow(display, client.window, frame.window, 0,
		    frame.title_h + 1);
    XSelectInput(display, frame.window, ButtonPressMask | EnterWindowMask |
	SubstructureRedirectMask);
  } else {
    XSelectInput(display, frame.border, NoEventMask);
    XReparentWindow(display, client.window, frame.border, frame.bevel_w,
		    frame.bevel_w);

    XSelectInput(display, frame.border, KeyPressMask | KeyReleaseMask |
		 ButtonPressMask | ButtonReleaseMask | ExposureMask | 
		 EnterWindowMask | LeaveWindowMask | SubstructureRedirectMask);
  }

  XFlush(display);

  attrib_set.event_mask = PropertyChangeMask | FocusChangeMask |
    StructureNotifyMask;
  attrib_set.do_not_propagate_mask = ButtonPressMask|ButtonReleaseMask;
  attrib_set.save_under = False;

  if (! validateClient()) return;
  XChangeWindowAttributes(display, client.window, CWEventMask|CWDontPropagate|
                          CWSaveUnder, &attrib_set);


#ifdef SHAPE
  if (blackbox->shapeExtensions()) {
    XShapeSelectInput(display, client.window, ShapeNotifyMask);
    
    int foo;
    unsigned int ufoo;

    if (! validateClient()) return;
    XShapeQueryExtents(display, client.window, &frame.shaped, &foo, &foo,
		       &ufoo, &ufoo, &foo, &foo, &foo, &ufoo, &ufoo);
    
    if (frame.shaped) {
      XShapeCombineShape(display, frame.window, ShapeBounding,
			 ((transient) ? 0: frame.bevel_w),
			 frame.title_h + ((transient) ? 1 :
					  (frame.bevel_w + 1)),
			 client.window, ShapeBounding, ShapeSet);
      
      int num = 1;
      XRectangle xrect[2];
      xrect[0].x = xrect[0].y = 0;
      xrect[0].width = frame.title_w;
      xrect[0].height = frame.title_h + 1;
      
      if (resizable) {
	xrect[1].x = frame.border_w + 1;
	xrect[1].y = frame.title_h + 1;
	xrect[1].width = frame.handle_w;
	xrect[1].height = frame.border_h;
	num++;
      }
      
      XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
			      xrect, num, ShapeUnion, Unsorted);
    }
  }
#endif
  
  createIconifyButton();
  createMaximizeButton();
  
  if (frame.ubutton && frame.close_button)
    XSetWindowBackgroundPixmap(display, frame.close_button, frame.ubutton);
  if (frame.ubutton && frame.maximize_button)
    XSetWindowBackgroundPixmap(display, frame.maximize_button, frame.ubutton);
  if (frame.ubutton && frame.iconify_button)
    XSetWindowBackgroundPixmap(display, frame.iconify_button, frame.ubutton);

  XUngrabServer(display);
}


void BlackboxWindow::createDecorations(void) {
  BImage *t_image = new BImage(blackbox, frame.title_w, frame.title_h,
			     blackbox->Depth());
  frame.ftitle = t_image->renderImage(blackbox->wDecorTexture(),
				      blackbox->wFColor(),
				      blackbox->wFColorTo());
  frame.utitle = t_image->renderImage(blackbox->wDecorTexture(),
				      blackbox->wUColor(),
				      blackbox->wUColorTo());
  delete t_image;

  if (! transient) {
    BImage *b_image = new BImage(blackbox, frame.border_w, frame.border_h,
				 blackbox->Depth());
    Pixmap p =
      b_image->renderSolidImage(blackbox->wFrameTexture()|BImageNoDitherSolid,
				blackbox->wFrameColor());
    
    delete b_image;
    XSetWindowBackgroundPixmap(display, frame.border, p);
    XClearWindow(display, frame.border);
    if (p) {
      XFreePixmap(display, p);
      p = None;
    }
    
    if (resizable) {
      BImage *h_image = new BImage(blackbox, frame.handle_w, frame.handle_h,
				   blackbox->Depth());
      frame.fhandle = h_image->renderImage(blackbox->wDecorTexture(),
					   blackbox->wFColor(),
					   blackbox->wFColorTo());
      frame.uhandle = h_image->renderImage(blackbox->wDecorTexture(),
					   blackbox->wUColor(),
					   blackbox->wUColorTo());
      delete h_image;
      
      BImage *rh_image = new BImage(blackbox, frame.rh_w, frame.rh_h,
				    blackbox->Depth());
      p = rh_image->renderImage(blackbox->wHandleTexture(),
				blackbox->wUColor(),
				blackbox->wUColorTo());
      delete rh_image;
      XSetWindowBackgroundPixmap(display, frame.resize_handle, p);
      XClearWindow(display, frame.resize_handle);
      if (p) {
	XFreePixmap(display, p);
	p = None;
      }
    }
  }
    
  BImage *b_image = new BImage(blackbox, frame.button_w, frame.button_h,
			       blackbox->Depth());
  frame.fbutton = b_image->renderImage(blackbox->wButtonTexture(),
				       blackbox->wFColor(),
				       blackbox->wFColorTo());
  frame.ubutton = b_image->renderImage(blackbox->wButtonTexture(),
				      blackbox->wUColor(),
				      blackbox->wUColorTo());
  frame.pbutton = b_image->renderInvertedImage(blackbox->wButtonTexture(),
					       blackbox->wFColor(),
					       blackbox->wFColorTo());
  delete b_image;

  XGCValues gcv;
  gcv.foreground = blackbox->wUTextColor().pixel;
  gcv.font = blackbox->titleFont()->fid;
  frame.utextGC = XCreateGC(display, frame.window, GCForeground|GCBackground|
			    GCFont, &gcv);

  gcv.foreground = blackbox->wFTextColor().pixel;
  gcv.font = blackbox->titleFont()->fid;
  frame.ftextGC = XCreateGC(display, frame.window, GCForeground|GCBackground|
			    GCFont, &gcv);
}


void BlackboxWindow::positionButtons(void) {
  if (frame.title_w > ((frame.button_w + 4) * 6)) {
    if (frame.iconify_button != None) {
      XMoveResizeWindow(display, frame.iconify_button, 2, 2, frame.button_w,
			frame.button_h);
      XMapWindow(display, frame.iconify_button);
      XClearWindow(display, frame.iconify_button);
    }
    
    int bx = frame.title_w - (frame.button_w + 4);

    if (protocols.WM_DELETE_WINDOW && frame.close_button != None) {
      XMoveResizeWindow(display, frame.close_button, bx, 2, frame.button_w,
			frame.button_h);
      bx -= (frame.button_w + 4);
      XMapWindow(display, frame.close_button);
      XClearWindow(display, frame.close_button);
    }

    if (resizable && frame.maximize_button != None) {
      XMoveResizeWindow(display, frame.maximize_button, bx, 2, frame.button_w,
			frame.button_h);
      XMapWindow(display, frame.maximize_button);
      XClearWindow(display, frame.maximize_button);
    }
  } else {
    if (frame.iconify_button) XUnmapWindow(display, frame.iconify_button);
    if (frame.maximize_button) XUnmapWindow(display, frame.maximize_button);
    if (frame.close_button) XUnmapWindow(display, frame.close_button);
  }
}


void BlackboxWindow::createCloseButton(void) {
  if (protocols.WM_DELETE_WINDOW && frame.title != None &&
      frame.close_button == None) {
    frame.close_button =
      createChildWindow(frame.title, 0, 0, frame.button_w, frame.button_h, 1);
    if (frame.ubutton != None)
      XSetWindowBackgroundPixmap(display, frame.close_button, frame.ubutton);
    blackbox->saveWindowSearch(frame.close_button, this);
  }
}


void BlackboxWindow::createIconifyButton(void) {
  if (frame.title != None) {
    frame.iconify_button =
      createChildWindow(frame.title, 0, 0, frame.button_w, frame.button_h, 1);
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
  XGCValues gcv;
  gcv.foreground = blackbox->wUTextColor().pixel;
  gcv.font = blackbox->titleFont()->fid;
  XChangeGC(display, frame.utextGC, GCForeground|GCBackground|GCFont, &gcv);

  gcv.foreground = blackbox->wFTextColor().pixel;
  gcv.font = blackbox->titleFont()->fid;
  XChangeGC(display, frame.ftextGC, GCForeground|GCBackground|GCFont, &gcv);
  
  frame.bevel_w = blackbox->bevelWidth();
  frame.border_w = ((transient) ? 0 : client.width + (frame.bevel_w * 2));
  frame.border_h = ((transient) ? 0 : client.height + (frame.bevel_w * 2));
  frame.title_h = blackbox->titleFont()->ascent +
    blackbox->titleFont()->descent + (frame.bevel_w * 2);
  
  frame.button_h = frame.title_h - 6;
  frame.button_w = frame.button_h;

  frame.rh_w = ((resizable) ? blackbox->handleWidth() : 0);
  frame.rh_h = ((resizable) ? frame.button_h : 0);
  frame.handle_w = ((resizable) ? blackbox->handleWidth() : 0);
  frame.handle_h = ((resizable) ? frame.border_h - frame.rh_h - 1 : 0);

  frame.title_w = ((transient) ? client.width :
		   frame.border_w + ((resizable) ? frame.handle_w + 1: 0));
  
  frame.width = frame.title_w;
  frame.height = ((transient) ? client.height + frame.title_h + 1 :
		  frame.border_h + frame.title_h + 1);

  if (windowmenu) {
    windowmenu->Reconfigure();
  } 
  
  client.x = frame.x + ((transient) ? 1 : (frame.bevel_w + 1));
  client.y = frame.y + frame.title_h + ((transient) ? 1 : (frame.bevel_w + 1));
  
  XResizeWindow(display, frame.window, frame.width,
                ((shaded) ? frame.title_h : frame.height));
  XResizeWindow(display, frame.title, frame.title_w, frame.title_h);
  
  if (! transient) {
    XMoveResizeWindow(display, frame.border, 0, frame.title_h + 1,
		      frame.border_w, frame.border_h);

    if (resizable) {
      XMoveResizeWindow(display, frame.handle, frame.border_w + 1,
			frame.title_h + 1,
			frame.handle_w, frame.handle_h);
      
      XMoveResizeWindow(display, frame.resize_handle, frame.border_w + 1,
			frame.handle_h + frame.title_h + 2,
			frame.rh_w, frame.rh_h);
    }
    
    XMoveResizeWindow(display, client.window, frame.bevel_w, frame.bevel_w,
		      client.width, client.height);
  } else
    XMoveResizeWindow(display, client.window, 0, frame.title_h + 1,
		      client.width, client.height);
  
#ifdef SHAPE
  if (blackbox->shapeExtensions()) {
    if (frame.shaped) {
      XShapeCombineShape(display, frame.window, ShapeBounding,
			 ((transient) ? 0 : frame.bevel_w),
			 frame.title_h + ((transient) ? 1:
					  (frame.bevel_w + 1)), client.window,
			 ShapeBounding, ShapeSet);
      
      int num = 1;
      XRectangle xrect[2];
      xrect[0].x = xrect[0].y = 0;
      xrect[0].width = frame.title_w;
      xrect[0].height = frame.title_h + 1;
      
      if (resizable) {
	xrect[1].x = frame.border_w + 1;
	xrect[1].y = frame.title_h + 1;
	xrect[1].width = frame.handle_w;
	xrect[1].height = frame.border_h;
	num++;
      }
      
      XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
			      xrect, num, ShapeUnion, Unsorted);
    }
  }
#endif

  if (frame.fbutton) {
    XFreePixmap(display, frame.fbutton);
    frame.fbutton = None;
  }

  if (frame.ubutton) {
    XFreePixmap(display, frame.ubutton);
    frame.ubutton = None;
  }

  if (frame.pbutton) {
    XFreePixmap(display, frame.pbutton);
    frame.pbutton = None;
  }

  BImage *b_image = new BImage(blackbox, frame.button_w, frame.button_h,
			       blackbox->Depth());
  frame.fbutton = b_image->renderImage(blackbox->wButtonTexture(),
				      blackbox->wFColor(),
				      blackbox->wFColorTo());
  frame.ubutton = b_image->renderImage(blackbox->wButtonTexture(),
				       blackbox->wUColor(),
				       blackbox->wUColorTo());
  frame.pbutton = b_image->renderInvertedImage(blackbox->wButtonTexture(),
					       blackbox->wFColor(),
					       blackbox->wFColorTo());
  delete b_image;

  if (frame.iconify_button) XSetWindowBorder(display, frame.iconify_button,
					     blackbox->borderColor().pixel);
  if (frame.maximize_button) XSetWindowBorder(display, frame.maximize_button,
					      blackbox->borderColor().pixel);
  if (frame.close_button) XSetWindowBorder(display, frame.close_button,
					   blackbox->borderColor().pixel);

  positionButtons();
  
  if (frame.ftitle) {
    XFreePixmap(display, frame.ftitle);
    frame.ftitle = None;
  }

  if (frame.utitle) {
    XFreePixmap(display, frame.utitle);
    frame.utitle = None;
  }
  
  BImage *t_image = new BImage(blackbox, frame.title_w, frame.title_h,
			      blackbox->Depth());
  frame.ftitle = t_image->renderImage(blackbox->wDecorTexture(),
				      blackbox->wFColor(),
				      blackbox->wFColorTo());
  frame.utitle = t_image->renderImage(blackbox->wDecorTexture(),
				      blackbox->wUColor(),
				      blackbox->wUColorTo());
  delete t_image;

  if (! transient) {
    BImage *b_image = new BImage(blackbox, frame.border_w, frame.border_h,
				 blackbox->Depth());
    Pixmap p =
      b_image->renderSolidImage(blackbox->wFrameTexture()|BImageNoDitherSolid,
				blackbox->wFrameColor());
    
    delete b_image;
    XSetWindowBackgroundPixmap(display, frame.border, p);
    XClearWindow(display, frame.border);
    if (p) {
      XFreePixmap(display, p);
      p = None;
    }
    
    if (resizable) {
      if (frame.fhandle) XFreePixmap(display, frame.fhandle);
      if (frame.uhandle) XFreePixmap(display, frame.uhandle);
      
      BImage *h_image = new BImage(blackbox, frame.handle_w, frame.handle_h,
				   blackbox->Depth());
      frame.fhandle = h_image->renderImage(blackbox->wDecorTexture(),
					   blackbox->wFColor(),
					   blackbox->wFColorTo());
      frame.uhandle = h_image->renderImage(blackbox->wDecorTexture(),
					   blackbox->wUColor(),
					   blackbox->wUColorTo());
      delete h_image;

      BImage *rh_image = new BImage(blackbox, frame.rh_w, frame.rh_h,
				    blackbox->Depth());
      p = rh_image->renderImage(blackbox->wHandleTexture(),
				blackbox->wUColor(),
				blackbox->wUColorTo());
      delete rh_image;
      XSetWindowBackgroundPixmap(display, frame.resize_handle, p);
      XClearWindow(display, frame.resize_handle);
      if (p) {
	XFreePixmap(display, p);
	p = None;
      }
    }
  }
    
  XSetWindowBorder(display, frame.window, blackbox->borderColor().pixel);
  XSetWindowBackground(display, frame.window, blackbox->borderColor().pixel);
  XClearWindow(display, frame.window);
  XClearWindow(display, frame.border);
  setFocusFlag(focused);
  drawTitleWin();
  drawAllButtons();
  
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


// *************************************************************************
// Window protocol and ICCCM code
// *************************************************************************

Bool BlackboxWindow::getWMProtocols(void) {
  Atom *proto;
  int num_return = 0;

  if (! validateClient()) return False;
  if (! XGetWMProtocols(display, client.window, &proto, &num_return))
    return True;
    
  for (int i = 0; i < num_return; ++i) {
    if (proto[i] == blackbox->DeleteAtom()) {
      protocols.WM_DELETE_WINDOW = True;
      protocols.WMDeleteWindow = blackbox->DeleteAtom();
      createCloseButton();
      positionButtons();
    } else if (proto[i] ==  blackbox->FocusAtom()) {
      protocols.WM_TAKE_FOCUS = True;
    } else if (proto[i] ==  blackbox->StateAtom()) {
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
    } else  if (proto[i] ==  blackbox->ColormapAtom()) {
    }
  }
  
  XFree(proto);
  return True;
}


Bool BlackboxWindow::getWMHints(void) {
  if (! validateClient()) return False;

  XWMHints *wmhints;
  if ((wmhints = XGetWMHints(display, client.window)) == NULL) {
    visible = True;
    iconic = False;
    focus_mode = F_Passive;
    client.window_group = None;
    
    return True;
  }
  
  client.wmhint_flags = wmhints->flags;
  if (wmhints->flags & InputHint) {
    if (wmhints->input == True) {
      if (protocols.WM_TAKE_FOCUS)
	focus_mode = F_LocallyActive;
      else
	  focus_mode = F_Passive;
    } else {
      if (protocols.WM_TAKE_FOCUS)
	focus_mode = F_GloballyActive;
      else
	focus_mode = F_NoInput;
    }
  } else
    focus_mode = F_Passive;
  
  if (wmhints->flags & StateHint)
    client.initial_state = wmhints->initial_state;
  else
    client.initial_state = NormalState;
  
  // icon pixmap hint would be next, but since we now handle icons in a menu,
  // this has no bearing on us what so ever
  
  // same with icon window hint
  // and icon position hint
  // and icon mask hint
  
  if (wmhints->flags & WindowGroupHint) {
    if (! client.window_group) {
      client.window_group = wmhints->window_group;
      blackbox->saveGroupSearch(client.window_group, this);
      }
  } else
    client.window_group = None;
  
  XFree(wmhints);
  return True;
}


Bool BlackboxWindow::getWMNormalHints(XSizeHints *hint) {
  if (! validateClient()) return False;

  long icccm_mask;
  if (! XGetWMNormalHints(display, client.window, hint, &icccm_mask)) {
    client.inc_h = client.inc_w = 1;    
    client.min_w = client.min_h = 1;
    client.base_w = client.base_h = 1;
    client.max_w = blackbox->XResolution();
    client.max_h = blackbox->YResolution();
    
    return True;
  }
    
  client.normal_hint_flags = hint->flags;
  if (client.normal_hint_flags & PResizeInc) {
    client.inc_w = hint->width_inc;
    client.inc_h = hint->height_inc;
  } else
    client.inc_h = client.inc_w = 1;
  
  if (client.normal_hint_flags & PMinSize) {
    client.min_w = hint->min_width;
    client.min_h = hint->min_height;
  } else
    client.min_h = client.min_w = 1;
  
  if (client.normal_hint_flags & PBaseSize) {
    client.base_w = hint->base_width;
    client.base_h = hint->base_height;
  } else
    client.base_w = client.base_h = 0;
  
  if (client.normal_hint_flags & PMaxSize) {
    client.max_w = hint->max_width;
    client.max_h = hint->max_height;
  } else {
    client.max_w = blackbox->XResolution();
    client.max_h = blackbox->YResolution();
  }
  
  if (client.normal_hint_flags & PAspect) {
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
  if (dw > (client.max_w + ((transient) ? 0 : (frame.bevel_w * 2) +
			    ((resizable) ? frame.handle_w + 1 : 0))))
    dw = client.max_w + ((transient) ? 0: (frame.bevel_w * 2) +
			 ((resizable) ? frame.handle_w + 1 : 0));
  if (dh > (client.max_h + frame.title_h + ((transient) ? 1 :
					    ((frame.bevel_w * 2) + 1))))
    dh = client.max_h + frame.title_h + ((transient) ? 1 :
					 ((frame.bevel_w * 2) + 1));
  
  if ((((signed) frame.width) + dx) < 0) dx = 0;
  if ((((signed) frame.title_h) + dy) < 0) dy = 0;
    
  resize = ((dw != frame.width) || (dh != frame.height));
  send_event = ((dx != frame.x || dy != frame.y) && (! resize));
  
  if (resize) {
    frame.x = dx;
    frame.y = dy;
    frame.width = dw;
    frame.height = dh;
    
    frame.border_w = ((transient) ? 0 :
		      frame.width - ((resizable) ?
				     (frame.handle_w + 1) : 0));
    frame.border_h = ((transient) ? 0 :
		      frame.height - frame.title_h - 1);
    frame.title_w = frame.width;
    frame.handle_h = ((resizable) ? dh - frame.title_h - frame.rh_h - 2 : 0);
    
    client.x = dx + ((transient) ? 1 : (frame.bevel_w + 1));
    client.y = dy + frame.title_h + ((transient) ? 1 : (frame.bevel_w + 1));
    client.width = ((transient) ? frame.width : frame.border_w -
		    (frame.bevel_w * 2));
    client.height = ((transient) ? frame.height - frame.title_h - 1 :
		     frame.border_h - (frame.bevel_w * 2));
    
    XWindowChanges xwc;
    xwc.x = dx;
    xwc.y = dy;
    xwc.width = dw;
    xwc.height = ((shaded) ? frame.title_h : dh);
    XConfigureWindow(display, frame.window, CWX|CWY|CWWidth|CWHeight, &xwc);
    
    if (frame.ftitle) {
      XFreePixmap(display, frame.ftitle);
      frame.ftitle = None;
    }

    if (frame.utitle) {
      XFreePixmap(display, frame.utitle);
      frame.utitle = None;
    }
    
    BImage *t_image = new BImage(blackbox, frame.title_w, frame.title_h,
				 blackbox->Depth());
    frame.ftitle = t_image->renderImage(blackbox->wDecorTexture(),
					blackbox->wFColor(),
					blackbox->wFColorTo());
    frame.utitle = t_image->renderImage(blackbox->wDecorTexture(),
					blackbox->wUColor(),
					blackbox->wUColorTo());
    delete t_image;
    
    if (! transient) {
      if (resizable) {
	if (frame.fhandle) {
	  XFreePixmap(display, frame.fhandle);
	  frame.fhandle = None;
	}

	if (frame.uhandle) {
	  XFreePixmap(display, frame.uhandle);
	  frame.uhandle = None;
	}
	
	BImage *h_image = new BImage(blackbox, frame.handle_w,
				     frame.handle_h, blackbox->Depth());
	frame.fhandle = h_image->renderImage(blackbox->wDecorTexture(),
					     blackbox->wFColor(),
					     blackbox->wFColorTo());
	frame.uhandle = h_image->renderImage(blackbox->wDecorTexture(),
					     blackbox->wUColor(),
					     blackbox->wUColorTo());
	delete h_image;
      }
    }
    
    setFocusFlag(focused);
    XResizeWindow(display, frame.title, frame.title_w, frame.title_h);
    positionButtons();
    
    if (! transient) {
      XResizeWindow(display, frame.border, frame.border_w, frame.border_h);
      
      if (resizable) {
	XMoveResizeWindow(display, frame.handle, frame.border_w + 1,
			  frame.title_h + 1,
			  frame.handle_w, frame.handle_h);
	
	XMoveWindow(display, frame.resize_handle, frame.border_w + 1,
		    frame.title_h + frame.handle_h + 2);
      }
      
      if (! validateClient()) return;
      XMoveResizeWindow(display, client.window, frame.bevel_w,
			frame.bevel_w, client.width, client.height);
      
      BImage *b_image = new BImage(blackbox, frame.border_w, frame.border_h,
				   blackbox->Depth());
      Pixmap p =
	b_image->renderSolidImage(blackbox->wFrameTexture()|
				  BImageNoDitherSolid,
				  blackbox->wFrameColor());
      
      delete b_image;
      XSetWindowBackgroundPixmap(display, frame.border, p);
      XClearWindow(display, frame.border);
      if (p) {
	XFreePixmap(display, p);
	p = None;
      }
    } else {
      if (! validateClient()) return;
      XMoveResizeWindow(display, client.window, 0, frame.title_h + 1,
			client.width, client.height);
    }
    
    drawTitleWin();
    drawAllButtons();
  } else {
    frame.x = dx;
    frame.y = dy;
    
    client.x = dx + ((transient) ? 1 : (frame.bevel_w + 1));
    client.y = dy + frame.title_h + ((transient) ? 1 : (frame.bevel_w + 1));
    
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
    
    if (! validateClient()) return;
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
    if (! validateClient()) return False;
    if (XSetInputFocus(display, client.window, RevertToParent, CurrentTime))
      return True;
    break;
  }

  return False;
}


void BlackboxWindow::iconifyWindow(Bool prop) {
  if (windowmenu) windowmenu->Hide();
  
  XUnmapWindow(display, frame.window);
  visible = False;
  iconic = True;
  focused = False;    
  
  if (transient) {
    if (! client.transient_for->iconic)
      client.transient_for->iconifyWindow();
  } else
    icon = new BlackboxIcon(blackbox, this);
  
  if (client.transient)
    if (! client.transient->iconic)
      client.transient->iconifyWindow();
  
  unsigned long state[2];
  state[0] = (unsigned long) IconicState;
  state[1] = (unsigned long) None;

  if (! validateClient()) return;
  if (prop)
    XChangeProperty(display, client.window, blackbox->StateAtom(),
		    blackbox->StateAtom(), 32, PropModeReplace,
		    (unsigned char *) state, 2);
}


void BlackboxWindow::deiconifyWindow(Bool prop) {
  blackbox->reassociateWindow(this);
  
  XMapWindow(display, frame.window);
    
  XMapSubwindows(display, frame.window);
  visible = True;
  iconic = False;
  
  if (client.transient) client.transient->deiconifyWindow();
    
  unsigned long state[2];
  state[0] = (unsigned long) NormalState;
  state[1] = (unsigned long) None;

  if (! validateClient()) return;
  if (prop)
    XChangeProperty(display, client.window, blackbox->StateAtom(),
		    blackbox->StateAtom(), 32, PropModeReplace,
		    (unsigned char *) state, 2);
    
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
  ce.xclient.data.l[2] = 0l;
  ce.xclient.data.l[3] = 0l;
  ce.xclient.data.l[4] = 0l;
  
  if (! validateClient()) return;
  XSendEvent(display, client.window, False, NoEventMask, &ce);
}


void BlackboxWindow::withdrawWindow(Bool prop) {
  unsigned long state[2];
  state[0] = (unsigned long) WithdrawnState;
  state[1] = (unsigned long) None;
  
  focused = False;
  visible = False;
  
  XUnmapWindow(display, frame.window);
  if (windowmenu) windowmenu->Hide();
  
  if (! validateClient()) return;
  if (prop)
    XChangeProperty(display, client.window, blackbox->StateAtom(),
		    blackbox->StateAtom(), 32, PropModeReplace,
		    (unsigned char *) state, 2);
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
  static int px, py;
  static unsigned int pw, ph;

  if (! maximized) {
    int dx, dy;
    unsigned int dw, dh;

    px = frame.x;
    py = frame.y;
    pw = frame.width;
    ph = frame.height;
    
    dw = blackbox->XResolution() - frame.handle_w - client.base_w -
      ((frame.bevel_w * 2) + 1);
    dh = blackbox->YResolution() - blackbox->workspaceManager()->Height() -
      frame.title_h - client.base_h - ((frame.bevel_w * 2) + 1);

    if (dw < client.min_w) dw = client.min_w;
    if (dh < client.min_h) dh = client.min_h;
    if (dw > client.max_w) dw = client.max_w;
    if (dh > client.max_h) dh = client.max_h;
    
    dw /= client.inc_w;
    dh /= client.inc_h;
    
    dw = (dw * client.inc_w) + client.base_w;
    dh = (dh * client.inc_h) + client.base_h;

    dw += frame.handle_w + ((frame.bevel_w * 2) + 1);
    dh += frame.title_h + ((frame.bevel_w * 2) + 1);

    dx = ((blackbox->XResolution() - dw) / 2) - 1;
    dy = ((blackbox->YResolution() - blackbox->workspaceManager()->Height())
	  - dh) / 2;
    
    maximized = True;
    shaded = False;
    configureWindow(dx, dy, dw, dh);
    blackbox->workspaceManager()->workspace(workspace_number)->
      raiseWindow(this);
  } else {
    configureWindow(px, py, pw, ph);
    maximized = False;
  }
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
  focused = focus;
  XSetWindowBackgroundPixmap(display, frame.title,
			     (focused) ? frame.ftitle : frame.utitle);
  XClearWindow(display, frame.title);

  if (! transient && resizable) {
    XSetWindowBackgroundPixmap(display, frame.handle,
			       (focused) ? frame.fhandle : frame.uhandle);
    XClearWindow(display, frame.handle);
  }
  
  drawTitleWin();
  drawAllButtons();
}


// *************************************************************************
// utility function to fetch the _XA_WM_STATE property of the specified
// window - this follows twm's GetWMState() function closely
// *************************************************************************

Bool BlackboxWindow::fetchWMState(unsigned long *state_return) {
  Atom atom_return;
  Bool ret = False;
  int foo;
  unsigned long *state, ulfoo, nitems;

  if ((XGetWindowProperty(display, client.window, blackbox->StateAtom(),
			  0l, 2l, False, blackbox->StateAtom(), &atom_return,
			  &foo, &nitems, &ulfoo,
			  (unsigned char **) &state) != Success) || (! state))
    return False;

  if (nitems <= 2) {
    *state_return = (unsigned long) state[0];
    ret = True;
  }
  
  XFree((void *) state);
  return ret;
}


// *************************************************************************
// Window drawing code
// *************************************************************************

void BlackboxWindow::drawTitleWin(void) {  
  switch (blackbox->Justification()) {
  case Blackbox::B_LeftJustify: {
    int dx = ((frame.title_w > ((frame.button_w + 4) * 6)) ?
	      frame.button_w + 4 : 4);
    
    XDrawString(display, frame.title,
		((focused) ? frame.ftextGC : frame.utextGC), dx,
		blackbox->titleFont()->ascent + frame.bevel_w, client.title,
		strlen(client.title));
    break; }
  
  case Blackbox::B_RightJustify: {
    int dx = ((frame.title_w > ((frame.button_w + 4) * 6)) ?
	      (frame.button_w + 4) * 3 : 4);

    int off = XTextWidth(blackbox->titleFont(), client.title,
			 strlen(client.title)) + dx;
    
    XDrawString(display, frame.title,
		((focused) ? frame.ftextGC : frame.utextGC),
		frame.title_w - off, blackbox->titleFont()->ascent +
		frame.bevel_w, client.title, strlen(client.title));
    break;  }
  
  case Blackbox::B_CenterJustify: {
    int dx = ((frame.title_w > ((frame.button_w + 4) * 6)) ?
	      (frame.button_w + 4) * 3 : 4);
    
    int ins = ((frame.width - dx) -
	       (XTextWidth(blackbox->titleFont(), client.title,
			   strlen(client.title)))) / 2;
    
    XDrawString(display, frame.title,
		((focused) ? frame.ftextGC : frame.utextGC),
		ins, blackbox->titleFont()->ascent + frame.bevel_w,
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
  if (! pressed) {
    XSetWindowBackgroundPixmap(display, frame.iconify_button,
			       ((focused) ? frame.fbutton : frame.ubutton));
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
  if (! pressed) {
    XSetWindowBackgroundPixmap(display, frame.maximize_button,
			       ((focused) ? frame.fbutton : frame.ubutton));
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
  if (! pressed) {
    XSetWindowBackgroundPixmap(display, frame.close_button,
			       ((focused) ? frame.fbutton : frame.ubutton));
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
    if ((! iconic) && (client.wmhint_flags & StateHint)) {
      unsigned long state;

      if (! validateClient()) return;
      if (! (blackbox->Startup() && fetchWMState(&state) &&
	     (state == NormalState || state == IconicState)))
	state = client.initial_state;
      
      switch (state) {
      case IconicState:
	iconifyWindow();
	break;

      case DontCareState:
      case NormalState:
      case InactiveState:
      case ZoomState:
	XMapWindow(display, client.window);
	XMapSubwindows(display, frame.window);
	XMapWindow(display, frame.window);	
	setFocusFlag(False);

	if (blackbox->Startup()) {
	  visible = True;
	  iconic = False;
	}

	unsigned long new_state[2];
	new_state[0] = NormalState;
	new_state[1] = None;

	if (! validateClient()) return;
	XChangeProperty(display, client.window, blackbox->StateAtom(),
			blackbox->StateAtom(), 32, PropModeReplace,
			(unsigned char *) new_state, 2);
	break;
      }
    } else
      deiconifyWindow();
  }
}


void BlackboxWindow::mapNotifyEvent(XMapEvent *ne) {
  if (! validateClient()) return;
  if ((ne->window == client.window) && (! ne->override_redirect) &&
      (! iconic)) {
    XGrabServer(display);
    
    positionButtons();
    
    unsigned long state[2];
    state[0] = (unsigned long) NormalState;
    state[1] = (unsigned long) None;
    
    if (! validateClient()) return;
    XChangeProperty(display, client.window, blackbox->StateAtom(),
		    blackbox->StateAtom(), 32, PropModeReplace,
		    (unsigned char *) state, 2);
    
    XMapSubwindows(display, frame.window);
    XMapWindow(display, frame.window);
    
    setFocusFlag(False);
    visible = True;
    iconic = False;
    
    XUngrabServer(display);
  }
}


void BlackboxWindow::unmapNotifyEvent(XUnmapEvent *ue) {
  if (ue->window == client.window) {
    if (! iconic && ! visible) return;
    unsigned long state[2];
    state[0] = (unsigned long) WithdrawnState;
    state[1] = (unsigned long) None;

    visible = False;
    iconic = False;
    XUnmapWindow(display, frame.window);

    if (! validateClient()) return;
    XChangeProperty(display, client.window, blackbox->StateAtom(),
		    blackbox->StateAtom(), 32, PropModeReplace,
		    (unsigned char *) state, 2);
    
    XEvent dummy;
    if (! validateClient()) return;
    if (! XCheckTypedWindowEvent(display, client.window, ReparentNotify,
				 &dummy))
      XReparentWindow(display, client.window, blackbox->Root(), client.x,
		      client.y);
    
    XChangeSaveSet(display, client.window, SetModeDelete);
    XSelectInput(display, client.window, NoEventMask);

    XFlush(display);
    delete this;
  }
}


void BlackboxWindow::destroyNotifyEvent(XDestroyWindowEvent *de) {
  if (de->window == client.window) {
    XUnmapWindow(display, frame.window);
    delete this;
  }
}


void BlackboxWindow::propertyNotifyEvent(Atom atom) {
  switch(atom) {
  case XA_WM_CLASS:
  case XA_WM_CLIENT_MACHINE:
  case XA_WM_COMMAND:   
  case XA_WM_TRANSIENT_FOR:
    break;
      
  case XA_WM_HINTS:
    if (! getWMHints()) return;
    break;
    
  case XA_WM_ICON_NAME:
    if (icon) icon->rereadLabel();
    break;
      
  case XA_WM_NAME:
    if (client.title)
      if (strcmp(client.title, "Unnamed")) {
	XFree(client.title);
	client.title = 0;
      }
    
    if (! validateClient()) return;
    if (! XFetchName(display, client.window, &client.title))
      client.title = "Unnamed";
    XClearWindow(display, frame.title);
    drawTitleWin();
    blackbox->workspaceManager()->workspace(workspace_number)->Update();
    break;
      
  case XA_WM_NORMAL_HINTS: {
    XSizeHints sizehint;
    if (! getWMNormalHints(&sizehint)) return;
    break; }
    
  default:
    if (atom == blackbox->ProtocolsAtom())
      if (! getWMProtocols()) return;
  }
}


void BlackboxWindow::exposeEvent(XExposeEvent *ee) {
  if (frame.title == ee->window)
    drawTitleWin();
  else if (frame.close_button == ee->window)
    drawCloseButton(False);
  else if (frame.maximize_button == ee->window)
    drawMaximizeButton(False);
  else if (frame.iconify_button == ee->window)
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
      cw = cr->width + (frame.bevel_w * 2) +
	((resizable) ? frame.handle_w + 1 : 0);
    else
      cw = frame.width;

    if (cr->value_mask & CWHeight)
      ch = cr->height + frame.title_h + ((frame.bevel_w * 2) + 1);
    else
      ch = frame.height;
    
    configureWindow(cx, cy, cw, ch);
  }
}


void BlackboxWindow::buttonPressEvent(XButtonEvent *be) {
  if (be->button == 1) {
    if (frame.title == be->window || frame.handle == be->window ||
	frame.resize_handle == be->window || frame.border == be->window)
      blackbox->workspaceManager()->workspace(workspace_number)->
	raiseWindow(this);
    else if (frame.iconify_button == be->window)
      drawIconifyButton(True);
    else if (frame.maximize_button == be->window)
      drawMaximizeButton(True);
    else if (frame.close_button == be->window)
      drawCloseButton(True);
  } else if (be->button == 2) {
    if (frame.title == be->window || frame.handle == be->window ||
        frame.resize_handle == be->window || frame.border == be->window) {
      blackbox->workspaceManager()->workspace(workspace_number)->
	lowerWindow(this);
    }
  } else if (be->button == 3) {
    if (frame.title == be->window) {
      if (windowmenu)
	if (! windowmenu->Visible()) {
	  windowmenu->Move(be->x_root - (windowmenu->Width() / 2),
			   frame.y + frame.title_h);
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

        if (! blackbox->opaqueMove()) {
	  if (! transient) {
	    XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			   frame.x_move, frame.y_move, frame.width,
			   frame.height);
	    XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			   frame.x_move + 1, frame.y_move + frame.title_h + 1,
			   frame.border_w - 2, frame.border_h - 2);
	  } else {
	    XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			   frame.x_move, frame.y_move, frame.width,
			   frame.height);
	    XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			   frame.x_move + 1, frame.y_move + frame.title_h + 1,
			   client.width - 2, client.height - 2);
	  }

	  configureWindow(frame.x_move, frame.y_move, frame.width,
			  frame.height);
	  XUngrabServer(display);
	} else
	  configureWindow(frame.x, frame.y, frame.width, frame.height);

	moving = False;
	XUngrabPointer(display, CurrentTime);
      } else if ((re->state & ControlMask))
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
      dx = frame.x_resize - frame.handle_w - client.base_w -
	((frame.bevel_w * 2) + 1);
      dy = frame.y_resize - frame.title_h - client.base_h -
	((frame.bevel_w * 2) + 1);
      
      if (dx < (signed) client.min_w) dx = client.min_w;
      if (dy < (signed) client.min_h) dy = client.min_h;
      if ((unsigned) dx > client.max_w) dx = client.max_w;
      if ((unsigned) dy > client.max_h) dy = client.max_h;
      
      dx /= client.inc_w;
      dy /= client.inc_h;
      
      dx = (dx * client.inc_w) + client.base_w;
      dy = (dy * client.inc_h) + client.base_h;
      
      frame.x_resize = dx + frame.handle_w + ((frame.bevel_w * 2) + 1);
      frame.y_resize = dy + frame.title_h + ((frame.bevel_w * 2) + 1);
      
      delete [] resizeLabel;
      configureWindow(frame.x, frame.y, frame.x_resize, frame.y_resize);
      resizing = False;
      XUngrabPointer(display, CurrentTime);
      XUngrabServer(display);
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
	  frame.x_grab = me->x;
	  frame.y_grab = me->y;

	  if (! blackbox->opaqueMove()) {
            XGrabServer(display);

	    frame.x_move = frame.x;
	    frame.y_move = frame.y;
	    
	    if (! transient) {
	      XDrawRectangle(display, blackbox->Root(),
			     blackbox->GCOperations(), frame.x_move,
			     frame.y_move, frame.width, frame.height);
	      XDrawRectangle(display, blackbox->Root(),
			     blackbox->GCOperations(), frame.x_move + 1,
			     frame.y_move + frame.title_h + 1,
			     frame.border_w - 2, frame.border_h - 2);
	    } else {
	      XDrawRectangle(display, blackbox->Root(),
			     blackbox->GCOperations(), frame.x_move,
			     frame.y_move, frame.width, frame.height);
	      XDrawRectangle(display, blackbox->Root(),
			     blackbox->GCOperations(), frame.x_move + 1,
			     frame.y_move + frame.title_h + 1,
			     client.width - 2, client.height - 2);
	    }
	  }
	} else
	  moving = False;
      } else {
	int dx = me->x_root - frame.x_grab,
	  dy = me->y_root - frame.y_grab;
	
	if (! blackbox->opaqueMove()) {
	  if (! transient) {
	    XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			   frame.x_move, frame.y_move, frame.width,
			   frame.height);
	    XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			   frame.x_move + 1, frame.y_move + frame.title_h + 1,
			   frame.border_w - 2, frame.border_h - 2);
	  } else {
	    XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			   frame.x_move, frame.y_move, frame.width,
			   frame.height);
	    XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			   frame.x_move + 1, frame.y_move + frame.title_h + 1,
			   client.width - 2, client.height - 2);
	  }
	  
	  frame.x_move = dx;
	  frame.y_move = dy;
	  
	  if (! transient) {
	    XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			   frame.x_move, frame.y_move, frame.width,
			   frame.height);
	    XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			   frame.x_move + 1, frame.y_move + frame.title_h + 1,
			   frame.border_w - 2, frame.border_h - 2);
	  } else {
	    XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			   frame.x_move, frame.y_move, frame.width,
			   frame.height);
	    XDrawRectangle(display, blackbox->Root(), blackbox->GCOperations(),
			   frame.x_move + 1, frame.y_move + frame.title_h + 1,
			   client.width - 2, client.height - 2);
	  }
	} else
	  configureWindow(dx, dy, frame.width, frame.height);
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

          XGrabServer(display);

	  frame.x_resize = frame.width;
	  frame.y_resize = frame.height;
	  
	  // calculate the size of the client window and conform it to the
	  // size specified by the size hints of the client window...
	  dx = frame.x_resize - frame.handle_w - client.base_w -
	    ((frame.bevel_w * 2) + 1);
	  dy = frame.y_resize - frame.title_h - client.base_h -
	    ((frame.bevel_w * 2) + 1);
	  
	  if (dx < (signed) client.min_w) dx = client.min_w;
	  if (dy < (signed) client.min_h) dy = client.min_h;
	  if ((unsigned) dx > client.max_w) dx = client.max_w;
	  if ((unsigned) dy > client.max_h) dy = client.max_h;
	  
	  dx /= client.inc_w;
	  dy /= client.inc_h;
	  
	  resizeLabel = new char[strlen("00000 x 00000")];
	  sprintf(resizeLabel, "%d x %d", dx, dy);
	  
	  dx = (dx * client.inc_w) + client.base_w;
	  dy = (dy * client.inc_h) + client.base_h;
	  
	  frame.x_resize = dx + frame.handle_w + ((frame.bevel_w * 2) + 1);
	  frame.y_resize = dy + frame.title_h + ((frame.bevel_w * 2) + 1);
	  
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
	dx = frame.x_resize - frame.handle_w - client.base_w -
	  ((frame.bevel_w * 2) + 1);
	dy = frame.y_resize - frame.title_h - client.base_h -
	  ((frame.bevel_w * 2) + 1);

	if (dx < (signed) client.min_w) dx = client.min_w;
	if (dy < (signed) client.min_h) dy = client.min_h;
	if ((unsigned) dx > client.max_w) dx = client.max_w;
	if ((unsigned) dy > client.max_h) dy = client.max_h;
	
	dx /= client.inc_w;
	dy /= client.inc_h;
	
	sprintf(resizeLabel, "%d x %d", dx, dy);
	
	dx = (dx * client.inc_w) + client.base_w;
	dy = (dy * client.inc_h) + client.base_h;

	frame.x_resize = dx + frame.handle_w + ((frame.bevel_w * 2) + 1);
	frame.y_resize = dy + frame.title_h + ((frame.bevel_w * 2) + 1);

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
      if (! validateClient()) return;
      XShapeCombineShape(display, frame.window, ShapeBounding,
			 ((transient) ? 0 : frame.bevel_w),
			 frame.title_h + ((transient) ? 1 :
					  (frame.bevel_w + 1)), client.window,
			 ShapeBounding, ShapeSet);
      
      int num = 1;
      XRectangle xrect[2];
      xrect[0].x = xrect[0].y = 0;
      xrect[0].width = frame.title_w;
      xrect[0].height = frame.title_h + 1;
      
      if (resizable) {
	xrect[1].x = frame.border_w + 1;
	xrect[1].y = frame.title_h + 1;
	xrect[1].width = frame.handle_w;
	xrect[1].height = frame.border_h;
	num++;
      }
      
      XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
			      xrect, num, ShapeUnion, Unsorted);
    }
  }
}

#endif


Bool BlackboxWindow::validateClient(void) {
  XEvent e;
  if (XCheckTypedWindowEvent(display, client.window, DestroyNotify, &e)) {
    XUngrabServer(display);
    delete this;
    return False;
  }

  return True;
}
