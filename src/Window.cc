// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Window.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh at debian.org>
// Copyright (c) 1997 - 2000, 2002 Bradley T Hughes <bhughes at trolltech.com>
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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#ifdef DEBUG
#  include <stdio.h>
#endif // DEBUG
}

#include "i18n.hh"
#include "blackbox.hh"
#include "Font.hh"
#include "Netwm.hh"
#include "Pen.hh"
#include "PixmapCache.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Util.hh"
#include "Window.hh"
#include "Windowmenu.hh"


#if 0
static
void watch_decorations(const char *msg,BlackboxWindow::DecorationFlags flags) {
  fprintf(stderr, "Decorations: %s\n", msg);
  fprintf(stderr, "title   : %d\n",
          (flags & BlackboxWindow::Decor_Titlebar) != 0);
  fprintf(stderr, "handle  : %d\n",
          (flags & BlackboxWindow::Decor_Handle) != 0);
  fprintf(stderr, "border  : %d\n",
          (flags & BlackboxWindow::Decor_Border) != 0);
  fprintf(stderr, "iconify : %d\n",
          (flags & BlackboxWindow::Decor_Iconify) != 0);
  fprintf(stderr, "maximize: %d\n",
          (flags & BlackboxWindow::Decor_Maximize) != 0);
  fprintf(stderr, "close   : %d\n",
          (flags & BlackboxWindow::Decor_Close) != 0);
}
#endif


/*
 * Initializes the class with default values/the window's set initial values.
 */
BlackboxWindow::BlackboxWindow(Blackbox *b, Window w, BScreen *s) {
  // fprintf(stderr, "BlackboxWindow size: %d bytes\n",
  // sizeof(BlackboxWindow));

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::BlackboxWindow(): creating 0x%lx\n", w);
#endif // DEBUG

  /*
    set timer to zero... it is initialized properly later, so we check
    if timer is zero in the destructor, and assume that the window is not
    fully constructed if timer is zero...
  */
  timer = (bt::Timer*) 0;
  blackbox = b;
  client.window = w;
  screen = s;
  windowmenu = (Windowmenu*) 0;
  lastButtonPressTime = 0;

  if (! validateClient()) {
    delete this;
    return;
  }

  // fetch client size and placement
  XWindowAttributes wattrib;
  if (! XGetWindowAttributes(blackbox->XDisplay(),
                             client.window, &wattrib) ||
      ! wattrib.screen || wattrib.override_redirect) {
#ifdef    DEBUG
    fprintf(stderr,
            "BlackboxWindow::BlackboxWindow(): XGetWindowAttributes failed\n");
#endif // DEBUG

    delete this;
    return;
  }

  // set the eventmask early in the game so that we make sure we get
  // all the events we are interested in
  XSetWindowAttributes attrib_set;
  attrib_set.event_mask = PropertyChangeMask | FocusChangeMask |
                          StructureNotifyMask;
  attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask |
                                     ButtonMotionMask;
  XChangeWindowAttributes(blackbox->XDisplay(), client.window,
                          CWEventMask|CWDontPropagate, &attrib_set);

  client.state.moving = client.state.resizing = client.state.shaded =
    client.state.iconic = client.state.focused = client.state.modal =
    client.state.fullscreen = client.state.send_focus_message =
    client.state.shaped = False;
  client.state.maximized = 0;
  client.state.skip = SKIP_NONE;
  client.state.layer = LAYER_NORMAL;
  client.workspace = screen->getCurrentWorkspaceID();
  window_number = bt::BSENTINEL;
  client.decorations = Decor_Titlebar | Decor_Border | Decor_Handle |
    Decor_Iconify | Decor_Maximize;
  client.functions = Func_Resize | Func_Move | Func_Iconify | Func_Maximize;
  client.normal_hint_flags = 0;
  client.window_group = None;
  client.transient_for = (BlackboxWindow*) 0;
  client.window_type = None;
  client.strut = (bt::Netwm::Strut*) 0;
  /*
    set the initial size and location of client window (relative to the
    _root window_). This position is the reference point used with the
    window's gravity to find the window's initial position.
  */
  client.rect.setRect(wattrib.x, wattrib.y, wattrib.width, wattrib.height);
  client.old_bw = wattrib.border_width;
  client.current_state = NormalState;

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
  frame.pbutton = frame.ugrip = frame.fgrip = None;

  timer = new bt::Timer(blackbox, this);
  timer->setTimeout(blackbox->getAutoRaiseDelay());

  // get size, aspect, minimum/maximum size, ewmh and other hints set by the
  // client
  getNetwmHints();
  if (client.window_type == None)
    getMWMHints();
  getWMProtocols();
  getWMHints();
  getWMNormalHints();
  getTransientInfo();

  if (client.window_type == None) {
    if (isTransient())
      client.window_type = blackbox->netwm().wmWindowTypeDialog();
    else
      client.window_type = blackbox->netwm().wmWindowTypeNormal();
  }

  // adjust the window decorations based on transience, window type
  // and window sizes
  if (client.window_type == blackbox->netwm().wmWindowTypeDialog()) {
    client.decorations &= ~(Decor_Maximize | Decor_Handle);
    client.functions &= ~Func_Maximize;
  } else if (client.window_type == blackbox->netwm().wmWindowTypeSplash()) {
    client.decorations = client.functions = 0l;
  } else if (client.window_type == blackbox->netwm().wmWindowTypeUtility()) {
    client.decorations &= ~(Decor_Maximize | Decor_Iconify);
    client.functions   &= ~(Func_Maximize | Func_Iconify);
  } else if (client.window_type == blackbox->netwm().wmWindowTypeDock()) {
    client.decorations = 0l;
    client.functions = 0l;
  } else if (client.window_type == blackbox->netwm().wmWindowTypeDesktop()) {
    client.decorations = client.functions = 0l;
  } else if (isTransient()) {
    client.decorations &= ~(Decor_Maximize | Decor_Handle);
    client.functions &= ~Func_Maximize;
  }

  if ((client.normal_hint_flags & PMinSize) &&
      (client.normal_hint_flags & PMaxSize) &&
      client.max_width <= client.min_width &&
      client.max_height <= client.min_height) {
    client.decorations &= ~(Decor_Maximize | Decor_Handle);
    client.functions &= ~(Func_Resize | Func_Maximize);
  }

  frame.window = createToplevelWindow();
  blackbox->insertEventHandler(frame.window, this);

  frame.plate = createChildWindow(frame.window, ExposureMask);
  blackbox->insertEventHandler(frame.plate, this);

  if (client.decorations & Decor_Titlebar)
    createTitlebar();

  if (client.decorations & Decor_Handle)
    createHandle();

  // apply the size and gravity hint to the frame

  upsize();

  if (blackbox->isStartup() || isTransient() ||
      client.window_type == blackbox->netwm().wmWindowTypeDesktop() ||
      client.normal_hint_flags & (PPosition|USPosition)) {
    applyGravity(frame.rect);
  }

  /*
    the server needs to be grabbed here to prevent client's from sending
    events while we are in the process of configuring their window.
    We hold the grab until after we are done moving the window around.
  */

  XGrabServer(blackbox->XDisplay());

  associateClientWindow();

  blackbox->insertEventHandler(client.window, this);
  blackbox->insertWindow(client.window, this);

  // preserve the window's initial state on first map, and its current state
  // across a restart
  unsigned long initial_state = client.current_state;
  if (! getState())
    client.current_state = initial_state;

  if (client.state.iconic) {
    // prepare the window to be iconified
    client.current_state = IconicState;
    client.state.iconic = False;
  } else if (client.workspace != screen->getCurrentWorkspaceID()) {
    client.current_state = WithdrawnState;
  }

  configure(frame.rect.x(), frame.rect.y(),
            frame.rect.width(), frame.rect.height());

  positionWindows();

  XUngrabServer(blackbox->XDisplay());

#ifdef    SHAPE
  if (blackbox->hasShapeExtensions() && client.state.shaped)
    configureShape();
#endif // SHAPE

  // now that we know where to put the window and what it should look like
  // we apply the decorations
  decorate();

  grabButtons();

  XMapSubwindows(blackbox->XDisplay(), frame.window);

  // this ensures the title, buttons, and other decor are properly displayed
  redrawWindowFrame();

  client.premax = frame.rect;

  if (client.state.shaded) {
    client.state.shaded = False;
    initial_state = client.current_state;
    shade();

    /*
      At this point in the life of a window, current_state should only be set
      to IconicState if the window was an *icon*, not if it was shaded.
    */
    if (initial_state != IconicState)
      client.current_state = initial_state;
  }

  if (client.state.maximized && (client.functions & Func_Maximize))
    remaximize();
  else
    client.state.maximized = 0;

  // create this last so it only needs to be configured once
  windowmenu =
    new Windowmenu(*blackbox, screen->screenNumber(), this);
}


BlackboxWindow::~BlackboxWindow(void) {
#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::~BlackboxWindow: destroying 0x%lx\n",
          client.window);
#endif // DEBUG

  if (! timer) // window not managed...
    return;

  if (client.state.moving || client.state.resizing) {
    screen->hideGeometry();
    XUngrabPointer(blackbox->XDisplay(), CurrentTime);
  }

  delete timer;

  delete windowmenu;

  if (client.strut) {
    screen->removeStrut(client.strut);
    delete client.strut;
  }

  if (client.window_group) {
    BWindowGroup *group = blackbox->findWindowGroup(client.window_group);
    if (group) group->removeWindow(this);
  }

  // remove ourselves from our transient_for
  if (isTransient()) {
    if (client.transient_for != (BlackboxWindow *) ~0ul)
      client.transient_for->client.transientList.remove(this);

    client.transient_for = (BlackboxWindow*) 0;
  }

  if (! client.transientList.empty()) {
    // reset transient_for for all transients
    BlackboxWindowList::iterator it, end = client.transientList.end();
    for (it = client.transientList.begin(); it != end; ++it)
      (*it)->client.transient_for = (BlackboxWindow*) 0;
  }

  if (frame.title)
    destroyTitlebar();

  if (frame.handle)
    destroyHandle();

  if (frame.plate) {
    blackbox->removeEventHandler(frame.plate);
    XDestroyWindow(blackbox->XDisplay(), frame.plate);
  }

  if (frame.window) {
    blackbox->removeEventHandler(frame.window);
    XDestroyWindow(blackbox->XDisplay(), frame.window);
  }

  blackbox->removeEventHandler(client.window);
  blackbox->removeWindow(client.window);
}


/*
 * Creates a new top level window, with a given location, size, and border
 * width.
 * Returns: the newly created window
 */
Window BlackboxWindow::createToplevelWindow(void) {
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWColormap | CWOverrideRedirect | CWEventMask;

  attrib_create.colormap = screen->screenInfo().colormap();
  attrib_create.override_redirect = True;
  attrib_create.event_mask = EnterWindowMask | LeaveWindowMask;

  return XCreateWindow(blackbox->XDisplay(), screen->screenInfo().rootWindow(),
                       0, 0, 1, 1, frame.border_w,
                       screen->screenInfo().depth(), InputOutput,
                       screen->screenInfo().visual(),
                       create_mask, &attrib_create);
}


/*
 * Creates a child window, and optionally associates a given cursor with
 * the new window.
 */
Window BlackboxWindow::createChildWindow(Window parent,
                                         unsigned long event_mask,
                                         Cursor cursor) {
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWEventMask;

  attrib_create.event_mask = event_mask;

  if (cursor) {
    create_mask |= CWCursor;
    attrib_create.cursor = cursor;
  }

  return XCreateWindow(blackbox->XDisplay(), parent, 0, 0, 1, 1, 0,
                       screen->screenInfo().depth(), InputOutput,
                       screen->screenInfo().visual(),
                       create_mask, &attrib_create);
}


void BlackboxWindow::associateClientWindow(void) {
  XSetWindowBorderWidth(blackbox->XDisplay(), client.window, 0);
  getWMName();
  getWMIconName();

  XChangeSaveSet(blackbox->XDisplay(), client.window, SetModeInsert);

  XSelectInput(blackbox->XDisplay(), frame.plate, SubstructureRedirectMask);

  /*
    note we used to grab around this call to XReparentWindow however the
    server is now grabbed before this method is called
  */
  unsigned long event_mask = PropertyChangeMask | FocusChangeMask |
                             StructureNotifyMask;
  XSelectInput(blackbox->XDisplay(), client.window,
               event_mask & ~StructureNotifyMask);
  XReparentWindow(blackbox->XDisplay(), client.window, frame.plate, 0, 0);
  XSelectInput(blackbox->XDisplay(), client.window, event_mask);

  XRaiseWindow(blackbox->XDisplay(), frame.plate);
  XMapSubwindows(blackbox->XDisplay(), frame.plate);

#ifdef    SHAPE
  if (blackbox->hasShapeExtensions()) {
    XShapeSelectInput(blackbox->XDisplay(), client.window,
                      ShapeNotifyMask);

    Bool shaped = False;
    int foo;
    unsigned int ufoo;

    XShapeQueryExtents(blackbox->XDisplay(), client.window, &shaped,
                       &foo, &foo, &ufoo, &ufoo, &foo, &foo, &foo,
                       &ufoo, &ufoo);
    client.state.shaped = shaped;
  }
#endif // SHAPE
}


