//
// blackbox.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@tcac.net
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#include "blackbox.hh"
#include "Basemenu.hh"
#include "Rootmenu.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"

#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif

#ifdef HAVE_SIGNAL_H
#  include <signal.h>
#endif

#ifdef HAVE_STDIO_H
#  include <stdio.h>
#endif

#ifdef STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <sys/types.h>
#  include <unistd.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 255
#endif

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h>
#endif

#ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else
#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else
#    include <time.h>
#  endif
#endif

#if (defined(HAVE_PROCESS_H) && defined(__EMX__))
#  include <process.h>
#endif

#ifndef HAVE_BASENAME

// *************************************************************************
// this is taken from the GNU liberty codebase
// *************************************************************************

char *basename (const char *name) {
  const char *base = name;

  while (*name) if (*name++ == '/') base = name;

  return (char *) base;
}

#endif


// *************************************************************************
// signal handler to allow for proper and gentle shutdown
// *************************************************************************

Blackbox *blackbox;

static RETSIGTYPE signalhandler(int sig) {
  static int re_enter = 0;
  
  switch (sig) {
  case SIGHUP:
    blackbox->Reconfigure();
    break;

  default:
    fprintf(stderr, "%s: signal %d caught\n", __FILE__, sig);
    if (! re_enter) {
      re_enter = 1;
      fprintf(stderr, "shutting down\n");
      blackbox->Shutdown();
    }

    if (sig != SIGTERM && sig != SIGINT) {
      fprintf(stderr, "aborting... dumping core\n");
      abort();
    } else
      fprintf(stderr, "exiting\n");

    exit(0);
    break;
  }
}


// *************************************************************************
// X error handler to handle any and all X errors while blackbox is running
// *************************************************************************

static int handleXErrors(Display *d, XErrorEvent *e) {
  char errtxt[128];
  XGetErrorText(d, e->error_code, errtxt, 128);
  fprintf(stderr,
          "blackbox:  [ X Error event received. ]\n"
          "  X Error of failed request:  %d %s\n"
          "  Major/minor opcode of failed request:  %d / %d\n"
          "  Resource id in failed request:  0x%lx\n", e->error_code, errtxt,
          e->request_code, e->minor_code, e->resourceid);

  return(False);
}


// *************************************************************************
// Blackbox class code constructor and destructor
// *************************************************************************

Blackbox::Blackbox(int m_argc, char **m_argv, char *dpy_name) {
  // install signal handler for fatal signals
  signal(SIGSEGV, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGFPE, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGTERM, (RETSIGTYPE (*)(int)) signalhandler);
  signal(SIGINT, (RETSIGTYPE (*)(int)) signalhandler);

  // sighup will cause blackbox to reconfigure itself
  signal(SIGHUP, (RETSIGTYPE (*)(int)) signalhandler);

  ::blackbox = this;
  argc = m_argc;
  argv = m_argv;

  shutdown = False;
  startup = True;
  server_grabs = 0;
  resource.menu_file = resource.style_file = (char *) 0;

  focused_window = (BlackboxWindow *) 0;

  if ((display = XOpenDisplay(dpy_name)) == NULL) {
    fprintf(stderr, "Blackbox::Blackbox: connection to X server failed\n");
    exit(2);

    if (fcntl(ConnectionNumber(display), F_SETFD, 1) == -1) {
      fprintf(stderr, "Blackbox::Blackbox: couldn't mark display connection "
              "as close-on-exec\n");
      exit(2);
    }
  }

  number_of_screens = ScreenCount(display);
  display_name = XDisplayName(dpy_name);
  
  // grab the display server... so that when we select the input events we
  // want, we don't loose any events
  grab();

#ifdef SHAPE
  shape.extensions = XShapeQueryExtension(display, &shape.event_basep,
					  &shape.error_basep);
#else
  shape.extensions = False;
#endif

  _XA_WM_COLORMAP_WINDOWS = XInternAtom(display, "WM_COLORMAP_WINDOWS",
					False);
  _XA_WM_PROTOCOLS = XInternAtom(display, "WM_PROTOCOLS", False);
  _XA_WM_STATE = XInternAtom(display, "WM_STATE", False);
  _XA_WM_CHANGE_STATE = XInternAtom(display, "WM_CHANGE_STATE", False);
  _XA_WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
  _XA_WM_TAKE_FOCUS = XInternAtom(display, "WM_TAKE_FOCUS", False);
  _MOTIF_WM_HINTS = XInternAtom(display, "_MOTIF_WM_HINTS", False);
  
  cursor.session = XCreateFontCursor(display, XC_left_ptr);
  cursor.move = XCreateFontCursor(display, XC_fleur);

  windowSearchList = new LinkedList<WindowSearch>;
  menuSearchList = new LinkedList<MenuSearch>;
  toolbarSearchList = new LinkedList<ToolbarSearch>;
  groupSearchList = new LinkedList<GroupSearch>;
  
  XrmInitialize();
  LoadRC();
  
  // if (resource.image_dither && visual->c_class == TrueColor && depth >= 24)
  //   resource.image_dither = False;

  XSetErrorHandler((XErrorHandler) handleXErrors);
  
  screenList = new LinkedList<BScreen>;
  int i;
  for (i = 0; i < number_of_screens; i++) {
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
    
    exit(3);
  }
  
  XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);
  XSynchronize(display, False);
  XSync(display, False);

  ungrab();
}


