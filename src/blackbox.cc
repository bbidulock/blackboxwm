// blackbox.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 1999 by Brad Hughes, bhughes@tcac.net
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// (See the included file COPYING / GPL-2.0)
//

#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#ifdef    SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE

#include "blackbox.hh"
#include "Basemenu.hh"
#include "Rootmenu.hh"
#include "Screen.hh"

#ifdef    SLIT
#include "Slit.hh"
#endif // SLIT

#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"

#ifdef    HAVE_SIGNAL_H
#  include <signal.h>
#endif // HAVE_SIGNAL_H

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS

#ifdef    HAVE_UNISTD_H
#  include <sys/types.h>
#  include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef    HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

#ifndef   MAXPATHLEN
#define   MAXPATHLEN 255
#endif // MAXPATHLEN

#ifdef    HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif // HAVE_SYS_SELECT_H

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

#ifdef    HAVE_SYS_WAIT_H
#  include <sys/types.h>
#  include <sys/wait.h>
#endif // HAVE_SYS_WAIT_H

#ifdef    HAVE_LIBGEN_H
#  include <libgen.h>
#endif // HAVE_LIBGEN_H

#if (defined(HAVE_PROCESS_H) && defined(__EMX__))
#  include <process.h>
#endif //    HAVE_PROCESS_H             __EMX__

#ifndef   HAVE_BASENAME
// this is taken from the GNU liberty codebase
static char *basename (const char *name) {
  const char *base = name;

  while (*name) if (*name++ == '/') base = name;

  return (char *) base;
}
#endif // HAVE_BASENAME


// signal handler to allow for proper and gentle shutdown
static Bool internal_error = False;
Blackbox *blackbox;

#ifndef   HAVE_SIGACTION
static RETSIGTYPE signalhandler(int sig) {
#else //  HAVE_SIGACTION
static void signalhandler(int sig) {
#endif // HAVE_SIGACTION

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

  case SIGHUP:
    blackbox->reconfigure();

#ifndef   HAVE_SIGACTION
    signal(SIGHUP, (RETSIGTYPE (*)(int)) signalhandler);

    break;
#endif // HAVE_SIGACTION

  case SIGUSR2:
    blackbox->rereadMenu();

#ifndef   HAVE_SIGACTION
    signal(SIGUSR2, (RETSIGTYPE (*)(int)) signalhandler);

    break;
#endif // HAVE_SIGACTION
    
  case SIGALRM:
#ifndef   HAVE_SIGACTION
    signal(SIGALRM, (RETSIGTYPE (*)(int)) signalhandler);
#endif // HAVE_SIGACTION
    
    break;
    
  default:
    fprintf(stderr, "%s:  signal %d caught\n", __FILE__, sig);

    if (! blackbox->isStartup() && ! re_enter) {
      internal_error = True;

      re_enter = 1;
      fprintf(stderr, "shutting down\n");
      blackbox->shutdown();
    }

    if (sig != SIGTERM && sig != SIGINT) {
      fprintf(stderr, "aborting... dumping core\n");
      abort();
    }
    
    exit(0);
    
    break;
  }
}


// X error handler to handle any and all X errors while blackbox is running
static int handleXErrors(Display *d, XErrorEvent *e) {
  char errtxt[128];
  XGetErrorText(d, e->error_code, errtxt, 128);
  fprintf(stderr, "%s:  X error: %s(%d) opcodes %d/%d\n", __FILE__, errtxt,
          e->error_code, e->request_code, e->minor_code);

  if (internal_error) abort();

  return(False);
}


// X event scanner for enter/leave notifies - adapted from twm
typedef struct scanargs {
  Window w;
  Bool leave, inferior, enter;
} scanargs;

static Bool queueScanner(Display *, XEvent *e, char *args) {
  if ((e->type == LeaveNotify) &&
      (e->xcrossing.window == ((scanargs *) args)->w) &&
      (e->xcrossing.mode == NotifyNormal)) {
    ((scanargs *) args)->leave = True;
    ((scanargs *) args)->inferior = (e->xcrossing.detail == NotifyInferior);
  } else if ((e->type == EnterNotify) &&
             (e->xcrossing.mode == NotifyUngrab))
    ((scanargs *) args)->enter = True;

  return False;
}
  

Blackbox::Blackbox(int m_argc, char **m_argv, char *dpy_name, char *rc)
  : BaseDisplay(dpy_name)
{
  grab();
  
#ifdef    HAVE_SIGACTION
  struct sigaction action;

  action.sa_handler = signalhandler;
  action.sa_mask = sigset_t();
  action.sa_flags = SA_NOCLDSTOP | SA_NODEFER;

  sigaction(SIGSEGV, &action, NULL);
  sigaction(SIGFPE, &action, NULL);
  sigaction(SIGTERM, &action, NULL);
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGCHLD, &action, NULL);
  sigaction(SIGHUP, &action, NULL);
  sigaction(SIGALRM, &action, NULL);
  sigaction(SIGUSR2, &action, NULL);
#else // !HAVE_SIGACTION
  signal(SIGSEGV, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGFPE, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGTERM, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGINT, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGUSR2, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGHUP, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGALRM, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGCHLD, (RETSIGTYPE (*)(int)) signalhandler);
#endif // HAVE_SIGACTION
  
  ::blackbox = this;
  argc = m_argc;
  argv = m_argv;
  rc_file = rc;

  no_focus = auto_raise_pending = _reconfigure = _reread_menu = False;

  resource.menu_file = resource.style_file = (char *) 0;
  resource.auto_raise_delay_sec = resource.auto_raise_delay_usec = 0;
  resource.colormap_focus_follows_mouse = False;

  auto_raise_window = focused_window = (BlackboxWindow *) 0;

#ifdef    KDE
  kwm_current_desktop = XInternAtom(getDisplay(), "KWM_CURRENT_DESKTOP", False);
  kwm_number_of_desktops =
    XInternAtom(getDisplay(), "KWM_NUMBER_OF_DESKTOPS", False);
  kwm_active_window = XInternAtom(getDisplay(), "KWM_ACTIVE_WINDOW", False);
  kwm_win_iconified = XInternAtom(getDisplay(), "KWM_WIN_ICONIFIED", False);
  kwm_win_sticky = XInternAtom(getDisplay(), "KWM_WIN_STICKY", False);
  kwm_win_maximized = XInternAtom(getDisplay(), "KWM_WIN_MAXIMIZED", False);
  kwm_win_decoration = XInternAtom(getDisplay(), "KWM_WIN_DECORATION", False);
  kwm_win_icon = XInternAtom(getDisplay(), "KWM_WIN_ICON", False);
  kwm_win_desktop = XInternAtom(getDisplay(), "KWM_WIN_DESKTOP", False);
  kwm_win_frame_geometry =
    XInternAtom(getDisplay(), "KWM_WIN_FRAME_GEOMETRY", False);
     
  kwm_command = XInternAtom(getDisplay(), "KWM_COMMAND", False);
  kwm_do_not_manage = XInternAtom(getDisplay(), "KWM_DO_NOT_MANAGE", False);
  kwm_activate_window =
     XInternAtom(getDisplay(), "KWM_ACTIVATE_WINDOW", False);

  kwm_running = XInternAtom(getDisplay(), "KWM_RUNNING", False);
  kwm_module = XInternAtom(getDisplay(), "KWM_MODULE", False);
  kwm_module_init = XInternAtom(getDisplay(), "KWM_MODULE_INIT", False);
  kwm_module_initialized =
    XInternAtom(getDisplay(), "KWM_MODULE_INITIALIZED", False);
  kwm_module_desktop_change =
    XInternAtom(getDisplay(), "KWM_MODULE_DESKTOP_CHANGE", False);
  kwm_module_win_change =
    XInternAtom(getDisplay(), "KWM_MODULE_WIN_CHANGE", False);
  kwm_module_desktop_name_change =
    XInternAtom(getDisplay(), "KWM_MODULE_DESKTOP_NAME_CHANGE", False);
  kwm_module_desktop_number_change =
    XInternAtom(getDisplay(), "KWM_MODULE_DESKTOP_NUMBER_CHANGE", False);
  kwm_module_win_add = XInternAtom(getDisplay(), "KWM_MODULE_WIN_ADD", False);
  kwm_module_win_remove =
    XInternAtom(getDisplay(), "KWM_MODULE_WIN_REMOVE", False);
  kwm_module_win_raise =
    XInternAtom(getDisplay(), "KWM_MODULE_WIN_RAISE", False);
  kwm_module_win_lower =
    XInternAtom(getDisplay(), "KWM_MODULE_WIN_LOWER", False);
  kwm_module_win_activate= 
    XInternAtom(getDisplay(), "KWM_MODULE_WIN_ACTIVATE", False);

  kwm_window_region_1 = XInternAtom(getDisplay(), "KWM_WINDOW_REGION_1", False);
