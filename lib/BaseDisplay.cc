// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// BaseDisplay.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
// Copyright (c) 2001        Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 2002 Brad Hughes (bhughes@tcac.net),
//                    Sean 'Shaleh' Perry <shaleh@debian.org>
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
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef    SHAPE
#  include <X11/extensions/shape.h>
#endif // SHAPE

#include <stdio.h>

#ifdef    HAVE_FCNTL_H
#  include <fcntl.h>
#endif // HAVE_FCNTL_H

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif // HAVE_STDLIB_H

#ifdef HAVE_STRING_H
#  include <string.h>
#endif // HAVE_STRING_H

#ifdef    HAVE_UNISTD_H
#  include <sys/types.h>
#  include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef    HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif // HAVE_SYS_SELECT_H

#ifdef    HAVE_SIGNAL_H
#  include <signal.h>
#endif // HAVE_SIGNAL_H

#ifndef   SA_NODEFER
#  ifdef   SA_INTERRUPT
#    define SA_NODEFER SA_INTERRUPT
#  else // !SA_INTERRUPT
#    define SA_NODEFER (0)
#  endif // SA_INTERRUPT
#endif // SA_NODEFER

#ifdef    HAVE_SYS_WAIT_H
#  include <sys/types.h>
#  include <sys/wait.h>
#endif // HAVE_SYS_WAIT_H
}

#include <string>

#include "BaseDisplay.hh"
#include "EventHandler.hh"
#include "GCCache.hh"
#include "Timer.hh"
#include "Util.hh"


// X error handler to handle any and all X errors while the application is
// running
static bool internal_error = False;

BaseDisplay *base_display;

static int handleXErrors(Display *d, XErrorEvent *e) {
#ifdef    DEBUG
  char errtxt[128];

  XGetErrorText(d, e->error_code, errtxt, 128);
  fprintf(stderr, "%s:  X error: %s(%d) opcodes %d/%d\n  resource 0x%lx\n",
          base_display->getApplicationName(), errtxt, e->error_code,
          e->request_code, e->minor_code, e->resourceid);
#else
  // shutup gcc
  (void) d;
  (void) e;
#endif // DEBUG

  if (internal_error) abort();

  return(False);
}


// signal handler to allow for proper and gentle shutdown

#ifndef   HAVE_SIGACTION
static RETSIGTYPE signalhandler(int sig)
#else //  HAVE_SIGACTION
static void signalhandler(int sig)
#endif // HAVE_SIGACTION
{
  static int re_enter = 0;

  switch (sig) {
  case SIGCHLD:
    int status;
    waitpid(-1, &status, WNOHANG | WUNTRACED);

#ifndef   HAVE_SIGACTION
    // assume broken, braindead sysv signal semantics
    signal(SIGCHLD, (RETSIGTYPE (*)(int)) signalhandler);
#endif // HAVE_SIGACTION

    break;

  default:
    if (base_display->handleSignal(sig)) {

#ifndef   HAVE_SIGACTION
      // assume broken, braindead sysv signal semantics
      signal(sig, (RETSIGTYPE (*)(int)) signalhandler);
#endif // HAVE_SIGACTION

      return;
    }

    if (! base_display->isStartup() && ! re_enter) {
      internal_error = True;

      re_enter = 1;
      base_display->shutdown();
    }

    if (sig != SIGTERM && sig != SIGINT)
      abort();

    exit(0);

    break;
  }
}


BaseDisplay::BaseDisplay(const char *app_name, const char *dpy_name)
  : run_state(STARTUP), gccache(0), application_name(app_name)
{
  ::base_display = this;

#ifdef    HAVE_SIGACTION
  struct sigaction action;

  action.sa_handler = signalhandler;
  action.sa_mask = sigset_t();
  action.sa_flags = SA_NOCLDSTOP | SA_NODEFER;

  sigaction(SIGPIPE, &action, NULL);
  sigaction(SIGSEGV, &action, NULL);
  sigaction(SIGFPE, &action, NULL);
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGCHLD, &action, NULL);
  sigaction(SIGHUP, &action, NULL);
  sigaction(SIGUSR1, &action, NULL);
  sigaction(SIGUSR2, &action, NULL);
#else // !HAVE_SIGACTION
  signal(SIGPIPE, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGSEGV, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGFPE, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGTERM, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGINT, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGUSR1, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGUSR2, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGHUP, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGCHLD, (RETSIGTYPE (*)(int)) signalhandler);
