// Window.cc for Blackbox - an X11 Window manager
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

// stupid macros needed to access some functions in version 2 of the GNU C
// library
#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "blackbox.hh"
#include "Icon.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Windowmenu.hh"
#include "Workspace.hh"

#ifdef    SLIT
#  include "Slit.hh"
#endif // SLIT

#include <X11/Xatom.h>
#include <X11/keysym.h>

#ifdef    STDC_HEADERS
#  include <string.h>
#endif // STDC_HEADERS

#ifdef    DEBUG
#  ifdef    HAVE_STDIO_H
#    include <stdio.h>
#  endif // HAVE_STDIO_H
#endif // DEBUG

BlackboxWindow::BlackboxWindow(Blackbox *b, Window w, BScreen *s) {
#ifdef    DEBUG
  fprintf(stderr,
	  i18n->
	  getMessage(
#  ifdef    NLS
		     WindowSet, WindowCreating,
#  else // !NLS
		     0, 0,
#  endif // NLS
		     "BlackboxWindow::BlackboxWindow(): creating 0x%lx\n"),
	  w);
#endif // DEBUG

  blackbox = b;
  display = blackbox->getXDisplay();

  moving = resizing = shaded = maximized = visible = iconic = False;
  transient = focused = stuck = modal =  send_focus_message = managed = False;

  blackbox_attrib.workspace = workspace_number = window_number = -1;

  blackbox_attrib.flags = blackbox_attrib.attrib = blackbox_attrib.stack = 0l;
  blackbox_attrib.premax_x = blackbox_attrib.premax_y = 0;
  blackbox_attrib.premax_w = blackbox_attrib.premax_h = 0;

  client.window = w;

  frame.window = frame.plate = frame.title = frame.handle = frame.border = None;
  frame.close_button = frame.iconify_button = frame.maximize_button = None;
  frame.right_grip = frame.left_grip = None;

  frame.utitle = frame.ftitle = frame.uhandle = frame.fhandle = None;
  frame.ulabel = frame.flabel = frame.ubutton = frame.fbutton = None;
  frame.pbutton = frame.uborder = frame.fborder = frame.ugrip = None;
  frame.fgrip = None;

  decorations.titlebar = decorations.border = decorations.handle = True;
  decorations.iconify = decorations.maximize = decorations.menu = True;
  functions.resize = functions.move = functions.iconify = functions.maximize = True;
  functions.close = decorations.close = False;

  client.wm_hint_flags = client.normal_hint_flags = 0;
  client.transient_for = client.transient = 0;
  client.title = 0;
  client.title_len = 0;
  client.icon_title = 0;
  client.mwm_hint = (MwmHints *) 0;
  client.blackbox_hint = (BlackboxHints *) 0;

  windowmenu = 0;
  lastButtonPressTime = 0;
  timer = 0;
  screen = 0;
  image_ctrl = 0;

  blackbox->grab();
  if (! validateClient()) return;

  // fetch client size and placement
  XWindowAttributes wattrib;
  if ((! XGetWindowAttributes(display, client.window, &wattrib)) ||
      (! wattrib.screen) || wattrib.override_redirect) {
#ifdef    DEBUG
    fprintf(stderr,
            i18n->
	    getMessage(
#  ifdef    NLS
		       WindowSet, WindowXGetWindowAttributesFail,
#  else // !NLS
		       0, 0,
#  endif // NLS
		       "BlackboxWindow::BlackboxWindow(): XGetWindowAttributes "
		       "failed\n"));
#endif // DEBUG

    delete this;

    blackbox->ungrab();
    return;
  }

  if (s)
    screen = s;
  else
    screen = blackbox->searchScreen(RootWindowOfScreen(wattrib.screen));

  if (! screen) {
#ifdef    DEBUG
    fprintf(stderr,
	    i18n->
	    getMessage(
#  ifdef    NLS
		       WindowSet, WindowCannotFindScreen,
#  else // !NLS
		       0, 0,
#  endif // NLS
		       "BlackboxWindow::BlackboxWindow(): can't find screen\n"
		       "	for root window 0x%lx\n"),
	    RootWindowOfScreen(wattrib.screen));
#endif // DEBUG

    delete this;

    blackbox->ungrab();
    return;
  }

  image_ctrl = screen->getImageControl();

  client.x = wattrib.x;
  client.y = wattrib.y;
  client.width = wattrib.width;
  client.height = wattrib.height;
  client.old_bw = wattrib.border_width;

  timer = new BTimer(blackbox, this);
  timer->setTimeout(blackbox->getAutoRaiseDelay());
  timer->fireOnce(True);

  getBlackboxHints();
  if (! client.blackbox_hint)
    getMWMHints();

  // get size, aspect, minimum/maximum size and other hints set by the
  // client
  getWMProtocols();
  getWMHints();
  getWMNormalHints();

#ifdef    SLIT
  if (client.initial_state == WithdrawnState) {
    screen->getSlit()->addClient(client.window);
    delete this;

    blackbox->ungrab();

    return;
  }
#endif // SLIT

  managed = True;
  blackbox->saveWindowSearch(client.window, this);

  // determine if this is a transient window
  Window win;
  if (XGetTransientForHint(display, client.window, &win)) {
    if (win && (win != client.window)) {
      BlackboxWindow *tr;
      if ((tr = blackbox->searchWindow(win))) {
	while (tr->client.transient) tr = tr->client.transient;
	client.transient_for = tr;
	tr->client.transient = this;
	stuck = client.transient_for->stuck;
	transient = True;
      } else if (win == client.window_group) {
	if ((tr = blackbox->searchGroup(win, this))) {
	  while (tr->client.transient) tr = tr->client.transient;
	  client.transient_for = tr;
	  tr->client.transient = this;
	  stuck = client.transient_for->stuck;
	  transient = True;
	}
      }
    }

    if (win == screen->getRootWindow()) modal = True;
  }

  // edit the window decorations for the transient window
  if (! transient) {
    if ((client.normal_hint_flags & PMinSize) &&
	(client.normal_hint_flags & PMaxSize))
      if (client.max_width <= client.min_width &&
	  client.max_height <= client.min_height)
	decorations.maximize = decorations.handle =
	    functions.resize = functions.maximize = False;
  } else {
    decorations.border = decorations.handle = decorations.maximize =
      functions.resize = functions.maximize = False;
  }

  upsize();

  Bool place_window = True;
  if (blackbox->isStartup() || transient ||
      client.normal_hint_flags & (PPosition|USPosition)) {
    setGravityOffsets();

    if ((blackbox->isStartup()) ||
	(frame.x >= 0 &&
	 (signed) (frame.y + frame.y_border) >= 0 &&
	 frame.x <= (signed) screen->getWidth() &&
	 frame.y <= (signed) screen->getHeight()))
      place_window = False;
  }

  frame.window = createToplevelWindow(frame.x, frame.y, frame.width,
				      frame.height,
				      screen->getBorderWidth());
  blackbox->saveWindowSearch(frame.window, this);

  frame.plate = createChildWindow(frame.window);
  blackbox->saveWindowSearch(frame.plate, this);

  if (decorations.titlebar) {
    frame.title = createChildWindow(frame.window);
    frame.label = createChildWindow(frame.title);
    blackbox->saveWindowSearch(frame.title, this);
    blackbox->saveWindowSearch(frame.label, this);
  }

  if (decorations.border) {
    frame.border = createChildWindow(frame.window);
    blackbox->saveWindowSearch(frame.border, this);
  }

  if (decorations.handle) {
    frame.handle = createChildWindow(frame.window);
    blackbox->saveWindowSearch(frame.handle, this);

    frame.left_grip =
      createChildWindow(frame.handle, blackbox->getLowerLeftAngleCursor());
    blackbox->saveWindowSearch(frame.left_grip, this);

    frame.right_grip =
      createChildWindow(frame.handle, blackbox->getLowerRightAngleCursor());
    blackbox->saveWindowSearch(frame.right_grip, this);
  }

  associateClientWindow();

  if (! screen->isSloppyFocus())
    XGrabButton(display, Button1, 0, frame.plate, True,
		ButtonReleaseMask, GrabModeSync, GrabModeSync,
		frame.plate, blackbox->getSessionCursor());

  XGrabButton(display, Button1, Mod1Mask, frame.window, True,
	      ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
	      GrabModeAsync, None, blackbox->getMoveCursor());
  XGrabButton(display, Button2, Mod1Mask, frame.window, True,
	      ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
	      GrabModeAsync, None, blackbox->getLowerRightAngleCursor());

  positionWindows();
  if (decorations.border) XLowerWindow(display, frame.border);
  XRaiseWindow(display, frame.plate);
  XMapSubwindows(display, frame.plate);
  if (decorations.titlebar) XMapSubwindows(display, frame.title);
  XMapSubwindows(display, frame.window);

  if (decorations.menu)
    windowmenu = new Windowmenu(this);

  decorate();

  if (workspace_number < 0 || workspace_number >= screen->getCount())
    screen->getCurrentWorkspace()->addWindow(this, place_window);
  else
    screen->getWorkspace(workspace_number)->addWindow(this, place_window);

  configure(frame.x, frame.y, frame.width, frame.height);

  if (shaded) {
    shaded = False;
    shade();
  }

  if (maximized && functions.maximize) {
    int m = maximized;
    maximized = False;

    maximize(m);
  }

  setFocusFlag(False);

  blackbox->ungrab();
}


BlackboxWindow::~BlackboxWindow(void) {
  if (moving || resizing) XUngrabPointer(display, CurrentTime);

  if (workspace_number != -1 && window_number != -1)
    screen->getWorkspace(workspace_number)->removeWindow(this);
  else if (iconic)
    screen->removeIcon(this);

  if (timer) {
    if (timer->isTiming()) timer->stop();
    delete timer;
  }

  if (windowmenu) delete windowmenu;

  if (client.title)
    delete [] client.title;

  if (client.icon_title)
    delete [] client.icon_title;

  if (client.mwm_hint)
    XFree(client.mwm_hint);

  if (client.blackbox_hint)
    XFree(client.blackbox_hint);

  if (client.window_group)
    blackbox->removeGroupSearch(client.window_group);

  if (transient && client.transient_for)
    client.transient_for->client.transient = 0;

  if (frame.close_button) {
    blackbox->removeWindowSearch(frame.close_button);
    XDestroyWindow(display, frame.close_button);
  }

  if (frame.iconify_button) {
    blackbox->removeWindowSearch(frame.iconify_button);
    XDestroyWindow(display, frame.iconify_button);
  }

  if (frame.maximize_button) {
    blackbox->removeWindowSearch(frame.maximize_button);
    XDestroyWindow(display, frame.maximize_button);
  }

  if (frame.title) {
    if (frame.ftitle)
      image_ctrl->removeImage(frame.ftitle);

    if (frame.utitle)
      image_ctrl->removeImage(frame.utitle);

    if (frame.flabel)
      image_ctrl->removeImage(frame.flabel);

    if( frame.ulabel)
      image_ctrl->removeImage(frame.ulabel);

    blackbox->removeWindowSearch(frame.label);
    blackbox->removeWindowSearch(frame.title);
    XDestroyWindow(display, frame.label);
    XDestroyWindow(display, frame.title);
  }

  if (frame.handle) {
    if (frame.fhandle)
      image_ctrl->removeImage(frame.fhandle);

    if (frame.uhandle)
      image_ctrl->removeImage(frame.uhandle);

    if (frame.fgrip)
      image_ctrl->removeImage(frame.fgrip);

    if (frame.ugrip)
      image_ctrl->removeImage(frame.ugrip);

    blackbox->removeWindowSearch(frame.handle);
    blackbox->removeWindowSearch(frame.right_grip);
    blackbox->removeWindowSearch(frame.left_grip);
    XDestroyWindow(display, frame.right_grip);
    XDestroyWindow(display, frame.left_grip);
    XDestroyWindow(display, frame.handle);
  }

  if (frame.border) {
    if (frame.fborder)
      image_ctrl->removeImage(frame.fborder);

    if (frame.uborder)
      image_ctrl->removeImage(frame.uborder);

    blackbox->removeWindowSearch(frame.border);
    XDestroyWindow(display, frame.border);
  }

  if (frame.fbutton)
    image_ctrl->removeImage(frame.fbutton);

  if (frame.ubutton)
    image_ctrl->removeImage(frame.ubutton);

  if (frame.pbutton)
    image_ctrl->removeImage(frame.pbutton);

  if (frame.window) {
    blackbox->removeWindowSearch(frame.plate);
    XDestroyWindow(display, frame.plate);
  }

  if (frame.window) {
    blackbox->removeWindowSearch(frame.window);
    XDestroyWindow(display, frame.window);
  }

  if (managed) {
    blackbox->removeWindowSearch(client.window);
    screen->removeNetizen(client.window);
  }
}