#endif // KDE

  windowSearchList = new LinkedList<WindowSearch>;
  menuSearchList = new LinkedList<MenuSearch>;

#ifdef    SLIT
  slitSearchList = new LinkedList<SlitSearch>;
#endif // SLIT

  toolbarSearchList = new LinkedList<ToolbarSearch>;
  groupSearchList = new LinkedList<GroupSearch>;

  menuTimestamps = new LinkedList<MenuTimestamp>;
  
  XrmInitialize();
  load_rc();

  XSetErrorHandler((XErrorHandler) handleXErrors);
  
  screenList = new LinkedList<BScreen>;
  int i;
  for (i = 0; i < getNumberOfScreens(); i++) {
    BScreen *screen = new BScreen(this, i);
    
    if (! screen->isScreenManaged()) {
      delete screen;
      continue;
    }
    
    screenList->insert(screen);
  }

  if (! screenList->count()) {
    fprintf(stderr,
            "Blackbox::Blackbox: no managable screens found, aborting.\n");
    
    ::exit(3);
  }
  
  XSynchronize(getDisplay(), False);
  XSync(getDisplay(), False);
  
  ungrab();
}


Blackbox::~Blackbox(void) {
  while (screenList->count())
    delete screenList->remove(0);

  while (menuTimestamps->count()) {
    MenuTimestamp *ts = menuTimestamps->remove(0);

    if (ts->filename) delete [] ts->filename;
    delete ts;
  }

  if (resource.menu_file) delete [] resource.menu_file;
  if (resource.style_file) delete [] resource.style_file;

  delete screenList;
  delete menuTimestamps;

  delete windowSearchList;
  delete menuSearchList;
  delete toolbarSearchList;
  delete groupSearchList;

#ifdef    SLIT
  delete slitSearchList;
#endif // SLIT
}


void Blackbox::eventLoop(void) {
  run();
  _reconfigure = _reread_menu = False;

  int xfd = ConnectionNumber(getDisplay());
  time_t lastTime = time(NULL);

  // scale time to the current minute.. since blackbox will redraw the clock
  // every minute on the minute
  lastTime = ((lastTime / 60) * 60);

  while (! doShutdown() && (! internal_error)) {
    if (_reconfigure) {
      do_reconfigure();
      _reconfigure = False;
    } else if (_reread_menu) {
      rereadMenu();
      _reread_menu = False;
    } else if (XPending(getDisplay())) {
      XEvent e;
      XNextEvent(getDisplay(), &e);
      process_event(&e);

      if (time(NULL) - lastTime > 59) {
	LinkedListIterator<BScreen> it(screenList);
	for (; it.current(); it++)
	  it.current()->getToolbar()->checkClock();
	
	lastTime = time(NULL);
      } else if (time(NULL) < lastTime)
        lastTime = time(NULL);
    } else if (auto_raise_pending) {
      struct itimerval remain;

      getitimer(ITIMER_REAL, &remain);

      if ((remain.it_value.tv_usec == 0) &&
          (remain.it_value.tv_sec == 0)) {
        grab();

        if (auto_raise_window && auto_raise_window->validateClient()) {
          auto_raise_window->getScreen()->
            getWorkspace(auto_raise_window->getWorkspaceNumber())->
              raiseWindow(auto_raise_window);
        }

        auto_raise_window = (BlackboxWindow *) 0;
        auto_raise_pending = False;

        ungrab();
      } 
    } else {
      // put a wait on the network file descriptor for the X connection...
      // this saves blackbox from eating all available cpu

      if (time(NULL) - lastTime < 60) {
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(xfd, &rfds);
	
	struct timeval tv;
	tv.tv_sec = 60 - (time(NULL) - lastTime);
	tv.tv_usec = 0;
	
	select(xfd + 1, &rfds, 0, 0, &tv);
      } else {
	// this will be done every 60 seconds
	LinkedListIterator<BScreen> it(screenList);
	for (; it.current(); it++)
	  it.current()->getToolbar()->checkClock();
	
	lastTime = time(NULL);
      }
    }
  }

  if (internal_error) {
    shutdown();
    save_rc();

    fprintf(stderr, "aborting... dumping core\n");
    abort();
  }

  save_rc();
}


