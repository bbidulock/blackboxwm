// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Toolbar.hh for Blackbox - an X11 Window manager
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

#ifndef   __Toolbar_hh
#define   __Toolbar_hh

#include <Timer.hh>
#include <Util.hh>

#include "Screen.hh"

class Toolbar : public StackEntity, public bt::TimeoutHandler,
                public bt::EventHandler, public bt::NoCopy
{
private:
  bool hidden;
  Display *display;

  struct ToolbarFrame {
    unsigned long button_pixel, pbutton_pixel;
    Pixmap base, slabel, wlabel, clk, button, pbutton;
    Window window, workspace_label, window_label, clock, psbutton, nsbutton,
      pwbutton, nwbutton;

    int y_hidden;
    bt::Rect rect, slabel_rect, wlabel_rect, clock_rect, ps_rect, ns_rect,
      pw_rect, nw_rect;
  } frame;

  Blackbox *blackbox;
  BScreen *_screen;
  bt::Timer *clock_timer, *hide_timer;
  bt::EWMH::Strut strut;

  int clock_timer_resolution;

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

  bool isHidden(void) const { return hidden; }

  inline const bt::Rect &rect(void) const
  { return frame.rect; }

  unsigned int exposedHeight(void) const;

  // StackEntity interface
  Window windowID(void) const { return frame.window; }

  // TimeoutHandler interface
  void timeout(bt::Timer *timer);

  // EventHandler interface
  void buttonPressEvent(const XButtonEvent * const event);
  void buttonReleaseEvent(const XButtonEvent * const event);
  void enterNotifyEvent(const XCrossingEvent * const /*unused*/);
  void leaveNotifyEvent(const XCrossingEvent * const /*unused*/);
  void exposeEvent(const XExposeEvent * const event);

  void reconfigure(void);
  void toggleAutoHide(void);

  void redrawWindowLabel(void);
  void redrawWorkspaceLabel(void);

  enum Placement { TopLeft = 1, BottomLeft, TopCenter,
                   BottomCenter, TopRight, BottomRight };

  void setPlacement(Placement placement);
};

#endif // __Toolbar_hh