Window BlackboxWindow::createToplevelWindow(int x, int y, unsigned int width,
					    unsigned int height,
					    unsigned int borderwidth)
{
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap | CWBorderPixel |
    CWOverrideRedirect | CWEventMask;

  attrib_create.background_pixmap = None;
  attrib_create.override_redirect = True;
  attrib_create.event_mask = ButtonPressMask | ButtonReleaseMask |
    EnterWindowMask;

  return (XCreateWindow(display, screen->getRootWindow(), x, y, width, height,
			borderwidth, screen->getDepth(), InputOutput,
			screen->getVisual(), create_mask,
			&attrib_create));
}


Window BlackboxWindow::createChildWindow(Window parent, Cursor cursor) {
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap | CWBorderPixel |
    CWEventMask;

  attrib_create.background_pixmap = None;
  attrib_create.event_mask = ButtonPressMask | ButtonReleaseMask |
    ButtonMotionMask | ExposureMask | EnterWindowMask | LeaveWindowMask;

  if (cursor) {
    create_mask |= CWCursor;
    attrib_create.cursor = cursor;
  }

  return (XCreateWindow(display, parent, 0, 0, 1, 1, 0l,
			screen->getDepth(), InputOutput, screen->getVisual(),
			create_mask, &attrib_create));
}


void BlackboxWindow::associateClientWindow(void) {
  XSetWindowBorderWidth(display, client.window, 0);
  getWMName();
  getWMIconName();

  XChangeSaveSet(display, client.window, SetModeInsert);
  XSetWindowAttributes attrib_set;

  XSelectInput(display, frame.plate, NoEventMask);
  XReparentWindow(display, client.window, frame.plate, 0, 0);
  XSelectInput(display, frame.plate, SubstructureRedirectMask);

  XFlush(display);

  attrib_set.event_mask = PropertyChangeMask | StructureNotifyMask;
  attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask |
    ButtonMotionMask;

  XChangeWindowAttributes(display, client.window, CWEventMask|CWDontPropagate,
                          &attrib_set);

#ifdef    SHAPE
  if (blackbox->hasShapeExtensions()) {
    XShapeSelectInput(display, client.window, ShapeNotifyMask);

    int foo;
    unsigned int ufoo;

    XShapeQueryExtents(display, client.window, &frame.shaped, &foo, &foo,
		       &ufoo, &ufoo, &foo, &foo, &foo, &ufoo, &ufoo);

    if (frame.shaped) {
      XShapeCombineShape(display, frame.window, ShapeBounding,
			 (frame.bevel_w * decorations.border),
			 frame.y_border + (frame.bevel_w * decorations.border),
			 client.window, ShapeBounding, ShapeSet);

      int num = 1;
      XRectangle xrect[2];
      xrect[0].x = xrect[0].y = 0;
      xrect[0].width = frame.width;
      xrect[0].height = frame.y_border;

      if (decorations.handle) {
	xrect[1].x = 0;
	xrect[1].y = frame.y_handle;
	xrect[1].width = frame.width;
	xrect[1].height = frame.handle_h + screen->getBorderWidth();
	num++;
      }

      XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
			      xrect, num, ShapeUnion, Unsorted);
    }
  }
#endif // SHAPE

  if (decorations.iconify) createIconifyButton();
  if (decorations.maximize) createMaximizeButton();
  if (decorations.close) createCloseButton();

  if (frame.ubutton && frame.close_button)
    XSetWindowBackgroundPixmap(display, frame.close_button, frame.ubutton);
  if (frame.ubutton && frame.maximize_button)
    XSetWindowBackgroundPixmap(display, frame.maximize_button, frame.ubutton);
  if (frame.ubutton && frame.iconify_button)
    XSetWindowBackgroundPixmap(display, frame.iconify_button, frame.ubutton);
}


void BlackboxWindow::decorate(void) {
  Pixmap tmp = frame.fbutton;
  frame.fbutton =
    image_ctrl->renderImage(frame.button_w, frame.button_h,
			    &screen->getWindowStyle()->b_focus);
  if (tmp) image_ctrl->removeImage(tmp);

  tmp = frame.ubutton;
  frame.ubutton =
    image_ctrl->renderImage(frame.button_w, frame.button_h,
			    &(screen->getWindowStyle()->b_unfocus));
  if (tmp) image_ctrl->removeImage(tmp);

  tmp = frame.pbutton;
  frame.pbutton =
    image_ctrl->renderImage(frame.button_w, frame.button_h,
			    &(screen->getWindowStyle()->b_pressed));
  if (tmp) image_ctrl->removeImage(tmp);

  if (decorations.titlebar) {
    tmp = frame.ftitle;
    frame.ftitle =
      image_ctrl->renderImage(frame.title_w, frame.title_h,
			      &(screen->getWindowStyle()->t_focus));
    if (tmp) image_ctrl->removeImage(tmp);

    tmp = frame.utitle;
    frame.utitle =
      image_ctrl->renderImage(frame.title_w, frame.title_h,
			      &(screen->getWindowStyle()->t_unfocus));
    if (tmp) image_ctrl->removeImage(tmp);

    tmp = frame.flabel;
    frame.flabel =
      image_ctrl->renderImage(frame.label_w, frame.label_h,
			      &screen->getWindowStyle()->l_focus);
    if (tmp) image_ctrl->removeImage(tmp);

    tmp = frame.ulabel;
    frame.ulabel =
      image_ctrl->renderImage(frame.label_w, frame.label_h,
			      &screen->getWindowStyle()->l_unfocus);
    if (tmp) image_ctrl->removeImage(tmp);

    XSetWindowBorder(display, frame.title,
                     screen->getBorderColor()->getPixel());
  }

  if (decorations.border) {
    tmp = frame.fborder;
    frame.fborder =
      image_ctrl->renderImage(frame.border_w, frame.border_h,
			      &(screen->getWindowStyle()->f_focus));
    if (tmp) image_ctrl->removeImage(tmp);

    tmp = frame.uborder;
    frame.uborder =
      image_ctrl->renderImage(frame.border_w, frame.border_h,
			      &screen->getWindowStyle()->f_unfocus);
    if (tmp) image_ctrl->removeImage(tmp);
  }

  if (decorations.handle) {
    tmp = frame.fhandle;
    frame.fhandle =
      image_ctrl->renderImage(frame.handle_w, frame.handle_h,
			      &screen->getWindowStyle()->h_focus);
    if (tmp) image_ctrl->removeImage(tmp);

    tmp = frame.uhandle;
    frame.uhandle =
      image_ctrl->renderImage(frame.handle_w, frame.handle_h,
			      &screen->getWindowStyle()->h_unfocus);
    if (tmp) image_ctrl->removeImage(tmp);

    tmp = frame.fgrip;
    frame.fgrip =
      image_ctrl->renderImage(frame.grip_w, frame.grip_h,
			      &screen->getWindowStyle()->g_focus);
    if (tmp) image_ctrl->removeImage(tmp);

    tmp = frame.ugrip;
    frame.ugrip =
      image_ctrl->renderImage(frame.grip_w, frame.grip_h,
			      &screen->getWindowStyle()->g_unfocus);
    if (tmp) image_ctrl->removeImage(tmp);

    XSetWindowBorder(display, frame.handle,
                     screen->getBorderColor()->getPixel());
    XSetWindowBorder(display, frame.left_grip,
                     screen->getBorderColor()->getPixel());
    XSetWindowBorder(display, frame.right_grip,
                     screen->getBorderColor()->getPixel());
  }

  XSetWindowBorder(display, frame.window,
                   screen->getBorderColor()->getPixel());
}


void BlackboxWindow::createCloseButton(void) {
  if (decorations.close && frame.title != None) {
    frame.close_button = createChildWindow(frame.title);
    blackbox->saveWindowSearch(frame.close_button, this);
  }
}


void BlackboxWindow::createIconifyButton(void) {
  if (decorations.iconify && frame.title != None) {
    frame.iconify_button = createChildWindow(frame.title);
    blackbox->saveWindowSearch(frame.iconify_button, this);
  }
}


void BlackboxWindow::createMaximizeButton(void) {
  if (decorations.maximize && frame.title != None) {
    frame.maximize_button = createChildWindow(frame.title);
    blackbox->saveWindowSearch(frame.maximize_button, this);
  }
}


void BlackboxWindow::positionButtons(void) {
  unsigned int bw = frame.button_w + frame.bevel_w + 1,
    by = frame.bevel_w + 1, lx = by, lw = frame.width - by;

  if (decorations.iconify && frame.iconify_button != None) {
    XMoveResizeWindow(display, frame.iconify_button, by, by,
		      frame.button_w, frame.button_h);
    XMapWindow(display, frame.iconify_button);
    XClearWindow(display, frame.iconify_button);

    lx += bw;
    lw -= bw;
  } else if (frame.iconify_button)
    XUnmapWindow(display, frame.iconify_button);

  int bx = frame.width - bw;

  if (decorations.close && frame.close_button != None) {
    XMoveResizeWindow(display, frame.close_button, bx, by,
		      frame.button_w, frame.button_h);
    XMapWindow(display, frame.close_button);
    XClearWindow(display, frame.close_button);

    bx -= bw;
    lw -= bw;
  } else if (frame.close_button)
    XUnmapWindow(display, frame.close_button);

  if (decorations.maximize && frame.maximize_button != None) {
    XMoveResizeWindow(display, frame.maximize_button, bx, by,
		      frame.button_w, frame.button_h);
    XMapWindow(display, frame.maximize_button);
    XClearWindow(display, frame.maximize_button);

    lw -= bw;
  } else if (frame.maximize_button)
    XUnmapWindow(display, frame.maximize_button);

  frame.label_w = lw - by;
  XMoveResizeWindow(display, frame.label, lx, frame.bevel_w,
                    frame.label_w, frame.label_h);

  redrawLabel();
  redrawAllButtons();
}


