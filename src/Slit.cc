// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Slit.cc for Blackbox - an X11 Window Manager
// Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2003
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <X11/keysym.h>
}

#include "Slit.hh"
#include "Menu.hh"
#include "PixmapCache.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "i18n.hh"


class Slitmenu : public bt::Menu {
public:
  Slitmenu(bt::Application &app, unsigned int screen, Slit *slit);

  void refresh(void);

protected:
  void itemClicked(unsigned int id, unsigned int button);

private:
  Slit *_slit;
};


class SlitDirectionmenu : public bt::Menu {
public:
  SlitDirectionmenu(bt::Application &app, unsigned int screen, Slit *slit);

  void refresh(void);

protected:
  void itemClicked(unsigned int id, unsigned int button);

private:
  Slit *_slit;
};


class SlitPlacementmenu : public bt::Menu {
public:
  SlitPlacementmenu(bt::Application &app, unsigned int screen, Slit *slit);

protected:
  void itemClicked(unsigned int id, unsigned int button);

private:
  Slit *_slit;
};


Slitmenu::Slitmenu(bt::Application &app, unsigned int screen, Slit *slit)
  : bt::Menu(app, screen), _slit(slit) {
  insertItem(bt::i18n(CommonSet, CommonDirectionTitle, "Direction"),
             new SlitDirectionmenu(app, screen, slit), 0u);
  insertItem(bt::i18n(CommonSet, CommonPlacementTitle, "Placement"),
             new SlitPlacementmenu(app, screen, slit), 1u);
  insertSeparator();
  insertItem(bt::i18n(CommonSet, CommonAlwaysOnTop, "Always on top"), 2u);
  insertItem(bt::i18n(CommonSet, CommonAutoHide, "Auto hide"), 3u);
}


void Slitmenu::refresh(void) {
  setItemChecked(2u, _slit->isOnTop());
  setItemChecked(3u, _slit->doAutoHide());
}


void Slitmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  switch (id) {
  case 2u: // always on top
    _slit->toggleOnTop();
    break;

  case 3u: // auto hide
    _slit->toggleAutoHide();
    break;

  default:
    break;
  } // switch
}


SlitDirectionmenu::SlitDirectionmenu(bt::Application &app, unsigned int screen,
                                     Slit *slit)
  : bt::Menu(app, screen), _slit(slit) {
  insertItem(bt::i18n(CommonSet, CommonDirectionHoriz, "Horizontal"),
             Slit::Horizontal);
  insertItem(bt::i18n(CommonSet, CommonDirectionVert, "Vertical"),
             Slit::Vertical);
}


void SlitDirectionmenu::refresh(void) {
  setItemChecked(Slit::Horizontal, _slit->direction() == Slit::Horizontal);
  setItemChecked(Slit::Vertical, _slit->direction() == Slit::Vertical);
}


void SlitDirectionmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  _slit->setDirection(static_cast<Slit::Direction>(id));
}


SlitPlacementmenu::SlitPlacementmenu(bt::Application &app, unsigned int screen,
                                     Slit *slit)
  : bt::Menu(app, screen), _slit(slit) {
  insertItem(bt::i18n(CommonSet, CommonPlacementTopLeft, "Top Left"),
             Slit::TopLeft);
  insertItem(bt::i18n(CommonSet, CommonPlacementCenterLeft, "Center Left"),
             Slit::CenterLeft);
  insertItem(bt::i18n(CommonSet, CommonPlacementBottomLeft, "Bottom Left"),
             Slit::BottomLeft);
  insertSeparator();
  insertItem(bt::i18n(CommonSet, CommonPlacementTopCenter, "Top Center"),
             Slit::TopCenter);
  insertItem(bt::i18n(CommonSet, CommonPlacementBottomCenter, "Bottom Center"),
             Slit::BottomCenter);
  insertSeparator();
  insertItem(bt::i18n(CommonSet, CommonPlacementTopRight, "Top Right"),
             Slit::TopRight);
  insertItem(bt::i18n(CommonSet, CommonPlacementCenterRight, "Center Right"),
             Slit::CenterRight);
  insertItem(bt::i18n(CommonSet, CommonPlacementBottomRight, "Bottom Right"),
             Slit::BottomRight);
}


void SlitPlacementmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  _slit->setPlacement(static_cast<Slit::Placement>(id));
}