void BlackboxWindow::decorate(void) {
  bt::Texture texture;

  if (client.decorations & Decor_Titlebar) {
    // render focused button texture
    texture = screen->getWindowStyle()->b_focus;
    frame.fbutton =
      bt::PixmapCache::find(screen->screenNumber(), texture,
                            frame.button_w, frame.button_w, frame.fbutton);
    if (! frame.fbutton)
      frame.fbutton_pixel = texture.color().pixel(screen->screenNumber());

    // render unfocused button texture
    texture = screen->getWindowStyle()->b_unfocus;
    frame.ubutton =
      bt::PixmapCache::find(screen->screenNumber(), texture,
                            frame.button_w, frame.button_w,
                            frame.ubutton);
    if (! frame.ubutton)
      frame.ubutton_pixel = texture.color().pixel(screen->screenNumber());

    // render pressed button texture
    texture = screen->getWindowStyle()->b_pressed;
    frame.pbutton =
      bt::PixmapCache::find(screen->screenNumber(), texture,
                            frame.button_w, frame.button_w, frame.pbutton);
    if (! frame.pbutton)
      frame.pbutton_pixel = texture.color().pixel(screen->screenNumber());

    // render focused titlebar texture
    texture = screen->getWindowStyle()->t_focus;
    frame.ftitle =
      bt::PixmapCache::find(screen->screenNumber(), texture,
                            frame.inside_w, frame.title_h, frame.ftitle);
    if (! frame.ftitle)
      frame.ftitle_pixel = texture.color().pixel(screen->screenNumber());

    // render unfocused titlebar texture
    texture = screen->getWindowStyle()->t_unfocus;
    frame.utitle =
      bt::PixmapCache::find(screen->screenNumber(), texture,
                            frame.inside_w, frame.title_h, frame.utitle);
    if (! frame.utitle)
      frame.utitle_pixel = texture.color().pixel(screen->screenNumber());

    XSetWindowBorder(blackbox->XDisplay(), frame.title,
                     screen->getBorderColor()->pixel(screen->screenNumber()));

    decorateLabel();
  }

  if (client.decorations & Decor_Border) {
    frame.fborder_pixel =
      screen->getWindowStyle()->f_focus.color().pixel(screen->screenNumber());
    frame.uborder_pixel =
      screen->getWindowStyle()->
      f_unfocus.color().pixel(screen->screenNumber());
  }

  if (client.decorations & Decor_Handle) {
    texture = screen->getWindowStyle()->h_focus;
    frame.fhandle =
      bt::PixmapCache::find(screen->screenNumber(), texture,
                            frame.inside_w, frame.handle_h, frame.fhandle);
    if (! frame.fhandle)
      frame.fhandle_pixel = texture.color().pixel(screen->screenNumber());

    texture = screen->getWindowStyle()->h_unfocus;
    frame.uhandle =
      bt::PixmapCache::find(screen->screenNumber(), texture,
                            frame.inside_w, frame.handle_h, frame.uhandle);
    if (! frame.uhandle)
      frame.uhandle_pixel = texture.color().pixel(screen->screenNumber());

    texture = screen->getWindowStyle()->g_focus;
    frame.fgrip =
      bt::PixmapCache::find(screen->screenNumber(), texture,
                            frame.grip_w, frame.handle_h, frame.fgrip);
    if (! frame.fgrip)
      frame.fgrip_pixel = texture.color().pixel(screen->screenNumber());

    texture = screen->getWindowStyle()->g_unfocus;
    frame.ugrip =
      bt::PixmapCache::find(screen->screenNumber(), texture,
                            frame.grip_w, frame.handle_h, frame.ugrip);
    if (! frame.ugrip)
      frame.ugrip_pixel = texture.color().pixel(screen->screenNumber());

    XSetWindowBorder(blackbox->XDisplay(), frame.handle,
                     screen->getBorderColor()->pixel(screen->screenNumber()));
    XSetWindowBorder(blackbox->XDisplay(), frame.left_grip,
                     screen->getBorderColor()->pixel(screen->screenNumber()));
    XSetWindowBorder(blackbox->XDisplay(), frame.right_grip,
                     screen->getBorderColor()->pixel(screen->screenNumber()));
  }

  XSetWindowBorder(blackbox->XDisplay(), frame.window,
                   screen->getBorderColor()->pixel(screen->screenNumber()));
}


void BlackboxWindow::decorateLabel(void) {
  bt::Texture texture;

  texture = screen->getWindowStyle()->l_focus;
  frame.flabel =
    bt::PixmapCache::find(screen->screenNumber(), texture,
                          frame.label_w, frame.label_h, frame.flabel);
  if (! frame.flabel)
    frame.flabel_pixel = texture.color().pixel(screen->screenNumber());

  texture = screen->getWindowStyle()->l_unfocus;
  frame.ulabel =
    bt::PixmapCache::find(screen->screenNumber(), texture,
                          frame.label_w, frame.label_h, frame.ulabel);
  if (! frame.ulabel)
    frame.ulabel_pixel = texture.color().pixel(screen->screenNumber());
}


void BlackboxWindow::createHandle(void) {
  frame.handle = createChildWindow(frame.window,
                                   ButtonPressMask | ButtonReleaseMask |
                                   ButtonMotionMask | ExposureMask);
  blackbox->insertEventHandler(frame.handle, this);

  frame.left_grip =
    createChildWindow(frame.handle,
                      ButtonPressMask | ButtonReleaseMask |
                      ButtonMotionMask | ExposureMask,
                      blackbox->getLowerLeftAngleCursor());
  blackbox->insertEventHandler(frame.left_grip, this);

  frame.right_grip =
    createChildWindow(frame.handle,
                      ButtonPressMask | ButtonReleaseMask |
                      ButtonMotionMask | ExposureMask,
                      blackbox->getLowerRightAngleCursor());
  blackbox->insertEventHandler(frame.right_grip, this);
}


void BlackboxWindow::destroyHandle(void) {
  bt::PixmapCache::release(frame.fhandle);
  bt::PixmapCache::release(frame.uhandle);
  bt::PixmapCache::release(frame.fgrip);
  bt::PixmapCache::release(frame.ugrip);

  blackbox->removeEventHandler(frame.left_grip);
  blackbox->removeEventHandler(frame.right_grip);

  XDestroyWindow(blackbox->XDisplay(), frame.left_grip);
  XDestroyWindow(blackbox->XDisplay(), frame.right_grip);
  frame.left_grip = frame.right_grip = None;

  blackbox->removeEventHandler(frame.handle);
  XDestroyWindow(blackbox->XDisplay(), frame.handle);
  frame.handle = None;
}


void BlackboxWindow::createTitlebar(void) {
  frame.title = createChildWindow(frame.window,
                                  ButtonPressMask | ButtonReleaseMask |
                                  ButtonMotionMask | ExposureMask);
  frame.label = createChildWindow(frame.title,
                                  ButtonPressMask | ButtonReleaseMask |
                                  ButtonMotionMask | ExposureMask);
  blackbox->insertEventHandler(frame.title, this);
  blackbox->insertEventHandler(frame.label, this);

  if (client.decorations & Decor_Iconify) createIconifyButton();
  if (client.decorations & Decor_Maximize) createMaximizeButton();
  if (client.decorations & Decor_Close) createCloseButton();
}


void BlackboxWindow::destroyTitlebar(void) {
  if (frame.close_button)
    destroyCloseButton();

  if (frame.iconify_button)
    destroyIconifyButton();

  if (frame.maximize_button)
    destroyMaximizeButton();

  bt::PixmapCache::release(frame.ftitle);
  bt::PixmapCache::release(frame.utitle);

  bt::PixmapCache::release(frame.flabel);
  bt::PixmapCache::release(frame.ulabel);

  bt::PixmapCache::release(frame.fbutton);
  bt::PixmapCache::release(frame.ubutton);
  bt::PixmapCache::release(frame.pbutton);

  blackbox->removeEventHandler(frame.title);
  blackbox->removeEventHandler(frame.label);

  XDestroyWindow(blackbox->XDisplay(), frame.label);
  XDestroyWindow(blackbox->XDisplay(), frame.title);
  frame.title = frame.label = None;
}


void BlackboxWindow::createCloseButton(void) {
  if (frame.title != None) {
    frame.close_button = createChildWindow(frame.title,
                                           ButtonPressMask |
                                           ButtonReleaseMask |
                                           ButtonMotionMask | ExposureMask);
    blackbox->insertEventHandler(frame.close_button, this);
  }
}


void BlackboxWindow::destroyCloseButton(void) {
  blackbox->removeEventHandler(frame.close_button);
  XDestroyWindow(blackbox->XDisplay(), frame.close_button);
  frame.close_button = None;
}


void BlackboxWindow::createIconifyButton(void) {
  if (frame.title != None) {
    frame.iconify_button = createChildWindow(frame.title,
                                             ButtonPressMask |
                                             ButtonReleaseMask |
                                             ButtonMotionMask | ExposureMask);
    blackbox->insertEventHandler(frame.iconify_button, this);
  }
}


void BlackboxWindow::destroyIconifyButton(void) {
  blackbox->removeEventHandler(frame.iconify_button);
  XDestroyWindow(blackbox->XDisplay(), frame.iconify_button);
  frame.iconify_button = None;
}


void BlackboxWindow::createMaximizeButton(void) {
  if (frame.title != None) {
    frame.maximize_button = createChildWindow(frame.title,
                                              ButtonPressMask |
                                              ButtonReleaseMask |
                                              ButtonMotionMask | ExposureMask);
    blackbox->insertEventHandler(frame.maximize_button, this);
  }
}


void BlackboxWindow::destroyMaximizeButton(void) {
  blackbox->removeEventHandler(frame.maximize_button);
  XDestroyWindow(blackbox->XDisplay(), frame.maximize_button);
  frame.maximize_button = None;
}


void BlackboxWindow::positionButtons(bool redecorate_label) {
  unsigned int bw = frame.button_w + frame.bevel_w + 1,
    by = frame.bevel_w + 1, lx = by, lw = frame.inside_w - by;

  if (client.decorations & Decor_Iconify) {
    if (frame.iconify_button == None) createIconifyButton();

    XMoveResizeWindow(blackbox->XDisplay(), frame.iconify_button, by, by,
                      frame.button_w, frame.button_w);
    XMapWindow(blackbox->XDisplay(), frame.iconify_button);

    lx += bw;
    lw -= bw;
  } else if (frame.iconify_button) {
    destroyIconifyButton();
  }
  int bx = frame.inside_w - bw;

  if (client.decorations & Decor_Close) {
    if (frame.close_button == None) createCloseButton();

    XMoveResizeWindow(blackbox->XDisplay(), frame.close_button, bx, by,
                      frame.button_w, frame.button_w);
    XMapWindow(blackbox->XDisplay(), frame.close_button);

    bx -= bw;
    lw -= bw;
  } else if (frame.close_button) {
    destroyCloseButton();
  }
  if (client.decorations & Decor_Maximize) {
    if (frame.maximize_button == None) createMaximizeButton();

    XMoveResizeWindow(blackbox->XDisplay(), frame.maximize_button, bx, by,
                      frame.button_w, frame.button_w);
    XMapWindow(blackbox->XDisplay(), frame.maximize_button);
    XClearWindow(blackbox->XDisplay(), frame.maximize_button);

    lw -= bw;
  } else if (frame.maximize_button) {
    destroyMaximizeButton();
  }
  frame.label_w = lw - by;
  XMoveResizeWindow(blackbox->XDisplay(), frame.label, lx, frame.bevel_w,
                    frame.label_w, frame.label_h);
  if (redecorate_label) decorateLabel();

  redrawLabel();
  redrawAllButtons();
}


void BlackboxWindow::reconfigure(void) {
  restoreGravity(client.rect);
  upsize();
  applyGravity(frame.rect);
  positionWindows();
  decorate();
  redrawWindowFrame();

  ungrabButtons();
  grabButtons();

  if (windowmenu) windowmenu->reconfigure();
}


void BlackboxWindow::grabButtons(void) {
  if (! screen->isSloppyFocus() || screen->doClickRaise())
    // grab button 1 for changing focus/raising
    blackbox->grabButton(Button1, 0, frame.plate, True, ButtonPressMask,
                         GrabModeSync, GrabModeSync, frame.plate, None,
                         screen->allowScrollLock());

  if (client.functions & Func_Move)
    blackbox->grabButton(Button1, Mod1Mask, frame.window, True,
                         ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                         GrabModeAsync, frame.window,
                         blackbox->getMoveCursor(),
                         screen->allowScrollLock());
  if (client.functions & Func_Resize)
    blackbox->grabButton(Button3, Mod1Mask, frame.window, True,
                         ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                         GrabModeAsync, frame.window,
                         blackbox->getLowerRightAngleCursor(),
                         screen->allowScrollLock());
  // alt+middle lowers the window
  blackbox->grabButton(Button2, Mod1Mask, frame.window, True,
                       ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
                       frame.window, None, screen->allowScrollLock());
}


void BlackboxWindow::ungrabButtons(void) {
  blackbox->ungrabButton(Button1, 0, frame.plate);
  blackbox->ungrabButton(Button1, Mod1Mask, frame.window);
  blackbox->ungrabButton(Button2, Mod1Mask, frame.window);
  blackbox->ungrabButton(Button3, Mod1Mask, frame.window);
}


