// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Window.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh at debian.org>
// Copyright (c) 1997 - 2000, 2002 Brad Hughes <bhughes at trolltech.com>
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <X11/Xatom.h>
#include <X11/keysym.h>

#ifdef HAVE_STRING_H
#  include <string.h>
#endif // HAVE_STRING_H

#ifdef    DEBUG
#  ifdef    HAVE_STDIO_H
#    include <stdio.h>
#  endif // HAVE_STDIO_H
#endif // DEBUG
}

#include <cstdlib>

#include "i18n.hh"
#include "blackbox.hh"
#include "GCCache.hh"
#include "Iconmenu.hh"
#include "Image.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Util.hh"
#include "Window.hh"
#include "Windowmenu.hh"
#include "Workspace.hh"
#include "Slit.hh"


/*
 * Initializes the class with default values/the window's set initial values.
 */
BlackboxWindow::BlackboxWindow(Blackbox *b, Window w, BScreen *s) {
#ifdef    DEBUG
  fprintf(stderr,
          i18n(WindowSet, WindowCreating,
               "BlackboxWindow::BlackboxWindow(): creating 0x%lx\n"),
          w);
#endif // DEBUG

  // set timer to zero... it is initialized properly later, so we check
  // if timer is zero in the destructor, and assume that the window is not
  // fully constructed if timer is zero...
  timer = 0;
  client.window = w;
  blackbox = b;

  if (! validateClient()) {
    delete this;
    return;
  }

  // fetch client size and placement
  XWindowAttributes wattrib;
  if ((! XGetWindowAttributes(blackbox->getXDisplay(),
                              client.window, &wattrib)) ||
      (! wattrib.screen) || wattrib.override_redirect) {
#ifdef    DEBUG
    fprintf(stderr,
            i18n(WindowSet, WindowXGetWindowAttributesFail,
                 "BlackboxWindow::BlackboxWindow(): XGetWindowAttributes "
                 "failed\n"));
#endif // DEBUG

    delete this;
    return;
  }

  if (s) {
    screen = s;
  } else {
    screen = blackbox->searchScreen(RootWindowOfScreen(wattrib.screen));
    if (! screen) {
#ifdef    DEBUG
      fprintf(stderr,
              i18n(WindowSet, WindowCannotFindScreen,
                   "BlackboxWindow::BlackboxWindow(): can't find screen\n"
                   "\tfor root window 0x%lx\n"),
              RootWindowOfScreen(wattrib.screen));
#endif // DEBUG

      delete this;
      return;
    }
  }

  flags.moving = flags.resizing = flags.shaded = flags.visible =
    flags.iconic = flags.focused = flags.stuck = flags.modal =
    flags.send_focus_message = flags.shaped = False;
  flags.maximized = 0;

  blackbox_attrib.workspace = window_number = BSENTINEL;

  blackbox_attrib.flags = blackbox_attrib.attrib = blackbox_attrib.stack
    = blackbox_attrib.decoration = 0l;
  blackbox_attrib.premax_x = blackbox_attrib.premax_y = 0;
  blackbox_attrib.premax_w = blackbox_attrib.premax_h = 0;

  frame.border_w = 1;
  frame.window = frame.plate = frame.title = frame.handle = None;
  frame.close_button = frame.iconify_button = frame.maximize_button = None;
  frame.right_grip = frame.left_grip = None;

  frame.ulabel_pixel = frame.flabel_pixel = frame.utitle_pixel =
  frame.ftitle_pixel = frame.uhandle_pixel = frame.fhandle_pixel =
    frame.ubutton_pixel = frame.fbutton_pixel = frame.pbutton_pixel =
    frame.uborder_pixel = frame.fborder_pixel = frame.ugrip_pixel =
    frame.fgrip_pixel = 0;
  frame.utitle = frame.ftitle = frame.uhandle = frame.fhandle = None;
  frame.ulabel = frame.flabel = frame.ubutton = frame.fbutton = None;
  frame.pbutton = frame.ugrip = frame.fgrip = decorations;

  decorations = Decor_Titlebar | Decor_Border | Decor_Handle |
                Decor_Iconify | Decor_Maximize | Decor_Menu;
  functions = Func_Resize | Func_Move | Func_Iconify | Func_Maximize;

  client.wm_hint_flags = client.normal_hint_flags = 0;
  client.transient_for = None;
  client.transient = 0;

  // get the initial size and location of client window (relative to the
  // _root window_). This position is the reference point used with the
  // window's gravity to find the window's initial position.
  client.rect = Rect(wattrib.x, wattrib.y, wattrib.width, wattrib.height);
  client.old_bw = wattrib.border_width;

  windowmenu = 0;
  lastButtonPressTime = 0;

  timer = new BTimer(blackbox, this);
  timer->setTimeout(blackbox->getAutoRaiseDelay());

  XSetWindowAttributes attrib_set;
  attrib_set.event_mask = PropertyChangeMask | FocusChangeMask;
  attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask |
                                     ButtonMotionMask;
  XChangeWindowAttributes(blackbox->getXDisplay(), client.window,
                          CWEventMask|CWDontPropagate, &attrib_set);

  if (! getBlackboxHints())
    getMWMHints();

  // get size, aspect, minimum/maximum size and other hints set by the
  // client
  getWMProtocols();
  getWMHints();
  getWMNormalHints();

  if (client.initial_state == WithdrawnState) {
    screen->getSlit()->addClient(client.window);
    delete this;
    return;
  }

  frame.window = createToplevelWindow();
  frame.plate = createChildWindow(frame.window);
  associateClientWindow();

  blackbox->saveWindowSearch(frame.window, this);
  blackbox->saveWindowSearch(frame.plate, this);
  blackbox->saveWindowSearch(client.window, this);

  // determine if this is a transient window
  getTransientInfo();

  // adjust the window decorations based on transience and window sizes
  if (isTransient()) {
    decorations &= ~(Decor_Maximize | Decor_Handle);
    functions &= ~Func_Maximize;
  }

  if ((client.normal_hint_flags & PMinSize) &&
      (client.normal_hint_flags & PMaxSize) &&
       client.max_width <= client.min_width &&
      client.max_height <= client.min_height) {
    decorations &= ~(Decor_Maximize | Decor_Handle);
    functions &= ~(Func_Resize | Func_Maximize);
  }
  upsize();

  Bool place_window = True;
  if (blackbox->isStartup() || isTransient() ||
      client.normal_hint_flags & (PPosition|USPosition)) {
    setGravityOffsets();


    if (blackbox->isStartup() ||
        client.rect.intersects(Rect(screen->availableArea())))
      place_window = False;
  }

  if (decorations & Decor_Titlebar)
    createTitlebar();

  if (decorations & Decor_Handle)
    createHandle();

#ifdef    SHAPE
  if (blackbox->hasShapeExtensions() && flags.shaped) {
    configureShape();
  }
#endif // SHAPE

  if ((! screen->isSloppyFocus()) || screen->doClickRaise()) {
    // grab button 1 for changing focus/raising
    blackbox->grabButton(Button1, 0, frame.plate, True, ButtonPressMask,
                         GrabModeSync, GrabModeSync, frame.plate, None);
  }

  blackbox->grabButton(Button1, Mod1Mask, frame.window, True,
                       ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                       GrabModeAsync, frame.window, blackbox->getMoveCursor());
  blackbox->grabButton(Button2, Mod1Mask, frame.window, True,
                       ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
                       frame.window, None);
  blackbox->grabButton(Button3, Mod1Mask, frame.window, True,
                       ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                       GrabModeAsync, frame.window,
                       blackbox->getLowerRightAngleCursor());

  positionWindows();
  decorate();

  if (decorations & Decor_Titlebar)
    XMapSubwindows(blackbox->getXDisplay(), frame.title);
  XMapSubwindows(blackbox->getXDisplay(), frame.window);

  if (decorations & Decor_Menu)
    windowmenu = new Windowmenu(this);

  if (blackbox_attrib.workspace >= screen->getWorkspaceCount())
    screen->getCurrentWorkspace()->addWindow(this, place_window);
  else
    screen->getWorkspace(blackbox_attrib.workspace)->
      addWindow(this, place_window);

  configure(frame.rect.x(), frame.rect.y(),
            frame.rect.width(), frame.rect.height());

  if (flags.shaded) {
    flags.shaded = False;
    shade();
  }

  if (flags.maximized && (functions & Func_Maximize)) {
    remaximize();
  }

  setFocusFlag(False);
}


BlackboxWindow::~BlackboxWindow(void) {
  if (! timer) // window not managed...
    return;

  if (flags.moving || flags.resizing) {
    screen->hideGeometry();
    XUngrabPointer(blackbox->getXDisplay(), CurrentTime);
  }

  delete timer;

  delete windowmenu;

  if (client.window_group)
    blackbox->removeGroupSearch(client.window_group);

  if (isTransient()) {
    BlackboxWindow *bw = blackbox->searchWindow(client.transient_for);
    if (bw) {
      assert(client.transient == 0 || client.transient != bw);
      bw->client.transient = client.transient;
    }
  }

  if (frame.title)
    destroyTitlebar();

  if (frame.handle)
    destroyHandle();

  if (frame.plate) {
    blackbox->removeWindowSearch(frame.plate);
    XDestroyWindow(blackbox->getXDisplay(), frame.plate);
  }

  if (frame.window) {
    blackbox->removeWindowSearch(frame.window);
    XDestroyWindow(blackbox->getXDisplay(), frame.window);
  }

  blackbox->removeWindowSearch(client.window);
}


/*
 * Creates a new top level window, with a given location, size, and border
 * width.
 * Returns: the newly created window
 */
Window BlackboxWindow::createToplevelWindow() {
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap | CWBorderPixel | CWColormap |
                              CWOverrideRedirect | CWEventMask;

  attrib_create.background_pixmap = None;
  attrib_create.colormap = screen->getColormap();
  attrib_create.override_redirect = True;
  attrib_create.event_mask = ButtonPressMask | ButtonReleaseMask |
                             ButtonMotionMask | EnterWindowMask;

  return XCreateWindow(blackbox->getXDisplay(), screen->getRootWindow(),
                       frame.rect.x(), frame.rect.y(),
                       frame.rect.width(), frame.rect.height(),
                       frame.border_w, screen->getDepth(),
                       InputOutput, screen->getVisual(), create_mask,
                       &attrib_create);
}


/*
 * Creates a child window, and optionally associates a given cursor with
 * the new window.
 */
Window BlackboxWindow::createChildWindow(Window parent, Cursor cursor) {
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap | CWBorderPixel |
                              CWEventMask;

  attrib_create.background_pixmap = None;
  attrib_create.event_mask = ButtonPressMask | ButtonReleaseMask |
                             ButtonMotionMask | ExposureMask;

  if (cursor) {
    create_mask |= CWCursor;
    attrib_create.cursor = cursor;
  }

  return XCreateWindow(blackbox->getXDisplay(), parent, 0, 0, 1, 1, 0,
                       screen->getDepth(), InputOutput, screen->getVisual(),
                       create_mask, &attrib_create);
}


void BlackboxWindow::associateClientWindow(void) {
  XSetWindowBorderWidth(blackbox->getXDisplay(), client.window, 0);
  getWMName();
  getWMIconName();

  XChangeSaveSet(blackbox->getXDisplay(), client.window, SetModeInsert);

  XSelectInput(blackbox->getXDisplay(), frame.plate,
               SubstructureRedirectMask | SubstructureNotifyMask);
  XReparentWindow(blackbox->getXDisplay(), client.window, frame.plate, 0, 0);

#ifdef    SHAPE
  if (blackbox->hasShapeExtensions()) {
    XShapeSelectInput(blackbox->getXDisplay(), client.window, ShapeNotifyMask);

    int foo;
    unsigned int ufoo;

    XShapeQueryExtents(blackbox->getXDisplay(), client.window, &flags.shaped,
                       &foo, &foo, &ufoo, &ufoo, &foo, &foo, &foo,
                       &ufoo, &ufoo);
  }
#endif // SHAPE

  XRaiseWindow(blackbox->getXDisplay(), frame.plate);
  XMapSubwindows(blackbox->getXDisplay(), frame.plate);
}


