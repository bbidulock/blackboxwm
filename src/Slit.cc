// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Slit.cc for Blackbox - an X11 Window manager
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

#ifdef    SLIT

#include <X11/keysym.h>

#include "i18n.hh"
#include "blackbox.hh"
#include "Image.hh"
#include "Screen.hh"
#include "Slit.hh"
#include "Toolbar.hh"


Slit::Slit(BScreen *scr)
{
  screen = scr;
  blackbox = Blackbox::instance();

  on_top = screen->isSlitOnTop();
  hidden = do_auto_hide = screen->doSlitAutoHide();

  frame.window = frame.pixmap = None;

  timer = new BTimer(blackbox, this);
  timer->setTimeout(blackbox->getAutoRaiseDelay());

  clientList = new LinkedList<SlitClient>;

  slitmenu = new Slitmenu(this);

  XSetWindowAttributes attrib;
  unsigned long create_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
                              CWColormap | CWOverrideRedirect | CWEventMask;
  attrib.background_pixmap = None;
  attrib.background_pixel = attrib.border_pixel = screen->style()->borderColor().pixel();
  attrib.colormap = screen->colormap();
  attrib.override_redirect = True;
  attrib.event_mask = SubstructureRedirectMask |
                      ButtonPressMask | ButtonReleaseMask |
                      EnterWindowMask | LeaveWindowMask;

  frame.x = frame.y = 0;
  frame.width = frame.height = 1;

  frame.window =
    XCreateWindow(*blackbox, screen->rootWindow(), frame.x, frame.y,
		  frame.width, frame.height, screen->style()->borderWidth(),
                  screen->depth(), InputOutput, screen->visual(),
                  create_mask, &attrib);
  blackbox->saveSlitSearch(frame.window, this);

  screen->addStrut(&strut);

  reconfigure();
}


Slit::~Slit() {
  if (timer->isTiming()) timer->stop();
  delete timer;

  delete clientList;
  delete slitmenu;

  screen->getImageControl()->removeImage(frame.pixmap);

  blackbox->removeSlitSearch(frame.window);

  XDestroyWindow(*blackbox, frame.window);
}


void Slit::addClient(Window w) {
  if (blackbox->validateWindow(w)) {
    SlitClient *client = new SlitClient;
    client->client_window = w;

    XWMHints *wmhints = XGetWMHints(*blackbox, w);

    if (wmhints) {
      if ((wmhints->flags & IconWindowHint) &&
	  (wmhints->icon_window != None)) {
	XMoveWindow(*blackbox, client->client_window, screen->width() + 10,
		    screen->height() + 10);
	XMapWindow(*blackbox, client->client_window);

	client->icon_window = wmhints->icon_window;
	client->window = client->icon_window;
      } else {
	client->icon_window = None;
	client->window = client->client_window;
      }

      XFree(wmhints);
    } else {
      client->icon_window = None;
      client->window = client->client_window;
    }

    XWindowAttributes attrib;
    if (XGetWindowAttributes(*blackbox, client->window, &attrib)) {
      client->width = attrib.width;
      client->height = attrib.height;
    } else {
      client->width = client->height = 64;
    }

    XSetWindowBorderWidth(*blackbox, client->window, 0);

    XSelectInput(*blackbox, frame.window, NoEventMask);
    XSelectInput(*blackbox, client->window, NoEventMask);

    XReparentWindow(*blackbox, client->window, frame.window, 0, 0);
    XMapRaised(*blackbox, client->window);
    XChangeSaveSet(*blackbox, client->window, SetModeInsert);

    XSelectInput(*blackbox, frame.window, SubstructureRedirectMask |
		 ButtonPressMask | ButtonReleaseMask |
                 EnterWindowMask | LeaveWindowMask);
    XSelectInput(*blackbox, client->window, StructureNotifyMask |
                 SubstructureNotifyMask | EnterWindowMask);
    XFlush(*blackbox);

    clientList->insert(client);

    blackbox->saveSlitSearch(client->client_window, this);
    blackbox->saveSlitSearch(client->icon_window, this);
    reconfigure();
  }
}