void BlackboxWindow::positionWindows(void) {
  XMoveResizeWindow(blackbox->XDisplay(), frame.window,
                    frame.rect.x(), frame.rect.y(), frame.inside_w,
                    (client.state.shaded) ? frame.title_h : frame.inside_h);
  XSetWindowBorderWidth(blackbox->XDisplay(), frame.window,
                        frame.border_w);
  XSetWindowBorderWidth(blackbox->XDisplay(), frame.plate,
                        frame.mwm_border_w);
  XMoveResizeWindow(blackbox->XDisplay(), frame.plate,
                    frame.margin.left - frame.mwm_border_w - frame.border_w,
                    frame.margin.top - frame.mwm_border_w - frame.border_w,
                    client.rect.width(), client.rect.height());
  XMoveResizeWindow(blackbox->XDisplay(), client.window,
                    0, 0, client.rect.width(), client.rect.height());
  // ensure client.rect contains the real location
  client.rect.setPos(frame.rect.left() + frame.margin.left,
                     frame.rect.top() + frame.margin.top);

  if (client.decorations & Decor_Titlebar) {
    if (frame.title == None) createTitlebar();

    XSetWindowBorderWidth(blackbox->XDisplay(), frame.title,
                          frame.border_w);
    XMoveResizeWindow(blackbox->XDisplay(), frame.title, -frame.border_w,
                      -frame.border_w, frame.inside_w, frame.title_h);

    positionButtons();
    XMapSubwindows(blackbox->XDisplay(), frame.title);
    XMapWindow(blackbox->XDisplay(), frame.title);
  } else if (frame.title) {
    destroyTitlebar();
  }

  if (client.decorations & Decor_Handle) {
    if (frame.handle == None) createHandle();
    XSetWindowBorderWidth(blackbox->XDisplay(), frame.handle,
                          frame.border_w);
    XSetWindowBorderWidth(blackbox->XDisplay(), frame.left_grip,
                          frame.border_w);
    XSetWindowBorderWidth(blackbox->XDisplay(), frame.right_grip,
                          frame.border_w);

    // use client.rect here so the value is correct even if shaded
    XMoveResizeWindow(blackbox->XDisplay(), frame.handle,
                      -frame.border_w,
                      client.rect.height() + frame.margin.top +
                      frame.mwm_border_w - frame.border_w,
                      frame.inside_w, frame.handle_h);
    XMoveResizeWindow(blackbox->XDisplay(), frame.left_grip,
                      -frame.border_w, -frame.border_w,
                      frame.grip_w, frame.handle_h);
    XMoveResizeWindow(blackbox->XDisplay(), frame.right_grip,
                      frame.inside_w - frame.grip_w - frame.border_w,
                      -frame.border_w, frame.grip_w, frame.handle_h);

    XMapSubwindows(blackbox->XDisplay(), frame.handle);
    XMapWindow(blackbox->XDisplay(), frame.handle);
  } else if (frame.handle) {
    destroyHandle();
  }

  XSync(blackbox->XDisplay(), False);
}


void BlackboxWindow::getWMName(void) {
  XTextProperty text_prop;

  std::string name;

  if (! blackbox->netwm().readWMName(client.window, name) || name.empty()) {
    if (XGetWMName(blackbox->XDisplay(), client.window, &text_prop)) {
      name = bt::textPropertyToString(blackbox->XDisplay(), text_prop);
      XFree((char *) text_prop.value);
    }
  }

  if (! name.empty()) {
    client.title = name;
#if 0
    // need to ellide titles that are longer than the titlebar
    client.title = bt::ellideText(name, 60, "...");
    if (client.title != name)
      blackbox->netwm().setWMVisibleName(client.window, client.title);
#endif
  } else {
    client.title = bt::i18n(WindowSet, WindowUnnamed, "Unnamed");
    blackbox->netwm().setWMVisibleName(client.window, client.title);
  }
}


void BlackboxWindow::getWMIconName(void) {
  XTextProperty text_prop;

  std::string name;

  if (! blackbox->netwm().readWMIconName(client.window, name) ||
      name.empty()) {
    if (XGetWMIconName(blackbox->XDisplay(), client.window, &text_prop)) {
      name = bt::textPropertyToString(blackbox->XDisplay(), text_prop);
      XFree((char *) text_prop.value);
    }
  }

  if (! name.empty()) {
    client.icon_title = name;
  } else {
    client.icon_title = client.title;
    blackbox->netwm().setWMVisibleIconName(client.window, client.icon_title);
  }
}