void BlackboxWindow::decorate(void) {
  BTexture* texture;

  texture = &(screen->getWindowStyle()->b_focus);
  frame.fbutton = texture->render(frame.button_w, frame.button_h,
                                  frame.fbutton);
  if (! frame.fbutton)
    frame.fbutton_pixel = texture->color().pixel();

  texture = &(screen->getWindowStyle()->b_unfocus);
  frame.ubutton = texture->render(frame.button_w, frame.button_h,
                                  frame.ubutton);
  if (! frame.ubutton)
    frame.ubutton_pixel = texture->color().pixel();

  texture = &(screen->getWindowStyle()->b_pressed);
  frame.pbutton = texture->render(frame.button_w, frame.button_h,
                                  frame.pbutton);
  if (! frame.pbutton)
    frame.pbutton_pixel = texture->color().pixel();

  if (decorations & Decor_Titlebar) {
    texture = &(screen->getWindowStyle()->t_focus);
    frame.ftitle = texture->render(frame.rect.width(), frame.title_h,
                                   frame.ftitle);
    if (! frame.ftitle)
      frame.ftitle_pixel = texture->color().pixel();

    texture = &(screen->getWindowStyle()->t_unfocus);
    frame.utitle = texture->render(frame.rect.width(), frame.title_h,
                                   frame.utitle);
    if (! frame.utitle)
      frame.utitle_pixel = texture->color().pixel();

    XSetWindowBorder(blackbox->getXDisplay(), frame.title,
                     screen->getBorderColor()->pixel());

    decorateLabel();
  }

  if (decorations & Decor_Border) {
    frame.fborder_pixel = screen->getWindowStyle()->f_focus.pixel();
    frame.uborder_pixel = screen->getWindowStyle()->f_unfocus.pixel();
    blackbox_attrib.flags |= AttribDecoration;
    blackbox_attrib.decoration = DecorNormal;
  } else {
    blackbox_attrib.flags |= AttribDecoration;
    blackbox_attrib.decoration = DecorNone;
  }

  if (decorations & Decor_Handle) {
    texture = &(screen->getWindowStyle()->h_focus);
    frame.fhandle = texture->render(frame.rect.width(), frame.handle_h,
                                    frame.fhandle);
    if (! frame.fhandle)
      frame.fhandle_pixel = texture->color().pixel();

    texture = &(screen->getWindowStyle()->h_unfocus);
    frame.uhandle = texture->render(frame.rect.width(), frame.handle_h,
                                    frame.uhandle);
    if (! frame.uhandle)
      frame.uhandle_pixel = texture->color().pixel();

    texture = &(screen->getWindowStyle()->g_focus);
    frame.fgrip = texture->render(frame.grip_w, frame.grip_h, frame.fgrip);
    if (! frame.fgrip)
      frame.fgrip_pixel = texture->color().pixel();

    texture = &(screen->getWindowStyle()->g_unfocus);
    frame.ugrip = texture->render(frame.grip_w, frame.grip_h, frame.ugrip);
    if (! frame.ugrip)
      frame.ugrip_pixel = texture->color().pixel();

    XSetWindowBorder(blackbox->getXDisplay(), frame.handle,
                     screen->getBorderColor()->pixel());
    XSetWindowBorder(blackbox->getXDisplay(), frame.left_grip,
                     screen->getBorderColor()->pixel());
    XSetWindowBorder(blackbox->getXDisplay(), frame.right_grip,
                     screen->getBorderColor()->pixel());
  }

  XSetWindowBorder(blackbox->getXDisplay(), frame.window,
                   screen->getBorderColor()->pixel());
}


void BlackboxWindow::decorateLabel(void) {
  BTexture *texture;

  texture = &(screen->getWindowStyle()->l_focus);
  frame.flabel = texture->render(frame.label_w, frame.label_h, frame.flabel);
  if (! frame.flabel)
    frame.flabel_pixel = texture->color().pixel();

  texture = &(screen->getWindowStyle()->l_unfocus);
  frame.ulabel = texture->render(frame.label_w, frame.label_h, frame.ulabel);
  if (! frame.ulabel)
    frame.ulabel_pixel = texture->color().pixel();
}


void BlackboxWindow::createHandle(void) {
  frame.handle = createChildWindow(frame.window);
  blackbox->saveWindowSearch(frame.handle, this);

  frame.left_grip =
    createChildWindow(frame.handle, blackbox->getLowerLeftAngleCursor());
  blackbox->saveWindowSearch(frame.left_grip, this);

  frame.right_grip =
    createChildWindow(frame.handle, blackbox->getLowerRightAngleCursor());
  blackbox->saveWindowSearch(frame.right_grip, this);
}


void BlackboxWindow::destroyHandle(void) {
  if (frame.fhandle)
    screen->getImageControl()->removeImage(frame.fhandle);

  if (frame.uhandle)
    screen->getImageControl()->removeImage(frame.uhandle);

  if (frame.fgrip)
    screen->getImageControl()->removeImage(frame.fgrip);

  if (frame.ugrip)
    screen->getImageControl()->removeImage(frame.ugrip);

  blackbox->removeWindowSearch(frame.left_grip);
  blackbox->removeWindowSearch(frame.right_grip);

  XDestroyWindow(blackbox->getXDisplay(), frame.left_grip);
  XDestroyWindow(blackbox->getXDisplay(), frame.right_grip);
  frame.left_grip = frame.right_grip = None;

  blackbox->removeWindowSearch(frame.handle);
  XDestroyWindow(blackbox->getXDisplay(), frame.handle);
  frame.handle = None;
}


void BlackboxWindow::createTitlebar(void) {
  frame.title = createChildWindow(frame.window);
  frame.label = createChildWindow(frame.title);
  blackbox->saveWindowSearch(frame.title, this);
  blackbox->saveWindowSearch(frame.label, this);

  if (decorations & Decor_Iconify) createIconifyButton();
  if (decorations & Decor_Maximize) createMaximizeButton();
  if (decorations & Decor_Close) createCloseButton();
}


void BlackboxWindow::destroyTitlebar(void) {
  if (frame.close_button)
    destroyCloseButton();

  if (frame.iconify_button)
    destroyIconifyButton();

  if (frame.maximize_button)
    destroyMaximizeButton();

  if (frame.ftitle)
    screen->getImageControl()->removeImage(frame.ftitle);

  if (frame.utitle)
    screen->getImageControl()->removeImage(frame.utitle);

  if (frame.flabel)
    screen->getImageControl()->removeImage(frame.flabel);

  if( frame.ulabel)
    screen->getImageControl()->removeImage(frame.ulabel);

  if (frame.fbutton)
    screen->getImageControl()->removeImage(frame.fbutton);

  if (frame.ubutton)
    screen->getImageControl()->removeImage(frame.ubutton);

  if (frame.pbutton)
    screen->getImageControl()->removeImage(frame.pbutton);

  blackbox->removeWindowSearch(frame.title);
  blackbox->removeWindowSearch(frame.label);

  XDestroyWindow(blackbox->getXDisplay(), frame.label);
  XDestroyWindow(blackbox->getXDisplay(), frame.title);
  frame.title = frame.label = None;
}


void BlackboxWindow::createCloseButton(void) {
  if (frame.title != None) {
    frame.close_button = createChildWindow(frame.title);
    blackbox->saveWindowSearch(frame.close_button, this);
  }
}


void BlackboxWindow::destroyCloseButton(void) {
  blackbox->removeWindowSearch(frame.close_button);
  XDestroyWindow(blackbox->getXDisplay(), frame.close_button);
  frame.close_button = None;
}


void BlackboxWindow::createIconifyButton(void) {
  if (frame.title != None) {
    frame.iconify_button = createChildWindow(frame.title);
    blackbox->saveWindowSearch(frame.iconify_button, this);
  }
}


void BlackboxWindow::destroyIconifyButton(void) {
  blackbox->removeWindowSearch(frame.iconify_button);
  XDestroyWindow(blackbox->getXDisplay(), frame.iconify_button);
  frame.iconify_button = None;
}


void BlackboxWindow::createMaximizeButton(void) {
  if (frame.title != None) {
    frame.maximize_button = createChildWindow(frame.title);
    blackbox->saveWindowSearch(frame.maximize_button, this);
  }
}


void BlackboxWindow::destroyMaximizeButton(void) {
  blackbox->removeWindowSearch(frame.maximize_button);
  XDestroyWindow(blackbox->getXDisplay(), frame.maximize_button);
  frame.maximize_button = None;
}


void BlackboxWindow::positionButtons(Bool redecorate_label) {
  unsigned int bw = frame.button_w + frame.bevel_w + 1,
    by = frame.bevel_w + 1, lx = by, lw = frame.rect.width() - by;

  if ((decorations & Decor_Iconify) && frame.iconify_button != None) {
    XMoveResizeWindow(blackbox->getXDisplay(), frame.iconify_button, by, by,
                      frame.button_w, frame.button_h);
    XMapWindow(blackbox->getXDisplay(), frame.iconify_button);
    XClearWindow(blackbox->getXDisplay(), frame.iconify_button);

    lx += bw;
    lw -= bw;
  } else if (frame.iconify_button) {
    XUnmapWindow(blackbox->getXDisplay(), frame.iconify_button);
  }
  int bx = frame.rect.width() - bw;

  if ((decorations & Decor_Close) && frame.close_button != None) {
    XMoveResizeWindow(blackbox->getXDisplay(), frame.close_button, bx, by,
                      frame.button_w, frame.button_h);
    XMapWindow(blackbox->getXDisplay(), frame.close_button);
    XClearWindow(blackbox->getXDisplay(), frame.close_button);

    bx -= bw;
    lw -= bw;
  } else if (frame.close_button) {
    XUnmapWindow(blackbox->getXDisplay(), frame.close_button);
  }
  if ((decorations & Decor_Maximize) && frame.maximize_button != None) {
    XMoveResizeWindow(blackbox->getXDisplay(), frame.maximize_button, bx, by,
                      frame.button_w, frame.button_h);
    XMapWindow(blackbox->getXDisplay(), frame.maximize_button);
    XClearWindow(blackbox->getXDisplay(), frame.maximize_button);

    lw -= bw;
  } else if (frame.maximize_button) {
    XUnmapWindow(blackbox->getXDisplay(), frame.maximize_button);
  }
  frame.label_w = lw - by;
  XMoveResizeWindow(blackbox->getXDisplay(), frame.label, lx, frame.bevel_w,
                    frame.label_w, frame.label_h);
  if (redecorate_label) decorateLabel();

  redrawLabel();
  redrawAllButtons();
}


void BlackboxWindow::reconfigure(void) {
  upsize();

  client.rect.setPos(frame.rect.x() + frame.mwm_border_w + frame.border_w,
                     frame.rect.y() + frame.y_border + frame.mwm_border_w +
                     frame.border_w);

  if (! client.title.empty()) {
    if (i18n.multibyte()) {
      XRectangle ink, logical;
      XmbTextExtents(screen->getWindowStyle()->fontset,
                     client.title.c_str(), client.title.length(),
                     &ink, &logical);
      client.title_text_w = logical.width;
    } else {
      client.title_text_w = XTextWidth(screen->getWindowStyle()->font,
                                       client.title.c_str(),
                                       client.title.length());
    }
    client.title_text_w += (frame.bevel_w * 4);
  }

  positionWindows();
  decorate();

  XClearWindow(blackbox->getXDisplay(), frame.window);
  setFocusFlag(flags.focused);

  configure(frame.rect.x(), frame.rect.y(),
            frame.rect.width(), frame.rect.height());

  if (windowmenu) {
    windowmenu->move(windowmenu->getX(), frame.rect.y() + frame.title_h);
    windowmenu->reconfigure();
  }
}


void BlackboxWindow::updateFocusModel(void) {
  if ((! screen->isSloppyFocus()) || screen->doClickRaise()) {
    // grab button 1 for changing focus/raising
    blackbox->grabButton(Button1, 0, frame.plate, True, ButtonPressMask,
                         GrabModeSync, GrabModeSync, None, None);
  } else {
    blackbox->ungrabButton(Button1, 0, frame.plate);
  }
}


