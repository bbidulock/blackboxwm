// -*- mode: C++; indent-tabs-mode: nil; -*-
// Toolbar.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh@debian.org>
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <X11/keysym.h>

#ifdef HAVE_STRING_H
#  include <string.h>
#endif // HAVE_STRING_H

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

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
using std::string;

#include "i18n.hh"
#include "blackbox.hh"
#include "Clientmenu.hh"
#include "GCCache.hh"
#include "Iconmenu.hh"
#include "Image.hh"
#include "Rootmenu.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"
#include "Slit.hh"


static long aMinuteFromNow(void) {
  timeval now;
  gettimeofday(&now, 0);
  return ((60 - (now.tv_sec % 60)) * 1000);
}


Toolbar::Toolbar(BScreen *scrn) {
  screen = scrn;
  blackbox = screen->getBlackbox();

  // get the clock updating every minute
  clock_timer = new BTimer(blackbox, this);
  clock_timer->setTimeout(aMinuteFromNow());
  clock_timer->recurring(True);
  clock_timer->start();
  frame.minute = frame.hour = -1;

  hide_handler.toolbar = this;
  hide_timer = new BTimer(blackbox, &hide_handler);
  hide_timer->setTimeout(blackbox->getAutoRaiseDelay());

  on_top = screen->isToolbarOnTop();
  hidden = do_auto_hide = screen->doToolbarAutoHide();

  editing = False;
  new_name_pos = 0;
  frame.grab_x = frame.grab_y = 0;

  toolbarmenu = new Toolbarmenu(this);

  display = blackbox->getXDisplay();
  XSetWindowAttributes attrib;
  unsigned long create_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
                              CWColormap | CWOverrideRedirect | CWEventMask;
  attrib.background_pixmap = None;
  attrib.background_pixel = attrib.border_pixel =
    screen->getBorderColor()->pixel();
  attrib.colormap = screen->getColormap();
  attrib.override_redirect = True;
  attrib.event_mask = ButtonPressMask | ButtonReleaseMask |
                      EnterWindowMask | LeaveWindowMask;

  frame.window =
    XCreateWindow(display, screen->getRootWindow(), 0, 0, 1, 1, 0,
                  screen->getDepth(), InputOutput, screen->getVisual(),
                  create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.window, this);

  attrib.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask |
                      KeyPressMask | EnterWindowMask;

  frame.workspace_label =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
                  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.workspace_label, this);

  frame.window_label =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
                  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.window_label, this);

  frame.clock =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
                  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.clock, this);

  frame.psbutton =
    XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
                  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.psbutton, this);

  frame.nsbutton =
    XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
                  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.nsbutton, this);

  frame.pwbutton =
    XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
                  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.pwbutton, this);

  frame.nwbutton =
    XCreateWindow(display ,frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
                  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.nwbutton, this);

  frame.base = frame.label = frame.wlabel = frame.clk = frame.button =
    frame.pbutton = None;

  screen->addStrut(&strut);

  reconfigure();

  XMapSubwindows(display, frame.window);
  XMapWindow(display, frame.window);
}


Toolbar::~Toolbar(void) {
  XUnmapWindow(display, frame.window);

  if (frame.base) screen->getImageControl()->removeImage(frame.base);
  if (frame.label) screen->getImageControl()->removeImage(frame.label);
  if (frame.wlabel) screen->getImageControl()->removeImage(frame.wlabel);
  if (frame.clk) screen->getImageControl()->removeImage(frame.clk);
  if (frame.button) screen->getImageControl()->removeImage(frame.button);
  if (frame.pbutton) screen->getImageControl()->removeImage(frame.pbutton);

  blackbox->removeToolbarSearch(frame.window);
  blackbox->removeToolbarSearch(frame.workspace_label);
  blackbox->removeToolbarSearch(frame.window_label);
  blackbox->removeToolbarSearch(frame.clock);
  blackbox->removeToolbarSearch(frame.psbutton);
  blackbox->removeToolbarSearch(frame.nsbutton);
  blackbox->removeToolbarSearch(frame.pwbutton);
  blackbox->removeToolbarSearch(frame.nwbutton);

  XDestroyWindow(display, frame.workspace_label);
  XDestroyWindow(display, frame.window_label);
  XDestroyWindow(display, frame.clock);

  XDestroyWindow(display, frame.window);

  delete hide_timer;
  delete clock_timer;
  delete toolbarmenu;
}