void BlackboxWindow::getNetwmHints(void) {
  // wm_name and wm_icon_name are read separately

  bool ret;
  const bt::Netwm& netwm = blackbox->netwm();

  bt::Netwm::AtomList atoms;
  ret = netwm.readWMWindowType(client.window, atoms);
  if (ret) {
    bt::Netwm::AtomList::iterator it = atoms.begin(), end = atoms.end();
    for (; it != end; ++it) {
      if (netwm.isSupportedWMWindowType(*it)) {
        client.window_type = *it;
        break;
      }
    }
    if (client.window_type == netwm.wmWindowTypeDesktop()) {
      // make me omnipresent
      client.state.layer = LAYER_DESKTOP;
    }
  }

  atoms.clear();
  ret = netwm.readWMState(client.window, atoms);
  if (ret) {
    bt::Netwm::AtomList::iterator it = atoms.begin(), end = atoms.end();
    for (; it != end; ++it) {
      Atom state = *it;
      if (state == netwm.wmStateModal()) {
        if (isTransient())
          client.state.modal = True;
      } else if (state == netwm.wmStateMaximizedVert()) {
        if (client.state.maximized == 0)
          client.state.maximized = 2;
        else if (client.state.maximized == 3)
          client.state.maximized = 1;
      } else if (state == netwm.wmStateMaximizedHorz()) {
        if (client.state.maximized == 0)
          client.state.maximized = 3;
        else if (client.state.maximized == 2)
          client.state.maximized = 1;
      } else if (state == netwm.wmStateShaded()) {
        client.state.shaded = True;
      } else if (state == netwm.wmStateSkipTaskbar()) {
        if (client.state.skip == SKIP_NONE)
          client.state.skip = SKIP_TASKBAR;
        else if (client.state.skip == SKIP_PAGER)
          client.state.skip = SKIP_BOTH;
      } else if (state == netwm.wmStateSkipPager()) {
        if (client.state.skip == SKIP_NONE)
          client.state.skip = SKIP_PAGER;
        else if (client.state.skip == SKIP_TASKBAR)
          client.state.skip = SKIP_BOTH;
      } else if (state == netwm.wmStateHidden()) {
        client.state.iconic = True;
      } else if (state == netwm.wmStateFullscreen()) {
        client.state.fullscreen = True;
        client.state.layer = LAYER_FULLSCREEN;
      } else if (state == netwm.wmStateAbove()) {
        client.state.layer = LAYER_ABOVE;
      } else if (state == netwm.wmStateBelow()) {
        client.state.layer = LAYER_BELOW;
      }
    }
  }

  unsigned int desktop;
  ret = netwm.readWMDesktop(client.window, desktop);
  if (ret) {
    if (desktop != 0xFFFFFFFF)
      client.workspace = desktop;
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

  if (XGetWMProtocols(blackbox->XDisplay(), client.window,
                      &proto, &num_return)) {
    for (int i = 0; i < num_return; ++i) {
      if (proto[i] == blackbox->getWMDeleteAtom()) {
        client.decorations |= Decor_Close;
        client.functions |= Func_Close;
      } else if (proto[i] == blackbox->getWMTakeFocusAtom()) {
        client.state.send_focus_message = True;
      }
    }
    XFree(proto);
  }
}


/*
 * Gets the value of the WM_HINTS property.
 * If the property is not set, then use a set of default values.
 */
void BlackboxWindow::getWMHints(void) {
  client.focus_mode = F_Passive;

  // remove from current window group
  if (client.window_group) {
    BWindowGroup *group = blackbox->findWindowGroup(client.window_group);
    if (group) group->removeWindow(this);
  }
  client.window_group = None;

  XWMHints *wmhint = XGetWMHints(blackbox->XDisplay(), client.window);
  if (! wmhint)
    return;

  if (wmhint->flags & InputHint) {
    if (wmhint->input == True) {
      if (client.state.send_focus_message)
        client.focus_mode = F_LocallyActive;
    } else {
      if (client.state.send_focus_message)
        client.focus_mode = F_GloballyActive;
      else
        client.focus_mode = F_NoInput;
    }
  }

  if (wmhint->flags & StateHint)
    client.current_state = wmhint->initial_state;

  if (wmhint->flags & WindowGroupHint &&
      wmhint->window_group != screen->screenInfo().rootWindow()) {
    client.window_group = wmhint->window_group;

    // add window to the appropriate group
    BWindowGroup *group = blackbox->findWindowGroup(client.window_group);
    if (! group) { // no group found, create it!
      new BWindowGroup(blackbox, client.window_group);
      group = blackbox->findWindowGroup(client.window_group);
    }
    if (group)
      group->addWindow(this);
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

  client.min_width = client.min_height =
    client.width_inc = client.height_inc = 1;
  client.base_width = client.base_height = 0;
  client.win_gravity = NorthWestGravity;
#if 0
  client.min_aspect_x = client.min_aspect_y =
    client.max_aspect_x = client.max_aspect_y = 1;
#endif

  /*
    use the full screen, not the strut modified size. otherwise when the
    availableArea changes max_width/height will be incorrect and lead to odd
    rendering bugs.
  */
  client.max_width = (unsigned)-1;
  client.max_height = (unsigned)-1;

  if (! XGetWMNormalHints(blackbox->XDisplay(), client.window,
                          &sizehint, &icccm_mask))
    return;

  client.normal_hint_flags = sizehint.flags;

  if (sizehint.flags & PMinSize) {
    if (sizehint.min_width >= 0)
      client.min_width = sizehint.min_width;
    if (sizehint.min_height >= 0)
      client.min_height = sizehint.min_height;
  }

  if (sizehint.flags & PMaxSize) {
    if (sizehint.max_width > static_cast<signed>(client.min_width))
      client.max_width = sizehint.max_width;
    else
      client.max_width = client.min_width;

    if (sizehint.max_height > static_cast<signed>(client.min_height))
      client.max_height = sizehint.max_height;
    else
      client.max_height = client.min_height;
  }

  if (sizehint.flags & PResizeInc) {
    client.width_inc = sizehint.width_inc;
    client.height_inc = sizehint.height_inc;
  }

#if 0 // we do not support this at the moment
  if (sizehint.flags & PAspect) {
    client.min_aspect_x = sizehint.min_aspect.x;
    client.min_aspect_y = sizehint.min_aspect.y;
    client.max_aspect_x = sizehint.max_aspect.x;
    client.max_aspect_y = sizehint.max_aspect.y;
  }
#endif

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
  MwmHints *mwm_hint = (MwmHints*) 0;

  int ret = XGetWindowProperty(blackbox->XDisplay(), client.window,
                               blackbox->getMotifWMHintsAtom(), 0,
                               PropMwmHintsElements, False,
                               blackbox->getMotifWMHintsAtom(), &atom_return,
                               &format, &num, &len,
                               (unsigned char **) &mwm_hint);

  if (ret != Success || ! mwm_hint || num != PropMwmHintsElements)
    return;

  if (mwm_hint->flags & MwmHintsDecorations) {
    if (mwm_hint->decorations & MwmDecorAll) {
      client.decorations = Decor_Titlebar | Decor_Handle | Decor_Border |
                    Decor_Iconify | Decor_Maximize | Decor_Close;
    } else {
      client.decorations = 0l;

      if (mwm_hint->decorations & MwmDecorBorder)
        client.decorations |= Decor_Border;
      if (mwm_hint->decorations & MwmDecorHandle)
        client.decorations |= Decor_Handle;
      if (mwm_hint->decorations & MwmDecorTitle)
        client.decorations |= Decor_Titlebar;
      if (mwm_hint->decorations & MwmDecorIconify)
        client.decorations |= Decor_Iconify;
      if (mwm_hint->decorations & MwmDecorMaximize)
        client.decorations |= Decor_Maximize;
    }
  }

  if (mwm_hint->flags & MwmHintsFunctions) {
    if (mwm_hint->functions & MwmFuncAll) {
      client.functions = Func_Resize | Func_Move | Func_Iconify |
        Func_Maximize | Func_Close;
    } else {
      client.functions = 0l;

      if (mwm_hint->functions & MwmFuncResize)
        client.functions |= Func_Resize;
      if (mwm_hint->functions & MwmFuncMove)
        client.functions |= Func_Move;
      if (mwm_hint->functions & MwmFuncIconify)
        client.functions |= Func_Iconify;
      if (mwm_hint->functions & MwmFuncMaximize)
        client.functions |= Func_Maximize;
      if (mwm_hint->functions & MwmFuncClose)
        client.functions |= Func_Close;
    }
  }
  XFree(mwm_hint);
}


void BlackboxWindow::getTransientInfo(void) {
  if (client.transient_for &&
      client.transient_for != (BlackboxWindow *) ~0ul) {
    // reset transient_for in preparation of looking for a new owner
    client.transient_for->client.transientList.remove(this);
  }

  // we have no transient_for until we find a new one
  client.transient_for = (BlackboxWindow *) 0;

  Window trans_for;
  if (!XGetTransientForHint(blackbox->XDisplay(), client.window,
                            &trans_for)) {
    // transient_for hint not set
    return;
  }

  if (trans_for == client.window) {
    // wierd client... treat this window as a normal window
    return;
  }

  if (trans_for == None || trans_for == screen->screenInfo().rootWindow()) {
    // this is an undocumented interpretation of the ICCCM. a transient
    // associated with None/Root/itself is assumed to be a modal root
    // transient.  we don't support the concept of a global transient,
    // so we just associate this transient with nothing, and perhaps
    // we will add support later for global modality.
    client.transient_for = (BlackboxWindow *) ~0ul;
    client.state.modal = True;
    return;
  }

  client.transient_for = blackbox->findWindow(trans_for);
  if (! client.transient_for &&
      client.window_group && trans_for == client.window_group) {
    // no direct transient_for, perhaps this is a group transient?
    BWindowGroup *group = blackbox->findWindowGroup(client.window_group);
    if (group) client.transient_for = group->find(screen);
  }

  if (! client.transient_for || client.transient_for == this) {
    // no transient_for found, or we have a wierd client that wants to be
    // a transient for itself, so we treat this window as a normal window
    client.transient_for = (BlackboxWindow*) 0;
    return;
  }

  // Check for a circular transient state: this can lock up Blackbox
  // when it tries to find the non-transient window for a transient.
  BlackboxWindow *w = this;
  while(w->client.transient_for &&
        w->client.transient_for != (BlackboxWindow *) ~0ul) {
    if(w->client.transient_for == this) {
      client.transient_for = (BlackboxWindow*) 0;
      break;
    }
    w = w->client.transient_for;
  }

  if (client.transient_for) {
    // register ourselves with our new transient_for
    client.transient_for->client.transientList.push_back(this);
  }
}


BlackboxWindow *BlackboxWindow::getTransientFor(void) const {
  if (client.transient_for &&
      client.transient_for != (BlackboxWindow*) ~0ul)
    return client.transient_for;
  return 0;
}


/*
 * This function is responsible for updating both the client and the frame
 * rectangles.
 * According to the ICCCM a client message is not sent for a resize, only a
 * move.
 */
void BlackboxWindow::configure(int dx, int dy,
                               unsigned int dw, unsigned int dh) {
  bool send_event = ((frame.rect.x() != dx || frame.rect.y() != dy) &&
                     ! client.state.moving);

  if (dw != frame.rect.width() || dh != frame.rect.height()) {
    frame.rect.setRect(dx, dy, dw, dh);
    frame.inside_w = frame.rect.width() - (frame.border_w * 2);
    frame.inside_h = frame.rect.height() - (frame.border_w * 2);

    if (frame.rect.right() <= 0 || frame.rect.bottom() <= 0)
      frame.rect.setPos(0, 0);

    client.rect.setCoords(frame.rect.left() + frame.margin.left,
                          frame.rect.top() + frame.margin.top,
                          frame.rect.right() - frame.margin.right,
                          frame.rect.bottom() - frame.margin.bottom);

#ifdef    SHAPE
    if (blackbox->hasShapeExtensions() && client.state.shaped) {
      configureShape();
    }
#endif // SHAPE

    positionWindows();
    decorate();
    redrawWindowFrame();
  } else {
    frame.rect.setPos(dx, dy);

    XMoveWindow(blackbox->XDisplay(), frame.window,
                frame.rect.x(), frame.rect.y());
    /*
      we may have been called just after an opaque window move, so even though
      the old coords match the new ones no ConfigureNotify has been sent yet.
      There are likely other times when this will be relevant as well.
    */
    if (! client.state.moving) send_event = True;
  }

  if (send_event) {
    // if moving, the update and event will occur when the move finishes
    client.rect.setPos(frame.rect.left() + frame.margin.left,
                       frame.rect.top() + frame.margin.top);

    XEvent event;
    event.type = ConfigureNotify;

    event.xconfigure.display = blackbox->XDisplay();
    event.xconfigure.event = client.window;
    event.xconfigure.window = client.window;
    event.xconfigure.x = client.rect.x();
    event.xconfigure.y = client.rect.y();
    event.xconfigure.width = client.rect.width();
    event.xconfigure.height = client.rect.height();
    event.xconfigure.border_width = client.old_bw;
    event.xconfigure.above = frame.window;
    event.xconfigure.override_redirect = False;

    XSendEvent(blackbox->XDisplay(), client.window, False,
               StructureNotifyMask, &event);
    XFlush(blackbox->XDisplay());
  }
}


#ifdef SHAPE
void BlackboxWindow::configureShape(void) {
  XShapeCombineShape(blackbox->XDisplay(), frame.window, ShapeBounding,
                     frame.margin.left - frame.border_w,
                     frame.margin.top - frame.border_w,
                     client.window, ShapeBounding, ShapeSet);

  int num = 0;
  XRectangle xrect[2];

  if (client.decorations & Decor_Titlebar) {
    xrect[0].x = xrect[0].y = -frame.border_w;
    xrect[0].width = frame.rect.width();
    xrect[0].height = frame.title_h + (frame.border_w * 2);
    ++num;
  }

  if (client.decorations & Decor_Handle) {
    xrect[1].x = -frame.border_w;
    xrect[1].y = frame.rect.height() - frame.margin.bottom +
                 frame.mwm_border_w - frame.border_w;
    xrect[1].width = frame.rect.width();
    xrect[1].height = frame.handle_h + (frame.border_w * 2);
    ++num;
  }

  XShapeCombineRectangles(blackbox->XDisplay(), frame.window,
                          ShapeBounding, 0, 0, xrect, num,
                          ShapeUnion, Unsorted);
}
#endif // SHAPE


bool BlackboxWindow::setInputFocus(void) {
  if (! isVisible()) return False;
  if (client.state.focused) return True;

  // do not give focus to a window that is about to close
  if (! validateClient()) return False;

  if (! frame.rect.intersects(screen->screenInfo().rect())) {
    // client is outside the screen, move it to the center
    configure((screen->screenInfo().width() - frame.rect.width()) / 2,
              (screen->screenInfo().height() - frame.rect.height()) / 2,
              frame.rect.width(), frame.rect.height());
  }

  if (! client.transientList.empty()) {
    // transfer focus to any modal transients
    BlackboxWindowList::iterator it, end = client.transientList.end();
    for (it = client.transientList.begin(); it != end; ++it)
      if ((*it)->client.state.modal) return (*it)->setInputFocus();
  }

  bool ret = True;
  switch (client.focus_mode) {
  case F_Passive:
  case F_LocallyActive:
    XSetInputFocus(blackbox->XDisplay(), client.window,
                   RevertToPointerRoot, CurrentTime);
    blackbox->setFocusedWindow(this);
    break;

  case F_GloballyActive:
  case F_NoInput:
    /*
     * we could set the focus to none, since the window doesn't accept focus,
     * but we shouldn't set focus to nothing since this would surely make
     * someone angry
     */
    ret = False;
    break;
  }

  if (client.state.send_focus_message) {
    XEvent ce;
    ce.xclient.type = ClientMessage;
    ce.xclient.message_type = blackbox->getWMProtocolsAtom();
    ce.xclient.display = blackbox->XDisplay();
    ce.xclient.window = client.window;
    ce.xclient.format = 32;
    ce.xclient.data.l[0] = blackbox->getWMTakeFocusAtom();
    ce.xclient.data.l[1] = blackbox->getLastTime();
    ce.xclient.data.l[2] = 0l;
    ce.xclient.data.l[3] = 0l;
    ce.xclient.data.l[4] = 0l;
    XSendEvent(blackbox->XDisplay(), client.window, False,
               NoEventMask, &ce);
    XFlush(blackbox->XDisplay());
  }

  return ret;
}


void BlackboxWindow::iconify(void) {
  // walk up to the topmost transient_for that is not iconified
  if (isTransient() &&
      client.transient_for != (BlackboxWindow *) ~0ul &&
      ! client.transient_for->isIconic()) {

    client.transient_for->iconify();
    return;
  }

  if (client.state.iconic) return;

  /*
   * unmap the frame window first, so when all the transients are
   * unmapped, we don't get an enter event in sloppy focus mode
   */
  XUnmapWindow(blackbox->XDisplay(), frame.window);
  client.state.iconic = True;

  if (windowmenu) windowmenu->hide();

  setState(IconicState);

  // iconify all transients first
  if (! client.transientList.empty()) {
    std::for_each(client.transientList.begin(), client.transientList.end(),
                  std::mem_fun(&BlackboxWindow::iconify));
  }

  /*
   * remove the window from the workspace and add it to the screen's
   * icons *AFTER* we have process all transients.  since we always
   * iconify transients, it's pointless to have focus reverted to one
   * of them (since they are above their transient_for) for a split
   * second
   */
  screen->iconifyWindow(this);

  /*
   * we don't want this XUnmapWindow call to generate an UnmapNotify event, so
   * we need to clear the event mask on client.window for a split second.
   * HOWEVER, since X11 is asynchronous, the window could be destroyed in that
   * split second, leaving us with a ghost window... so, we need to do this
   * while the X server is grabbed
   */
  unsigned long event_mask = PropertyChangeMask | FocusChangeMask |
                             StructureNotifyMask;
  XGrabServer(blackbox->XDisplay());
  XSelectInput(blackbox->XDisplay(), client.window,
               event_mask & ~StructureNotifyMask);
  XUnmapWindow(blackbox->XDisplay(), client.window);
  XSelectInput(blackbox->XDisplay(), client.window, event_mask);
  XUngrabServer(blackbox->XDisplay());
}


void BlackboxWindow::show(void) {
  client.current_state = (client.state.shaded) ? IconicState : NormalState;
  client.state.iconic = False;
  setState(client.current_state);

  XMapWindow(blackbox->XDisplay(), client.window);
  XMapSubwindows(blackbox->XDisplay(), frame.window);
  XMapWindow(blackbox->XDisplay(), frame.window);

#ifdef DEBUG
  int real_x, real_y;
  Window child;
  XTranslateCoordinates(blackbox->XDisplay(), client.window,
                        screen->screenInfo().rootWindow(),
                        0, 0, &real_x, &real_y, &child);
  fprintf(stderr, "%s -- assumed: (%d, %d), real: (%d, %d)\n", getTitle(),
          client.rect.left(), client.rect.top(), real_x, real_y);
  assert(client.rect.left() == real_x && client.rect.top() == real_y);
#endif
}


void BlackboxWindow::deiconify(bool reassoc, bool raise) {
  if (client.state.iconic || reassoc)
    screen->reassociateWindow(this, bt::BSENTINEL);
  else if (client.workspace != screen->getCurrentWorkspaceID())
    return;

  show();

  // reassociate and deiconify all transients
  if (reassoc && ! client.transientList.empty()) {
    BlackboxWindowList::iterator it, end = client.transientList.end();
    for (it = client.transientList.begin(); it != end; ++it)
      (*it)->deiconify(True, False);
  }

  if (raise)
    screen->raiseWindow(this);
}


void BlackboxWindow::close(void) {
  XEvent ce;
  ce.xclient.type = ClientMessage;
  ce.xclient.message_type = blackbox->getWMProtocolsAtom();
  ce.xclient.display = blackbox->XDisplay();
  ce.xclient.window = client.window;
  ce.xclient.format = 32;
  ce.xclient.data.l[0] = blackbox->getWMDeleteAtom();
  ce.xclient.data.l[1] = CurrentTime;
  ce.xclient.data.l[2] = 0l;
  ce.xclient.data.l[3] = 0l;
  ce.xclient.data.l[4] = 0l;
  XSendEvent(blackbox->XDisplay(), client.window, False, NoEventMask, &ce);
  XFlush(blackbox->XDisplay());
}


void BlackboxWindow::withdraw(void) {
  setState(client.current_state);

  client.state.iconic = False;

  XUnmapWindow(blackbox->XDisplay(), frame.window);

  XGrabServer(blackbox->XDisplay());

  unsigned long event_mask = PropertyChangeMask | FocusChangeMask |
                             StructureNotifyMask;
  XSelectInput(blackbox->XDisplay(), client.window,
               event_mask & ~StructureNotifyMask);
  XUnmapWindow(blackbox->XDisplay(), client.window);
  XSelectInput(blackbox->XDisplay(), client.window, event_mask);

  XUngrabServer(blackbox->XDisplay());

  if (windowmenu) windowmenu->hide();
}


void BlackboxWindow::maximize(unsigned int button) {
  // handle case where menu is open then the max button is used instead
  if (windowmenu && windowmenu->isVisible()) windowmenu->hide();

  if (client.state.maximized) {
    client.state.maximized = 0;

    /*
      when a resize is begun, maximize(0) is called to clear any maximization
      flags currently set.  Otherwise it still thinks it is maximized.
      so we do not need to call configure() because resizing will handle it
    */
    if (! client.state.resizing)
      configure(client.premax.x(), client.premax.y(),
                client.premax.width(), client.premax.height());

    redrawAllButtons(); // in case it is not called in configure()
    setState(client.current_state);
    return;
  }

  const bt::Rect &screen_area = screen->availableArea();
  frame.changing = screen_area;
  client.premax = frame.rect;

  switch(button) {
  case 1:
    break;

  case 2:
    frame.changing.setX(client.premax.x());
    frame.changing.setWidth(client.premax.width());
    break;

  case 3:
    frame.changing.setY(client.premax.y());
    frame.changing.setHeight(client.premax.height());
    break;

  default:
    assert(0);
  }

  constrain(TopLeft);

  if (client.state.shaded)
    client.state.shaded = False;

  client.state.maximized = button;

  configure(frame.changing.x(), frame.changing.y(),
            frame.changing.width(), frame.changing.height());
  redrawAllButtons(); // in case it is not called in configure()
  setState(client.current_state);
}


// re-maximizes the window to take into account availableArea changes
void BlackboxWindow::remaximize(void) {
  if (client.state.shaded)
    return;

  bt::Rect tmp = client.premax;
  unsigned int button = client.state.maximized;
  client.state.maximized = 0; // trick maximize() into working
  maximize(button);
  client.premax = tmp;
}


void BlackboxWindow::setWorkspace(unsigned int n) {
  client.workspace = n;
}


void BlackboxWindow::shade(void) {
  if (client.state.shaded) {
    client.state.shaded = False;

    if (client.state.maximized) {
      remaximize();
    } else {
      XResizeWindow(blackbox->XDisplay(), frame.window,
                    frame.inside_w, frame.inside_h);
      // set the frame rect to the normal size
      frame.rect.setHeight(client.rect.height() + frame.margin.top +
                           frame.margin.bottom);
    }

    setState(NormalState);
  } else {
    if (! (client.decorations & Decor_Titlebar))
      return; // can't shade it without a titlebar!

    XResizeWindow(blackbox->XDisplay(), frame.window,
                  frame.inside_w, frame.title_h);
    client.state.shaded = True;

    setState(IconicState);

    // set the frame rect to the shaded size
    frame.rect.setHeight(frame.title_h + (frame.border_w * 2));
  }
}


void BlackboxWindow::redrawWindowFrame(void) const {
  if (client.decorations & Decor_Titlebar) {
    redrawTitle();
    redrawLabel();
    redrawAllButtons();
  }

  if (client.decorations & Decor_Handle) {
    redrawHandle();
    redrawGrips();
  }

  if (client.decorations & Decor_Border) {
    if (client.state.focused)
      XSetWindowBorder(blackbox->XDisplay(),
                       frame.plate, frame.fborder_pixel);
    else
      XSetWindowBorder(blackbox->XDisplay(),
                       frame.plate, frame.uborder_pixel);
  }
}


void BlackboxWindow::setFocusFlag(bool focus) {
  if (focus && ! isVisible())
    return;

  client.state.focused = focus;

  redrawWindowFrame();

  if (client.state.focused)
    blackbox->setFocusedWindow(this);
}


void BlackboxWindow::installColormap(bool install) {
  int i = 0, ncmap = 0;
  Colormap *cmaps = XListInstalledColormaps(blackbox->XDisplay(),
                                            client.window, &ncmap);
  if (cmaps) {
    XWindowAttributes wattrib;
    if (XGetWindowAttributes(blackbox->XDisplay(),
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
          XInstallColormap(blackbox->XDisplay(), wattrib.colormap);
      } else {
        // uninstall the window's colormap
        for (i = 0; i < ncmap; i++) {
          if (*(cmaps + i) == wattrib.colormap)
            // we found the colormap to uninstall
            XUninstallColormap(blackbox->XDisplay(), wattrib.colormap);
        }
      }
    }

    XFree(cmaps);
  }
}


void BlackboxWindow::setState(unsigned long new_state, bool closing) {
  client.current_state = new_state;

  unsigned long state[2];
  state[0] = client.current_state;
  state[1] = None;
  XChangeProperty(blackbox->XDisplay(), client.window,
                  blackbox->getWMStateAtom(), blackbox->getWMStateAtom(), 32,
                  PropModeReplace, (unsigned char *) state, 2);

  const bt::Netwm& netwm = blackbox->netwm();

  if (closing) {
    netwm.removeProperty(client.window, netwm.wmDesktop());
    netwm.removeProperty(client.window, netwm.wmState());
    netwm.removeProperty(client.window, netwm.wmAllowedActions());
    netwm.removeProperty(client.window, netwm.wmVisibleName());
    netwm.removeProperty(client.window, netwm.wmVisibleIconName());
    return;
  }

  if (client.state.iconic)
    netwm.removeProperty(client.window, netwm.wmDesktop());
  else
    blackbox->netwm().setWMDesktop(client.window, client.workspace);

  bt::Netwm::AtomList atoms;
  if (client.state.modal)
    atoms.push_back(netwm.wmStateModal());

  if (client.state.maximized == 0) {
    /* do nothing */
  } else if (client.state.maximized == 1) {
    atoms.push_back(netwm.wmStateMaximizedVert());
    atoms.push_back(netwm.wmStateMaximizedHorz());
  } else if (client.state.maximized == 2) {
    atoms.push_back(netwm.wmStateMaximizedVert());
  }  else if (client.state.maximized == 3) {
    atoms.push_back(netwm.wmStateMaximizedHorz());
  }

  if (client.state.shaded)
    atoms.push_back(netwm.wmStateShaded());

  if (client.state.skip == SKIP_NONE) {
    /* do nothing */
  } else if (client.state.skip == SKIP_BOTH) {
    atoms.push_back(netwm.wmStateSkipTaskbar());
    atoms.push_back(netwm.wmStateSkipPager());
  } else if (client.state.skip == SKIP_TASKBAR) {
    atoms.push_back(netwm.wmStateSkipTaskbar());
  }  else if (client.state.skip == SKIP_PAGER) {
    atoms.push_back(netwm.wmStateSkipPager());
  }

  if (client.state.iconic)
    atoms.push_back(netwm.wmStateHidden());

  if (client.state.layer == LAYER_NORMAL)
    /* do nothing */;
  else if (client.state.layer == LAYER_FULLSCREEN)
    atoms.push_back(netwm.wmStateFullscreen());
  else if (client.state.layer == LAYER_ABOVE)
    atoms.push_back(netwm.wmStateAbove());
  else if (client.state.layer == LAYER_BELOW)
    atoms.push_back(netwm.wmStateBelow());

  if (atoms.empty())
    netwm.removeProperty(client.window, netwm.wmState());
  else
    netwm.setWMState(client.window, atoms);

  atoms.clear();

  if (! client.state.iconic) {
    atoms.push_back(netwm.wmActionChangeDesktop());

    if (client.functions & Func_Move)
      atoms.push_back(netwm.wmActionMove());

    if (client.functions & Func_Iconify)
      atoms.push_back(netwm.wmActionMinimize());

    if (client.functions & Func_Resize) {
      atoms.push_back(netwm.wmActionResize());
      atoms.push_back(netwm.wmActionMaximizeHorz());
      atoms.push_back(netwm.wmActionMaximizeVert());
      atoms.push_back(netwm.wmActionFullscreen());
    }

    if (client.decorations & Decor_Titlebar)
      atoms.push_back(netwm.wmActionShade());
  }

  if (client.functions & Func_Close)
    atoms.push_back(netwm.wmActionClose());

  netwm.setWMAllowedActions(client.window, atoms);
}


bool BlackboxWindow::getState(void) {
  client.current_state = 0;

  Atom atom_return;
  bool ret = False;
  int foo;
  unsigned long *state, ulfoo, nitems;

  if ((XGetWindowProperty(blackbox->XDisplay(), client.window,
                          blackbox->getWMStateAtom(),
                          0l, 2l, False, blackbox->getWMStateAtom(),
                          &atom_return, &foo, &nitems, &ulfoo,
                          (unsigned char **) &state) != Success) ||
      (! state)) {
    return False;
  }

  if (nitems >= 1) {
    client.current_state = static_cast<unsigned long>(state[0]);

    ret = True;
  }

  XFree((void *) state);

  return ret;
}


/*
 * Positions the bt::Rect r according the the client window position and
 * window gravity.
 */
void BlackboxWindow::applyGravity(bt::Rect &r) {
  // apply horizontal window gravity
  switch (client.win_gravity) {
  default:
  case NorthWestGravity:
  case SouthWestGravity:
  case WestGravity:
    r.setX(client.rect.x());
    break;

  case NorthGravity:
  case SouthGravity:
  case CenterGravity:
    r.setX(client.rect.x() - (frame.margin.left + frame.margin.right) / 2);
    break;

  case NorthEastGravity:
  case SouthEastGravity:
  case EastGravity:
    r.setX(client.rect.x() - (frame.margin.left + frame.margin.right) + 2);
    break;

  case ForgetGravity:
  case StaticGravity:
    r.setX(client.rect.x() - frame.margin.left);
    break;
  }

  // apply vertical window gravity
  switch (client.win_gravity) {
  default:
  case NorthWestGravity:
  case NorthEastGravity:
  case NorthGravity:
    r.setY(client.rect.y());
    break;

  case CenterGravity:
  case EastGravity:
  case WestGravity:
    r.setY(client.rect.y() - ((frame.margin.top + frame.margin.bottom) / 2));
    break;

  case SouthWestGravity:
  case SouthEastGravity:
  case SouthGravity:
    r.setY(client.rect.y() - (frame.margin.bottom + frame.margin.top) + 2);
    break;

  case ForgetGravity:
  case StaticGravity:
    r.setY(client.rect.y() - frame.margin.top);
    break;
  }
}


/*
 * The reverse of the applyGravity function.
 *
 * Positions the bt::Rect r according to the frame window position and
 * window gravity.
 */
void BlackboxWindow::restoreGravity(bt::Rect &r) {
  // restore horizontal window gravity
  switch (client.win_gravity) {
  default:
  case NorthWestGravity:
  case SouthWestGravity:
  case WestGravity:
    r.setX(frame.rect.x());
    break;

  case NorthGravity:
  case SouthGravity:
  case CenterGravity:
    r.setX(frame.rect.x() + (frame.margin.left + frame.margin.right) / 2);
    break;

  case NorthEastGravity:
  case SouthEastGravity:
  case EastGravity:
    r.setX(frame.rect.x() + (frame.margin.left + frame.margin.right) - 2);
    break;

  case ForgetGravity:
  case StaticGravity:
    r.setX(frame.rect.x() + frame.margin.left);
    break;
  }

  // restore vertical window gravity
  switch (client.win_gravity) {
  default:
  case NorthWestGravity:
  case NorthEastGravity:
  case NorthGravity:
    r.setY(frame.rect.y());
    break;

  case CenterGravity:
  case EastGravity:
  case WestGravity:
    r.setY(frame.rect.y() + (frame.margin.top + frame.margin.bottom) / 2);
    break;

  case SouthWestGravity:
  case SouthEastGravity:
  case SouthGravity:
    r.setY(frame.rect.y() + (frame.margin.top + frame.margin.bottom) - 2);
    break;

  case ForgetGravity:
  case StaticGravity:
    r.setY(frame.rect.y() + frame.margin.top);
    break;
  }
}


void BlackboxWindow::redrawTitle(void) const {
  const WindowStyle * const style = screen->getWindowStyle();
  bt::Rect u(0, 0, frame.inside_w, frame.title_h);
  bt::drawTexture(screen->screenNumber(),
                  (client.state.focused ? style->t_focus : style->t_unfocus),
                  frame.title, u, u,
                  (client.state.focused ? frame.ftitle : frame.utitle));
}


void BlackboxWindow::redrawLabel(void) const {
  const WindowStyle * const style = screen->getWindowStyle();

  bt::Rect u(0, 0, frame.label_w, frame.label_h);
  Pixmap p = (client.state.focused ? frame.flabel : frame.ulabel);
  if (p == ParentRelative) {
    bt::Rect t(-(frame.bevel_w + 1 +
                 ((client.decorations & Decor_Iconify) ?
                  frame.button_w + frame.bevel_w + 1 : 0)),
               -(frame.bevel_w), frame.inside_w, frame.title_h);
    bt::drawTexture(screen->screenNumber(),
                    (client.state.focused ? style->t_focus : style->t_unfocus),
                    frame.label, t, u,
                    (client.state.focused ? frame.ftitle : frame.utitle));
  } else {
    bt::drawTexture(screen->screenNumber(),
                    (client.state.focused ? style->l_focus : style->l_unfocus),
                    frame.label, u, u, p);
  }

  bt::Pen pen(screen->screenNumber(),
              ((client.state.focused) ?
               style->l_text_focus : style->l_text_unfocus));
  u.setCoords(u.left()  + frame.bevel_w, u.top()    + frame.bevel_w,
              u.right() - frame.bevel_w, u.bottom() - frame.bevel_w);
  bt::drawText(style->font, pen, frame.label, u,
               style->alignment, client.title);
}


void BlackboxWindow::redrawAllButtons(void) const {
  if (frame.iconify_button) redrawIconifyButton(False);
  if (frame.maximize_button) redrawMaximizeButton(client.state.maximized);
  if (frame.close_button) redrawCloseButton(False);
}


void BlackboxWindow::redrawIconifyButton(bool pressed) const {
  const WindowStyle * const style = screen->getWindowStyle();

  bt::Rect u(0, 0, frame.button_w, frame.button_w);
  Pixmap p = (pressed ? frame.pbutton :
              (client.state.focused ? frame.fbutton : frame.ubutton));
  if (p == ParentRelative) {
    bt::Rect t(-(frame.button_w + frame.bevel_w + 1),
               -(frame.bevel_w + 1), frame.inside_w, frame.title_h);
    bt::drawTexture(screen->screenNumber(),
                    (client.state.focused ? style->t_focus : style->t_unfocus),
                    frame.iconify_button, t, u,
                    (client.state.focused ? frame.ftitle : frame.utitle));
  } else {
    bt::drawTexture(screen->screenNumber(),
                    (pressed ? style->b_pressed :
                     (client.state.focused ? style->b_focus :
                      style->b_unfocus)),
                    frame.iconify_button, u, u, p);
  }

  bt::Pen pen(screen->screenNumber(),
              (client.state.focused ?
               style->b_pic_focus : style->b_pic_unfocus));
  u.setCoords(u.left()  + 2, u.bottom() - 3,
              u.right() - 3, u.bottom() - 2);

  XFillRectangle(blackbox->XDisplay(), frame.iconify_button, pen.gc(),
                 u.x(), u.y(), u.width(), u.height());
}


void BlackboxWindow::redrawMaximizeButton(bool pressed) const {
  const WindowStyle * const style = screen->getWindowStyle();

  bt::Rect u(0, 0, frame.button_w, frame.button_w);
  Pixmap p = (pressed ? frame.pbutton :
              (client.state.focused ? frame.fbutton : frame.ubutton));
  if (p == ParentRelative) {
    bt::Rect t(-(frame.inside_w -
                 ((frame.button_w + frame.bevel_w + 1) *
                  ((client.decorations & Decor_Close) ? 2 : 1))),
               -(frame.bevel_w + 1), frame.inside_w, frame.title_h);
    bt::drawTexture(screen->screenNumber(),
                    (client.state.focused ? style->t_focus : style->t_unfocus),
                    frame.maximize_button, t, u,
                    (client.state.focused ? frame.ftitle : frame.utitle));
  } else {
    bt::drawTexture(screen->screenNumber(),
                    (pressed ? style->b_pressed :
                     (client.state.focused ? style->b_focus :
                      style->b_unfocus)),
                    frame.maximize_button, u, u, p);
  }

  bt::Pen pen(screen->screenNumber(),
              (client.state.focused) ?
              style->b_pic_focus : style->b_pic_unfocus);
  u.setCoords(u.left()  + 2, u.top()    + 2,
              u.right() - 3, u.bottom() - 3);

  XDrawRectangle(blackbox->XDisplay(), frame.maximize_button, pen.gc(),
                 u.x(), u.y(), u.width(), u.height());
  XDrawLine(blackbox->XDisplay(), frame.maximize_button, pen.gc(),
            u.left(), u.top() + 1, u.right(), u.top() + 1);
}


void BlackboxWindow::redrawCloseButton(bool pressed) const {
  const WindowStyle * const style = screen->getWindowStyle();

  bt::Rect u(0, 0, frame.button_w, frame.button_w);
  Pixmap p = (pressed ? frame.pbutton :
              (client.state.focused ? frame.fbutton : frame.ubutton));
  if (p == ParentRelative) {
    bt::Rect t(-(frame.inside_w - frame.button_w - frame.bevel_w - 1),
               -(frame.bevel_w + 1), frame.inside_w, frame.title_h);
    bt::drawTexture(screen->screenNumber(),
                    (client.state.focused ? style->t_focus : style->t_unfocus),
                    frame.close_button, t, u,
                    (client.state.focused ? frame.ftitle : frame.utitle));
  } else {
    bt::drawTexture(screen->screenNumber(),
                    (pressed ? style->b_pressed :
                     (client.state.focused ? style->b_focus :
                      style->b_unfocus)),
                    frame.close_button, u, u, p);
  }

  bt::Pen pen(screen->screenNumber(),
              (client.state.focused ?
               style->b_pic_focus : style->b_pic_unfocus));
  u.setCoords(u.left()  + 2, u.top()    + 2,
              u.right() - 2, u.bottom() - 2);
  XDrawLine(blackbox->XDisplay(), frame.close_button, pen.gc(),
            u.left(), u.top(), u.right(), u.bottom());
  XDrawLine(blackbox->XDisplay(), frame.close_button, pen.gc(),
            u.left(), u.bottom(), u.right(), u.top());
}


void BlackboxWindow::redrawHandle(void) const {
  const WindowStyle * const style = screen->getWindowStyle();
  bt::Rect u(0, 0, frame.inside_w, frame.handle_h);
  bt::drawTexture(screen->screenNumber(),
                  (client.state.focused ? style->h_focus : style->h_unfocus),
                  frame.handle, u, u,
                  (client.state.focused ? frame.fhandle : frame.uhandle));
}


void BlackboxWindow::redrawGrips(void) const {
  const WindowStyle * const style = screen->getWindowStyle();

  bt::Rect u(0, 0, frame.grip_w, frame.handle_h);
  Pixmap p = (client.state.focused ? frame.fgrip : frame.ugrip);
  if (p == ParentRelative) {
    bt::Rect t(0, 0, frame.inside_w, frame.handle_h);
    bt::drawTexture(screen->screenNumber(),
                    (client.state.focused ? style->h_focus : style->h_unfocus),
                    frame.right_grip, t, u, p);

    t.setPos(-(frame.inside_w - frame.grip_w), 0);
    bt::drawTexture(screen->screenNumber(),
                    (client.state.focused ? style->h_focus : style->h_unfocus),
                    frame.right_grip, t, u, p);
  } else {
    bt::drawTexture(screen->screenNumber(),
                    (client.state.focused ? style->g_focus : style->g_unfocus),
                    frame.left_grip, u, u, p);

    bt::drawTexture(screen->screenNumber(),
                    (client.state.focused ? style->g_focus : style->g_unfocus),
                    frame.right_grip, u, u, p);
  }
}


void BlackboxWindow::clientMessageEvent(const XClientMessageEvent* const ce) {
  if (ce->format != 32) return;

  const bt::Netwm& netwm = blackbox->netwm();

  if (ce->message_type == blackbox->getWMChangeStateAtom()) {
    if (ce->data.l[0] == IconicState)
      iconify();
    if (ce->data.l[0] == NormalState)
      deiconify();
  } else if (ce->message_type == netwm.activeWindow()) {
    if (client.state.iconic)
      deiconify(False, False);

    if (client.workspace != screen->getCurrentWorkspaceID())
      screen->changeWorkspaceID(client.workspace);

    if (setInputFocus()) {
      screen->raiseWindow(this);
      installColormap(True);
    }
  } else if (ce->message_type == netwm.closeWindow()) {
    close();
  } else if (ce->message_type == netwm.moveresizeWindow()) {
    XConfigureRequestEvent request;
    request.window = ce->window;
    request.x = ce->data.l[1];
    request.y = ce->data.l[2];
    request.width = ce->data.l[3];
    request.height = ce->data.l[4];
    request.value_mask = CWX | CWY | CWWidth | CWHeight;

    const int old_gravity = client.win_gravity;
    if (ce->data.l[0] != 0)
      client.win_gravity = ce->data.l[0];

    configureRequestEvent(&request);

    client.win_gravity = old_gravity;
  } else if (ce->message_type == netwm.wmDesktop()) {
    const unsigned int desktop = ce->data.l[0];
    if (desktop != 0xFFFFFFFF && desktop != client.workspace) {
      withdraw();
      screen->reassociateWindow(this, desktop);
    }
  } else if (ce->message_type == netwm.wmState()) {
    Atom action = ce->data.l[0],
      first = ce->data.l[1],
      second = ce->data.l[2];

    int max_horz = 0, max_vert = 0,
      skip_taskbar = 0, skip_pager = 0;

    if (first == netwm.wmStateModal() || second == netwm.wmStateModal()) {
      if ((action == netwm.wmStateAdd() ||
           (action == netwm.wmStateToggle() && ! client.state.modal)) &&
          isTransient())
        client.state.modal = True;
      else
        client.state.modal = False;
    }
    if (first == netwm.wmStateMaximizedHorz() ||
        second == netwm.wmStateMaximizedHorz()) {
      if (action == netwm.wmStateAdd() ||
          (action == netwm.wmStateToggle() &&
           ! (client.state.maximized == 1 || client.state.maximized == 3)))
        max_horz = 1;
      else
        max_horz = -1;
    }
    if (first == netwm.wmStateMaximizedVert() ||
        second == netwm.wmStateMaximizedVert()) {
      if (action == netwm.wmStateAdd() ||
          (action == netwm.wmStateToggle() &&
           ! (client.state.maximized == 1 || client.state.maximized == 2)))
        max_vert = 1;
      else
        max_vert = -1;
    }
    if (first == netwm.wmStateShaded() ||
        second == netwm.wmStateShaded()) {
      if ((action == netwm.wmStateRemove() && client.state.shaded) ||
          action == netwm.wmStateToggle() ||
          (action == netwm.wmStateAdd() && ! client.state.shaded))
        shade();
    }
    if (first == netwm.wmStateSkipTaskbar() ||
        second == netwm.wmStateSkipTaskbar()) {
      if (action == netwm.wmStateAdd() ||
          (action == netwm.wmStateToggle() &&
           ! (client.state.skip == SKIP_TASKBAR ||
              client.state.skip == SKIP_BOTH)))
        skip_taskbar = 1;
      else
        skip_taskbar = -1;
    }
    if (first == netwm.wmStateSkipPager() ||
        second == netwm.wmStateSkipPager()) {
      if (action == netwm.wmStateAdd() ||
          (action == netwm.wmStateToggle() &&
           ! (client.state.skip == SKIP_PAGER ||
              client.state.skip == SKIP_BOTH)))
        skip_pager = 1;
      else
        skip_pager = -1;
    }
    if (first == netwm.wmStateHidden() ||
               second == netwm.wmStateHidden()) {
      /* ignore this message */
    }
    if (first == netwm.wmStateFullscreen() ||
        second == netwm.wmStateFullscreen()) {
      if (action == netwm.wmStateAdd() ||
          (action == netwm.wmStateToggle() &&
           ! client.state.fullscreen)) {
        client.state.fullscreen = True;
        client.state.layer = LAYER_FULLSCREEN;
        reconfigure();
      } else if (action == netwm.wmStateToggle() ||
                 action == netwm.wmStateRemove()) {
        client.state.fullscreen = False;
        client.state.layer = LAYER_NORMAL;
        reconfigure();
      }
    }
    if (first == netwm.wmStateAbove() ||
        second == netwm.wmStateAbove()) {
      if (action == netwm.wmStateAdd() ||
          (action == netwm.wmStateToggle() &&
           client.state.layer != LAYER_ABOVE)) {
        client.state.layer = LAYER_ABOVE;
      } else if (action == netwm.wmStateToggle() ||
                 action == netwm.wmStateRemove()) {
        client.state.layer = LAYER_NORMAL;
      }
    }
    if (first == netwm.wmStateBelow() ||
        second == netwm.wmStateBelow()) {
      if (action == netwm.wmStateAdd() ||
          (action == netwm.wmStateToggle() &&
           client.state.layer != LAYER_BELOW)) {
        client.state.layer = LAYER_BELOW;
      } else if (action == netwm.wmStateToggle() ||
                 action == netwm.wmStateRemove()) {
        client.state.layer = LAYER_NORMAL;
      }
    }

    if (max_horz != 0 || max_vert != 0) {
      if (client.state.maximized)
        maximize(0);
      unsigned int button = 0;
      if (max_horz == 1 && max_vert != 1)
        button = 3;
      else if (max_vert == 1 && max_horz != 1)
        button = 2;
      else if (max_vert == 1 && max_horz == 1)
        button = 1;
      if (button)
        maximize(button);
    }

    if (skip_taskbar != 0 || skip_pager != 0) {
      if (skip_taskbar == 1 && skip_pager != 1)
        client.state.skip = SKIP_TASKBAR;
      else if (skip_pager == 1 && skip_taskbar != 1)
        client.state.skip = SKIP_PAGER;
      else if (skip_pager == 1 && skip_taskbar == 1)
        client.state.skip = SKIP_BOTH;
      else
        client.state.skip = SKIP_NONE;
    }

    setState(client.current_state);
  } else if (ce->message_type == netwm.wmStrut()) {
    if (! client.strut) {
      client.strut = new bt::Netwm::Strut;
      screen->addStrut(client.strut);
    }

    netwm.readWMStrut(client.window, client.strut);
    if (client.strut->left || client.strut->right ||
        client.strut->top || client.strut->bottom) {
      screen->updateStrut();
    } else {
      screen->removeStrut(client.strut);
      delete client.strut;
    }
  }
}


void BlackboxWindow::mapRequestEvent(const XMapRequestEvent* const re) {
  if (re->window != client.window)
    return;

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::mapRequestEvent() for 0x%lx\n",
          client.window);
#endif // DEBUG

  switch (client.current_state) {
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
    show();
    screen->raiseWindow(this);
    if (! blackbox->isStartup() && (isTransient() || screen->doFocusNew())) {
      XSync(blackbox->XDisplay(), False); // make sure the frame is mapped..
      setInputFocus();
    }
    break;
  }
}


void BlackboxWindow::unmapNotifyEvent(const XUnmapEvent *ue) {
  if (ue->window != client.window)
    return;

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::unmapNotifyEvent() for 0x%lx\n",
          client.window);
#endif // DEBUG

  screen->releaseWindow(this, False);
}