void BlackboxWindow::positionWindows(void) {
  XResizeWindow(blackbox->getXDisplay(), frame.window, frame.rect.width(),
                ((flags.shaded) ? frame.title_h : frame.rect.height()));
  XSetWindowBorderWidth(blackbox->getXDisplay(), frame.window, frame.border_w);
  XSetWindowBorderWidth(blackbox->getXDisplay(), frame.plate,
                        frame.mwm_border_w);
  XMoveResizeWindow(blackbox->getXDisplay(), frame.plate, 0, frame.y_border,
                    client.rect.width(), client.rect.height());
  XMoveResizeWindow(blackbox->getXDisplay(), client.window, 0, 0,
                    client.rect.width(), client.rect.height());

  if (decorations & Decor_Titlebar) {
    if (frame.title == None) createTitlebar();
    XSetWindowBorderWidth(blackbox->getXDisplay(), frame.title,
                          frame.border_w);
    XMoveResizeWindow(blackbox->getXDisplay(), frame.title, -frame.border_w,
                      -frame.border_w, frame.rect.width(), frame.title_h);

    positionButtons();
  } else if (frame.title) {
    destroyTitlebar();
  }
  if (decorations & Decor_Handle) {
    if (frame.handle == None) createHandle();
    XSetWindowBorderWidth(blackbox->getXDisplay(), frame.handle,
                          frame.border_w);
    XSetWindowBorderWidth(blackbox->getXDisplay(), frame.left_grip,
                          frame.border_w);
    XSetWindowBorderWidth(blackbox->getXDisplay(), frame.right_grip,
                          frame.border_w);

    XMoveResizeWindow(blackbox->getXDisplay(), frame.handle, -frame.border_w,
                      frame.y_handle - frame.border_w,
                      frame.rect.width(), frame.handle_h);
    XMoveResizeWindow(blackbox->getXDisplay(), frame.left_grip,
                      -frame.border_w, -frame.border_w,
                      frame.grip_w, frame.grip_h);
    XMoveResizeWindow(blackbox->getXDisplay(), frame.right_grip,
                      (frame.rect.width() - frame.grip_w - frame.border_w),
                      -frame.border_w, frame.grip_w, frame.grip_h);
    XMapSubwindows(blackbox->getXDisplay(), frame.handle);
  } else if (frame.handle) {
    destroyHandle();
  }
}


void BlackboxWindow::getWMName(void) {
  XTextProperty text_prop;
  char **list;
  int num;

  if (XGetWMName(blackbox->getXDisplay(), client.window, &text_prop)) {
    if (text_prop.value && text_prop.nitems > 0) {
      if (text_prop.encoding != XA_STRING) {
        text_prop.nitems = strlen((char *) text_prop.value);

        if ((XmbTextPropertyToTextList(blackbox->getXDisplay(), &text_prop,
                                       &list, &num) == Success) &&
            (num > 0) && *list) {
          client.title = *list;
          XFreeStringList(list);
        } else {
          client.title = (char *) text_prop.value;
        }
      } else {
        client.title = (char *) text_prop.value;
      }
      XFree((char *) text_prop.value);
    } else {
      client.title = i18n(WindowSet, WindowUnnamed, "Unnamed");
    }
  } else {
    client.title = i18n(WindowSet, WindowUnnamed, "Unnamed");
  }

  if (i18n.multibyte()) {
    XRectangle ink, logical;
    XmbTextExtents(screen->getWindowStyle()->fontset,
                   client.title.c_str(), client.title.length(),
                   &ink, &logical);
    client.title_text_w = logical.width;
  } else {
    client.title_text_w = XTextWidth(screen->getWindowStyle()->font,
                                     client.title.c_str(),
                                     client.title.length());
  }

  client.title_text_w += (frame.bevel_w * 4);
}


void BlackboxWindow::getWMIconName(void) {
  XTextProperty text_prop;
  char **list;
  int num;

  if (XGetWMIconName(blackbox->getXDisplay(), client.window, &text_prop)) {
    if (text_prop.value && text_prop.nitems > 0) {
      if (text_prop.encoding != XA_STRING) {
         text_prop.nitems = strlen((char *) text_prop.value);

         if ((XmbTextPropertyToTextList(blackbox->getXDisplay(), &text_prop,
                                        &list, &num) == Success) &&
             (num > 0) && *list) {
           client.icon_title = *list;
           XFreeStringList(list);
         } else {
           client.icon_title = (char *) text_prop.value;
        }
      } else {
         client.icon_title = (char *) text_prop.value;
      }
      XFree((char *) text_prop.value);
    } else {
      client.icon_title = client.title;
    }
  } else {
    client.icon_title = client.title;
  }
}


/*
 * Retrieve which WM Protocols are supported by the client window.
 * If the WM_DELETE_WINDOW protocol is supported, add the close button to the
 * window's decorations and allow the close behavior.
 * If the WM_TAKE_FOCUS protocol is supported, save a value that indicates
 * this.
 */
void BlackboxWindow::getWMProtocols(void) {
  Atom *proto;
  int num_return = 0;

  if (XGetWMProtocols(blackbox->getXDisplay(), client.window,
                      &proto, &num_return)) {
    for (int i = 0; i < num_return; ++i) {
      if (proto[i] == blackbox->getWMDeleteAtom()) {
        decorations |= Decor_Close;
        functions |= Func_Close;
      } else if (proto[i] == blackbox->getWMTakeFocusAtom())
        flags.send_focus_message = True;
      else if (proto[i] == blackbox->getBlackboxStructureMessagesAtom())
        screen->addNetizen(new Netizen(screen, client.window));
    }

    XFree(proto);
  }
}


/*
 * Gets the value of the WM_HINTS property.
 * If the property is not set, then use a set of default values.
 */
void BlackboxWindow::getWMHints(void) {
  XWMHints *wmhint = XGetWMHints(blackbox->getXDisplay(), client.window);
  if (! wmhint) {
    focus_mode = F_Passive;
    client.window_group = None;
    client.initial_state = NormalState;
    return;
  }
  client.wm_hint_flags = wmhint->flags;
  if (wmhint->flags & InputHint) {
    if (wmhint->input == True) {
      if (flags.send_focus_message)
        focus_mode = F_LocallyActive;
      else
        focus_mode = F_Passive;
    } else {
      if (flags.send_focus_message)
        focus_mode = F_GloballyActive;
      else
        focus_mode = F_NoInput;
    }
  } else {
    focus_mode = F_Passive;
  }

  if (wmhint->flags & StateHint)
    client.initial_state = wmhint->initial_state;
  else
    client.initial_state = NormalState;

  if (wmhint->flags & WindowGroupHint) {
    if (! client.window_group) {
      client.window_group = wmhint->window_group;
      blackbox->saveGroupSearch(client.window_group, this);
    }
  } else {
    client.window_group = None;
  }
  XFree(wmhint);
}


/*
 * Gets the value of the WM_NORMAL_HINTS property.
 * If the property is not set, then use a set of default values.
 */
void BlackboxWindow::getWMNormalHints(void) {
  long icccm_mask;
  XSizeHints sizehint;

  const XRectangle& screen_area = screen->availableArea();

  client.min_width = client.min_height =
    client.base_width = client.base_height =
    client.width_inc = client.height_inc = 1;
  client.max_width = screen_area.width;
  client.max_height = screen_area.height;
  client.min_aspect_x = client.min_aspect_y =
    client.max_aspect_x = client.max_aspect_y = 1;
  client.win_gravity = NorthWestGravity;

  if (! XGetWMNormalHints(blackbox->getXDisplay(), client.window,
                          &sizehint, &icccm_mask))
    return;

  client.normal_hint_flags = sizehint.flags;

  if (sizehint.flags & PMinSize) {
    client.min_width = sizehint.min_width;
    client.min_height = sizehint.min_height;
  }

  if (sizehint.flags & PMaxSize) {
    client.max_width = sizehint.max_width;
    client.max_height = sizehint.max_height;
  }

  if (sizehint.flags & PResizeInc) {
    client.width_inc = sizehint.width_inc;
    client.height_inc = sizehint.height_inc;
  }

  if (sizehint.flags & PAspect) {
    client.min_aspect_x = sizehint.min_aspect.x;
    client.min_aspect_y = sizehint.min_aspect.y;
    client.max_aspect_x = sizehint.max_aspect.x;
    client.max_aspect_y = sizehint.max_aspect.y;
  }

  if (sizehint.flags & PBaseSize) {
    client.base_width = sizehint.base_width;
    client.base_height = sizehint.base_height;
  }

  if (sizehint.flags & PWinGravity)
    client.win_gravity = sizehint.win_gravity;
}


/*
 * Gets the MWM hints for the class' contained window.
 * This is used while initializing the window to its first state, and not
 * thereafter.
 * Returns: true if the MWM hints are successfully retreived and applied;
 * false if they are not.
 */
void BlackboxWindow::getMWMHints(void) {
  int format;
  Atom atom_return;
  unsigned long num, len;
  MwmHints *mwm_hint = 0;

  int ret = XGetWindowProperty(blackbox->getXDisplay(), client.window,
                               blackbox->getMotifWMHintsAtom(), 0,
                               PropMwmHintsElements, False,
                               blackbox->getMotifWMHintsAtom(), &atom_return,
                               &format, &num, &len,
                               (unsigned char **) &mwm_hint);

  if (ret != Success || ! mwm_hint || num != PropMwmHintsElements)
    return;

  if (mwm_hint->flags & MwmHintsDecorations) {
    if (mwm_hint->decorations & MwmDecorAll) {
      decorations = Decor_Titlebar | Decor_Handle | Decor_Border |
                    Decor_Iconify | Decor_Maximize | Decor_Close |
                    Decor_Menu;
    } else {
      decorations = 0;

      if (mwm_hint->decorations & MwmDecorBorder)
        decorations |= Decor_Border;
      if (mwm_hint->decorations & MwmDecorHandle)
        decorations |= Decor_Handle;
      if (mwm_hint->decorations & MwmDecorTitle)
        decorations |= Decor_Titlebar;
      if (mwm_hint->decorations & MwmDecorMenu)
        decorations |= Decor_Menu;
      if (mwm_hint->decorations & MwmDecorIconify)
        decorations |= Decor_Iconify;
      if (mwm_hint->decorations & MwmDecorMaximize)
        decorations |= Decor_Maximize;
    }
  }

  if (mwm_hint->flags & MwmHintsFunctions) {
    if (mwm_hint->functions & MwmFuncAll) {
      functions = Func_Resize | Func_Move | Func_Iconify | Func_Maximize |
                  Func_Close;
    } else {
      functions = 0;

      if (mwm_hint->functions & MwmFuncResize)
        functions |= Func_Resize;
      if (mwm_hint->functions & MwmFuncMove)
        functions |= Func_Move;
      if (mwm_hint->functions & MwmFuncIconify)
        functions |= Func_Iconify;
      if (mwm_hint->functions & MwmFuncMaximize)
        functions |= Func_Maximize;
      if (mwm_hint->functions & MwmFuncClose)
        functions |= Func_Close;
    }
  }
  XFree(mwm_hint);
}


/*
 * Gets the blackbox hints from the class' contained window.
 * This is used while initializing the window to its first state, and not
 * thereafter.
 * Returns: true if the hints are successfully retreived and applied; false if
 * they are not.
 */
