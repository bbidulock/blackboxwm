// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// blackbox.cc for Blackbox - an X11 Window manager
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#ifdef    SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

#include <stdio.h>

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

#ifdef    HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

#ifdef    HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif // HAVE_SYS_SELECT_H

#ifdef    HAVE_SIGNAL_H
#  include <signal.h>
#endif // HAVE_SIGNAL_H

#ifdef    HAVE_SYS_SIGNAL_H
#  include <sys/signal.h>
#endif // HAVE_SYS_SIGNAL_H

#ifdef    HAVE_SYS_STAT_H
#  include <sys/types.h>
#  include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#ifdef    TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else // !TIME_WITH_SYS_TIME
#  ifdef    HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else // !HAVE_SYS_TIME_H
#    include <time.h>
#  endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME

#ifdef    HAVE_LIBGEN_H
#  include <libgen.h>
#endif // HAVE_LIBGEN_H
}

#include <assert.h>

#include <algorithm>
#include <string>

#include "i18n.hh"
#include "blackbox.hh"
#include "Clientmenu.hh"
#include "Image.hh"
#include "Netwm.hh"
#include "Pen.hh"
#include "PixmapCache.hh"
#include "Rootmenu.hh"
#include "Screen.hh"
#include "Slit.hh"
#include "Toolbar.hh"
#include "Util.hh"
#include "Window.hh"


Blackbox::Blackbox(char **m_argv, const char *dpy_name, const char *rc,
                   bool multi_head)
  : bt::Application(m_argv[0], dpy_name, multi_head) {
  if (! XSupportsLocale())
    fprintf(stderr, "X server does not support locale\n");

  if (XSetLocaleModifiers("") == NULL)
    fprintf(stderr, "cannot set locale modifiers\n");

  argv = m_argv;
  if (! rc) rc = "~/.blackboxrc";
  rc_file = bt::expandTilde(rc);

  no_focus = False;

  resource.auto_raise_delay.tv_sec = resource.auto_raise_delay.tv_usec = 0;

  active_screen = 0;
  focused_window = (BlackboxWindow *) 0;
  _netwm = (bt::Netwm*) 0;

  XrmInitialize();
  load_rc();

  init_icccm();

  cursor.session = XCreateFontCursor(XDisplay(), XC_left_ptr);
  cursor.move = XCreateFontCursor(XDisplay(), XC_fleur);
  cursor.ll_angle = XCreateFontCursor(XDisplay(), XC_ll_angle);
  cursor.lr_angle = XCreateFontCursor(XDisplay(), XC_lr_angle);

  screen_list_count = 0;
  if (! multi_head || display().screenCount() == 1) {
    BScreen* screen = new BScreen(this, DefaultScreen(XDisplay()));

    if (! screen->isScreenManaged()) {
      delete screen;
    } else {
      ++screen_list_count;
      screen_list = new BScreen*[screen_list_count];
      screen_list[0] = screen;
    }
  } else {
    screen_list = new BScreen*[display().screenCount()];
    for (unsigned int i = 0; i < display().screenCount(); i++) {
      BScreen *screen = new BScreen(this, i);

      if (! screen->isScreenManaged()) {
        delete screen;
        continue;
      }

      screen_list[i] = screen;
      ++screen_list_count;
    }
  }

  if (screen_list_count == 0) {
    fprintf(stderr,
            bt::i18n(blackboxSet, blackboxNoManagableScreens,
              "Blackbox::Blackbox: no managable screens found, aborting.\n"));
    ::exit(3);
  }

  // start with the first managed screen as the active screen
  setActiveScreen(screen_list[0]);

  XSynchronize(XDisplay(), False);
  XSync(XDisplay(), False);

  reconfigure_wait = reread_menu_wait = False;

  timer = new bt::Timer(this, this);
  timer->setTimeout(0l);
}


Blackbox::~Blackbox(void) {
  std::for_each(screen_list, screen_list + screen_list_count,
                bt::PointerAssassin());

  delete [] screen_list;
  std::for_each(menuTimestamps.begin(), menuTimestamps.end(),
                bt::PointerAssassin());

  delete timer;
  delete _netwm;
}


