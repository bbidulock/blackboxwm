// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// BaseDisplay.hh for Blackbox - an X11 Window manager
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

#ifndef   __BaseDisplay_hh
#define   __BaseDisplay_hh

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#ifdef    SHAPE
#  include <X11/extensions/shape.h>
#endif // SHAPE
}

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "Timer.hh"
#include "Util.hh"


namespace bt {

  // forward declaration
  class Display;
  class EventHandler;
  class Menu;

  class ScreenInfo: public NoCopy {
  private:
    Display& display;
    Visual *visual;
    Window root_window;
    Colormap colormap;

    int depth;
    unsigned int screen_number;
    std::string display_string;
    Rect rect;

  public:
    ScreenInfo(Display& d, unsigned int num);

    Display& getDisplay(void) const { return display; }
    Visual *getVisual(void) const { return visual; }
    Window getRootWindow(void) const { return root_window; }
    Colormap getColormap(void) const { return colormap; }
    int getDepth(void) const { return depth; }
    unsigned int getScreenNumber(void) const
    { return screen_number; }
    const Rect& getRect(void) const { return rect; }
    unsigned int getWidth(void) const { return rect.width(); }
    unsigned int getHeight(void) const { return rect.height(); }
    const std::string& displayString(void) const
    { return display_string; }
  };


  class Display: public NoCopy {
  private:
    ::Display *xdisplay;

    typedef std::vector<ScreenInfo*> ScreenInfoList;
    ScreenInfoList screenInfoList;

  public:
    Display(const char* dpy_name);
    ~Display(void);

    ::Display* XDisplay(void) const { return xdisplay; }
    unsigned int screenCount(void) const { return screenInfoList.size(); }
    const ScreenInfo* screenNumber(unsigned int i) const;
  };


  class Application: public TimerQueueManager, public NoCopy {
  private:
    struct BShape {
      bool extensions;
      int event_basep, error_basep;
    };
    BShape shape;

    Display display;

    enum RunState { STARTUP, RUNNING, SHUTDOWN };
    RunState run_state;
    Time xserver_time;

    const char *application_name;

    typedef std::map<Window,EventHandler*> EventHandlerMap;
    EventHandlerMap eventhandlers;

    TimerQueue timerList;

    typedef std::deque<Menu*> MenuStack;
    MenuStack menus;
    bool menu_grab;

    unsigned int MaskList[8];
    size_t MaskListLength;

    // the masks of the modifiers which are ignored in button events.
    unsigned int NumLockMask, ScrollLockMask;

  protected:
    /*
      Processes the X11 event {event} by delivering the event to the
      appropriate EventHandler.

      Reimplement this function if you need to filter/intercept events
      before normal processing.
    */
    virtual void process_event(XEvent *event);

  public:
    Application(const char *app_name, const char *dpy_name = 0);
    virtual ~Application(void);

    const ScreenInfo& getScreenInfo(unsigned int s) const
    { return *(display.screenNumber(s)); }

    bool hasShapeExtensions(void) const
    { return shape.extensions; }
    bool doShutdown(void) const
    { return run_state == SHUTDOWN; }
    bool isStartup(void) const
    { return run_state == STARTUP; }

    ::Display *getXDisplay(void) const { return display.XDisplay(); }
    const Display& getDisplay(void) const { return display; }

    const char *getApplicationName(void) const
    { return application_name; }

    unsigned int getNumberOfScreens(void) const
    { return display.screenCount(); }
    int getShapeEventBase(void) const
    { return shape.event_basep; }

    void shutdown(void) { run_state = SHUTDOWN; }
    void run(void) { run_state = RUNNING; }

    void grabButton(unsigned int button, unsigned int modifiers,
                    Window grab_window, bool owner_events,
                    unsigned int event_mask, int pointer_mode,
                    int keyboard_mode, Window confine_to, Cursor cursor,
                    bool allow_scroll_lock) const;
    void ungrabButton(unsigned int button, unsigned int modifiers,
                      Window grab_window) const;

    void eventLoop(void);

    unsigned int scrollLockMask(void) const { return ScrollLockMask; }
    unsigned int numLockMask(void) const { return NumLockMask; }

    // from TimerQueueManager interface
    virtual void addTimer(Timer *timer);
    virtual void removeTimer(Timer *timer);

    // another pure virtual... this is used to handle signals that
    // Display doesn't understand itself
    virtual bool handleSignal(int sig) = 0;

    /*
      Inserts the EventHandler {handler} for Window {window}.  All
      events generated for {window} will be sent through {handler}.
    */
    void insertEventHandler(Window window, EventHandler *handler);
    /*
      Removes all EventHandlers for Window {window}.
    */
    void removeEventHandler(Window window);

    /*
      Opens the specified menu as a popup, grabbing the pointer and
      keyboard if not already grabbed.

      The Menu class calls this function automatically.  You should
      never need to call this function.
    */
    void openMenu(Menu *menu);
    /*
      Closes the specified menu, ungrabbing the pointer and keyboard if
      it is the last popup menu.

      The Menu class calls this function automatically.  You should
      never need to call this function.
    */
    void closeMenu(Menu *menu);
  };

} // namespace bt

#endif // __BaseDisplay_hh
