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

#include "blackbox.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"

#include <Pen.hh>
#include <PixmapCache.hh>
#include <Util.hh>

#include <X11/Xresource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>


Blackbox::Blackbox(char **m_argv, const char *dpy_name,
                   const std::string& rc, bool multi_head)
  : bt::Application(m_argv[0], dpy_name, multi_head), _resource(rc) {
  if (! XSupportsLocale())
    fprintf(stderr, "X server does not support locale\n");

  if (XSetLocaleModifiers("") == NULL)
    fprintf(stderr, "cannot set locale modifiers\n");

  argv = m_argv;

  no_focus = False;

  active_screen = 0;
  focused_window = (BlackboxWindow *) 0;
  _netwm = (bt::Netwm*) 0;

  init_icccm();

  if (! multi_head || display().screenCount() == 1)
    screen_list_count = 1;
  else
    screen_list_count = display().screenCount();

  _resource.load(*this);

  screen_list = new BScreen*[screen_list_count];
  unsigned int managed = 0;
  for (unsigned int i = 0; i < screen_list_count; ++i) {
    BScreen *screen = new BScreen(this, i);

    if (! screen->isScreenManaged()) {
      delete screen;
      continue;
    }

    screen_list[i] = screen;
    ++managed;
  }

  if (managed == 0) {
    fprintf(stderr, "%s: no managable screens found, exiting...\n",
            applicationName().c_str());
    ::exit(3);
  }

  screen_list_count = managed;

  // start with the first managed screen as the active screen
  setActiveScreen(screen_list[0]);

  XSynchronize(XDisplay(), False);
  XSync(XDisplay(), False);

  reconfigure_wait = False;

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
      bool focus = false;
      if (win->isIconic()) {
        win->show();
        focus = true;
      }
      if (win->isShaded()) {
        win->setShaded(false);
        focus = true;
      }

      if (focus &&
          (!activeScreen() || activeScreen() == win->getScreen()) &&
          (win->isTransient() || win->getScreen()->resource().doFocusNew()))
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

  case FocusIn: {
    if (e->xfocus.detail != NotifyNonlinear) {
      /*
        don't process FocusIns when:
        1. the new focus window isn't an ancestor or inferior of the
        old focus window (NotifyNonlinear)
      */
      break;
    }

    BlackboxWindow *win = findWindow(e->xfocus.window);
    if (!win) break;

    if (!win->isFocused())
      win->setFocused(true);

    /*
      set the event window to None.  when the FocusOut event handler calls
      this function recursively, it uses this as an indication that focus
      has moved to a known window.
    */
    e->xfocus.window = None;
    break;
  }

  case FocusOut: {
    if (e->xfocus.detail != NotifyNonlinear) {
      /*
        don't process FocusOuts when:
        1. the new focus window isn't an ancestor or inferior of the
        old focus window (NotifyNonlinear)
      */
      break;
    }

    BlackboxWindow *win = findWindow(e->xfocus.window);
    if (!win || !win->isFocused()) break;

    /*
      before we mark "win" as unfocused, we need to verify that focus
      is going to a known location, is in a known location, or set
      focus to a known location.
    */

    XEvent event;
    // don't check the current focus if FocusOut was generated by a grab
    bool check_focus = (e->xfocus.mode != NotifyGrab);

    /*
      First, check if there is a pending FocusIn event waiting.  if
      there is, process it and determine if focus has moved to another
      window (the FocusIn event handler sets the window in the event
      structure to None to indicate this).
    */
    if (XCheckTypedEvent(XDisplay(), FocusIn, &event)) {

      process_event(&event);
      if (event.xfocus.window == None) {
        // focus has moved
        check_focus = False;
      }
    }

    if (!check_focus) break;

    /*
      Second, we query the X server for the current input focus.  to
      make sure that we keep a consistent state.
    */
    BlackboxWindow *focus;
    Window w;
    int revert;
    XGetInputFocus(XDisplay(), &w, &revert);
    focus = findWindow(w);
    if (focus) {
      /*
        focus got from "win" to "focus" under some very strange
        circumstances, and we need to make sure that the focus
        indication is correct.
      */
      setFocusedWindow(focus);
    } else {
      // we have no idea where focus went... so we set it to PointerRoot
      setFocusedWindow(0);
    }

    break;
  }

  default:
    // Send the event through the default EventHandlers.
    bt::Application::process_event(e);
    break;
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
  std::string name = bt::basename(argv[0]);
  execvp(name.c_str(), argv);
}


void Blackbox::shutdown(void) {
  bt::Application::shutdown();

  XSetInputFocus(XDisplay(), PointerRoot, RevertToNone, CurrentTime);

  std::for_each(screen_list, screen_list + screen_list_count,
                std::mem_fun(&BScreen::shutdown));

  XSync(XDisplay(), False);
}


void Blackbox::save_rc(void) {
  _resource.save(*this);
}


void Blackbox::load_rc(void) {
  _resource.load(*this);
}


void Blackbox::reload_rc(void) {
  load_rc();
  reconfigure();
}


void Blackbox::reconfigure(void) {
  if (! timer->isTiming())
    timer->start();
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
  std::for_each(menuTimestamps.begin(), menuTimestamps.end(),
                bt::PointerAssassin());
  menuTimestamps.clear();

  std::for_each(screen_list, screen_list + screen_list_count,
                std::mem_fun(&BScreen::rereadMenu));
}


void Blackbox::saveMenuFilename(const std::string& filename) {
  assert(!filename.empty());
  bool found = false;

  MenuTimestampList::iterator it = menuTimestamps.begin();
  for (; it != menuTimestamps.end() && !found; ++it)
    found = (*it)->filename == filename;
  if (found)
    return;

  struct stat buf;
  if (stat(filename.c_str(), &buf) != 0)
    return; // file doesn't exist

  MenuTimestamp *ts = new MenuTimestamp;
  ts->filename = filename;
  ts->timestamp = buf.st_ctime;
  menuTimestamps.push_back(ts);
}


void Blackbox::timeout(bt::Timer *) {
  XrmDatabase new_blackboxrc = (XrmDatabase) 0;

  std::string style = "session.styleFile: ";
  style += _resource.styleFilename();
  XrmPutLineResource(&new_blackboxrc, style.c_str());

  XrmDatabase old_blackboxrc = XrmGetFileDatabase(_resource.rcFilename());

  XrmMergeDatabases(new_blackboxrc, &old_blackboxrc);
  XrmPutFileDatabase(old_blackboxrc, _resource.rcFilename());
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
    focused_window->setFocused(false);
    old_screen = focused_window->getScreen();

    if (focused_window->isFullScreen() &&
        focused_window->layer() != StackingList::LayerBelow) {
      // move full-screen windows under all other windows (except
      // desktop windows) when loosing focus
      old_screen->changeLayer(focused_window, StackingList::LayerBelow);
    }
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
    if (active_screen->toolbar())
      active_screen->toolbar()->redrawWindowLabel();
    _netwm->setActiveWindow(active_screen->screenInfo().rootWindow(), active);
  }

  if (old_screen && old_screen != active_screen) {
    if (old_screen->toolbar())
      old_screen->toolbar()->redrawWindowLabel();
    _netwm->setActiveWindow(old_screen->screenInfo().rootWindow(), active);
  }
}


BScreen* Blackbox::screenNumber(unsigned int n) {
  assert(n < screen_list_count);
  return screen_list[n];
}
