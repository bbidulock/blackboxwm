// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Slit.hh for Blackbox - an X11 Window manager
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

#ifndef   __Slit_hh
#define   __Slit_hh

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
}

#include <list>

#include "Screen.hh"
#include "Util.hh"

// forward declaration
class Slitmenu;


class Slit : public bt::TimeoutHandler, public bt::EventHandler,
             public bt::NoCopy {
public:
  struct SlitClient {
    Window window, client_window, icon_window;

    bt::Rect rect;
  };

private:
  typedef std::list<SlitClient*> SlitClientList;

  bool on_top, hidden, do_auto_hide;
  Display *display;

  Blackbox *blackbox;
  BScreen *screen;
  bt::Timer *timer;
  bt::Netwm::Strut strut;

  SlitClientList clientList;
  Slitmenu *slitmenu;

  struct SlitFrame {
    Pixmap pixmap;
    Window window;

    int x_hidden, y_hidden;
    bt::Rect rect;
  } frame;

  void updateStrut(void);

  friend class Slitmenu;

public:
  Slit(BScreen *scr);
  virtual ~Slit(void);

  inline bool isOnTop(void) const { return on_top; }
  inline bool isHidden(void) const { return hidden; }
  inline bool doAutoHide(void) const { return do_auto_hide; }

  inline Window getWindowID(void) const { return frame.window; }

  inline int getX(void) const
  { return ((hidden) ? frame.x_hidden : frame.rect.x()); }
  inline int getY(void) const
  { return ((hidden) ? frame.y_hidden : frame.rect.y()); }

  inline unsigned int getWidth(void) const { return frame.rect.width(); }
  inline unsigned int getExposedWidth(void) const {
    if (screen->getSlitDirection() == Vertical && do_auto_hide)
      return screen->getBevelWidth();
    return frame.rect.width();
  }
  inline unsigned int getHeight(void) const { return frame.rect.height(); }
  inline unsigned int getExposedHeight(void) const {
    if (screen->getSlitDirection() == Horizontal && do_auto_hide)
      return screen->getBevelWidth();
    return frame.rect.height();
  }

  void addClient(Window w);
  void removeClient(SlitClient *client, bool remap = True);
  void removeClient(Window w, bool remap = True);
  void reconfigure(void);
  void updateSlit(void);
  void reposition(void);
  void shutdown(void);
  void toggleOnTop(void);
  void toggleAutoHide(void);

  // EventHandler interface
  void buttonPressEvent(const XButtonEvent * const e);
  void enterNotifyEvent(const XCrossingEvent * const /*unused*/);
  void leaveNotifyEvent(const XCrossingEvent * const /*unused*/);
  void configureRequestEvent(const XConfigureRequestEvent * const e);
  void unmapNotifyEvent(const XUnmapEvent * const e);
  void reparentNotifyEvent(const XReparentEvent * const event);

  virtual void timeout(bt::Timer *);

  enum Direction { Vertical = 1, Horizontal };
  enum Placement { TopLeft = 1, CenterLeft, BottomLeft,
                   TopCenter, BottomCenter,
                   TopRight, CenterRight, BottomRight };

  Direction direction(void) const;
  void setDirection(Direction new_direction);

  Placement placement(void) const;
  void setPlacement(Placement new_placement);
};


#endif // __Slit_hh
