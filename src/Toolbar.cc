// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Toolbar.cc for Blackbox - an X11 Window manager
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
#include <Unicode.hh>

#include <X11/Xutil.h>
#include <sys/time.h>
#include <assert.h>


long nextTimeout(int resolution)
{
  timeval now;
  gettimeofday(&now, 0);
  return (std::max(1000l, ((((resolution - (now.tv_sec % resolution)) * 1000l))
                           - (now.tv_usec / 1000l))));
}


Toolbar::Toolbar(BScreen *scrn) {
  _screen = scrn;
  blackbox = _screen->blackbox();

  // get the clock updating every minute
  clock_timer = new bt::Timer(blackbox, this);
  clock_timer->recurring(True);

  const ToolbarOptions &options = _screen->resource().toolbarOptions();

  const std::string &time_format = options.strftime_format;
  if (time_format.find("%S") != std::string::npos
      || time_format.find("%s") != std::string::npos
      || time_format.find("%r") != std::string::npos
      || time_format.find("%T") != std::string::npos) {
    clock_timer_resolution = 1;
  } else if (time_format.find("%M") != std::string::npos
             || time_format.find("%R") != std::string::npos) {
    clock_timer_resolution = 60;
  } else {
    clock_timer_resolution = 3600;
  }

  hide_timer = new bt::Timer(blackbox, this);
  hide_timer->setTimeout(blackbox->resource().autoRaiseDelay());

  setLayer(options.always_on_top
           ? StackingList::LayerAbove
           : StackingList::LayerNormal);
  hidden = options.auto_hide;

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

  attrib.event_mask =
    ButtonPressMask | ButtonReleaseMask | ExposureMask | EnterWindowMask;

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

  clock_timer->setTimeout(nextTimeout(clock_timer_resolution));
  clock_timer->start();

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


unsigned int Toolbar::exposedHeight(void) const {
  const ToolbarOptions &options = _screen->resource().toolbarOptions();
  const ToolbarStyle &style = _screen->resource().toolbarStyle();
  return (options.auto_hide ? style.hidden_height : style.toolbar_height);
}


void Toolbar::reconfigure(void) {
  ScreenResource &resource = _screen->resource();
  const ToolbarOptions &options = resource.toolbarOptions();
  const ToolbarStyle &style = resource.toolbarStyle();

  unsigned int width = (_screen->screenInfo().width() *
                        options.width_percent) / 100;

  const unsigned int border_width = style.toolbar.borderWidth();
  const unsigned int extra =
    style.frame_margin == 0 ? style.button.borderWidth() : 0;
  frame.rect.setSize(width, style.toolbar_height);

  int x, y;
  switch (options.placement) {
  case TopLeft:
  case TopRight:
  case TopCenter:
    switch (options.placement) {
    case TopLeft:
      x = 0;
      break;
    case TopRight:
      x = _screen->screenInfo().width() - frame.rect.width();
      break;
    default:
      x = (_screen->screenInfo().width() - frame.rect.width()) / 2;
      break;
    }
    y = 0;
    frame.y_hidden = style.hidden_height - frame.rect.height();
    break;

  case BottomLeft:
  case BottomRight:
  case BottomCenter:
  default:
    switch (options.placement) {
    case BottomLeft:
      x = 0;
      break;
    case BottomRight:
      x = _screen->screenInfo().width() - frame.rect.width();
      break;
    default:
      x = (_screen->screenInfo().width() - frame.rect.width()) / 2;
      break;
    }
    y = _screen->screenInfo().height() - frame.rect.height();
    frame.y_hidden = _screen->screenInfo().height() - style.hidden_height;
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
      int len = strftime(t, 1024, options.strftime_format.c_str(), tt);
      if (len == 0) { // invalid time format found
        // so use the default
        const_cast<std::string &>(options.strftime_format) = "%I:%M %p";
        len = strftime(t, 1024, options.strftime_format.c_str(), tt);
      }
      /*
       * find the length of the rendered string and add room for two extra
       * characters to it.  This allows for variable width output of the fonts.
       * two 'w' are used to get the widest possible width
       */
      clock_w =
        bt::textRect(_screen->screenNumber(), style.font,
                     bt::toUnicode(t)).width() +
        bt::textRect(_screen->screenNumber(), style.font,
                     bt::toUnicode("ww")).width();
    }
  }

  for (unsigned int i = 0; i < _screen->workspaceCount(); i++) {
    width =
      bt::textRect(_screen->screenNumber(), style.font,
                   _screen->resource().workspaceName(i)).width();
    label_w = std::max(label_w, width);
  }

  label_w = clock_w = std::max(label_w, clock_w) + (style.label_margin * 2);

  unsigned int window_label_w =
    (frame.rect.width() - (border_width * 2)
     - (clock_w + (style.button_width * 4) + label_w
        + (style.frame_margin * 8))
     + extra*6);

  XMoveResizeWindow(display, frame.window, frame.rect.x(),
                    hidden ? frame.y_hidden : frame.rect.y(),
                    frame.rect.width(), frame.rect.height());

  // workspace label
  frame.slabel_rect.setRect(border_width + style.frame_margin,
                            border_width + style.frame_margin,
                            label_w,
                            style.label_height);
  // previous workspace button
  frame.ps_rect.setRect(border_width + (style.frame_margin * 2) + label_w
                        - extra,
                        border_width + style.frame_margin,
                        style.button_width,
                        style.button_width);
  // next workspace button
  frame.ns_rect.setRect(border_width + (style.frame_margin * 3)
                        + label_w + style.button_width - (extra * 2),
                        border_width + style.frame_margin,
                        style.button_width,
                        style.button_width);
  // window label
  frame.wlabel_rect.setRect(border_width + (style.frame_margin * 4)
                            + (style.button_width * 2) + label_w - (extra * 3),
                            border_width + style.frame_margin,
                            window_label_w,
                            style.label_height);
  // previous window button
  frame.pw_rect.setRect(border_width + (style.frame_margin * 5)
                        + (style.button_width * 2) + label_w
                        + window_label_w - (extra * 4),
                        border_width + style.frame_margin,
                        style.button_width,
                        style.button_width);
  // next window button
  frame.nw_rect.setRect(border_width + (style.frame_margin * 6)
                        + (style.button_width * 3) + label_w
                        + window_label_w - (extra * 5),
                        border_width + style.frame_margin,
                        style.button_width,
                        style.button_width);
  // clock
  frame.clock_rect.setRect(frame.rect.width() - clock_w - style.frame_margin
                           - border_width,
                           border_width + style.frame_margin,
                           clock_w,
                           style.label_height);

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
    bt::PixmapCache::find(_screen->screenNumber(), style.toolbar,
                          frame.rect.width(), frame.rect.height(),
                          frame.base);
  frame.slabel =
    bt::PixmapCache::find(_screen->screenNumber(), style.slabel,
                          frame.slabel_rect.width(),
                          frame.slabel_rect.height(),
                          frame.slabel);
  frame.wlabel =
    bt::PixmapCache::find(_screen->screenNumber(), style.wlabel,
                          frame.wlabel_rect.width(),
                          frame.wlabel_rect.height(),
                          frame.wlabel);
  frame.clk =
    bt::PixmapCache::find(_screen->screenNumber(), style.clock,
                          frame.clock_rect.width(),
                          frame.clock_rect.height(),
                          frame.clk);
  frame.button =
    bt::PixmapCache::find(_screen->screenNumber(), style.button,
                          style.button_width, style.button_width,
                          frame.button);
  frame.pbutton =
    bt::PixmapCache::find(_screen->screenNumber(),
                          style.pressed,
                          style.button_width, style.button_width,
                          frame.pbutton);

  XClearArea(display, frame.window, 0, 0,
             frame.rect.width(), frame.rect.height(), True);

  XClearArea(display, frame.workspace_label, 0, 0,
             label_w, style.label_height, True);
  XClearArea(display, frame.window_label, 0, 0,
             window_label_w, style.label_height, True);
  XClearArea(display, frame.clock, 0, 0,
             clock_w, style.label_height, True);

  XClearArea(display, frame.psbutton, 0, 0,
             style.button_width, style.button_width, True);
  XClearArea(display, frame.nsbutton, 0, 0,
             style.button_width, style.button_width, True);
  XClearArea(display, frame.pwbutton, 0, 0,
             style.button_width, style.button_width, True);
  XClearArea(display, frame.nwbutton, 0, 0,
             style.button_width, style.button_width, True);
}