Slit::Slit(BScreen *scr) {
  screen = scr;
  blackbox = screen->getBlackbox();

  ScreenResource& res = screen->resource();

  on_top = res.isSlitOnTop();
  hidden = do_auto_hide = res.doSlitAutoHide();

  display = screen->screenInfo().display().XDisplay();
  frame.window = frame.pixmap = None;

  timer = new bt::Timer(blackbox, this);
  timer->setTimeout(blackbox->getAutoRaiseDelay());

  slitmenu =
    new Slitmenu(*blackbox, screen->screenNumber(), this);

  XSetWindowAttributes attrib;
  unsigned long create_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
                              CWColormap | CWOverrideRedirect | CWEventMask;
  attrib.background_pixmap = None;
  attrib.background_pixel = attrib.border_pixel =
    res.borderColor()->pixel(screen->screenNumber());
  attrib.colormap = screen->screenInfo().colormap();
  attrib.override_redirect = True;
  attrib.event_mask = SubstructureRedirectMask | ButtonPressMask |
                      EnterWindowMask | LeaveWindowMask;

  frame.rect.setSize(1, 1);

  frame.window =
    XCreateWindow(display, screen->screenInfo().rootWindow(),
                  frame.rect.x(), frame.rect.y(),
                  frame.rect.width(), frame.rect.height(),
                  res.borderWidth(), screen->screenInfo().depth(),
                  InputOutput, screen->screenInfo().visual(),
                  create_mask, &attrib);
  blackbox->insertEventHandler(frame.window, this);

  screen->addStrut(&strut);

  reconfigure();
}


Slit::~Slit(void) {
  screen->removeStrut(&strut);

  bt::PixmapCache::release(frame.pixmap);

  blackbox->removeEventHandler(frame.window);

  XDestroyWindow(display, frame.window);

  delete timer;
  delete slitmenu;
}


void Slit::addClient(Window w) {
  if (! blackbox->validateWindow(w))
    return;

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

  XGrabServer(display);
  XSelectInput(display, frame.window, NoEventMask);
  XSelectInput(display, client->window, NoEventMask);
  XReparentWindow(display, client->window, frame.window, 0, 0);
  XMapRaised(display, client->window);
  XChangeSaveSet(display, client->window, SetModeInsert);
  XSelectInput(display, frame.window, SubstructureRedirectMask |
               ButtonPressMask | EnterWindowMask | LeaveWindowMask);
  XSelectInput(display, client->window, StructureNotifyMask |
               SubstructureNotifyMask | EnterWindowMask);

  XUngrabServer(display);

  clientList.push_back(client);

  blackbox->insertEventHandler(client->client_window, this);
  blackbox->insertEventHandler(client->icon_window, this);
  reconfigure();
}


void Slit::removeClient(SlitClient *client, bool remap) {
  blackbox->removeEventHandler(client->client_window);
  blackbox->removeEventHandler(client->icon_window);
  clientList.remove(client);

  if (remap && blackbox->validateWindow(client->window)) {
    XGrabServer(display);
    XSelectInput(display, frame.window, NoEventMask);
    XSelectInput(display, client->window, NoEventMask);
    XReparentWindow(display, client->window, screen->screenInfo().rootWindow(),
                    client->rect.x(), client->rect.y());
    XChangeSaveSet(display, client->window, SetModeDelete);
    XSelectInput(display, frame.window, SubstructureRedirectMask |
                 ButtonPressMask | EnterWindowMask | LeaveWindowMask);
    XUngrabServer(display);
  }

  delete client;
  client = 0;
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
  if (it != end) {
    removeClient(*it, remap);
    reconfigure();
  }
}


void Slit::reconfigure(void) {
  if (clientList.empty()) {
    XUnmapWindow(display, frame.window);
    strut.left = strut.right = strut.top = strut.bottom = 0;
    updateStrut();
    return;
  }

  SlitClientList::iterator it = clientList.begin();
  const SlitClientList::iterator end = clientList.end();
  SlitClient *client;

  unsigned int width = 0, height = 0;

  switch (screen->resource().slitDirection()) {
  case Vertical:
    for (; it != end; ++it) {
      client = *it;
      height += client->rect.height() + screen->resource().bevelWidth();

      if (width < client->rect.width())
        width = client->rect.width();
    }

    if (width < 1)
      width = 1;
    else
      width += (screen->resource().bevelWidth() * 2);

    if (height < 1)
      height = 1;
    else
      height += screen->resource().bevelWidth();

    break;

  case Horizontal:
    for (; it != end; ++it) {
      client = *it;
      width += client->rect.width() + screen->resource().bevelWidth();

      if (height < client->rect.height())
        height = client->rect.height();
    }

    if (width < 1)
      width = 1;
    else
      width += screen->resource().bevelWidth();

    if (height < 1)
      height = 1;
    else
      height += (screen->resource().bevelWidth() * 2);

    break;
  }
  frame.rect.setSize(width, height);

  reposition();

  XSetWindowBorderWidth(display, frame.window,
                        screen->resource().borderWidth());
  XSetWindowBorder(display, frame.window,
                   screen->resource().borderColor()->pixel(screen->screenNumber()));

  XMapWindow(display, frame.window);

  bt::Texture texture = screen->resource().toolbarStyle()->toolbar;
  frame.pixmap =
    bt::PixmapCache::find(screen->screenNumber(), texture,
                          frame.rect.width(), frame.rect.height(),
                          frame.pixmap);
  if (! frame.pixmap)
    XSetWindowBackground(display, frame.window,
                         texture.color().pixel(screen->screenNumber()));
  else
    XSetWindowBackgroundPixmap(display, frame.window, frame.pixmap);

  XClearWindow(display, frame.window);

  it = clientList.begin();

  int x, y;

  switch (screen->resource().slitDirection()) {
  case Vertical:
    x = 0;
    y = screen->resource().bevelWidth();

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

      y += client->rect.height() + screen->resource().bevelWidth();
    }

    break;

  case Horizontal:
    x = screen->resource().bevelWidth();
    y = 0;

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

      x += client->rect.width() + screen->resource().bevelWidth();
    }
    break;
  }

  slitmenu->reconfigure();
}


