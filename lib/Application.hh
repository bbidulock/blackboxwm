// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Application.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry <shaleh at debian.org>
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

#ifndef __Application_hh
#define __Application_hh

#include <map>
#include <string>

#include "Display.hh"
#include "Timer.hh"
#include "Util.hh"


namespace bt {
  // forward declaration
  class EventHandler;
  class Menu;

  class Application: public TimerQueueManager, public NoCopy {
  protected:
    enum RunState { STARTUP, RUNNING, SHUTDOWN, FATAL_SIGNAL };

  private:
    struct BShape {
      bool extensions;
      int event_basep, error_basep;
    };
    BShape shape;

    Display _display;
    std::string _app_name;

    RunState run_state;
    Time xserver_time;

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
    RunState runState(void) const { return run_state; }
    void setRunState(RunState new_state) { run_state = new_state; }

    virtual void startup(void);
    virtual void shutdown(void);

    /*
      Processes the X11 event {event} by delivering the event to the
      appropriate EventHandler.

      Reimplement this function if you need to filter/intercept events
      before normal processing.
    */
    virtual void process_event(XEvent *event);

    /*
      Processes the specified signal.  Returns true if the signal was
      handled; otherwise it returns false.

      Reimplement this function if you need to handle signals.
    */
    virtual bool process_signal(int signal);

  public:
    Application(const std::string &app_name, const char *dpy_name,
                bool multi_head);
    virtual ~Application(void);

    bool hasShapeExtensions(void) const
    { return shape.extensions; }

    bool startingUp(void) const
    { return run_state == STARTUP; }
    bool running(void) const
    { return run_state == RUNNING; }
    bool shuttingDown(void) const
    { return run_state == SHUTDOWN; }

    void quit(void) { setRunState( SHUTDOWN ); }

    ::Display *XDisplay(void) const { return _display.XDisplay(); }
    const Display& display(void) const { return _display; }

    const std::string &applicationName(void) const { return _app_name; }

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

#endif // __Application_hh