void BlackboxWindow::reconfigure(void) {
  upsize();

  client.x = frame.x + (decorations.border * frame.bevel_w) +
	     screen->getBorderWidth();
  client.y = frame.y + frame.y_border + (decorations.border * frame.bevel_w) +
	     screen->getBorderWidth();

  if (client.title) {
    if (i18n->multibyte()) {
      XRectangle ink, logical;
      XmbTextExtents(screen->getWindowStyle()->fontset,
		     client.title, client.title_len, &ink, &logical);
      client.title_text_w = logical.width;
    } else
      client.title_text_w = XTextWidth(screen->getWindowStyle()->font,
				       client.title, client.title_len);

    client.title_text_w += (frame.bevel_w * 4);
  }

#ifdef    SHAPE
  if (blackbox->hasShapeExtensions()) {
    if (frame.shaped) {
      XShapeCombineShape(display, frame.window, ShapeBounding,
			 (frame.bevel_w * decorations.border),
			 frame.y_border + (frame.bevel_w * decorations.border),
			 client.window, ShapeBounding, ShapeSet);

      int num = 1;
      XRectangle xrect[2];
      xrect[0].x = xrect[0].y = 0;
      xrect[0].width = frame.width;
      xrect[0].height = frame.y_border;

      if (decorations.handle) {
	xrect[1].x = 0;
	xrect[1].y = frame.y_handle;
	xrect[1].width = frame.width;
	xrect[1].height = frame.handle_h + screen->getBorderWidth();
	num++;
      }

      XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
			      xrect, num, ShapeUnion, Unsorted);
    }
  }
#endif // SHAPE

  positionWindows();
  positionButtons();
  decorate();

  XClearWindow(display, frame.window);
  setFocusFlag(focused);
  if (decorations.titlebar) redrawLabel();
  redrawAllButtons();

  configure(frame.x, frame.y, frame.width, frame.height);

  if (! screen->isSloppyFocus())
    XGrabButton(display, Button1, 0, frame.plate, True,
		ButtonReleaseMask, GrabModeSync, GrabModeSync,
		frame.plate, blackbox->getSessionCursor());
  else
    XUngrabButton(display, Button1, 0, frame.plate);

  if (windowmenu) {
    windowmenu->move(windowmenu->getX(), frame.y + frame.title_h);
    windowmenu->reconfigure();
  }
}


void BlackboxWindow::positionWindows(void) {
  XResizeWindow(display, frame.window, frame.width,
                ((shaded) ? frame.title_h : frame.height));
  XSetWindowBorderWidth(display, frame.window, screen->getBorderWidth());
  XMoveResizeWindow(display, frame.plate, (frame.bevel_w * decorations.border),
                    frame.y_border + (frame.bevel_w * decorations.border),
                    client.width, client.height);
  XMoveResizeWindow(display, client.window, 0, 0, client.width, client.height);

  if (decorations.titlebar) {
    XSetWindowBorderWidth(display, frame.title, screen->getBorderWidth());
    XMoveResizeWindow(display, frame.title,
                      -screen->getBorderWidth(), -screen->getBorderWidth(),
                      frame.title_w, frame.title_h);

    positionButtons();
  } else if (frame.title)
    XUnmapWindow(display, frame.title);

  if (decorations.border)
    XMoveResizeWindow(display, frame.border, 0, frame.y_border,
		      frame.border_w, frame.border_h);
  else if (frame.border)
    XUnmapWindow(display, frame.border);

  if (decorations.handle) {
    XSetWindowBorderWidth(display, frame.handle, screen->getBorderWidth());
    XSetWindowBorderWidth(display, frame.left_grip, screen->getBorderWidth());
    XSetWindowBorderWidth(display, frame.right_grip, screen->getBorderWidth());

    XMoveResizeWindow(display, frame.handle, -screen->getBorderWidth(),
                      frame.y_handle - screen->getBorderWidth(),
		      frame.handle_w, frame.handle_h);
    XMoveResizeWindow(display, frame.left_grip, -screen->getBorderWidth(),
		      -screen->getBorderWidth(), frame.grip_w, frame.grip_h);
    XMoveResizeWindow(display, frame.right_grip,
		      frame.width - frame.grip_w - screen->getBorderWidth(),
                      -screen->getBorderWidth(), frame.grip_w, frame.grip_h);
    XMapSubwindows(display, frame.handle);
  } else if (frame.handle)
    XUnmapWindow(display, frame.handle);
}


void BlackboxWindow::getWMName(void) {
  if (client.title) {
    delete [] client.title;
    client.title = 0;
  }

  XTextProperty text_prop;
  char **list;
  int num;

  if (XGetWMName(display, client.window, &text_prop)) {
    if (text_prop.value && text_prop.nitems >0) {
      if (text_prop.encoding != XA_STRING) {
	text_prop.nitems = strlen((char *) text_prop.value);

	if ((XmbTextPropertyToTextList(display, &text_prop,
				       &list, &num) == Success) &&
	    (num > 0) && *list) {
	  client.title = bstrdup(*list);
	  XFreeStringList(list);
	} else
	  client.title = bstrdup((char *) text_prop.value);
      } else
	client.title = bstrdup((char *) text_prop.value);

      XFree((char *) text_prop.value);
    } else
      client.title = bstrdup(i18n->getMessage(
#ifdef    NLS
					      WindowSet, WindowUnnamed,
#else // !NLS
					      0, 0,
#endif //
					      "Unnamed"));
  } else
    client.title = bstrdup(i18n->getMessage(
#ifdef    NLS
					    WindowSet, WindowUnnamed,
#else // !NLS
					    0, 0,
#endif //
					    "Unnamed"));

  client.title_len = strlen(client.title);

  if (i18n->multibyte()) {
    XRectangle ink, logical;
    XmbTextExtents(screen->getWindowStyle()->fontset,
		   client.title, client.title_len, &ink, &logical);
    client.title_text_w = logical.width;
  } else {
    client.title_len = strlen(client.title);
    client.title_text_w = XTextWidth(screen->getWindowStyle()->font,
				     client.title, client.title_len);
  }
  
  client.title_text_w += (frame.bevel_w * 4);
}


void BlackboxWindow::getWMIconName(void) {
  if (client.icon_title) {
    delete [] client.icon_title;
    client.icon_title = (char *) 0;
  }

  XTextProperty text_prop;
  char **list;
  int num;

  if (XGetWMIconName(display, client.window, &text_prop)) {
    if (text_prop.value && text_prop.nitems >0) {
      if (text_prop.encoding != XA_STRING) {
 	text_prop.nitems = strlen((char *) text_prop.value);

 	if ((XmbTextPropertyToTextList(display, &text_prop,
 				       &list, &num) == Success) &&
 	    (num > 0) && *list) {
 	  client.icon_title = bstrdup(*list);
 	  XFreeStringList(list);
 	} else
 	  client.icon_title = bstrdup((char *) text_prop.value);
      } else
 	client.icon_title = bstrdup((char *) text_prop.value);

      XFree((char *) text_prop.value);
    } else
      client.icon_title = bstrdup(i18n->getMessage(
#ifdef    NLS
						   IconSet, IconUnnamed,
#else // !NLS
						   0, 0,
#endif //
						   "Unnamed"));
  } else
    client.icon_title = bstrdup(i18n->getMessage(
#ifdef    NLS
						 IconSet, IconUnnamed,
#else // !NLS
						 0, 0,
#endif //
						 "Unnamed"));
}


void BlackboxWindow::getWMProtocols(void) {
  Atom *proto;
  int num_return = 0;

  if (XGetWMProtocols(display, client.window, &proto, &num_return)) {
    for (int i = 0; i < num_return; ++i) {
      if (proto[i] == blackbox->getWMDeleteAtom())
	functions.close = decorations.close = True;
      else if (proto[i] == blackbox->getWMTakeFocusAtom())
        send_focus_message = True;
      else if (proto[i] == blackbox->getBlackboxStructureMessagesAtom())
        screen->addNetizen(new Netizen(screen, client.window));
    }

    XFree(proto);
  }
}


void BlackboxWindow::getWMHints(void) {
  XWMHints *wmhint = XGetWMHints(display, client.window);
  if (! wmhint) {
    visible = True;
    iconic = False;
    focus_mode = F_Passive;
    client.window_group = None;
    client.initial_state = NormalState;
  } else {
    client.wm_hint_flags = wmhint->flags;
    if (wmhint->flags & InputHint) {
      if (wmhint->input == True) {
	if (send_focus_message)
	  focus_mode = F_LocallyActive;
	else
	  focus_mode = F_Passive;
      } else {
	if (send_focus_message)
	  focus_mode = F_GloballyActive;
	else
	  focus_mode = F_NoInput;
      }
    } else
      focus_mode = F_Passive;

    if (wmhint->flags & StateHint)
      client.initial_state = wmhint->initial_state;
    else
      client.initial_state = NormalState;

    if (wmhint->flags & WindowGroupHint) {
      if (! client.window_group) {
	client.window_group = wmhint->window_group;
	blackbox->saveGroupSearch(client.window_group, this);
      }
    } else
      client.window_group = None;

    XFree(wmhint);
  }
}


void BlackboxWindow::getWMNormalHints(void) {
  long icccm_mask;
  XSizeHints sizehint;
  if (! XGetWMNormalHints(display, client.window, &sizehint, &icccm_mask)) {
    client.min_width = client.min_height =
      client.base_width = client.base_height =
      client.width_inc = client.height_inc = 1;
    client.max_width = screen->getWidth();
    client.max_height = screen->getHeight();
    client.min_aspect_x = client.min_aspect_y =
      client.max_aspect_x = client.max_aspect_y = 1;
    client.win_gravity = NorthWestGravity;
  } else {
    client.normal_hint_flags = sizehint.flags;

    if (sizehint.flags & PMinSize) {
      client.min_width = sizehint.min_width;
      client.min_height = sizehint.min_height;
    } else
      client.min_width = client.min_height = 1;

    if (sizehint.flags & PMaxSize) {
      client.max_width = sizehint.max_width;
      client.max_height = sizehint.max_height;
    } else {
      client.max_width = screen->getWidth();
      client.max_height = screen->getHeight();
    }

    if (sizehint.flags & PResizeInc) {
      client.width_inc = sizehint.width_inc;
      client.height_inc = sizehint.height_inc;
    } else
      client.width_inc = client.height_inc = 1;

    if (sizehint.flags & PAspect) {
      client.min_aspect_x = sizehint.min_aspect.x;
      client.min_aspect_y = sizehint.min_aspect.y;
      client.max_aspect_x = sizehint.max_aspect.x;
      client.max_aspect_y = sizehint.max_aspect.y;
    } else
      client.min_aspect_x = client.min_aspect_y =
	client.max_aspect_x = client.max_aspect_y = 1;

    if (sizehint.flags & PBaseSize) {
      client.base_width = sizehint.base_width;
      client.base_height = sizehint.base_height;
    } else
      client.base_width = client.base_height = 0;

    if (sizehint.flags & PWinGravity)
      client.win_gravity = sizehint.win_gravity;
    else
      client.win_gravity = NorthWestGravity;
  }
}


void BlackboxWindow::getMWMHints(void) {
  int format;
  Atom atom_return;
  unsigned long num, len;

  if (XGetWindowProperty(display, client.window,
                         blackbox->getMotifWMHintsAtom(), 0,
                         PropMwmHintsElements, False,
                         blackbox->getMotifWMHintsAtom(), &atom_return,
                         &format, &num, &len,
                         (unsigned char **) &client.mwm_hint) == Success &&
      client.mwm_hint)
    if (num == PropMwmHintsElements) {
      if (client.mwm_hint->flags & MwmHintsDecorations)
        if (client.mwm_hint->decorations & MwmDecorAll)
          decorations.titlebar = decorations.handle = decorations.border =
            decorations.iconify = decorations.maximize =
            decorations.close = decorations.menu = True;
        else {
          decorations.titlebar = decorations.handle = decorations.border =
            decorations.iconify = decorations.maximize =
            decorations.close = decorations.menu = False;

          if (client.mwm_hint->decorations & MwmDecorBorder)
            decorations.border = True;
          if (client.mwm_hint->decorations & MwmDecorHandle)
            decorations.handle = True;
          if (client.mwm_hint->decorations & MwmDecorTitle)
            decorations.titlebar = True;
          if (client.mwm_hint->decorations & MwmDecorMenu)
            decorations.menu = True;
          if (client.mwm_hint->decorations & MwmDecorIconify)
            decorations.iconify = True;
          if (client.mwm_hint->decorations & MwmDecorMaximize)
            decorations.maximize = True;
        }

      if (client.mwm_hint->flags & MwmHintsFunctions)
        if (client.mwm_hint->functions & MwmFuncAll)
          functions.resize = functions.move = functions.iconify =
            functions.maximize = functions.close = True;
        else {
          functions.resize = functions.move = functions.iconify =
            functions.maximize = functions.close = False;

          if (client.mwm_hint->functions & MwmFuncResize)
            functions.resize = True;
          if (client.mwm_hint->functions & MwmFuncMove)
            functions.move = True;
          if (client.mwm_hint->functions & MwmFuncIconify)
            functions.iconify = True;
          if (client.mwm_hint->functions & MwmFuncMaximize)
            functions.maximize = True;
          if (client.mwm_hint->functions & MwmFuncClose)
            functions.close = True;
        }
    }
}