Blackbox::~Blackbox(void) {
  if (resource.menu_file) delete [] resource.menu_file;
  if (resource.style_file) delete [] resource.style_file;

  delete windowSearchList;
  delete menuSearchList;
  delete toolbarSearchList;
  delete groupSearchList;

  XCloseDisplay(display);
}


// *************************************************************************
// Event handling/dispatching methods
// *************************************************************************

void Blackbox::EventLoop(void) {
  shutdown = False;
  startup = False;
  reconfigure = False;

  int xfd = ConnectionNumber(display);
  time_t lastTime = time(NULL);

  // scale time to the current minute.. since blackbox will redraw the clock
  // every minute on the minute
  lastTime = ((lastTime / 60) * 60);

  while (! shutdown) {
    if (reconfigure) {
      do_reconfigure();
      reconfigure = False;
    } else if (XPending(display)) {
      XEvent e;
      XNextEvent(display, &e);
      ProcessEvent(&e);

      if (time(NULL) - lastTime > 59) {
	LinkedListIterator<BScreen> it(screenList);
	for (; it.current(); it++)
	  it.current()->getToolbar()->checkClock();
	
	lastTime = time(NULL);
      } else if (time(NULL) < lastTime)
        lastTime = time(NULL);
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

  SaveRC();
}


void Blackbox::ProcessEvent(XEvent *e) {
  switch (e->type) {
  case ButtonPress:
    {
      BlackboxWindow *bWin = NULL;
      Basemenu *rMenu = NULL;
      Toolbar *tbar = NULL;
      
      if ((bWin = searchWindow(e->xbutton.window)) != NULL) {
	bWin->buttonPressEvent(&e->xbutton);
	if (e->xbutton.button == 1)
          bWin->installColormap(True);
      } else if ((rMenu = searchMenu(e->xbutton.window)) != NULL) {
	rMenu->buttonPressEvent(&e->xbutton);
      } else if ((tbar = searchToolbar(e->xbutton.window)) != NULL) {
	tbar->buttonPressEvent(&e->xbutton);
      } else {
	LinkedListIterator<BScreen> it(screenList);
	for (; it.current(); it++) {
	  BScreen *screen = it.current();
	  if (e->xbutton.window == screen->getRootWindow()) {
	    if (e->xbutton.button == 3) {
	      int mx = e->xbutton.x_root -
		(screen->getRootmenu()->getWidth() / 2);
	      int my = e->xbutton.y_root -
		(screen->getRootmenu()->getTitleHeight() / 2);
	      
	      if (mx < 0) mx = 0;
	      if (my < 0) my = 0;
	      if (mx + screen->getRootmenu()->getWidth() > screen->getXRes())
		mx = screen->getXRes() -
		  screen->getRootmenu()->getWidth() - 1;
	      if (my + screen->getRootmenu()->getHeight() > screen->getYRes())
		my = screen->getYRes() -
		  screen->getRootmenu()->getHeight() - 1;

	      screen->getRootmenu()->move(mx, my);
	      
	      if (! screen->getRootmenu()->isVisible())
		screen->getRootmenu()->show();
	    } else if (e->xbutton.button == 1 &&
		       (! screen->isRootColormapInstalled()))
	      screen->getImageControl()->installRootColormap();
	  }
	}
      }
      
      break;
    }
    
  case ButtonRelease:
    {
      BlackboxWindow *win = NULL;
      Basemenu *menu = NULL;
      Toolbar *tbar = NULL;
      
      if ((win = searchWindow(e->xbutton.window)) != NULL)
	win->buttonReleaseEvent(&e->xbutton);
      else if ((menu = searchMenu(e->xbutton.window)) != NULL)
	menu->buttonReleaseEvent(&e->xbutton);
      else if ((tbar = searchToolbar(e->xbutton.window)) != NULL)
	tbar->buttonReleaseEvent(&e->xbutton);
      
      break;
    }
    
  case ConfigureRequest:
    {
      BlackboxWindow *cWin = searchWindow(e->xconfigurerequest.window);
      if (cWin != NULL)
	cWin->configureRequestEvent(&e->xconfigurerequest);
      else {
	// configure a window we haven't mapped yet
	XWindowChanges xwc;
	
	xwc.x = e->xconfigurerequest.x;
	xwc.y = e->xconfigurerequest.y;	
	xwc.width = e->xconfigurerequest.width;
	xwc.height = e->xconfigurerequest.height;
	xwc.border_width = 0;
	xwc.sibling = e->xconfigurerequest.above;
	xwc.stack_mode = e->xconfigurerequest.detail;
	
	XConfigureWindow(display, e->xconfigurerequest.window,
			 e->xconfigurerequest.value_mask, &xwc);
      }
      
      break;
    }
    
  case MapRequest:
    {
      BlackboxWindow *win = searchWindow(e->xmaprequest.window);
      
      if (win == NULL && validateWindow(e->xmaprequest.window)) {
	BScreen *screen = searchScreen(e->xmaprequest.parent);
	if (screen)
	  win = new BlackboxWindow(this, screen, e->xmaprequest.window);
      }

      if ((win = searchWindow(e->xmaprequest.window)) != NULL)
	win->mapRequestEvent(&e->xmaprequest);
      
      break;
    }
  
  case MapNotify:
    {
      BlackboxWindow *mWin = searchWindow(e->xmap.window);
      if (mWin != NULL)
	mWin->mapNotifyEvent(&e->xmap);
      
      break;
    }
  
  case UnmapNotify:
    {
      BlackboxWindow *win = searchWindow(e->xunmap.window);

      if (win) {
        if (focused_window == win) {
          XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);

	  BScreen *screen = win->getScreen();
	  screen->getWorkspace(win->getWorkspaceNumber())->setFocusWindow(-1);

          focused_window = (BlackboxWindow *) 0;
        }

	win->unmapNotifyEvent(&e->xunmap);
      } else if (focused_window) {
        if (e->xunmap.window == focused_window->getClientWindow()) {
          XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);

	  BScreen *screen = focused_window->getScreen();
	  screen->getWorkspace(focused_window->getWorkspaceNumber())->
	    setFocusWindow(-1);

          focused_window = (BlackboxWindow *) 0;
        }
      }

      break;
    }
    
  case DestroyNotify:
    {
      BlackboxWindow *win = searchWindow(e->xdestroywindow.window);
      
      if (win) {
        if (focused_window == win) {
	  XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);

	  BScreen *screen = win->getScreen();
	  screen->getWorkspace(win->getWorkspaceNumber())->setFocusWindow(-1);

          focused_window = (BlackboxWindow *) 0;
	}

        win->destroyNotifyEvent(&e->xdestroywindow);
      } else if (focused_window) {
        if (e->xdestroywindow.window == focused_window->getClientWindow()) {
          XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);

	  BScreen *screen = focused_window->getScreen();
	  screen->getWorkspace(focused_window->getWorkspaceNumber())->
	    setFocusWindow(-1);

          focused_window = (BlackboxWindow *) 0;
        }
      }


      break;
    }
    
  case MotionNotify:
    {
      BlackboxWindow *mWin = NULL;
      Basemenu *rMenu = NULL;
      
      if ((mWin = searchWindow(e->xmotion.window)) != NULL)
	mWin->motionNotifyEvent(&e->xmotion);
      else if ((rMenu = searchMenu(e->xmotion.window)) != NULL)
	rMenu->motionNotifyEvent(&e->xmotion);
      
      break;
    }
    
  case PropertyNotify:
    {
      if (e->xproperty.state != PropertyDelete) {
	BlackboxWindow *pWin = searchWindow(e->xproperty.window);
	
	if (pWin != NULL)
	  pWin->propertyNotifyEvent(e->xproperty.atom);
      }
      
      break;
    }
  
  case EnterNotify:
    {
      BlackboxWindow *win = NULL;
      Basemenu *menu = NULL;
      
      if ((win = searchWindow(e->xcrossing.window)) != NULL) {
	if (win->getScreen()->isSloppyFocus()) {
	  grab();
	  
	  if (win->validateClient())
	    if ((! win->isFocused()) && win->isVisible())
	      if (win->setInputFocus() && win->getScreen()->doAutoRaise())
		win->getScreen()->getCurrentWorkspace()->raiseWindow(win);
	  
	  ungrab();
	}
      } else if ((menu = searchMenu(e->xcrossing.window)) != NULL)
	menu->enterNotifyEvent(&e->xcrossing);
      
      break;
    }
    
  case LeaveNotify:
    {
      Basemenu *menu = NULL;
      
      if ((menu = searchMenu(e->xcrossing.window)) != NULL)
	menu->leaveNotifyEvent(&e->xcrossing);
      
      break;
    }
    
  case Expose:
    {
      BlackboxWindow *win = NULL;
      Basemenu *menu = NULL;
      Toolbar *tbar = NULL;
      
      if ((win = searchWindow(e->xexpose.window)) != NULL)
	win->exposeEvent(&e->xexpose);
      else if ((menu = searchMenu(e->xexpose.window)) != NULL)
	menu->exposeEvent(&e->xexpose);
      else if ((tbar = searchToolbar(e->xexpose.window)) != NULL)
	tbar->exposeEvent(&e->xexpose);
      
      break;
    } 
    
  case FocusIn:
    {
      grab();

      BlackboxWindow *win = searchWindow(e->xfocus.window);
      
      if ((e->xfocus.mode != NotifyGrab) &&
	(e->xfocus.mode != NotifyUngrab) &&
	(e->xfocus.mode != NotifyNonlinearVirtual)) {

        if (focused_window && focused_window->validateClient())
	  focused_window->setFocusFlag(False);

	if (win && (! win->isFocused())) {
          if (focused_window)
            if (focused_window->getScreen()->getScreenNumber() !=
                win->getScreen()->getScreenNumber())
              focused_window->getScreen()->
                getWorkspace(focused_window->getWorkspaceNumber())->
		  setFocusWindow(-1);

          win->setFocusFlag(True);
          focused_window = win;

	  if (win->getScreen()->doAutoRaise())
	    win->getScreen()->
	      getWorkspace(focused_window->getWorkspaceNumber())->
	      raiseWindow(focused_window);
        } else {
          XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);
	  focused_window = (BlackboxWindow *) 0;
        }

        if (focused_window)
	  focused_window->getScreen()->
	    getWorkspace(focused_window->getWorkspaceNumber())->
            setFocusWindow(focused_window->getWindowNumber());
	else {
	  LinkedListIterator<BScreen> it(screenList);
	  for (; it.current(); it++)
	    it.current()->getCurrentWorkspace()->setFocusWindow(-1);
	}
      }
      
      ungrab();
      
      break;
    }
    
  case KeyPress:
    {
      BScreen *screen = searchScreen(e->xkey.root);
      Toolbar *tbar = (Toolbar *) 0;

      if (screen)
	if (e->xkey.state & Mod1Mask) {
	  if (XKeycodeToKeysym(display, e->xkey.keycode, 0) == XK_Tab) {
	    screen->nextFocus();
	  }
	} else if (e->xkey.state & ControlMask) {
	  if (XKeycodeToKeysym(display, e->xkey.keycode, 0) == XK_Left){
	    if (screen->getCurrentWorkspaceID() > 1)
	      screen->changeWorkspaceID(screen->getCurrentWorkspaceID() - 1);
	    else
	      screen->changeWorkspaceID(screen->getCount() - 1);
	  } else if (XKeycodeToKeysym(display, e->xkey.keycode, 0) ==
		     XK_Right){
	    if (screen->getCurrentWorkspaceID() != screen->getCount() - 1)
	      screen->changeWorkspaceID(screen->getCurrentWorkspaceID() + 1);
	    else
	      screen->changeWorkspaceID(1);
	  }
	} else if ((tbar = searchToolbar(e->xkey.window))) {
	  tbar->keyPressEvent(&e->xkey);
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
	  (e->xclient.message_type == _XA_WM_CHANGE_STATE) &&
	  (e->xclient.data.l[0] == IconicState)) {
	BlackboxWindow *win = searchWindow(e->xclient.window);
	
	if (win != NULL)
	  win->iconify();
      }
      
      break;
    }
    
    
  default:
    {
#ifdef SHAPE
      if (e->type == shape.event_basep) {
	XShapeEvent *shape_event = (XShapeEvent *) e;
	
	BlackboxWindow *eWin = NULL;
	if (((eWin = searchWindow(e->xany.window)) != NULL) ||
	    (shape_event->kind != ShapeBounding))
	  eWin->shapeEvent(shape_event);
      }
#endif
    }
  }
}


