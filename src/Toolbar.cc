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
#include "Window.hh"
#include "Windowmenu.hh"
#include "Workspacemenu.hh"

#include <Menu.hh>
#include <Pen.hh>
#include <PixmapCache.hh>

#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <sys/time.h>
#include <assert.h>


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


enum {
  Placement,
  AlwaysOnTop,
  AutoHide,
  EditWorkspaceName
};

Toolbarmenu::Toolbarmenu(bt::Application &app, unsigned int screen,
                         Toolbar *toolbar)
  : bt::Menu(app, screen), _toolbar(toolbar)
{
  ToolbarPlacementmenu *menu = new ToolbarPlacementmenu(app, screen, toolbar);
  insertItem("Placement", menu, Placement);
  insertSeparator();
  insertItem("Always on top", AlwaysOnTop);
  insertItem("Auto Hide", AutoHide);
  insertSeparator();
  insertItem("Edit current workspace name", EditWorkspaceName);
}


void Toolbarmenu::refresh(void) {
  setItemChecked(AlwaysOnTop, _toolbar->isOnTop());
  setItemChecked(AutoHide, _toolbar->doAutoHide());
}


void Toolbarmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  switch (id) {
  case AlwaysOnTop:
    _toolbar->toggleOnTop();
    break;

  case AutoHide:
    _toolbar->toggleAutoHide();
    break;

  case EditWorkspaceName:
    _toolbar->edit();
    break;

  default:
    break;
  } // switch
}


ToolbarPlacementmenu::ToolbarPlacementmenu(bt::Application &app,
                                           unsigned int screen,
                                           Toolbar *toolbar)
  : bt::Menu(app, screen), _toolbar(toolbar)
{
  insertItem("Top Left",      Toolbar::TopLeft);
  insertItem("Top Center",    Toolbar::TopCenter);
  insertItem("Top Right",     Toolbar::TopRight);
  insertSeparator();
  insertItem("Bottom Left",   Toolbar::BottomLeft);
  insertItem("Bottom Center", Toolbar::BottomCenter);
  insertItem("Bottom Right",  Toolbar::BottomRight);
}


void ToolbarPlacementmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  _toolbar->setPlacement((Toolbar::Placement) id);
}


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

  setLayer(isOnTop() ? StackingList::LayerAbove : StackingList::LayerNormal);
  hidden = doAutoHide();

  editing = False;
  new_name_pos = 0;

  toolbarmenu = new Toolbarmenu(*blackbox, _screen->screenNumber(), this);

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

  frame.base = frame.label = frame.wlabel = frame.clk = frame.button =
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
  ScreenResource& resource = _screen->resource();
  unsigned int height = 0,
                width = (_screen->screenInfo().width() *
                         resource.toolbarWidthPercent()) / 100;

  const ScreenResource::ToolbarStyle* const style = resource.toolbarStyle();
  height = bt::textHeight(_screen->screenNumber(), style->font);
  frame.bevel_w = resource.bevelWidth();
  frame.button_w = height;
  height += 2;
  frame.label_h = height;
  height += (frame.bevel_w * 2);

  unsigned int bw = style->toolbar.borderWidth();
  width  += bw * 2;
  height += bw * 2;

  frame.rect.setSize(width, height);

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

    frame.x_hidden = x;
    frame.y_hidden = resource.bevelWidth() - frame.rect.height();
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

    frame.x_hidden = x;
    frame.y_hidden = _screen->screenInfo().height() - resource.bevelWidth();
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
      frame.clock_w =
        bt::textRect(_screen->screenNumber(), style->font, t).width() +
        bt::textRect(_screen->screenNumber(), style->font, "ww").width();
    }
  }

  frame.workspace_label_w = 0;

  for (unsigned int i = 0; i < _screen->resource().numberOfWorkspaces(); i++) {
    width =
      bt::textRect(_screen->screenNumber(), style->font,
                   _screen->resource().nameOfWorkspace(i)).width();
    frame.workspace_label_w = std::max(frame.workspace_label_w, width);
  }

  frame.workspace_label_w = frame.clock_w =
    std::max(frame.workspace_label_w, frame.clock_w) + (frame.bevel_w * 4);

  frame.window_label_w =(frame.rect.width() - (bw * 2) -
                         (frame.clock_w + (frame.button_w * 4) +
                          frame.workspace_label_w + (frame.bevel_w * 8) + 7));

  if (hidden)
    XMoveResizeWindow(display, frame.window, frame.x_hidden, frame.y_hidden,
                      frame.rect.width(), frame.rect.height());
  else
    XMoveResizeWindow(display, frame.window, frame.rect.x(), frame.rect.y(),
                      frame.rect.width(), frame.rect.height());

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

  frame.base = bt::PixmapCache::find(_screen->screenNumber(), style->toolbar,
                                     frame.rect.width(), frame.rect.height(),
                                     frame.base);
  frame.label = bt::PixmapCache::find(_screen->screenNumber(), style->window,
                                      frame.window_label_w, frame.label_h,
                                      frame.label);
  frame.wlabel = bt::PixmapCache::find(_screen->screenNumber(), style->label,
                                       frame.workspace_label_w, frame.label_h,
                                       frame.wlabel);
  frame.clk = bt::PixmapCache::find(_screen->screenNumber(), style->clock,
                                    frame.clock_w, frame.label_h, frame.clk);
  frame.button = bt::PixmapCache::find(_screen->screenNumber(), style->button,
                                       frame.button_w, frame.button_w,
                                       frame.button);
  frame.pbutton = bt::PixmapCache::find(_screen->screenNumber(),
                                        style->pressed,
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
  time_t tmp = 0;
  struct tm *tt = 0;

  if ((tmp = time(NULL)) == -1) return; // should not happen

  tt = localtime(&tmp);
  if (! tt) return; // should not happen either

  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();

  bt::Rect u(0, 0, frame.clock_w, frame.label_h);
  if (frame.clk == ParentRelative) {
    int bw = style->toolbar.borderWidth();
    bt::Rect t(-(frame.rect.width() - frame.clock_w -
                 (frame.bevel_w * 2) - bw),
               -(bw + frame.bevel_w),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style->toolbar,
                    frame.clock, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(), style->window,
                    frame.clock, u, u, frame.clk);
  }

  char str[1024];
  if (! strftime(str, 1024, _screen->resource().strftimeFormat(), tt)) return;

  bt::Pen pen(_screen->screenNumber(), style->c_text);
  u.setCoords(u.left()  + frame.bevel_w, u.top()    + frame.bevel_w,
              u.right() - frame.bevel_w, u.bottom() - frame.bevel_w);
  bt::drawText(style->font, pen, frame.clock, u, style->alignment, str);
}


void Toolbar::redrawWindowLabel(void) {
  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();

  bt::Rect u(0, 0, frame.window_label_w, frame.label_h);
  if (frame.label == ParentRelative) {
    int bw = style->toolbar.borderWidth();
    bt::Rect t(-(bw + ((frame.bevel_w * 4) + (frame.button_w * 2) +
                       frame.workspace_label_w + 3)),
               -(bw + frame.bevel_w),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style->toolbar,
                    frame.window_label, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(), style->window,
                    frame.window_label, u, u, frame.label);
  }

  BlackboxWindow *foc = _screen->getBlackbox()->getFocusedWindow();
  if (! foc || foc->getScreen() != _screen) return;

  bt::Pen pen(_screen->screenNumber(), style->w_text);
  u.setCoords(u.left()  + frame.bevel_w, u.top()    + frame.bevel_w,
              u.right() - frame.bevel_w, u.bottom() - frame.bevel_w);
  bt::drawText(style->font, pen, frame.window_label, u,
               style->alignment, foc->getTitle());
}


void Toolbar::redrawWorkspaceLabel(void) {
  const std::string& name =
    _screen->resource().nameOfWorkspace(_screen->currentWorkspace());
  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();

  bt::Rect u(0, 0, frame.workspace_label_w, frame.label_h);
  if (frame.wlabel == ParentRelative) {
    int bw = style->toolbar.borderWidth();
    bt::Rect t(-(bw + frame.bevel_w), -(bw + frame.bevel_w),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style->toolbar,
                    frame.workspace_label, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(), style->label,
                    frame.workspace_label, u, u, frame.wlabel);
  }

  bt::Pen pen(_screen->screenNumber(), style->l_text);
  u.setCoords(u.left()  + frame.bevel_w, u.top()    + frame.bevel_w,
              u.right() - frame.bevel_w, u.bottom() - frame.bevel_w);
  bt::drawText(style->font, pen, frame.workspace_label, u,
               style->alignment, name);
}