void BlackboxWindow::destroyNotifyEvent(const XDestroyWindowEvent *de) {
  if (de->window != client.window)
    return;

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::destroyNotifyEvent() for 0x%lx\n",
          client.window);
#endif // DEBUG

  screen->releaseWindow(this, False);
}


void BlackboxWindow::reparentNotifyEvent(const XReparentEvent *re) {
  if (re->window != client.window || re->parent == frame.plate)
    return;

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::reparentNotifyEvent(): reparent 0x%lx to "
          "0x%lx.\n", client.window, re->parent);
#endif // DEBUG

  XEvent ev;
  ev.xreparent = *re;
  XPutBackEvent(blackbox->XDisplay(), &ev);
  screen->releaseWindow(this, True);
}


void BlackboxWindow::propertyNotifyEvent(const XPropertyEvent *pe) {
  if (pe->state == PropertyDelete || ! validateClient())
    return;

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::propertyNotifyEvent(): for 0x%lx\n",
          client.window);
#endif

  switch(pe->atom) {
  case XA_WM_CLASS:
  case XA_WM_CLIENT_MACHINE:
  case XA_WM_COMMAND:
    break;

  case XA_WM_TRANSIENT_FOR: {
    // determine if this is a transient window
    getTransientInfo();

    // adjust the window decorations based on transience
    if (isTransient()) {
      client.decorations &= ~(Decor_Maximize | Decor_Handle);
      client.functions &= ~Func_Maximize;
    }

    reconfigure();
  }
    break;

  case XA_WM_HINTS:
    getWMHints();
    break;

  case XA_WM_ICON_NAME:
    getWMIconName();
    if (client.state.iconic) screen->propagateWindowName(this);
    break;

  case XA_WM_NAME:
    getWMName();

    if (client.decorations & Decor_Titlebar)
      redrawLabel();

    screen->propagateWindowName(this);
    break;

  case XA_WM_NORMAL_HINTS: {
    getWMNormalHints();

    if ((client.normal_hint_flags & PMinSize) &&
        (client.normal_hint_flags & PMaxSize)) {
      // the window now can/can't resize itself, so the buttons need to be
      // regrabbed.
      ungrabButtons();
      if (client.max_width <= client.min_width &&
          client.max_height <= client.min_height) {
        client.decorations &= ~(Decor_Maximize | Decor_Handle);
        client.functions &= ~(Func_Resize | Func_Maximize);
      } else {
        if (! isTransient()) {
          client.decorations |= Decor_Maximize | Decor_Handle;
          client.functions |= Func_Maximize;
        }
        client.functions |= Func_Resize;
      }
      grabButtons();
    }

    bt::Rect old_rect = frame.rect;

    upsize();

    if (old_rect != frame.rect)
      reconfigure();

    break;
  }

  default:
    if (pe->atom == blackbox->getWMProtocolsAtom()) {
      getWMProtocols();

      if (client.decorations & Decor_Close && !frame.close_button) {
        createCloseButton();
        if (client.decorations & Decor_Titlebar) {
          positionButtons(True);
          XMapSubwindows(blackbox->XDisplay(), frame.title);
        }
        if (windowmenu) windowmenu->reconfigure();
      }
    }

    break;
  }
}


