// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Toolbar.cc for Blackbox - an X11 Window manager
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

#include "Toolbar.hh"
#include "Iconmenu.hh"
#include "Screen.hh"
#include "Slit.hh"
#include "Toolbarmenu.hh"
#include "Window.hh"
#include "Windowmenu.hh"
#include "Workspacemenu.hh"

#include <Pen.hh>
#include <PixmapCache.hh>

#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <sys/time.h>
#include <assert.h>


static
long aMinuteFromNow(void) {
  timeval now;
  gettimeofday(&now, 0);
  return ((60 - (now.tv_sec % 60)) * 1000);
}


Toolbar::Toolbar(BScreen *scrn) {
  _screen = scrn;
  blackbox = _screen->getBlackbox();

  // get the clock updating every minute
  clock_timer = new bt::Timer(blackbox, this);
  clock_timer->setTimeout(aMinuteFromNow());
  clock_timer->recurring(True);
  clock_timer->start();

  hide_timer = new bt::Timer(blackbox, this);
  hide_timer->setTimeout(blackbox->resource().autoRaiseDelay());

  ScreenResource& res = _screen->resource();

  setLayer(res.isToolbarOnTop()
           ? StackingList::LayerAbove
           : StackingList::LayerNormal);
  hidden = res.doToolbarAutoHide();

  editing = False;
  new_name_pos = 0;

  display = blackbox->XDisplay();
  XSetWindowAttributes attrib;
  unsigned long create_mask = CWColormap | CWOverrideRedirect | CWEventMask;
  attrib.colormap = _screen->screenInfo().colormap();
  attrib.override_redirect = True;
  attrib.event_mask = ButtonPressMask | ButtonReleaseMask |
                      EnterWindowMask | LeaveWindowMask | ExposureMask;

  frame.window =
    XCreateWindow(display, _screen->screenInfo().rootWindow(), 0, 0, 1, 1, 0,
                  _screen->screenInfo().depth(), InputOutput,
                  _screen->screenInfo().visual(),
                  create_mask, &attrib);
  blackbox->insertEventHandler(frame.window, this);

  attrib.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask |
                      KeyPressMask | EnterWindowMask;

  frame.workspace_label =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0,
                  _screen->screenInfo().depth(), InputOutput,
                  _screen->screenInfo().visual(),
                  create_mask, &attrib);
  blackbox->insertEventHandler(frame.workspace_label, this);

  frame.window_label =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0,
                  _screen->screenInfo().depth(), InputOutput,
                  _screen->screenInfo().visual(),
                  create_mask, &attrib);
  blackbox->insertEventHandler(frame.window_label, this);

  frame.clock =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0,
                  _screen->screenInfo().depth(), InputOutput,
                  _screen->screenInfo().visual(),
                  create_mask, &attrib);
  blackbox->insertEventHandler(frame.clock, this);

  frame.psbutton =
    XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0,
                  _screen->screenInfo().depth(), InputOutput,
                  _screen->screenInfo().visual(),
                  create_mask, &attrib);
  blackbox->insertEventHandler(frame.psbutton, this);

  frame.nsbutton =
    XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0,
                  _screen->screenInfo().depth(), InputOutput,
                  _screen->screenInfo().visual(),
                  create_mask, &attrib);
  blackbox->insertEventHandler(frame.nsbutton, this);

  frame.pwbutton =
    XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0,
                  _screen->screenInfo().depth(), InputOutput,
                  _screen->screenInfo().visual(),
                  create_mask, &attrib);
  blackbox->insertEventHandler(frame.pwbutton, this);

  frame.nwbutton =
    XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0,
                  _screen->screenInfo().depth(), InputOutput,
                  _screen->screenInfo().visual(),
                  create_mask, &attrib);
  blackbox->insertEventHandler(frame.nwbutton, this);

  frame.base = frame.slabel = frame.wlabel = frame.clk = frame.button =
    frame.pbutton = None;

  _screen->addStrut(&strut);

  reconfigure();

  XMapSubwindows(display, frame.window);
  XMapWindow(display, frame.window);
}