void Blackbox::process_event(XEvent *e) {
  switch (e->type) {
  case MapRequest: {
#ifdef    DEBUG
    fprintf(stderr, "Blackbox::process_event(): MapRequest for 0x%lx\n",
            e->xmaprequest.window);
#endif // DEBUG

    BlackboxWindow *win = findWindow(e->xmaprequest.window);

    if (win) {
      bool focus = False;
      if (win->isIconic()) {
        win->deiconify();
        focus = True;
      }
      if (win->isShaded()) {
        win->shade();
        focus = True;
      }

      if (focus && (win->isTransient() || win->getScreen()->doFocusNew()))
        win->setInputFocus();
    } else {
      BScreen *screen = findScreen(e->xmaprequest.parent);

      if (! screen) {
        /*
          we got a map request for a window who's parent isn't root. this
          can happen in only one circumstance:

          a client window unmapped a managed window, and then remapped it
          somewhere between unmapping the client window and reparenting it
          to root.

          regardless of how it happens, we need to find the screen that
          the window is on
        */
        XWindowAttributes wattrib;
        if (! XGetWindowAttributes(XDisplay(), e->xmaprequest.window,
                                   &wattrib)) {
          // failed to get the window attributes, perhaps the window has
          // now been destroyed?
          break;
        }

        screen = findScreen(wattrib.root);
        assert(screen != 0); // this should never happen
      }
      screen->addWindow(e->xmaprequest.window);
    }

    break;
  }

  case ColormapNotify: {
    BScreen *screen = findScreen(e->xcolormap.window);

    if (screen)
      screen->setRootColormapInstalled((e->xcolormap.state ==
                                        ColormapInstalled) ? True : False);

    break;
  }

  case FocusIn: {
    if (e->xfocus.detail != NotifyNonlinear) {
      /*
        don't process FocusIns when:
        1. the new focus window isn't an ancestor or inferior of the old
        focus window (NotifyNonlinear)
      */
      break;
    }

    BlackboxWindow *win = findWindow(e->xfocus.window);
    if (win) {
      if (! win->isFocused())
        win->setFocusFlag(True);

      /*
        set the event window to None.  when the FocusOut event handler calls
        this function recursively, it uses this as an indication that focus
        has moved to a known window.
      */
      e->xfocus.window = None;
    }

    break;
  }

  case FocusOut: {
    if (e->xfocus.detail != NotifyNonlinear) {
      /*
        don't process FocusOuts when:
        2. the new focus window isn't an ancestor or inferior of the old
        focus window (NotifyNonlinear)
      */
      break;
    }

    BlackboxWindow *win = findWindow(e->xfocus.window);
    if (win && win->isFocused()) {
      /*
        before we mark "win" as unfocused, we need to verify that focus is
        going to a known location, is in a known location, or set focus
        to a known location.
      */

      XEvent event;
      // don't check the current focus if FocusOut was generated by a grab
      bool check_focus = (e->xfocus.mode != NotifyGrab);

      /*
        First, check if there is a pending FocusIn event waiting.  if there
        is, process it and determine if focus has moved to another window
        (the FocusIn event handler sets the window in the event
        structure to None to indicate this).
      */
      if (XCheckTypedEvent(XDisplay(), FocusIn, &event)) {

        process_event(&event);
        if (event.xfocus.window == None) {
          // focus has moved
          check_focus = False;
        }
      }

      if (check_focus) {
        /*
          Second, we query the X server for the current input focus.
          to make sure that we keep a consistent state.
        */
        BlackboxWindow *focus;
        Window w;
        int revert;
        XGetInputFocus(XDisplay(), &w, &revert);
        focus = findWindow(w);
        if (focus) {
          /*
            focus got from "win" to "focus" under some very strange
            circumstances, and we need to make sure that the focus indication
            is correct.
          */
          setFocusedWindow(focus);
        } else {
          // we have no idea where focus went... so we set it to PointerRoot
          setFocusedWindow(0);
        }
      }
    }

    break;
  }

  default: {
    // Send the event through the default EventHandlers.
    bt::Application::process_event(e);
    break;
  }
  } // switch
}


bool Blackbox::handleSignal(int sig) {
  switch (sig) {
  case SIGHUP:
    reconfigure();
    break;

  case SIGUSR1:
    reload_rc();
    break;

  case SIGUSR2:
    rereadMenu();
    break;

  case SIGPIPE:
  case SIGSEGV:
  case SIGFPE:
  case SIGINT:
  case SIGTERM:
    shutdown();

  default:
    return False;
  }

  return True;
}


void Blackbox::init_icccm(void) {
  char* atoms[7] = {
    "WM_COLORMAP_WINDOWS",
    "WM_PROTOCOLS",
    "WM_STATE",
    "WM_CHANGE_STATE",
    "WM_DELETE_WINDOW",
    "WM_TAKE_FOCUS",
    "_MOTIF_WM_HINTS"
  };
  Atom atoms_return[7];
  XInternAtoms(XDisplay(), atoms, 7, False, atoms_return);
  xa_wm_colormap_windows = atoms_return[0];
  xa_wm_protocols = atoms_return[1];
  xa_wm_state = atoms_return[2];
  xa_wm_change_state = atoms_return[3];
  xa_wm_delete_window = atoms_return[4];
  xa_wm_take_focus = atoms_return[5];
  motif_wm_hints = atoms_return[6];

  _netwm = new bt::Netwm(XDisplay());
}