void Blackbox::process_event(XEvent *e) {
  switch (e->type) {
  case ButtonPress:
    {
      BlackboxWindow *win = (BlackboxWindow *) 0;
      Basemenu *menu = (Basemenu *) 0;

#ifdef    SLIT
      Slit *slit = (Slit *) 0;
#endif // SLIT

      Toolbar *tbar = (Toolbar *) 0; 
      
      if ((win = searchWindow(e->xbutton.window))) {
	win->buttonPressEvent(&e->xbutton);

	if (e->xbutton.button == 1)
	  win->installColormap(True);
      } else if ((menu = searchMenu(e->xbutton.window))) {
	menu->buttonPressEvent(&e->xbutton);

#ifdef    SLIT
      } else if ((slit = searchSlit(e->xbutton.window))) {
	slit->buttonPressEvent(&e->xbutton);
#endif // SLIT
	
      } else if ((tbar = searchToolbar(e->xbutton.window))) {
	tbar->buttonPressEvent(&e->xbutton);
      } else {
	LinkedListIterator<BScreen> it(screenList);

	for (; it.current(); it++) {
	  BScreen *screen = it.current();

	  if (e->xbutton.window == screen->getRootWindow()) {
	    if (e->xbutton.button == 1) {
              if (! screen->isRootColormapInstalled())
	        screen->getImageControl()->installRootColormap();
	      
	      if (screen->getWorkspacemenu()->isVisible())
		screen->getWorkspacemenu()->hide();

              if (screen->getRootmenu()->isVisible())
                screen->getRootmenu()->hide();
            } else if (e->xbutton.button == 2) {
	      int mx = e->xbutton.x_root -
		(screen->getWorkspacemenu()->getWidth() / 2);
	      int my = e->xbutton.y_root -
		(screen->getWorkspacemenu()->getTitleHeight() / 2);
	      
	      if (mx < 0) mx = 0;
	      if (my < 0) my = 0;

	      if (mx + screen->getWorkspacemenu()->getWidth() >
		  screen->getWidth())
		mx = screen->getWidth() -
		  screen->getWorkspacemenu()->getWidth() -
                  screen->getBorderWidth();

	      if (my + screen->getWorkspacemenu()->getHeight() >
		  screen->getHeight())
		my = screen->getHeight() -
		  screen->getWorkspacemenu()->getHeight() - 
                  screen->getBorderWidth();
	      
	      screen->getWorkspacemenu()->move(mx, my);
	      
	      if (! screen->getWorkspacemenu()->isVisible())
		screen->getWorkspacemenu()->show();
	    } else if (e->xbutton.button == 3) {
	      int mx = e->xbutton.x_root -
		(screen->getRootmenu()->getWidth() / 2);
	      int my = e->xbutton.y_root -
		(screen->getRootmenu()->getTitleHeight() / 2);
	      
	      if (mx < 0) mx = 0;
	      if (my < 0) my = 0;

	      if (mx + screen->getRootmenu()->getWidth() > screen->getWidth())
		mx = screen->getWidth() -
		  screen->getRootmenu()->getWidth() -
                  screen->getBorderWidth();

	      if (my + screen->getRootmenu()->getHeight() > screen->getHeight())
		my = screen->getHeight() -
		  screen->getRootmenu()->getHeight() -
                  screen->getBorderWidth();

	      screen->getRootmenu()->move(mx, my);
	      
	      if (! screen->getRootmenu()->isVisible()) {
		checkMenu();
		screen->getRootmenu()->show();
	      }
	    }
	  }
	}
      }
      
      break;
    }
    
  case ButtonRelease:
    {
      BlackboxWindow *win = (BlackboxWindow *) 0;
      Basemenu *menu = (Basemenu *) 0;
      Toolbar *tbar = (Toolbar *) 0;
      
      if ((win = searchWindow(e->xbutton.window)))
	win->buttonReleaseEvent(&e->xbutton);
      else if ((menu = searchMenu(e->xbutton.window)))
	menu->buttonReleaseEvent(&e->xbutton);
      else if ((tbar = searchToolbar(e->xbutton.window)))
	tbar->buttonReleaseEvent(&e->xbutton);
      
      break;
    }
    
  case ConfigureRequest:
    {
      BlackboxWindow *win = (BlackboxWindow *) 0;

#ifdef    SLIT
      Slit *slit = (Slit *) 0;
#endif // SLIT
      
      if ((win = searchWindow(e->xconfigurerequest.window))) {
	win->configureRequestEvent(&e->xconfigurerequest);
	
#ifdef    SLIT
      } else if ((slit = searchSlit(e->xconfigurerequest.window))) {
	slit->configureRequestEvent(&e->xconfigurerequest);
#endif // SLIT
	
      } else {
	grab();
	
	if (validateWindow(e->xconfigurerequest.window)) {
	  XWindowChanges xwc;
	  
	  xwc.x = e->xconfigurerequest.x;
	  xwc.y = e->xconfigurerequest.y;	
	  xwc.width = e->xconfigurerequest.width;
	  xwc.height = e->xconfigurerequest.height;
	  xwc.border_width = e->xconfigurerequest.border_width;
	  xwc.sibling = e->xconfigurerequest.above;
	  xwc.stack_mode = e->xconfigurerequest.detail;
	  
	  XConfigureWindow(getDisplay(), e->xconfigurerequest.window,
			   e->xconfigurerequest.value_mask, &xwc);
	}
	
	ungrab();
      }
      
      break;
    }
    
  case MapRequest:
    {
      BlackboxWindow *win = searchWindow(e->xmaprequest.window);
      
      if ((! win) && validateWindow(e->xmaprequest.window)) {
	grab();

	BScreen *screen = searchScreen(e->xmaprequest.parent);
	
#ifdef    SLIT
	if (screen) {
	  XWMHints *wmhints = XGetWMHints(getDisplay(), e->xmaprequest.window);
	  
	  if (wmhints) {
            if ((wmhints->flags & StateHint) &&
	        (wmhints->initial_state == WithdrawnState))
	      screen->getSlit()->addClient(e->xmaprequest.window);
	    else
#endif // SLIT
	    
	      win = new BlackboxWindow(this, screen, e->xmaprequest.window);
	    
#ifdef    SLIT
            XFree(wmhints);
          } else
	    win = new BlackboxWindow(this, screen, e->xmaprequest.window);
	}
#endif // SLIT
	
	ungrab();
      }
      
      if ((win = searchWindow(e->xmaprequest.window)))
	win->mapRequestEvent(&e->xmaprequest);
      
      break;
    }
  
  case MapNotify:
    {
      BlackboxWindow *win = searchWindow(e->xmap.window);

      if (win)
	win->mapNotifyEvent(&e->xmap);
      
      break;
    }
  
  case UnmapNotify:
    {
      BlackboxWindow *win = (BlackboxWindow *) 0;

#ifdef    SLIT
      Slit *slit = (Slit *) 0;
#endif // SLIT

      if ((win = searchWindow(e->xunmap.window))) {
        if (auto_raise_window == win && auto_raise_pending) {
          auto_raise_window = (BlackboxWindow *) 0;
          auto_raise_pending = False;
        }

	win->unmapNotifyEvent(&e->xunmap);

#ifdef    SLIT
      } else if ((slit = searchSlit(e->xunmap.window))) {
        slit->removeClient(e->xunmap.window);
#endif // SLIT

      }

      break;
    }
    
  case DestroyNotify:
    {
      BlackboxWindow *win = (BlackboxWindow *) 0;

#ifdef    SLIT
      Slit *slit = (Slit *) 0;
#endif // SLIT
      
      if ((win = searchWindow(e->xdestroywindow.window))) {
        if (auto_raise_window == win && auto_raise_pending) {
          auto_raise_window = (BlackboxWindow *) 0;
          auto_raise_pending = False;
        }

        win->destroyNotifyEvent(&e->xdestroywindow);

#ifdef    SLIT
      } else if ((slit = searchSlit(e->xdestroywindow.window))) {
        slit->removeClient(e->xdestroywindow.window);
#endif // SLIT

      }

#ifdef    KDE
      LinkedListIterator<BScreen> it(screenList);
      for (; it.current(); it++)
        it.current()->removeKWMWindow(e->xdestroywindow.window);
#endif // KDE

      break;
    }
    
  case MotionNotify:
    {
      BlackboxWindow *win = (BlackboxWindow *) 0;
      Basemenu *menu = (Basemenu *) 0;
      
      if ((win = searchWindow(e->xmotion.window)))
	win->motionNotifyEvent(&e->xmotion);
      else if ((menu = searchMenu(e->xmotion.window)))
	menu->motionNotifyEvent(&e->xmotion);
      
      break;
    }
    
  case PropertyNotify:
    {
      if (e->xproperty.state != PropertyDelete) {
	BlackboxWindow *win = searchWindow(e->xproperty.window);
	
	if (win)
	  win->propertyNotifyEvent(e->xproperty.atom);

#ifdef    KDE
	else {
          if (e->xproperty.atom == kwm_current_desktop ||
              e->xproperty.atom == kwm_number_of_desktops) {
            BScreen *screen = searchScreen(e->xproperty.window);
	  
	    Atom ajunk;
	    int ijunk;
	    unsigned int ujunk;
	    unsigned long *data = (unsigned long *) 0, uljunk;
	  
	    grab();

	    if ((! screen) && (validateWindow(e->xproperty.window))) {
	      Window root = None;
	    
	      if (XGetGeometry(getDisplay(), (Window) e->xproperty.window, &root,
			     &ijunk, &ijunk, &ujunk, &ujunk, &ujunk, &ujunk))
	        screen = searchScreen(root);
            }
	  
            if (screen) {
	      if (XGetWindowProperty(getDisplay(), screen->getRootWindow(),
				     e->xproperty.atom, 0l, 1l, False,
				     e->xproperty.atom, &ajunk, &ijunk, &uljunk,
				     &uljunk,
				     (unsigned char **) &data) == Success) {
	        if (e->xproperty.atom == kwm_current_desktop) {
		  if (screen->getCurrentWorkspace()->getWorkspaceID() !=
		      (((int) *data) - 1))
		    screen->changeWorkspaceID(((int) *data) - 1);
	        } else if (e->xproperty.atom == kwm_number_of_desktops) {
                  if (screen->getCount() != ((int) *data))
		    if (screen->getCount() < (((int) *data) - 1)) {
		      while (screen->getCount() < (((int) *data) - 1))
		        screen->addWorkspace();
                    } else if (screen->getCount() > (((int) *data) - 1)) {
		        while (screen->getCount() > (((int) *data) - 1))
		          screen->removeLastWorkspace();
                    }
                }

                XFree((char *) data);
	      }
            }

            ungrab();
          } else {
            grab();

            LinkedListIterator<BScreen> it(screenList);
            for (; it.current(); it++)
              it.current()->scanWorkspaceNames();

            ungrab();
	  }
	}

#endif // KDE

      }
      
      break;
    }
    
  case EnterNotify:
    {
      BScreen *screen = (BScreen *) 0;
      BlackboxWindow *win = (BlackboxWindow *) 0;
      Basemenu *menu = (Basemenu *) 0;

      if (e->xcrossing.mode == NotifyGrab) break;

      XEvent dummy;
      scanargs sa;
      sa.w = e->xcrossing.window;
      sa.enter = sa.leave = False;
      (void) XCheckIfEvent(getDisplay(), &dummy, queueScanner, (char *) &sa);

      if ((e->xcrossing.window == e->xcrossing.root) &&
          (screen = searchScreen(e->xcrossing.window))) {
        screen->getImageControl()->installRootColormap();
      } else if ((win = searchWindow(e->xcrossing.window))) {
        if (win->getScreen()->isSloppyFocus() && (! win->isFocused()) &&
            (! no_focus)) {
          grab();

          if (((! sa.leave) || sa.inferior) && win->isVisible() &&
              win->setInputFocus())
            if (resource.colormap_focus_follows_mouse)  
              win->installColormap(True);

          ungrab();
        }
      } else if ((menu = searchMenu(e->xcrossing.window)))
	menu->enterNotifyEvent(&e->xcrossing);
      
      break;
    }
    
  case LeaveNotify:
    {
      BlackboxWindow *win = (BlackboxWindow *) 0;
      Basemenu *menu = (Basemenu *) 0;
      
      if ((menu = searchMenu(e->xcrossing.window)))
	menu->leaveNotifyEvent(&e->xcrossing);
      else if (resource.colormap_focus_follows_mouse &&
               (win = searchWindow(e->xcrossing.window)))
        win->installColormap(False);

      break;
    }
    
  case Expose:
    {
      BlackboxWindow *win = (BlackboxWindow *) 0;
      Basemenu *menu = (Basemenu *) 0;
      Toolbar *tbar = (Toolbar *) 0;
      
      if ((win = searchWindow(e->xexpose.window)))
	win->exposeEvent(&e->xexpose);
      else if ((menu = searchMenu(e->xexpose.window)))
	menu->exposeEvent(&e->xexpose);
      else if ((tbar = searchToolbar(e->xexpose.window)))
	tbar->exposeEvent(&e->xexpose);
      
      break;
    } 

  case KeyPress:
    {
      BScreen *screen = (BScreen *) 0;
      Toolbar *tbar = (Toolbar *) 0;

      if ((tbar = searchToolbar(e->xkey.window)) && tbar->isEditing()) {
        tbar->keyPressEvent(&e->xkey);
      } else if ((screen = searchScreen(e->xkey.root))) {
	if (e->xkey.state == resource.wkspc_change_mask) {
	  if (XKeycodeToKeysym(getDisplay(), e->xkey.keycode, 0) == XK_Left){
	    if (screen->getCurrentWorkspaceID() > 0)
	      screen->changeWorkspaceID(screen->getCurrentWorkspaceID() - 1);
	    else
	      screen->changeWorkspaceID(screen->getCount() - 1);
	  } else if (XKeycodeToKeysym(getDisplay(), e->xkey.keycode, 0) ==
		     XK_Right) {
	    if (screen->getCurrentWorkspaceID() != screen->getCount() - 1)
	      screen->changeWorkspaceID(screen->getCurrentWorkspaceID() + 1);
	    else
	      screen->changeWorkspaceID(0);
	  }
	} else if (e->xkey.state == resource.cycle_mask) {
          if (XKeycodeToKeysym(getDisplay(), e->xkey.keycode, 0) == XK_Tab) {
            screen->nextFocus();
          }
        } else if (e->xkey.state & (resource.cycle_mask | ShiftMask)) {
          if (XKeycodeToKeysym(getDisplay(), e->xkey.keycode, 0) == XK_Tab) {
              screen->prevFocus();
          }
	}
      }
 
      break;
    }
    
  case ColormapNotify:
    {
      BScreen *screen = searchScreen(e->xcolormap.window);
      
      if (screen)
	screen->setRootColormapInstalled((e->xcolormap.state ==
					  ColormapInstalled) ? True : False);
      
      break;
    }

  case ClientMessage:
    {
      if ((e->xclient.format == 32) &&
	  (e->xclient.message_type == getWMChangeStateAtom()) &&
	  (e->xclient.data.l[0] == IconicState)) {
	BlackboxWindow *win = searchWindow(e->xclient.window);
	
	if (win)
	  win->iconify();
      }

#ifdef    KDE
      else {
	BScreen *screen = (BScreen *) 0;

	Window root_search = None;
	
	int ijunk;
	unsigned int ujunk;
        unsigned long data = e->xclient.data.l[0];

	grab();

	if (validateWindow(e->xclient.window)) {
	  if (XGetGeometry(getDisplay(), (Window) e->xclient.window,
                           &root_search, &ijunk, &ijunk, &ujunk, &ujunk,
                           &ujunk, &ujunk))
	    screen = searchScreen(root_search);

	  if (screen) {
	    if (e->xclient.message_type == kwm_current_desktop) {
	      grab();

	      screen->changeWorkspaceID((int) data);

	      LinkedListIterator<BScreen> it(screenList);
	      for (; it.current(); it++)
		screen->scanWorkspaceNames();

	      ungrab();
	    } else if (e->xclient.message_type == kwm_module) {
	      screen->addKWMModule((Window) data);
	    } else if (e->xclient.message_type == kwm_activate_window) {
	      BlackboxWindow *win = searchWindow((Window) data);

	      if (win)
		win->setInputFocus();
	    } else {
              screen->sendToKWMModules(&e->xclient);
            }
	  }
	}
	
	ungrab();
      }
#endif // KDE
      
      break;
    }
    
    
  default:
    {

#ifdef    SHAPE
      if (e->type == getShapeEventBase()) {
	XShapeEvent *shape_event = (XShapeEvent *) e;	
	BlackboxWindow *win = (BlackboxWindow *) 0;

	if ((win = searchWindow(e->xany.window)) ||
	    (shape_event->kind != ShapeBounding))
	  win->shapeEvent(shape_event);
      }
#endif // SHAPE

    }
  }
}