void BlackboxWindow::getBlackboxHints(void) {
  int format;
  Atom atom_return;
  unsigned long num, len;

  if (XGetWindowProperty(display, client.window,
                         blackbox->getBlackboxHintsAtom(), 0,
                         PropBlackboxHintsElements, False,
                         blackbox->getBlackboxHintsAtom(), &atom_return,
                         &format, &num, &len,
                         (unsigned char **) &client.blackbox_hint) == Success &&
      client.blackbox_hint)
    if (num == PropBlackboxHintsElements) {
      if (client.blackbox_hint->flags & AttribShaded)
        shaded = (client.blackbox_hint->attrib & AttribShaded);

      if ((client.blackbox_hint->flags & AttribMaxHoriz) &&
          (client.blackbox_hint->flags & AttribMaxVert))
        maximized = ((client.blackbox_hint->attrib &
                      (AttribMaxHoriz | AttribMaxVert)) ?  1 : 0);
      else if (client.blackbox_hint->flags & AttribMaxVert)
        maximized = ((client.blackbox_hint->attrib & AttribMaxVert) ? 2 : 0);
      else if (client.blackbox_hint->flags & AttribMaxHoriz)
        maximized = ((client.blackbox_hint->attrib & AttribMaxHoriz) ? 3 : 0);

      if (client.blackbox_hint->flags & AttribOmnipresent)
        stuck = (client.blackbox_hint->attrib & AttribOmnipresent);

      if (client.blackbox_hint->flags & AttribWorkspace)
        workspace_number = client.blackbox_hint->workspace;

      // if (client.blackbox_hint->flags & AttribStack)
      //   don't yet have always on top/bottom for blackbox yet... working
      //   on that

      if (client.blackbox_hint->flags & AttribDecoration) {
        switch (client.blackbox_hint->decoration) {
        case DecorNone:
          decorations.titlebar = decorations.border = decorations.handle =
            decorations.iconify = decorations.maximize =
            decorations.menu = False;
          functions.resize = functions.move = functions.iconify =
            functions.maximize = False;

          break;

        default:
        case DecorNormal:
          decorations.titlebar = decorations.border = decorations.handle =
            decorations.iconify = decorations.maximize =
            decorations.menu = True;
          functions.resize = functions.move = functions.iconify =
            functions.maximize = True;

          break;

        case DecorTiny:
          decorations.titlebar = decorations.iconify = decorations.menu =
            functions.move = functions.iconify = True;
          decorations.border = decorations.handle = decorations.maximize =
            functions.resize = functions.maximize = False;

          break;

        case DecorTool:
          decorations.titlebar = decorations.menu = functions.move = True;
          decorations.iconify = decorations.border = decorations.handle =
            decorations.maximize = functions.resize = functions.maximize =
            functions.iconify = False;

          break;
        }

        reconfigure();
      }
    }
}


void BlackboxWindow::configure(int dx, int dy,
                               unsigned int dw, unsigned int dh) {
  if ((dw != frame.width) || (dh != frame.height)) {
    if ((((signed) frame.width) + dx) < 0) dx = 0;
    if ((((signed) frame.height) + dy) < 0) dy = 0;

    frame.x = dx;
    frame.y = dy;
    frame.width = dw;
    frame.height = dh;

    downsize();

    XMoveResizeWindow(display, frame.window, frame.x, frame.y, frame.width,
                      frame.height);

    positionWindows();
    decorate();
    setFocusFlag(focused);
    redrawAllButtons();
    shaded = False;
  } else {
    frame.x = dx;
    frame.y = dy;

    XMoveWindow(display, frame.window, frame.x, frame.y);

    if (! moving) {
      client.x = dx + (frame.bevel_w * decorations.border) +
        screen->getBorderWidth();
      client.y = dy + frame.y_border + (frame.bevel_w * decorations.border) +
        screen->getBorderWidth();

      XEvent event;
      event.type = ConfigureNotify;

      event.xconfigure.display = display;
      event.xconfigure.event = client.window;
      event.xconfigure.window = client.window;
      event.xconfigure.x = client.x;
      event.xconfigure.y = client.y;
      event.xconfigure.width = client.width;
      event.xconfigure.height = client.height;
      event.xconfigure.border_width = client.old_bw;
      event.xconfigure.above = frame.window;
      event.xconfigure.override_redirect = False;

      XSendEvent(display, client.window, False, StructureNotifyMask, &event);

      screen->updateNetizenConfigNotify(&event);
    }
  }
}


Bool BlackboxWindow::setInputFocus(void) {
  if (((signed) (frame.x + frame.width)) < 0) {
    if (((signed) (frame.y + frame.y_border)) < 0)
      configure(screen->getBorderWidth(), screen->getBorderWidth(),
                frame.width, frame.height);
    else if (frame.y > (signed) screen->getHeight())
      configure(screen->getBorderWidth(), screen->getHeight() - frame.height,
                frame.width, frame.height);
    else
      configure(screen->getBorderWidth(), frame.y + screen->getBorderWidth(),
                frame.width, frame.height);
  } else if (frame.x > (signed) screen->getWidth()) {
    if (((signed) (frame.y + frame.y_border)) < 0)
      configure(screen->getWidth() - frame.width, screen->getBorderWidth(),
                frame.width, frame.height);
    else if (frame.y > (signed) screen->getHeight())
      configure(screen->getWidth() - frame.width,
	        screen->getHeight() - frame.height, frame.width, frame.height);
    else
      configure(screen->getWidth() - frame.width,
                frame.y + screen->getBorderWidth(), frame.width, frame.height);
  }


  blackbox->grab();
  if (! validateClient()) return False;

  Bool ret = False;

  if (client.transient && modal)
    ret = client.transient->setInputFocus();
  else {
    if (! focused) {
      if (focus_mode == F_LocallyActive || focus_mode == F_Passive)
	XSetInputFocus(display, client.window,
                       RevertToPointerRoot, CurrentTime);
      else
        XSetInputFocus(display, screen->getRootWindow(),
                       RevertToNone, CurrentTime);

      blackbox->setFocusedWindow(this);

      if (send_focus_message) {
        XEvent ce;
        ce.xclient.type = ClientMessage;
        ce.xclient.message_type = blackbox->getWMProtocolsAtom();
        ce.xclient.display = display;
        ce.xclient.window = client.window;
        ce.xclient.format = 32;
        ce.xclient.data.l[0] = blackbox->getWMTakeFocusAtom();
        ce.xclient.data.l[1] = blackbox->getLastTime();
        ce.xclient.data.l[2] = 0l;
        ce.xclient.data.l[3] = 0l;
        ce.xclient.data.l[4] = 0l;
        XSendEvent(display, client.window, False, NoEventMask, &ce);
      }

      if (screen->isSloppyFocus() && screen->doAutoRaise()) timer->start();

      ret = True;
    }
  }

  blackbox->ungrab();

  return ret;
}


void BlackboxWindow::iconify(void) {
  if (iconic) return;

  if (windowmenu) windowmenu->hide();

  setState(IconicState);

  XSelectInput(display, client.window, NoEventMask);
  XUnmapWindow(display, client.window);
  XSelectInput(display, client.window,
               PropertyChangeMask | StructureNotifyMask);

  XUnmapWindow(display, frame.window);
  visible = False;
  iconic = True;

  screen->getWorkspace(workspace_number)->removeWindow(this);

  if (transient && client.transient_for) {
    if (! client.transient_for->iconic)
      client.transient_for->iconify();
  } else
    screen->addIcon(this);

  if (client.transient)
    if (! client.transient->iconic)
      client.transient->iconify();
}


void BlackboxWindow::deiconify(Bool reassoc, Bool raise) {
  if (iconic || reassoc)
    screen->reassociateWindow(this, -1, False);
  else if (workspace_number != screen->getCurrentWorkspace()->getWorkspaceID())
    return;

  setState(NormalState);

  XSelectInput(display, client.window, NoEventMask);
  XMapWindow(display, client.window);
  XSelectInput(display, client.window,
               PropertyChangeMask | StructureNotifyMask);

  XMapSubwindows(display, frame.window);
  XMapWindow(display, frame.window);

  if (iconic && screen->doFocusNew()) setInputFocus();

  visible = True;
  iconic = False;

  if (client.transient) client.transient->deiconify();

  if (raise)
    screen->getWorkspace(workspace_number)->raiseWindow(this);
}


void BlackboxWindow::close(void) {
  XEvent ce;
  ce.xclient.type = ClientMessage;
  ce.xclient.message_type = blackbox->getWMProtocolsAtom();
  ce.xclient.display = display;
  ce.xclient.window = client.window;
  ce.xclient.format = 32;
  ce.xclient.data.l[0] = blackbox->getWMDeleteAtom();
  ce.xclient.data.l[1] = CurrentTime;
  ce.xclient.data.l[2] = 0l;
  ce.xclient.data.l[3] = 0l;
  ce.xclient.data.l[4] = 0l;
  XSendEvent(display, client.window, False, NoEventMask, &ce);
}


void BlackboxWindow::withdraw(void) {
  visible = False;
  iconic = False;

  setState(WithdrawnState);
  XUnmapWindow(display, frame.window);

  XSelectInput(display, client.window, NoEventMask);
  XUnmapWindow(display, client.window);
  XSelectInput(display, client.window,
               PropertyChangeMask | StructureNotifyMask);

  if (windowmenu) windowmenu->hide();
}