bool Blackbox::validateWindow(Window window) {
  XEvent event;
  if (XCheckTypedWindowEvent(XDisplay(), window, DestroyNotify, &event)) {
    XPutBackEvent(XDisplay(), &event);

    return False;
  }

  return True;
}

BScreen *Blackbox::findScreen(Window window) {
  for (unsigned int i = 0; i < screen_list_count; ++i)
    if (screen_list[i]->screenInfo().rootWindow() == window)
      return screen_list[i];
  return 0;
}

BlackboxWindow *Blackbox::findWindow(Window window) {
  WindowLookup::iterator it = windowSearchList.find(window);
  if (it != windowSearchList.end())
    return it->second;
  return 0;
}


void Blackbox::insertWindow(Window window, BlackboxWindow *data) {
  windowSearchList.insert(WindowLookupPair(window, data));
}


void Blackbox::removeWindow(Window window) {
  windowSearchList.erase(window);
}


BWindowGroup *Blackbox::findWindowGroup(Window window) {
  GroupLookup::iterator it = groupSearchList.find(window);
  if (it != groupSearchList.end())
    return it->second;

  return (BWindowGroup *) 0;
}


void Blackbox::insertWindowGroup(Window window, BWindowGroup *data) {
  groupSearchList.insert(GroupLookupPair(window, data));
}


void Blackbox::removeWindowGroup(Window window) {
  groupSearchList.erase(window);
}


void Blackbox::restart(const std::string &prog) {
  shutdown();

  if (! prog.empty()) {
    putenv(const_cast<char *>
           (display().screenInfo(0).displayString().c_str()));
    execlp(prog.c_str(), prog.c_str(), NULL);
    perror(prog.c_str());
  }

  // fall back in case the above execlp doesn't work
  execvp(argv[0], argv);
  std::string name = basename(argv[0]);
  execvp(name.c_str(), argv);
}


void Blackbox::shutdown(void) {
  bt::Application::shutdown();

  XSetInputFocus(XDisplay(), PointerRoot, RevertToNone, CurrentTime);

  std::for_each(screen_list, screen_list + screen_list_count,
                std::mem_fun(&BScreen::shutdown));

  XSync(XDisplay(), False);

  save_rc();
}