Bool BlackboxWindow::getBlackboxHints(void) {
  int format;
  Atom atom_return;
  unsigned long num, len;
  BlackboxHints *blackbox_hint = 0;

  int ret = XGetWindowProperty(blackbox->getXDisplay(), client.window,
                               blackbox->getBlackboxHintsAtom(), 0,
                               PropBlackboxHintsElements, False,
                               blackbox->getBlackboxHintsAtom(), &atom_return,
                               &format, &num, &len,
                               (unsigned char **) &blackbox_hint);
  if (ret != Success || ! blackbox_hint || num != PropBlackboxHintsElements)
    return False;

  if (blackbox_hint->flags & AttribShaded)
    flags.shaded = (blackbox_hint->attrib & AttribShaded);

  if ((blackbox_hint->flags & AttribMaxHoriz) &&
      (blackbox_hint->flags & AttribMaxVert))
    flags.maximized = (blackbox_hint->attrib &
                       (AttribMaxHoriz | AttribMaxVert)) ? 1 : 0;
  else if (blackbox_hint->flags & AttribMaxVert)
    flags.maximized = (blackbox_hint->attrib & AttribMaxVert) ? 2 : 0;
  else if (blackbox_hint->flags & AttribMaxHoriz)
    flags.maximized = (blackbox_hint->attrib & AttribMaxHoriz) ? 3 : 0;

  if (blackbox_hint->flags & AttribOmnipresent)
    flags.stuck = (blackbox_hint->attrib & AttribOmnipresent);

  if (blackbox_hint->flags & AttribWorkspace)
    blackbox_attrib.workspace = blackbox_hint->workspace;

  // if (blackbox_hint->flags & AttribStack)
  //   don't yet have always on top/bottom for blackbox yet... working
  //   on that

  if (blackbox_hint->flags & AttribDecoration) {
    switch (blackbox_hint->decoration) {
    case DecorNone:
      // clear all decorations except close
      decorations &= Decor_Close;
      // clear all functions except close
      functions &= Func_Close;

      break;

    case DecorTiny:
      decorations |= Decor_Titlebar | Decor_Iconify | Decor_Menu;
      decorations &= ~(Decor_Border | Decor_Handle | Decor_Maximize);
      functions |= Func_Move | Func_Iconify;
      functions &= ~(Func_Resize | Func_Maximize);

      break;

    case DecorTool:
      decorations |= Decor_Titlebar | Decor_Menu;
      decorations &= ~(Decor_Iconify | Decor_Border | Decor_Handle |
                       Decor_Menu);
      functions |= Func_Move;
      functions &= ~(Func_Resize | Func_Maximize | Func_Iconify);

      break;

    case DecorNormal:
    default:
      decorations |= Decor_Titlebar | Decor_Border | Decor_Handle |
                     Decor_Iconify | Decor_Maximize | Decor_Menu;
      functions |= Func_Resize | Func_Move | Func_Iconify | Func_Maximize;

      break;
    }

    reconfigure();
  }
  XFree(blackbox_hint);
  return True;
}


void BlackboxWindow::getTransientInfo(void) {
  if (!XGetTransientForHint(blackbox->getXDisplay(), client.window,
                            &(client.transient_for))) {
    client.transient_for = None;
    return;
  }

  if (client.transient_for == None || client.transient_for == client.window) {
    client.transient_for = screen->getRootWindow();
  } else {
    BlackboxWindow *tr;
    if ((tr = blackbox->searchWindow(client.transient_for))) {
      tr->client.transient = this;
      flags.stuck = tr->flags.stuck;
    } else if (client.transient_for == client.window_group) {
      if ((tr = blackbox->searchGroup(client.transient_for, this))) {
        tr->client.transient = this;
        flags.stuck = tr->flags.stuck;
      }
    }
  }

  if (client.transient == this) client.transient = 0;

  if (client.transient_for == screen->getRootWindow())
    flags.modal = True;
}


void BlackboxWindow::configure(int dx, int dy,
                               unsigned int dw, unsigned int dh) {
  Bool send_event = (frame.rect.x() != dx || frame.rect.y() != dy);

  if ((dw != frame.rect.width()) || (dh != frame.rect.height())) {
    if ((static_cast<signed>(frame.rect.width()) + dx) < 0) dx = 0;
    if ((static_cast<signed>(frame.rect.height()) + dy) < 0) dy = 0;

    frame.rect.setRect(dx, dy, dw, dh);
    downsize();

#ifdef    SHAPE
    if (blackbox->hasShapeExtensions() && flags.shaped) {
      configureShape();
    }
#endif // SHAPE

    XMoveWindow(blackbox->getXDisplay(), frame.window,
                frame.rect.x(), frame.rect.y());

    positionWindows();
    decorate();
    setFocusFlag(flags.focused);
    redrawAllButtons();
  } else {
    frame.rect.setPos(dx, dy);

    XMoveWindow(blackbox->getXDisplay(), frame.window,
                frame.rect.x(), frame.rect.y());

    if (! flags.moving) send_event = True;
  }

  if (send_event && ! flags.moving) {
    client.rect.setPos(dx + frame.mwm_border_w + frame.border_w,
                       dy + frame.y_border + frame.mwm_border_w +
                       frame.border_w);

    XEvent event;
    event.type = ConfigureNotify;

    event.xconfigure.display = blackbox->getXDisplay();
    event.xconfigure.event = client.window;
    event.xconfigure.window = client.window;
    event.xconfigure.x = client.rect.x();
    event.xconfigure.y = client.rect.y();
    event.xconfigure.width = client.rect.width();
    event.xconfigure.height = client.rect.height();
    event.xconfigure.border_width = client.old_bw;
    event.xconfigure.above = frame.window;
    event.xconfigure.override_redirect = False;

    XSendEvent(blackbox->getXDisplay(), client.window, True,
               NoEventMask, &event);

    screen->updateNetizenConfigNotify(&event);
  }
}


#ifdef SHAPE
void BlackboxWindow::configureShape(void) {
  XShapeCombineShape(blackbox->getXDisplay(), frame.window, ShapeBounding,
                     frame.mwm_border_w, frame.y_border + frame.mwm_border_w,
                     client.window, ShapeBounding, ShapeSet);

  int num = 1;
  XRectangle xrect[2];
  xrect[0].x = xrect[0].y = 0;
  xrect[0].width = frame.rect.width();
  xrect[0].height = frame.y_border;

  if (decorations & Decor_Handle) {
    xrect[1].x = 0;
    xrect[1].y = frame.y_handle;
    xrect[1].width = frame.rect.width();
    xrect[1].height = frame.handle_h + frame.border_w;
    num++;
  }

  XShapeCombineRectangles(blackbox->getXDisplay(), frame.window,
                          ShapeBounding, 0, 0, xrect, num,
                          ShapeUnion, Unsorted);
}
#endif // SHAPE


Bool BlackboxWindow::setInputFocus(void) {
  if (flags.focused) return True;

  if (((frame.rect.right())) < 0) {
    if (((frame.rect.y() + frame.y_border)) < 0)
      configure(frame.border_w, frame.border_w,
                frame.rect.width(), frame.rect.height());
    else if (frame.rect.y() > static_cast<signed>(screen->getHeight()))
      configure(frame.border_w, screen->getHeight() - frame.rect.height(),
                frame.rect.width(), frame.rect.height());
    else
      configure(frame.border_w, frame.rect.y() + frame.border_w,
                frame.rect.width(), frame.rect.height());
  } else if (frame.rect.x() > static_cast<signed>(screen->getWidth())) {
    if (((frame.rect.y() + frame.y_border)) < 0)
      configure(screen->getWidth() - frame.rect.width(), frame.border_w,
                frame.rect.width(), frame.rect.height());
    else if (frame.rect.y() > static_cast<signed>(screen->getHeight()))
      configure(screen->getWidth() - frame.rect.width(),
                screen->getHeight() - frame.rect.height(),
                frame.rect.width(), frame.rect.height());
    else
      configure(screen->getWidth() - frame.rect.width(),
                frame.rect.y() + frame.border_w,
                frame.rect.width(), frame.rect.height());
  }

  if (client.transient && flags.modal)
    return client.transient->setInputFocus();

  if (focus_mode == F_LocallyActive || focus_mode == F_Passive) {
    XSetInputFocus(blackbox->getXDisplay(), client.window,
                   RevertToPointerRoot, CurrentTime);
  } else {
    /* we could set the focus to none, since the window doesn't accept focus,
     * but we shouldn't set focus to nothing since this would surely make
     * someone angry
     */
      return False;
  }

  blackbox->setFocusedWindow(this);

  if (flags.send_focus_message) {
    XEvent ce;
    ce.xclient.type = ClientMessage;
    ce.xclient.message_type = blackbox->getWMProtocolsAtom();
    ce.xclient.display = blackbox->getXDisplay();
    ce.xclient.window = client.window;
    ce.xclient.format = 32;
    ce.xclient.data.l[0] = blackbox->getWMTakeFocusAtom();
    ce.xclient.data.l[1] = blackbox->getLastTime();
    ce.xclient.data.l[2] = 0l;
    ce.xclient.data.l[3] = 0l;
    ce.xclient.data.l[4] = 0l;
    XSendEvent(blackbox->getXDisplay(), client.window, False,
               NoEventMask, &ce);
  }

  if (screen->isSloppyFocus() && screen->doAutoRaise()) {
    timer->start();
  }

  return True;
}


void BlackboxWindow::iconify(void) {
  if (flags.iconic) return;

  if (windowmenu) windowmenu->hide();

  setState(IconicState);

  /*
   * we don't want this XUnmapWindow call to generate an UnmapNotify event, so
   * we need to clear the event mask on frame.plate for a split second.
   * HOWEVER, since X11 is asynchronous, the window could be destroyed in that
   * split second, leaving us with a ghost window... so, we need to do this
   * while the X server is grabbed
   */
  XGrabServer(blackbox->getXDisplay());
  XSelectInput(blackbox->getXDisplay(), frame.plate, NoEventMask);
  XUnmapWindow(blackbox->getXDisplay(), client.window);
  XSelectInput(blackbox->getXDisplay(), frame.plate,
               SubstructureRedirectMask | SubstructureNotifyMask);
  XUngrabServer(blackbox->getXDisplay());

  XUnmapWindow(blackbox->getXDisplay(), frame.window);
  flags.visible = False;
  flags.iconic = True;

  screen->getWorkspace(blackbox_attrib.workspace)->removeWindow(this);

  if (isTransient()) {
    BlackboxWindow *transientOwner =
      blackbox->searchWindow(client.transient_for);
    if (transientOwner && !transientOwner->flags.iconic)
      transientOwner->iconify();
  }
  screen->addIcon(this);

  if (client.transient && !client.transient->flags.iconic) {
    client.transient->iconify();
  }
}


void BlackboxWindow::show(void) {
  setState(NormalState);

  XMapWindow(blackbox->getXDisplay(), client.window);
  XMapSubwindows(blackbox->getXDisplay(), frame.window);
  XMapWindow(blackbox->getXDisplay(), frame.window);

  flags.visible = True;
  flags.iconic = False;
}


void BlackboxWindow::deiconify(Bool reassoc, Bool raise) {
  if (flags.iconic || reassoc)
    screen->reassociateWindow(this, BSENTINEL, False);
  else if (blackbox_attrib.workspace != screen->getCurrentWorkspace()->getID())
    return;

  show();

  if (reassoc && client.transient) client.transient->deiconify(True, False);

  if (raise)
    screen->getWorkspace(blackbox_attrib.workspace)->raiseWindow(this);
}


void BlackboxWindow::close(void) {
  XEvent ce;
  ce.xclient.type = ClientMessage;
  ce.xclient.message_type = blackbox->getWMProtocolsAtom();
  ce.xclient.display = blackbox->getXDisplay();
  ce.xclient.window = client.window;
  ce.xclient.format = 32;
  ce.xclient.data.l[0] = blackbox->getWMDeleteAtom();
  ce.xclient.data.l[1] = CurrentTime;
  ce.xclient.data.l[2] = 0l;
  ce.xclient.data.l[3] = 0l;
  ce.xclient.data.l[4] = 0l;
  XSendEvent(blackbox->getXDisplay(), client.window, False, NoEventMask, &ce);
}


void BlackboxWindow::withdraw(void) {
  setState(current_state);

  flags.visible = False;
  flags.iconic = False;

  XUnmapWindow(blackbox->getXDisplay(), frame.window);

  XGrabServer(blackbox->getXDisplay());
  XSelectInput(blackbox->getXDisplay(), frame.plate, NoEventMask);
  XUnmapWindow(blackbox->getXDisplay(), client.window);
  XSelectInput(blackbox->getXDisplay(), frame.plate,
               SubstructureRedirectMask | SubstructureNotifyMask);
  XUngrabServer(blackbox->getXDisplay());

  if (windowmenu) windowmenu->hide();
}


