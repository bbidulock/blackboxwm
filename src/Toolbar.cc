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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <X11/keysym.h>
#include <assert.h>

#ifdef HAVE_STRING_H
#  include <string.h>
#endif // HAVE_STRING_H

#include <stdio.h>

#ifdef    TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else // !TIME_WITH_SYS_TIME
# ifdef    HAVE_SYS_TIME_H
#  include <sys/time.h>
# else // !HAVE_SYS_TIME_H
#  include <time.h>
# endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME
}

#include <string>

#include "i18n.hh"
#include "blackbox.hh"
#include "Menu.hh"
#include "Pen.hh"
#include "PixmapCache.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Clientmenu.hh"
#include "Slit.hh"


class Toolbarmenu : public bt::Menu {
public:
  Toolbarmenu(bt::Application &app, unsigned int screen, Toolbar *toolbar);

  void refresh(void);

protected:
  void itemClicked(unsigned int id, unsigned int button);

private:
  Toolbar *_toolbar;

  friend class Toolbar;
};


class ToolbarPlacementmenu : public bt::Menu {
public:
  ToolbarPlacementmenu(bt::Application &app, unsigned int screen,
                       Toolbar *toolbar);

protected:
  void itemClicked(unsigned int id, unsigned int button);

private:
  Toolbar *_toolbar;
};


Toolbarmenu::Toolbarmenu(bt::Application &app, unsigned int screen,
                         Toolbar *toolbar)
  : bt::Menu(app, screen), _toolbar(toolbar) {
  ToolbarPlacementmenu *menu = new ToolbarPlacementmenu(app, screen, toolbar);
  insertItem(bt::i18n(CommonSet, CommonPlacementTitle, "Placement"), menu, 1u);
  insertSeparator();
  insertItem(bt::i18n(CommonSet, CommonAlwaysOnTop, "Always on top"), 2u);
  insertItem(bt::i18n(CommonSet, CommonAutoHide, "Auto Hide"), 3u);
  insertSeparator();
  insertItem(bt::i18n(ToolbarSet, ToolbarEditWkspcName,
                      "Edit current workspace name"), 4u);
}


void Toolbarmenu::refresh(void) {
  setItemChecked(2u, _toolbar->isOnTop());
  setItemChecked(3u, _toolbar->doAutoHide());
}


void Toolbarmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  switch (id) {
  case 2u: // always on top
    _toolbar->toggleOnTop();
    break;

  case 3u: // auto hide
    _toolbar->toggleAutoHide();
    break;

  case 4u:
    _toolbar->edit();
    break;

  default:
    break;
  } // switch
}


ToolbarPlacementmenu::ToolbarPlacementmenu(bt::Application &app,
                                           unsigned int screen,
                                           Toolbar *toolbar)
  : bt::Menu(app, screen), _toolbar(toolbar) {
  insertItem(bt::i18n(CommonSet, CommonPlacementTopLeft,
                      "Top Left"), Toolbar::TopLeft);
  insertItem(bt::i18n(CommonSet, CommonPlacementTopCenter,
                      "Top Center"), Toolbar::TopCenter);
  insertItem(bt::i18n(CommonSet, CommonPlacementTopRight,
                      "Top Right"), Toolbar::TopRight);
  insertSeparator();
  insertItem(bt::i18n(CommonSet, CommonPlacementBottomLeft,
                      "Bottom Left"), Toolbar::BottomLeft);
  insertItem(bt::i18n(CommonSet, CommonPlacementBottomCenter,
                      "Bottom Center"), Toolbar::BottomCenter);
  insertItem(bt::i18n(CommonSet, CommonPlacementBottomRight,
                      "Bottom Right"), Toolbar::BottomRight);
}


void ToolbarPlacementmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  _toolbar->setPlacement((Toolbar::Placement) id);
}




static long aMinuteFromNow(void) {
  timeval now;
  gettimeofday(&now, 0);
  return ((60 - (now.tv_sec % 60)) * 1000);
}


