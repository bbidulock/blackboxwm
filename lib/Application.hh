// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Application.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh at debian.org>
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

#ifndef __Application_hh
#define __Application_hh

#include "Timer.hh"
#include "Util.hh"

#include <map>

namespace bt {

  // forward declarations
  class Display;
  class EventHandler;
  class Menu;

  /*
    The application object.  It provides event delivery, timer
    activation and signal handling functionality to fit most
    application needs.
  */
  class Application : public TimerQueueManager, public NoCopy {
  protected:
    enum RunState { STARTUP, RUNNING, SHUTDOWN, SIGNALLED };

  private:
    struct {
      bool extensions;
      int event_basep, error_basep;
    } shape;

    Display *_display;
    std::string _app_name;

    RunState run_state;
    Time xserver_time;

    typedef std::map<Window,EventHandler*> EventHandlerMap;
    EventHandlerMap eventhandlers;

    timeval currentTime;
    TimerQueue timerList;
    void adjustTimers(const timeval &offset);

    typedef std::deque<Menu*> MenuStack;
    MenuStack menus;
    bool menu_grab;
    void openMenu(Menu *menu);
    void closeMenu(Menu *menu);
    friend class Menu; // give Menu access to the above 2 functions

    unsigned int MaskList[8];
    size_t MaskListLength;

    // the masks of the modifiers which are ignored in button events.
    unsigned int NumLockMask, ScrollLockMask;

  protected:
    inline RunState runState(void) const
    { return run_state; }
    inline void setRunState(RunState new_state)
    { run_state = new_state; }

    /*
      Called from run() just before the event loop starts.
     */
    virtual void startup(void);
    /*
      Called from shutdown() after the event loop stops.
    */
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
    Application(const std::string &app_name, const char *dpy_name = 0,
                bool multi_head = false);
    virtual ~Application(void);

    inline bool hasShapeExtensions(void) const
    { return shape.extensions; }

    inline bool startingUp(void) const
    { return run_state == STARTUP; }
    inline bool running(void) const
    { return run_state == RUNNING; }
    inline bool shuttingDown(void) const
    { return run_state == SHUTDOWN; }

    ::Display *XDisplay(void) const;
    inline const Display& display(void) const
    { return *_display; }
    inline Time XTime() const
    { return xserver_time; }

    inline const std::string &applicationName(void) const
    { return _app_name; }

    void grabButton(unsigned int button, unsigned int modifiers,
                    Window grab_window, bool owner_events,
                    unsigned int event_mask, int pointer_mode,
                    int keyboard_mode, Window confine_to, Cursor cursor,
                    bool allow_scroll_lock) const;
    void ungrabButton(unsigned int button, unsigned int modifiers,
                      Window grab_window) const;

    void run(void);
    inline void quit(void)
    { setRunState( SHUTDOWN ); }

    inline unsigned int scrollLockMask(void) const
    { return ScrollLockMask; }
    inline unsigned int numLockMask(void) const
    { return NumLockMask; }

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
      Returns the event handler registered for Window {window}.  If no
      handler has been registered, this function returns zero.
    */
    EventHandler *findEventHandler(Window window);
  };

} // namespace bt

#endif // __Application_hh