void Toolbar::updateStrut(void) {
  // left and right are always 0
  strut.top = strut.bottom = 0;

  switch(_screen->resource().toolbarOptions().placement) {
  case TopLeft:
  case TopCenter:
  case TopRight:
    strut.top = exposedHeight();
    break;
  default:
    strut.bottom = exposedHeight();
  }

  _screen->updateStrut();
}


void Toolbar::redrawClockLabel(void) {
  const ToolbarOptions &options = _screen->resource().toolbarOptions();
  const ToolbarStyle &style = _screen->resource().toolbarStyle();

  bt::Rect u(0, 0, frame.clock_rect.width(), frame.clock_rect.height());
  if (frame.clk == ParentRelative) {
    bt::Rect t(-frame.clock_rect.x(), -frame.clock_rect.y(),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style.toolbar,
                    frame.clock, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(), style.clock,
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
  if (! strftime(str, sizeof(str), options.strftime_format.c_str(), tt))
    return; // ditto

  bt::Pen pen(_screen->screenNumber(), style.clock_text);
  bt::drawText(style.font, pen, frame.clock, u, style.alignment,
               bt::toUnicode(str));
}


void Toolbar::redrawWindowLabel(void) {
  const ToolbarStyle &style = _screen->resource().toolbarStyle();

  bt::Rect u(0, 0, frame.wlabel_rect.width(), frame.wlabel_rect.height());
  if (frame.wlabel == ParentRelative) {
    bt::Rect t(-frame.wlabel_rect.x(), -frame.wlabel_rect.y(),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style.toolbar,
                    frame.window_label, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(), style.wlabel,
                    frame.window_label, u, u, frame.wlabel);
  }

  BlackboxWindow *foc = _screen->blackbox()->focusedWindow();
  if (! foc || foc->screen() != _screen)
    return;

  bt::Pen pen(_screen->screenNumber(), style.wlabel_text);
  bt::drawText(style.font, pen, frame.window_label, u,
               style.alignment,
               bt::ellideText(foc->title(), u.width(), bt::toUnicode("..."),
                              _screen->screenNumber(), style.font));
}


void Toolbar::redrawWorkspaceLabel(void) {
  const bt::ustring &name =
    _screen->resource().workspaceName(_screen->currentWorkspace());
  const ToolbarStyle &style = _screen->resource().toolbarStyle();

  bt::Rect u(0, 0, frame.slabel_rect.width(), frame.slabel_rect.height());
  if (frame.slabel == ParentRelative) {
    bt::Rect t(-frame.slabel_rect.x(), -frame.slabel_rect.y(),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style.toolbar,
                    frame.workspace_label, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(), style.slabel,
                    frame.workspace_label, u, u, frame.slabel);
  }

  bt::Pen pen(_screen->screenNumber(), style.slabel_text);
  bt::drawText(style.font, pen, frame.workspace_label, u,
               style.alignment, name);
}


void Toolbar::redrawPrevWorkspaceButton(bool pressed) {
  const ToolbarStyle &style =
    _screen->resource().toolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, style.button_width, style.button_width);
  if (p == ParentRelative) {
    bt::Rect t(-frame.ps_rect.x(), -frame.ps_rect.y(),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style.toolbar,
                    frame.psbutton, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    pressed ? style.pressed : style.button,
                    frame.psbutton, u, u, p);
  }

  const bt::Pen pen(_screen->screenNumber(), style.foreground);
  bt::drawBitmap(bt::Bitmap::leftArrow(_screen->screenNumber()),
                 pen, frame.psbutton, u);
}


void Toolbar::redrawNextWorkspaceButton(bool pressed) {
  const ToolbarStyle &style =
    _screen->resource().toolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, style.button_width, style.button_width);
  if (p == ParentRelative) {
    bt::Rect t(-frame.ns_rect.x(), -frame.ns_rect.y(),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style.toolbar,
                    frame.nsbutton, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    pressed ? style.pressed : style.button,
                    frame.nsbutton, u, u, p);
  }

  const bt::Pen pen(_screen->screenNumber(), style.foreground);
  bt::drawBitmap(bt::Bitmap::rightArrow(_screen->screenNumber()),
                 pen, frame.nsbutton, u);
}