Toolbar::~Toolbar(void) {
  _screen->removeStrut(&strut);

  XUnmapWindow(display, frame.window);

  bt::PixmapCache::release(frame.base);
  bt::PixmapCache::release(frame.slabel);
  bt::PixmapCache::release(frame.wlabel);
  bt::PixmapCache::release(frame.clk);
  bt::PixmapCache::release(frame.button);
  bt::PixmapCache::release(frame.pbutton);

  blackbox->removeEventHandler(frame.window);
  blackbox->removeEventHandler(frame.workspace_label);
  blackbox->removeEventHandler(frame.window_label);
  blackbox->removeEventHandler(frame.clock);
  blackbox->removeEventHandler(frame.psbutton);
  blackbox->removeEventHandler(frame.nsbutton);
  blackbox->removeEventHandler(frame.pwbutton);
  blackbox->removeEventHandler(frame.nwbutton);

  // all children windows are destroyed by this call as well
  XDestroyWindow(display, frame.window);

  delete hide_timer;
  delete clock_timer;
}


void Toolbar::reconfigure(void) {
  ScreenResource& resource = _screen->resource();
  unsigned int width = (_screen->screenInfo().width() *
                        resource.toolbarWidthPercent()) / 100;

  const ScreenResource::ToolbarStyle* const style = resource.toolbarStyle();
  const unsigned int border_width = style->toolbar.borderWidth();
  const unsigned int extra =
    style->frame_margin == 0 ? style->button.borderWidth() : 0;
  frame.rect.setSize(width, style->toolbar_height);

  int x, y;
  switch (resource.toolbarPlacement()) {
  case TopLeft:
  case TopRight:
  case TopCenter:
    if (resource.toolbarPlacement() == TopLeft)
      x = 0;
    else if (resource.toolbarPlacement() == TopRight)
      x = _screen->screenInfo().width() - frame.rect.width();
    else
      x = (_screen->screenInfo().width() - frame.rect.width()) / 2;
    y = 0;
    frame.y_hidden = style->hidden_height - frame.rect.height();
    break;

  case BottomLeft:
  case BottomRight:
  case BottomCenter:
  default:
    if (resource.toolbarPlacement() == BottomLeft)
      x = 0;
    else if (resource.toolbarPlacement() == BottomRight)
      x = _screen->screenInfo().width() - frame.rect.width();
    else
      x = (_screen->screenInfo().width() - frame.rect.width()) / 2;
    y = _screen->screenInfo().height() - frame.rect.height();
    frame.y_hidden = _screen->screenInfo().height() - style->hidden_height;
    break;
  }

  frame.rect.setPos(x, y);

  updateStrut();

  time_t ttmp = time(NULL);

  unsigned int clock_w = 0u, label_w = 0u;

  if (ttmp != -1) {
    struct tm *tt = localtime(&ttmp);
    if (tt) {
      char t[1024];
      int len = strftime(t, 1024, resource.strftimeFormat(), tt);
      if (len == 0) { // invalid time format found
        // so use the default
        resource.saveStrftimeFormat("%I:%M %p");
        len = strftime(t, 1024, resource.strftimeFormat(), tt);
      }
      /*
       * find the length of the rendered string and add room for two extra
       * characters to it.  This allows for variable width output of the fonts.
       * two 'w' are used to get the widest possible width
       */
      clock_w =
        bt::textRect(_screen->screenNumber(), style->font, t).width() +
        bt::textRect(_screen->screenNumber(), style->font, "ww").width();
    }
  }

  for (unsigned int i = 0; i < _screen->workspaceCount(); i++) {
    width =
      bt::textRect(_screen->screenNumber(), style->font,
                   _screen->resource().nameOfWorkspace(i)).width();
    label_w = std::max(label_w, width);
  }

  label_w = clock_w = std::max(label_w, clock_w) + (style->label_margin * 2);

  unsigned int window_label_w =
    (frame.rect.width() - (border_width * 2)
     - (clock_w + (style->button_width * 4) + label_w
        + (style->frame_margin * 8))
     + extra*6);

  XMoveResizeWindow(display, frame.window, frame.rect.x(),
                    hidden ? frame.y_hidden : frame.rect.y(),
                    frame.rect.width(), frame.rect.height());

  // workspace label
  frame.slabel_rect.setRect(border_width + style->frame_margin,
                         border_width + style->frame_margin,
                         label_w,
                         style->label_height);
  // previous workspace button
  frame.ps_rect.setRect(border_width + (style->frame_margin * 2) + label_w
                         - extra,
                         border_width + style->frame_margin,
                         style->button_width,
                         style->button_width);
  // next workspace button
  frame.ns_rect.setRect(border_width + (style->frame_margin * 3)
                         + label_w + style->button_width - (extra * 2),
                         border_width + style->frame_margin,
                         style->button_width,
                         style->button_width);
  // window label
  frame.wlabel_rect.setRect(border_width + (style->frame_margin * 4)
                         + (style->button_width * 2) + label_w - (extra * 3),
                         border_width + style->frame_margin,
                         window_label_w,
                         style->label_height);
  // previous window button
  frame.pw_rect.setRect(border_width + (style->frame_margin * 5)
                         + (style->button_width * 2) + label_w
                         + window_label_w - (extra * 4),
                         border_width + style->frame_margin,
                         style->button_width,
                         style->button_width);
  // next window button
  frame.nw_rect.setRect(border_width + (style->frame_margin * 6)
                         + (style->button_width * 3) + label_w
                         + window_label_w - (extra * 5),
                         border_width + style->frame_margin,
                         style->button_width,
                         style->button_width);
  // clock
  frame.clock_rect.setRect(frame.rect.width() - clock_w - style->frame_margin
                         - border_width,
                         border_width + style->frame_margin,
                         clock_w,
                         style->label_height);

  XMoveResizeWindow(display, frame.workspace_label,
                    frame.slabel_rect.x(), frame.slabel_rect.y(),
                    frame.slabel_rect.width(), frame.slabel_rect.height());
  XMoveResizeWindow(display, frame.psbutton,
                    frame.ps_rect.x(), frame.ps_rect.y(),
                    frame.ps_rect.width(), frame.ps_rect.height());
  XMoveResizeWindow(display, frame.nsbutton,
                    frame.ns_rect.x(), frame.ns_rect.y(),
                    frame.ns_rect.width(), frame.ns_rect.height());
  XMoveResizeWindow(display, frame.window_label,
                    frame.wlabel_rect.x(), frame.wlabel_rect.y(),
                    frame.wlabel_rect.width(), frame.wlabel_rect.height());
  XMoveResizeWindow(display, frame.pwbutton,
                    frame.pw_rect.x(), frame.pw_rect.y(),
                    frame.pw_rect.width(), frame.pw_rect.height());
  XMoveResizeWindow(display, frame.nwbutton,
                    frame.nw_rect.x(), frame.nw_rect.y(),
                    frame.nw_rect.width(), frame.nw_rect.height());
  XMoveResizeWindow(display, frame.clock,
                    frame.clock_rect.x(), frame.clock_rect.y(),
                    frame.clock_rect.width(), frame.clock_rect.height());

  frame.base =
    bt::PixmapCache::find(_screen->screenNumber(), style->toolbar,
                          frame.rect.width(), frame.rect.height(),
                          frame.base);
  frame.slabel =
    bt::PixmapCache::find(_screen->screenNumber(), style->slabel,
                          frame.slabel_rect.width(),
                          frame.slabel_rect.height(),
                          frame.slabel);
  frame.wlabel =
    bt::PixmapCache::find(_screen->screenNumber(), style->wlabel,
                          frame.wlabel_rect.width(),
                          frame.wlabel_rect.height(),
                          frame.wlabel);
  frame.clk =
    bt::PixmapCache::find(_screen->screenNumber(), style->clock,
                          frame.clock_rect.width(),
                          frame.clock_rect.height(),
                          frame.clk);
  frame.button =
    bt::PixmapCache::find(_screen->screenNumber(), style->button,
                          style->button_width, style->button_width,
                          frame.button);
  frame.pbutton =
    bt::PixmapCache::find(_screen->screenNumber(),
                          style->pressed,
                          style->button_width, style->button_width,
                          frame.pbutton);

  XClearArea(display, frame.window, 0, 0,
             frame.rect.width(), frame.rect.height(), True);

  XClearArea(display, frame.workspace_label, 0, 0,
             label_w, style->label_height, True);
  XClearArea(display, frame.window_label, 0, 0,
             window_label_w, style->label_height, True);
  XClearArea(display, frame.clock, 0, 0,
             clock_w, style->label_height, True);

  XClearArea(display, frame.psbutton, 0, 0,
             style->button_width, style->button_width, True);
  XClearArea(display, frame.nsbutton, 0, 0,
             style->button_width, style->button_width, True);
  XClearArea(display, frame.pwbutton, 0, 0,
             style->button_width, style->button_width, True);
  XClearArea(display, frame.nwbutton, 0, 0,
             style->button_width, style->button_width, True);
}