void Slit::removeClient(SlitClient *client, Bool remap) {
  blackbox->removeSlitSearch(client->client_window);
  blackbox->removeSlitSearch(client->icon_window);
  clientList->remove(client);

  screen->removeNetizen(client->window);

  if (remap && blackbox->validateWindow(client->window)) {
    XSelectInput(*blackbox, frame.window, NoEventMask);
    XSelectInput(*blackbox, client->window, NoEventMask);
    XReparentWindow(*blackbox, client->window, screen->rootWindow(),
		    client->x, client->y);
    XChangeSaveSet(*blackbox, client->window, SetModeDelete);
    XSelectInput(*blackbox, frame.window, SubstructureRedirectMask |
		 ButtonPressMask | ButtonReleaseMask |
                 EnterWindowMask | LeaveWindowMask);
    XFlush(*blackbox);
  }

  delete client;
  client = (SlitClient *) 0;
}


void Slit::removeClient(Window w, Bool remap) {
  Bool reconf = False;

  LinkedListIterator<SlitClient> it(clientList);
  for (SlitClient *tmp = it.current(); tmp; it++, tmp = it.current()) {
    if (tmp->window == w) {
      removeClient(tmp, remap);
      reconf = True;

      break;
    }
  }

  if (reconf) reconfigure();
}


void Slit::reconfigure(void) {
  frame.width = 0;
  frame.height = 0;
  LinkedListIterator<SlitClient> it(clientList);
  SlitClient *client;

  switch (screen->getSlitDirection()) {
  case Vertical:
    for (client = it.current(); client; it++, client = it.current()) {
      frame.height += client->height + screen->style()->bevelWidth();

      if (frame.width < client->width)
        frame.width = client->width;
    }

    if (frame.width < 1)
      frame.width = 1;
    else
      frame.width += (screen->style()->bevelWidth() * 2);

    if (frame.height < 1)
      frame.height = 1;
    else
      frame.height += screen->style()->bevelWidth();
    break;

  case Horizontal:
    for (client = it.current(); client; it++, client = it.current()) {
      frame.width += client->width + screen->style()->bevelWidth();

      if (frame.height < client->height)
        frame.height = client->height;
    }

    if (frame.width < 1)
      frame.width = 1;
    else
      frame.width += screen->style()->bevelWidth();

    if (frame.height < 1)
      frame.height = 1;
    else
      frame.height += (screen->style()->bevelWidth() * 2);
    break;
  }

  reposition();

  XSetWindowBorderWidth(*blackbox ,frame.window, screen->style()->borderWidth());
  XSetWindowBorder(*blackbox, frame.window, screen->style()->borderColor().pixel());

  if (! clientList->count())
    XUnmapWindow(*blackbox, frame.window);
  else
    XMapWindow(*blackbox, frame.window);

  BTexture texture = screen->style()->toolbar();
  frame.pixmap = texture.render( Size( frame.width, frame.height ),
                                 frame.pixmap );
  if ( ! frame.pixmap )
    XSetWindowBackground(*blackbox, frame.window, texture.color().pixel());
  XClearWindow(*blackbox, frame.window);

  int x, y;
  it.reset();

  strut.top = strut.bottom = strut.left = strut.right = 0;

  switch (screen->getSlitDirection()) {
  case Vertical:
    x = 0;
    y = screen->style()->bevelWidth();

    for (client = it.current(); client; it++, client = it.current()) {
      x = (frame.width - client->width) / 2;

      XMoveResizeWindow(*blackbox, client->window, x, y,
                        client->width, client->height);
      XMapWindow(*blackbox, client->window);

      // for ICCCM compliance
      client->x = x;
      client->y = y;

      XEvent event;
      event.type = ConfigureNotify;

      event.xconfigure.display = *blackbox;
      event.xconfigure.event = client->window;
      event.xconfigure.window = client->window;
      event.xconfigure.x = x;
      event.xconfigure.y = y;
      event.xconfigure.width = client->width;
      event.xconfigure.height = client->height;
      event.xconfigure.border_width = 0;
      event.xconfigure.above = frame.window;
      event.xconfigure.override_redirect = False;

      XSendEvent(*blackbox, client->window, False, StructureNotifyMask, &event);

      y += client->height + screen->style()->bevelWidth();
    }
    switch (screen->getSlitPlacement()) {
    case TopCenter:
      strut.top = height() + 1;
      break;
    case BottomCenter:
      strut.bottom = (screen->height() - getY()) - 1;
      break;
    case TopLeft:
    case CenterLeft:
    case BottomLeft:
      strut.left = width() + 1;
      break;
    case TopRight:
    case CenterRight:
    case BottomRight:
      strut.right = (screen->width() - getX()) - 1;
      break;
    }
    break;

  case Horizontal:
    x = screen->style()->bevelWidth();
    y = 0;

    for (client = it.current(); client; it++, client = it.current()) {
      y = (frame.height - client->height) / 2;

      XMoveResizeWindow(*blackbox, client->window, x, y,
                        client->width, client->height);
      XMapWindow(*blackbox, client->window);

      // for ICCCM compliance
      client->x = x;
      client->y = y;

      XEvent event;
      event.type = ConfigureNotify;

      event.xconfigure.display = *blackbox;
      event.xconfigure.event = client->window;
      event.xconfigure.window = client->window;
      event.xconfigure.x = x;
      event.xconfigure.y = y;
      event.xconfigure.width = client->width;
      event.xconfigure.height = client->height;
      event.xconfigure.border_width = 0;
      event.xconfigure.above = frame.window;
      event.xconfigure.override_redirect = False;

      XSendEvent(*blackbox, client->window, False, StructureNotifyMask, &event);

      x += client->width + screen->style()->bevelWidth();
    }
    switch (screen->getSlitPlacement()) {
    case TopCenter:
    case TopLeft:
    case TopRight:
      strut.top = height() + 1;
      break;
    case BottomCenter:
    case BottomLeft:
    case BottomRight:
      strut.bottom = (screen->height() - getY()) - 1;
      break;
    case CenterLeft:
      strut.left = width() + 1;
      break;
    case CenterRight:
      strut.right = (screen->width() - getX()) - 1;
      break;
    }
    break;
  }

  // update area with new Strut info
  screen->updateAvailableArea();
  slitmenu->reconfigure();
}


