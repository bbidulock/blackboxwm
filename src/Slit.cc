// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Slit.cc for Blackbox - an X11 Window Manager
// Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2005
//         Bradley T Hughes <bhughes at trolltech.com>
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

#include "Slit.hh"
#include "Screen.hh"
#include "Slitmenu.hh"
#include "Toolbar.hh"

#include <PixmapCache.hh>

#include <algorithm>

#include <X11/Xutil.h>
#include <assert.h>


Slit::Slit(BScreen *scr) {
  screen = scr;
  blackbox = screen->blackbox();

  const SlitOptions &options = screen->resource().slitOptions();

  setLayer(options.always_on_top
           ? StackingList::LayerAbove
           : StackingList::LayerNormal);

  hidden = options.auto_hide;

  display = screen->screenInfo().display().XDisplay();
  frame.window = frame.pixmap = None;

  timer = new bt::Timer(blackbox, this);
  timer->setTimeout(blackbox->resource().autoRaiseDelay());

  XSetWindowAttributes attrib;
  unsigned long create_mask = CWColormap | CWEventMask;
  attrib.colormap = screen->screenInfo().colormap();
  attrib.event_mask = SubstructureRedirectMask | ButtonPressMask |
                      EnterWindowMask | LeaveWindowMask | ExposureMask;

  frame.rect.setSize(1, 1);

  frame.window =
    XCreateWindow(display, screen->screenInfo().rootWindow(),
                  frame.rect.x(), frame.rect.y(),
                  frame.rect.width(), frame.rect.height(),
                  0, screen->screenInfo().depth(),
                  InputOutput, screen->screenInfo().visual(),
                  create_mask, &attrib);
  blackbox->insertEventHandler(frame.window, this);

  screen->addStrut(&strut);
}


Slit::~Slit(void) {
  screen->removeStrut(&strut);

  bt::PixmapCache::release(frame.pixmap);

  blackbox->removeEventHandler(frame.window);

  XDestroyWindow(display, frame.window);

  delete timer;
}


unsigned int Slit::exposedWidth(void) const {
  const SlitOptions &options = screen->resource().slitOptions();
  if (options.direction == Vertical && options.auto_hide)
    return screen->resource().slitStyle().margin;
  return frame.rect.width();
}


unsigned int Slit::exposedHeight(void) const {
  const SlitOptions &options = screen->resource().slitOptions();
  if (options.direction == Horizontal && options.auto_hide)
    return screen->resource().slitStyle().margin;
  return frame.rect.height();
}