void Toolbar::updateStrut(void) {
  // left and right are always 0
  strut.top = strut.bottom = 0;

  switch(_screen->resource().toolbarPlacement()) {
  case TopLeft:
  case TopCenter:
  case TopRight:
    strut.top = getExposedHeight();
    break;
  default:
    strut.bottom = getExposedHeight();
  }

  _screen->updateStrut();
}


void Toolbar::redrawClockLabel(void) {
  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();

  bt::Rect u(0, 0, frame.clock_rect.width(), frame.clock_rect.height());
  if (frame.clk == ParentRelative) {
    bt::Rect t(-frame.clock_rect.x(), -frame.clock_rect.y(),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style->toolbar,
                    frame.clock, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(), style->clock,
                    frame.clock, u, u, frame.clk);
  }

  time_t tmp = 0;
  struct tm *tt = 0;
  char str[1024];
  if ((tmp = time(NULL)) == -1)
    return; // should not happen
  tt = localtime(&tmp);
  if (! tt)
    return; // ditto
  if (! strftime(str, 1024, _screen->resource().strftimeFormat(), tt))
    return; // ditto

  bt::Pen pen(_screen->screenNumber(), style->clock_text);
  bt::drawText(style->font, pen, frame.clock, u, style->alignment, str);
}


void Toolbar::redrawWindowLabel(void) {
  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();

  bt::Rect u(0, 0, frame.wlabel_rect.width(), frame.wlabel_rect.height());
  if (frame.wlabel == ParentRelative) {
    bt::Rect t(-frame.wlabel_rect.x(), -frame.wlabel_rect.y(),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style->toolbar,
                    frame.window_label, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(), style->wlabel,
                    frame.window_label, u, u, frame.wlabel);
  }

  BlackboxWindow *foc = _screen->getBlackbox()->getFocusedWindow();
  if (! foc || foc->getScreen() != _screen) return;

  bt::Pen pen(_screen->screenNumber(), style->wlabel_text);
  bt::drawText(style->font, pen, frame.window_label, u,
               style->alignment,
               bt::ellideText(foc->getTitle(), u.width(), "...",
                              _screen->screenNumber(), style->font));
}