void Blackbox::save_rc(void) {
  XrmDatabase new_blackboxrc = (XrmDatabase) 0;
  char rc_string[1024];

  load_rc();

  sprintf(rc_string, "session.menuFile:  %s", getMenuFilename());
  XrmPutLineResource(&new_blackboxrc, rc_string);

  sprintf(rc_string, "session.colorsPerChannel:  %d",
          bt::Image::colorsPerChannel());
  XrmPutLineResource(&new_blackboxrc, rc_string);

  sprintf(rc_string, "session.doubleClickInterval:  %lu",
          resource.double_click_interval);
  XrmPutLineResource(&new_blackboxrc, rc_string);

  sprintf(rc_string, "session.autoRaiseDelay:  %lu",
          ((resource.auto_raise_delay.tv_sec * 1000) +
           (resource.auto_raise_delay.tv_usec / 1000)));
  XrmPutLineResource(&new_blackboxrc, rc_string);

  sprintf(rc_string, "session.cacheLife: %lu", resource.cache_life / 60000);
  XrmPutLineResource(&new_blackboxrc, rc_string);

  sprintf(rc_string, "session.cacheMax: %lu", resource.cache_max);
  XrmPutLineResource(&new_blackboxrc, rc_string);

  for (unsigned int i = 0; i < screen_list_count; ++i) {
    BScreen *screen = screen_list[i];
    int screen_number = screen->screenNumber();

    char *placement = (char *) 0;

    switch (screen->getSlitPlacement()) {
    case Slit::TopLeft: placement = "TopLeft"; break;
    case Slit::CenterLeft: placement = "CenterLeft"; break;
    case Slit::BottomLeft: placement = "BottomLeft"; break;
    case Slit::TopCenter: placement = "TopCenter"; break;
    case Slit::BottomCenter: placement = "BottomCenter"; break;
    case Slit::TopRight: placement = "TopRight"; break;
    case Slit::BottomRight: placement = "BottomRight"; break;
    case Slit::CenterRight: default: placement = "CenterRight"; break;
    }

    sprintf(rc_string, "session.screen%d.slit.placement: %s", screen_number,
            placement);
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.slit.direction: %s", screen_number,
            ((screen->getSlitDirection() == Slit::Horizontal) ? "Horizontal" :
             "Vertical"));
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.slit.onTop: %s", screen_number,
            ((screen->getSlit()->isOnTop()) ? "True" : "False"));
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.slit.autoHide: %s", screen_number,
            ((screen->getSlit()->doAutoHide()) ? "True" : "False"));
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.opaqueMove: %s",
            ((screen->doOpaqueMove()) ? "True" : "False"));
    XrmPutLineResource(&new_blackboxrc, rc_string);

    const char *ditherMode;
    switch (bt::Image::ditherMode()) {
    case bt::OrderedDither:        ditherMode = "OrderedDither";        break;
    case bt::FloydSteinbergDither: ditherMode = "FloydSteinbergDither"; break;
    default: ditherMode = "NoDither"; break;
    }
    sprintf(rc_string, "session.imageDither: %s", ditherMode);
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.placementIgnoresShaded: %s",
            screen_number,
            (screen->placementIgnoresShaded()) ? "True" : "False");
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.fullMaximization: %s", screen_number,
            ((screen->doFullMax()) ? "True" : "False"));
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.focusNewWindows: %s", screen_number,
            ((screen->doFocusNew()) ? "True" : "False"));
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.focusLastWindow: %s", screen_number,
            ((screen->doFocusLast()) ? "True" : "False"));
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.disableBindingsWithScrollLock: %s",
            screen_number, ((screen->allowScrollLock()) ? "True" : "False"));
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.rowPlacementDirection: %s",
            screen_number,
            ((screen->getRowPlacementDirection() == BScreen::LeftRight) ?
             "LeftToRight" : "RightToLeft"));
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.colPlacementDirection: %s",
            screen_number,
            ((screen->getColPlacementDirection() == BScreen::TopBottom) ?
             "TopToBottom" : "BottomToTop"));
    XrmPutLineResource(&new_blackboxrc, rc_string);

    switch (screen->getPlacementPolicy()) {
    case BScreen::CascadePlacement:
      placement = "CascadePlacement";
      break;
    case BScreen::ColSmartPlacement:
      placement = "ColSmartPlacement";
      break;

    case BScreen::RowSmartPlacement:
    default:
      placement = "RowSmartPlacement";
      break;
    }
    sprintf(rc_string, "session.screen%d.windowPlacement:  %s", screen_number,
            placement);
    XrmPutLineResource(&new_blackboxrc, rc_string);

    std::string fmodel;
    if (screen->isSloppyFocus()) {
      fmodel = "SloppyFocus";
      if (screen->doAutoRaise()) fmodel += " AutoRaise";
      if (screen->doClickRaise()) fmodel += " ClickRaise";
    } else {
      fmodel = "ClickToFocus";
    }
    sprintf(rc_string, "session.screen%d.focusModel:  %s", screen_number,
            fmodel.c_str());
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.workspaces:  %d", screen_number,
            screen->getWorkspaceCount());
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.toolbar.onTop:  %s", screen_number,
            ((screen->getToolbar()->isOnTop()) ? "True" : "False"));
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.toolbar.autoHide:  %s",
            screen_number,
            ((screen->getToolbar()->doAutoHide()) ? "True" : "False"));
    XrmPutLineResource(&new_blackboxrc, rc_string);

    switch (screen->getToolbarPlacement()) {
    case Toolbar::TopLeft: placement = "TopLeft"; break;
    case Toolbar::BottomLeft: placement = "BottomLeft"; break;
    case Toolbar::TopCenter: placement = "TopCenter"; break;
    case Toolbar::TopRight: placement = "TopRight"; break;
    case Toolbar::BottomRight: placement = "BottomRight"; break;
    case Toolbar::BottomCenter: default: placement = "BottomCenter"; break;
    }

    sprintf(rc_string, "session.screen%d.toolbar.placement: %s",
            screen_number, placement);
    XrmPutLineResource(&new_blackboxrc, rc_string);

    load_rc(screen);

    // these are static, but may not be saved in the user's .blackboxrc,
    // writing these resources will allow the user to edit them at a later
    // time... but loading the defaults before saving allows us to rewrite the
    // users changes...

    sprintf(rc_string, "session.screen%d.strftimeFormat: %s", screen_number,
            screen->getStrftimeFormat());
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.edgeSnapThreshold: %d",
            screen_number, screen->getEdgeSnapThreshold());
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.toolbar.widthPercent:  %d",
            screen_number, screen->getToolbarWidthPercent());
    XrmPutLineResource(&new_blackboxrc, rc_string);

    // write out the user's workspace names
    std::string save_string = screen->getWorkspaceName(0);
    for (unsigned int j = 1; j < screen->getWorkspaceCount(); ++j) {
      save_string += ',';
      save_string += screen->getWorkspaceName(j);
    }

    char *resource_string = new char[save_string.length() + 48];
    sprintf(resource_string, "session.screen%d.workspaceNames:  %s",
            screen_number, save_string.c_str());
    XrmPutLineResource(&new_blackboxrc, resource_string);

    delete [] resource_string;
  }

  XrmDatabase old_blackboxrc = XrmGetFileDatabase(rc_file.c_str());

  XrmMergeDatabases(new_blackboxrc, &old_blackboxrc);
  XrmPutFileDatabase(old_blackboxrc, rc_file.c_str());
  XrmDestroyDatabase(old_blackboxrc);
}