BScreen *Blackbox::searchScreen(Window window) {
  if (validateWindow(window)) {
    BScreen *screen = (BScreen *) 0;
    LinkedListIterator<BScreen> it(screenList);

    for (; it.current(); it++) {
      if (it.current())
	if (it.current()->getRootWindow() == window) {
	  screen = it.current();
	  return screen;
	}
    }
  }

  return (BScreen *) 0;
}


BlackboxWindow *Blackbox::searchWindow(Window window) {
  if (validateWindow(window)) {
    BlackboxWindow *win = (BlackboxWindow *) 0;
    LinkedListIterator<WindowSearch> it(windowSearchList);
    
    for (; it.current(); it++) {
      WindowSearch *tmp = it.current();
      if (tmp)
	if (tmp->window == window) {
	  win = tmp->data;
	  return win;
	}
    }
  }
  
  return (BlackboxWindow *) 0;
}


BlackboxWindow *Blackbox::searchGroup(Window window, BlackboxWindow *win) {
  if (validateWindow(window)) {
    BlackboxWindow *w = (BlackboxWindow *) 0;
    LinkedListIterator<GroupSearch> it(groupSearchList);
    
    for (; it.current(); it++) {
      GroupSearch *tmp = it.current();
      if (tmp)
	if (tmp->window == window) {
	  w = tmp->data;
	  if (w->getClientWindow() != win->getClientWindow())
	    return win;
	}
    }
  }
  
  return (BlackboxWindow *) 0;
}