void Toolbar::redrawWorkspaceLabel(void) {
  const std::string& name =
    _screen->resource().nameOfWorkspace(_screen->currentWorkspace());
  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();

  bt::Rect u(0, 0, frame.slabel_rect.width(), frame.slabel_rect.height());
  if (frame.slabel == ParentRelative) {
    bt::Rect t(-frame.slabel_rect.x(), -frame.slabel_rect.y(),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style->toolbar,
                    frame.workspace_label, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(), style->slabel,
                    frame.workspace_label, u, u, frame.slabel);
  }

  bt::Pen pen(_screen->screenNumber(), style->slabel_text);
  bt::drawText(style->font, pen, frame.workspace_label, u,
               style->alignment, name);
}


void Toolbar::redrawPrevWorkspaceButton(bool pressed) {
  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, style->button_width, style->button_width);
  if (p == ParentRelative) {
    bt::Rect t(-frame.ps_rect.x(), -frame.ps_rect.y(),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style->toolbar,
                    frame.psbutton, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    pressed ? style->pressed : style->button,
                    frame.psbutton, u, u, p);
  }

  const bt::Pen pen(_screen->screenNumber(), style->foreground);
  bt::drawBitmap(bt::Bitmap::leftArrow(_screen->screenNumber()),
                 pen, frame.psbutton, u);
}


