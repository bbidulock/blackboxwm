// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Toolbar.hh for Blackbox - an X11 Window manager
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

#ifndef   __Toolbar_hh
#define   __Toolbar_hh

extern "C" {
#include <X11/Xlib.h>
}

#include "Screen.hh"
#include "Timer.hh"
#include "Util.hh"

// forward declaration
class Toolbarmenu;


class Toolbar : public bt::TimeoutHandler, public bt::EventHandler,
                public bt::NoCopy {
private:
  bool on_top, editing, hidden, do_auto_hide;
  Display *display;

  struct ToolbarFrame {
    unsigned long button_pixel, pbutton_pixel;
    Pixmap base, label, wlabel, clk, button, pbutton;
    Window window, workspace_label, window_label, clock, psbutton, nsbutton,
      pwbutton, nwbutton;

    int x_hidden, y_hidden;
    unsigned int window_label_w, workspace_label_w, clock_w,
      button_w, bevel_w, label_h;

    bt::Rect rect;
  } frame;

  Blackbox *blackbox;
  BScreen *screen;
  bt::Timer *clock_timer, *hide_timer;
  Toolbarmenu *toolbarmenu;
  bt::Netwm::Strut strut;

  std::string new_workspace_name;
  size_t new_name_pos;

  void redrawPrevWorkspaceButton(bool pressed = False);
  void redrawNextWorkspaceButton(bool pressed = False);
  void redrawPrevWindowButton(bool preseed = False);
  void redrawNextWindowButton(bool preseed = False);
  void redrawClockLabel(void);

  void updateStrut(void);


public:
  Toolbar(BScreen *scrn);
  virtual ~Toolbar(void);

  inline bool isEditing(void) const { return editing; }
  inline bool isOnTop(void) const { return on_top; }
  inline bool isHidden(void) const { return hidden; }
  inline bool doAutoHide(void) const { return do_auto_hide; }

  inline Window getWindowID(void) const { return frame.window; }

  inline const bt::Rect& getRect(void) const { return frame.rect; }
  inline unsigned int getWidth(void) const { return frame.rect.width(); }
  inline unsigned int getHeight(void) const { return frame.rect.height(); }
  inline unsigned int getExposedHeight(void) const
  { return ((do_auto_hide) ? frame.bevel_w : frame.rect.height()); }
  inline int getX(void) const
  { return ((hidden) ? frame.x_hidden : frame.rect.x()); }
  inline int getY(void) const
  { return ((hidden) ? frame.y_hidden : frame.rect.y()); }

  void buttonPressEvent(const XButtonEvent *be);
  void buttonReleaseEvent(const XButtonEvent *re);
  void enterNotifyEvent(const XCrossingEvent * /*unused*/);
  void leaveNotifyEvent(const XCrossingEvent * /*unused*/);
  void exposeEvent(const XExposeEvent *ee);
  void keyPressEvent(const XKeyEvent *ke);

  void edit(void);
  void reconfigure(void);
  void toggleAutoHide(void);
  void toggleOnTop(void);

  void redrawWindowLabel(void);
  void redrawWorkspaceLabel(void);

  virtual void timeout(bt::Timer *timer);

  enum Placement { TopLeft = 1, BottomLeft, TopCenter,
                   BottomCenter, TopRight, BottomRight };

  void setPlacement(Placement placement);
};


#endif // __Toolbar_hh