void Toolbar::redrawPrevWindowButton(bool pressed) {
  const ToolbarStyle &style =
    _screen->resource().toolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, style.button_width, style.button_width);
  if (p == ParentRelative) {
    bt::Rect t(-frame.pw_rect.x(), -frame.pw_rect.y(),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style.toolbar,
                    frame.pwbutton, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    pressed ? style.pressed : style.button,
                    frame.pwbutton, u, u, p);
  }

  const bt::Pen pen(_screen->screenNumber(), style.foreground);
  bt::drawBitmap(bt::Bitmap::leftArrow(_screen->screenNumber()),
                 pen, frame.pwbutton, u);
}


void Toolbar::redrawNextWindowButton(bool pressed) {
  const ToolbarStyle &style =
    _screen->resource().toolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, style.button_width, style.button_width);
  if (p == ParentRelative) {
    bt::Rect t(-frame.nw_rect.x(), -frame.nw_rect.y(),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style.toolbar,
                    frame.nwbutton, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    pressed ? style.pressed : style.button,
                    frame.nwbutton, u, u, p);
  }

  const bt::Pen pen(_screen->screenNumber(), style.foreground);
  bt::drawBitmap(bt::Bitmap::rightArrow(_screen->screenNumber()),
                 pen, frame.nwbutton, u);
}