void Toolbar::redrawNextWorkspaceButton(bool pressed) {
  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, style->button_width, style->button_width);
  if (p == ParentRelative) {
    bt::Rect t(-frame.ns_rect.x(), -frame.ns_rect.y(),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style->toolbar,
                    frame.nsbutton, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    pressed ? style->pressed : style->button,
                    frame.nsbutton, u, u, p);
  }

  const bt::Pen pen(_screen->screenNumber(), style->foreground);
  bt::drawBitmap(bt::Bitmap::rightArrow(_screen->screenNumber()),
                 pen, frame.nsbutton, u);
}


void Toolbar::redrawPrevWindowButton(bool pressed) {
  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, style->button_width, style->button_width);
  if (p == ParentRelative) {
    bt::Rect t(-frame.pw_rect.x(), -frame.pw_rect.y(),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style->toolbar,
                    frame.pwbutton, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    pressed ? style->pressed : style->button,
                    frame.pwbutton, u, u, p);
  }

  const bt::Pen pen(_screen->screenNumber(), style->foreground);
  bt::drawBitmap(bt::Bitmap::leftArrow(_screen->screenNumber()),
                 pen, frame.pwbutton, u);
}


void Toolbar::redrawNextWindowButton(bool pressed) {
  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, style->button_width, style->button_width);
  if (p == ParentRelative) {
    bt::Rect t(-frame.nw_rect.x(), -frame.nw_rect.y(),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style->toolbar,
                    frame.nwbutton, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    pressed ? style->pressed : style->button,
                    frame.nwbutton, u, u, p);
  }

  const bt::Pen pen(_screen->screenNumber(), style->foreground);
  bt::drawBitmap(bt::Bitmap::rightArrow(_screen->screenNumber()),
                 pen, frame.nwbutton, u);
}


void Toolbar::edit(void) {
  Window window;
  int foo;

  editing = True;
  XGetInputFocus(display, &window, &foo);
  if (window == frame.workspace_label)
    return;

  XSetInputFocus(display, frame.workspace_label,
                 RevertToPointerRoot, CurrentTime);
  XClearWindow(display, frame.workspace_label);

  blackbox->setNoFocus(True);
  if (blackbox->getFocusedWindow())
    blackbox->getFocusedWindow()->setFocused(false);

  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();
  bt::Pen pen(_screen->screenNumber(), style->slabel_text);
  XDrawRectangle(pen.XDisplay(), frame.workspace_label, pen.gc(),
                 frame.slabel_rect.width() / 2, 0, 1,
                 style->label_height - 1);
  // change the background of the window to that of an active window label
  bt::Texture texture = _screen->resource().windowStyle()->focus.label;
  frame.slabel = bt::PixmapCache::find(_screen->screenNumber(), texture,
                                       frame.slabel_rect.width(),
                                       frame.slabel_rect.height(),
                                       frame.slabel);
  if (! frame.slabel)
    XSetWindowBackground(display, frame.workspace_label,
                         texture.color().pixel(_screen->screenNumber()));
  else
    XSetWindowBackgroundPixmap(display, frame.workspace_label, frame.slabel);
}