void BlackboxWindow::maximize(unsigned int button) {
  if (! maximized) {
    int dx, dy;
    unsigned int dw, dh;

    blackbox_attrib.premax_x = frame.x;
    blackbox_attrib.premax_y = frame.y;
    blackbox_attrib.premax_w = frame.width;
    blackbox_attrib.premax_h = frame.height;

    dw = screen->getWidth();
    dw -= screen->getBorderWidth2x();
    dw -= ((frame.bevel_w * 2) * decorations.border);
    dw -= client.base_width;

    dh = screen->getHeight();
    dh -= screen->getBorderWidth2x();
    dh -= ((frame.bevel_w * 2) * decorations.border);
    dh -= ((frame.handle_h + screen->getBorderWidth()) * decorations.handle);
    dh -= client.base_height;
    dh -= frame.y_border;

    if (! screen->doFullMax())
      dh -= screen->getToolbar()->getHeight();

    if (dw < client.min_width) dw = client.min_width;
    if (dh < client.min_height) dh = client.min_height;
    if (dw > client.max_width) dw = client.max_width;
    if (dh > client.max_height) dh = client.max_height;

    dw -= (dw % client.width_inc);
    dw += client.base_width;
    dh -= (dh % client.height_inc);
    dh += client.base_height;

    dw += ((frame.bevel_w * 2) * decorations.border);

    dh += frame.y_border;
    dh += (frame.handle_h + screen->getBorderWidth());
    dh += ((frame.bevel_w * 2) * decorations.border);

    dx = ((screen->getWidth() - dw) / 2) - screen->getBorderWidth();

    if (screen->doFullMax()) {
      dy = ((screen->getHeight() - dh) / 2) - screen->getBorderWidth();
    } else {
      dy = (((screen->getHeight() -
	      screen->getToolbar()->getHeight()) - dh) / 2) -
	   screen->getBorderWidth();

      switch (screen->getToolbarPlacement()) {
      case Toolbar::TopLeft:
      case Toolbar::TopCenter:
      case Toolbar::TopRight:
        dy += screen->getToolbar()->getHeight() + screen->getBorderWidth();
        break;
      }
    }

    if (button == 2) {
      dw = frame.width;
      dx = frame.x;
    } else if (button == 3) {
      dh = frame.height;
      dy = frame.y;
    }

    switch(button) {
    case 1:
      blackbox_attrib.flags |= AttribMaxHoriz | AttribMaxVert;
      blackbox_attrib.attrib |= AttribMaxHoriz | AttribMaxVert;

      break;

    case 2:
      blackbox_attrib.flags |= AttribMaxVert;
      blackbox_attrib.attrib |= AttribMaxVert;

      break;

    case 3:
      blackbox_attrib.flags |= AttribMaxHoriz;
      blackbox_attrib.attrib |= AttribMaxHoriz;

      break;
    }

    if (shaded) {
      blackbox_attrib.flags ^= AttribShaded;
      blackbox_attrib.attrib ^= AttribShaded;
      shaded = False;
    }

    maximized = True;

    configure(dx, dy, dw, dh);
    screen->getWorkspace(workspace_number)->raiseWindow(this);
    setState(current_state);
  } else {
    maximized = False;

    if (blackbox_attrib.flags & AttribMaxHoriz)
      blackbox_attrib.flags ^= AttribMaxHoriz;
    if (blackbox_attrib.flags & AttribMaxVert)
      blackbox_attrib.flags ^= AttribMaxVert;
    if (blackbox_attrib.attrib & AttribMaxHoriz)
      blackbox_attrib.attrib ^= AttribMaxHoriz;
    if(blackbox_attrib.attrib & AttribMaxVert)
      blackbox_attrib.attrib ^= AttribMaxVert;

    configure(blackbox_attrib.premax_x, blackbox_attrib.premax_y,
	      blackbox_attrib.premax_w, blackbox_attrib.premax_h);

    blackbox_attrib.premax_x = blackbox_attrib.premax_y = 0;
    blackbox_attrib.premax_w = blackbox_attrib.premax_h = 0;

    redrawAllButtons();
    setState(current_state);
  }
}


void BlackboxWindow::setWorkspace(int n) {
  workspace_number = n;

  blackbox_attrib.flags |= AttribWorkspace;
  blackbox_attrib.workspace = workspace_number;
}


void BlackboxWindow::shade(void) {
  if (decorations.titlebar)
    if (shaded) {
      XResizeWindow(display, frame.window, frame.width, frame.height);
      shaded = False;
      blackbox_attrib.flags ^= AttribShaded;
      blackbox_attrib.attrib ^= AttribShaded;

      setState(NormalState);
    } else {
      XResizeWindow(display, frame.window, frame.width, frame.title_h);
      shaded = True;
      blackbox_attrib.flags |= AttribShaded;
      blackbox_attrib.attrib |= AttribShaded;

      setState(IconicState);
    }
}


void BlackboxWindow::stick(void) {
  if (stuck) {
    blackbox_attrib.flags ^= AttribOmnipresent;
    blackbox_attrib.attrib ^= AttribOmnipresent;

    stuck = False;

    if (! iconic)
      screen->reassociateWindow(this, -1, True);

    setState(current_state);
  } else {
    stuck = True;

    blackbox_attrib.flags |= AttribOmnipresent;
    blackbox_attrib.attrib |= AttribOmnipresent;

    setState(current_state);
  }
}


void BlackboxWindow::setFocusFlag(Bool focus) {
  focused = focus;

  if (decorations.titlebar) {
    XSetWindowBackgroundPixmap(display, frame.title,
			       (focused) ? frame.ftitle : frame.utitle);
    XSetWindowBackgroundPixmap(display, frame.label,
			       (focused) ? frame.flabel : frame.ulabel);
    XClearWindow(display, frame.title);
    XClearWindow(display, frame.label);

    redrawLabel();
    redrawAllButtons();
  }

  if (decorations.handle) {
    XSetWindowBackgroundPixmap(display, frame.handle,
			       (focused) ? frame.fhandle : frame.uhandle);
    XSetWindowBackgroundPixmap(display, frame.right_grip,
			       (focused) ? frame.fgrip : frame.ugrip);
    XSetWindowBackgroundPixmap(display, frame.left_grip,
			       (focused) ? frame.fgrip : frame.ugrip);
    XClearWindow(display, frame.handle);
    XClearWindow(display, frame.right_grip);
    XClearWindow(display, frame.left_grip);
  }

  if (decorations.border) {
    XSetWindowBackgroundPixmap(display, frame.border,
			       (focused) ? frame.fborder : frame.uborder);
    XClearWindow(display, frame.border);
  }

  if (screen->isSloppyFocus() && screen->doAutoRaise()) timer->stop();
}


void BlackboxWindow::installColormap(Bool install) {
  blackbox->grab();
  if (! validateClient()) return;

  int i = 0, ncmap = 0;
  Colormap *cmaps = XListInstalledColormaps(display, client.window, &ncmap);
  XWindowAttributes wattrib;
  if (cmaps) {
    if (XGetWindowAttributes(display, client.window, &wattrib)) {
      if (install) {
	// install the window's colormap
	for (i = 0; i < ncmap; i++)
	  if (*(cmaps + i) == wattrib.colormap)
	    // this window is using an installed color map... do not install
	    install = False;

	// otherwise, install the window's colormap
	if (install)
	  XInstallColormap(display, wattrib.colormap);
      } else {
	// uninstall the window's colormap
	for (i = 0; i < ncmap; i++)
	  if (*(cmaps + i) == wattrib.colormap)
	    // we found the colormap to uninstall
	    XUninstallColormap(display, wattrib.colormap);
      }
    }

    XFree(cmaps);
  }

  blackbox->ungrab();
}


void BlackboxWindow::setState(unsigned long new_state) {
  current_state = new_state;

  unsigned long state[2];
  state[0] = (unsigned long) current_state;
  state[1] = (unsigned long) None;
  XChangeProperty(display, client.window, blackbox->getWMStateAtom(),
		  blackbox->getWMStateAtom(), 32, PropModeReplace,
		  (unsigned char *) state, 2);

  XChangeProperty(display, client.window, blackbox->getBlackboxAttributesAtom(),
                  blackbox->getBlackboxAttributesAtom(), 32, PropModeReplace,
                  (unsigned char *) &blackbox_attrib, PropBlackboxAttributesElements);
}


Bool BlackboxWindow::getState(void) {
  current_state = 0;

  Atom atom_return;
  Bool ret = False;
  int foo;
  unsigned long *state, ulfoo, nitems;

  if ((XGetWindowProperty(display, client.window, blackbox->getWMStateAtom(),
			  0l, 2l, False, blackbox->getWMStateAtom(),
			  &atom_return, &foo, &nitems, &ulfoo,
			  (unsigned char **) &state) != Success) ||
      (! state)) {
    blackbox->ungrab();
    return False;
  }

  if (nitems >= 1) {
    current_state = (unsigned long) state[0];

    ret = True;
  }

  XFree((void *) state);

  return ret;
}


void BlackboxWindow::setGravityOffsets(void) {
  // translate x coordinate
  switch (client.win_gravity) {
    // handle Westward gravity
  case NorthWestGravity:
  case WestGravity:
  case SouthWestGravity:
  default:
    frame.x = client.x;
      break;

    // handle Eastward gravity
  case NorthEastGravity:
  case EastGravity:
  case SouthEastGravity:
    frame.x = (client.x + client.width) - frame.width;
    break;

    // no x translation desired - default
  case StaticGravity:
  case ForgetGravity:
  case CenterGravity:
    frame.x = client.x - (frame.bevel_w * decorations.border) -
      screen->getBorderWidth();
  }

  // translate y coordinate
  switch (client.win_gravity) {
    // handle Northbound gravity
  case NorthWestGravity:
  case NorthGravity:
  case NorthEastGravity:
  default:
    frame.y = client.y;
    break;

    // handle Southbound gravity
  case SouthWestGravity:
  case SouthGravity:
  case SouthEastGravity:
    frame.y = (client.y + client.height) - frame.height;
    break;

    // no y translation desired - default
  case StaticGravity:
  case ForgetGravity:
  case CenterGravity:
    frame.y = client.y - frame.y_border -
      (frame.bevel_w * decorations.border) - screen->getBorderWidth();
    break;
  }
}


void BlackboxWindow::restoreAttributes(void) {
  if (! getState()) current_state = NormalState;

  Atom atom_return;
  int foo;
  unsigned long ulfoo, nitems;

  BlackboxAttributes *net;
  if (XGetWindowProperty(display, client.window,
			 blackbox->getBlackboxAttributesAtom(), 0l,
			 PropBlackboxAttributesElements, False,
			 blackbox->getBlackboxAttributesAtom(), &atom_return, &foo,
			 &nitems, &ulfoo, (unsigned char **) &net) ==
      Success && net && nitems == PropBlackboxAttributesElements) {
    blackbox_attrib.flags = net->flags;
    blackbox_attrib.attrib = net->attrib;
    blackbox_attrib.workspace = net->workspace;
    blackbox_attrib.stack = net->stack;
    blackbox_attrib.premax_x = net->premax_x;
    blackbox_attrib.premax_y = net->premax_y;
    blackbox_attrib.premax_w = net->premax_w;
    blackbox_attrib.premax_h = net->premax_h;

    XFree((void *) net);
  } else
    return;

  if (blackbox_attrib.flags & AttribShaded &&
      blackbox_attrib.attrib & AttribShaded) {
    int save_state =
      ((current_state == IconicState) ? NormalState : current_state);

    shaded = False;
    shade();

    current_state = save_state;
  }

  if (((int) blackbox_attrib.workspace != screen->getCurrentWorkspaceID()) &&
      ((int) blackbox_attrib.workspace < screen->getCount())) {
    screen->reassociateWindow(this, blackbox_attrib.workspace, True);

    if (current_state == NormalState) current_state = WithdrawnState;
  } else if (current_state == WithdrawnState)
    current_state = NormalState;

  if (blackbox_attrib.flags & AttribOmnipresent &&
      blackbox_attrib.attrib & AttribOmnipresent) {
    stuck = False;
    stick();

    current_state = NormalState;
  }

  if ((blackbox_attrib.flags & AttribMaxHoriz) ||
      (blackbox_attrib.flags & AttribMaxVert)) {
    int x = blackbox_attrib.premax_x, y = blackbox_attrib.premax_y;
    unsigned int w = blackbox_attrib.premax_w, h = blackbox_attrib.premax_h;
    maximized = False;

    int m;
    if ((blackbox_attrib.flags & AttribMaxHoriz) &&
        (blackbox_attrib.flags & AttribMaxVert))
      m = ((blackbox_attrib.attrib & (AttribMaxHoriz | AttribMaxVert)) ?
           1 : 0);
    else if (blackbox_attrib.flags & AttribMaxVert)
      m = ((blackbox_attrib.attrib & AttribMaxVert) ? 2 : 0);
    else if (blackbox_attrib.flags & AttribMaxHoriz)
      m = ((blackbox_attrib.attrib & AttribMaxHoriz) ? 3 : 0);
    else
      m = 0;

    if (m) maximize(m);

    blackbox_attrib.premax_x = x;
    blackbox_attrib.premax_y = y;
    blackbox_attrib.premax_w = w;
    blackbox_attrib.premax_h = h;
  }

  setState(current_state);
}