void Slit::reposition(void) {
  // place the slit in the appropriate place
  switch (screen->getSlitPlacement()) {
  case TopLeft:
    frame.x = 0;
    frame.y = 0;
    if (screen->getSlitDirection() == Vertical) {
      frame.x_hidden = screen->style()->bevelWidth() - screen->style()->borderWidth()
	               - frame.width;
      frame.y_hidden = 0;
    } else {
      frame.x_hidden = 0;
      frame.y_hidden = screen->style()->bevelWidth() - screen->style()->borderWidth()
	               - frame.height;
    }
    break;

  case CenterLeft:
    frame.x = 0;
    frame.y = (screen->height() - frame.height) / 2;
    frame.x_hidden = screen->style()->bevelWidth() - screen->style()->borderWidth()
	             - frame.width;
    frame.y_hidden = frame.y;
    break;

  case BottomLeft:
    frame.x = 0;
    frame.y = screen->height() - frame.height
              - (screen->style()->borderWidth() * 2);
    if (screen->getSlitDirection() == Vertical) {
      frame.x_hidden = screen->style()->bevelWidth() - screen->style()->borderWidth()
	               - frame.width;
      frame.y_hidden = frame.y;
    } else {
      frame.x_hidden = 0;
      frame.y_hidden = screen->height() - screen->style()->bevelWidth()
	               - screen->style()->borderWidth();
    }
    break;

  case TopCenter:
    frame.x = (screen->width() - frame.width) / 2;
    frame.y = 0;
    frame.x_hidden = frame.x;
    frame.y_hidden = screen->style()->bevelWidth() - screen->style()->borderWidth()
                     - frame.height;
    break;

  case BottomCenter:
    frame.x = (screen->width() - frame.width) / 2;
    frame.y = screen->height() - frame.height
              - (screen->style()->borderWidth() * 2);
    frame.x_hidden = frame.x;
    frame.y_hidden = screen->height() - screen->style()->bevelWidth()
                     - screen->style()->borderWidth();
    break;

  case TopRight:
    frame.x = screen->width() - frame.width
              - (screen->style()->borderWidth() * 2);
    frame.y = 0;
    if (screen->getSlitDirection() == Vertical) {
      frame.x_hidden = screen->width() - screen->style()->bevelWidth()
	               - screen->style()->borderWidth();
      frame.y_hidden = 0;
    } else {
      frame.x_hidden = frame.x;
      frame.y_hidden = screen->style()->bevelWidth() - screen->style()->borderWidth()
                       - frame.height;
    }
    break;

  case CenterRight:
  default:
    frame.x = screen->width() - frame.width
              - (screen->style()->borderWidth() * 2);
    frame.y = (screen->height() - frame.height) / 2;
    frame.x_hidden = screen->width() - screen->style()->bevelWidth()
                     - screen->style()->borderWidth();
    frame.y_hidden = frame.y;
    break;

  case BottomRight:
    frame.x = screen->width() - frame.width
              - (screen->style()->borderWidth() * 2);
    frame.y = screen->height() - frame.height
              - (screen->style()->borderWidth() * 2);
    if (screen->getSlitDirection() == Vertical) {
      frame.x_hidden = screen->width() - screen->style()->bevelWidth()
	               - screen->style()->borderWidth();
      frame.y_hidden = frame.y;
    } else {
      frame.x_hidden = frame.x;
      frame.y_hidden = screen->height() - screen->style()->bevelWidth()
                       - screen->style()->borderWidth();
    }
    break;
  }

  Toolbar *tbar = screen->getToolbar();
  int sw = frame.width + (screen->style()->borderWidth() * 2),
      sh = frame.height + (screen->style()->borderWidth() * 2),
      tw = tbar->width() + screen->style()->borderWidth(),
      th = tbar->height() + screen->style()->borderWidth();

  if (tbar->getX() < frame.x + sw && tbar->getX() + tw > frame.x &&
      tbar->getY() < frame.y + sh && tbar->getY() + th > frame.y) {
    if (frame.y < th) {
      frame.y += tbar->getExposedHeight();
      if (screen->getSlitDirection() == Vertical)
        frame.y_hidden += tbar->getExposedHeight();
      else
	frame.y_hidden = frame.y;
    } else {
      frame.y -= tbar->getExposedHeight();
      if (screen->getSlitDirection() == Vertical)
        frame.y_hidden -= tbar->getExposedHeight();
      else
	frame.y_hidden = frame.y;
    }
  }

  if (hidden)
    XMoveResizeWindow(*blackbox, frame.window, frame.x_hidden,
		      frame.y_hidden, frame.width, frame.height);
  else
    XMoveResizeWindow(*blackbox, frame.window, frame.x,
		      frame.y, frame.width, frame.height);
}