void BlackboxWindow::maximize(unsigned int button) {
  // handle case where menu is open then the max button is used instead
  if (windowmenu && windowmenu->isVisible()) windowmenu->hide();

  if (flags.maximized) {
    flags.maximized = 0;

    blackbox_attrib.flags &= ! (AttribMaxHoriz | AttribMaxVert);
    blackbox_attrib.attrib &= ! (AttribMaxHoriz | AttribMaxVert);

    // when a resize is begun, maximize(0) is called to clear any maximization
    // flags currently set.  Otherwise it still thinks it is maximized.
    // so we do not need to call configure() because resizing will handle it
    if (!flags.resizing)
      configure(blackbox_attrib.premax_x, blackbox_attrib.premax_y,
                blackbox_attrib.premax_w, blackbox_attrib.premax_h);

    blackbox_attrib.premax_x = blackbox_attrib.premax_y = 0;
    blackbox_attrib.premax_w = blackbox_attrib.premax_h = 0;

    redrawAllButtons();
    setState(current_state);
    return;
  }

  blackbox_attrib.premax_x = frame.rect.x();
  blackbox_attrib.premax_y = frame.rect.y();
  blackbox_attrib.premax_w = frame.rect.width();
  blackbox_attrib.premax_h = frame.rect.height();

  const XRectangle &screen_area = screen->availableArea();

  int dx = screen_area.x, dy = screen_area.y;
  unsigned int dw = screen_area.width, dh = screen_area.height;

  dw -= frame.border_w * 2;
  dw -= frame.mwm_border_w * 2;
  dw -= client.base_width;

  dh -= frame.border_w * 2;
  dh -= frame.mwm_border_w * 2;
  if (decorations & Decor_Handle) dh -= (frame.handle_h + frame.border_w);
  dh -= client.base_height;
  dh -= frame.y_border;

  if (dw < client.min_width) dw = client.min_width;
  if (dh < client.min_height) dh = client.min_height;
  if (dw > client.max_width) dw = client.max_width;
  if (dh > client.max_height) dh = client.max_height;

  dw -= (dw % client.width_inc);
  dw += client.base_width;
  dw += frame.mwm_border_w * 2;

  dh -= (dh % client.height_inc);
  dh += client.base_height;
  dh += frame.y_border;
  if (decorations & Decor_Handle) dh += (frame.handle_h + frame.border_w);
  dh += frame.mwm_border_w * 2;

  switch(button) {
  case 1:
    blackbox_attrib.flags |= AttribMaxHoriz | AttribMaxVert;
    blackbox_attrib.attrib |= AttribMaxHoriz | AttribMaxVert;
    break;

  case 2:
    blackbox_attrib.flags |= AttribMaxVert;
    blackbox_attrib.attrib |= AttribMaxVert;

    dw = frame.rect.width();
    dx = frame.rect.x();
    break;

  case 3:
    blackbox_attrib.flags |= AttribMaxHoriz;
    blackbox_attrib.attrib |= AttribMaxHoriz;

    dh = frame.rect.height();
    dy = frame.rect.y();
    break;
  }

  if (flags.shaded) {
    blackbox_attrib.flags ^= AttribShaded;
    blackbox_attrib.attrib ^= AttribShaded;
    flags.shaded = False;
  }

  flags.maximized = button;

  configure(dx, dy, dw, dh);
  screen->getWorkspace(blackbox_attrib.workspace)->raiseWindow(this);
  redrawAllButtons();
  setState(current_state);
}


// re-maximizes the window to take into account availableArea changes
void BlackboxWindow::remaximize(void) {
  // save the original dimensions because maximize will wipe them out
  int premax_x = blackbox_attrib.premax_x,
    premax_y = blackbox_attrib.premax_y,
    premax_w = blackbox_attrib.premax_w,
    premax_h = blackbox_attrib.premax_h;

  unsigned int button = flags.maximized;
  flags.maximized = 0; // trick maximize() into working
  maximize(button);

  // restore saved values
  blackbox_attrib.premax_x = premax_x;
  blackbox_attrib.premax_y = premax_y;
  blackbox_attrib.premax_w = premax_w;
  blackbox_attrib.premax_h = premax_h;
}


void BlackboxWindow::setWorkspace(unsigned int n) {
  blackbox_attrib.flags |= AttribWorkspace;
  blackbox_attrib.workspace = n;
}


void BlackboxWindow::shade(void) {
  if (! (decorations & Decor_Titlebar))
    return;

  if (flags.shaded) {
    XResizeWindow(blackbox->getXDisplay(), frame.window,
                  frame.rect.width(), frame.rect.height());
    flags.shaded = False;
    blackbox_attrib.flags ^= AttribShaded;
    blackbox_attrib.attrib ^= AttribShaded;

    setState(NormalState);
  } else {
    XResizeWindow(blackbox->getXDisplay(), frame.window,
                  frame.rect.width(), frame.title_h);
    flags.shaded = True;
    blackbox_attrib.flags |= AttribShaded;
    blackbox_attrib.attrib |= AttribShaded;

    setState(IconicState);
  }
}


void BlackboxWindow::stick(void) {
  if (flags.stuck) {
    blackbox_attrib.flags ^= AttribOmnipresent;
    blackbox_attrib.attrib ^= AttribOmnipresent;

    flags.stuck = False;

    if (! flags.iconic)
      screen->reassociateWindow(this, BSENTINEL, True);

    setState(current_state);
  } else {
    flags.stuck = True;

    blackbox_attrib.flags |= AttribOmnipresent;
    blackbox_attrib.attrib |= AttribOmnipresent;

    setState(current_state);
  }
}


void BlackboxWindow::setFocusFlag(Bool focus) {
  flags.focused = focus;

  if (decorations & Decor_Titlebar) {
    if (flags.focused) {
      if (frame.ftitle)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.title, frame.ftitle);
      else
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.title, frame.ftitle_pixel);
    } else {
      if (frame.utitle)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.title, frame.utitle);
      else
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.title, frame.utitle_pixel);
    }
    XClearWindow(blackbox->getXDisplay(), frame.title);

    redrawLabel();
    redrawAllButtons();
  }

  if (decorations & Decor_Handle) {
    if (flags.focused) {
      if (frame.fhandle)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.handle, frame.fhandle);
      else
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.handle, frame.fhandle_pixel);

      if (frame.fgrip) {
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.left_grip, frame.fgrip);
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.right_grip, frame.fgrip);
      } else {
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.left_grip, frame.fgrip_pixel);
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.right_grip, frame.fgrip_pixel);
      }
    } else {
      if (frame.uhandle)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.handle, frame.uhandle);
      else
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.handle, frame.uhandle_pixel);

      if (frame.ugrip) {
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.left_grip, frame.ugrip);
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.right_grip, frame.ugrip);
      } else {
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.left_grip, frame.ugrip_pixel);
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.right_grip, frame.ugrip_pixel);
      }
    }
    XClearWindow(blackbox->getXDisplay(), frame.handle);
    XClearWindow(blackbox->getXDisplay(), frame.left_grip);
    XClearWindow(blackbox->getXDisplay(), frame.right_grip);
  }

  if (decorations & Decor_Border) {
    if (flags.focused)
      XSetWindowBorder(blackbox->getXDisplay(),
                       frame.plate, frame.fborder_pixel);
    else
      XSetWindowBorder(blackbox->getXDisplay(),
                       frame.plate, frame.uborder_pixel);
  }

  if (! focus && screen->isSloppyFocus() && screen->doAutoRaise() &&
      timer->isTiming()) {
    timer->stop();
  }

  if (isFocused())
    blackbox->setFocusedWindow(this);
}


void BlackboxWindow::installColormap(Bool install) {
  int i = 0, ncmap = 0;
  Colormap *cmaps = XListInstalledColormaps(blackbox->getXDisplay(),
                                            client.window, &ncmap);
  XWindowAttributes wattrib;
  if (cmaps) {
    if (XGetWindowAttributes(blackbox->getXDisplay(),
                             client.window, &wattrib)) {
      if (install) {
        // install the window's colormap
        for (i = 0; i < ncmap; i++) {
          if (*(cmaps + i) == wattrib.colormap)
            // this window is using an installed color map... do not install
            install = False;
        }
        // otherwise, install the window's colormap
        if (install)
          XInstallColormap(blackbox->getXDisplay(), wattrib.colormap);
      } else {
        // uninstall the window's colormap
        for (i = 0; i < ncmap; i++) {
          if (*(cmaps + i) == wattrib.colormap)
            // we found the colormap to uninstall
            XUninstallColormap(blackbox->getXDisplay(), wattrib.colormap);
        }
      }
    }

    XFree(cmaps);
  }
}


void BlackboxWindow::setState(unsigned long new_state) {
  current_state = new_state;

  unsigned long state[2];
  state[0] = current_state;
  state[1] = None;
  XChangeProperty(blackbox->getXDisplay(), client.window,
                  blackbox->getWMStateAtom(), blackbox->getWMStateAtom(), 32,
                  PropModeReplace, (unsigned char *) state, 2);

  XChangeProperty(blackbox->getXDisplay(), client.window,
                  blackbox->getBlackboxAttributesAtom(),
                  blackbox->getBlackboxAttributesAtom(), 32, PropModeReplace,
                  (unsigned char *) &blackbox_attrib,
                  PropBlackboxAttributesElements);
}


Bool BlackboxWindow::getState(void) {
  current_state = 0;

  Atom atom_return;
  Bool ret = False;
  int foo;
  unsigned long *state, ulfoo, nitems;

  if ((XGetWindowProperty(blackbox->getXDisplay(), client.window,
                          blackbox->getWMStateAtom(),
                          0l, 2l, False, blackbox->getWMStateAtom(),
                          &atom_return, &foo, &nitems, &ulfoo,
                          (unsigned char **) &state) != Success) ||
      (! state)) {
    return False;
  }

  if (nitems >= 1) {
    current_state = static_cast<unsigned long>(state[0]);

    ret = True;
  }

  XFree((void *) state);

  return ret;
}


void BlackboxWindow::restoreAttributes(void) {
  if (! getState()) current_state = NormalState;

  Atom atom_return;
  int foo;
  unsigned long ulfoo, nitems;

  BlackboxAttributes *net;
  int ret = XGetWindowProperty(blackbox->getXDisplay(), client.window,
                               blackbox->getBlackboxAttributesAtom(), 0l,
                               PropBlackboxAttributesElements, False,
                               blackbox->getBlackboxAttributesAtom(),
                               &atom_return, &foo, &nitems, &ulfoo,
                               (unsigned char **) &net);
  if (ret != Success || !net || nitems != PropBlackboxAttributesElements)
    return;

  if (net->flags & AttribShaded &&
      net->attrib & AttribShaded) {
    int save_state =
      ((current_state == IconicState) ? NormalState : current_state);

    flags.shaded = False;
    shade();

    current_state = save_state;
  }

  if ((net->workspace != screen->getCurrentWorkspaceID()) &&
      (net->workspace < screen->getWorkspaceCount())) {
    screen->reassociateWindow(this, net->workspace, True);

    if (current_state == NormalState) current_state = WithdrawnState;
  } else if (current_state == WithdrawnState) {
    current_state = NormalState;
  }

  if (net->flags & AttribOmnipresent &&
      net->attrib & AttribOmnipresent) {
    flags.stuck = False;
    stick();

    current_state = NormalState;
  }

  if ((net->flags & AttribMaxHoriz) ||
      (net->flags & AttribMaxVert)) {
    int x = net->premax_x, y = net->premax_y;
    unsigned int w = net->premax_w, h = net->premax_h;
    flags.maximized = 0;

    unsigned int m = 0;
    if ((net->flags & AttribMaxHoriz) &&
        (net->flags & AttribMaxVert))
      m = (net->attrib & (AttribMaxHoriz | AttribMaxVert)) ? 1 : 0;
    else if (net->flags & AttribMaxVert)
      m = (net->attrib & AttribMaxVert) ? 2 : 0;
    else if (net->flags & AttribMaxHoriz)
      m = (net->attrib & AttribMaxHoriz) ? 3 : 0;

    if (m) maximize(m);

    blackbox_attrib.premax_x = x;
    blackbox_attrib.premax_y = y;
    blackbox_attrib.premax_w = w;
    blackbox_attrib.premax_h = h;
  }

  setState(current_state);

  XFree((void *) net);
}


/*
 * Positions the frame according the the client window position and window
 * gravity.
 */