#endif // HAVE_SIGACTION

  if (! (display = XOpenDisplay(dpy_name)))
    ::exit(2);
  if (fcntl(ConnectionNumber(display), F_SETFD, 1) == -1)
    ::exit(2);

#ifdef    SHAPE
  shape.extensions = XShapeQueryExtension(display, &shape.event_basep,
                                          &shape.error_basep);
#else // !SHAPE
  shape.extensions = False;
#endif // SHAPE

  XSetErrorHandler((XErrorHandler) handleXErrors);

  screenInfoList.reserve(ScreenCount(display));
  for (int i = 0; i < ScreenCount(display); ++i)
    screenInfoList.push_back(ScreenInfo(this, i));

  NumLockMask = ScrollLockMask = 0;

  const XModifierKeymap* const modmap = XGetModifierMapping(display);
  if (modmap && modmap->max_keypermod > 0) {
    const int mask_table[] = {
      ShiftMask, LockMask, ControlMask, Mod1Mask,
      Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
    };
    const size_t size = (sizeof(mask_table) / sizeof(mask_table[0])) *
      modmap->max_keypermod;
    // get the values of the keyboard lock modifiers
    // Note: Caps lock is not retrieved the same way as Scroll and Num lock
    // since it doesn't need to be.
    const KeyCode num_lock = XKeysymToKeycode(display, XK_Num_Lock);
    const KeyCode scroll_lock = XKeysymToKeycode(display, XK_Scroll_Lock);

    for (size_t cnt = 0; cnt < size; ++cnt) {
      if (! modmap->modifiermap[cnt]) continue;

      if (num_lock == modmap->modifiermap[cnt])
        NumLockMask = mask_table[cnt / modmap->max_keypermod];
      if (scroll_lock == modmap->modifiermap[cnt])
        ScrollLockMask = mask_table[cnt / modmap->max_keypermod];
    }
  }

  MaskList[0] = 0;
  MaskList[1] = LockMask;
  MaskList[2] = NumLockMask;
  MaskList[3] = LockMask | NumLockMask;
  MaskList[4] = ScrollLockMask;
  MaskList[5] = ScrollLockMask | LockMask;
  MaskList[6] = ScrollLockMask | NumLockMask;
  MaskList[7] = ScrollLockMask | LockMask | NumLockMask;
  MaskListLength = sizeof(MaskList) / sizeof(MaskList[0]);

  if (modmap) XFreeModifiermap(const_cast<XModifierKeymap*>(modmap));
}


BaseDisplay::~BaseDisplay(void) {
  delete gccache;

  XCloseDisplay(display);
}


void BaseDisplay::eventLoop(void) {
  run();

  const int xfd = ConnectionNumber(display);

  while (run_state == RUNNING && ! internal_error) {
    if (XPending(display)) {
      XEvent e;
      XNextEvent(display, &e);
      process_event(&e);
    } else {
      fd_set rfds;
      timeval now, tm, *timeout = (timeval *) 0;

      FD_ZERO(&rfds);
      FD_SET(xfd, &rfds);

      if (! timerList.empty()) {
        const BTimer* const timer = timerList.top();

        gettimeofday(&now, 0);
        tm = timer->timeRemaining(now);

        timeout = &tm;
      }

      select(xfd + 1, &rfds, 0, 0, timeout);

      // check for timer timeout
      gettimeofday(&now, 0);

      // there is a small chance for deadlock here:
      // *IF* the timer list keeps getting refreshed *AND* the time between
      // timer->start() and timer->shouldFire() is within the timer's period
      // then the timer will keep firing.  This should be VERY near impossible.
      while (! timerList.empty()) {
        BTimer *timer = timerList.top();
        if (! timer->shouldFire(now))
          break;

        timerList.pop();

        timer->fireTimeout();
        timer->halt();
        if (timer->isRecurring())
          timer->start();
      }
    }
  }
}