void Toolbar::buttonPressEvent(const XButtonEvent * const event) {
  if (event->button == 1) {
    _screen->raiseWindow(this);
    if (event->window == frame.psbutton)
      redrawPrevWorkspaceButton(True);
    else if (event->window == frame.nsbutton)
      redrawNextWorkspaceButton(True);
    else if (event->window == frame.pwbutton)
      redrawPrevWindowButton(True);
    else if (event->window == frame.nwbutton)
      redrawNextWindowButton(True);
  } else if (event->button == 2) {
    _screen->lowerWindow(this);
  } else if (event->button == 3) {
    BlackboxWindow *focus = blackbox->getFocusedWindow();
    if (event->window == frame.window_label &&
        focus && focus->getScreen() == _screen) {
      Windowmenu *windowmenu = _screen->windowmenu(focus);
      windowmenu->popup(event->x_root, event->y_root,
                        _screen->availableArea());
    } else {
      _screen->toolbarmenu()->popup(event->x_root, event->y_root,
                                    _screen->availableArea());
    }
  }
}


void Toolbar::buttonReleaseEvent(const XButtonEvent * const event) {
  if (event->button == 1) {
    const ScreenResource::ToolbarStyle* const style =
      _screen->resource().toolbarStyle();

    if (event->window == frame.psbutton) {
      redrawPrevWorkspaceButton(False);

      if (bt::within(event->x, event->y,
                     style->button_width, style->button_width)) {
        int new_workspace;
        if (_screen->currentWorkspace() > 0)
          new_workspace = _screen->currentWorkspace() - 1;
        else
          new_workspace = _screen->workspaceCount() - 1;
        _screen->setCurrentWorkspace(new_workspace);
      }
    } else if (event->window == frame.nsbutton) {
      redrawNextWorkspaceButton(False);

      if (bt::within(event->x, event->y,
                     style->button_width, style->button_width))
        if (_screen->currentWorkspace() < _screen->workspaceCount() - 1)
          _screen->setCurrentWorkspace(_screen->currentWorkspace() + 1);
        else
          _screen->setCurrentWorkspace(0);
    } else if (event->window == frame.pwbutton) {
      redrawPrevWindowButton(False);

      if (bt::within(event->x, event->y,
                     style->button_width, style->button_width))
        _screen->prevFocus();
    } else if (event->window == frame.nwbutton) {
      redrawNextWindowButton(False);

      if (bt::within(event->x, event->y,
                     style->button_width, style->button_width))
        _screen->nextFocus();
    } else if (event->window == frame.window_label)
      _screen->raiseFocus();
  }
}


void Toolbar::enterNotifyEvent(const XCrossingEvent * const /*unused*/) {
  if (! _screen->resource().doToolbarAutoHide())
    return;

  if (hidden) {
    if (! hide_timer->isTiming())
      hide_timer->start();
  } else if (hide_timer->isTiming()) {
    hide_timer->stop();
  }
}

void Toolbar::leaveNotifyEvent(const XCrossingEvent * const /*unused*/) {
  if (! _screen->resource().doToolbarAutoHide())
    return;

  if (hidden) {
    if (hide_timer->isTiming())
      hide_timer->stop();
  } else if (! hide_timer->isTiming()) {
    hide_timer->start();
  }
}


void Toolbar::exposeEvent(const XExposeEvent * const event) {
  if (event->window == frame.clock) redrawClockLabel();
  else if (event->window == frame.workspace_label && (! editing))
    redrawWorkspaceLabel();
  else if (event->window == frame.window_label) redrawWindowLabel();
  else if (event->window == frame.psbutton) redrawPrevWorkspaceButton();
  else if (event->window == frame.nsbutton) redrawNextWorkspaceButton();
  else if (event->window == frame.pwbutton) redrawPrevWindowButton();
  else if (event->window == frame.nwbutton) redrawNextWindowButton();
  else if (event->window == frame.window) {
    bt::Rect t(0, 0, frame.rect.width(), frame.rect.height());
    bt::Rect r(event->x, event->y, event->width, event->height);
    bt::drawTexture(_screen->screenNumber(),
                    _screen->resource().toolbarStyle()->toolbar,
                    frame.window, t, r & t, frame.base);
  }
}