Toolbar::Toolbar(BScreen *scrn) {
  screen = scrn;
  blackbox = screen->getBlackbox();

  // get the clock updating every minute
  clock_timer = new bt::Timer(blackbox, this);
  clock_timer->setTimeout(aMinuteFromNow());
  clock_timer->recurring(True);
  clock_timer->start();

  hide_timer = new bt::Timer(blackbox, this);
  hide_timer->setTimeout(blackbox->getAutoRaiseDelay());

  on_top = screen->isToolbarOnTop();
  hidden = do_auto_hide = screen->doToolbarAutoHide();

  editing = False;
  new_name_pos = 0;

  toolbarmenu =
    new Toolbarmenu(*blackbox, screen->screenNumber(), this);

  display = blackbox->XDisplay();
  XSetWindowAttributes attrib;
  unsigned long create_mask = CWColormap | CWOverrideRedirect | CWEventMask;
  attrib.colormap = screen->screenInfo().colormap();
  attrib.override_redirect = True;
  attrib.event_mask = ButtonPressMask | ButtonReleaseMask |
                      EnterWindowMask | LeaveWindowMask | ExposureMask;

  frame.window =
    XCreateWindow(display, screen->screenInfo().rootWindow(), 0, 0, 1, 1, 0,
                  screen->screenInfo().depth(), InputOutput,
                  screen->screenInfo().visual(), create_mask, &attrib);
  blackbox->insertEventHandler(frame.window, this);

  attrib.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask |
                      KeyPressMask | EnterWindowMask;

  frame.workspace_label =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0,
                  screen->screenInfo().depth(), InputOutput,
                  screen->screenInfo().visual(), create_mask, &attrib);
  blackbox->insertEventHandler(frame.workspace_label, this);

  frame.window_label =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0,
                  screen->screenInfo().depth(), InputOutput,
                  screen->screenInfo().visual(), create_mask, &attrib);
  blackbox->insertEventHandler(frame.window_label, this);

  frame.clock =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0,
                  screen->screenInfo().depth(), InputOutput,
                  screen->screenInfo().visual(), create_mask, &attrib);
  blackbox->insertEventHandler(frame.clock, this);

  frame.psbutton =
    XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0,
                  screen->screenInfo().depth(), InputOutput,
                  screen->screenInfo().visual(), create_mask, &attrib);
  blackbox->insertEventHandler(frame.psbutton, this);

  frame.nsbutton =
    XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0,
                  screen->screenInfo().depth(), InputOutput,
                  screen->screenInfo().visual(), create_mask, &attrib);
  blackbox->insertEventHandler(frame.nsbutton, this);

  frame.pwbutton =
    XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0,
                  screen->screenInfo().depth(), InputOutput,
                  screen->screenInfo().visual(), create_mask, &attrib);
  blackbox->insertEventHandler(frame.pwbutton, this);

  frame.nwbutton =
    XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0,
                  screen->screenInfo().depth(), InputOutput,
                  screen->screenInfo().visual(), create_mask, &attrib);
  blackbox->insertEventHandler(frame.nwbutton, this);

  frame.base = frame.label = frame.wlabel = frame.clk = frame.button =
    frame.pbutton = None;

  screen->addStrut(&strut);

  reconfigure();

  XMapSubwindows(display, frame.window);
  XMapWindow(display, frame.window);
}


Toolbar::~Toolbar(void) {
  screen->removeStrut(&strut);

  XUnmapWindow(display, frame.window);

  bt::PixmapCache::release(frame.base);
  bt::PixmapCache::release(frame.label);
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
  delete toolbarmenu;
}


