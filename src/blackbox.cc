// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// blackbox.cc for Blackbox - an X11 Window manager
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

#include "blackbox.hh"
#include "Screen.hh"
#include "Slit.hh"
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

// #define FOCUS_DEBUG
#ifdef FOCUS_DEBUG
static const char *Mode[] = {
  "Normal",
  "Grab",
  "Ungrab",
  "WhileGrabbed"
};

static const char *Detail[] = {
  "Ancestor",
  "Virtual",
  "Inferior",
  "Nonlinear",
  "NonlinearVirtual",
  "Pointer",
  "PointerRoot",
  "DetailNone"
};
#endif // FOCUS_DEBUG


void Blackbox::save_rc(void)
{ _resource.save(*this); }


void Blackbox::load_rc(void)
{ _resource.load(*this); }


void Blackbox::reload_rc(void) {
  load_rc();
  reconfigure();
}

void Blackbox::init_icccm(void) {
  const char* atoms[13] = {
    "MANAGER",
    "_MOTIF_WM_HINTS",
    "SM_CLIENT_ID",
    "WM_CHANGE_STATE",
    "WM_CLIENT_LEADER",
    "WM_COLORMAP_NOTIFY",
    "WM_COLORMAP_WINDOWS",
    "WM_DELETE_WINDOW",
    "WM_PROTOCOLS",
    "WM_SAVE_YOURSELF",
    "WM_STATE",
    "WM_TAKE_FOCUS",
    "WM_WINDOW_ROLE"
  };
  Atom atoms_return[13];
  XInternAtoms(XDisplay(), const_cast<char **>(atoms), 13, false, atoms_return);
  xa_manager = atoms_return[0];
  motif_wm_hints = atoms_return[1];
  xa_sm_client_id = atoms_return[2];
  xa_wm_change_state = atoms_return[3];
  xa_wm_client_leader = atoms_return[4];
  xa_wm_colormap_notify = atoms_return[5];
  xa_wm_colormap_windows = atoms_return[6];
  xa_wm_delete_window = atoms_return[7];
  xa_wm_protocols = atoms_return[8];
  xa_wm_save_yourself = atoms_return[9];
  xa_wm_state = atoms_return[10];
  xa_wm_take_focus = atoms_return[11];
  xa_wm_window_role = atoms_return[12];

  _ewmh = new bt::EWMH(display());
}


void Blackbox::updateActiveWindow() const {
  Window active = (focused_window) ? focused_window->clientWindow() : None;
  for (unsigned int i = 0; i < display().screenCount(); ++i)
    _ewmh->setActiveWindow(display().screenInfo(i).rootWindow(), active);
}


void Blackbox::shutdown(void) {
  bt::Application::shutdown();

  XGrabServer();

  XSetInputFocus(XDisplay(), PointerRoot, RevertToPointerRoot, XTime());

  std::for_each(screen_list, screen_list + screen_list_count,
                std::mem_fun(&BScreen::shutdown));

  XSync(XDisplay(), false);

  XUngrabServer();
}