Basemenu *Blackbox::searchMenu(Window window) {
  if (validateWindow(window)) {
    Basemenu *menu = (Basemenu *) 0;
    LinkedListIterator<MenuSearch> it(menuSearchList);
    
    for (; it.current(); it++) {
      MenuSearch *tmp = it.current();
      
      if (tmp)
	if (tmp->window == window) {
	  menu = tmp->data;
	  return menu;
	}
    }
  }

  return (Basemenu *) 0;
}


Toolbar *Blackbox::searchToolbar(Window window) {
  if (validateWindow(window)) {
    Toolbar *tbar = (Toolbar *) 0;
    LinkedListIterator<ToolbarSearch> it(toolbarSearchList);
    
    for (; it.current(); it++) {
      ToolbarSearch *tmp = it.current();
      
      if (tmp)
	if (tmp->window == window) {
	  tbar = tmp->data;
	  return tbar;
	}
    }
  }
  
  return (Toolbar *) 0;
}


#ifdef    SLIT
Slit *Blackbox::searchSlit(Window window) {
  if (validateWindow(window)) {
    Slit *s = (Slit *) 0;
    LinkedListIterator<SlitSearch> it(slitSearchList);

    for (; it.current(); it++) {
      SlitSearch *tmp = it.current();

      if (tmp)
	if (tmp->window == window) {
	  s = tmp->data;
	  return s;
	}
    }
  }

  return (Slit *) 0;
}
#endif // SLIT


void Blackbox::saveWindowSearch(Window window, BlackboxWindow *data) {
  WindowSearch *tmp = new WindowSearch;
  tmp->window = window;
  tmp->data = data;
  windowSearchList->insert(tmp);
}


void Blackbox::saveGroupSearch(Window window, BlackboxWindow *data) {
  GroupSearch *tmp = new GroupSearch;
  tmp->window = window;
  tmp->data = data;
  groupSearchList->insert(tmp);
}


void Blackbox::saveMenuSearch(Window window, Basemenu *data) {
  MenuSearch *tmp = new MenuSearch;
  tmp->window = window;
  tmp->data = data;
  menuSearchList->insert(tmp);
}


void Blackbox::saveToolbarSearch(Window window, Toolbar *data) {
  ToolbarSearch *tmp = new ToolbarSearch;
  tmp->window = window;
  tmp->data = data;
  toolbarSearchList->insert(tmp);
}


#ifdef    SLIT
void Blackbox::saveSlitSearch(Window window, Slit *data) {
  SlitSearch *tmp = new SlitSearch;
  tmp->window = window;
  tmp->data = data;
  slitSearchList->insert(tmp);
}
#endif // SLIT


void Blackbox::removeWindowSearch(Window window) {
  LinkedListIterator<WindowSearch> it(windowSearchList);
  for (; it.current(); it++) {
    WindowSearch *tmp = it.current();

    if (tmp)
      if (tmp->window == window) {
	windowSearchList->remove(tmp);
	delete tmp;
	break;
      }
  }
}


void Blackbox::removeGroupSearch(Window window) {
  LinkedListIterator<GroupSearch> it(groupSearchList);
  for (; it.current(); it++) {
    GroupSearch *tmp = it.current();
    
    if (tmp)
      if (tmp->window == window) {
	groupSearchList->remove(tmp);
	delete tmp;
	break;
      }
  }
}


void Blackbox::removeMenuSearch(Window window) {
  LinkedListIterator<MenuSearch> it(menuSearchList);
  for (; it.current(); it++) {
    MenuSearch *tmp = it.current();

    if (tmp)
      if (tmp->window == window) {
	menuSearchList->remove(tmp);
	delete tmp;
	break;
      }
  }
}


void Blackbox::removeToolbarSearch(Window window) {
  LinkedListIterator<ToolbarSearch> it(toolbarSearchList);
  for (; it.current(); it++) {
    ToolbarSearch *tmp = it.current();

    if (tmp)
      if (tmp->window == window) {
	toolbarSearchList->remove(tmp);
	delete tmp;
	break;
      }
  }
}


#ifdef    SLIT
void Blackbox::removeSlitSearch(Window window) {
  LinkedListIterator<SlitSearch> it(slitSearchList);
  for (; it.current(); it++) {
    SlitSearch *tmp = it.current();
    
    if (tmp)
      if (tmp->window == window) {
	slitSearchList->remove(tmp);
	delete tmp;
	break;
      }
  }
}
#endif // SLIT


void Blackbox::restart(char *prog) {
  shutdown();
  save_rc();
  
  if (prog) {
    execlp(prog, prog, NULL);
    perror(prog);
  }
  
  // fall back in case the above execlp doesn't work
  execvp(argv[0], argv);
  execvp(basename(argv[0]), argv);
  
  // fall back in case we can't re execvp() ourself
  shutdown();
}


void Blackbox::shutdown(void) {
  BaseDisplay::shutdown();

  XSetInputFocus(getDisplay(), PointerRoot, None, CurrentTime);
  
  LinkedListIterator<BScreen> it(screenList);
  for (; it.current(); it++)
    it.current()->shutdown();

  save_rc();
}