void Toolbar::redrawPrevWorkspaceButton(bool pressed) {
  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, frame.button_w, frame.button_w);
  if (p == ParentRelative) {
    int bw = style->toolbar.borderWidth();
    bt::Rect t(-(bw + ((frame.bevel_w * 2) + frame.workspace_label_w + 1)),
               -(bw + frame.bevel_w + 1),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style->toolbar,
                    frame.psbutton, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    pressed ? style->pressed : style->button,
                    frame.psbutton, u, u, p);
  }

  bt::drawBitmap(bt::Bitmap::leftArrow(_screen->screenNumber()),
                 bt::Pen(_screen->screenNumber(), style->b_pic),
                 frame.psbutton, u);
}


void Toolbar::redrawNextWorkspaceButton(bool pressed) {
  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, frame.button_w, frame.button_w);
  if (p == ParentRelative) {
    int bw = style->toolbar.borderWidth();
    bt::Rect t(-(bw + ((frame.bevel_w * 3) + frame.workspace_label_w +
                       frame.button_w + 2)),
               -(bw + frame.bevel_w + 1),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style->toolbar,
                    frame.nsbutton, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    pressed ? style->pressed : style->button,
                    frame.nsbutton, u, u, p);
  }

  bt::drawBitmap(bt::Bitmap::rightArrow(_screen->screenNumber()),
                 bt::Pen(_screen->screenNumber(), style->b_pic),
                 frame.nsbutton, u);
}


void Toolbar::redrawPrevWindowButton(bool pressed) {
  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, frame.button_w, frame.button_w);
  if (p == ParentRelative) {
    int bw = style->toolbar.borderWidth();
    bt::Rect t(-(bw + ((frame.bevel_w * 5) + (frame.button_w * 2) +
                       frame.workspace_label_w + frame.window_label_w + 4)),
               -(bw + frame.bevel_w + 1),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style->toolbar,
                    frame.pwbutton, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    pressed ? style->pressed : style->button,
                    frame.pwbutton, u, u, p);
  }

  bt::drawBitmap(bt::Bitmap::leftArrow(_screen->screenNumber()),
                 bt::Pen(_screen->screenNumber(), style->b_pic),
                 frame.pwbutton, u);
}


void Toolbar::redrawNextWindowButton(bool pressed) {
  const ScreenResource::ToolbarStyle* const style =
    _screen->resource().toolbarStyle();

  Pixmap p = pressed ? frame.pbutton : frame.button;
  bt::Rect u(0, 0, frame.button_w, frame.button_w);
  if (p == ParentRelative) {
    int bw = style->toolbar.borderWidth();
    bt::Rect t(-(bw + ((frame.bevel_w * 6) + (frame.button_w * 3) +
                       frame.workspace_label_w + frame.window_label_w + 5)),
               -(bw + frame.bevel_w + 1),
               frame.rect.width(), frame.rect.height());
    bt::drawTexture(_screen->screenNumber(), style->toolbar,
                    frame.nwbutton, t, u, frame.base);
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    pressed ? style->pressed : style->button,
                    frame.nwbutton, u, u, p);
  }

  bt::drawBitmap(bt::Bitmap::rightArrow(_screen->screenNumber()),
                 bt::Pen(_screen->screenNumber(), style->b_pic),
                 frame.nwbutton, u);
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
  bt::Pen pen(_screen->screenNumber(), style->l_text);
  XDrawRectangle(pen.XDisplay(), frame.workspace_label, pen.gc(),
                 frame.workspace_label_w / 2, 0, 1,
                 frame.label_h - 1);
  // change the background of the window to that of an active window label
  bt::Texture texture = _screen->resource().windowStyle()->l_focus;
  frame.wlabel = bt::PixmapCache::find(_screen->screenNumber(), texture,
                                       frame.workspace_label_w, frame.label_h,
                                       frame.wlabel);
  if (! frame.wlabel)
    XSetWindowBackground(display, frame.workspace_label,
                         texture.color().pixel(_screen->screenNumber()));
  else
    XSetWindowBackgroundPixmap(display, frame.workspace_label, frame.wlabel);
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
      toolbarmenu->popup(event->x_root, event->y_root,
                         _screen->availableArea());
    }
  }
}