void Toolbar::reconfigure(void) {
  frame.bevel_w = screen->getBevelWidth();
  frame.width = screen->getWidth() * screen->getToolbarWidthPercent() / 100;

  if (i18n.multibyte())
    frame.height =
      screen->getToolbarStyle()->fontset_extents->max_ink_extent.height;
  else
    frame.height = screen->getToolbarStyle()->font->ascent +
                   screen->getToolbarStyle()->font->descent;
  frame.button_w = frame.height;
  frame.height += 2;
  frame.label_h = frame.height;
  frame.height += (frame.bevel_w * 2);

  // left and right are always 0
  strut.top = strut.bottom = 0;

  switch (screen->getToolbarPlacement()) {
  case TopLeft:
    frame.x = 0;
    frame.y = 0;
    frame.x_hidden = 0;
    frame.y_hidden = screen->getBevelWidth() - screen->getBorderWidth()
                     - frame.height;
    break;

  case BottomLeft:
    frame.x = 0;
    frame.y = screen->getHeight() - frame.height
      - (screen->getBorderWidth() * 2);
    frame.x_hidden = 0;
    frame.y_hidden = screen->getHeight() - screen->getBevelWidth()
                     - screen->getBorderWidth();
    break;

  case TopCenter:
    frame.x = (screen->getWidth() - frame.width) / 2;
    frame.y = 0;
    frame.x_hidden = frame.x;
    frame.y_hidden = screen->getBevelWidth() - screen->getBorderWidth()
                     - frame.height;
    break;

  case BottomCenter:
  default:
    frame.x = (screen->getWidth() - frame.width) / 2;
    frame.y = screen->getHeight() - frame.height
      - (screen->getBorderWidth() * 2);
    frame.x_hidden = frame.x;
    frame.y_hidden = screen->getHeight() - screen->getBevelWidth()
                     - screen->getBorderWidth();
    break;

  case TopRight:
    frame.x = screen->getWidth() - frame.width
      - (screen->getBorderWidth() * 2);
    frame.y = 0;
    frame.x_hidden = frame.x;
    frame.y_hidden = screen->getBevelWidth() - screen->getBorderWidth()
                     - frame.height;
    break;

  case BottomRight:
    frame.x = screen->getWidth() - frame.width
      - (screen->getBorderWidth() * 2);
    frame.y = screen->getHeight() - frame.height
      - (screen->getBorderWidth() * 2);
    frame.x_hidden = frame.x;
    frame.y_hidden = screen->getHeight() - screen->getBevelWidth()
                     - screen->getBorderWidth();
    break;
  }

  switch(screen->getToolbarPlacement()) {
  case TopLeft:
  case TopCenter:
  case TopRight:
    strut.top = getHeight() + 1;
    break;
  default:
    strut.bottom = screen->getHeight() - getY() - 1;
  }

  screen->updateAvailableArea();

#ifdef    HAVE_STRFTIME
  time_t ttmp = time(NULL);
  struct tm *tt = 0;

  if (ttmp != -1) {
    tt = localtime(&ttmp);
    if (tt) {
      char t[1024];
      int len = strftime(t, 1024, screen->getStrftimeFormat(), tt);
      // XXX: check for len == 0 and handle it
      if (i18n.multibyte()) {
        XRectangle ink, logical;
        XmbTextExtents(screen->getToolbarStyle()->fontset, t, len,
                       &ink, &logical);
        frame.clock_w = logical.width;
      } else {
        frame.clock_w = XTextWidth(screen->getToolbarStyle()->font, t, len);
      }
      frame.clock_w += (frame.bevel_w * 4);
    } else {
      frame.clock_w = 0;
    }
  } else {
    frame.clock_w = 0;
  }
#else // !HAVE_STRFTIME
  frame.clock_w =
    XTextWidth(screen->getToolbarStyle()->font,
               i18n(ToolbarSet, ToolbarNoStrftimeLength, "00:00000"),
               strlen(i18n(ToolbarSet, ToolbarNoStrftimeLength,
                           "00:00000"))) + (frame.bevel_w * 4);
#endif // HAVE_STRFTIME

  unsigned int w = 0;
  frame.workspace_label_w = 0;

  for (unsigned int i = 0; i < screen->getWorkspaceCount(); i++) {
    const string& workspace_name = screen->getWorkspace(i)->getName();
    if (i18n.multibyte()) {
      XRectangle ink, logical;
      XmbTextExtents(screen->getToolbarStyle()->fontset,
                     workspace_name.c_str(), workspace_name.length(),
                     &ink, &logical);
      w = logical.width;
    } else {
      w = XTextWidth(screen->getToolbarStyle()->font,
                     workspace_name.c_str(), workspace_name.length());
    }
    w += (frame.bevel_w * 4);

    if (w > frame.workspace_label_w) frame.workspace_label_w = w;
  }

  if (frame.workspace_label_w < frame.clock_w)
    frame.workspace_label_w = frame.clock_w;
  else if (frame.workspace_label_w > frame.clock_w)
    frame.clock_w = frame.workspace_label_w;

  frame.window_label_w =
    (frame.width - (frame.clock_w + (frame.button_w * 4) +
                    frame.workspace_label_w + (frame.bevel_w * 8) + 6));

  if (hidden) {
    XMoveResizeWindow(display, frame.window, frame.x_hidden, frame.y_hidden,
                      frame.width, frame.height);
  } else {
    XMoveResizeWindow(display, frame.window, frame.x, frame.y,
                      frame.width, frame.height);
  }

  XMoveResizeWindow(display, frame.workspace_label, frame.bevel_w,
                    frame.bevel_w, frame.workspace_label_w,
                    frame.label_h);
  XMoveResizeWindow(display, frame.psbutton, (frame.bevel_w * 2) +
                    frame.workspace_label_w + 1, frame.bevel_w + 1,
                    frame.button_w, frame.button_w);
  XMoveResizeWindow(display ,frame.nsbutton, (frame.bevel_w * 3) +
                    frame.workspace_label_w + frame.button_w + 2,
                    frame.bevel_w + 1, frame.button_w, frame.button_w);
  XMoveResizeWindow(display, frame.window_label, (frame.bevel_w * 4) +
                    (frame.button_w * 2) + frame.workspace_label_w + 3,
                    frame.bevel_w, frame.window_label_w, frame.label_h);
  XMoveResizeWindow(display, frame.pwbutton, (frame.bevel_w * 5) +
                    (frame.button_w * 2) + frame.workspace_label_w +
                    frame.window_label_w + 4, frame.bevel_w + 1,
                    frame.button_w, frame.button_w);
  XMoveResizeWindow(display, frame.nwbutton, (frame.bevel_w * 6) +
                    (frame.button_w * 3) + frame.workspace_label_w +
                    frame.window_label_w + 5, frame.bevel_w + 1,
                    frame.button_w, frame.button_w);
  XMoveResizeWindow(display, frame.clock, frame.width - frame.clock_w -
                    frame.bevel_w, frame.bevel_w, frame.clock_w,
                    frame.label_h);

  ToolbarStyle *style = screen->getToolbarStyle();
  frame.base = style->toolbar.render(frame.width, frame.height, frame.base);
  if (! frame.base)
    XSetWindowBackground(display, frame.window,
                         style->toolbar.color().pixel());
  else
    XSetWindowBackgroundPixmap(display, frame.window, frame.base);

  frame.label = style->window.render(frame.window_label_w, frame.label_h,
                                     frame.label);
  if (! frame.label)
    XSetWindowBackground(display, frame.window_label,
                         style->window.color().pixel());
  else
    XSetWindowBackgroundPixmap(display, frame.window_label, frame.label);

  frame.wlabel = style->label.render(frame.workspace_label_w, frame.label_h,
                                frame.wlabel);
  if (! frame.wlabel)
    XSetWindowBackground(display, frame.workspace_label,
                         style->label.color().pixel());
  else
    XSetWindowBackgroundPixmap(display, frame.workspace_label, frame.wlabel);

  frame.clk = style->clock.render(frame.clock_w, frame.label_h, frame.clk);
  if (! frame.clk)
    XSetWindowBackground(display, frame.clock, style->clock.color().pixel());
  else
    XSetWindowBackgroundPixmap(display, frame.clock, frame.clk);

  frame.button = style->button.render(frame.button_w, frame.button_w,
                                frame.button);
  if (! frame.button) {
    frame.button_pixel = style->button.color().pixel();
    XSetWindowBackground(display, frame.psbutton, frame.button_pixel);
    XSetWindowBackground(display, frame.nsbutton, frame.button_pixel);
    XSetWindowBackground(display, frame.pwbutton, frame.button_pixel);
    XSetWindowBackground(display, frame.nwbutton, frame.button_pixel);
  } else {
    XSetWindowBackgroundPixmap(display, frame.psbutton, frame.button);
    XSetWindowBackgroundPixmap(display, frame.nsbutton, frame.button);
    XSetWindowBackgroundPixmap(display, frame.pwbutton, frame.button);
    XSetWindowBackgroundPixmap(display, frame.nwbutton, frame.button);
  }

  frame.pbutton = style->pressed.render(frame.button_w, frame.button_w,
                                        frame.pbutton);
  if (! frame.pbutton)
    frame.pbutton_pixel = style->pressed.color().pixel();

  XSetWindowBorder(display, frame.window,
                   screen->getBorderColor()->pixel());
  XSetWindowBorderWidth(display, frame.window, screen->getBorderWidth());

  XClearWindow(display, frame.window);
  XClearWindow(display, frame.workspace_label);
  XClearWindow(display, frame.window_label);
  XClearWindow(display, frame.clock);
  XClearWindow(display, frame.psbutton);
  XClearWindow(display, frame.nsbutton);
  XClearWindow(display, frame.pwbutton);
  XClearWindow(display, frame.nwbutton);

  redrawWindowLabel();
  redrawWorkspaceLabel();
  redrawPrevWorkspaceButton();
  redrawNextWorkspaceButton();
  redrawPrevWindowButton();
  redrawNextWindowButton();
  checkClock(True);

  toolbarmenu->reconfigure();
}