static Bool scanForFocusIn(Display *, XEvent *e, XPointer) {
  if (e->type == FocusIn
      && e->xfocus.mode != NotifyGrab
      && (e->xfocus.detail == NotifyNonlinearVirtual
          || e->xfocus.detail == NotifyVirtual)) {
    return true;
  }
  return false;
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
      if ((!activeScreen() || activeScreen() == win->screen()) &&
          (win->isTransient() || _resource.focusNewWindows()))
        win->activate();
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

  case ConfigureRequest: {
    BlackboxWindow *win = findWindow(e->xconfigurerequest.window);
    if (win) {
      // a window wants to resize
      win->configureRequestEvent(&e->xconfigurerequest);
      break;
    }

    Slit *slit =
      dynamic_cast<Slit *>(findEventHandler(e->xconfigurerequest.parent));
    if (slit) {
      // something in the slit wants to resize
      slit->configureRequestEvent(&e->xconfigurerequest);
      break;
    }

    /*
      handle configure requests for windows that have no EventHandlers
      by simply configuring them as requested.

      note: the event->window parameter points to the window being
      configured, and event->parent points to the window that received
      the event (in this case, the root window, since
      SubstructureRedirect has been selected).
    */
    XWindowChanges xwc;
    xwc.x = e->xconfigurerequest.x;
    xwc.y = e->xconfigurerequest.y;
    xwc.width = e->xconfigurerequest.width;
    xwc.height = e->xconfigurerequest.height;
    xwc.border_width = e->xconfigurerequest.border_width;
    xwc.sibling = e->xconfigurerequest.above;
    xwc.stack_mode = e->xconfigurerequest.detail;
    XConfigureWindow(XDisplay(),
                     e->xconfigurerequest.window,
                     e->xconfigurerequest.value_mask,
                     &xwc);
    break;
  }

  case FocusIn: {
#ifdef FOCUS_DEBUG
    printf("FocusIn : window %8lx mode %s detail %s\n",
           e->xfocus.window, Mode[e->xfocus.mode], Detail[e->xfocus.detail]);
#endif

    if (e->xfocus.mode == NotifyGrab
        || (e->xfocus.detail != NotifyNonlinearVirtual
            && e->xfocus.detail != NotifyVirtual)) {
      /*
        don't process FocusIns when:
        1. they are the result of a grab
        2. the new focus window isn't an ancestor or inferior of the
        old focus window (NotifyNonlinearVirtual and NotifyVirtual)
      */
      break;
    }

    BlackboxWindow *win = findWindow(e->xfocus.window);
    if (!win || win->isFocused())
      break;

#ifdef FOCUS_DEBUG
    printf("          win %p got focus\n", win);
#endif
    win->setFocused(true);
    setFocusedWindow(win);

    /*
      set the event window to None.  when the FocusOut event handler calls
      this function recursively, it uses this as an indication that focus
      has moved to a known window.
    */
    e->xfocus.window = None;

    break;
  }

  case FocusOut: {
#ifdef FOCUS_DEBUG
    printf("FocusOut: window %8lx mode %s detail %s\n",
           e->xfocus.window, Mode[e->xfocus.mode], Detail[e->xfocus.detail]);
#endif

    if (e->xfocus.mode == NotifyGrab
        || (e->xfocus.detail != NotifyNonlinearVirtual
            && e->xfocus.detail != NotifyVirtual)) {
      /*
        don't process FocusOuts when:
        1. they are the result of a grab
        2. the new focus window isn't an ancestor or inferior of the
        old focus window (NotifyNonlinearVirtual and NotifyNonlinearVirtual)
      */
      break;
    }

    BlackboxWindow *win = findWindow(e->xfocus.window);
    if (!win || !win->isFocused())
      break;

    bool lost_focus = true; // did the window really lose focus?
    bool no_focus = true;   // did another window get focus?

    XEvent event;
    if (XCheckIfEvent(XDisplay(), &event, scanForFocusIn, NULL)) {
      process_event(&event);

      if (event.xfocus.window == None)
        no_focus = false;
    } else {
      XWindowAttributes attr;
      Window w;
      int unused;
      XGetInputFocus(XDisplay(), &w, &unused);
      if (w != None
          && XGetWindowAttributes(XDisplay(), w, &attr)
          && attr.override_redirect) {
#ifdef FOCUS_DEBUG
        printf("          focused moved to an override_redirect window\n");
#endif
        lost_focus = (e->xfocus.mode == NotifyNormal);
      }
    }

    if (lost_focus) {
#ifdef FOCUS_DEBUG
      printf("          win %p lost focus\n", win);
#endif
      win->setFocused(false);

      if (no_focus) {
#ifdef FOCUS_DEBUG
        printf("          no window has focus\n");
#endif
        setFocusedWindow(0);
      }
    }

    break;
  }

  case SelectionClear: {
    // shutdown if lost WM_S[n] selection
    Atom selection = e->xselectionclear.selection;
    for (unsigned int i = 0; i < screen_list_count; ++i)
      if (selection == screen_list[i]->wmSelection())
        setRunState(SHUTDOWN);
    break;
  }

  default:
    // Send the event through the default EventHandlers.
    bt::Application::process_event(e);
    break;
  } // switch
}


bool Blackbox::process_signal(int sig) {
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

  case SIGQUIT:
    restart();
    break;

  default:
    return bt::Application::process_signal(sig);
  } // switch

  return true;
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

  std::for_each(screen_list, screen_list + screen_list_count,
                std::mem_fun(&BScreen::reconfigure));

  bt::Color::clearCache();
  bt::Font::clearCache();
  bt::PixmapCache::clearCache();
}