void BlackboxWindow::exposeEvent(const XExposeEvent *ee) {
#ifdef DEBUG
  fprintf(stderr, "BlackboxWindow::exposeEvent() for 0x%lx\n", client.window);
#endif

  if (frame.title == ee->window)
    redrawTitle();
  else if (frame.label == ee->window)
    redrawLabel();
  else if (frame.close_button == ee->window)
    redrawCloseButton(False);
  else if (frame.maximize_button == ee->window)
    redrawMaximizeButton(client.state.maximized);
  else if (frame.iconify_button == ee->window)
    redrawIconifyButton(False);
  else if (frame.handle == ee->window)
    redrawHandle();
  else if (frame.left_grip == ee->window || frame.right_grip == ee->window)
    redrawGrips();
}


void BlackboxWindow::configureRequestEvent(const XConfigureRequestEvent *cr) {
  if (cr->window != client.window || client.state.iconic)
    return;

  if (cr->value_mask & CWBorderWidth)
    client.old_bw = cr->border_width;

  if (cr->value_mask & (CWX | CWY | CWWidth | CWHeight)) {
    bt::Rect req = frame.rect;

    if (cr->value_mask & (CWX | CWY)) {
      if (cr->value_mask & CWX)
        client.rect.setX(cr->x);
      if (cr->value_mask & CWY)
        client.rect.setY(cr->y);

      applyGravity(req);
    }

    if (cr->value_mask & CWWidth)
      req.setWidth(cr->width + frame.margin.left + frame.margin.right);

    if (cr->value_mask & CWHeight)
      req.setHeight(cr->height + frame.margin.top + frame.margin.bottom);

    configure(req.x(), req.y(), req.width(), req.height());
  }

  if (cr->value_mask & CWStackMode) {
    switch (cr->detail) {
    case Below:
    case BottomIf:
      screen->lowerWindow(this);
      break;

    case Above:
    case TopIf:
    default:
      screen->raiseWindow(this);
      break;
    }
  }
}