#ifdef    HAVE_STRFTIME
void Toolbar::checkClock(Bool redraw) {
#else // !HAVE_STRFTIME
void Toolbar::checkClock(Bool redraw, Bool date) {
#endif // HAVE_STRFTIME
  time_t tmp = 0;
  struct tm *tt = 0;

  if ((tmp = time(NULL)) != -1) {
    if (! (tt = localtime(&tmp))) return;
    if (tt->tm_min != frame.minute || tt->tm_hour != frame.hour) {
      frame.hour = tt->tm_hour;
      frame.minute = tt->tm_min;
      XClearWindow(display, frame.clock);
      redraw = True;
    }
  }

  if (redraw) {
#ifdef    HAVE_STRFTIME
    char t[1024];
    if (! strftime(t, 1024, screen->getStrftimeFormat(), tt))
      return;
#else // !HAVE_STRFTIME
    char t[9];
    if (date) {
      // format the date... with special consideration for y2k ;)
      if (screen->getDateFormat() == Blackbox::B_EuropeanDate)
        sprintf(t, 18n(ToolbarSet, ToolbarNoStrftimeDateFormatEu,
                       "%02d.%02d.%02d"),
                tt->tm_mday, tt->tm_mon + 1,
                (tt->tm_year >= 100) ? tt->tm_year - 100 : tt->tm_year);
      else
        sprintf(t, i18n(ToolbarSet, ToolbarNoStrftimeDateFormat,
                        "%02d/%02d/%02d"),
                tt->tm_mon + 1, tt->tm_mday,
                (tt->tm_year >= 100) ? tt->tm_year - 100 : tt->tm_year);
    } else {
      if (screen->isClock24Hour())
        sprintf(t, i18n(ToolbarSet, ToolbarNoStrftimeTimeFormat24,
                        "  %02d:%02d "),
                frame.hour, frame.minute);
      else
        sprintf(t, i18n(ToolbarSet, ToolbarNoStrftimeTimeFormat12,
                        "%02d:%02d %sm"),
                ((frame.hour > 12) ? frame.hour - 12 :
                 ((frame.hour == 0) ? 12 : frame.hour)), frame.minute,
                ((frame.hour >= 12) ?
                 i18n(ToolbarSet, ToolbarNoStrftimeTimeFormatP, "p") :
                 i18n(ToolbarSet, ToolbarNoStrftimeTimeFormatA, "a")));
    }
#endif // HAVE_STRFTIME

    int dx = (frame.bevel_w * 2), dlen = strlen(t);
    unsigned int l;

    if (i18n.multibyte()) {
      XRectangle ink, logical;
      XmbTextExtents(screen->getToolbarStyle()->fontset,
                     t, dlen, &ink, &logical);
      l = logical.width;
    } else {
      l = XTextWidth(screen->getToolbarStyle()->font, t, dlen);
    }

    l += (frame.bevel_w * 4);

    if (l > frame.clock_w) {
      for (; dlen >= 0; dlen--) {
        if (i18n.multibyte()) {
          XRectangle ink, logical;
          XmbTextExtents(screen->getToolbarStyle()->fontset,
                         t, dlen, &ink, &logical);
          l = logical.width;
        } else {
          l = XTextWidth(screen->getToolbarStyle()->font, t, dlen);
        }
        l+= (frame.bevel_w * 4);

        if (l < frame.clock_w)
          break;
      }
    }
    switch (screen->getToolbarStyle()->justify) {
    case BScreen::RightJustify:
      dx += frame.clock_w - l;
      break;

    case BScreen::CenterJustify:
      dx += (frame.clock_w - l) / 2;
      break;
    }

    ToolbarStyle *style = screen->getToolbarStyle();
    BPen pen(style->c_text, style->font);
    if (i18n.multibyte())
      XmbDrawString(display, frame.clock, style->fontset, pen.gc(),
                    dx, (1 - style->fontset_extents->max_ink_extent.y),
                    t, dlen);
    else
      XDrawString(display, frame.clock, pen.gc(), dx,
                  (style->font->ascent + 1), t, dlen);
  }
}


void Toolbar::redrawWindowLabel(Bool redraw) {
  BlackboxWindow *foc = screen->getBlackbox()->getFocusedWindow();
  if (! foc) {
    XClearWindow(display, frame.window_label);
    return;
  }

  if (redraw)
    XClearWindow(display, frame.window_label);

  if (foc->getScreen() != screen) return;

  const char *title = foc->getTitle();
  int dx = (frame.bevel_w * 2), dlen = strlen(title);
  unsigned int l;

  if (i18n.multibyte()) {
    XRectangle ink, logical;
    XmbTextExtents(screen->getToolbarStyle()->fontset, title, dlen,
                   &ink, &logical);
    l = logical.width;
  } else {
    l = XTextWidth(screen->getToolbarStyle()->font, title, dlen);
  }
  l += (frame.bevel_w * 4);

  if (l > frame.window_label_w) {
    for (; dlen >= 0; dlen--) {
      if (i18n.multibyte()) {
        XRectangle ink, logical;
        XmbTextExtents(screen->getToolbarStyle()->fontset, title, dlen,
                       &ink, &logical);
        l = logical.width;
      } else {
        l = XTextWidth(screen->getToolbarStyle()->font, title, dlen);
      }
      l += (frame.bevel_w * 4);

      if (l < frame.window_label_w)
        break;
    }
  }
  switch (screen->getToolbarStyle()->justify) {
  case BScreen::RightJustify:
    dx += frame.window_label_w - l;
    break;

  case BScreen::CenterJustify:
    dx += (frame.window_label_w - l) / 2;
    break;
  }

  ToolbarStyle *style = screen->getToolbarStyle();
  BPen pen(style->w_text, style->font);
  if (i18n.multibyte())
    XmbDrawString(display, frame.window_label, style->fontset, pen.gc(), dx,
                  (1 - style->fontset_extents->max_ink_extent.y),
                  title, dlen);
  else
    XDrawString(display, frame.window_label, pen.gc(), dx,
                (style->font->ascent + 1), title, dlen);
}


void Toolbar::redrawWorkspaceLabel(Bool redraw) {
  const string& name = screen->getCurrentWorkspace()->getName();

  if (redraw)
    XClearWindow(display, frame.workspace_label);

  int dx = (frame.bevel_w * 2), dlen = name.length();
  unsigned int l;

  if (i18n.multibyte()) {
    XRectangle ink, logical;
    XmbTextExtents(screen->getToolbarStyle()->fontset, name.c_str(),
                   dlen, &ink, &logical);
    l = logical.width;
  } else {
    l = XTextWidth(screen->getToolbarStyle()->font, name.c_str(), dlen);
  }
  l += (frame.bevel_w * 4);

  if (l > frame.workspace_label_w) {
    for (; dlen >= 0; dlen--) {
      if (i18n.multibyte()) {
        XRectangle ink, logical;
        XmbTextExtents(screen->getToolbarStyle()->fontset, name.c_str(), dlen,
                       &ink, &logical);
        l = logical.width;
      } else {
        l = XTextWidth(screen->getWindowStyle()->font, name.c_str(), dlen);
      }
      l += (frame.bevel_w * 4);

      if (l < frame.workspace_label_w)
        break;
    }
  }
  switch (screen->getToolbarStyle()->justify) {
  case BScreen::RightJustify:
    dx += frame.workspace_label_w - l;
    break;

  case BScreen::CenterJustify:
    dx += (frame.workspace_label_w - l) / 2;
    break;
  }

  ToolbarStyle *style = screen->getToolbarStyle();
  BPen pen(style->l_text, style->font);
  if (i18n.multibyte())
    XmbDrawString(display, frame.workspace_label, style->fontset, pen.gc(), dx,
                  (1 - style->fontset_extents->max_ink_extent.y),
                  name.c_str(), dlen);
  else
    XDrawString(display, frame.workspace_label, pen.gc(), dx,
                (style->font->ascent + 1),
                name.c_str(), dlen);
}


void Toolbar::redrawPrevWorkspaceButton(Bool pressed, Bool redraw) {
  if (redraw) {
    if (pressed) {
      if (frame.pbutton)
        XSetWindowBackgroundPixmap(display, frame.psbutton, frame.pbutton);
      else
        XSetWindowBackground(display, frame.psbutton, frame.pbutton_pixel);
    } else {
      if (frame.button)
        XSetWindowBackgroundPixmap(display, frame.psbutton, frame.button);
      else
        XSetWindowBackground(display, frame.psbutton, frame.button_pixel);
    }
    XClearWindow(display, frame.psbutton);
  }

  int hh = frame.button_w / 2, hw = frame.button_w / 2;

  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh;
  pts[1].x = 4; pts[1].y = 2;
  pts[2].x = 0; pts[2].y = -4;

  ToolbarStyle *style = screen->getToolbarStyle();
  BPen pen(style->b_pic, style->font);
  XFillPolygon(display, frame.psbutton, pen.gc(),
               pts, 3, Convex, CoordModePrevious);
}


void Toolbar::redrawNextWorkspaceButton(Bool pressed, Bool redraw) {
  if (redraw) {
    if (pressed) {
      if (frame.pbutton)
        XSetWindowBackgroundPixmap(display, frame.nsbutton, frame.pbutton);
      else
        XSetWindowBackground(display, frame.nsbutton, frame.pbutton_pixel);
    } else {
      if (frame.button)
        XSetWindowBackgroundPixmap(display, frame.nsbutton, frame.button);
      else
        XSetWindowBackground(display, frame.nsbutton, frame.button_pixel);
    }
    XClearWindow(display, frame.nsbutton);
  }

  int hh = frame.button_w / 2, hw = frame.button_w / 2;

  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh - 2;
  pts[1].x = 4; pts[1].y =  2;
  pts[2].x = -4; pts[2].y = 2;

  ToolbarStyle *style = screen->getToolbarStyle();
  BPen pen(style->b_pic, style->font);
  XFillPolygon(display, frame.nsbutton, pen.gc(),
               pts, 3, Convex, CoordModePrevious);
}


void Toolbar::redrawPrevWindowButton(Bool pressed, Bool redraw) {
  if (redraw) {
    if (pressed) {
      if (frame.pbutton)
        XSetWindowBackgroundPixmap(display, frame.pwbutton, frame.pbutton);
      else
        XSetWindowBackground(display, frame.pwbutton, frame.pbutton_pixel);
    } else {
      if (frame.button)
        XSetWindowBackgroundPixmap(display, frame.pwbutton, frame.button);
      else
        XSetWindowBackground(display, frame.pwbutton, frame.button_pixel);
    }
    XClearWindow(display, frame.pwbutton);
  }

  int hh = frame.button_w / 2, hw = frame.button_w / 2;

  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh;
  pts[1].x = 4; pts[1].y = 2;
  pts[2].x = 0; pts[2].y = -4;

  ToolbarStyle *style = screen->getToolbarStyle();
  BPen pen(style->b_pic, style->font);
  XFillPolygon(display, frame.pwbutton, pen.gc(),
               pts, 3, Convex, CoordModePrevious);
}


void Toolbar::redrawNextWindowButton(Bool pressed, Bool redraw) {
  if (redraw) {
    if (pressed) {
      if (frame.pbutton)
        XSetWindowBackgroundPixmap(display, frame.nwbutton, frame.pbutton);
      else
        XSetWindowBackground(display, frame.nwbutton, frame.pbutton_pixel);
    } else {
      if (frame.button)
        XSetWindowBackgroundPixmap(display, frame.nwbutton, frame.button);
      else
        XSetWindowBackground(display, frame.nwbutton, frame.button_pixel);
    }
    XClearWindow(display, frame.nwbutton);
  }

  int hh = frame.button_w / 2, hw = frame.button_w / 2;

  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh - 2;
  pts[1].x = 4; pts[1].y =  2;
  pts[2].x = -4; pts[2].y = 2;

  ToolbarStyle *style = screen->getToolbarStyle();
  BPen pen(style->b_pic, style->font);
  XFillPolygon(display, frame.nwbutton, pen.gc(), pts, 3, Convex,
               CoordModePrevious);
}


void Toolbar::edit(void) {
  Window window;
  int foo;

  editing = True;
  XGetInputFocus(display, &window, &foo);
  if (window == frame.workspace_label)
    return;

  XSetInputFocus(display, frame.workspace_label,
                 ((screen->isSloppyFocus()) ? RevertToPointerRoot :
                  RevertToParent), CurrentTime);
  XClearWindow(display, frame.workspace_label);

  blackbox->setNoFocus(True);
  if (blackbox->getFocusedWindow())
    blackbox->getFocusedWindow()->setFocusFlag(False);

  ToolbarStyle *style = screen->getToolbarStyle();
  BPen pen(style->l_text, style->font);
  XDrawRectangle(display, frame.workspace_label, pen.gc(),
                 frame.workspace_label_w / 2, 0, 1,
                 frame.label_h - 1);
  // change the background of the window to that of an active window label
  BTexture *texture = &(screen->getWindowStyle()->l_focus);
  frame.wlabel = texture->render(frame.workspace_label_w, frame.label_h,
                                 frame.wlabel);
  if (! frame.wlabel)
    XSetWindowBackground(display, frame.workspace_label,
                         texture->color().pixel());
  else
    XSetWindowBackgroundPixmap(display, frame.workspace_label, frame.wlabel);
}


void Toolbar::buttonPressEvent(XButtonEvent *be) {
  if (be->button == 1) {
    if (be->window == frame.psbutton)
      redrawPrevWorkspaceButton(True, True);
    else if (be->window == frame.nsbutton)
      redrawNextWorkspaceButton(True, True);
    else if (be->window == frame.pwbutton)
      redrawPrevWindowButton(True, True);
    else if (be->window == frame.nwbutton)
      redrawNextWindowButton(True, True);
#ifndef   HAVE_STRFTIME
    else if (be->window == frame.clock) {
      XClearWindow(display, frame.clock);
      checkClock(True, True);
    }
#endif // HAVE_STRFTIME
    else if (! on_top) {
      Window w[1] = { frame.window };
      screen->raiseWindows(w, 1);
    }
  } else if (be->button == 2 && (! on_top)) {
    XLowerWindow(display, frame.window);
  } else if (be->button == 3) {
    if (toolbarmenu->isVisible()) {
      toolbarmenu->hide();
    } else {
      int x, y;

      x = be->x_root - (toolbarmenu->getWidth() / 2);
      y = be->y_root - (toolbarmenu->getHeight() / 2);

      if (x < 0)
        x = 0;
      else if (x + toolbarmenu->getWidth() > screen->getWidth())
        x = screen->getWidth() - toolbarmenu->getWidth();

      if (y < 0)
        y = 0;
      else if (y + toolbarmenu->getHeight() > screen->getHeight())
        y = screen->getHeight() - toolbarmenu->getHeight();

      toolbarmenu->move(x, y);
      toolbarmenu->show();
    }
  }
}



void Toolbar::buttonReleaseEvent(XButtonEvent *re) {
  if (re->button == 1) {
    if (re->window == frame.psbutton) {
      redrawPrevWorkspaceButton(False, True);

      if (re->x >= 0 && re->x < static_cast<signed>(frame.button_w) &&
          re->y >= 0 && re->y < static_cast<signed>(frame.button_w))
       if (screen->getCurrentWorkspace()->getID() > 0)
          screen->changeWorkspaceID(screen->getCurrentWorkspace()->
                                    getID() - 1);
        else
          screen->changeWorkspaceID(screen->getWorkspaceCount() - 1);
    } else if (re->window == frame.nsbutton) {
      redrawNextWorkspaceButton(False, True);

      if (re->x >= 0 && re->x < static_cast<signed>(frame.button_w) &&
          re->y >= 0 && re->y < static_cast<signed>(frame.button_w))
        if (screen->getCurrentWorkspace()->getID() <
            (screen->getWorkspaceCount() - 1))
          screen->changeWorkspaceID(screen->getCurrentWorkspace()->
                                    getID() + 1);
        else
          screen->changeWorkspaceID(0);
    } else if (re->window == frame.pwbutton) {
      redrawPrevWindowButton(False, True);

      if (re->x >= 0 && re->x < static_cast<signed>(frame.button_w) &&
          re->y >= 0 && re->y < static_cast<signed>(frame.button_w))
        screen->prevFocus();
    } else if (re->window == frame.nwbutton) {
      redrawNextWindowButton(False, True);

      if (re->x >= 0 && re->x < static_cast<signed>(frame.button_w) &&
          re->y >= 0 && re->y < static_cast<signed>(frame.button_w))
        screen->nextFocus();
    } else if (re->window == frame.window_label)
      screen->raiseFocus();
#ifndef   HAVE_STRFTIME
    else if (re->window == frame.clock) {
      XClearWindow(display, frame.clock);
      checkClock(True);
    }
#endif // HAVE_STRFTIME
  }
}


void Toolbar::enterNotifyEvent(XCrossingEvent *) {
  if (! do_auto_hide)
    return;

  if (hidden) {
    if (! hide_timer->isTiming()) hide_timer->start();
  } else {
    if (hide_timer->isTiming()) hide_timer->stop();
  }
}

void Toolbar::leaveNotifyEvent(XCrossingEvent *) {
  if (! do_auto_hide)
    return;

  if (hidden) {
    if (hide_timer->isTiming()) hide_timer->stop();
  } else if (! toolbarmenu->isVisible()) {
    if (! hide_timer->isTiming()) hide_timer->start();
  }
}


void Toolbar::exposeEvent(XExposeEvent *ee) {
  if (ee->window == frame.clock) checkClock(True);
  else if (ee->window == frame.workspace_label && (! editing))
    redrawWorkspaceLabel();
  else if (ee->window == frame.window_label) redrawWindowLabel();
  else if (ee->window == frame.psbutton) redrawPrevWorkspaceButton();
  else if (ee->window == frame.nsbutton) redrawNextWorkspaceButton();
  else if (ee->window == frame.pwbutton) redrawPrevWindowButton();
  else if (ee->window == frame.nwbutton) redrawNextWindowButton();
}


void Toolbar::keyPressEvent(XKeyEvent *ke) {
  if (ke->window == frame.workspace_label && editing) {
    if (new_workspace_name.empty()) {
      new_name_pos = 0;
    }

    KeySym ks;
    char keychar[1];
    XLookupString(ke, keychar, 1, &ks, 0);

    // either we are told to end with a return or we hit 127 chars
    if (ks == XK_Return || new_name_pos == 127) {
      editing = False;

      blackbox->setNoFocus(False);
      if (blackbox->getFocusedWindow()) {
        blackbox->getFocusedWindow()->setInputFocus();
      } else {
        blackbox->setFocusedWindow(0);
      }

      Workspace *wkspc = screen->getCurrentWorkspace();
      wkspc->setName(new_workspace_name);
      wkspc->getMenu()->hide();

      screen->getWorkspacemenu()->changeItemLabel(wkspc->getID() + 2,
                                                  wkspc->getName());
      screen->getWorkspacemenu()->update();

      new_workspace_name.erase();
      new_name_pos = 0;

      // reset the background to that of the workspace label (its normal
      // setting)
      BTexture *texture = &(screen->getToolbarStyle()->label);
      frame.wlabel = texture->render(frame.workspace_label_w, frame.label_h,
                                     frame.wlabel);
      if (! frame.wlabel)
        XSetWindowBackground(display, frame.workspace_label,
                             texture->color().pixel());
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
      unsigned int l = new_workspace_name.length(), tw, x;

      if (i18n.multibyte()) {
        XRectangle ink, logical;
        XmbTextExtents(screen->getToolbarStyle()->fontset,
                       new_workspace_name.c_str(), l, &ink, &logical);
        tw = logical.width;
      } else {
        tw = XTextWidth(screen->getToolbarStyle()->font,
                        new_workspace_name.c_str(), l);
      }
      x = (frame.workspace_label_w - tw) / 2;

      if (x < frame.bevel_w) x = frame.bevel_w;

      ToolbarStyle *style = screen->getToolbarStyle();
      BPen pen(style->l_text, style->font);
      if (i18n.multibyte())
        XmbDrawString(display, frame.workspace_label, style->fontset,
                      pen.gc(), x,
                      (1 - style->fontset_extents->max_ink_extent.y),
                      new_workspace_name.c_str(), l);
      else
        XDrawString(display, frame.workspace_label, pen.gc(), x,
                    (style->font->ascent + 1),
                    new_workspace_name.c_str(), l);
      XDrawRectangle(display, frame.workspace_label, pen.gc(), x + tw, 0, 1,
                     frame.label_h - 1);
    }
  }
}


void Toolbar::timeout(void) {
  checkClock(True);

  clock_timer->setTimeout(aMinuteFromNow());
}


void Toolbar::HideHandler::timeout(void) {
  toolbar->hidden = ! toolbar->hidden;
  if (toolbar->hidden)
    XMoveWindow(toolbar->display, toolbar->frame.window,
                toolbar->frame.x_hidden, toolbar->frame.y_hidden);
  else
    XMoveWindow(toolbar->display, toolbar->frame.window,
                toolbar->frame.x, toolbar->frame.y);
}


Toolbarmenu::Toolbarmenu(Toolbar *tb) : Basemenu(tb->screen) {
  toolbar = tb;

  setLabel(i18n(ToolbarSet, ToolbarToolbarTitle, "Toolbar"));
  setInternalMenu();

  placementmenu = new Placementmenu(this);

  insert(i18n(CommonSet, CommonPlacementTitle, "Placement"),
         placementmenu);
  insert(i18n(CommonSet, CommonAlwaysOnTop, "Always on top"), 1);
  insert(i18n(CommonSet, CommonAutoHide, "Auto hide"), 2);
  insert(i18n(ToolbarSet, ToolbarEditWkspcName,
              "Edit current workspace name"), 3);

  update();

  if (toolbar->isOnTop()) setItemSelected(1, True);
  if (toolbar->doAutoHide()) setItemSelected(2, True);
}


Toolbarmenu::~Toolbarmenu(void) {
  delete placementmenu;
}


void Toolbarmenu::itemSelected(int button, int index) {
  if (button != 1)
    return;

  BasemenuItem *item = find(index);
  if (! item) return;

  switch (item->function()) {
  case 1: { // always on top
    Bool change = ((toolbar->isOnTop()) ? False : True);
    toolbar->on_top = change;
    setItemSelected(1, change);

    if (toolbar->isOnTop()) getScreen()->raiseWindows((Window *) 0, 0);
    break;
  }

  case 2: { // auto hide
    Bool change = ((toolbar->doAutoHide()) ?  False : True);
    toolbar->do_auto_hide = change;
    setItemSelected(2, change);

    getScreen()->getSlit()->reposition();
    break;
  }

  case 3: { // edit current workspace name
    toolbar->edit();
    hide();

    break;
  }
  } // switch
}


void Toolbarmenu::internal_hide(void) {
  Basemenu::internal_hide();
  if (toolbar->doAutoHide() && ! toolbar->isEditing())
    toolbar->hide_handler.timeout();
}


void Toolbarmenu::reconfigure(void) {
  placementmenu->reconfigure();

  Basemenu::reconfigure();
}


Toolbarmenu::Placementmenu::Placementmenu(Toolbarmenu *tm)
  : Basemenu(tm->toolbar->screen) {
  setLabel(i18n(ToolbarSet, ToolbarToolbarPlacement, "Toolbar Placement"));
  setInternalMenu();
  setMinimumSublevels(3);

  insert(i18n(CommonSet, CommonPlacementTopLeft, "Top Left"),
         Toolbar::TopLeft);
  insert(i18n(CommonSet, CommonPlacementBottomLeft, "Bottom Left"),
         Toolbar::BottomLeft);
  insert(i18n(CommonSet, CommonPlacementTopCenter, "Top Center"),
         Toolbar::TopCenter);
  insert(i18n(CommonSet, CommonPlacementBottomCenter, "Bottom Center"),
         Toolbar::BottomCenter);
  insert(i18n(CommonSet, CommonPlacementTopRight, "Top Right"),
         Toolbar::TopRight);
  insert(i18n(CommonSet, CommonPlacementBottomRight, "Bottom Right"),
         Toolbar::BottomRight);
  update();
}


void Toolbarmenu::Placementmenu::itemSelected(int button, int index) {
  if (button != 1)
    return;

  BasemenuItem *item = find(index);
  if (! item) return;

  getScreen()->saveToolbarPlacement(item->function());
  hide();
  getScreen()->getToolbar()->reconfigure();

  // reposition the slit as well to make sure it doesn't intersect the
  // toolbar
  getScreen()->getSlit()->reposition();
}