void Slit::updateStrut(void) {
  strut.top = strut.bottom = strut.left = strut.right = 0;

  if (! clientList.empty()) {
    // when not hidden both borders are in use, when hidden only one is
    unsigned int border_width = screen->resource().borderWidth();
    if (! do_auto_hide)
      border_width *= 2;

    switch (screen->resource().slitDirection()) {
    case Vertical:
      switch (screen->resource().slitPlacement()) {
      case TopCenter:
        strut.top = getExposedHeight() + border_width;
        break;
      case BottomCenter:
        strut.bottom = getExposedHeight() + border_width;
        break;
      case TopLeft:
      case CenterLeft:
      case BottomLeft:
        strut.left = getExposedWidth() + border_width;
        break;
      case TopRight:
      case CenterRight:
      case BottomRight:
        strut.right = getExposedWidth() + border_width;
        break;
      }
      break;
    case Horizontal:
      switch (screen->resource().slitPlacement()) {
      case TopCenter:
      case TopLeft:
      case TopRight:
        strut.top = frame.rect.top() + getExposedHeight() + border_width;
        break;
      case BottomCenter:
      case BottomLeft:
      case BottomRight: {
        const int pos = (do_auto_hide) ? frame.y_hidden : frame.rect.y();
        strut.bottom = (screen->screenInfo().rect().bottom() - pos);
      }
        break;
      case CenterLeft:
        strut.left = getExposedWidth() + border_width;
        break;
      case CenterRight:
        strut.right = getExposedWidth() + border_width;
        break;
      }
      break;
    }
  }
  screen->updateStrut();
}


void Slit::reposition(void) {
  int x = 0, y = 0;

  switch (screen->resource().slitPlacement()) {
  case TopLeft:
  case CenterLeft:
  case BottomLeft:
    x = 0;
    frame.x_hidden = screen->resource().bevelWidth()
      - screen->resource().borderWidth() - frame.rect.width();

    if (screen->resource().slitPlacement() == TopLeft)
      y = 0;
    else if (screen->resource().slitPlacement() == CenterLeft)
      y = (screen->screenInfo().height() - frame.rect.height()) / 2;
    else
      y = screen->screenInfo().height() - frame.rect.height()
	- (screen->resource().borderWidth() * 2);

    break;

  case TopCenter:
  case BottomCenter:
    x = (screen->screenInfo().width() - frame.rect.width()) / 2;
    frame.x_hidden = x;

    if (screen->resource().slitPlacement() == TopCenter)
      y = 0;
    else
      y = screen->screenInfo().height() - frame.rect.height()
	- (screen->resource().borderWidth() * 2);

    break;

  case TopRight:
  case CenterRight:
  case BottomRight:
    x = screen->screenInfo().width() - frame.rect.width()
      - (screen->resource().borderWidth() * 2);
    frame.x_hidden = screen->screenInfo().width()
      - screen->resource().bevelWidth() - screen->resource().borderWidth();

    if (screen->resource().slitPlacement() == TopRight)
      y = 0;
    else if (screen->resource().slitPlacement() == CenterRight)
      y = (screen->screenInfo().height() - frame.rect.height()) / 2;
    else
      y = screen->screenInfo().height() - frame.rect.height()
	- (screen->resource().borderWidth() * 2);
    break;
  }

  frame.rect.setPos(x, y);

  // we have to add the border to the rect as it is not accounted for
  bt::Rect tbar_rect = screen->getToolbar()->getRect();
  tbar_rect.setSize(tbar_rect.width() + (screen->resource().borderWidth() * 2),
                    tbar_rect.height() +
                    (screen->resource().borderWidth() * 2));
  bt::Rect slit_rect = frame.rect;
  slit_rect.setSize(slit_rect.width() + (screen->resource().borderWidth() * 2),
                    slit_rect.height() +
                    (screen->resource().borderWidth() * 2));

  if (slit_rect.intersects(tbar_rect)) {
    int delta = screen->getToolbar()->getExposedHeight() +
      screen->resource().borderWidth();
    if (frame.rect.bottom() <= tbar_rect.bottom())
      delta = -delta;

    frame.rect.setY(frame.rect.y() + delta);
  }

  if (screen->resource().slitPlacement() == TopCenter)
    frame.y_hidden = 0 - frame.rect.height() + screen->resource().borderWidth()
      + screen->resource().bevelWidth();
  else if (screen->resource().slitPlacement() == BottomCenter)
    frame.y_hidden = screen->screenInfo().height()
      - screen->resource().borderWidth() - screen->resource().bevelWidth();
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
  while (! clientList.empty())
    removeClient(clientList.front());
}