void Blackbox::load_rc(void) {
  XrmDatabase database = (XrmDatabase) 0;

  database = XrmGetFileDatabase(rc_file.c_str());

  XrmValue value;
  char *value_type;
  int int_value;
  unsigned long long_value;

  if (XrmGetResource(database, "session.menuFile", "Session.MenuFile",
                     &value_type, &value)) {
    resource.menu_file = bt::expandTilde(value.addr);
  } else {
    resource.menu_file = DEFAULTMENU;
  }

  if (XrmGetResource(database, "session.colorsPerChannel",
                     "Session.ColorsPerChannel", &value_type, &value) &&
      sscanf(value.addr, "%d", &int_value) == 1) {
    bt::Image::setColorsPerChannel(int_value);
  }

  if (XrmGetResource(database, "session.styleFile", "Session.StyleFile",
                     &value_type, &value))
    resource.style_file = bt::expandTilde(value.addr);
  else
    resource.style_file = DEFAULTSTYLE;

  resource.double_click_interval = 250;
  if (XrmGetResource(database, "session.doubleClickInterval",
                     "Session.DoubleClickInterval", &value_type, &value) &&
      sscanf(value.addr, "%lu", &long_value) == 1) {
    resource.double_click_interval = long_value;
  }

  resource.auto_raise_delay.tv_usec = 400;
  if (XrmGetResource(database, "session.autoRaiseDelay",
                     "Session.AutoRaiseDelay", &value_type, &value) &&
      sscanf(value.addr, "%lu", &long_value) == 1) {
    resource.auto_raise_delay.tv_usec = long_value;
  }

  resource.auto_raise_delay.tv_sec = resource.auto_raise_delay.tv_usec / 1000;
  resource.auto_raise_delay.tv_usec -=
    (resource.auto_raise_delay.tv_sec * 1000);
  resource.auto_raise_delay.tv_usec *= 1000;

  resource.cache_life = 5l;
  if (XrmGetResource(database, "session.cacheLife", "Session.CacheLife",
                     &value_type, &value) &&
      sscanf(value.addr, "%lu", &long_value) == 1) {
    resource.cache_life = long_value;
  }
  resource.cache_life *= 60000;

  resource.cache_max = 200;
  if (XrmGetResource(database, "session.cacheMax", "Session.CacheMax",
                     &value_type, &value) &&
      sscanf(value.addr, "%lu", &long_value) == 1) {
    resource.cache_max = long_value;
  }
}