// *************************************************************************
// Linked list lookup/save/remove methods 
// *************************************************************************

Bool Blackbox::validateWindow(Window window) {
  XEvent event;
  if (XCheckTypedWindowEvent(display, window, DestroyNotify, &event)) {
    ProcessEvent(&event);
    return False;
  }

  return True;
}


void Blackbox::grab(void) { 
  if (! server_grabs++)
    XGrabServer(display);
  
  XSync(display, False);
}


void Blackbox::ungrab(void) {
  if (! --server_grabs)
    XUngrabServer(display);
  
  if (server_grabs < 0) server_grabs = 0;
}


BScreen *Blackbox::searchScreen(Window window) {
  if (validateWindow(window)) {
    BScreen *screen;
    LinkedListIterator<BScreen> it(screenList);

    for (; it.current(); it++) {
      if (it.current())
	if (it.current()->getRootWindow() == window) {
	  screen = it.current();
	  return screen;
	}
    }
  }

  return 0;
}


BlackboxWindow *Blackbox::searchWindow(Window window) {
  if (validateWindow(window)) {
    BlackboxWindow *win;
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
  
  return 0;
}


BlackboxWindow *Blackbox::searchGroup(Window window, BlackboxWindow *win) {
  if (validateWindow(window)) {
    BlackboxWindow *w;
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
  
  return 0;
}


Basemenu *Blackbox::searchMenu(Window window) {
  if (validateWindow(window)) {
    Basemenu *menu = NULL;
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

  return 0;
}


Toolbar *Blackbox::searchToolbar(Window window) {
  if (validateWindow(window)) {
    Toolbar *t = NULL;
    LinkedListIterator<ToolbarSearch> it(toolbarSearchList);
    
    for (; it.current(); it++) {
      ToolbarSearch *tmp = it.current();
      
      if (tmp)
	if (tmp->window == window) {
	  t = tmp->data;
	  return t;
	}
    }
  }
  
  return 0;
}


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


// *************************************************************************
// Exit, Shutdown and Restart methods
// *************************************************************************

void Blackbox::Exit(void) {
  XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);

  LinkedListIterator<BScreen> it(screenList);
  for (; it.current(); it++)
    XSelectInput(display, it.current()->getRootWindow(), NoEventMask);
  
  shutdown = True;
}


void Blackbox::Restart(char *prog) {
  Exit();
  SaveRC();
  
  if (prog) {
    execlp(prog, prog, NULL);
    perror(prog);
  }
  
  // fall back in case the above execlp doesn't work
  execvp(argv[0], argv);
  execvp(basename(argv[0]), argv);
  
  // fall back in case we can't re execvp() ourself
  Exit();
}


void Blackbox::Shutdown(void) {
  Exit();
  SaveRC();
}


void Blackbox::SaveRC(void) {
  XrmDatabase new_blackboxrc = 0;
  char rc_string[1024], style[MAXPATHLEN + 64];
  char *homedir = getenv("HOME"), *rcfile = new char[strlen(homedir) + 32];
  sprintf(rcfile, "%s/.blackboxrc", homedir);

  sprintf(style, "session.styleFile: %s", resource.style_file);
  XrmPutLineResource(&new_blackboxrc, style);

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

  LinkedListIterator<BScreen> it(screenList);
  for (; it.current(); it++) {
    BScreen *screen = it.current();
    int screen_number = screen->getScreenNumber();
    
    LoadRC(screen);

    sprintf(rc_string, "session.screen%d.workspaces:  %d", screen_number,
	    screen->getCount() - 1);
    XrmPutLineResource(&new_blackboxrc, rc_string);
    
    sprintf(rc_string, "session.screen%d.toolbarRaised:  %s", screen_number,
	    ((screen->getToolbar()->isRaised()) ? "True" : "False"));
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
    for (i = 1; i < screen->getCount(); i++)
      len += strlen((screen->getWorkspace(i)->getName()) ?
		    screen->getWorkspace(i)->getName() : "Null") + 1;
    
    char *resource_string = new char[len + 1024],
      *save_string = new char[len], *save_string_pos = save_string,
      *name_string_pos;
    if (save_string) {
      for (i = 1; i < screen->getCount(); i++) {
      len = strlen((screen->getWorkspace(i)->getName()) ? : "Null") + 1;
      name_string_pos = ((screen->getWorkspace(i)->getName()) ?
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
  
  XrmDatabase old_blackboxrc = XrmGetFileDatabase(rcfile);

  XrmMergeDatabases(new_blackboxrc, &old_blackboxrc);
  XrmPutFileDatabase(old_blackboxrc, rcfile);
  XrmDestroyDatabase(old_blackboxrc);
  
  delete [] rcfile;
}


// *************************************************************************
// Resource loading
// *************************************************************************

void Blackbox::LoadRC(void) {
  XrmDatabase database = 0;
  char *homedir = getenv("HOME"), *rcfile = new char[strlen(homedir) + 32];
  sprintf(rcfile, "%s/.blackboxrc", homedir);

  if ((database = XrmGetFileDatabase(rcfile)) == NULL)
    database = XrmGetFileDatabase(DEFAULTRC);

  delete [] rcfile;

  XrmValue value;
  char *value_type;

  if (resource.menu_file) delete [] resource.menu_file;
  if (XrmGetResource(database, "session.menuFile", "Session.MenuFile",
		     &value_type, &value)) {
    int len = strlen(value.addr);
    resource.menu_file = new char[len + 1];
    memset(resource.menu_file, 0, len + 1);
    strncpy(resource.menu_file, value.addr, len);
  } else {
    int len = strlen(DEFAULTMENU);
    resource.menu_file = new char[len + 1];
    memset(resource.menu_file, 0, len + 1);
    strncpy(resource.menu_file, DEFAULTMENU, len);
  }
  
  if (XrmGetResource(database, "session.imageDither", "Session.ImageDither",
		     &value_type, &value)) {
    if (! strncasecmp("true", value.addr, value.size))
      resource.image_dither = True;
    else
      resource.image_dither = False;
  } else
    resource.image_dither = True;
  
  if (XrmGetResource(database, "session.colorPerChannel",
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
    memset(resource.style_file, 0, len + 1);
    strncpy(resource.style_file, value.addr, len);
  } else {
    int len = strlen(DEFAULTSTYLE);
    resource.style_file = new char[len + 1];
    memset(resource.style_file, 0, len + 1);
    strncpy(resource.style_file, DEFAULTSTYLE, len);
  }

  if (XrmGetResource(database, "session.doubleClickInterval",
		     "Session.DoubleClickInterval", &value_type, &value)) {
    if (sscanf(value.addr, "%lu", &resource.double_click_interval) != 1)
      resource.double_click_interval = 250;
  } else
    resource.double_click_interval = 250;
}


void Blackbox::LoadRC(BScreen *screen) {  
  XrmDatabase database = 0;
  char *homedir = getenv("HOME"), *rcfile = new char[strlen(homedir) + 32];
  sprintf(rcfile, "%s/.blackboxrc", homedir);

  if ((database = XrmGetFileDatabase(rcfile)) == NULL)
    database = XrmGetFileDatabase(DEFAULTRC);

  delete [] rcfile;

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
  
  screen->removeWorkspaceNames();
  
  sprintf(name_lookup,  "session.screen%d.workspaceNames", screen_number);
  sprintf(class_lookup, "Session.Screen%d.WorkspaceNames", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    char *search = new char[value.size];
    strncpy(search, value.addr, value.size);
    
    int i;
    for (i = 0; i < screen->getNumberOfWorkspaces(); i++) {
      char *nn;
      
      if (! i) nn = strtok(search, ",");
      else nn = strtok(NULL, ",");
      
      if (nn) screen->addWorkspaceName(nn);
      else break;
    }
  }

  sprintf(name_lookup,  "session.screen%d.toolbarRaised", screen_number);
  sprintf(class_lookup, "Session.Screen%d.ToolbarRaised", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    if (! strncasecmp(value.addr, "true", value.size))
      screen->saveToolbarRaised(True);
    else
      screen->saveToolbarRaised(False);
  } else
    screen->saveToolbarRaised(False);

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

#ifdef    HAVE_STRFTIME
  char *format;

  sprintf(name_lookup,  "session.screen%d.strftimeFormat", screen_number);
  sprintf(class_lookup, "Session.Screen%d.StrftimeFormat", screen_number);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
		     &value)) {
    int len = strlen(value.addr);
    format = new char[len + 1];
    memset(format, 0, len + 1);
    strncpy(format, value.addr, len);
    *(format + len + 1) = '\0';

    screen->saveStrftimeFormat(format);
  } else {
    int len = strlen("%I:%M %p");
    format = new char[len + 1];
    memset(format, 0, len + 1);
    strncpy(format, "%I:%M %p", len);
    *(format + len + 1) = '\0';

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


// *************************************************************************
// Resource reconfiguration
// *************************************************************************

void Blackbox::Reconfigure(void) {
  reconfigure = True;
}


void Blackbox::do_reconfigure(void) {
  grab();

  LinkedListIterator<BScreen> it(screenList);
  for (; it.current(); it++) {
    BScreen *screen = it.current();

    screen->reconfigure();
  }
  
  ungrab();
}


void Blackbox::saveStyleFilename(char *filename) {
  if (resource.style_file) delete [] resource.style_file;

  resource.style_file = new char[strlen(filename) + 1];
  sprintf(resource.style_file, "%s", filename);
}