void Slit::buttonPressEvent(const XButtonEvent * const e) {
  if (e->window != frame.window) return;

  if (e->button == Button1 && ! on_top) {
    WindowStack w;
    w.push_back(frame.window);
    screen->raiseWindows(&w);
  } else if (e->button == Button2 && ! on_top) {
    XLowerWindow(display, frame.window);
  } else if (e->button == Button3) {
    slitmenu->popup(e->x_root, e->y_root);
  }
}


void Slit::enterNotifyEvent(const XCrossingEvent * const) {
  if (! do_auto_hide)
    return;

  if (hidden) {
    if (! timer->isTiming()) timer->start();
  } else {
    if (timer->isTiming()) timer->stop();
  }
}


void Slit::leaveNotifyEvent(const XCrossingEvent * const) {
  if (! do_auto_hide)
    return;

  if (hidden) {
    if (timer->isTiming()) timer->stop();
  } else if (! slitmenu->isVisible()) {
    if (! timer->isTiming()) timer->start();
  }
}


void Slit::configureRequestEvent(const XConfigureRequestEvent * const e) {
  if (! blackbox->validateWindow(e->window))
    return;

  XWindowChanges xwc;

  xwc.x = e->x;
  xwc.y = e->y;
  xwc.width = e->width;
  xwc.height = e->height;
  xwc.border_width = 0;
  xwc.sibling = e->above;
  xwc.stack_mode = e->detail;

  XConfigureWindow(display, e->window, e->value_mask, &xwc);

  SlitClientList::iterator it = clientList.begin();
  const SlitClientList::iterator end = clientList.end();
  for (; it != end; ++it) {
    SlitClient *client = *it;
    if (client->window == e->window &&
        (static_cast<signed>(client->rect.width()) != e->width ||
         static_cast<signed>(client->rect.height()) != e->height)) {
      client->rect.setSize(e->width, e->height);

      reconfigure();
      return;
    }
  }
}


void Slit::timeout(bt::Timer *) {
  hidden = ! hidden;
  if (hidden)
    XMoveWindow(display, frame.window, frame.x_hidden, frame.y_hidden);
  else
    XMoveWindow(display, frame.window, frame.rect.x(), frame.rect.y());
}


void Slit::toggleOnTop(void) {
  on_top = ! on_top;
  if (on_top) screen->raiseWindows(0);
}



void Slit::toggleAutoHide(void) {
  do_auto_hide = ! do_auto_hide;

  updateStrut();

  if (do_auto_hide == False && hidden) {
    // force the slit to be visible
    if (timer->isTiming()) timer->stop();
    timer->fireTimeout();
  }
}


void Slit::unmapNotifyEvent(const XUnmapEvent * const e) {
  removeClient(e->window);
}


void Slit::reparentNotifyEvent(const XReparentEvent * const event) {
  if (event->parent != frame.window)
    removeClient(event->window, False);
}


Slit::Direction Slit::direction(void) const {
  return static_cast<Slit::Direction>(screen->resource().slitDirection());
}


void Slit::setDirection(Slit::Direction new_direction) {
  screen->resource().saveSlitDirection(new_direction);
  reconfigure();
}


Slit::Placement Slit::placement(void) const {
  return static_cast<Slit::Placement>(screen->resource().slitPlacement());
}


void Slit::setPlacement(Slit::Placement new_placement) {
  screen->resource().saveSlitPlacement(new_placement);
  reconfigure();
}