void Blackbox::load_rc(BScreen *screen) {
  XrmDatabase database = (XrmDatabase) 0;

  database = XrmGetFileDatabase(rc_file.c_str());

  XrmValue value;
  char *value_type, name_lookup[1024], class_lookup[1024];
  int screen_number = screen->screenNumber();
  int int_value;

  sprintf(name_lookup,  "session.screen%d.placementIgnoresShaded",
          screen_number);
  sprintf(class_lookup, "Session.Screen%d.placementIgnoresShaded",
          screen_number);
  screen->resource().savePlacementIgnoresShaded(True);
  if (XrmGetResource(database, name_lookup, class_lookup,
                     &value_type, &value) &&
      ! strncasecmp(value.addr, "false", value.size)) {
    screen->resource().savePlacementIgnoresShaded(False);
  }

  sprintf(name_lookup,  "session.screen%d.fullMaximization", screen_number);
  sprintf(class_lookup, "Session.Screen%d.FullMaximization", screen_number);
  screen->resource().saveFullMax(False);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "true", value.size)) {
    screen->resource().saveFullMax(True);
  }

  sprintf(name_lookup,  "session.screen%d.focusNewWindows", screen_number);
  sprintf(class_lookup, "Session.Screen%d.FocusNewWindows", screen_number);
  screen->resource().saveFocusNew(False);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "true", value.size)) {
    screen->resource().saveFocusNew(True);
  }

  sprintf(name_lookup,  "session.screen%d.focusLastWindow", screen_number);
  sprintf(class_lookup, "Session.Screen%d.focusLastWindow", screen_number);
  screen->resource().saveFocusLast(False);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "true", value.size)) {
    screen->resource().saveFocusLast(True);
  }

  sprintf(name_lookup,  "session.screen%d.disableBindingsWithScrollLock",
          screen_number);
  sprintf(class_lookup, "Session.Screen%d.disableBindingsWithScrollLock",
          screen_number);
  screen->resource().saveAllowScrollLock(False);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "true", value.size)) {
    screen->resource().saveAllowScrollLock(True);
  }

  sprintf(name_lookup,  "session.screen%d.rowPlacementDirection",
          screen_number);
  sprintf(class_lookup, "Session.Screen%d.RowPlacementDirection",
          screen_number);
  screen->resource().saveRowPlacementDirection(BScreen::LeftRight);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "righttoleft", value.size)) {
    screen->resource().saveRowPlacementDirection(BScreen::RightLeft);
  }

  sprintf(name_lookup,  "session.screen%d.colPlacementDirection",
          screen_number);
  sprintf(class_lookup, "Session.Screen%d.ColPlacementDirection",
          screen_number);
  screen->resource().saveColPlacementDirection(BScreen::TopBottom);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "bottomtotop", value.size)) {
    screen->resource().saveColPlacementDirection(BScreen::BottomTop);
  }

  sprintf(name_lookup,  "session.screen%d.workspaces", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Workspaces", screen_number);
  screen->resource().saveWorkspaces(1);
  if (XrmGetResource(database, name_lookup, class_lookup,
                     &value_type, &value) &&
      sscanf(value.addr, "%d", &int_value) == 1 &&
      int_value > 0 && int_value < 128) {
    screen->resource().saveWorkspaces(int_value);
  }

  sprintf(name_lookup,  "session.screen%d.toolbar.widthPercent",
          screen_number);
  sprintf(class_lookup, "Session.Screen%d.Toolbar.WidthPercent",
          screen_number);
  screen->resource().saveToolbarWidthPercent(66);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      sscanf(value.addr, "%d", &int_value) == 1 &&
      int_value > 0 && int_value <= 100) {
    screen->resource().saveToolbarWidthPercent(int_value);
  }

  sprintf(name_lookup, "session.screen%d.toolbar.placement", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Toolbar.Placement", screen_number);
  screen->resource().saveToolbarPlacement(Toolbar::BottomCenter);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    if (! strncasecmp(value.addr, "TopLeft", value.size))
      screen->resource().saveToolbarPlacement(Toolbar::TopLeft);
    else if (! strncasecmp(value.addr, "BottomLeft", value.size))
      screen->resource().saveToolbarPlacement(Toolbar::BottomLeft);
    else if (! strncasecmp(value.addr, "TopCenter", value.size))
      screen->resource().saveToolbarPlacement(Toolbar::TopCenter);
    else if (! strncasecmp(value.addr, "TopRight", value.size))
      screen->resource().saveToolbarPlacement(Toolbar::TopRight);
    else if (! strncasecmp(value.addr, "BottomRight", value.size))
      screen->resource().saveToolbarPlacement(Toolbar::BottomRight);
  }

  sprintf(name_lookup,  "session.screen%d.workspaceNames", screen_number);
  sprintf(class_lookup, "Session.Screen%d.WorkspaceNames", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    std::string search = value.addr;
    std::string::const_iterator it = search.begin(),
                               end = search.end();
    while (1) {
      std::string::const_iterator tmp = it; // current string.begin()
      it = std::find(tmp, end, ',');   // look for comma between tmp and end
      screen->addWorkspaceName(std::string(tmp, it)); // string = search[tmp:it]
      if (it == end) break;
      ++it;
    }
  }

  sprintf(name_lookup,  "session.screen%d.toolbar.onTop", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Toolbar.OnTop", screen_number);
  screen->resource().saveToolbarOnTop(False);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "true", value.size)) {
    screen->resource().saveToolbarOnTop(True);
  }

  sprintf(name_lookup,  "session.screen%d.toolbar.autoHide", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Toolbar.autoHide", screen_number);
  screen->resource().saveToolbarAutoHide(False);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "true", value.size)) {
    screen->resource().saveToolbarAutoHide(True);
  }

  sprintf(name_lookup,  "session.screen%d.focusModel", screen_number);
  sprintf(class_lookup, "Session.Screen%d.FocusModel", screen_number);
  screen->resource().saveSloppyFocus(True);
  screen->resource().saveAutoRaise(False);
  screen->resource().saveClickRaise(False);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    std::string fmodel = value.addr;

    if (fmodel.find("ClickToFocus") != std::string::npos) {
      screen->resource().saveSloppyFocus(False);
    } else {
      // must be sloppy

      if (fmodel.find("AutoRaise") != std::string::npos)
        screen->resource().saveAutoRaise(True);
      if (fmodel.find("ClickRaise") != std::string::npos)
        screen->resource().saveClickRaise(True);
    }
  }

  sprintf(name_lookup,  "session.screen%d.windowPlacement", screen_number);
  sprintf(class_lookup, "Session.Screen%d.WindowPlacement", screen_number);
  screen->resource().savePlacementPolicy(BScreen::RowSmartPlacement);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    if (! strncasecmp(value.addr, "RowSmartPlacement", value.size))
      /* pass */;
    else if (! strncasecmp(value.addr, "ColSmartPlacement", value.size))
      screen->resource().savePlacementPolicy(BScreen::ColSmartPlacement);
    else if (! strncasecmp(value.addr, "CascadePlacement", value.size))
      screen->resource().savePlacementPolicy(BScreen::CascadePlacement);
  }

  sprintf(name_lookup, "session.screen%d.slit.placement", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Slit.Placement", screen_number);
  screen->resource().saveSlitPlacement(Slit::CenterRight);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    if (! strncasecmp(value.addr, "TopLeft", value.size))
      screen->resource().saveSlitPlacement(Slit::TopLeft);
    else if (! strncasecmp(value.addr, "CenterLeft", value.size))
      screen->resource().saveSlitPlacement(Slit::CenterLeft);
    else if (! strncasecmp(value.addr, "BottomLeft", value.size))
      screen->resource().saveSlitPlacement(Slit::BottomLeft);
    else if (! strncasecmp(value.addr, "TopCenter", value.size))
      screen->resource().saveSlitPlacement(Slit::TopCenter);
    else if (! strncasecmp(value.addr, "BottomCenter", value.size))
      screen->resource().saveSlitPlacement(Slit::BottomCenter);
    else if (! strncasecmp(value.addr, "TopRight", value.size))
      screen->resource().saveSlitPlacement(Slit::TopRight);
    else if (! strncasecmp(value.addr, "BottomRight", value.size))
      screen->resource().saveSlitPlacement(Slit::BottomRight);
  }

  sprintf(name_lookup, "session.screen%d.slit.direction", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Slit.Direction", screen_number);
  screen->resource().saveSlitDirection(Slit::Vertical);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "Horizontal", value.size)) {
    screen->resource().saveSlitDirection(Slit::Horizontal);
  }

  sprintf(name_lookup, "session.screen%d.slit.onTop", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Slit.OnTop", screen_number);
  screen->resource().saveSlitOnTop(False);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "True", value.size)) {
    screen->resource().saveSlitOnTop(True);
  }

  sprintf(name_lookup, "session.screen%d.slit.autoHide", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Slit.AutoHide", screen_number);
  screen->resource().saveSlitAutoHide(False);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "true", value.size)) {
    screen->resource().saveSlitAutoHide(True);
  }

  sprintf(name_lookup,  "session.screen%d.strftimeFormat", screen_number);
  sprintf(class_lookup, "Session.Screen%d.StrftimeFormat", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    screen->resource().saveStrftimeFormat(value.addr);
  } else {
    screen->resource().saveStrftimeFormat("%I:%M %p");
  }

  sprintf(name_lookup,  "session.screen%d.edgeSnapThreshold", screen_number);
  sprintf(class_lookup, "Session.Screen%d.EdgeSnapThreshold", screen_number);
  screen->resource().saveEdgeSnapThreshold(0);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      sscanf(value.addr, "%d", &int_value) == 1) {
    screen->resource().saveEdgeSnapThreshold(int_value);
  }

  bt::DitherMode ditherMode = bt::OrderedDither;
  if (XrmGetResource(database, "session.imageDither", "Session.ImageDither",
                     &value_type, &value)) {
    if (! strncasecmp("ordereddither", value.addr, value.size))
      ditherMode = bt::OrderedDither;
    else if (! strncasecmp("floydsteinbergdither", value.addr, value.size))
      ditherMode = bt::FloydSteinbergDither;
    else if (! strncasecmp("nodither", value.addr, value.size))
      ditherMode = bt::NoDither;
  }
  bt::Image::setDitherMode(ditherMode);

  screen->resource().saveOpaqueMove(False);
  if (XrmGetResource(database, "session.opaqueMove", "Session.OpaqueMove",
                     &value_type, &value) &&
      ! strncasecmp("true", value.addr, value.size)) {
    screen->resource().saveOpaqueMove(True);
  }

  XrmDestroyDatabase(database);
}