void Blackbox::save_rc(void) {
  XrmDatabase new_blackboxrc = (XrmDatabase) 0;
  char rc_string[1024], style[MAXPATHLEN + 64];

  char *dbfile = (char *) 0;

  if (rc_file) {
    int rc_len = strlen(rc_file) + 1;
    dbfile = new char[rc_len];             
    strncpy(dbfile, rc_file, rc_len);
  } else {
    char *homedir = getenv("HOME");
    dbfile = new char[strlen(homedir) + 32];               
    sprintf(dbfile, "%s/.blackboxrc", homedir);                              
  }

  sprintf(style, "session.styleFile: %s", resource.style_file);
  XrmPutLineResource(&new_blackboxrc, style);

  load_rc();

  sprintf(rc_string, "session.opaqueMove: %s",
          ((resource.opaque_move) ? "True" : "False"));
  XrmPutLineResource(&new_blackboxrc, rc_string);

  sprintf(rc_string, "session.imageDither:  %s",
          ((resource.image_dither) ? "True" : "False"));
  XrmPutLineResource(&new_blackboxrc, rc_string);

  sprintf(rc_string, "session.menuFile:  %s", resource.menu_file);
  XrmPutLineResource(&new_blackboxrc, rc_string);

  sprintf(rc_string, "session.colorsPerChannel:  %d",
          resource.colors_per_channel);
  XrmPutLineResource(&new_blackboxrc, rc_string);

  sprintf(rc_string, "session.doubleClickInterval:  %lu",
          resource.double_click_interval);
  XrmPutLineResource(&new_blackboxrc, rc_string);

  sprintf(rc_string, "session.colormapFocusModel:  %s",
          (resource.colormap_focus_follows_mouse) ? "FollowsMouse" : "Click");
  XrmPutLineResource(&new_blackboxrc, rc_string);

  sprintf(rc_string, "session.autoRaiseDelay:  %lu",
          ((resource.auto_raise_delay_sec * 1000) +
           (resource.auto_raise_delay_usec / 1000)));
  XrmPutLineResource(&new_blackboxrc, rc_string);

  // write out our key modifiers
  char modifier[1024], *mptr = modifier;
  
  *mptr = '\0'; 
  if (resource.wkspc_change_mask & ShiftMask) {
    strcpy(mptr, " Shift");
    mptr += 6;
  }
 
  if (resource.wkspc_change_mask & LockMask) {
    strcpy(mptr, " Lock");
    mptr += 5;
  }
  if (resource.wkspc_change_mask & ControlMask) {
    strcpy(mptr, " Control");
    mptr += 8;
  }
  if (resource.wkspc_change_mask & Mod1Mask) {
    strcpy(mptr, " Mod1");
    mptr += 5;
  }
  if (resource.wkspc_change_mask & Mod2Mask) {
    strcpy(mptr, " Mod2");
    mptr += 5;
  }
  if (resource.wkspc_change_mask & Mod3Mask) {
    strcpy(mptr, " Mod3");
    mptr += 5;
  }
  if (resource.wkspc_change_mask & Mod4Mask) {
    strcpy(mptr, " Mod4");
    mptr += 5;
  }
  if (resource.wkspc_change_mask & Mod5Mask) {
    strcpy(mptr, " Mod5");
    mptr += 5;
  }
  
  sprintf(rc_string, "session.workspaceChangeModifier: %s", modifier);
  XrmPutLineResource(&new_blackboxrc, rc_string);

  mptr = modifier;
  *mptr = '\0';
  if (resource.cycle_mask & ShiftMask) {
    strcpy(mptr, " Shift");
    mptr += 6;
  }
  if (resource.cycle_mask & LockMask) {
    strcpy(mptr, " Lock");
    mptr += 5;
  }
  if (resource.cycle_mask & ControlMask) {
    strcpy(mptr, " Control");
    mptr += 8;
  }
  if (resource.cycle_mask & Mod1Mask) {
    strcpy(mptr, " Mod1");
    mptr += 5;
  }
  if (resource.cycle_mask & Mod2Mask) {
    strcpy(mptr, " Mod2");
    mptr += 5;
  }
  if (resource.cycle_mask & Mod3Mask) {
    strcpy(mptr, " Mod3");
    mptr += 5;
  }
  if (resource.cycle_mask & Mod4Mask) {
    strcpy(mptr, " Mod4");
    mptr += 5;
  }
  if (resource.cycle_mask & Mod5Mask) {
    strcpy(mptr, " Mod5");
    mptr += 5;
  }
  
  sprintf(rc_string, "session.windowCycleModifier: %s", modifier);
  XrmPutLineResource(&new_blackboxrc, rc_string);
  
  LinkedListIterator<BScreen> it(screenList);
  for (; it.current(); it++) {
    BScreen *screen = it.current();
    int screen_number = screen->getScreenNumber();
    
#ifdef    SLIT
    char *slit_placement = (char *) 0;

    switch (screen->getSlitPlacement()) {
    case BScreen::TopLeft:
      slit_placement = "TopLeft";
      break;

    case BScreen::CenterLeft:
      slit_placement = "CenterLeft";
      break;

    case BScreen::BottomLeft:
      slit_placement = "BottomLeft";
      break;

    case BScreen::TopRight:
      slit_placement = "TopRight";
      break;

    case BScreen::BottomRight:
      slit_placement = "BottomRight";
      break;
      
    case BScreen::CenterRight:
    default:
      slit_placement = "CenterRight";
      break;
    }
    
    sprintf(rc_string, "session.screen%d.slitPlacement: %s", screen_number,
	    slit_placement);
    XrmPutLineResource(&new_blackboxrc, rc_string);
#endif // SLIT

    load_rc(screen);

    sprintf(rc_string, "session.screen%d.windowPlacement:  %s", screen_number,
	    ((screen->getPlacementPolicy() == BScreen::SmartPlacement) ?
	     "SmartPlacement" : "CascadePlacement"));
    XrmPutLineResource(&new_blackboxrc, rc_string);

    sprintf(rc_string, "session.screen%d.workspaces:  %d", screen_number,
	    screen->getCount());
    XrmPutLineResource(&new_blackboxrc, rc_string);
    
    sprintf(rc_string, "session.screen%d.toolbarWidthPercent:  %d",
	    screen_number, screen->getToolbarWidthPercent());
    XrmPutLineResource(&new_blackboxrc, rc_string);
    
    sprintf(rc_string, "session.screen%d.toolbarOnTop:  %s", screen_number,
	    ((screen->getToolbar()->isOnTop()) ? "True" : "False"));
    XrmPutLineResource(&new_blackboxrc, rc_string);
    
    // these are static, but may not be saved in the users .blackboxrc,
    // writing these resources will allow the user to edit them at a later
    // time... but loading the defaults before saving allows us to rewrite the
    // users changes...
    sprintf(rc_string, "session.screen%d.focusModel:  %s", screen_number,
	    ((screen->isSloppyFocus()) ?
	     ((screen->doAutoRaise()) ? "AutoRaiseSloppyFocus" :
	      "SloppyFocus") :
	     "ClickToFocus"));
    XrmPutLineResource(&new_blackboxrc, rc_string);
    
#ifdef HAVE_STRFTIME
    sprintf(rc_string, "session.screen%d.strftimeFormat: %s", screen_number,
	    screen->getStrftimeFormat());
    XrmPutLineResource(&new_blackboxrc, rc_string);
#else
    sprintf(rc_string, "session.screen%d.dateFormat:  %s", screen_number,
	    ((screen->getDateFormat() == B_EuropeanDate) ?
	     "European" : "American"));
    XrmPutLineResource(&new_blackboxrc, rc_string);
    
    sprintf(rc_string, "session.screen%d.clockFormat:  %d", screen_number,
	    ((screen->isClock24hour()) ? 24 : 12));
    XrmPutLineResource(&new_blackboxrc, rc_string);
#endif
    
    // write out the users workspace names
    int i, len = 0;
    for (i = 0; i < screen->getCount(); i++)
      len += strlen((screen->getWorkspace(i)->getName()) ?
		    screen->getWorkspace(i)->getName() : "Null") + 1;
    
    char *resource_string = new char[len + 1024],
      *save_string = new char[len], *save_string_pos = save_string,
      *name_string_pos;
    if (save_string) {
      for (i = 0; i < screen->getCount(); i++) {
      len = strlen((screen->getWorkspace(i)->getName()) ?
                   screen->getWorkspace(i)->getName() : "Null") + 1;
      name_string_pos =
	(char *) ((screen->getWorkspace(i)->getName()) ?
		  screen->getWorkspace(i)->getName() : "Null");
      
      while (--len) *(save_string_pos++) = *(name_string_pos++);
      *(save_string_pos++) = ',';
      }
    }
    
    *(--save_string_pos) = '\0';
  
    sprintf(resource_string, "session.screen%d.workspaceNames:  %s",
	    screen_number, save_string);
    XrmPutLineResource(&new_blackboxrc, resource_string);

    delete [] resource_string;
    delete [] save_string;
  }
  
  XrmDatabase old_blackboxrc = XrmGetFileDatabase(dbfile);

  XrmMergeDatabases(new_blackboxrc, &old_blackboxrc);
  XrmPutFileDatabase(old_blackboxrc, dbfile);
  XrmDestroyDatabase(old_blackboxrc);
  
  delete [] dbfile;
}