void Toolbar::reconfigure(void) {
  unsigned int height = 0,
                width = (screen->screenInfo().width() *
                         screen->getToolbarWidthPercent()) / 100;

  const ToolbarStyle * const style = screen->getToolbarStyle();
  height = bt::textHeight(style->font);
  frame.bevel_w = screen->getBevelWidth();
  frame.button_w = height;
  height += 2;
  frame.label_h = height;
  height += (frame.bevel_w * 2);

  unsigned int bw = style->toolbar.borderWidth();
  width  += bw * 2;
  height += bw * 2;

  frame.rect.setSize(width, height);

  int x, y;
  switch (screen->getToolbarPlacement()) {
  case TopLeft:
  case TopRight:
  case TopCenter:
    if (screen->getToolbarPlacement() == TopLeft)
      x = 0;
    else if (screen->getToolbarPlacement() == TopRight)
      x = screen->screenInfo().width() - frame.rect.width();
    else
      x = (screen->screenInfo().width() - frame.rect.width()) / 2;

    y = 0;

    frame.x_hidden = x;
    frame.y_hidden = screen->getBevelWidth() - frame.rect.height();
    break;

  case BottomLeft:
  case BottomRight:
  case BottomCenter:
  default:
    if (screen->getToolbarPlacement() == BottomLeft)
      x = 0;
    else if (screen->getToolbarPlacement() == BottomRight)
      x = screen->screenInfo().width() - frame.rect.width();
    else
      x = (screen->screenInfo().width() - frame.rect.width()) / 2;

    y = screen->screenInfo().height() - frame.rect.height();

    frame.x_hidden = x;
    frame.y_hidden = screen->screenInfo().height() - screen->getBevelWidth();
    break;
  }

  frame.rect.setPos(x, y);

  updateStrut();

  time_t ttmp = time(NULL);

  frame.clock_w = 0;
  if (ttmp != -1) {
    struct tm *tt = localtime(&ttmp);
    if (tt) {
      char t[1024];
      int len = strftime(t, 1024, screen->getStrftimeFormat(), tt);
      if (len == 0) { // invalid time format found
        screen->saveStrftimeFormat("%I:%M %p"); // so use the default
        len = strftime(t, 1024, screen->getStrftimeFormat(), tt);
      }
      // find the length of the rendered string and add room for two extra
      // characters to it.  This allows for variable width output of the fonts
      frame.clock_w = bt::textRect(style->font, t).width() +
                      bt::textRect(style->font, "ww").width();
    }
  }

  frame.workspace_label_w = 0;

  for (unsigned int i = 0; i < screen->getWorkspaceCount(); i++) {
    width = bt::textRect(style->font, screen->getWorkspaceName(i)).width();
    if (width > frame.workspace_label_w) frame.workspace_label_w = width;
  }

  frame.workspace_label_w = frame.clock_w =
                            std::max(frame.workspace_label_w,
                                     frame.clock_w) + (frame.bevel_w * 4);

  frame.window_label_w =
    (frame.rect.width() - (bw * 2) -
     (frame.clock_w + (frame.button_w * 4) +
      frame.workspace_label_w + (frame.bevel_w * 8) + 7));

  if (hidden) {
    XMoveResizeWindow(display, frame.window, frame.x_hidden, frame.y_hidden,
                      frame.rect.width(), frame.rect.height());
  } else {
    XMoveResizeWindow(display, frame.window, frame.rect.x(), frame.rect.y(),
                      frame.rect.width(), frame.rect.height());
  }

  XMoveResizeWindow(display, frame.workspace_label,
                    bw + frame.bevel_w, bw + frame.bevel_w,
                    frame.workspace_label_w, frame.label_h);
  XMoveResizeWindow(display, frame.psbutton,
                    bw + ((frame.bevel_w * 2) + frame.workspace_label_w + 1),
                    bw + frame.bevel_w + 1, frame.button_w, frame.button_w);
  XMoveResizeWindow(display, frame.nsbutton,
                    bw + ((frame.bevel_w * 3) + frame.workspace_label_w +
                          frame.button_w + 2), bw + frame.bevel_w + 1,
                    frame.button_w, frame.button_w);
  XMoveResizeWindow(display, frame.window_label,
                    bw + ((frame.bevel_w * 4) + (frame.button_w * 2) +
                          frame.workspace_label_w + 3), bw + frame.bevel_w,
                    frame.window_label_w, frame.label_h);
  XMoveResizeWindow(display, frame.pwbutton,
                    bw + ((frame.bevel_w * 5) + (frame.button_w * 2) +
                          frame.workspace_label_w + frame.window_label_w + 4),
                    bw + frame.bevel_w + 1, frame.button_w, frame.button_w);
  XMoveResizeWindow(display, frame.nwbutton,
                    bw + ((frame.bevel_w * 6) + (frame.button_w * 3) +
                          frame.workspace_label_w + frame.window_label_w + 5),
                    bw + frame.bevel_w + 1, frame.button_w, frame.button_w);
  XMoveResizeWindow(display, frame.clock,
                    frame.rect.width() - frame.clock_w -
                    (frame.bevel_w * 2) - bw, bw + frame.bevel_w,
                    frame.clock_w, frame.label_h);

  frame.base = bt::PixmapCache::find(screen->screenNumber(), style->toolbar,
                                     frame.rect.width(), frame.rect.height(),
                                     frame.base);
  frame.label = bt::PixmapCache::find(screen->screenNumber(), style->window,
                                      frame.window_label_w, frame.label_h,
                                      frame.label);
  frame.wlabel = bt::PixmapCache::find(screen->screenNumber(), style->label,
                                       frame.workspace_label_w, frame.label_h,
                                       frame.wlabel);
  frame.clk = bt::PixmapCache::find(screen->screenNumber(), style->clock,
                                    frame.clock_w, frame.label_h, frame.clk);
  frame.button = bt::PixmapCache::find(screen->screenNumber(), style->button,
                                       frame.button_w, frame.button_w,
                                       frame.button);
  frame.pbutton = bt::PixmapCache::find(screen->screenNumber(), style->pressed,
                                        frame.button_w, frame.button_w,
                                        frame.pbutton);

  XClearArea(display, frame.window, 0, 0,
             frame.rect.width(), frame.rect.height(), True);

  XClearArea(display, frame.workspace_label, 0, 0,
             frame.workspace_label_w, frame.label_h, True);
  XClearArea(display, frame.window_label, 0, 0,
             frame.window_label_w, frame.label_h, True);
  XClearArea(display, frame.clock, 0, 0,
             frame.clock_w, frame.label_h, True);

  XClearArea(display, frame.psbutton, 0, 0,
             frame.button_w, frame.button_w, True);
  XClearArea(display, frame.nsbutton, 0, 0,
             frame.button_w, frame.button_w, True);
  XClearArea(display, frame.pwbutton, 0, 0,
             frame.button_w, frame.button_w, True);
  XClearArea(display, frame.nwbutton, 0, 0,
             frame.button_w, frame.button_w, True);

  toolbarmenu->reconfigure();
}