void Blackbox::reload_rc(void) {
  load_rc();
  reconfigure();
}


void Blackbox::reconfigure(void) {
  reconfigure_wait = True;

  if (! timer->isTiming()) timer->start();
}


void Blackbox::real_reconfigure(void) {
  XrmDatabase new_blackboxrc = (XrmDatabase) 0;

  std::string style = "session.styleFile: " + resource.style_file;
  XrmPutLineResource(&new_blackboxrc, style.c_str());

  XrmDatabase old_blackboxrc = XrmGetFileDatabase(rc_file.c_str());

  XrmMergeDatabases(new_blackboxrc, &old_blackboxrc);
  XrmPutFileDatabase(old_blackboxrc, rc_file.c_str());
  if (old_blackboxrc) XrmDestroyDatabase(old_blackboxrc);

  std::for_each(menuTimestamps.begin(), menuTimestamps.end(),
                bt::PointerAssassin());
  menuTimestamps.clear();

  bt::Color::clearCache();
  bt::Font::clearCache();
  bt::Pen::clearCache();

  std::for_each(screen_list, screen_list + screen_list_count,
                std::mem_fun(&BScreen::reconfigure));

  bt::PixmapCache::clearCache();
}


void Blackbox::checkMenu(void) {
  bool reread = False;
  MenuTimestampList::iterator it = menuTimestamps.begin();
  for(; it != menuTimestamps.end(); ++it) {
    MenuTimestamp *tmp = *it;
    struct stat buf;

    if (! stat(tmp->filename.c_str(), &buf)) {
      if (tmp->timestamp != buf.st_ctime)
        reread = True;
    } else {
      reread = True;
    }
  }

  if (reread) rereadMenu();
}