void Blackbox::load_rc(void) {
  XrmDatabase database = (XrmDatabase) 0;

  char *dbfile = (char *) 0;

  if (rc_file) {
    int rc_len = strlen(rc_file) + 1;
    dbfile = new char[rc_len];
    strncpy(dbfile, rc_file, rc_len);
  } else {
    char *homedir = getenv("HOME");
    dbfile = new char[strlen(homedir) + 32];
    sprintf(dbfile, "%s/.blackboxrc", homedir);
  }

  database = XrmGetFileDatabase(dbfile);

  delete [] dbfile;

  XrmValue value;
  char *value_type;

  if (resource.menu_file) delete [] resource.menu_file;
  if (XrmGetResource(database, "session.menuFile", "Session.MenuFile",
		     &value_type, &value)) {
    int len = strlen(value.addr);
    resource.menu_file = new char[len + 1];
    sprintf(resource.menu_file, "%s", value.addr);
  } else {
    int len = strlen(DEFAULTMENU);
    resource.menu_file = new char[len + 1];
    sprintf(resource.menu_file, "%s", DEFAULTMENU);
  }
  
  if (XrmGetResource(database, "session.imageDither", "Session.ImageDither",
		     &value_type, &value)) {
    if (! strncasecmp("true", value.addr, value.size))
      resource.image_dither = True;
    else
      resource.image_dither = False;
  } else
    resource.image_dither = True;
  
  if (XrmGetResource(database, "session.colorsPerChannel",
		     "Session.ColorsPerChannel", &value_type, &value)) {
    if (sscanf(value.addr, "%d", &resource.colors_per_channel) != 1) {
      resource.colors_per_channel = 4;
    } else {
      if (resource.colors_per_channel < 2) resource.colors_per_channel = 2;
      if (resource.colors_per_channel > 6) resource.colors_per_channel = 6;
    }
  } else
    resource.colors_per_channel = 4;

  if (resource.style_file) delete [] resource.style_file;
  if (XrmGetResource(database, "session.styleFile", "Session.StyleFile",
		     &value_type, &value)) {
    int len = strlen(value.addr);
    resource.style_file = new char[len + 1];
    sprintf(resource.style_file, "%s", value.addr);
  } else {
    int len = strlen(DEFAULTSTYLE);
    resource.style_file = new char[len + 1];
    sprintf(resource.style_file, "%s", DEFAULTSTYLE);
  }

  if (XrmGetResource(database, "session.doubleClickInterval",
		     "Session.DoubleClickInterval", &value_type, &value)) {
    if (sscanf(value.addr, "%lu", &resource.double_click_interval) != 1)
      resource.double_click_interval = 250;
  } else
    resource.double_click_interval = 250;

  if (XrmGetResource(database, "session.autoRaiseDelay",
                     "Session.AutoRaiseDelay", &value_type, &value)) {
    if (sscanf(value.addr, "%lu", &resource.auto_raise_delay_usec) != 1)
      resource.auto_raise_delay_usec = 250;
  } else
    resource.auto_raise_delay_usec = 250;

  resource.auto_raise_delay_sec = resource.auto_raise_delay_usec / 1000;
  resource.auto_raise_delay_usec -= (resource.auto_raise_delay_sec * 1000);
  resource.auto_raise_delay_usec *= 1000;

  if (XrmGetResource(database, "session.colormapFocusModel",
                     "Session.ColormapFocusModel", &value_type, &value)) {
    if (! strncasecmp("followsmouse", value.addr, value.size))
      resource.colormap_focus_follows_mouse = True;
    else
      resource.colormap_focus_follows_mouse = False;
  } else
    resource.colormap_focus_follows_mouse = False;
  
  resource.wkspc_change_mask = 0;
  if (XrmGetResource(database, "session.workspaceChangeModifier",
		     "Session.WorkspaceChangeModifier", &value_type, &value)) {
    if (strstr(value.addr, "shift") || strstr(value.addr, "Shift"))
      resource.wkspc_change_mask |= ShiftMask;
    
    if (strstr(value.addr, "lock") || strstr(value.addr, "Lock"))
      resource.wkspc_change_mask |= LockMask;
    
    if (strstr(value.addr, "control") || strstr(value.addr, "Control"))
      resource.wkspc_change_mask |= ControlMask;
    
    if (strstr(value.addr, "alt") || strstr(value.addr, "Alt") ||
	strstr(value.addr, "mod1") || strstr(value.addr, "Mod1"))
      resource.wkspc_change_mask |= Mod1Mask;
    
    if (strstr(value.addr, "mod2") || strstr(value.addr, "Mod2"))
      resource.wkspc_change_mask |= Mod2Mask;
    
    if (strstr(value.addr, "mod3") || strstr(value.addr, "Mod3"))
      resource.wkspc_change_mask |= Mod3Mask;
      
    if (strstr(value.addr, "mod4") || strstr(value.addr, "Mod4"))
      resource.wkspc_change_mask |= Mod4Mask;
    
    if (strstr(value.addr, "mod5") || strstr(value.addr, "Mod5"))
      resource.wkspc_change_mask |= Mod5Mask;
  } else
    resource.wkspc_change_mask = ControlMask;
  
  resource.cycle_mask = 0;
  if (XrmGetResource(database, "session.windowCycleModifier",
		     "Session.WindowCycleModifier", &value_type, &value)) {
    if (strstr(value.addr, "shift") || strstr(value.addr, "Shift"))
      resource.cycle_mask |= ShiftMask;
    
    if (strstr(value.addr, "lock") || strstr(value.addr, "Lock"))
      resource.cycle_mask |= LockMask;
    
    if (strstr(value.addr, "control") || strstr(value.addr, "Control"))
      resource.cycle_mask |= ControlMask;
    
    if (strstr(value.addr, "alt") || strstr(value.addr, "Alt") ||
	strstr(value.addr, "mod1") || strstr(value.addr, "Mod1"))
      resource.cycle_mask |= Mod1Mask;
    
    if (strstr(value.addr, "mod2") || strstr(value.addr, "Mod2"))
      resource.cycle_mask |= Mod2Mask;
    
    if (strstr(value.addr, "mod3") || strstr(value.addr, "Mod3"))
      resource.cycle_mask |= Mod3Mask;
    
    if (strstr(value.addr, "mod4") || strstr(value.addr, "Mod4"))
      resource.cycle_mask |= Mod4Mask;
    
    if (strstr(value.addr, "mod5") || strstr(value.addr, "Mod5"))
      resource.cycle_mask |= Mod5Mask;
  } else
    resource.cycle_mask = Mod1Mask;

  if (XrmGetResource(database, "session.opaqueMove", "Session.OpaqueMove",
                     &value_type, &value)) { 
    if (! strncasecmp("true", value.addr, value.size))
      resource.opaque_move = True;
    else
      resource.opaque_move = False;
  } else
    resource.opaque_move = False;
}