void BlackboxWindow::setGravityOffsets(void) {
  // x coordinates for each gravity type
  const int x_west = client.rect.x();
  const int x_east = client.rect.x() + client.rect.width() -
                     frame.rect.width();
  const int x_center = client.rect.x() + client.rect.width() -
                       (frame.rect.width()/2);
  // y coordinates for each gravity type
  const int y_north = client.rect.y();
  const int y_south = client.rect.y() + client.rect.height() -
                      frame.rect.height();
  const int y_center = client.rect.y() + client.rect.height() -
                       (frame.rect.height()/2);

  switch (client.win_gravity) {
  default:
  case NorthWestGravity: frame.rect.setPos(x_west,   y_north);  break;
  case NorthGravity:     frame.rect.setPos(x_center, y_north);  break;
  case NorthEastGravity: frame.rect.setPos(x_east,   y_north);  break;
  case SouthWestGravity: frame.rect.setPos(x_west,   y_south);  break;
  case SouthGravity:     frame.rect.setPos(x_center, y_south);  break;
  case SouthEastGravity: frame.rect.setPos(x_east,   y_south);  break;
  case WestGravity:      frame.rect.setPos(x_west,   y_center); break;
  case CenterGravity:    frame.rect.setPos(x_center, y_center); break;
  case EastGravity:      frame.rect.setPos(x_east,   y_center); break;

  case ForgetGravity:
  case StaticGravity:
    frame.rect.setPos(client.rect.x() - frame.mwm_border_w + frame.border_w,
                      client.rect.y() - frame.y_border - frame.mwm_border_w -
                      frame.border_w);
    break;
  }
}


/*
 * The reverse of the setGravityOffsets function. Uses the frame window's
 * position to find the window's reference point.
 */
void BlackboxWindow::restoreGravity(void) {
  // x coordinates for each gravity type
  const int x_west = frame.rect.x();
  const int x_east = frame.rect.right() - client.rect.width() + 1;
  const int x_center = frame.rect.x() + (frame.rect.width()/2) -
                       client.rect.width();
  // y coordinates for each gravity type
  const int y_north = frame.rect.y();
  const int y_south = frame.rect.bottom() - client.rect.height() + 1;
  const int y_center = frame.rect.y() + (frame.rect.height()/2) -
                       client.rect.height();

  switch(client.win_gravity) {
  default:
  case NorthWestGravity: client.rect.setPos(x_west,   y_north);  break;
  case NorthGravity:     client.rect.setPos(x_center, y_north);  break;
  case NorthEastGravity: client.rect.setPos(x_east,   y_north);  break;
  case SouthWestGravity: client.rect.setPos(x_west,   y_south);  break;
  case SouthGravity:     client.rect.setPos(x_center, y_south);  break;
  case SouthEastGravity: client.rect.setPos(x_east,   y_south);  break;
  case WestGravity:      client.rect.setPos(x_west,   y_center); break;
  case CenterGravity:    client.rect.setPos(x_center, y_center); break;
  case EastGravity:      client.rect.setPos(x_east,   y_center); break;

  case ForgetGravity:
  case StaticGravity:
    client.rect.setPos(frame.rect.x() + frame.mwm_border_w + frame.border_w,
                       frame.rect.y() + frame.y_border + frame.mwm_border_w +
                       frame.border_w);
    break;
  }
}


void BlackboxWindow::redrawLabel(void) {
  int dx = frame.bevel_w * 2, dlen = client.title.length();
  unsigned int l = client.title_text_w;

  if (flags.focused) {
    if (frame.flabel)
      XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                 frame.label, frame.flabel);
    else
      XSetWindowBackground(blackbox->getXDisplay(),
                           frame.label, frame.flabel_pixel);
  } else {
    if (frame.ulabel)
      XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                 frame.label, frame.ulabel);
    else
      XSetWindowBackground(blackbox->getXDisplay(),
                           frame.label, frame.ulabel_pixel);
  }
  XClearWindow(blackbox->getXDisplay(), frame.label);

  if (client.title_text_w > frame.label_w) {
    for (; dlen >= 0; dlen--) {
      if (i18n.multibyte()) {
        XRectangle ink, logical;
        XmbTextExtents(screen->getWindowStyle()->fontset,
                       client.title.c_str(), dlen, &ink, &logical);
        l = logical.width;
      } else {
        l = XTextWidth(screen->getWindowStyle()->font,
                       client.title.c_str(), dlen);
      }
      l += (frame.bevel_w * 4);

      if (l < frame.label_w)
        break;
    }
  }

  switch (screen->getWindowStyle()->justify) {
  case BScreen::RightJustify:
    dx += frame.label_w - l;
    break;

  case BScreen::CenterJustify:
    dx += (frame.label_w - l) / 2;
    break;
  }

  WindowStyle *style = screen->getWindowStyle();
  BPen pen((flags.focused) ? style->l_text_focus : style->l_text_unfocus,
           style->font);
  if (i18n.multibyte())
    XmbDrawString(blackbox->getXDisplay(), frame.label, style->fontset,
                  pen.gc(), dx, (1 - style->fontset_extents->max_ink_extent.y),
                  client.title.c_str(), dlen);
  else
    XDrawString(blackbox->getXDisplay(), frame.label, pen.gc(), dx,
                (style->font->ascent + 1), client.title.c_str(), dlen);
}


void BlackboxWindow::redrawAllButtons(void) {
  if (frame.iconify_button) redrawIconifyButton(False);
  if (frame.maximize_button) redrawMaximizeButton(flags.maximized);
  if (frame.close_button) redrawCloseButton(False);
}


void BlackboxWindow::redrawIconifyButton(Bool pressed) {
  if (! pressed) {
    if (flags.focused) {
      if (frame.fbutton)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.iconify_button, frame.fbutton);
      else
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.iconify_button, frame.fbutton_pixel);
    } else {
      if (frame.ubutton)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.iconify_button, frame.ubutton);
      else
        XSetWindowBackground(blackbox->getXDisplay(), frame.iconify_button,
                             frame.ubutton_pixel);
    }
  } else {
    if (frame.pbutton)
      XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                 frame.iconify_button, frame.pbutton);
    else
      XSetWindowBackground(blackbox->getXDisplay(),
                           frame.iconify_button, frame.pbutton_pixel);
  }
  XClearWindow(blackbox->getXDisplay(), frame.iconify_button);

  BPen pen((flags.focused) ? screen->getWindowStyle()->b_pic_focus :
           screen->getWindowStyle()->b_pic_unfocus);
  XDrawRectangle(blackbox->getXDisplay(), frame.iconify_button, pen.gc(),
                 2, (frame.button_h - 5), (frame.button_w - 5), 2);
}


void BlackboxWindow::redrawMaximizeButton(Bool pressed) {
  if (! pressed) {
    if (flags.focused) {
      if (frame.fbutton)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.maximize_button, frame.fbutton);
      else
        XSetWindowBackground(blackbox->getXDisplay(), frame.maximize_button,
                             frame.fbutton_pixel);
    } else {
      if (frame.ubutton)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.maximize_button, frame.ubutton);
      else
        XSetWindowBackground(blackbox->getXDisplay(), frame.maximize_button,
                             frame.ubutton_pixel);
    }
  } else {
    if (frame.pbutton)
      XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                 frame.maximize_button, frame.pbutton);
    else
      XSetWindowBackground(blackbox->getXDisplay(), frame.maximize_button,
                           frame.pbutton_pixel);
  }
  XClearWindow(blackbox->getXDisplay(), frame.maximize_button);

  BPen pen((flags.focused) ? screen->getWindowStyle()->b_pic_focus :
           screen->getWindowStyle()->b_pic_unfocus);
  XDrawRectangle(blackbox->getXDisplay(), frame.maximize_button, pen.gc(),
                 2, 2, (frame.button_w - 5), (frame.button_h - 5));
  XDrawLine(blackbox->getXDisplay(), frame.maximize_button, pen.gc(),
            2, 3, (frame.button_w - 3), 3);
}


void BlackboxWindow::redrawCloseButton(Bool pressed) {
  if (! pressed) {
    if (flags.focused) {
      if (frame.fbutton)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(), frame.close_button,
                                   frame.fbutton);
      else
        XSetWindowBackground(blackbox->getXDisplay(), frame.close_button,
                             frame.fbutton_pixel);
    } else {
      if (frame.ubutton)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(), frame.close_button,
                                   frame.ubutton);
      else
        XSetWindowBackground(blackbox->getXDisplay(), frame.close_button,
                             frame.ubutton_pixel);
    }
  } else {
    if (frame.pbutton)
      XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                 frame.close_button, frame.pbutton);
    else
      XSetWindowBackground(blackbox->getXDisplay(),
                           frame.close_button, frame.pbutton_pixel);
  }
  XClearWindow(blackbox->getXDisplay(), frame.close_button);

  BPen pen((flags.focused) ? screen->getWindowStyle()->b_pic_focus :
           screen->getWindowStyle()->b_pic_unfocus);
  XDrawLine(blackbox->getXDisplay(), frame.close_button, pen.gc(),
            2, 2, (frame.button_w - 3), (frame.button_h - 3));
  XDrawLine(blackbox->getXDisplay(), frame.close_button, pen.gc(),
            2, (frame.button_h - 3), (frame.button_w - 3), 2);
}


void BlackboxWindow::mapRequestEvent(XMapRequestEvent *re) {
  if (re->window != client.window)
    return;
#ifdef    DEBUG
  fprintf(stderr, i18n(WindowSet, WindowMapRequest,
                       "BlackboxWindow::mapRequestEvent() for 0x%lx\n"),
          client.window);
#endif // DEBUG

  Bool get_state_ret = getState();
  if (! (get_state_ret && blackbox->isStartup())) {
    if ((client.wm_hint_flags & StateHint) &&
        (! (current_state == NormalState || current_state == IconicState)))
      current_state = client.initial_state;
    else
      current_state = NormalState;
  } else if (flags.iconic) {
    current_state = NormalState;
  }

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
    if (! blackbox->isStartup() && (isTransient() || screen->doFocusNew())) {
      XSync(blackbox->getXDisplay(), False); // make sure the frame is mapped..
      setInputFocus();
    }
    break;
  }
}


void BlackboxWindow::unmapNotifyEvent(XUnmapEvent *ue) {
  if (ue->window != client.window)
    return;

#ifdef    DEBUG
  fprintf(stderr, i18n(WindowSet, WindowUnmapNotify,
                       "BlackboxWindow::unmapNotifyEvent() for 0x%lx\n"),
          client.window);
#endif // DEBUG

  screen->unmanageWindow(this, False);
}


void BlackboxWindow::destroyNotifyEvent(XDestroyWindowEvent *de) {
  if (de->window != client.window)
    return;

  screen->unmanageWindow(this, False);
}


void BlackboxWindow::reparentNotifyEvent(XReparentEvent *re) {
  if (re->window != client.window || re->parent == frame.plate)
    return;

#ifdef    DEBUG
  fprintf(stderr,
          i18n(WindowSet, WindowReparentNotify,
                "BlackboxWindow::reparentNotifyEvent(): reparent 0x%lx to "
                "0x%lx.\n"), client.window, re->parent);
#endif // DEBUG

  XEvent ev;
  ev.xreparent = *re;
  XPutBackEvent(blackbox->getXDisplay(), &ev);
  screen->unmanageWindow(this, True);
}


void BlackboxWindow::propertyNotifyEvent(Atom atom) {
  switch(atom) {
  case XA_WM_CLASS:
  case XA_WM_CLIENT_MACHINE:
  case XA_WM_COMMAND:
    break;

  case XA_WM_TRANSIENT_FOR: {
    // determine if this is a transient window
    getTransientInfo();

    // adjust the window decorations based on transience
    if (isTransient()) {
      decorations &= ~(Decor_Maximize | Decor_Handle);
      functions &= ~Func_Maximize;
    }

    reconfigure();
  }
    break;

  case XA_WM_HINTS:
    getWMHints();
    break;

  case XA_WM_ICON_NAME:
    getWMIconName();
    if (flags.iconic) screen->propagateWindowName(this);
    break;

  case XA_WM_NAME:
    getWMName();

    if (decorations & Decor_Titlebar)
      redrawLabel();

    screen->propagateWindowName(this);
    break;

  case XA_WM_NORMAL_HINTS: {
    getWMNormalHints();

    if ((client.normal_hint_flags & PMinSize) &&
        (client.normal_hint_flags & PMaxSize)) {
      if (client.max_width <= client.min_width &&
          client.max_height <= client.min_height) {
        decorations &= ~(Decor_Maximize | Decor_Handle);
        functions &= ~(Func_Resize | Func_Maximize);
      } else {
        decorations |= Decor_Maximize | Decor_Handle;
        functions |= Func_Resize | Func_Maximize;
      }
    }

    int x = frame.rect.x(), y = frame.rect.y();
    unsigned int w = frame.rect.width(), h = frame.rect.height();

    upsize();

    if ((x != frame.rect.x()) || (y != frame.rect.y()) ||
        (w != frame.rect.width()) || (h != frame.rect.height()))
      reconfigure();

    break;
  }

  default:
    if (atom == blackbox->getWMProtocolsAtom()) {
      getWMProtocols();

      if ((decorations & Decor_Close) && (! frame.close_button)) {
        createCloseButton();
        if (decorations & Decor_Titlebar) positionButtons(True);
        if (windowmenu) windowmenu->reconfigure();
      }
    }

    break;
  }
}


