// -*- mode: C++; indent-tabs-mode: nil; -*-
// BaseDisplay.hh for Blackbox - an X11 Window manager
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

#ifndef   __BaseDisplay_hh
#define   __BaseDisplay_hh

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <vector>

// forward declaration
class BaseDisplay;
class ScreenInfo;
class BGCCache;

#include "Timer.hh"


class BaseDisplay: public TimerQueueManager {
private:
  struct BShape {
    Bool extensions;
    int event_basep, error_basep;
  };
  BShape shape;

  unsigned int MaskList[8];
  size_t MaskListLength;

  Bool _startup, _shutdown;
  Display *display;
  mutable BGCCache *gccache;

  typedef std::vector<ScreenInfo> ScreenInfoList;
  ScreenInfoList screenInfoList;
  TimerQueue timerList;

  const char *display_name, *application_name;
  unsigned int number_of_screens;

  // no copying!
  BaseDisplay(const BaseDisplay &);
  BaseDisplay& operator=(const BaseDisplay&);

protected:
  // pure virtual function... you must override this
  virtual void process_event(XEvent *e) = 0;

  // the masks of the modifiers which are ignored in button events.
  int NumLockMask, ScrollLockMask;


public:
  BaseDisplay(const char *app_name, const char *dpy_name = 0);
  virtual ~BaseDisplay(void);

  const ScreenInfo* getScreenInfo(const unsigned int s) const;

  BGCCache *gcCache() const;

  inline Bool hasShapeExtensions(void) const
    { return shape.extensions; }
  inline Bool doShutdown(void) const
    { return _shutdown; }
  inline Bool isStartup(void) const
    { return _startup; }

  inline Display *getXDisplay(void) const { return display; }

  inline const char *getXDisplayName(void) const
    { return display_name; }
  inline const char *getApplicationName(void) const
    { return application_name; }

  inline unsigned int getNumberOfScreens(void) const
    { return number_of_screens; }
  inline int getShapeEventBase(void) const
    { return shape.event_basep; }

  inline void shutdown(void) { _shutdown = True; }
  inline void run(void) { _startup = _shutdown = False; }

  void grabButton(unsigned int button, unsigned int modifiers,
                  Window grab_window, Bool owner_events,
                  unsigned int event_mask, int pointer_mode,
                  int keyboard_mode, Window confine_to, Cursor cursor) const;
  void ungrabButton(unsigned int button, unsigned int modifiers,
                    Window grab_window) const;

  void eventLoop(void);

  // from TimerQueueManager interface
  virtual void addTimer(BTimer *timer);
  virtual void removeTimer(BTimer *timer);

  // another pure virtual... this is used to handle signals that BaseDisplay
  // doesn't understand itself
  virtual Bool handleSignal(int sig) = 0;
};


class ScreenInfo {
private:
  BaseDisplay *basedisplay;
  Visual *visual;
  Window root_window;
  Colormap colormap;

  int depth;
  unsigned int screen_number;
  XRectangle rect;

protected:
  const XRectangle& getRect(void) const { return rect; }

public:
  ScreenInfo(BaseDisplay *d, unsigned int num);

  inline BaseDisplay *getBaseDisplay(void) const { return basedisplay; }
  inline Visual *getVisual(void) const { return visual; }
  inline Window getRootWindow(void) const { return root_window; }
  inline Colormap getColormap(void) const { return colormap; }
  inline int getDepth(void) const { return depth; }
  inline unsigned int getScreenNumber(void) const
    { return screen_number; }
  inline unsigned short getWidth(void) const { return rect.width; }
  inline unsigned short getHeight(void) const { return rect.height; }
};


#endif // __BaseDisplay_hh