void Blackbox::load_rc(BScreen *screen) {  
  XrmDatabase database = (XrmDatabase) 0;

  char *dbfile = (char *) 0;

  if (rc_file) {
    int rc_len = strlen(rc_file) + 1;
    dbfile = new char[rc_len];             
    strncpy(dbfile, rc_file, rc_len);
  } else {
    char *homedir = getenv("HOME");
    dbfile = new char[strlen(homedir) + 32];               
    sprintf(dbfile, "%s/.blackboxrc", homedir);                              
  }

  database = XrmGetFileDatabase(dbfile);
  
  delete [] dbfile;

  XrmValue value;
  char *value_type, name_lookup[1024], class_lookup[1024];
  int screen_number = screen->getScreenNumber();
  
  sprintf(name_lookup,  "session.screen%d.workspaces", screen_number);
  sprintf(class_lookup, "Session.Screen%d.Workspaces", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    int i;
    if (sscanf(value.addr, "%d", &i) != 1) i = 1;
    screen->saveWorkspaces(i);
  } else
    screen->saveWorkspaces(1);

  sprintf(name_lookup,  "session.screen%d.toolbarWidthPercent", screen_number);
  sprintf(class_lookup, "Session.Screen%d.ToolbarWidthPercent", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    int i;
    if (sscanf(value.addr, "%d", &i) != 1) i = 66;

    if (i <= 0 || i > 100)
      i = 66;

    screen->saveToolbarWidthPercent(i);
  } else
    screen->saveToolbarWidthPercent(66);
  
  screen->removeWorkspaceNames();
  
  sprintf(name_lookup,  "session.screen%d.workspaceNames", screen_number);
  sprintf(class_lookup, "Session.Screen%d.WorkspaceNames", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    char *search = new char[value.size];
    sprintf(search, "%s", value.addr);
    
    int i;
    for (i = 0; i < screen->getNumberOfWorkspaces(); i++) {
      char *nn;
      
      if (! i) nn = strtok(search, ",");
      else nn = strtok(NULL, ",");
      
      if (nn) screen->addWorkspaceName(nn);
      else break;
    }

    delete [] search;
  }

  sprintf(name_lookup,  "session.screen%d.toolbarOnTop", screen_number);
  sprintf(class_lookup, "Session.Screen%d.ToolbarOnTop", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    if (! strncasecmp(value.addr, "true", value.size))
      screen->saveToolbarOnTop(True);
    else
      screen->saveToolbarOnTop(False);
  } else
    screen->saveToolbarOnTop(False);

  sprintf(name_lookup,  "session.screen%d.focusModel", screen_number);
  sprintf(class_lookup, "Session.Screen%d.FocusModel", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    if (! strncasecmp(value.addr, "clicktofocus", value.size)) {
      screen->saveAutoRaise(False);
      screen->saveSloppyFocus(False);
    } else if (! strncasecmp(value.addr, "autoraisesloppyfocus", value.size)) {
      screen->saveSloppyFocus(True);
      screen->saveAutoRaise(True);
    } else {
      screen->saveSloppyFocus(True);
      screen->saveAutoRaise(False);
    }
  } else {
    screen->saveSloppyFocus(True);
    screen->saveAutoRaise(False);
  }

  sprintf(name_lookup,  "session.screen%d.windowPlacement", screen_number);
  sprintf(class_lookup, "Session.Screen%d.WindowPlacement", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value))
    if (! strncasecmp(value.addr, "SmartPlacement", value.size))
      screen->savePlacementPolicy(BScreen::SmartPlacement);
    else
      screen->savePlacementPolicy(BScreen::CascadePlacement);
  else
    screen->savePlacementPolicy(BScreen::SmartPlacement);

#ifdef    SLIT
  sprintf(name_lookup, "session.screen%d.slitPlacement", screen_number);
  sprintf(class_lookup, "Session.Screen%d.SlitPlacement", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value))
    if (! strncasecmp(value.addr, "TopLeft", value.size))
      screen->saveSlitPlacement(BScreen::TopLeft);
    else if (! strncasecmp(value.addr, "CenterLeft", value.size))
      screen->saveSlitPlacement(BScreen::CenterLeft);
    else if (! strncasecmp(value.addr, "BottomLeft", value.size))
      screen->saveSlitPlacement(BScreen::BottomLeft);
    else if (! strncasecmp(value.addr, "TopRight", value.size))
      screen->saveSlitPlacement(BScreen::TopRight);
    else if (! strncasecmp(value.addr, "BottomRight", value.size))
      screen->saveSlitPlacement(BScreen::BottomRight);
    else
      screen->saveSlitPlacement(BScreen::CenterRight);
  else
    screen->saveSlitPlacement(BScreen::CenterRight);
#endif // SLIT
  
#ifdef    HAVE_STRFTIME
  char *format = (char *) 0;
  
  sprintf(name_lookup,  "session.screen%d.strftimeFormat", screen_number);
  sprintf(class_lookup, "Session.Screen%d.StrftimeFormat", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    int len = strlen(value.addr);
    format = new char[len + 1];
    sprintf (format, "%s", value.addr);

    screen->saveStrftimeFormat(format);
  } else {
    int len = strlen("%I:%M %p");
    format = new char[len + 1];
    sprintf (format, "%%I:%%M %%p");

    screen->saveStrftimeFormat(format);
  }

  if (format) delete [] format;
#else //  HAVE_STRFTIME
  sprintf(name_lookup,  "session.screen%d.dateFormat", screen_number);
  sprintf(class_lookup, "Session.Screen%d.DateFormat", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    if (strncasecmp(value.addr, "european", value.size))
      screen->saveDateFormat(B_AmericanDate);
    else
      screen->saveDateFormat(B_EuropeanDate);
  } else
    screen->saveDateFormat(B_AmericanDate);
  
  sprintf(name_lookup,  "session.screen%d.clockFormat", screen_number);
  sprintf(class_lookup, "Session.Screen%d.ClockFormat", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    int clock;
    if (sscanf(value.addr, "%d", &clock) != 1) screen->saveClock24Hour(False);
    else if (clock == 24) screen->saveClock24Hour(True);
    else screen->saveClock24Hour(False);
  } else
    screen->saveClock24Hour(False);
#endif // HAVE_STRFTIME

  XrmDestroyDatabase(database);
}


void Blackbox::reconfigure(void) {
  _reconfigure = True;
}


void Blackbox::do_reconfigure(void) {
  grab();

  XrmDatabase new_blackboxrc = (XrmDatabase) 0;
  char style[MAXPATHLEN + 64];

  char *dbfile = (char *) 0;

  if (rc_file) {
    int rc_len = strlen(rc_file) + 1;
    dbfile = new char[rc_len];             
    strncpy(dbfile, rc_file, rc_len);
  } else {
    char *homedir = getenv("HOME");
    dbfile = new char[strlen(homedir) + 32];               
    sprintf(dbfile, "%s/.blackboxrc", homedir);                              
  }

  sprintf(style, "session.styleFile: %s", resource.style_file);
  XrmPutLineResource(&new_blackboxrc, style);
 
  XrmDatabase old_blackboxrc = XrmGetFileDatabase(dbfile);
 
  XrmMergeDatabases(new_blackboxrc, &old_blackboxrc);
  XrmPutFileDatabase(old_blackboxrc, dbfile);
  if (old_blackboxrc) XrmDestroyDatabase(old_blackboxrc);
 
  delete [] dbfile;
  
  for (int i = 0, n = menuTimestamps->count(); i < n; i++) {
    MenuTimestamp *ts = menuTimestamps->remove(0);
  
    if (ts) {
      if (ts->filename) delete [] ts->filename;
      delete ts;
    }
  }

  LinkedListIterator<BScreen> it(screenList);
  for (; it.current(); it++) {
    BScreen *screen = it.current();

    screen->reconfigure();
  }
  
  ungrab();
}


void Blackbox::checkMenu(void) {
  LinkedListIterator<MenuTimestamp> it(menuTimestamps);
  for (; it.current() && (! _reread_menu); it++) {
    struct stat buf;
    
    if (! stat(it.current()->filename, &buf)) {
      if (it.current()->timestamp != buf.st_ctime)
        _reread_menu = True;
    } else
      _reread_menu = True;
  }
}


void Blackbox::rereadMenu(void) {
  for (int i = 0, n = menuTimestamps->count(); i < n; i++) {
    MenuTimestamp *ts = menuTimestamps->remove(0);
    
    if (ts) {
      if (ts->filename) delete [] ts->filename;
      delete ts;
    }
  }
  
  LinkedListIterator<BScreen> it(screenList);
  for (; it.current(); it++)
    it.current()->rereadMenu();
}
 
 
void Blackbox::saveStyleFilename(char *filename) {
  if (resource.style_file) delete [] resource.style_file;

  resource.style_file = new char[strlen(filename) + 1];
  sprintf(resource.style_file, "%s", filename);
}


void Blackbox::saveMenuFilename(char *filename) {
  Bool found = False;

  LinkedListIterator<MenuTimestamp> it(menuTimestamps);
  for (; it.current() && (! found); it++)
    if (! strcmp(it.current()->filename, filename)) found = True;

  if (! found) {
    struct stat buf;

    if (! stat(filename, &buf)) {
      MenuTimestamp *ts = new MenuTimestamp;

      int flen = strlen(filename) + 1;
      ts->filename = new char[flen];
      strncpy(ts->filename, filename, flen);
      ts->timestamp = buf.st_ctime;

      menuTimestamps->insert(ts);
    }
  }
}