void BlackboxWindow::exposeEvent(XExposeEvent *ee) {
  if (frame.label == ee->window && (decorations & Decor_Titlebar))
    redrawLabel();
  else if (frame.close_button == ee->window)
    redrawCloseButton(False);
  else if (frame.maximize_button == ee->window)
    redrawMaximizeButton(flags.maximized);
  else if (frame.iconify_button == ee->window)
    redrawIconifyButton(False);
}


void BlackboxWindow::configureRequestEvent(XConfigureRequestEvent *cr) {
  if (cr->window != client.window || flags.iconic)
    return;

  int cx = frame.rect.x(), cy = frame.rect.y();
  unsigned int cw = frame.rect.width(), ch = frame.rect.height();

  if (cr->value_mask & CWBorderWidth)
    client.old_bw = cr->border_width;

  if (cr->value_mask & CWX)
    cx = cr->x - frame.mwm_border_w - frame.border_w;

  if (cr->value_mask & CWY)
    cy = cr->y - frame.y_border - frame.mwm_border_w - frame.border_w;

  if (cr->value_mask & CWWidth)
    cw = cr->width + (frame.mwm_border_w * 2);

  if (cr->value_mask & CWHeight) {
    ch = cr->height + frame.y_border + (frame.mwm_border_w * 2) +
         frame.handle_h;
    if (decorations & Decor_Handle) ch += frame.border_w;
  }

  if (frame.rect.x() != cx || frame.rect.y() != cy ||
      frame.rect.width() != cw || frame.rect.height() != ch)
    configure(cx, cy, cw, ch);

  if (cr->value_mask & CWStackMode) {
    switch (cr->detail) {
    case Below:
    case BottomIf:
      screen->getWorkspace(blackbox_attrib.workspace)->lowerWindow(this);
      break;

    case Above:
    case TopIf:
    default:
      screen->getWorkspace(blackbox_attrib.workspace)->raiseWindow(this);
      break;
    }
  }
}


void BlackboxWindow::buttonPressEvent(XButtonEvent *be) {
  if (frame.maximize_button == be->window) {
    redrawMaximizeButton(True);
  } else if (be->button == 1 || (be->button == 3 && be->state == Mod1Mask)) {
    if (! flags.focused)
      setInputFocus();

    if (frame.iconify_button == be->window) {
      redrawIconifyButton(True);
    } else if (frame.close_button == be->window) {
      redrawCloseButton(True);
    } else if (frame.plate == be->window) {
      if (! flags.focused)
        setInputFocus();

      if (windowmenu && windowmenu->isVisible()) windowmenu->hide();

      screen->getWorkspace(blackbox_attrib.workspace)->raiseWindow(this);

      XAllowEvents(blackbox->getXDisplay(), ReplayPointer, be->time);
    } else {
      if (frame.title == be->window || frame.label == be->window) {
        if (((be->time - lastButtonPressTime) <=
             blackbox->getDoubleClickInterval()) ||
            (be->state & ControlMask)) {
          lastButtonPressTime = 0;
          shade();
        } else {
          lastButtonPressTime = be->time;
        }
      }

      frame.grab_x = be->x_root - frame.rect.x() - frame.border_w;
      frame.grab_y = be->y_root - frame.rect.y() - frame.border_w;

      if (windowmenu && windowmenu->isVisible()) windowmenu->hide();

      screen->getWorkspace(blackbox_attrib.workspace)->raiseWindow(this);
    }
  } else if (be->button == 2 && (be->window != frame.iconify_button) &&
                                (be->window != frame.close_button)) {
    screen->getWorkspace(blackbox_attrib.workspace)->lowerWindow(this);
  } else if (windowmenu && be->button == 3 &&
             (frame.title == be->window || frame.label == be->window ||
              frame.handle == be->window || frame.window == be->window)) {
    int mx = 0, my = 0;

    if (frame.title == be->window || frame.label == be->window) {
      mx = be->x_root - (windowmenu->getWidth() / 2);
      my = frame.rect.y() + frame.title_h;
    } else if (frame.handle == be->window) {
      mx = be->x_root - (windowmenu->getWidth() / 2);
      my = frame.rect.y() + frame.y_handle - windowmenu->getHeight();
    } else {
      mx = be->x_root - (windowmenu->getWidth() / 2);

      if (be->y <= static_cast<signed>(frame.bevel_w))
        my = frame.rect.y() + frame.y_border;
      else
        my = be->y_root - (windowmenu->getHeight() / 2);
    }

    if (mx > (frame.rect.right() - static_cast<signed>(windowmenu->getWidth())))
      mx = frame.rect.x() + frame.rect.width() - windowmenu->getWidth();
    if (mx < frame.rect.x())
      mx = frame.rect.x();

    if (my > (frame.rect.y() + frame.y_handle -
              static_cast<signed>(windowmenu->getHeight())))
      my = frame.rect.y() + frame.y_handle - windowmenu->getHeight();
    if (my < static_cast<signed>((frame.rect.y() +
                                  ((decorations & Decor_Titlebar) ?
                                   frame.title_h : frame.y_border))))
      my = frame.rect.y() +
           ((decorations & Decor_Titlebar) ? frame.title_h : frame.y_border);

    if (windowmenu) {
      if (! windowmenu->isVisible()) {
        windowmenu->move(mx, my);
        windowmenu->show();
        XRaiseWindow(blackbox->getXDisplay(), windowmenu->getWindowID());
        XRaiseWindow(blackbox->getXDisplay(),
                     windowmenu->getSendToMenu()->getWindowID());
      } else {
        windowmenu->hide();
      }
    }
  }
}


void BlackboxWindow::buttonReleaseEvent(XButtonEvent *re) {
  if (re->window == frame.maximize_button) {
    if ((re->x >= 0 && re->x <= static_cast<signed>(frame.button_w)) &&
        (re->y >= 0 && re->y <= static_cast<signed>(frame.button_h))) {
      maximize(re->button);
    } else {
      redrawMaximizeButton(flags.maximized);
    }
  } else if (re->window == frame.iconify_button) {
    if ((re->x >= 0 && re->x <= static_cast<signed>(frame.button_w)) &&
        (re->y >= 0 && re->y <= static_cast<signed>(frame.button_h))) {
      iconify();
    } else {
      redrawIconifyButton(False);
    }
  } else if (re->window == frame.close_button) {
    if ((re->x >= 0 && re->x <= static_cast<signed>(frame.button_w)) &&
        (re->y >= 0 && re->y <= static_cast<signed>(frame.button_h)))
      close();
    redrawCloseButton(False);
  } else if (flags.moving) {
    flags.moving = False;

    if (! screen->doOpaqueMove()) {
      /* when drawing the rubber band, we need to make sure we only draw inside
       * the frame... frame.changing_* contain the new coords for the window,
       * so we need to subtract 1 from changing_w/changing_h every where we
       * draw the rubber band (for both moving and resizing)
       */
      XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                     screen->getOpGC(), frame.changing_x, frame.changing_y,
                     frame.changing_w - 1, frame.changing_h - 1);
      XUngrabServer(blackbox->getXDisplay());

      configure(frame.changing_x, frame.changing_y,
                frame.rect.width(), frame.rect.height());
    } else {
      configure(frame.rect.x(), frame.rect.y(),
                frame.rect.width(), frame.rect.height());
    }
    screen->hideGeometry();
    XUngrabPointer(blackbox->getXDisplay(), CurrentTime);
  } else if (flags.resizing) {
    XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                   screen->getOpGC(), frame.changing_x, frame.changing_y,
                   frame.changing_w - 1, frame.changing_h - 1);
    XUngrabServer(blackbox->getXDisplay());

    screen->hideGeometry();

    if (re->window == frame.left_grip)
      left_fixsize();
    else
      right_fixsize();

    // unset maximized state when resized after fully maximized
    if (flags.maximized == 1)
      maximize(0);
    flags.resizing = False;
    configure(frame.changing_x, frame.changing_y,
              frame.changing_w - (frame.border_w * 2),
              frame.changing_h - (frame.border_w * 2));

    XUngrabPointer(blackbox->getXDisplay(), CurrentTime);
  } else if (re->window == frame.window) {
    if (re->button == 2 && re->state == Mod1Mask)
      XUngrabPointer(blackbox->getXDisplay(), CurrentTime);
  }
}


void BlackboxWindow::motionNotifyEvent(XMotionEvent *me) {
  if (!flags.resizing && (me->state & Button1Mask) &&
      (functions & Func_Move) &&
      (frame.title == me->window || frame.label == me->window ||
       frame.handle == me->window || frame.window == me->window)) {
    if (! flags.moving) {
      XGrabPointer(blackbox->getXDisplay(), me->window, False,
                   Button1MotionMask | ButtonReleaseMask,
                   GrabModeAsync, GrabModeAsync,
                   None, blackbox->getMoveCursor(), CurrentTime);

      if (windowmenu && windowmenu->isVisible())
        windowmenu->hide();

      flags.moving = True;

      if (! screen->doOpaqueMove()) {
        XGrabServer(blackbox->getXDisplay());

        frame.changing_x = frame.rect.x();
        frame.changing_y = frame.rect.y();
        frame.changing_w = frame.rect.width() + (frame.border_w * 2);
        frame.changing_h = ((flags.shaded) ?
                            frame.title_h : frame.rect.height()) +
                           (frame.border_w * 2);

        screen->showPosition(frame.rect.x(), frame.rect.y());

        XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                       screen->getOpGC(), frame.changing_x, frame.changing_y,
                       frame.changing_w - 1, frame.changing_h - 1);
      }
    } else {
      int dx = me->x_root - frame.grab_x, dy = me->y_root - frame.grab_y;
      unsigned int snap_w = frame.rect.width() + (frame.border_w * 2),
                   snap_h = ((flags.shaded) ?
                             frame.title_h : frame.rect.height()) +
                            (frame.border_w * 2);
      dx -= frame.border_w;
      dy -= frame.border_w;

      const int snap_distance = screen->getEdgeSnapThreshold();

      if (snap_distance) {
        const XRectangle &srect = screen->availableArea();
        // screen corners
        const int sleft = srect.x,
                 sright = srect.x + srect.width - 1,
                   stop = srect.y,
                sbottom = srect.y + srect.height - 1;
        // window corners
        const int wleft = dx,
                 wright = dx + snap_w - 1,
                   wtop = dy,
                wbottom = dy + snap_h - 1;

        const int dleft = std::abs(wleft - sleft),
                 dright = std::abs(wright - sright),
                   dtop = std::abs(wtop - stop),
                dbottom = std::abs(wbottom - sbottom);

        // snap left?
        if (dleft < snap_distance && dleft < dright)
          dx = sleft;
        // snap right?
        else if (dright < snap_distance && dright < dleft)
          dx = sright - snap_w;

        // snap top?
        if (dtop < snap_distance && dtop < dbottom)
          dy = stop;
        // snap bottom?
        else if (dbottom < snap_distance && dbottom < dtop)
          dy = sbottom - snap_h;
      }

      if (screen->doOpaqueMove()) {
        configure(dx, dy, frame.rect.width(), frame.rect.height());
      } else {
        XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                       screen->getOpGC(), frame.changing_x, frame.changing_y,
                       frame.changing_w -1, frame.changing_h - 1);

        frame.changing_x = dx;
        frame.changing_y = dy;

        XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                       screen->getOpGC(), frame.changing_x, frame.changing_y,
                       frame.changing_w - 1, frame.changing_h - 1);
      }

      screen->showPosition(dx, dy);
    }
  } else if ((functions & Func_Resize) &&
             (((me->state & Button1Mask) &&
               (me->window == frame.right_grip ||
                me->window == frame.left_grip)) ||
              (me->state & (Mod1Mask | Button3Mask) &&
               me->window == frame.window))) {
    Bool left = (me->window == frame.left_grip);

    if (! flags.resizing) {
      XGrabServer(blackbox->getXDisplay());
      XGrabPointer(blackbox->getXDisplay(), me->window, False,
                   ButtonMotionMask | ButtonReleaseMask,
                   GrabModeAsync, GrabModeAsync, None,
                   ((left) ? blackbox->getLowerLeftAngleCursor() :
                             blackbox->getLowerRightAngleCursor()),
                   CurrentTime);

      flags.resizing = True;

      int gx, gy;
      frame.grab_x = me->x - frame.border_w;
      frame.grab_y = me->y - (frame.border_w * 2);
      frame.changing_x = frame.rect.x();
      frame.changing_y = frame.rect.y();
      frame.changing_w = frame.rect.width() + (frame.border_w * 2);
      frame.changing_h = frame.rect.height() + (frame.border_w * 2);

      if (left)
        left_fixsize(&gx, &gy);
      else
        right_fixsize(&gx, &gy);

      screen->showGeometry(gx, gy);

      XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                     screen->getOpGC(), frame.changing_x, frame.changing_y,
                     frame.changing_w - 1, frame.changing_h - 1);
    } else {
      XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                     screen->getOpGC(), frame.changing_x, frame.changing_y,
                     frame.changing_w - 1, frame.changing_h - 1);

      int gx, gy;

      frame.changing_h = frame.rect.height() + (me->y - frame.grab_y);
      if (frame.changing_h < 1) frame.changing_h = 1;

      if (left) {
        frame.changing_x = me->x_root - frame.grab_x;
        if (frame.changing_x > (frame.rect.right()))
          frame.changing_x = frame.changing_x + frame.rect.width() - 1;

        left_fixsize(&gx, &gy);
      } else {
        frame.changing_w = frame.rect.width() + (me->x - frame.grab_x);
        if (frame.changing_w < 1) frame.changing_w = 1;

        right_fixsize(&gx, &gy);
      }

      XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                     screen->getOpGC(), frame.changing_x, frame.changing_y,
                     frame.changing_w - 1, frame.changing_h - 1);

      screen->showGeometry(gx, gy);
    }
  }
}