void BlackboxWindow::buttonPressEvent(const XButtonEvent * const be) {
#ifdef DEBUG
  fprintf(stderr, "BlackboxWindow::buttonPressEvent() for 0x%lx\n",
          client.window);
#endif

  if (frame.maximize_button == be->window && be->button < 4) {
    redrawMaximizeButton(True);
  } else if (be->button == 1 || (be->button == 3 && be->state == Mod1Mask)) {
    if (! client.state.focused)
      setInputFocus();

    if (frame.iconify_button == be->window) {
      redrawIconifyButton(True);
    } else if (frame.close_button == be->window) {
      redrawCloseButton(True);
    } else if (frame.plate == be->window) {
      if (windowmenu && windowmenu->isVisible()) windowmenu->hide();

      screen->raiseWindow(this);

      XAllowEvents(blackbox->XDisplay(), ReplayPointer, be->time);
    } else {
      if (frame.title == be->window || frame.label == be->window) {
        if (((be->time - lastButtonPressTime) <=
             blackbox->getDoubleClickInterval()) ||
            (be->state == ControlMask)) {
          lastButtonPressTime = 0;
          shade();
        } else {
          lastButtonPressTime = be->time;
        }
      }

      frame.grab_x = be->x_root - frame.rect.x() - frame.border_w;
      frame.grab_y = be->y_root - frame.rect.y() - frame.border_w;

      if (windowmenu && windowmenu->isVisible()) windowmenu->hide();

      screen->raiseWindow(this);
    }
  } else if (be->button == 2 && (be->window != frame.iconify_button) &&
             (be->window != frame.close_button)) {
    screen->lowerWindow(this);
  } else if (windowmenu && be->button == 3) {

    int mx = be->x_root;
    int my = be->y_root;

    if (frame.title == be->window || frame.label == be->window) {
      my = client.rect.top() - (frame.border_w + frame.mwm_border_w);
    } else if (frame.handle == be->window) {
      my = client.rect.bottom() + (frame.border_w * 2) + frame.mwm_border_w +
           frame.handle_h;
    }

    windowmenu->popup(mx, my);
  }
}


void BlackboxWindow::buttonReleaseEvent(const XButtonEvent * const re) {
#ifdef DEBUG
  fprintf(stderr, "BlackboxWindow::buttonReleaseEvent() for 0x%lx\n",
          client.window);
#endif

  if (re->window == frame.maximize_button && re->button < 4) {
    if ((re->x >= 0 && re->x <= static_cast<signed>(frame.button_w)) &&
        (re->y >= 0 && re->y <= static_cast<signed>(frame.button_w))) {
      maximize(re->button);
      screen->raiseWindow(this);
    } else {
      redrawMaximizeButton(client.state.maximized);
    }
  } else if (re->window == frame.iconify_button && re->button == 1) {
    if ((re->x >= 0 && re->x <= static_cast<signed>(frame.button_w)) &&
        (re->y >= 0 && re->y <= static_cast<signed>(frame.button_w))) {
      iconify();
    } else {
      redrawIconifyButton(False);
    }
  } else if (re->window == frame.close_button && re->button == 1) {
    if ((re->x >= 0 && re->x <= static_cast<signed>(frame.button_w)) &&
        (re->y >= 0 && re->y <= static_cast<signed>(frame.button_w)))
      close();
    redrawCloseButton(False);
  } else if (client.state.moving) {
    client.state.moving = False;

    if (! screen->doOpaqueMove()) {
      /* when drawing the rubber band, we need to make sure we only draw inside
       * the frame... frame.changing_* contain the new coords for the window,
       * so we need to subtract 1 from changing_w/changing_h every where we
       * draw the rubber band (for both moving and resizing)
       */
      XDrawRectangle(blackbox->XDisplay(), screen->screenInfo().rootWindow(),
                     screen->getOpGC(), frame.changing.x(), frame.changing.y(),
                     frame.changing.width() - 1, frame.changing.height() - 1);
      XUngrabServer(blackbox->XDisplay());

      configure(frame.changing.x(), frame.changing.y(),
                frame.changing.width(), frame.changing.height());
    } else {
      configure(frame.rect.x(), frame.rect.y(),
                frame.rect.width(), frame.rect.height());
    }
    screen->hideGeometry();
    XUngrabPointer(blackbox->XDisplay(), CurrentTime);
  } else if (client.state.resizing) {
    XDrawRectangle(blackbox->XDisplay(), screen->screenInfo().rootWindow(),
                   screen->getOpGC(), frame.changing.x(), frame.changing.y(),
                   frame.changing.width() - 1, frame.changing.height() - 1);
    XUngrabServer(blackbox->XDisplay());

    screen->hideGeometry();

    constrain((re->window == frame.left_grip) ? TopRight : TopLeft);

    // unset maximized state when resized after fully maximized
    if (client.state.maximized == 1)
      maximize(0);
    client.state.resizing = False;
    configure(frame.changing.x(), frame.changing.y(),
              frame.changing.width(), frame.changing.height());

    XUngrabPointer(blackbox->XDisplay(), CurrentTime);
  } else if (re->window == frame.window) {
    if (re->button == 2 && re->state == Mod1Mask)
      XUngrabPointer(blackbox->XDisplay(), CurrentTime);
  }
}