void Toolbar::buttonPressEvent(const XButtonEvent * const event) {
  if (event->state == Mod1Mask) {
    if (event->button == 1)
      _screen->raiseWindow(this);
    else if (event->button == 2)
      _screen->lowerWindow(this);
    return;
  }

  if (event->button == 1) {
    if (event->window == frame.psbutton)
      redrawPrevWorkspaceButton(True);
    else if (event->window == frame.nsbutton)
      redrawNextWorkspaceButton(True);
    else if (event->window == frame.pwbutton)
      redrawPrevWindowButton(True);
    else if (event->window == frame.nwbutton)
      redrawNextWindowButton(True);
    return;
  }

  if (event->window == frame.window_label) {
    BlackboxWindow *focus = blackbox->focusedWindow();
    if (focus && focus->screen() == _screen) {
      if (event->button == 2) {
        _screen->lowerWindow(focus);
      } else if (event->button == 3) {
        Windowmenu *windowmenu = _screen->windowmenu(focus);
        windowmenu->popup(event->x_root, event->y_root,
                          _screen->availableArea());
      } else if (blackbox->resource().toolbarActionsWithMouseWheel() &&
                 blackbox->resource().shadeWindowWithMouseWheel() &&
                 focus->hasWindowFunction(WindowFunctionShade)) {
        if ((event->button == 4) && !focus->isShaded()) {
          focus->setShaded(true);
		  } else if ((event->button == 5) && focus->isShaded()) {
          focus->setShaded(false);
        }
      }
    }
    return;
  }

  if ((event->window == frame.pwbutton) || (event->window == frame.nwbutton)) {
    if (blackbox->resource().toolbarActionsWithMouseWheel()) {
      if (event->button == 4)
        _screen->nextFocus();
      else if (event->button == 5)
        _screen->prevFocus();
    }
    // XXX: current-workspace window-list menu with button 2 or 3 here..
    return;
  }

  // XXX: workspace-menu with button 2 or 3 on workspace-items (prev/next/label) here

  // default-handlers (scroll through workspaces, popup toolbar-menu)
  if (event->button == 3) {
    _screen->toolbarmenu()->popup(event->x_root, event->y_root,
                                  _screen->availableArea());
    return;
  }

  if (blackbox->resource().toolbarActionsWithMouseWheel()) {
    if (event->button == 4)
      _screen->nextWorkspace();
    else if (event->button == 5)
      _screen->prevWorkspace();
  }
}