#ifdef    SHAPE
void BlackboxWindow::shapeEvent(XShapeEvent *) {
  if (blackbox->hasShapeExtensions() && flags.shaped) {
    configureShape();
  }
}
#endif // SHAPE


Bool BlackboxWindow::validateClient(void) {
  XSync(blackbox->getXDisplay(), False);

  XEvent e;
  if (XCheckTypedWindowEvent(blackbox->getXDisplay(), client.window,
                             DestroyNotify, &e) ||
      XCheckTypedWindowEvent(blackbox->getXDisplay(), client.window,
                             UnmapNotify, &e)) {
    XPutBackEvent(blackbox->getXDisplay(), &e);

    return False;
  }

  return True;
}


void BlackboxWindow::restore(Bool remap) {
  XChangeSaveSet(blackbox->getXDisplay(), client.window, SetModeDelete);
  XSelectInput(blackbox->getXDisplay(), client.window, NoEventMask);
  XSelectInput(blackbox->getXDisplay(), frame.plate, NoEventMask);

  restoreGravity();

  XUnmapWindow(blackbox->getXDisplay(), frame.window);
  XUnmapWindow(blackbox->getXDisplay(), client.window);

  XSetWindowBorderWidth(blackbox->getXDisplay(), client.window, client.old_bw);

  XEvent ev;
  if (! XCheckTypedWindowEvent(blackbox->getXDisplay(), client.window,
                               ReparentNotify, &ev)) {
    // according to the ICCCM - if the client doesn't reparent to
    // root, then we have to do it for them
    XReparentWindow(blackbox->getXDisplay(), client.window,
                    screen->getRootWindow(),
                    client.rect.x(), client.rect.y());
  }

  if (remap) XMapWindow(blackbox->getXDisplay(), client.window);

  XFlush(blackbox->getXDisplay());
}


// timer for autoraise
void BlackboxWindow::timeout(void) {
  screen->getWorkspace(blackbox_attrib.workspace)->raiseWindow(this);
}


void BlackboxWindow::changeBlackboxHints(BlackboxHints *net) {
  if ((net->flags & AttribShaded) &&
      ((blackbox_attrib.attrib & AttribShaded) !=
       (net->attrib & AttribShaded)))
    shade();

  if (flags.visible && // watch out for requests when we can not be seen
      (net->flags & (AttribMaxVert | AttribMaxHoriz)) &&
      ((blackbox_attrib.attrib & (AttribMaxVert | AttribMaxHoriz)) !=
       (net->attrib & (AttribMaxVert | AttribMaxHoriz)))) {
    if (flags.maximized) {
      maximize(0);
    } else {
      int button = 0;

      if ((net->flags & AttribMaxHoriz) && (net->flags & AttribMaxVert))
        button = ((net->attrib & (AttribMaxHoriz | AttribMaxVert)) ?  1 : 0);
      else if (net->flags & AttribMaxVert)
        button = ((net->attrib & AttribMaxVert) ? 2 : 0);
      else if (net->flags & AttribMaxHoriz)
        button = ((net->attrib & AttribMaxHoriz) ? 3 : 0);

      maximize(button);
    }
  }

  if ((net->flags & AttribOmnipresent) &&
      ((blackbox_attrib.attrib & AttribOmnipresent) !=
       (net->attrib & AttribOmnipresent)))
    stick();

  if ((net->flags & AttribWorkspace) &&
      (blackbox_attrib.workspace != net->workspace)) {
    screen->reassociateWindow(this, net->workspace, True);

    if (screen->getCurrentWorkspaceID() != net->workspace)
      withdraw();
    else deiconify();
  }

  if (net->flags & AttribDecoration) {
    switch (net->decoration) {
    case DecorNone:
      // clear all decorations except close
      decorations &= Decor_Close;

      break;

    default:
    case DecorNormal:
      decorations |= Decor_Titlebar | Decor_Handle | Decor_Border |
                     Decor_Iconify | Decor_Maximize | Decor_Menu;

      break;

    case DecorTiny:
      decorations |= Decor_Titlebar | Decor_Iconify | Decor_Menu;
      decorations &= ~(Decor_Border | Decor_Handle | Decor_Maximize);

      break;

    case DecorTool:
      decorations |= Decor_Titlebar | Decor_Menu;
      decorations &= ~(Decor_Iconify | Decor_Border | Decor_Handle |
                       Decor_Menu);
      functions |= Func_Move;

      break;
    }
    if (frame.window) {
      XMapSubwindows(blackbox->getXDisplay(), frame.window);
      XMapWindow(blackbox->getXDisplay(), frame.window);
    }

    reconfigure();
    setState(current_state);
  }
}


/*
 * Set the sizes of all components of the window frame
 * (the window decorations).
 * These values are based upon the current style settings and the client
 * window's dimensions.
 */
void BlackboxWindow::upsize(void) {
  frame.bevel_w = screen->getBevelWidth();

  if (decorations & Decor_Border) {
    frame.border_w = screen->getBorderWidth();
    if (!isTransient())
      frame.mwm_border_w = screen->getFrameWidth();
    else
      frame.mwm_border_w = 0;
  } else {
    frame.mwm_border_w = frame.border_w = 0;
  }

  if (decorations & Decor_Titlebar) {
    // the height of the titlebar is based upon the height of the font being
    // used to display the window's title
    WindowStyle *style = screen->getWindowStyle();
    if (i18n.multibyte())
      frame.title_h = (style->fontset_extents->max_ink_extent.height +
                       (frame.bevel_w * 2) + 2);
    else
      frame.title_h = (style->font->ascent + style->font->descent +
                       (frame.bevel_w * 2) + 2);

    frame.label_h = frame.title_h - (frame.bevel_w * 2);
    frame.button_w = frame.button_h = (frame.label_h - 2);
    frame.y_border = frame.title_h + frame.border_w;
  } else {
    frame.title_h = 0;
    frame.label_h = 0;
    frame.button_w = frame.button_h = 0;
    frame.y_border = 0;
  }

  frame.border_h = client.rect.height() + (frame.mwm_border_w * 2);

  if (decorations & Decor_Handle) {
    frame.y_handle = frame.y_border + frame.border_h + frame.border_w;
    frame.grip_w = frame.button_w * 2;
    frame.grip_h = frame.handle_h = screen->getHandleWidth();
  } else {
    frame.y_handle = frame.y_border + frame.border_h;
    frame.handle_h = 0;
    frame.grip_w = frame.grip_h = 0;
  }

  frame.rect.setWidth(client.rect.width() + (frame.mwm_border_w * 2));
  frame.rect.setHeight(frame.y_handle + frame.handle_h);
}


/*
 * Set the size and position of the client window.
 * These values are based upon the current style settings and the frame
 * window's dimensions.
 */
void BlackboxWindow::downsize(void) {
  frame.y_handle = frame.rect.height() - frame.handle_h;
  frame.border_h = frame.y_handle - frame.y_border -
                   ((decorations & Decor_Handle) ? frame.border_w : 0);

  client.rect.setRect(frame.rect.x() + frame.mwm_border_w + frame.border_w,
                      frame.rect.y() + frame.y_border + frame.mwm_border_w +
                      frame.border_w,
                      frame.rect.width() - (frame.mwm_border_w * 2),
                      frame.rect.height() - frame.y_border -
                      (frame.mwm_border_w * 2) - frame.handle_h -
                      ((decorations & Decor_Handle) ? frame.border_w : 0));
  frame.y_handle = frame.border_h + frame.y_border + frame.border_w;
}


void BlackboxWindow::right_fixsize(int *gx, int *gy) {
  // calculate the size of the client window and conform it to the
  // size specified by the size hints of the client window...
  int dx = frame.changing_w - client.base_width - (frame.mwm_border_w * 2) -
           (frame.border_w * 2) + (client.width_inc / 2);
  int dy = frame.changing_h - frame.y_border - client.base_height -
           frame.handle_h - (frame.border_w * 3) - (frame.mwm_border_w * 2)
           + (client.height_inc / 2);

  if (dx < static_cast<signed>(client.min_width)) dx = client.min_width;
  if (dy < static_cast<signed>(client.min_height)) dy = client.min_height;
  if (dx > static_cast<signed>(client.max_width)) dx = client.max_width;
  if (dy > static_cast<signed>(client.max_height)) dy = client.max_height;

  dx /= client.width_inc;
  dy /= client.height_inc;

  if (gx) *gx = dx;
  if (gy) *gy = dy;

  dx = (dx * client.width_inc) + client.base_width;
  dy = (dy * client.height_inc) + client.base_height;

  frame.changing_w = dx + (frame.mwm_border_w * 2) + (frame.border_w * 2);
  frame.changing_h = dy + frame.y_border + frame.handle_h +
                     (frame.mwm_border_w * 2) +  (frame.border_w * 3);
}


void BlackboxWindow::left_fixsize(int *gx, int *gy) {
  // calculate the size of the client window and conform it to the
  // size specified by the size hints of the client window...
  int dx = frame.rect.x() + frame.rect.width() - frame.changing_x -
           client.base_width - (frame.mwm_border_w * 2) +
           (client.width_inc / 2);
  int dy = frame.changing_h - frame.y_border - client.base_height -
           frame.handle_h - (frame.border_w * 3) - (frame.mwm_border_w * 2) +
           (client.height_inc / 2);

  if (dx < static_cast<signed>(client.min_width)) dx = client.min_width;
  if (dy < static_cast<signed>(client.min_height)) dy = client.min_height;
  if (dx > static_cast<signed>(client.max_width)) dx = client.max_width;
  if (dy > static_cast<signed>(client.max_height)) dy = client.max_height;

  dx /= client.width_inc;
  dy /= client.height_inc;

  if (gx) *gx = dx;
  if (gy) *gy = dy;

  dx = (dx * client.width_inc) + client.base_width;
  dy = (dy * client.height_inc) + client.base_height;

  frame.changing_w = dx + (frame.mwm_border_w * 2) + (frame.border_w * 2);
  frame.changing_x = frame.rect.x() + frame.rect.width() - frame.changing_w +
                     (frame.border_w * 2);
  frame.changing_h = dy + frame.y_border + frame.handle_h +
                     (frame.mwm_border_w * 2) + (frame.border_w * 3);
}