void BlackboxWindow::restoreGravity(void) {
  // restore x coordinate
  switch (client.win_gravity) {
    // handle Westward gravity
  case NorthWestGravity:
  case WestGravity:
  case SouthWestGravity:
  default:
    client.x = frame.x;
    break;

    // handle Eastward gravity
  case NorthEastGravity:
  case EastGravity:
  case SouthEastGravity:
    client.x = (frame.x + frame.width) - client.width;
    break;
  }

  // restore y coordinate
  switch (client.win_gravity) {
    // handle Northbound gravity
  case NorthWestGravity:
  case NorthGravity:
  case NorthEastGravity:
  default:
    client.y = frame.y;
    break;

    // handle Southbound gravity
  case SouthWestGravity:
  case SouthGravity:
  case SouthEastGravity:
    client.y = (frame.y + frame.height) - client.height;
    break;
  }
}


void BlackboxWindow::redrawLabel(void) {
  int dx = (frame.bevel_w * 2), dlen = client.title_len;
  unsigned int l = client.title_text_w;

  if (client.title_text_w > frame.label_w)
    for (; dlen >= 0; dlen--) {
      if (i18n->multibyte()) {
	XRectangle ink, logical;
	XmbTextExtents(screen->getWindowStyle()->fontset, client.title, dlen,
		       &ink, &logical);
	l = logical.width;
      } else
	l = XTextWidth(screen->getWindowStyle()->font, client.title, dlen);
      
      l += (frame.bevel_w * 4);
      
      if (l < frame.label_w)
	break;
    }

  switch (screen->getWindowStyle()->justify) {
  case BScreen::RightJustify:
    dx += frame.label_w - l;
    break;

  case BScreen::CenterJustify:
    dx += (frame.label_w - l) / 2;
    break;
  }

  if (i18n->multibyte())
    XmbDrawString(display, frame.label,
		  screen->getWindowStyle()->fontset,
		  ((focused) ? screen->getWindowStyle()->l_text_focus_gc :
		   screen->getWindowStyle()->l_text_unfocus_gc), dx, 1 -
		  screen->getWindowStyle()->fontset_extents->max_ink_extent.y,
		  client.title, dlen);
  else
    XDrawString(display, frame.label,
		((focused) ? screen->getWindowStyle()->l_text_focus_gc :
		 screen->getWindowStyle()->l_text_unfocus_gc), dx,
		screen->getWindowStyle()->font->ascent + 1, client.title,
		dlen);
}


void BlackboxWindow::redrawAllButtons(void) {
  if (frame.iconify_button) redrawIconifyButton(False);
  if (frame.maximize_button) redrawMaximizeButton(maximized);
  if (frame.close_button) redrawCloseButton(False);
}


void BlackboxWindow::redrawIconifyButton(Bool pressed) {
  if (! pressed) {
    XSetWindowBackgroundPixmap(display, frame.iconify_button,
			       ((focused) ? frame.fbutton : frame.ubutton));
    XClearWindow(display, frame.iconify_button);
  } else if (frame.pbutton) {
    XSetWindowBackgroundPixmap(display, frame.iconify_button, frame.pbutton);
    XClearWindow(display, frame.iconify_button);
  }

  XDrawRectangle(display, frame.iconify_button,
		 ((focused) ? screen->getWindowStyle()->b_pic_focus_gc :
		  screen->getWindowStyle()->b_pic_unfocus_gc),
		 2, frame.button_h - 5, frame.button_w - 5, 2);
}


void BlackboxWindow::redrawMaximizeButton(Bool pressed) {
  if (! pressed) {
    XSetWindowBackgroundPixmap(display, frame.maximize_button,
			       ((focused) ? frame.fbutton : frame.ubutton));
    XClearWindow(display, frame.maximize_button);
  } else if (frame.pbutton) {
    XSetWindowBackgroundPixmap(display, frame.maximize_button, frame.pbutton);
    XClearWindow(display, frame.maximize_button);
  }

  XDrawRectangle(display, frame.maximize_button,
		 ((focused) ? screen->getWindowStyle()->b_pic_focus_gc :
		  screen->getWindowStyle()->b_pic_unfocus_gc),
		 2, 2, frame.button_w - 5, frame.button_h - 5);
  XDrawLine(display, frame.maximize_button,
	    ((focused) ? screen->getWindowStyle()->b_pic_focus_gc :
	     screen->getWindowStyle()->b_pic_unfocus_gc),
	    2, 3, frame.button_w - 3, 3);
}


void BlackboxWindow::redrawCloseButton(Bool pressed) {
  if (! pressed) {
    XSetWindowBackgroundPixmap(display, frame.close_button,
			       ((focused) ? frame.fbutton : frame.ubutton));
    XClearWindow(display, frame.close_button);
  } else if (frame.pbutton) {
    XSetWindowBackgroundPixmap(display, frame.close_button, frame.pbutton);
    XClearWindow(display, frame.close_button);
  }

  XDrawLine(display, frame.close_button,
	    ((focused) ? screen->getWindowStyle()->b_pic_focus_gc :
	     screen->getWindowStyle()->b_pic_unfocus_gc), 2, 2,
            frame.button_w - 3, frame.button_h - 3);
  XDrawLine(display, frame.close_button,
	    ((focused) ? screen->getWindowStyle()->b_pic_focus_gc :
	     screen->getWindowStyle()->b_pic_unfocus_gc), 2,
	    frame.button_h - 3,
            frame.button_w - 3, 2);
}


void BlackboxWindow::mapRequestEvent(XMapRequestEvent *re) {
  if (re->window == client.window) {
#ifdef    DEBUG
    fprintf(stderr,
	    i18n->getMessage(
#ifdef    NLS
			     WindowSet, WindowMapRequest,
#else // !NLS
			     0, 0,
#endif // NLS
			     "BlackboxWindow::mapRequestEvent() for 0x%lx\n"),
            client.window);
#endif // DEBUG

    blackbox->grab();
    if (! validateClient()) return;

    Bool get_state_ret = getState();
    if (! (get_state_ret && blackbox->isStartup())) {
      if ((client.wm_hint_flags & StateHint) &&
          (! (current_state == NormalState || current_state == IconicState)))
        current_state = client.initial_state;
      else
        current_state = NormalState;
    } else if (iconic)
      current_state = NormalState;

    switch (current_state) {
    case IconicState:
      iconify();

      break;

    case WithdrawnState:
      withdraw();

      break;

    case NormalState:
    case InactiveState:
    case ZoomState:
    default:
      deiconify(False);

      break;
    }

    blackbox->ungrab();
  }
}


void BlackboxWindow::mapNotifyEvent(XMapEvent *ne) {
  if ((ne->window == client.window) && (! ne->override_redirect) && (visible)) {
    blackbox->grab();
    if (! validateClient()) return;

    if (decorations.titlebar) positionButtons();

    setState(NormalState);

    redrawAllButtons();

    if (transient || screen->doFocusNew())
      setInputFocus();
    else
      setFocusFlag(False);

    visible = True;
    iconic = False;

    blackbox->ungrab();
  }
}