void Slit::shutdown(void) {
  while (clientList->count())
    removeClient(clientList->first());
}

void Slit::buttonPressEvent(XButtonEvent *e) {
  if (e->window != frame.window) return;

  if (e->button == Button1 && (! on_top)) {
    Window w[1] = { frame.window };
    screen->raiseWindows(w, 1);
  } else if (e->button == Button2 && (! on_top)) {
    XLowerWindow(*blackbox, frame.window);
  }
}

void Slit::buttonReleaseEvent(XButtonEvent *e)
{
  if (e->window != frame.window)
    return;

  if (e->button == Button3)
    slitmenu->popup(e->x_root, e->y_root);
}

void Slit::enterNotifyEvent(XCrossingEvent *) {
  if (! do_auto_hide)
    return;

  if (hidden) {
    if (! timer->isTiming()) timer->start();
  } else {
    if (timer->isTiming()) timer->stop();
  }
}

void Slit::leaveNotifyEvent(XCrossingEvent *) {
  if (! do_auto_hide)
    return;

  if (hidden) {
    if (timer->isTiming()) timer->stop();
  } else if (! slitmenu->isVisible()) {
    if (! timer->isTiming()) timer->start();
  }
}

void Slit::configureRequestEvent(XConfigureRequestEvent *e) {
  if (blackbox->validateWindow(e->window)) {
    Bool reconf = False;
    XWindowChanges xwc;

    xwc.x = e->x;
    xwc.y = e->y;
    xwc.width = e->width;
    xwc.height = e->height;
    xwc.border_width = 0;
    xwc.sibling = e->above;
    xwc.stack_mode = e->detail;

    XConfigureWindow(*blackbox, e->window, e->value_mask, &xwc);

    LinkedListIterator<SlitClient> it(clientList);
    SlitClient *client = it.current();
    for (; client; it++, client = it.current())
      if (client->window == e->window)
        if (client->width != ((unsigned) e->width) ||
            client->height != ((unsigned) e->height)) {
          client->width = (unsigned) e->width;
          client->height = (unsigned) e->height;

          reconf = True;

          break;
        }

    if (reconf) reconfigure();

  }
}