void Toolbar::buttonReleaseEvent(const XButtonEvent * const event) {
  if (event->button == 1) {
    if (event->window == frame.psbutton) {
      redrawPrevWorkspaceButton(False);

      if (bt::within(event->x, event->y, frame.button_w, frame.button_w)) {
        int new_workspace;
        if (_screen->currentWorkspace() > 0)
          new_workspace = _screen->currentWorkspace() - 1;
        else
          new_workspace = _screen->resource().numberOfWorkspaces() - 1;
        _screen->setCurrentWorkspace(new_workspace);
      }
    } else if (event->window == frame.nsbutton) {
      redrawNextWorkspaceButton(False);

      if (bt::within(event->x, event->y, frame.button_w, frame.button_w))
        if (_screen->currentWorkspace() <
            _screen->resource().numberOfWorkspaces() - 1)
          _screen->setCurrentWorkspace(_screen->currentWorkspace() + 1);
        else
          _screen->setCurrentWorkspace(0);
    } else if (event->window == frame.pwbutton) {
      redrawPrevWindowButton(False);

      if (bt::within(event->x, event->y, frame.button_w, frame.button_w))
        _screen->prevFocus();
    } else if (event->window == frame.nwbutton) {
      redrawNextWindowButton(False);

      if (bt::within(event->x, event->y, frame.button_w, frame.button_w))
        _screen->nextFocus();
    } else if (event->window == frame.window_label)
      _screen->raiseFocus();
  }
}


void Toolbar::enterNotifyEvent(const XCrossingEvent * const /*unused*/) {
  if (! doAutoHide())
    return;

  if (hidden) {
    if (! hide_timer->isTiming()) hide_timer->start();
  } else if (hide_timer->isTiming()) {
    hide_timer->stop();
  }
}

void Toolbar::leaveNotifyEvent(const XCrossingEvent * const /*unused*/) {
  if (! doAutoHide())
    return;

  if (hidden) {
    if (hide_timer->isTiming()) hide_timer->stop();
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
      bt::Texture texture = style->label;
      frame.wlabel =
        bt::PixmapCache::find(_screen->screenNumber(), texture,
                              frame.workspace_label_w, frame.label_h,
                              frame.wlabel);
      if (! frame.wlabel)
        XSetWindowBackground(display, frame.workspace_label,
                             texture.color().pixel(_screen->screenNumber()));
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
      bt::Rect textr =
        bt::textRect(_screen->screenNumber(), style->font, new_workspace_name);
      bt::Alignment align = (textr.width() >= frame.workspace_label_w) ?
                            bt::AlignRight : style->alignment;

      bt::Pen pen(_screen->screenNumber(), style->l_text);
      bt::drawText(style->font, pen, frame.workspace_label, rect, align,
                   new_workspace_name);

      // XDrawRectangle(pen.XDisplay(), frame.workspace_label, pen.gc(),
      //                x + tw, 0, 1, frame.label_h - 1);
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
  bool do_auto_hide = !doAutoHide();

  updateStrut();
  if (_screen->slit())
    _screen->slit()->reposition();

  if (!do_auto_hide && hidden) {
    // force the toolbar to be visible
    if (hide_timer->isTiming()) hide_timer->stop();
    hide_timer->fireTimeout();
  }
  _screen->resource().saveToolbarAutoHide(do_auto_hide);
  _screen->saveResource();
}


void Toolbar::toggleOnTop(void) {
  _screen->resource().saveToolbarOnTop(!isOnTop());
  _screen->saveResource();
  _screen->changeLayer(this, (isOnTop()
                              ? StackingList::LayerAbove
                              : StackingList::LayerNormal));
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