void BlackboxWindow::unmapNotifyEvent(XUnmapEvent *ue) {
  if (ue->window == client.window) {
#ifdef    DEBUG
    fprintf(stderr,
	    i18n->getMessage(
#ifdef    NLS
			     WindowSet, WindowUnmapNotify,
#else // !NLS
			     0, 0,
#endif // NLS
			     "BlackboxWindow::unmapNotifyEvent() for 0x%lx\n"),
            client.window);
#endif // DEBUG

    blackbox->grab();
    if (! validateClient()) return;

    XChangeSaveSet(display, client.window, SetModeDelete);
    XSelectInput(display, client.window, NoEventMask);

    XDeleteProperty(display, client.window, blackbox->getWMStateAtom());
    XDeleteProperty(display, client.window, blackbox->getBlackboxAttributesAtom());

    XUnmapWindow(display, frame.window);
    XUnmapWindow(display, client.window);

    XEvent dummy;
    if (! XCheckTypedWindowEvent(display, client.window, ReparentNotify,
				 &dummy)) {
#ifdef    DEBUG
      fprintf(stderr,
	      i18n->getMessage(
#ifdef    NLS
			       WindowSet, WindowUnmapNotifyReparent,
#else // !NLS
			       0, 0,
#endif // NLS
			       "BlackboxWindow::unmapNotifyEvent(): reparent 0x%lx to "
			       "root.\n"), client.window);
#endif // DEBUG

      restoreGravity();
      XReparentWindow(display, client.window, screen->getRootWindow(),
		      client.x, client.y);
    }

    XFlush(display);

    blackbox->ungrab();

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
  blackbox->grab();
  if (! validateClient()) return;

  switch(atom) {
  case XA_WM_CLASS:
  case XA_WM_CLIENT_MACHINE:
  case XA_WM_COMMAND:
    break;

  case XA_WM_TRANSIENT_FOR:
    // determine if this is a transient window
    Window win;
    if (XGetTransientForHint(display, client.window, &win)) {
      if (win && (win != client.window))
        if ((client.transient_for = blackbox->searchWindow(win))) {
          client.transient_for->client.transient = this;
          stuck = client.transient_for->stuck;
          transient = True;
        } else if (win == client.window_group) {
          if ((client.transient_for = blackbox->searchGroup(win, this))) {
            client.transient_for->client.transient = this;
            stuck = client.transient_for->stuck;
            transient = True;
          }
        }

      if (win == screen->getRootWindow()) modal = True;
    }

    // edit the window decorations for the transient window
    if (! transient) {
      if ((client.normal_hint_flags & PMinSize) &&
          (client.normal_hint_flags & PMaxSize)) {
        if (client.max_width <= client.min_width &&
            client.max_height <= client.min_height)
          decorations.maximize = decorations.handle =
	      functions.resize = functions.maximize = False;
        else
          decorations.maximize = decorations.handle =
	      functions.resize = functions.maximize = True;
      }
    } else {
      decorations.border = decorations.handle = decorations.maximize =
        functions.resize = functions.maximize = False;
    }

    reconfigure();

    break;

  case XA_WM_HINTS:
    getWMHints();
    break;

  case XA_WM_ICON_NAME:
    getWMIconName();
    if (iconic) screen->iconUpdate();
    break;

  case XA_WM_NAME:
    getWMName();

    if (decorations.titlebar) {
      XClearWindow(display, frame.label);
      redrawLabel();
    }

    if (! iconic)
      screen->getWorkspace(workspace_number)->update();

    break;

  case XA_WM_NORMAL_HINTS: {
    getWMNormalHints();

    if ((client.normal_hint_flags & PMinSize) &&
        (client.normal_hint_flags & PMaxSize)) {
      if (client.max_width <= client.min_width &&
          client.max_height <= client.min_height)
        decorations.maximize = decorations.handle =
	    functions.resize = functions.maximize = False;
      else
        decorations.maximize = decorations.handle =
	    functions.resize = functions.maximize = True;
    }

    int x = frame.x, y = frame.y;
    unsigned int w = frame.width, h = frame.height;

    upsize();

    if ((x != frame.x) || (y != frame.y) ||
        (w != frame.width) || (h != frame.height))
      reconfigure();

    break; }

  default:
    if (atom == blackbox->getWMProtocolsAtom()) {
      getWMProtocols();

      if (decorations.close && (! frame.close_button)) {
        createCloseButton();
        if (decorations.titlebar) positionButtons();
        if (windowmenu) windowmenu->reconfigure();
      }
    }

    break;
  }

  blackbox->ungrab();
}


void BlackboxWindow::exposeEvent(XExposeEvent *ee) {
  if (frame.label == ee->window && decorations.titlebar)
    redrawLabel();
  else if (frame.close_button == ee->window)
    redrawCloseButton(False);
  else if (frame.maximize_button == ee->window)
    redrawMaximizeButton(maximized);
  else if (frame.iconify_button == ee->window)
    redrawIconifyButton(False);
}


void BlackboxWindow::configureRequestEvent(XConfigureRequestEvent *cr) {
  if (cr->window == client.window) {
    blackbox->grab();
    if (! validateClient()) return;

    int cx = frame.x, cy = frame.y;
    unsigned int cw = frame.width, ch = frame.height;

    if (cr->value_mask & CWBorderWidth)
      client.old_bw = cr->border_width;

    if (cr->value_mask & CWX)
      cx = cr->x - (frame.bevel_w * decorations.border) -
        screen->getBorderWidth();

    if (cr->value_mask & CWY)
      cy = cr->y - frame.y_border - (frame.bevel_w * decorations.border) -
        screen->getBorderWidth();

    if (cr->value_mask & CWWidth)
      cw = cr->width + ((frame.bevel_w * 2) * decorations.border);

    if (cr->value_mask & CWHeight)
      ch = cr->height + frame.y_border +
	((frame.bevel_w * 2) * decorations.border) +
        (screen->getBorderWidth() * decorations.handle) + frame.handle_h;

    if (frame.x != cx || frame.y != cy ||
        frame.width != cw || frame.height != ch)
      configure(cx, cy, cw, ch);

    if (cr->value_mask & CWStackMode) {
      switch (cr->detail) {
      case Above:
      case TopIf:
      default:
	if (iconic) deiconify();
	screen->getWorkspace(workspace_number)->raiseWindow(this);
	break;

      case Below:
      case BottomIf:
	if (iconic) deiconify();
	screen->getWorkspace(workspace_number)->lowerWindow(this);
	break;
      }
    }

    blackbox->ungrab();
  }
}


void BlackboxWindow::buttonPressEvent(XButtonEvent *be) {
  blackbox->grab();
  if (! validateClient()) return;

  if (frame.maximize_button == be->window) {
    redrawMaximizeButton(True);
  } else if (be->button == 1 || (be->button == 2 && be->state & Mod1Mask)) {
    if ((! focused) && (! screen->isSloppyFocus()))
      setInputFocus();

    if (frame.title == be->window || frame.label == be->window ||
	frame.handle == be->window || frame.right_grip == be->window ||
	frame.left_grip == be->window || frame.border == be->window ||
	frame.window == be->window) {
      if (frame.title == be->window || frame.label == be->window) {
	if (((be->time - lastButtonPressTime) <=
	     blackbox->getDoubleClickInterval()) ||
            (be->state & ControlMask)) {
	  lastButtonPressTime = 0;
	  shade();
	} else
	  lastButtonPressTime = be->time;
      }

      frame.grab_x = be->x_root - frame.x - screen->getBorderWidth();
      frame.grab_y = be->y_root - frame.y - screen->getBorderWidth();

      if (windowmenu && windowmenu->isVisible()) windowmenu->hide();

      screen->getWorkspace(workspace_number)->raiseWindow(this);
    } else if (frame.iconify_button == be->window) {
      redrawIconifyButton(True);
    } else if (frame.close_button == be->window) {
      redrawCloseButton(True);
    } else if (frame.plate == be->window) {
      if ((! focused) && (! screen->isSloppyFocus()))
	setInputFocus();

      if (windowmenu && windowmenu->isVisible()) windowmenu->hide();

      screen->getWorkspace(workspace_number)->raiseWindow(this);

      XAllowEvents(display, ReplayPointer, be->time);
    }
  } else if (be->button == 2) {
    if (frame.title == be->window || frame.handle == be->window ||
        frame.right_grip == be->window || frame.left_grip == be->window ||
	frame.border == be->window || frame.label == be->window) {
      screen->getWorkspace(workspace_number)->lowerWindow(this);
    }
  } else if (windowmenu && be->button == 3 &&
	     (frame.title == be->window || frame.label == be->window ||
	      frame.border == be->window || frame.handle == be->window)) {
    int mx = 0, my = 0;

    if (frame.title == be->window || frame.label == be->window) {
      mx = be->x_root - (windowmenu->getWidth() / 2);
      my = frame.y + frame.title_h;
    } else if (frame.border == be->window) {
      mx = be->x_root - (windowmenu->getWidth() / 2);

      if (be->y <= (signed) frame.bevel_w)
	my = frame.y + frame.y_border;
      else
	my = be->y_root - (windowmenu->getHeight() / 2);
    } else if (frame.handle == be->window) {
      mx = be->x_root - (windowmenu->getWidth() / 2);
      my = frame.y + frame.y_handle - windowmenu->getHeight();
    }

    if (mx > (signed) (frame.x + frame.width - windowmenu->getWidth()))
      mx = frame.x + frame.width - windowmenu->getWidth();
    if (mx < frame.x)
      mx = frame.x;

    if (my > (signed) (frame.y + frame.y_handle - windowmenu->getHeight()))
      my = frame.y + frame.y_handle - windowmenu->getHeight();
    if (my < (signed) (frame.y + ((decorations.titlebar) ? frame.title_h :
			     	  frame.y_border)))
      my = frame.y +
	((decorations.titlebar) ? frame.title_h : frame.y_border);

    if (windowmenu)
      if (! windowmenu->isVisible()) {
	windowmenu->move(mx, my);
	windowmenu->show();
        XRaiseWindow(display, windowmenu->getWindowID());
        XRaiseWindow(display, windowmenu->getSendToMenu()->getWindowID());
      } else
	windowmenu->hide();
  }

  blackbox->ungrab();
}


void BlackboxWindow::buttonReleaseEvent(XButtonEvent *re) {
  blackbox->grab();
  if (! validateClient()) return;

  if (re->window == frame.maximize_button) {
    if ((re->x >= 0) && ((unsigned) re->x <= frame.button_w) &&
        (re->y >= 0) && ((unsigned) re->y <= frame.button_h)) {
      maximize(re->button);
    } else
      redrawMaximizeButton(maximized);
  } else {
    if (moving) {
      moving = False;

      blackbox->maskWindowEvents(0, (BlackboxWindow *) 0);

      if (! screen->doOpaqueMove()) {
        XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
	               frame.move_x, frame.move_y, frame.resize_w,
		       frame.resize_h);

	configure(frame.move_x, frame.move_y, frame.width, frame.height);
	blackbox->ungrab();
      } else
        configure(frame.x, frame.y, frame.width, frame.height);

      screen->hideGeometry();
      XUngrabPointer(display, CurrentTime);
    } else if (resizing) {
      XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
		     frame.resize_x, frame.resize_y,
		     frame.resize_w, frame.resize_h);

      screen->hideGeometry();

      if (re->window == frame.left_grip)
	left_fixsize();
      else
	right_fixsize();

      resizing = False;
      configure(frame.resize_x, frame.resize_y,
		frame.resize_w - screen->getBorderWidth2x(),
                frame.resize_h - screen->getBorderWidth2x());

      blackbox->ungrab();
      XUngrabPointer(display, CurrentTime);
    } else if (re->window == frame.iconify_button) {
      if ((re->x >= 0) && ((unsigned) re->x <= frame.button_w) &&
	  (re->y >= 0) && ((unsigned) re->y <= frame.button_h)) {
	iconify();
      }	else
	redrawIconifyButton(False);
    } else if (re->window == frame.close_button) {
      if ((re->x >= 0) && ((unsigned) re->x <= frame.button_w) &&
	  (re->y >= 0) && ((unsigned) re->y <= frame.button_h)) {
	close();
      } else
	redrawCloseButton(False);
    }
  }

  blackbox->ungrab();
}


void BlackboxWindow::motionNotifyEvent(XMotionEvent *me) {
  if ((me->state & Button1Mask) && functions.move &&
      (frame.title == me->window || frame.label == me->window ||
       frame.handle == me->window || frame.border == me->window ||
       frame.window == me->window)) {
    if (! moving) {
      XGrabPointer(display, me->window, False, Button1MotionMask |
                   ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
                   None, blackbox->getMoveCursor(), CurrentTime);

      if (windowmenu && windowmenu->isVisible())
        windowmenu->hide();

      moving = True;

      blackbox->maskWindowEvents(client.window, this);

      if (! screen->doOpaqueMove()) {
        blackbox->grab();

        frame.move_x = frame.x;
	frame.move_y = frame.y;
        frame.resize_w = frame.width + screen->getBorderWidth2x();
        frame.resize_h = ((shaded) ? frame.title_h : frame.height) +
          screen->getBorderWidth2x();

	screen->showPosition(frame.x, frame.y);

	XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
		       frame.move_x, frame.move_y,
		       frame.resize_w, frame.resize_h);
      }
    } else if (moving) {
      int dx = me->x_root - frame.grab_x, dy = me->y_root - frame.grab_y;

      dx -= screen->getBorderWidth();
      dy -= screen->getBorderWidth();

      if (screen->getEdgeSnapThreshold()) {
        int drx = screen->getWidth() - (dx + frame.snap_w);

        if (dx > 0 && dx < drx && dx < screen->getEdgeSnapThreshold()) dx = 0;
	else if (drx > 0 && drx < screen->getEdgeSnapThreshold())
	  dx = screen->getWidth() - frame.snap_w;

        int dtty, dbby, dty, dby;
        switch (screen->getToolbarPlacement()) {
        case Toolbar::TopLeft:
        case Toolbar::TopCenter:
        case Toolbar::TopRight:
          dtty = screen->getToolbar()->getHeight() + screen->getBorderWidth();
          dbby = screen->getHeight();
          break;

        default:
          dtty = 0;
          dbby = screen->getHeight() - screen->getToolbar()->getHeight() -
            screen->getBorderWidth();
          break;
        }

        dty = dy - dtty;
        dby = dbby - (dy + frame.snap_h);

	if (dy > 0 && dty < screen->getEdgeSnapThreshold()) dy = dtty;
        else if (dby > 0 && dby < screen->getEdgeSnapThreshold())
          dy = dbby - frame.snap_h;
      }

      if (! screen->doOpaqueMove()) {
	XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
		       frame.move_x, frame.move_y, frame.resize_w,
		       frame.resize_h);

	frame.move_x = dx;
	frame.move_y = dy;

	XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
		       frame.move_x, frame.move_y, frame.resize_w,
		       frame.resize_h);
      } else
	configure(dx, dy, frame.width, frame.height);

      screen->showPosition(dx, dy);
    }
  } else if (functions.resize &&
             (((me->state & Button1Mask) && (me->window == frame.right_grip ||
                                             me->window == frame.left_grip)) ||
              (me->window == frame.window && (me->state & Button2Mask)))) {
    Bool left = (me->window == frame.left_grip);

    if (! resizing) {
      XGrabPointer(display, me->window, False, ButtonMotionMask |
                   ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None,
                   ((left) ? blackbox->getLowerLeftAngleCursor() :
                             blackbox->getLowerRightAngleCursor()),
                   CurrentTime);

      resizing = True;

      blackbox->grab();

      int gx, gy;
      frame.grab_x = me->x - screen->getBorderWidth();
      frame.grab_y = me->y - screen->getBorderWidth2x();
      frame.resize_x = frame.x;
      frame.resize_y = frame.y;
      frame.resize_w = frame.width + screen->getBorderWidth2x();
      frame.resize_h = frame.height + screen->getBorderWidth2x();

      if (left)
        left_fixsize(&gx, &gy);
      else
	right_fixsize(&gx, &gy);

      screen->showGeometry(gx, gy);

      XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
		     frame.resize_x, frame.resize_y,
		     frame.resize_w, frame.resize_h);
    } else if (resizing) {
      XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
		     frame.resize_x, frame.resize_y,
		     frame.resize_w, frame.resize_h);

      int gx, gy;

      frame.resize_h = frame.height + (me->y - frame.grab_y);
      if (frame.resize_h < 1) frame.resize_h = 1;

      if (left) {
        frame.resize_x = me->x_root - frame.grab_x;
        if (frame.resize_x > (signed) (frame.x + frame.width))
          frame.resize_x = frame.resize_x + frame.width - 1;

        left_fixsize(&gx, &gy);
      } else {
	frame.resize_w = frame.width + (me->x - frame.grab_x);
	if (frame.resize_w < 1) frame.resize_w = 1;

	right_fixsize(&gx, &gy);
      }

      XDrawRectangle(display, screen->getRootWindow(), screen->getOpGC(),
		     frame.resize_x, frame.resize_y,
		     frame.resize_w, frame.resize_h);

      screen->showGeometry(gx, gy);
    }
  }
}