void Toolbar::keyPressEvent(const XKeyEvent * const event) {
  if (event->window == frame.workspace_label && editing) {
    if (new_workspace_name.empty())
      new_name_pos = 0;

    const ScreenResource::ToolbarStyle* const style =
      _screen->resource().toolbarStyle();

    KeySym ks;
    char keychar[1];
    XLookupString(const_cast<XKeyEvent*>(event), keychar, 1, &ks, 0);

    // either we are told to end with a return or we hit 127 chars
    if (ks == XK_Return || new_name_pos == 127) {
      editing = False;

      blackbox->setNoFocus(False);
      if (blackbox->getFocusedWindow())
        blackbox->getFocusedWindow()->setInputFocus();
      else
        blackbox->setFocusedWindow(0);

      _screen->resource().saveWorkspaceName(_screen->currentWorkspace(),
                                           new_workspace_name);

      _screen->getWorkspacemenu()->
        changeItem(_screen->currentWorkspace(),
                   _screen->resource().
                   nameOfWorkspace(_screen->currentWorkspace()));
      _screen->updateDesktopNamesHint();

      new_workspace_name.erase();
      new_name_pos = 0;

      // reset the background to that of the workspace label (its normal
      // setting)
      bt::Texture texture = style->slabel;
      frame.slabel =
        bt::PixmapCache::find(_screen->screenNumber(), texture,
                              frame.slabel_rect.width(),
                              frame.slabel_rect.height(),
                              frame.slabel);
      if (! frame.slabel)
        XSetWindowBackground(display, frame.workspace_label,
                             texture.color().pixel(_screen->screenNumber()));
      else
        XSetWindowBackgroundPixmap(display, frame.workspace_label,
                                   frame.slabel);
      reconfigure();
    } else if (! (ks == XK_Shift_L || ks == XK_Shift_R ||
                  ks == XK_Control_L || ks == XK_Control_R ||
                  ks == XK_Caps_Lock || ks == XK_Shift_Lock ||
                  ks == XK_Meta_L || ks == XK_Meta_R ||
                  ks == XK_Alt_L || ks == XK_Alt_R ||
                  ks == XK_Super_L || ks == XK_Super_R ||
                  ks == XK_Hyper_L || ks == XK_Hyper_R)) {
      if (ks == XK_BackSpace) {
        if (new_name_pos > 0) {
          --new_name_pos;
          new_workspace_name.erase(new_name_pos);
        } else {
          new_workspace_name.resize(0);
        }
      } else {
        new_workspace_name += (*keychar);
        ++new_name_pos;
      }

      XClearWindow(display, frame.workspace_label);

      bt::Rect rect(0, 0,
                    frame.slabel_rect.width(), frame.slabel_rect.height());
      bt::Rect textr =
        bt::textRect(_screen->screenNumber(), style->font, new_workspace_name);
      bt::Alignment align = (textr.width() >= rect.width()) ?
                            bt::AlignRight : style->alignment;

      bt::Pen pen(_screen->screenNumber(), style->slabel_text);
      bt::drawText(style->font, pen, frame.workspace_label, rect, align,
                   new_workspace_name);
    }
  }
}


void Toolbar::timeout(bt::Timer *timer) {
  if (timer == clock_timer) {
    redrawClockLabel();

    clock_timer->setTimeout(aMinuteFromNow());
  } else if (timer == hide_timer) {
    hidden = ! hidden;
    if (hidden)
      XMoveWindow(display, frame.window,
                  frame.rect.x(), frame.y_hidden);
    else
      XMoveWindow(display, frame.window,
                  frame.rect.x(), frame.rect.y());
  } else {
    // this should not happen
    assert(0);
  }
}


void Toolbar::toggleAutoHide(void) {
  bool do_auto_hide = !_screen->resource().doToolbarAutoHide();

  updateStrut();
  if (_screen->slit())
    _screen->slit()->reposition();

  if (!do_auto_hide && hidden) {
    // force the toolbar to be visible
    if (hide_timer->isTiming()) hide_timer->stop();
    hide_timer->fireTimeout();
  }
}


void Toolbar::setPlacement(Placement place) {
  _screen->resource().saveToolbarPlacement(place);
  reconfigure();

  // reposition the slit as well to make sure it doesn't intersect the
  // toolbar
  if (_screen->slit())
    _screen->slit()->reposition();

  _screen->saveResource();
}