void Blackbox::rereadMenu(void) {
  reread_menu_wait = True;

  if (! timer->isTiming()) timer->start();
}


void Blackbox::real_rereadMenu(void) {
  std::for_each(menuTimestamps.begin(), menuTimestamps.end(),
                bt::PointerAssassin());
  menuTimestamps.clear();

  std::for_each(screen_list, screen_list + screen_list_count,
                std::mem_fun(&BScreen::rereadMenu));
}


void Blackbox::saveStyleFilename(const std::string& filename) {
  assert(! filename.empty());
  resource.style_file = filename;
}


void Blackbox::saveMenuFilename(const std::string& filename) {
  assert(! filename.empty());
  bool found = False;

  MenuTimestampList::iterator it = menuTimestamps.begin();
  for (; it != menuTimestamps.end() && !found; ++it) {
    if ((*it)->filename == filename) found = True;
  }
  if (! found) {
    struct stat buf;

    if (! stat(filename.c_str(), &buf)) {
      MenuTimestamp *ts = new MenuTimestamp;

      ts->filename = filename;
      ts->timestamp = buf.st_ctime;

      menuTimestamps.push_back(ts);
    }
  }
}


void Blackbox::timeout(bt::Timer *) {
  if (reconfigure_wait)
    real_reconfigure();

  if (reread_menu_wait)
    real_rereadMenu();

  reconfigure_wait = reread_menu_wait = False;
}


void Blackbox::setActiveScreen(BScreen *screen) {
  if (active_screen && active_screen == screen) // nothing to do
    return;

  active_screen = screen;

  if (! focused_window || focused_window->getScreen() != active_screen)
    setFocusedWindow(0);
}


void Blackbox::setFocusedWindow(BlackboxWindow *win) {
  if (focused_window && focused_window == win) // nothing to do
    return;

  BScreen *old_screen = 0;

  if (focused_window) {
    focused_window->setFocusFlag(False);
    old_screen = focused_window->getScreen();
  }

  if (win && ! win->isIconic()) {
    // the active screen is the one with the newly focused window...
    active_screen = win->getScreen();
    focused_window = win;
  } else {
    // no active screen since no window is focused...
    active_screen = 0;
    focused_window = 0;
    // set input focus to PointerRoot
    XSetInputFocus(XDisplay(), PointerRoot, RevertToNone, CurrentTime);
  }

  Window active =
    (focused_window) ? focused_window->getClientWindow() : None;

  if (active_screen) {
    active_screen->getToolbar()->redrawWindowLabel();
    _netwm->setActiveWindow(active_screen->screenInfo().rootWindow(), active);
  }

  if (old_screen && old_screen != active_screen) {
    old_screen->getToolbar()->redrawWindowLabel();
    _netwm->setActiveWindow(old_screen->screenInfo().rootWindow(), active);
  }
}