#ifdef    SHAPE
void BlackboxWindow::shapeEvent(XShapeEvent *) {
  if (blackbox->hasShapeExtensions()) {
    if (frame.shaped) {
      blackbox->grab();
      if (! validateClient()) return;
      XShapeCombineShape(display, frame.window, ShapeBounding,
			 (frame.bevel_w * decorations.border),
			 frame.y_border + (frame.bevel_w * decorations.border),
			 client.window, ShapeBounding, ShapeSet);

      int num = 1;
      XRectangle xrect[2];
      xrect[0].x = xrect[0].y = 0;
      xrect[0].width = frame.width;
      xrect[0].height = frame.y_border;

      if (decorations.handle) {
	xrect[1].x = 0;
	xrect[1].y = frame.y_handle;
	xrect[1].width = frame.width;
	xrect[1].height = frame.handle_h + screen->getBorderWidth();
	num++;
      }

      XShapeCombineRectangles(display, frame.window, ShapeBounding, 0, 0,
			      xrect, num, ShapeUnion, Unsorted);
      blackbox->ungrab();
    }
  }
}
#endif // SHAPE


Bool BlackboxWindow::validateClient(void) {
  XSync(display, False);

  XEvent e;
  if (XCheckTypedWindowEvent(display, client.window, DestroyNotify, &e) ||
      XCheckTypedWindowEvent(display, client.window, UnmapNotify, &e)) {
    XPutBackEvent(display, &e);
    blackbox->ungrab();

    return False;
  }

  return True;
}


void BlackboxWindow::restore(void) {
  XChangeSaveSet(display, client.window, SetModeDelete);
  XSelectInput(display, client.window, NoEventMask);

  restoreGravity();

  XUnmapWindow(display, frame.window);
  XUnmapWindow(display, client.window);

  XSetWindowBorderWidth(display, client.window, client.old_bw);
  XReparentWindow(display, client.window, screen->getRootWindow(),
                  client.x, client.y);
  XMapWindow(display, client.window);

  XFlush(display);
}


void BlackboxWindow::timeout(void) {
  screen->getWorkspace(workspace_number)->raiseWindow(this);
}


void BlackboxWindow::changeBlackboxHints(BlackboxHints *net) {
  if ((net->flags & AttribShaded) &&
      ((blackbox_attrib.attrib & AttribShaded) !=
       (net->attrib & AttribShaded)))
    shade();

  if ((net->flags & (AttribMaxVert | AttribMaxHoriz)) &&
      ((blackbox_attrib.attrib & (AttribMaxVert | AttribMaxHoriz)) !=
       (net->attrib & (AttribMaxVert | AttribMaxHoriz)))) {
    if (maximized) {
      maximize(0);
    } else {
      int m = 0;

      if ((net->flags & AttribMaxHoriz) && (net->flags & AttribMaxVert))
        m = ((net->attrib & (AttribMaxHoriz | AttribMaxVert)) ?  1 : 0);
      else if (net->flags & AttribMaxVert)
        m = ((net->attrib & AttribMaxVert) ? 2 : 0);
      else if (net->flags & AttribMaxHoriz)
        m = ((net->attrib & AttribMaxHoriz) ? 3 : 0);

      maximize(m);
    }
  }

  if ((net->flags & AttribOmnipresent) &&
      ((blackbox_attrib.attrib & AttribOmnipresent) !=
       (net->attrib & AttribOmnipresent)))
    stick();

  if ((net->flags & AttribWorkspace) &&
      (workspace_number != (signed) net->workspace)) {
    screen->reassociateWindow(this, net->workspace, True);

    if (screen->getCurrentWorkspaceID() != (signed) net->workspace) withdraw();
    else deiconify();
  }

  if (net->flags & AttribDecoration) {
    switch (net->decoration) {
    case DecorNone:
      decorations.titlebar = decorations.border = decorations.handle =
       decorations.iconify = decorations.maximize =
	  decorations.menu = False;
      functions.resize = functions.move = functions.iconify =
    functions.maximize = False;

      break;

    default:
    case DecorNormal:
      decorations.titlebar = decorations.border = decorations.handle =
       decorations.iconify = decorations.maximize =
	  decorations.menu = True;
      functions.resize = functions.move = functions.iconify =
    functions.maximize = True;

      break;

    case DecorTiny:
      decorations.titlebar = decorations.iconify = decorations.menu =
	    functions.move = functions.iconify = True;
      decorations.border = decorations.handle = decorations.maximize =
	functions.resize = functions.maximize = False;

      break;

    case DecorTool:
      decorations.titlebar = decorations.menu = functions.move = True;
      decorations.iconify = decorations.border = decorations.handle =
     decorations.maximize = functions.resize = functions.maximize =
	functions.iconify = False;

      break;
    }

    reconfigure();
  }
}


void BlackboxWindow::upsize(void) {
  // convert client.width/height into frame sizes

  frame.bevel_w = screen->getBevelWidth();

  if (i18n->multibyte())
    frame.title_h = (screen->getWindowStyle()->fontset_extents->
		     max_ink_extent.height +
		     (frame.bevel_w * 2) + 2) * decorations.titlebar;
  else
    frame.title_h = (screen->getWindowStyle()->font->ascent +
		     screen->getWindowStyle()->font->descent +
		     (frame.bevel_w * 2) + 2) * decorations.titlebar;
  
  frame.label_h =
    (frame.title_h - (frame.bevel_w * 2)) * decorations.titlebar;
  frame.button_w = frame.button_h = frame.label_h - 2;
  frame.y_border = (frame.title_h + screen->getBorderWidth()) *
		   decorations.titlebar;
  frame.border_w = (client.width + (frame.bevel_w * 2)) *
		   decorations.border;
  frame.border_h = (client.height + (frame.bevel_w * 2)) *
		   decorations.border;
  frame.grip_h = screen->getHandleWidth() * decorations.handle;
  frame.grip_w = (frame.button_h * 2) * decorations.handle;
  frame.handle_h = decorations.handle * screen->getHandleWidth();
  frame.y_handle =
    ((decorations.border) ? frame.border_h : client.height) +
    frame.y_border + (screen->getBorderWidth() * decorations.handle);
  frame.width =
    frame.title_w =
    frame.handle_w =
    ((decorations.border) ? frame.border_w : client.width);
  frame.height = frame.y_handle + frame.handle_h;

  frame.snap_w = frame.width + screen->getBorderWidth2x();
  frame.snap_h = frame.height + screen->getBorderWidth2x();
}


void BlackboxWindow::downsize(void) {
  // convert frame.width/height into client sizes

  frame.y_handle = frame.height - frame.handle_h;
  frame.border_w = frame.width;
  frame.border_h = frame.y_handle - frame.y_border -
    (screen->getBorderWidth() * decorations.handle);
  frame.title_w = frame.width;
  frame.handle_w = frame.width;

  client.x = frame.x + (decorations.border * frame.bevel_w) +
    screen->getBorderWidth();
  client.y = frame.y + frame.y_border + (decorations.border * frame.bevel_w) +
    screen->getBorderWidth();

  client.width =
    ((decorations.border) ? frame.border_w - (frame.bevel_w * 2) : frame.width);
  client.height = frame.height - frame.y_border -
    ((frame.bevel_w * 2) * decorations.border) -
    (screen->getBorderWidth() * decorations.handle) - frame.handle_h;

  frame.y_handle = frame.border_h + frame.y_border + screen->getBorderWidth();

  frame.snap_w = frame.width + screen->getBorderWidth2x();
  frame.snap_h = frame.height + screen->getBorderWidth2x();
}


void BlackboxWindow::right_fixsize(int *gx, int *gy) {
  // calculate the size of the client window and conform it to the
  // size specified by the size hints of the client window...
  int dx = frame.resize_w - client.base_width -
    ((frame.bevel_w * 2) * decorations.border) - screen->getBorderWidth2x();
  int dy = frame.resize_h - frame.y_border - client.base_height -
    frame.handle_h - (screen->getBorderWidth() * 3) -
    ((frame.bevel_w * 2) * decorations.border);

  if (dx < (signed) client.min_width) dx = client.min_width;
  if (dy < (signed) client.min_height) dy = client.min_height;
  if ((unsigned) dx > client.max_width) dx = client.max_width;
  if ((unsigned) dy > client.max_height) dy = client.max_height;

  dx /= client.width_inc;
  dy /= client.height_inc;

  if (gx) *gx = dx;
  if (gy) *gy = dy;

  dx = (dx * client.width_inc) + client.base_width;
  dy = (dy * client.height_inc) + client.base_height;

  frame.resize_w = dx + ((frame.bevel_w * 2) * decorations.border) +
    screen->getBorderWidth2x();
  frame.resize_h = dy + frame.y_border + frame.handle_h +
    ((frame.bevel_w * 2) * decorations.border) +
    (screen->getBorderWidth() * 3);
}


void BlackboxWindow::left_fixsize(int *gx, int *gy) {
  // calculate the size of the client window and conform it to the
  // size specified by the size hints of the client window...
  int dx = frame.x + frame.width - frame.resize_x - client.base_width -
    ((frame.bevel_w * 2) * decorations.border);
  int dy = frame.resize_h - frame.y_border - client.base_height -
    frame.handle_h - (screen->getBorderWidth() * 3) -
    ((frame.bevel_w * 2) * decorations.border);

  if (dx < (signed) client.min_width) dx = client.min_width;
  if (dy < (signed) client.min_height) dy = client.min_height;
  if ((unsigned) dx > client.max_width) dx = client.max_width;
  if ((unsigned) dy > client.max_height) dy = client.max_height;

  dx /= client.width_inc;
  dy /= client.height_inc;

  if (gx) *gx = dx;
  if (gy) *gy = dy;

  dx = (dx * client.width_inc) + client.base_width;
  dy = (dy * client.height_inc) + client.base_height;

  frame.resize_w = dx + ((frame.bevel_w * 2) * decorations.border) +
    screen->getBorderWidth2x();
  frame.resize_x = ((frame.x + frame.width) - frame.resize_w) +
    screen->getBorderWidth2x();
  frame.resize_h = dy + frame.y_border + frame.handle_h +
    ((frame.bevel_w * 2) * decorations.border) +
    (screen->getBorderWidth() * 3);
}