Blackbox::Blackbox(char **m_argv, int m_argc, const char *dpy_name,
                   const std::string& rc, bool multi_head)
  : bt::Application(m_argv[0], dpy_name, multi_head),
    grab_count(0u), _resource(rc)
{
  if (! XSupportsLocale())
    fprintf(stderr, "X server does not support locale\n");

  if (XSetLocaleModifiers("") == NULL)
    fprintf(stderr, "cannot set locale modifiers\n");

  argv = m_argv;
  argc = m_argc;

  active_screen = 0;
  focused_window = (BlackboxWindow *) 0;
  _ewmh = (bt::EWMH*) 0;

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
    fprintf(stderr, "%s: no manageable screens found, exiting...\n",
            applicationName().c_str());
    ::exit(3);
  }

  screen_list_count = managed;

  // start with the first managed screen as the active screen
  setActiveScreen(screen_list[0]);

  XSynchronize(XDisplay(), false);
  XSync(XDisplay(), false);

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
  delete _ewmh;
}


void Blackbox::XGrabServer(void) {
  if (grab_count++ == 0)
    ::XGrabServer(XDisplay());
}


void Blackbox::XUngrabServer(void) {
  if (--grab_count == 0)
    ::XUngrabServer(XDisplay());
}


BScreen *Blackbox::findScreen(Window window) const {
  for (unsigned int i = 0; i < screen_list_count; ++i)
    if (screen_list[i]->screenInfo().rootWindow() == window)
      return screen_list[i];
  return 0;
}


void Blackbox::setActiveScreen(BScreen *screen) {
  if (active_screen && active_screen == screen) // nothing to do
    return;

  assert(screen != 0);
  active_screen = screen;

  // install screen colormap
  XInstallColormap(XDisplay(), active_screen->screenInfo().colormap());

  if (! focused_window || focused_window->screen() != active_screen)
    setFocusedWindow(0);
}


BScreen* Blackbox::screenNumber(unsigned int n) {
  assert(n < screen_list_count);
  return screen_list[n];
}


BlackboxWindow *Blackbox::findWindow(Window window) const {
  WindowLookup::const_iterator it = windowSearchList.find(window);
  if (it != windowSearchList.end())
    return it->second;
  return 0;
}


void Blackbox::insertWindow(Window window, BlackboxWindow *data)
{ windowSearchList.insert(WindowLookupPair(window, data)); }


void Blackbox::removeWindow(Window window)
{ windowSearchList.erase(window); }


BWindowGroup *Blackbox::findWindowGroup(Window window) const {
  GroupLookup::const_iterator it = groupSearchList.find(window);
  if (it != groupSearchList.end())
    return it->second;
  return 0;
}


void Blackbox::insertWindowGroup(Window window, BWindowGroup *data)
{ groupSearchList.insert(GroupLookupPair(window, data)); }


void Blackbox::removeWindowGroup(Window window)
{ groupSearchList.erase(window); }


void Blackbox::setFocusedWindow(BlackboxWindow *win) {
  if (focused_window && focused_window == win) // nothing to do
    return;

  if (win && !win->isIconic()) {
    // the active screen is the one with the newly focused window...
    active_screen = win->screen();
    focused_window = win;
  } else {
    // nothing has focus
    focused_window = 0;
    assert(active_screen != 0);
    XSetInputFocus(XDisplay(), active_screen->noFocusWindow(),
                   RevertToPointerRoot, XTime());
  }

  updateActiveWindow();
}


void Blackbox::restart(const std::string &prog) {
  setRunState(bt::Application::SHUTDOWN);

  /*
    since we don't allow control to return to the eventloop, we need
    to call shutdown() explicitly
  */
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


void Blackbox::reconfigure(void) {
  if (! timer->isTiming())
    timer->start();
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


void Blackbox::checkMenu(void) {
  bool reread = false;
  MenuTimestampList::iterator it = menuTimestamps.begin();
  for(; it != menuTimestamps.end(); ++it) {
    MenuTimestamp *tmp = *it;
    struct stat buf;

    if (! stat(tmp->filename.c_str(), &buf)) {
      if (tmp->timestamp != buf.st_ctime)
        reread = true;
    } else {
      reread = true;
    }
  }

  if (reread)
    rereadMenu();
}


void Blackbox::rereadMenu(void) {
  std::for_each(menuTimestamps.begin(), menuTimestamps.end(),
                bt::PointerAssassin());
  menuTimestamps.clear();

  std::for_each(screen_list, screen_list + screen_list_count,
                std::mem_fun(&BScreen::rereadMenu));
}