void BaseDisplay::process_event(XEvent *event) {
  EventHandlerMap::iterator it = eventhandlers.find(event->xany.window);
  if (it == eventhandlers.end()) return;

  EventHandler *handler = it->second;

  // deliver the event
  switch (event->type) {
  case ButtonPress: {
    // last_time = event->xbutton.time

    // strip the lock key modifiers
    event->xbutton.state &= ~(NumLockMask | ScrollLockMask | LockMask);
    handler->buttonPressEvent(&event->xbutton);
    break;
  }

  case ButtonRelease: {
    // last_time = event->xbutton.time

    // strip the lock key modifiers
    event->xbutton.state &= ~(NumLockMask | ScrollLockMask | LockMask);
    handler->buttonReleaseEvent(&event->xbutton);
    break;
  }

  case KeyPress: {
    // strip the lock key modifiers, except numlock, which can be useful
    event->xkey.state &= ~(ScrollLockMask | LockMask);
    handler->keyPressEvent(&event->xkey);
    break;
  }

  case KeyRelease: {
    // strip the lock key modifiers, except numlock, which can be useful
    event->xkey.state &= ~(ScrollLockMask | LockMask);
    handler->keyReleaseEvent(&event->xkey);
    break;
  }

  case MapNotify: {
    handler->mapNotifyEvent(&event->xmap);
    break;
  }

  case UnmapNotify: {
    handler->unmapNotifyEvent(&event->xunmap);
    break;
  }

  case ReparentNotify: {
    handler->reparentNotifyEvent(&event->xreparent);
    break;
  }

  case DestroyNotify: {
    handler->destroyNotifyEvent(&event->xdestroywindow);
    break;
  }

  case EnterNotify: {
    // last_time = event->xcrossing.time;

    handler->enterNotifyEvent(&event->xcrossing);
    break;
  }

  case LeaveNotify: {
    // last_time = event->xcrossing.time;

    handler->leaveNotifyEvent(&event->xcrossing);
    break;
  }

  case PropertyNotify: {
    // last_time = event->xproperty.time;

    handler->propertyNotifyEvent(&event->xproperty);
    break;
  }

  case ConfigureRequest: {
    handler->configureRequestEvent(&event->xconfigurerequest);
    break;
  }

  case MotionNotify: {
    // last_time = event->xmotion.time;

    // compress motion notify events
    XEvent realevent;
    unsigned int i = 0;
    while (XCheckTypedWindowEvent(getXDisplay(), event->xmotion.window,
                                  MotionNotify, &realevent)) {
      ++i;
    }

    // if we have compressed some motion events, use the last one
    if ( i > 0 )
      event = &realevent;

    // strip the lock key modifiers
    event->xbutton.state &= ~(NumLockMask | ScrollLockMask | LockMask);
    handler->motionNotifyEvent(&event->xmotion);
    break;
  }

  case Expose: {
    // compress expose events
    XEvent realevent;
    unsigned int i = 0;
    int ex1, ey1, ex2, ey2;
    ex1 = event->xexpose.x;
    ey1 = event->xexpose.y;
    ex2 = ex1 + event->xexpose.width - 1;
    ey2 = ey1 + event->xexpose.height - 1;
    while (XCheckTypedWindowEvent(getXDisplay(), event->xexpose.window,
                                  Expose, &realevent)) {
      ++i;

      // merge expose area
      ex1 = std::min(realevent.xexpose.x, ex1);
      ey1 = std::min(realevent.xexpose.y, ey1);
      ex2 = std::max(realevent.xexpose.x + realevent.xexpose.width - 1, ex2);
      ey2 = std::max(realevent.xexpose.y + realevent.xexpose.height - 1, ey2);
    }
    if ( i > 0 )
      event = &realevent;

    // use the merged area
    event->xexpose.x = ex1;
    event->xexpose.y = ey1;
    event->xexpose.width = ex2 - ex1 + 1;
    event->xexpose.height = ey2 - ey1 + 1;

    handler->exposeEvent(&event->xexpose);
    break;
  }

  case ConfigureNotify: {
    // compress configure notify events
    XEvent realevent;
    unsigned int i = 0;
    while (XCheckTypedWindowEvent(getXDisplay(), event->xconfigure.window,
                                  ConfigureNotify, &realevent)) {
      ++i;
    }

    // if we have compressed some configure notify events, use the last one
    if ( i > 0 )
      event = &realevent;

    handler->configureNotifyEvent(&event->xconfigure);
    break;
  }

  case ClientMessage: {
    handler->clientMessageEvent(&event->xclient);
    break;
  }

  case NoExpose: {
    // not handled, ignore
    break;
  }

  default: {
#ifdef    SHAPE
    if (event->type == getShapeEventBase()) {
      handler->shapeEvent((XShapeEvent *) event);
    } else
#endif // SHAPE
      {
#ifdef    DEBUG
        fprintf(stderr, "unhandled event %d\n", event->type);
#endif // DEBUG
      }
    break;
  }
  } // switch
}


void BaseDisplay::addTimer(BTimer *timer) {
  if (! timer) return;
  timerList.push(timer);
}