void BlackboxWindow::motionNotifyEvent(const XMotionEvent *me) {
#ifdef DEBUG
  fprintf(stderr, "BlackboxWindow::motionNotifyEvent() for 0x%lx\n",
          client.window);
#endif

  if (!client.state.resizing && me->state & Button1Mask && (client.functions & Func_Move) &&
      (frame.title == me->window || frame.label == me->window ||
       frame.handle == me->window || frame.window == me->window)) {
    if (! client.state.moving) {
      XGrabPointer(blackbox->XDisplay(), me->window, False,
                   Button1MotionMask | ButtonReleaseMask,
                   GrabModeAsync, GrabModeAsync,
                   None, blackbox->getMoveCursor(), CurrentTime);

      if (windowmenu && windowmenu->isVisible())
        windowmenu->hide();

      client.state.moving = True;

      if (! screen->doOpaqueMove()) {
        XGrabServer(blackbox->XDisplay());

        frame.changing = frame.rect;
        screen->showPosition(frame.changing.x(), frame.changing.y());

        XDrawRectangle(blackbox->XDisplay(), screen->screenInfo().rootWindow(),
                       screen->getOpGC(),
                       frame.changing.x(),
                       frame.changing.y(),
                       frame.changing.width() - 1,
                       frame.changing.height() - 1);
      }
    } else {
      int dx = me->x_root - frame.grab_x, dy = me->y_root - frame.grab_y;
      dx -= frame.border_w;
      dy -= frame.border_w;

      const int snap_distance = screen->getEdgeSnapThreshold();

      if (snap_distance) {
        bt::Rect srect = screen->availableArea();
        // window corners
        const int wleft = dx,
                 wright = dx + frame.rect.width() - 1,
                   wtop = dy,
                wbottom = dy + frame.rect.height() - 1;

        int dleft = abs(wleft - srect.left()),
           dright = abs(wright - srect.right()),
             dtop = abs(wtop - srect.top()),
          dbottom = abs(wbottom - srect.bottom());

        // snap left?
        if (dleft < snap_distance && dleft <= dright)
          dx = srect.left();
        // snap right?
        else if (dright < snap_distance)
          dx = srect.right() - frame.rect.width() + 1;

        // snap top?
        if (dtop < snap_distance && dtop <= dbottom)
          dy = srect.top();
        // snap bottom?
        else if (dbottom < snap_distance)
          dy = srect.bottom() - frame.rect.height() + 1;

        if (! screen->doFullMax()) {
          srect = screen->screenInfo().rect(); // now get the full screen

          dleft = abs(wleft - srect.left()),
             dright = abs(wright - srect.right()),
               dtop = abs(wtop - srect.top()),
            dbottom = abs(wbottom - srect.bottom());

          // snap left?
          if (dleft < snap_distance && dleft <= dright)
            dx = srect.left();
          // snap right?
          else if (dright < snap_distance)
            dx = srect.right() - frame.rect.width() + 1;

          // snap top?
          if (dtop < snap_distance && dtop <= dbottom)
            dy = srect.top();
          // snap bottom?
          else if (dbottom < snap_distance)
            dy = srect.bottom() - frame.rect.height() + 1;
        }
      }

      if (screen->doOpaqueMove()) {
        configure(dx, dy, frame.rect.width(), frame.rect.height());
      } else {
        XDrawRectangle(blackbox->XDisplay(), screen->screenInfo().rootWindow(),
                       screen->getOpGC(),
                       frame.changing.x(),
                       frame.changing.y(),
                       frame.changing.width() - 1,
                       frame.changing.height() - 1);

        frame.changing.setPos(dx, dy);

        XDrawRectangle(blackbox->XDisplay(), screen->screenInfo().rootWindow(),
                       screen->getOpGC(),
                       frame.changing.x(),
                       frame.changing.y(),
                       frame.changing.width() - 1,
                       frame.changing.height() - 1);
      }

      screen->showPosition(dx, dy);
    }
  } else if ((client.functions & Func_Resize) &&
             (me->state & Button1Mask && (me->window == frame.right_grip ||
                                          me->window == frame.left_grip)) ||
             (me->state & Button3Mask && me->state & Mod1Mask &&
              me->window == frame.window)) {
    bool left = (me->window == frame.left_grip);

    if (! client.state.resizing) {
      XGrabServer(blackbox->XDisplay());
      XGrabPointer(blackbox->XDisplay(), me->window, False,
                   ButtonMotionMask | ButtonReleaseMask,
                   GrabModeAsync, GrabModeAsync, None,
                   ((left) ? blackbox->getLowerLeftAngleCursor() :
                    blackbox->getLowerRightAngleCursor()),
                   CurrentTime);

      client.state.resizing = True;

      unsigned int gw, gh;
      frame.grab_x = me->x;
      frame.grab_y = me->y;
      frame.changing = frame.rect;

      constrain((left) ? TopRight : TopLeft, &gw, &gh);

      XDrawRectangle(blackbox->XDisplay(), screen->screenInfo().rootWindow(),
                     screen->getOpGC(), frame.changing.x(), frame.changing.y(),
                     frame.changing.width() - 1, frame.changing.height() - 1);

      screen->showGeometry(gw, gh);
    } else {
      XDrawRectangle(blackbox->XDisplay(), screen->screenInfo().rootWindow(),
                     screen->getOpGC(), frame.changing.x(), frame.changing.y(),
                     frame.changing.width() - 1, frame.changing.height() - 1);

      unsigned int gw, gh;

      Corner anchor;

      if (left) {
        anchor = TopRight;
        int delta =
          std::min<signed>(me->x_root - frame.grab_x,
                           frame.rect.right() -
                           (frame.margin.left + frame.margin.right + 1));
        frame.changing.setCoords(delta, frame.rect.top(),
                                 frame.rect.right(), frame.rect.bottom());
      } else {
        anchor = TopLeft;
        int delta = std::max<signed>(me->x - frame.grab_x + frame.rect.width(),
                                     frame.margin.left +
                                     frame.margin.right + 1);
        frame.changing.setWidth(delta);
      }

      int delta = std::max<signed>(me->y - frame.grab_y + frame.rect.height(),
                                   frame.margin.top + frame.margin.bottom + 1);
      frame.changing.setHeight(delta);

      constrain(anchor, &gw, &gh);

      XDrawRectangle(blackbox->XDisplay(), screen->screenInfo().rootWindow(),
                     screen->getOpGC(), frame.changing.x(), frame.changing.y(),
                     frame.changing.width() - 1, frame.changing.height() - 1);

      screen->showGeometry(gw, gh);
    }
  }
}


void BlackboxWindow::enterNotifyEvent(const XCrossingEvent* ce) {
  if (! (screen->isSloppyFocus() && isVisible()))
    return;

  XEvent e;
  bool leave = False, inferior = False;

  while (XCheckTypedWindowEvent(blackbox->XDisplay(), ce->window,
                                LeaveNotify, &e)) {
    if (e.type == LeaveNotify && e.xcrossing.mode == NotifyNormal) {
      leave = True;
      inferior = (e.xcrossing.detail == NotifyInferior);
    }
  }

  if ((! leave || inferior) && ! isFocused()) {
    bool success = setInputFocus();
    if (success)    // if focus succeeded install the colormap
      installColormap(True); // XXX: shouldnt we honour no install?
  }

  if (screen->doAutoRaise())
    timer->start();
}


void BlackboxWindow::leaveNotifyEvent(const XCrossingEvent*) {
  if (! (screen->isSloppyFocus() && screen->doAutoRaise()))
    return;

  installColormap(False);

  if (timer->isTiming())
    timer->stop();
}


#ifdef    SHAPE
void BlackboxWindow::shapeEvent(const XShapeEvent * const) {
  if (blackbox->hasShapeExtensions() && client.state.shaped) {
    configureShape();
  }
}
#endif // SHAPE


bool BlackboxWindow::validateClient(void) const {
  XSync(blackbox->XDisplay(), False);

  XEvent e;
  if (XCheckTypedWindowEvent(blackbox->XDisplay(), client.window,
                             DestroyNotify, &e) ||
      XCheckTypedWindowEvent(blackbox->XDisplay(), client.window,
                             UnmapNotify, &e)) {
    XPutBackEvent(blackbox->XDisplay(), &e);

    return False;
  }

  return True;
}


void BlackboxWindow::restore(bool remap) {
  XChangeSaveSet(blackbox->XDisplay(), client.window, SetModeDelete);
  XSelectInput(blackbox->XDisplay(), client.window, NoEventMask);
  XSelectInput(blackbox->XDisplay(), frame.plate, NoEventMask);

  bool reparent = False;
  XEvent ev;
  if (XCheckTypedWindowEvent(blackbox->XDisplay(), client.window,
                             ReparentNotify, &ev)) {
    reparent = True;
    remap = True;
  }

  // do not leave a shaded window as an icon unless it was an icon
  if (client.state.shaded && ! client.state.iconic)
    client.current_state = NormalState;

  // remove the wm hints unless the window is being remapped
  setState(client.current_state, ! remap);

  restoreGravity(client.rect);

  XGrabServer(blackbox->XDisplay());

  XUnmapWindow(blackbox->XDisplay(), frame.window);
  XUnmapWindow(blackbox->XDisplay(), client.window);

  XSetWindowBorderWidth(blackbox->XDisplay(), client.window, client.old_bw);
  XMoveWindow(blackbox->XDisplay(), client.window,
              client.rect.x() - frame.rect.x(),
              client.rect.y() - frame.rect.y());

  XUngrabServer(blackbox->XDisplay());

  if (! reparent) {
    // according to the ICCCM - if the client doesn't reparent to
    // root, then we have to do it for them
    XReparentWindow(blackbox->XDisplay(), client.window,
                    screen->screenInfo().rootWindow(),
                    client.rect.x(), client.rect.y());
  }

  if (remap) XMapWindow(blackbox->XDisplay(), client.window);
}


// timer for autoraise
void BlackboxWindow::timeout(bt::Timer *) {
  screen->raiseWindow(this);
}


/*
 * Set the sizes of all components of the window frame
 * (the window decorations).
 * These values are based upon the current style settings and the client
 * window's dimensions.
 */
void BlackboxWindow::upsize(void) {
  WindowStyle *style = screen->getWindowStyle();
  frame.bevel_w = screen->getBevelWidth();

  if (client.decorations & Decor_Border) {
    frame.border_w = screen->getBorderWidth();
    if (!isTransient())
      frame.mwm_border_w = style->frame_width;
    else
      frame.mwm_border_w = 0;
  } else {
    frame.mwm_border_w = frame.border_w = 0;
  }

  if (client.decorations & Decor_Titlebar) {
    // the height of the titlebar is based upon the height of the font being
    // used to display the window's title
    frame.label_h = bt::textHeight(style->font) + 2;
    frame.title_h = frame.label_h + (frame.bevel_w * 2);
    frame.button_w = (frame.label_h - 2);

    // set the top frame margin
    frame.margin.top = frame.border_w + frame.title_h +
                       frame.border_w + frame.mwm_border_w;
  } else {
    frame.label_h = 0;
    frame.title_h = 0;
    frame.button_w = 0;

    // set the top frame margin
    frame.margin.top = frame.border_w + frame.mwm_border_w;
  }

  // set the left/right frame margin
  frame.margin.left = frame.margin.right = frame.border_w + frame.mwm_border_w;

  if (client.decorations & Decor_Handle) {
    frame.grip_w = frame.button_w * 2;
    frame.handle_h = style->handle_width;

    // set the bottom frame margin
    frame.margin.bottom = frame.border_w + frame.handle_h +
                          frame.border_w + frame.mwm_border_w;
  } else {
    frame.handle_h = 0;
    frame.grip_w = 0;

    // set the bottom frame margin
    frame.margin.bottom = frame.border_w + frame.mwm_border_w;
  }

  /*
    We first get the normal dimensions and use this to define the inside_w/h
    then we modify the height if shading is in effect.
    If the shade state is not considered then frame.rect gets reset to the
    normal window size on a reconfigure() call resulting in improper
    dimensions appearing in move/resize and other events.
  */
  unsigned int
    height = client.rect.height() + frame.margin.top + frame.margin.bottom,
    width = client.rect.width() + frame.margin.left + frame.margin.right;

  frame.inside_w = width - (frame.border_w * 2);
  frame.inside_h = height - (frame.border_w * 2);

  if (client.state.shaded)
    height = frame.title_h + (frame.border_w * 2);
  frame.rect.setSize(width, height);
}


/*
 * Calculate the size of the client window and constrain it to the
 * size specified by the size hints of the client window.
 *
 * The logical width and height are placed into pw and ph, if they
 * are non-zero.  Logical size refers to the users perception of
 * the window size (for example an xterm resizes in cells, not in pixels).
 * pw and ph are then used to display the geometry during window moves, resize,
 * etc.
 *
 * The physical geometry is placed into frame.changing_{x,y,width,height}.
 * Physical geometry refers to the geometry of the window in pixels.
 */
void BlackboxWindow::constrain(Corner anchor,
                               unsigned int *pw, unsigned int *ph) {
  // frame.changing represents the requested frame size, we need to
  // strip the frame margin off and constrain the client size
  frame.changing.
    setCoords(frame.changing.left() + static_cast<signed>(frame.margin.left),
              frame.changing.top() + static_cast<signed>(frame.margin.top),
              frame.changing.right() - static_cast<signed>(frame.margin.right),
              frame.changing.bottom() -
              static_cast<signed>(frame.margin.bottom));

  unsigned int dw = frame.changing.width(), dh = frame.changing.height(),
    base_width = (client.base_width) ? client.base_width : client.min_width,
    base_height = (client.base_height) ? client.base_height :
                                         client.min_height;

  // constrain
  if (dw < client.min_width) dw = client.min_width;
  if (dh < client.min_height) dh = client.min_height;
  if (dw > client.max_width) dw = client.max_width;
  if (dh > client.max_height) dh = client.max_height;

  assert(dw >= base_width && dh >= base_height);

  if (client.width_inc > 1) {
    dw -= base_width;
    dw /= client.width_inc;
  }
  if (client.height_inc > 1) {
    dh -= base_height;
    dh /= client.height_inc;
  }

  if (pw)
      *pw = dw;

  if (ph)
      *ph = dh;

  if (client.width_inc > 1) {
    dw *= client.width_inc;
    dw += base_width;
  }
  if (client.height_inc > 1) {
    dh *= client.height_inc;
    dh += base_height;
  }

  frame.changing.setSize(dw, dh);

  // add the frame margin back onto frame.changing
  frame.changing.setCoords(frame.changing.left() - frame.margin.left,
                           frame.changing.top() - frame.margin.top,
                           frame.changing.right() + frame.margin.right,
                           frame.changing.bottom() + frame.margin.bottom);

  // move frame.changing to the specified anchor
  switch (anchor) {
  case TopLeft:
    // nothing to do
    break;

  case TopRight:
    int dx = frame.rect.right() - frame.changing.right();
    frame.changing.setPos(frame.changing.x() + dx, frame.changing.y());
    break;
  }
}


BWindowGroup::BWindowGroup(Blackbox *b, Window _group)
  : blackbox(b), group(_group) {
  XWindowAttributes wattrib;
  if (! XGetWindowAttributes(blackbox->XDisplay(), group, &wattrib)) {
    // group window doesn't seem to exist anymore
    delete this;
    return;
  }

  XSelectInput(blackbox->XDisplay(), group,
               PropertyChangeMask | FocusChangeMask | StructureNotifyMask);

  blackbox->insertWindowGroup(group, this);
}


BWindowGroup::~BWindowGroup(void) {
  blackbox->removeWindowGroup(group);
}


BlackboxWindow *
BWindowGroup::find(BScreen *screen, bool allow_transients) const {
  BlackboxWindow *ret = blackbox->getFocusedWindow();

  // does the focus window match (or any transient_fors)?
  for (; ret; ret = ret->getTransientFor()) {
    if (ret->getScreen() == screen && ret->getGroupWindow() == group &&
        (! ret->isTransient() || allow_transients))
      break;
  }

  if (ret) return ret;

  // the focus window didn't match, look in the group's window list
  BlackboxWindowList::const_iterator it, end = windowList.end();
  for (it = windowList.begin(); it != end; ++it) {
    ret = *it;
    if (ret->getScreen() == screen && ret->getGroupWindow() == group &&
        (! ret->isTransient() || allow_transients))
      break;
  }

  return ret;
}