void Toolbar::updateStrut(void) {
  // left and right are always 0
  strut.top = strut.bottom = 0;

  switch(screen->getToolbarPlacement()) {
  case TopLeft:
  case TopCenter:
  case TopRight:
    strut.top = getExposedHeight();
    break;
  default:
    strut.bottom = getExposedHeight();
  }

  screen->updateStrut();
}


void Toolbar::redrawClockLabel(void) {
  time_t tmp = 0;
  struct tm *tt = 0;

  if ((tmp = time(NULL)) == -1) return; // should not happen

  tt = localtime(&tmp);
  if (! tt) return; // should not happen either

  const ToolbarStyle * const style = screen->getToolbarStyle();

  bt::Rect u(0, 0, frame.clock_w, frame.label_h);
  if (frame.clk == ParentRelative) {
    int bw = style->toolbar.borderWidth();
    bt::Rect t(-(frame.rect.width() - frame.clock_w -
                 (frame.bevel_w * 2) - bw),
               -(bw + frame.bevel_w),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(screen->screenNumber(), style->toolbar,
                    frame.clock, t, u, frame.base);
  } else {
    bt::drawTexture(screen->screenNumber(), style->window,
                    frame.clock, u, u, frame.clk);
  }

  char str[1024];
  if (! strftime(str, 1024, screen->getStrftimeFormat(), tt)) return;

  bt::Pen pen(screen->screenNumber(), style->c_text);
  u.setCoords(u.left()  + frame.bevel_w, u.top()    + frame.bevel_w,
              u.right() - frame.bevel_w, u.bottom() - frame.bevel_w);
  bt::drawText(style->font, pen, frame.clock, u, style->alignment, str);
}


void Toolbar::redrawWindowLabel(void) {
  const ToolbarStyle * const style = screen->getToolbarStyle();

  bt::Rect u(0, 0, frame.window_label_w, frame.label_h);
  if (frame.label == ParentRelative) {
    int bw = style->toolbar.borderWidth();
    bt::Rect t(-(bw + ((frame.bevel_w * 4) + (frame.button_w * 2) +
                       frame.workspace_label_w + 3)),
               -(bw + frame.bevel_w),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(screen->screenNumber(), style->toolbar,
                    frame.window_label, t, u, frame.base);
  } else {
    bt::drawTexture(screen->screenNumber(), style->window,
                    frame.window_label, u, u, frame.label);
  }

  BlackboxWindow *foc = screen->getBlackbox()->getFocusedWindow();
  if (! foc || foc->getScreen() != screen) return;

  const char *title = foc->getTitle();
  bt::Pen pen(screen->screenNumber(), style->w_text);
  u.setCoords(u.left()  + frame.bevel_w, u.top()    + frame.bevel_w,
              u.right() - frame.bevel_w, u.bottom() - frame.bevel_w);
  bt::drawText(style->font, pen, frame.window_label, u,
               style->alignment, title);

}


void Toolbar::redrawWorkspaceLabel(void) {
  const std::string& name =
    screen->getWorkspaceName(screen->getCurrentWorkspaceID());
  const ToolbarStyle * const style = screen->getToolbarStyle();

  bt::Rect u(0, 0, frame.workspace_label_w, frame.label_h);
  if (frame.wlabel == ParentRelative) {
    int bw = style->toolbar.borderWidth();
    bt::Rect t(-(bw + frame.bevel_w), -(bw + frame.bevel_w),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(screen->screenNumber(), style->toolbar,
                    frame.workspace_label, t, u, frame.base);
  } else {
    bt::drawTexture(screen->screenNumber(), style->label,
                    frame.workspace_label, u, u, frame.wlabel);
  }

  bt::Pen pen(screen->screenNumber(), style->l_text);
  u.setCoords(u.left()  + frame.bevel_w, u.top()    + frame.bevel_w,
              u.right() - frame.bevel_w, u.bottom() - frame.bevel_w);
  bt::drawText(style->font, pen, frame.workspace_label, u,
               style->alignment, name);
}


void Toolbar::redrawPrevWorkspaceButton(bool pressed) {
  const ToolbarStyle * const style = screen->getToolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, frame.button_w, frame.button_w);
  if (p == ParentRelative) {
    int bw = style->toolbar.borderWidth();
    bt::Rect t(-(bw + ((frame.bevel_w * 2) + frame.workspace_label_w + 1)),
               -(bw + frame.bevel_w + 1),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(screen->screenNumber(), style->toolbar,
                    frame.psbutton, t, u, frame.base);
  } else {
    bt::drawTexture(screen->screenNumber(),
                    pressed ? style->pressed : style->button,
                    frame.psbutton, u, u, p);
  }

  int hh = frame.button_w / 2, hw = frame.button_w / 2;

  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh;
  pts[1].x = 4; pts[1].y = 2;
  pts[2].x = 0; pts[2].y = -4;

  bt::Pen pen(screen->screenNumber(), style->b_pic);
  XFillPolygon(display, frame.psbutton, pen.gc(),
               pts, 3, Convex, CoordModePrevious);
}


void Toolbar::redrawNextWorkspaceButton(bool pressed) {
  const ToolbarStyle * const style = screen->getToolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, frame.button_w, frame.button_w);
  if (p == ParentRelative) {
    int bw = style->toolbar.borderWidth();
    bt::Rect t(-(bw + ((frame.bevel_w * 3) + frame.workspace_label_w +
                       frame.button_w + 2)),
               -(bw + frame.bevel_w + 1),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(screen->screenNumber(), style->toolbar,
                    frame.nsbutton, t, u, frame.base);
  } else {
    bt::drawTexture(screen->screenNumber(),
                    pressed ? style->pressed : style->button,
                    frame.nsbutton, u, u, p);
  }

  int hh = frame.button_w / 2, hw = frame.button_w / 2;

  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh - 2;
  pts[1].x = 4; pts[1].y =  2;
  pts[2].x = -4; pts[2].y = 2;

  bt::Pen pen(screen->screenNumber(), style->b_pic);
  XFillPolygon(display, frame.nsbutton, pen.gc(),
               pts, 3, Convex, CoordModePrevious);
}


void Toolbar::redrawPrevWindowButton(bool pressed) {
  const ToolbarStyle * const style = screen->getToolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, frame.button_w, frame.button_w);
  if (p == ParentRelative) {
    int bw = style->toolbar.borderWidth();
    bt::Rect t(-(bw + ((frame.bevel_w * 5) + (frame.button_w * 2) +
                       frame.workspace_label_w + frame.window_label_w + 4)),
               -(bw + frame.bevel_w + 1),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(screen->screenNumber(), style->toolbar,
                    frame.pwbutton, t, u, frame.base);
  } else {
    bt::drawTexture(screen->screenNumber(),
                    pressed ? style->pressed : style->button,
                    frame.pwbutton, u, u, p);
  }

  int hh = frame.button_w / 2, hw = frame.button_w / 2;

  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh;
  pts[1].x = 4; pts[1].y = 2;
  pts[2].x = 0; pts[2].y = -4;

  bt::Pen pen(screen->screenNumber(), style->b_pic);
  XFillPolygon(display, frame.pwbutton, pen.gc(),
               pts, 3, Convex, CoordModePrevious);
}


void Toolbar::redrawNextWindowButton(bool pressed) {
  const ToolbarStyle * const style = screen->getToolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, frame.button_w, frame.button_w);
  if (p == ParentRelative) {
    int bw = style->toolbar.borderWidth();
    bt::Rect t(-(bw + ((frame.bevel_w * 6) + (frame.button_w * 3) +
                       frame.workspace_label_w + frame.window_label_w + 5)),
               -(bw + frame.bevel_w + 1),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(screen->screenNumber(), style->toolbar,
                    frame.nwbutton, t, u, frame.base);
  } else {
    bt::drawTexture(screen->screenNumber(),
                    pressed ? style->pressed : style->button,
                    frame.nwbutton, u, u, p);
  }

  int hh = frame.button_w / 2, hw = frame.button_w / 2;

  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh - 2;
  pts[1].x = 4; pts[1].y =  2;
  pts[2].x = -4; pts[2].y = 2;

  bt::Pen pen(screen->screenNumber(), style->b_pic);
  XFillPolygon(display, frame.nwbutton, pen.gc(),
               pts, 3, Convex, CoordModePrevious);
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
    blackbox->getFocusedWindow()->setFocusFlag(False);

  const ToolbarStyle * const style = screen->getToolbarStyle();
  bt::Pen pen(screen->screenNumber(), style->l_text);
  XDrawRectangle(display, frame.workspace_label, pen.gc(),
                 frame.workspace_label_w / 2, 0, 1,
                 frame.label_h - 1);
  // change the background of the window to that of an active window label
  bt::Texture texture = screen->getWindowStyle()->l_focus;
  frame.wlabel = bt::PixmapCache::find(screen->screenNumber(), texture,
                                       frame.workspace_label_w, frame.label_h,
                                       frame.wlabel);
  if (! frame.wlabel)
    XSetWindowBackground(display, frame.workspace_label,
                         texture.color().pixel(screen->screenNumber()));
  else
    XSetWindowBackgroundPixmap(display, frame.workspace_label, frame.wlabel);
}


void Toolbar::buttonPressEvent(const XButtonEvent *be) {
  if (be->button == 1) {
    if (be->window == frame.psbutton)
      redrawPrevWorkspaceButton(True);
    else if (be->window == frame.nsbutton)
      redrawNextWorkspaceButton(True);
    else if (be->window == frame.pwbutton)
      redrawPrevWindowButton(True);
    else if (be->window == frame.nwbutton)
      redrawNextWindowButton(True);
    else if (! on_top) {
      WindowStack w;
      w.push_back(frame.window);
      screen->raiseWindows(&w);
    }
  } else if (be->button == 2 && (! on_top)) {
    XLowerWindow(display, frame.window);
  } else if (be->button == 3) {
    int x = be->x_root, y;

    switch(screen->getToolbarPlacement()) {
    case TopLeft:
    case TopCenter:
    case TopRight:
      y = strut.top;
      break;

    default:
      y = screen->screenInfo().height() - strut.bottom;
      break;
    } // switch

    toolbarmenu->popup(x, y);
  }
}


void Toolbar::buttonReleaseEvent(const XButtonEvent *re) {
  if (re->button == 1) {
    if (re->window == frame.psbutton) {
      redrawPrevWorkspaceButton(False);

      if (re->x >= 0 && re->x < static_cast<signed>(frame.button_w) &&
          re->y >= 0 && re->y < static_cast<signed>(frame.button_w))
       if (screen->getCurrentWorkspaceID() > 0)
          screen->changeWorkspaceID(screen->getCurrentWorkspaceID() - 1);
        else
          screen->changeWorkspaceID(screen->getWorkspaceCount() - 1);
    } else if (re->window == frame.nsbutton) {
      redrawNextWorkspaceButton(False);

      if (re->x >= 0 && re->x < static_cast<signed>(frame.button_w) &&
          re->y >= 0 && re->y < static_cast<signed>(frame.button_w))
        if (screen->getCurrentWorkspaceID() < screen->getWorkspaceCount() - 1)
          screen->changeWorkspaceID(screen->getCurrentWorkspaceID() + 1);
        else
          screen->changeWorkspaceID(0);
    } else if (re->window == frame.pwbutton) {
      redrawPrevWindowButton(False);

      if (re->x >= 0 && re->x < static_cast<signed>(frame.button_w) &&
          re->y >= 0 && re->y < static_cast<signed>(frame.button_w))
        screen->prevFocus();
    } else if (re->window == frame.nwbutton) {
      redrawNextWindowButton(False);

      if (re->x >= 0 && re->x < static_cast<signed>(frame.button_w) &&
          re->y >= 0 && re->y < static_cast<signed>(frame.button_w))
        screen->nextFocus();
    } else if (re->window == frame.window_label)
      screen->raiseFocus();
  }
}


void Toolbar::enterNotifyEvent(const XCrossingEvent *) {
  if (! do_auto_hide)
    return;

  if (hidden) {
    if (! hide_timer->isTiming()) hide_timer->start();
  } else {
    if (hide_timer->isTiming()) hide_timer->stop();
  }
}

void Toolbar::leaveNotifyEvent(const XCrossingEvent *) {
  if (! do_auto_hide)
    return;

  if (hidden) {
    if (hide_timer->isTiming()) hide_timer->stop();
  } else {
    if (! hide_timer->isTiming()) hide_timer->start();
  }
}


void Toolbar::exposeEvent(const XExposeEvent *ee) {
  if (ee->window == frame.clock) redrawClockLabel();
  else if (ee->window == frame.workspace_label && (! editing))
    redrawWorkspaceLabel();
  else if (ee->window == frame.window_label) redrawWindowLabel();
  else if (ee->window == frame.psbutton) redrawPrevWorkspaceButton();
  else if (ee->window == frame.nsbutton) redrawNextWorkspaceButton();
  else if (ee->window == frame.pwbutton) redrawPrevWindowButton();
  else if (ee->window == frame.nwbutton) redrawNextWindowButton();
  else if (ee->window == frame.window) {
    bt::Rect t(0, 0, frame.rect.width(), frame.rect.height());
    bt::Rect r(ee->x, ee->y, ee->width, ee->height);
    bt::drawTexture(screen->screenNumber(), screen->getToolbarStyle()->toolbar,
                    frame.window, t, r & t, frame.base);
  }
}


void Toolbar::keyPressEvent(const XKeyEvent *ke) {
  if (ke->window == frame.workspace_label && editing) {
    if (new_workspace_name.empty()) {
      new_name_pos = 0;
    }

    const ToolbarStyle * const style = screen->getToolbarStyle();

    KeySym ks;
    char keychar[1];
    XLookupString(const_cast<XKeyEvent*>(ke), keychar, 1, &ks, 0);

    // either we are told to end with a return or we hit 127 chars
    if (ks == XK_Return || new_name_pos == 127) {
      editing = False;

      blackbox->setNoFocus(False);
      if (blackbox->getFocusedWindow())
        blackbox->getFocusedWindow()->setInputFocus();
      else
        blackbox->setFocusedWindow(0);

      screen->setWorkspaceName(screen->getCurrentWorkspaceID(),
                               new_workspace_name);

      screen->getWorkspacemenu()->
        changeItem(screen->getCurrentWorkspaceID(),
                   screen->getWorkspaceName(screen->getCurrentWorkspaceID()));
      screen->updateDesktopNamesHint();

      new_workspace_name.erase();
      new_name_pos = 0;

      // reset the background to that of the workspace label (its normal
      // setting)
      bt::Texture texture = style->label;
      frame.wlabel =
        bt::PixmapCache::find(screen->screenNumber(), texture,
                              frame.workspace_label_w, frame.label_h,
                              frame.wlabel);
      if (! frame.wlabel)
        XSetWindowBackground(display, frame.workspace_label,
                             texture.color().pixel(screen->screenNumber()));
      else
        XSetWindowBackgroundPixmap(display, frame.workspace_label,
                                   frame.wlabel);
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

      bt::Rect rect(frame.bevel_w, frame.bevel_w,
                    frame.workspace_label_w - (frame.bevel_w * 2),
                    frame.label_h - 2);
      bt::Rect textr = bt::textRect(style->font, new_workspace_name);
      bt::Alignment align = (textr.width() >= frame.workspace_label_w) ?
                            bt::AlignRight : style->alignment;

      bt::Pen pen(screen->screenNumber(), style->l_text);
      bt::drawText(style->font, pen, frame.workspace_label, rect, align,
                   new_workspace_name);

      // XDrawRectangle(display, frame.workspace_label, pen.gc(), x + tw, 0, 1,
      // frame.label_h - 1);
    }
  }
}


void Toolbar::timeout(bt::Timer *timer) {
  if (timer == clock_timer) {
    redrawClockLabel();

    clock_timer->setTimeout(aMinuteFromNow());
  } else if (timer == hide_timer) {
    hidden = !hidden;
    if (hidden)
      XMoveWindow(display, frame.window,
                  frame.x_hidden, frame.y_hidden);
    else
      XMoveWindow(display, frame.window,
                  frame.rect.x(), frame.rect.y());
  } else {
    // this should not happen
    assert(0);
  }
}


void Toolbar::toggleAutoHide(void) {
  do_auto_hide = (do_auto_hide) ?  False : True;

  updateStrut();
  screen->getSlit()->reposition();

  if (do_auto_hide == False && hidden) {
    // force the slit to be visible
    if (hide_timer->isTiming()) hide_timer->stop();
    hide_timer->fireTimeout();
  }
}


void Toolbar::toggleOnTop(void) {
  on_top = (! on_top);
  if (on_top) screen->raiseWindows((WindowStack *) 0);
}


void Toolbar::setPlacement(Placement place)
{
  screen->saveToolbarPlacement(place);
  reconfigure();

  // reposition the slit as well to make sure it doesn't intersect the
  // toolbar
  screen->getSlit()->reposition();
}