void Slit::addClient(Window w) {
  SlitClient *client = new SlitClient;
  client->client_window = w;

  XWMHints *wmhints = XGetWMHints(display, w);

  if (wmhints) {
    if ((wmhints->flags & IconWindowHint) &&
        (wmhints->icon_window != None)) {
      // some dock apps use separate windows, we need to hide these
      XMoveWindow(display, client->client_window,
                  screen->screenInfo().width() + 10,
                  screen->screenInfo().height() + 10);
      XMapWindow(display, client->client_window);

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
  if (XGetWindowAttributes(display, client->window, &attrib)) {
    client->rect.setSize(attrib.width, attrib.height);
  } else {
    client->rect.setSize(64, 64);
  }

  XSetWindowBorderWidth(display, client->window, 0);

  blackbox->XGrabServer();
  XSelectInput(display, client->window, NoEventMask);
  XReparentWindow(display, client->window, frame.window, 0, 0);
  XMapWindow(display, client->window);
  XChangeSaveSet(display, client->window, SetModeInsert);
  XSelectInput(display, client->window, StructureNotifyMask |
               SubstructureNotifyMask | EnterWindowMask);
  blackbox->XUngrabServer();

  clientList.push_back(client);

  blackbox->insertEventHandler(client->client_window, this);
  blackbox->insertEventHandler(client->icon_window, this);
  reconfigure();
}


void Slit::removeClient(SlitClient *client, bool remap) {
  blackbox->removeEventHandler(client->client_window);
  blackbox->removeEventHandler(client->icon_window);
  clientList.remove(client);

  if (remap) {
    blackbox->XGrabServer();
    XSelectInput(display, client->window, NoEventMask);
    XReparentWindow(display, client->window, screen->screenInfo().rootWindow(),
                    frame.rect.x() + client->rect.x(),
                    frame.rect.y() + client->rect.y());
    XChangeSaveSet(display, client->window, SetModeDelete);
    blackbox->XUngrabServer();
  }

  delete client;
}


struct SlitClientMatch {
  Window window;
  SlitClientMatch(Window w): window(w) {}
  inline bool operator()(const Slit::SlitClient* client) const {
    return (client->window == window);
  }
};


void Slit::removeClient(Window w, bool remap) {
  SlitClientList::iterator it = clientList.begin();
  const SlitClientList::iterator end = clientList.end();

  it = std::find_if(it, end, SlitClientMatch(w));
  if (it != end)
    removeClient(*it, remap);

  if (clientList.empty())
    screen->destroySlit();
  else
    reconfigure();
}


void Slit::reconfigure(void) {
  assert(!clientList.empty());

  SlitClientList::iterator it = clientList.begin();
  const SlitClientList::iterator end = clientList.end();
  SlitClient *client;

  unsigned int width = 0, height = 0;

  const SlitOptions &options = screen->resource().slitOptions();
  const SlitStyle &style = screen->resource().slitStyle();

  switch (options.direction) {
  case Vertical:
    for (; it != end; ++it) {
      client = *it;
      height += client->rect.height();
      width = std::max(width, client->rect.width());
    }

    width += (style.slit.borderWidth() + style.margin) * 2;
    height += (style.margin * (clientList.size() + 1))
              + (style.slit.borderWidth() * 2);
    break;

  case Horizontal:
    for (; it != end; ++it) {
      client = *it;
      width += client->rect.width();
      height = std::max(height, client->rect.height());
    }

    width += (style.margin * (clientList.size() + 1))
             + (style.slit.borderWidth() * 2);
    height += (style.slit.borderWidth() + style.margin) * 2;
    break;
  }
  frame.rect.setSize(width, height);

  reposition();

  XMapWindow(display, frame.window);

  const bt::Texture &texture = style.slit;
  frame.pixmap =
    bt::PixmapCache::find(screen->screenNumber(), texture,
                          frame.rect.width(), frame.rect.height(),
                          frame.pixmap);
  XClearArea(display, frame.window, 0, 0,
             frame.rect.width(), frame.rect.height(), True);

  it = clientList.begin();

  int x, y;
  x = y = style.slit.borderWidth() + style.margin;

  switch (options.direction) {
  case Vertical:
    for (; it != end; ++it) {
      client = *it;
      x = (frame.rect.width() - client->rect.width()) / 2;

      XMoveResizeWindow(display, client->window, x, y,
                        client->rect.width(), client->rect.height());
      XMapWindow(display, client->window);

      // for ICCCM compliance
      client->rect.setPos(x, y);

      XEvent event;
      event.type = ConfigureNotify;

      event.xconfigure.display = display;
      event.xconfigure.event = client->window;
      event.xconfigure.window = client->window;
      event.xconfigure.x = x;
      event.xconfigure.y = y;
      event.xconfigure.width = client->rect.width();
      event.xconfigure.height = client->rect.height();
      event.xconfigure.border_width = 0;
      event.xconfigure.above = frame.window;
      event.xconfigure.override_redirect = False;

      XSendEvent(display, client->window, False, StructureNotifyMask, &event);

      y += client->rect.height() + style.margin;
    }

    break;

  case Horizontal:
    for (; it != end; ++it) {
      client = *it;
      y = (frame.rect.height() - client->rect.height()) / 2;

      XMoveResizeWindow(display, client->window, x, y,
                        client->rect.width(), client->rect.height());
      XMapWindow(display, client->window);

      // for ICCCM compliance
      client->rect.setPos(x, y);

      XEvent event;
      event.type = ConfigureNotify;

      event.xconfigure.display = display;
      event.xconfigure.event = client->window;
      event.xconfigure.window = client->window;
      event.xconfigure.x = x;
      event.xconfigure.y = y;
      event.xconfigure.width = client->rect.width();
      event.xconfigure.height = client->rect.height();
      event.xconfigure.border_width = 0;
      event.xconfigure.above = frame.window;
      event.xconfigure.override_redirect = False;

      XSendEvent(display, client->window, False, StructureNotifyMask, &event);

      x += client->rect.width() + style.margin;
    }
    break;
  }
}


void Slit::updateStrut(void) {
  strut.top = strut.bottom = strut.left = strut.right = 0;

  const SlitOptions &options = screen->resource().slitOptions();
  switch (options.direction) {
  case Vertical:
    switch (options.placement) {
    case TopCenter:
      strut.top = exposedHeight();
      break;
    case BottomCenter:
      strut.bottom = exposedHeight();
      break;
    case TopLeft:
    case CenterLeft:
    case BottomLeft:
      strut.left = exposedWidth();
      break;
    case TopRight:
    case CenterRight:
    case BottomRight:
      strut.right = exposedWidth();
      break;
    }
    break;
  case Horizontal:
    switch (options.placement) {
    case TopCenter:
    case TopLeft:
    case TopRight:
      strut.top = frame.rect.top() + exposedHeight();
      break;
    case BottomCenter:
    case BottomLeft:
    case BottomRight:
      strut.bottom = (screen->screenInfo().rect().bottom()
                      - (options.auto_hide
                         ? frame.y_hidden
                         : frame.rect.y()));
      break;
    case CenterLeft:
      strut.left = exposedWidth();
      break;
    case CenterRight:
      strut.right = exposedWidth();
      break;
    }
    break;
  }

  screen->updateStrut();
}


void Slit::reposition(void) {
  int x = 0, y = 0;

  const SlitOptions &options = screen->resource().slitOptions();
  const SlitStyle &style = screen->resource().slitStyle();

  switch (options.placement) {
  case TopLeft:
  case CenterLeft:
  case BottomLeft:
    x = 0;
    frame.x_hidden = style.margin - frame.rect.width();

    if (options.placement == TopLeft)
      y = 0;
    else if (options.placement == CenterLeft)
      y = (screen->screenInfo().height() - frame.rect.height()) / 2;
    else
      y = screen->screenInfo().height() - frame.rect.height();

    break;

  case TopCenter:
  case BottomCenter:
    x = (screen->screenInfo().width() - frame.rect.width()) / 2;
    frame.x_hidden = x;

    if (options.placement == TopCenter)
      y = 0;
    else
      y = screen->screenInfo().height() - frame.rect.height();

    break;

  case TopRight:
  case CenterRight:
  case BottomRight:
    x = screen->screenInfo().width() - frame.rect.width();
    frame.x_hidden =
      screen->screenInfo().width() - style.margin;

    if (options.placement == TopRight)
      y = 0;
    else if (options.placement == CenterRight)
      y = (screen->screenInfo().height() - frame.rect.height()) / 2;
    else
      y = screen->screenInfo().height() - frame.rect.height();

    break;
  }

  frame.rect.setPos(x, y);

  if (screen->toolbar()) {
    bt::Rect tbar_rect = screen->toolbar()->rect();
    bt::Rect slit_rect = frame.rect;

    if (slit_rect.intersects(tbar_rect)) {
      int delta = screen->toolbar()->exposedHeight();
      if (frame.rect.bottom() <= tbar_rect.bottom())
        delta = -delta;

      frame.rect.setY(frame.rect.y() + delta);
    }
  }

  if (options.placement == TopCenter)
    frame.y_hidden = 0 - frame.rect.height() + style.margin;
  else if (options.placement == BottomCenter)
    frame.y_hidden =
      screen->screenInfo().height() - style.margin;
  else
    frame.y_hidden = frame.rect.y();

  updateStrut();

  if (hidden)
    XMoveResizeWindow(display, frame.window,
                      frame.x_hidden, frame.y_hidden,
                      frame.rect.width(), frame.rect.height());
  else
    XMoveResizeWindow(display, frame.window,
                      frame.rect.x(), frame.rect.y(),
                      frame.rect.width(), frame.rect.height());
}


void Slit::shutdown(void) {
  for (;;) {
    bool done = clientList.size() == 1;
    removeClient(clientList.front());
    if (done) break;
  }
}


void Slit::buttonPressEvent(const XButtonEvent * const event) {
  if (event->window != frame.window) return;

  switch (event->button) {
  case Button1:
    screen->raiseWindow(this);
    break;
  case Button2:
    screen->lowerWindow(this);
    break;
  case Button3:
    screen->slitmenu()->popup(event->x_root, event->y_root,
                              screen->availableArea());
    break;
  }
}


void Slit::enterNotifyEvent(const XCrossingEvent * const /*unused*/) {
  if (!screen->resource().slitOptions().auto_hide)
    return;

  if (hidden) {
    if (! timer->isTiming()) timer->start();
  } else {
    if (timer->isTiming()) timer->stop();
  }
}


void Slit::leaveNotifyEvent(const XCrossingEvent * const /*unused*/) {
  if (!screen->resource().slitOptions().auto_hide)
    return;

  if (hidden) {
    if (timer->isTiming())
      timer->stop();
  } else if (! screen->slitmenu()->isVisible()) {
    if (! timer->isTiming())
      timer->start();
  }
}


void Slit::configureRequestEvent(const XConfigureRequestEvent * const event) {
  XWindowChanges xwc;

  xwc.x = event->x;
  xwc.y = event->y;
  xwc.width = event->width;
  xwc.height = event->height;
  xwc.border_width = 0;
  xwc.sibling = event->above;
  xwc.stack_mode = event->detail;

  XConfigureWindow(display, event->window, event->value_mask, &xwc);

  SlitClientList::iterator it = clientList.begin();
  const SlitClientList::iterator end = clientList.end();
  for (; it != end; ++it) {
    SlitClient *client = *it;
    if (client->window == event->window &&
        (static_cast<signed>(client->rect.width()) != event->width ||
         static_cast<signed>(client->rect.height()) != event->height)) {
      client->rect.setSize(event->width, event->height);

      reconfigure();
      return;
    }
  }
}


void Slit::timeout(bt::Timer *) {
  hidden = !hidden;
  if (hidden)
    XMoveWindow(display, frame.window, frame.x_hidden, frame.y_hidden);
  else
    XMoveWindow(display, frame.window, frame.rect.x(), frame.rect.y());
}


void Slit::toggleAutoHide(void) {
  updateStrut();

  if (!screen->resource().slitOptions().auto_hide && hidden) {
    // force the slit to be visible
    if (timer->isTiming())
      timer->stop();
    timer->fireTimeout();
  }
}


void Slit::unmapNotifyEvent(const XUnmapEvent * const event) {
  removeClient(event->window);
}


void Slit::reparentNotifyEvent(const XReparentEvent * const event) {
  if (event->parent != frame.window)
    removeClient(event->window, False);
}


void Slit::exposeEvent(const XExposeEvent * const event) {
  bt::drawTexture(screen->screenNumber(),
                  screen->resource().slitStyle().slit,
                  frame.window,
                  bt::Rect(0, 0, frame.rect.width(), frame.rect.height()),
                  bt::Rect(event->x, event->y, event->width, event->height),
                  frame.pixmap);
}


Slit::Direction Slit::direction(void) const {
  const SlitOptions &options = screen->resource().slitOptions();
  return static_cast<Slit::Direction>(options.direction);
}


Slit::Placement Slit::placement(void) const {
  const SlitOptions &options = screen->resource().slitOptions();
  return static_cast<Slit::Placement>(options.placement);
}