void Slit::timeout(void) {
  hidden = ! hidden;
  if (hidden)
    XMoveWindow(*blackbox, frame.window, frame.x_hidden, frame.y_hidden);
  else
    XMoveWindow(*blackbox, frame.window, frame.x, frame.y);
}

Slitmenu::Slitmenu(Slit *sl)
  : Basemenu( sl->screen->screenNumber() )
{
  slit = sl;

  directionmenu = new Directionmenu(this);
  placementmenu = new Placementmenu(this);

  insert(i18n(CommonSet, CommonDirectionTitle, "Direction"),
         directionmenu);
  insert(i18n(CommonSet, CommonPlacementTitle, "Placement"),
         placementmenu);
  insert(i18n(CommonSet, CommonAlwaysOnTop, "Always on top"), 1);
  insert(i18n(CommonSet, CommonAutoHide, "Auto hide"), 2);

  if (slit->isOnTop()) setItemChecked(2, True);
  if (slit->doAutoHide()) setItemChecked(3, True);
}

void Slitmenu::itemClicked(const Point &, const Item &item, int button)
{
  if (button != 1)
    return;

  switch (item.function()) {
  case 1: // always on top
    slit->on_top = ((slit->isOnTop()) ?  False : True);
    setItemChecked(2, slit->on_top);
    if (slit->isOnTop())
      slit->screen->raiseWindows((Window *) 0, 0);
    break;

  case 2:  // auto hide
    slit->do_auto_hide = ((slit->doAutoHide()) ?  False : True);
    setItemChecked(3, slit->do_auto_hide);
    break;
  } // switch
}

void Slitmenu::hide(void)
{
  Basemenu::hide();
  if (slit->doAutoHide())
    slit->timeout();
}

void Slitmenu::reconfigure(void) {
  directionmenu->reconfigure();
  placementmenu->reconfigure();

  Basemenu::reconfigure();
}

Slitmenu::Directionmenu::Directionmenu(Slitmenu *sm)
  : Basemenu( sm->slit->screen->screenNumber() )
{
  slitmenu = sm;

  insert(i18n(CommonSet, CommonDirectionHoriz, "Horizontal"),
         Slit::Horizontal);
  insert(i18n(CommonSet, CommonDirectionVert, "Vertical"),
         Slit::Vertical);

  if (sm->slit->screen->getSlitDirection() == Slit::Horizontal)
    setItemChecked(0, True);
  else
    setItemChecked(1, True);
}

void Slitmenu::Directionmenu::itemClicked(const Point &, const Item &item, int button)
{
  if (button != 1)
    return;

  slitmenu->slit->screen->saveSlitDirection(item.function());

  if (item.function() == Slit::Horizontal) {
    setItemChecked(0, true);
    setItemChecked(1, false);
  } else {
    setItemChecked(0, false);
    setItemChecked(1, true);
  }

  slitmenu->slit->reconfigure();
}

Slitmenu::Placementmenu::Placementmenu(Slitmenu *sm)
  : Basemenu( sm->slit->screen->screenNumber() )
{
  slitmenu = sm;

  // setMinimumSublevels(3);

  insert(i18n(CommonSet, CommonPlacementTopLeft, "Top Left"),
         Slit::TopLeft);
  insert(i18n(CommonSet, CommonPlacementCenterLeft, "Center Left"),
         Slit::CenterLeft);
  insert(i18n(CommonSet, CommonPlacementBottomLeft, "Bottom Left"),
         Slit::BottomLeft);
  insert(i18n(CommonSet, CommonPlacementTopCenter, "Top Center"),
         Slit::TopCenter);
  insert(i18n(CommonSet, CommonPlacementBottomCenter, "Bottom Center"),
         Slit::BottomCenter);
  insert(i18n(CommonSet, CommonPlacementTopRight, "Top Right"),
         Slit::TopRight);
  insert(i18n(CommonSet, CommonPlacementCenterRight, "Center Right"),
         Slit::CenterRight);
  insert(i18n(CommonSet, CommonPlacementBottomRight, "Bottom Right"),
         Slit::BottomRight);
}

void Slitmenu::Placementmenu::itemClicked(const Point &, const Item &item, int button)
{
  if (button != 1)
    return;

  slitmenu->slit->screen->saveSlitPlacement(item.function());
  slitmenu->slit->reconfigure();
}

#endif // SLIT