void Toolbar::buttonReleaseEvent(const XButtonEvent * const event) {
  if (event->button == 1) {
    const ToolbarStyle &style =
      _screen->resource().toolbarStyle();

    if (event->window == frame.psbutton) {
      redrawPrevWorkspaceButton(False);

      if (bt::within(event->x, event->y,
                     style.button_width, style.button_width)) {
        _screen->prevWorkspace();
      }
    } else if (event->window == frame.nsbutton) {
      redrawNextWorkspaceButton(False);

      if (bt::within(event->x, event->y,
                     style.button_width, style.button_width)) {
        _screen->nextWorkspace();
      }
    } else if (event->window == frame.pwbutton) {
      redrawPrevWindowButton(False);

      if (bt::within(event->x, event->y,
                     style.button_width, style.button_width))
        _screen->prevFocus();
    } else if (event->window == frame.nwbutton) {
      redrawNextWindowButton(False);

      if (bt::within(event->x, event->y,
                     style.button_width, style.button_width))
        _screen->nextFocus();
    } else if (event->window == frame.window_label)
      _screen->raiseFocus();
  }
}


void Toolbar::enterNotifyEvent(const XCrossingEvent * const /*unused*/) {
  if (!_screen->resource().toolbarOptions().auto_hide)
    return;

  if (hidden) {
    if (! hide_timer->isTiming())
      hide_timer->start();
  } else if (hide_timer->isTiming()) {
    hide_timer->stop();
  }
}

void Toolbar::leaveNotifyEvent(const XCrossingEvent * const /*unused*/) {
  if (!_screen->resource().toolbarOptions().auto_hide)
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
  else if (event->window == frame.workspace_label) redrawWorkspaceLabel();
  else if (event->window == frame.window_label) redrawWindowLabel();
  else if (event->window == frame.psbutton) redrawPrevWorkspaceButton();
  else if (event->window == frame.nsbutton) redrawNextWorkspaceButton();
  else if (event->window == frame.pwbutton) redrawPrevWindowButton();
  else if (event->window == frame.nwbutton) redrawNextWindowButton();
  else if (event->window == frame.window) {
    bt::Rect t(0, 0, frame.rect.width(), frame.rect.height());
    bt::Rect r(event->x, event->y, event->width, event->height);
    bt::drawTexture(_screen->screenNumber(),
                    _screen->resource().toolbarStyle().toolbar,
                    frame.window, t, r & t, frame.base);
  }
}


void Toolbar::timeout(bt::Timer *timer) {
  if (timer == clock_timer) {
    redrawClockLabel();

    clock_timer->setTimeout(nextTimeout(clock_timer_resolution));
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
  bool do_auto_hide = !_screen->resource().toolbarOptions().auto_hide;

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
  ToolbarOptions &options =
    const_cast<ToolbarOptions &>(_screen->resource().toolbarOptions());
  options.placement = place;
  reconfigure();

  // reposition the slit as well to make sure it doesn't intersect the
  // toolbar
  if (_screen->slit())
    _screen->slit()->reposition();

  _screen->saveResource();
}