void BaseDisplay::removeTimer(BTimer *timer) {
  timerList.release(timer);
}


/*
 * Grabs a button, but also grabs the button in every possible combination
 * with the keyboard lock keys, so that they do not cancel out the event.

 * if allow_scroll_lock is true then only the top half of the lock mask
 * table is used and scroll lock is ignored.  This value defaults to false.
 */
void BaseDisplay::grabButton(unsigned int button, unsigned int modifiers,
                             Window grab_window, bool owner_events,
                             unsigned int event_mask, int pointer_mode,
                             int keyboard_mode, Window confine_to,
                             Cursor cursor, bool allow_scroll_lock) const {
  unsigned int length = (allow_scroll_lock) ? MaskListLength / 2:
                                              MaskListLength;
  for (size_t cnt = 0; cnt < length; ++cnt) {
    XGrabButton(display, button, modifiers | MaskList[cnt], grab_window,
                owner_events, event_mask, pointer_mode, keyboard_mode,
                confine_to, cursor);
  }
}


/*
 * Releases the grab on a button, and ungrabs all possible combinations of the
 * keyboard lock keys.
 */
void BaseDisplay::ungrabButton(unsigned int button, unsigned int modifiers,
                               Window grab_window) const {
  for (size_t cnt = 0; cnt < MaskListLength; ++cnt) {
    XUngrabButton(display, button, modifiers | MaskList[cnt], grab_window);
  }
}


const ScreenInfo* BaseDisplay::getScreenInfo(unsigned int s) const {
  if (s < screenInfoList.size())
    return &screenInfoList[s];
  return (const ScreenInfo*) 0;
}


BGCCache* BaseDisplay::gcCache(void) const {
  if (! gccache)
    gccache = new BGCCache(this, screenInfoList.size());
  return gccache;
}


void BaseDisplay::insertEventHandler(Window window, EventHandler *handler) {
  eventhandlers.insert(std::pair<Window,EventHandler*>(window, handler));
}


void BaseDisplay::removeEventHandler(Window window) {
  eventhandlers.erase(window);
}


ScreenInfo::ScreenInfo(BaseDisplay *d, unsigned int num) {
  basedisplay = d;
  screen_number = num;

  root_window = RootWindow(basedisplay->getXDisplay(), screen_number);

  rect.setSize(WidthOfScreen(ScreenOfDisplay(basedisplay->getXDisplay(),
                                             screen_number)),
               HeightOfScreen(ScreenOfDisplay(basedisplay->getXDisplay(),
                                              screen_number)));

  /*
    If the default depth is at least 8 we will use that,
    otherwise we try to find the largest TrueColor visual.
    Preference is given to 24 bit over larger depths if 24 bit is an option.
  */

  depth = DefaultDepth(basedisplay->getXDisplay(), screen_number);
  visual = DefaultVisual(basedisplay->getXDisplay(), screen_number);
  colormap = DefaultColormap(basedisplay->getXDisplay(), screen_number);

  if (depth < 8) {
    // search for a TrueColor Visual... if we can't find one...
    // we will use the default visual for the screen
    XVisualInfo vinfo_template, *vinfo_return;
    int vinfo_nitems;
    int best = -1;

    vinfo_template.screen = screen_number;
    vinfo_template.c_class = TrueColor;

    vinfo_return = XGetVisualInfo(basedisplay->getXDisplay(),
                                  VisualScreenMask | VisualClassMask,
                                  &vinfo_template, &vinfo_nitems);
    if (vinfo_return) {
      int max_depth = 1;
      for (int i = 0; i < vinfo_nitems; ++i) {
        if (vinfo_return[i].depth > max_depth) {
          if (max_depth == 24 && vinfo_return[i].depth > 24)
            break;          // prefer 24 bit over 32
          max_depth = vinfo_return[i].depth;
          best = i;
        }
      }
      if (max_depth < depth) best = -1;
    }

    if (best != -1) {
      depth = vinfo_return[best].depth;
      visual = vinfo_return[best].visual;
      colormap = XCreateColormap(basedisplay->getXDisplay(), root_window,
                                 visual, AllocNone);
    }

    XFree(vinfo_return);
  }

  // get the default display string and strip the screen number
  std::string default_string = DisplayString(basedisplay->getXDisplay());
  const std::string::size_type pos = default_string.rfind(".");
  if (pos != std::string::npos)
    default_string.resize(pos);

  display_string = std::string("DISPLAY=") + default_string + '.' +
    itostring(static_cast<unsigned long>(screen_number));
}
