//
// blackbox.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@arn.net
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
#include "Rootmenu.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "WorkspaceManager.hh"
#include "Application.hh"

#include "../lib/libBoxdefs.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

// *************************************************************************
// signal handler to allow for proper and gentle shutdown
// *************************************************************************

Blackbox *blackbox;

static void signalhandler(int sig) {
  static int re_enter = 0;
  
  switch (sig) {
  case SIGHUP:
    blackbox->Reconfigure();
    break;

  default:
    fprintf(stderr, "%s: toplevel:\n\t[ signal %d caught ]\n", __FILE__, sig);
    if (! re_enter) {
      re_enter = 1;
      fprintf(stderr, "\t[ shutting down ]\n");
      blackbox->Shutdown(False);
    }

    if (sig != SIGTERM && sig != SIGINT) {
      fprintf(stderr, "\t[ aborting... dumping core ]\n");
      abort();
    } else
      fprintf(stderr, "\t[ exiting ]\n");
    exit(0);
    break;
  }
}


// *************************************************************************
// X error handler to detect a running window manager
// *************************************************************************

static int anotherWMRunning(Display *, XErrorEvent *) {
  fprintf(stderr,
	  "blackbox: a fatal error occured while querying the X server.\n"
	  " - There is another window manager already running. -\n");
  exit(1);
  return(-1);
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
          "  Resource id in failed request:  0x%lx\n"
	  "  [ continuing ]\n", e->error_code, errtxt, e->request_code,
	  e->minor_code, e->resourceid);

  return(0);
}


// *************************************************************************
// Blackbox class code constructor and destructor
// *************************************************************************

Blackbox::Blackbox(int argc, char **argv, char *dpy_name) {
  // install signal handler for fatal signals
  signal(SIGSEGV, (void (*)(int)) signalhandler);
  signal(SIGFPE, (void (*)(int)) signalhandler);
  signal(SIGTERM, (void (*)(int)) signalhandler);
  signal(SIGINT, (void (*)(int)) signalhandler);

  // sighup will cause blackbox to reconfigure itself
  signal(SIGHUP, (void (*)(int)) signalhandler);

  ::blackbox = this;
  b_argc = argc;
  b_argv = argv;

  shutdown = False;
  startup = True;
  focus_window_number = -1;

  rootmenu = 0;
  resource.blackboxrc = 0;
  resource.font.menu = resource.font.title = 0;

  if ((display = XOpenDisplay(dpy_name)) == NULL) {
    fprintf(stderr, "%s: connection to X server failed\n", b_argv[0]);
    exit(2);

    if (fcntl(ConnectionNumber(display), F_SETFD, 1) == -1) {
      fprintf(stderr, "%s: mark close-on-exec on display connection failed\n",
              b_argv[0]);
      exit(2);
    }
  }

  screen = DefaultScreen(display);
  root = RootWindow(display, screen);
  v = DefaultVisual(display, screen);
  depth = DefaultDepth(display, screen);
  display_name = XDisplayName(dpy_name);

  int count; 
  XPixmapFormatValues *pmv = XListPixmapFormats(display, &count);

  for (int i = 0; i < count; i++)
    if (pmv[i].depth == depth) {
      bpp = pmv[i].bits_per_pixel;
      break;
  }
  if (bpp == 0) bpp = depth;

  event_mask = ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
    SubstructureRedirectMask | KeyPressMask | ButtonPressMask |
    ButtonReleaseMask;
  
  // grab the display server... so that when we select the input events we
  // want, we don't loose any events
  XGrabServer(display);

  XSetErrorHandler((XErrorHandler) anotherWMRunning);
  XSelectInput(display, root, event_mask);
  XSync(display, False);
  XSetErrorHandler((XErrorHandler) handleXErrors);

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
  _XA_WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
  _XA_WM_TAKE_FOCUS = XInternAtom(display, "WM_TAKE_FOCUS", False);

  _BLACKBOX_MESSAGE = XInternAtom(display, "BLACKBOX_MESSAGE", False);
  _BLACKBOX_CONTROL = XInternAtom(display, "BLACKBOX_CONTROL", False);
  
  cursor.session = XCreateFontCursor(display, XC_left_ptr);
  cursor.move = XCreateFontCursor(display, XC_fleur);
  XDefineCursor(display, root, cursor.session);

  xres = WidthOfScreen(ScreenOfDisplay(display, screen));
  yres = HeightOfScreen(ScreenOfDisplay(display, screen));

  windowSearchList = new LinkedList<WindowSearch>;
  menuSearchList = new LinkedList<MenuSearch>;
  wsManagerSearchList = new LinkedList<WSManagerSearch>;
  groupSearchList = new LinkedList<GroupSearch>;
  appSearchList = new LinkedList<ApplicationSearch>;

  XrmInitialize();
  LoadDefaults();
  InitColor();

  XGCValues gcv;
  gcv.foreground = getColor("white");
  gcv.function = GXxor;
  gcv.line_width = 2;
  gcv.subwindow_mode = IncludeInferiors;
  opGC = XCreateGC(display, root, GCForeground|GCFunction|GCSubwindowMode,
                   &gcv);

  InitMenu();
  wsManager = new WorkspaceManager(this, resource.workspaces);
  wsManager->stackWindows(0, 0);
  rootmenu->Update();

  unsigned long foo = wsManager->windowID();
  XChangeProperty(display, root, _BLACKBOX_CONTROL, _BLACKBOX_CONTROL, 32,
		  PropModeReplace, (unsigned char *) &foo, 1);

  unsigned int nchild;
  Window r, p, *children;
  XQueryTree(display, root, &r, &p, &children, &nchild);

  for (int i = 0; i < (int) nchild; ++i) {
    if (children[i] == None || (! validateWindow(children[i]))) continue;

    XWindowAttributes attrib;
    if (XGetWindowAttributes(display, children[i], &attrib))
      if (! attrib.override_redirect && attrib.map_state != IsUnmapped) {
	Atom atom;
	Bool app = False;
	int foo;
	unsigned long ulfoo, nitems;
	unsigned char *state;

	if (XGetWindowProperty(display, children[i], _BLACKBOX_CONTROL, 0l, 1l,
			       False, _BLACKBOX_CONTROL, &atom, &foo, &nitems,
			       &ulfoo, &state) == Success)
	  if (state && atom == _BLACKBOX_CONTROL) {
	    app = True;
	    (void) new Application(this, children[i]);
	    XFree(state);
	  } else {
	    BlackboxWindow *nWin = new BlackboxWindow(this, children[i]);

	    XMapRequestEvent mre;
	    mre.window = children[i];
	    nWin->mapRequestEvent(&mre);
          }
      }
  }

  //  while (XPending(display)) {
  //    XEvent foo;
  //    XNextEvent(display, &foo);
  //  }
  
  XSynchronize(display, False);
  XSync(display, False);
  XUngrabServer(display);
}


Blackbox::~Blackbox(void) {
  XSelectInput(display, root, NoEventMask);
  XDeleteProperty(display, root, _BLACKBOX_CONTROL);

  delete [] resource.menuFile;
  delete rootmenu;
  delete wsManager;

  delete windowSearchList;
  delete menuSearchList;
  delete wsManagerSearchList;
  delete groupSearchList;
  delete appSearchList;
  
  if (resource.font.title) {
    XFreeFont(display, resource.font.title);
    resource.font.title = 0;
  }

  if (resource.font.menu) {
    XFreeFont(display, resource.font.menu);
    resource.font.menu = 0;
  }

  XFreeGC(display, opGC);
  
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
	wsManager->checkClock();
	lastTime = time(NULL);
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

	wsManager->checkClock();
	lastTime = time(NULL);
      }
    }
  }
}


void Blackbox::ProcessEvent(XEvent *e) {
  switch (e->type) {
  case ButtonPress: {
    BlackboxWindow *bWin = NULL;
    Basemenu *rMenu = NULL;
    WorkspaceManager *wsMan = NULL;
    
    if ((bWin = searchWindow(e->xbutton.window)) != NULL) {
      bWin->buttonPressEvent(&e->xbutton);
    } else if ((rMenu = searchMenu(e->xbutton.window)) != NULL) {
      rMenu->buttonPressEvent(&e->xbutton);
    } else if ((wsMan = searchWSManager(e->xbutton.window)) != NULL) {
      wsMan->buttonPressEvent(&e->xbutton);
    } else if (e->xbutton.window == root && e->xbutton.button == 3) {
      int mx = e->xbutton.x_root - (rootmenu->Width() / 2),
	my = e->xbutton.y_root - (rootmenu->titleHeight() / 2);

      if (mx < 0) mx = 0;
      if (my < 0) my = 0;
      if (mx + rootmenu->Width() > xres) mx = xres - rootmenu->Width() - 1;
      if (my + rootmenu->Height() > yres) my = yres - rootmenu->Height() - 1;
      rootmenu->Move(mx, my);
      
      if (! rootmenu->Visible())
	rootmenu->Show();
    }
    
    break;
  }
  
  case ButtonRelease: {
    BlackboxWindow *bWin = NULL;
    Basemenu *rMenu = NULL;
    WorkspaceManager *wsMan = NULL;

    if ((bWin = searchWindow(e->xbutton.window)) != NULL)
      bWin->buttonReleaseEvent(&e->xbutton);
    else if ((rMenu = searchMenu(e->xbutton.window)) != NULL)
      rMenu->buttonReleaseEvent(&e->xbutton);
    else if ((wsMan = searchWSManager(e->xbutton.window)) != NULL)
      wsMan->buttonReleaseEvent(&e->xbutton);
    
    break;
  }
  
  case ConfigureRequest: {
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
    
    break; }
  
  case MapRequest: {
    BlackboxWindow *rWin = searchWindow(e->xmaprequest.window);
    
    if (rWin == NULL && validateWindow(e->xmaprequest.window))
      rWin = new BlackboxWindow(this, e->xmaprequest.window);
    
    if ((rWin = searchWindow(e->xmaprequest.window)) != NULL)
	rWin->mapRequestEvent(&e->xmaprequest);

    break; }
  
  case MapNotify: {
    BlackboxWindow *mWin = searchWindow(e->xmap.window);
    if (mWin != NULL)
      mWin->mapNotifyEvent(&e->xmap);
    
    break; }
  
  case UnmapNotify: {
    BlackboxWindow *uWin = searchWindow(e->xunmap.window);
    if (uWin != NULL)
      uWin->unmapNotifyEvent(&e->xunmap);
    
    break; }
  
  case DestroyNotify: {
    BlackboxWindow *dWin = NULL;
    Application *app = NULL;

    if ((dWin = searchWindow(e->xdestroywindow.window)) != NULL)
      dWin->destroyNotifyEvent(&e->xdestroywindow);
    else if ((app = searchApp(e->xdestroywindow.window)) != NULL)
      delete app;
    
    break;
  }
  
  case MotionNotify: {
    BlackboxWindow *mWin = NULL;
    Basemenu *rMenu = NULL;

    if ((mWin = searchWindow(e->xmotion.window)) != NULL)
      mWin->motionNotifyEvent(&e->xmotion);
    else if ((rMenu = searchMenu(e->xmotion.window)) != NULL)
      rMenu->motionNotifyEvent(&e->xmotion);
    
    break;
  }
  
  case PropertyNotify: {
    if (e->xproperty.state != PropertyDelete) {
      BlackboxWindow *pWin = searchWindow(e->xproperty.window);
      
      if (pWin != NULL)
	pWin->propertyNotifyEvent(e->xproperty.atom);
    } else if (e->xproperty.atom == _BLACKBOX_CONTROL) {
      unsigned long foo = wsManager->windowID();
      XChangeProperty(display, root, _BLACKBOX_CONTROL, _BLACKBOX_CONTROL, 32,
		      PropModeReplace, (unsigned char *) &foo, 1);      
    }
    
    break;
  }
  
  case EnterNotify: {
    BlackboxWindow *eWin = NULL;
    Basemenu *eMenu = NULL;
        
    if ((eWin = searchWindow(e->xcrossing.window)) != NULL) {
      XGrabServer(display);
      
      if (validateWindow(eWin->clientWindow()))
	if ((! eWin->isFocused()) && eWin->isVisible())
	  eWin->setInputFocus();
      
      XUngrabServer(display);
    } else if ((eMenu = searchMenu(e->xcrossing.window)) != NULL)
      eMenu->enterNotifyEvent(&e->xcrossing);
    
    break;
  }
  
  case LeaveNotify: {
    Basemenu *lMenu = NULL;

    if ((lMenu = searchMenu(e->xcrossing.window)) != NULL)
      lMenu->leaveNotifyEvent(&e->xcrossing);
    
    break;
  }
  
  case Expose: {
    BlackboxWindow *eWin = NULL;
    Basemenu *eMenu = NULL;
    WorkspaceManager *wsMan = NULL;

    if ((eWin = searchWindow(e->xexpose.window)) != NULL)
      eWin->exposeEvent(&e->xexpose);
    else if ((eMenu = searchMenu(e->xexpose.window)) != NULL)
      eMenu->exposeEvent(&e->xexpose);
    else if ((wsMan = searchWSManager(e->xexpose.window)) != NULL)
      wsMan->exposeEvent(&e->xexpose);
    
    break;
  } 
  
  case FocusIn: {
    BlackboxWindow *iWin = searchWindow(e->xfocus.window);
    
    if ((iWin != NULL) && (e->xfocus.mode != NotifyGrab) &&
	(e->xfocus.mode != NotifyUngrab)) {
      iWin->setFocusFlag(True);
      focus_window_number = iWin->windowNumber();
    }

    break;
  }
  
  case FocusOut: {
    BlackboxWindow *oWin = searchWindow(e->xfocus.window);

    if ((oWin != NULL) && (e->xfocus.mode != NotifyGrab) &&
	(e->xfocus.mode != NotifyWhileGrabbed))
      oWin->setFocusFlag(False);
   
    if ((e->xfocus.mode == NotifyNormal) &&
	(e->xfocus.detail == NotifyAncestor)) {
      XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);
      focus_window_number = -1;
    }

    break;
  }
  
  case KeyPress: {
    if (e->xkey.state & Mod1Mask) {
      if (XKeycodeToKeysym(display, e->xkey.keycode, 0) == XK_Tab) {
	if ((wsManager->currentWorkspace()->Count() > 1) &&
	    (focus_window_number >= 0)) {
	  BlackboxWindow *next, *current =
	    wsManager->currentWorkspace()->window(focus_window_number);
	  
	  int next_window_number, level = 0;
	  do {
	    do {
	      next_window_number =
		((focus_window_number + (++level)) <
		 wsManager->currentWorkspace()->Count())
		? focus_window_number + level : 0;
	      next =
		wsManager->currentWorkspace()->window(next_window_number);
	    } while (next->isIconic());
	  } while ((! next->setInputFocus()) &&
		   (next_window_number != focus_window_number));
	  
	  if (next_window_number != focus_window_number) {
	    current->setFocusFlag(False);
	    wsManager->currentWorkspace()->raiseWindow(next);
	  }
	} else if (wsManager->currentWorkspace()->Count() >= 1) {
	  wsManager->currentWorkspace()->window(0)->setInputFocus();
	}
      }
    } else if (e->xkey.state & ControlMask) {
      if (XKeycodeToKeysym(display, e->xkey.keycode, 0) == XK_Left){
	if (wsManager->currentWorkspaceID() > 1)
	  wsManager->
	    changeWorkspaceID(wsManager->currentWorkspaceID() - 1);
	else
	  wsManager->changeWorkspaceID(wsManager->count() - 1);
      } else if (XKeycodeToKeysym(display, e->xkey.keycode, 0) == XK_Right){
	if (wsManager->currentWorkspaceID() != wsManager->count() - 1)
	  wsManager->
	    changeWorkspaceID(wsManager->currentWorkspaceID() + 1);
	else
	  wsManager->changeWorkspaceID(1);
      }
    }

    break;
  }

  case ClientMessage: {
    if (e->xclient.message_type == _BLACKBOX_MESSAGE &&
	e->xclient.format == 32) {
      if (e->xclient.data.l[0] == __blackbox_confirmControl) {
	e->xclient.data.l[2] = __blackbox_accept;
	XSendEvent(display, e->xclient.data.l[1], False, NoEventMask, e);
      } else if (e->xclient.data.l[0] == __blackbox_addTopLevelWindow) {
	new Application(this, e->xclient.data.l[1]);
	e->xclient.data.l[2] = __blackbox_accept;
        XSendEvent(display, e->xclient.data.l[1], False, NoEventMask, e);
	
	unsigned long foo = e->xclient.data.l[1];
	XChangeProperty(display, (Window) e->xclient.data.l[1],
			_BLACKBOX_CONTROL, _BLACKBOX_CONTROL, 32,
			PropModeReplace, (unsigned char *) &foo, 1);
      } else {
	Application *app = searchApp((Window) e->xclient.data.l[1]);
	if (app  != NULL)
	  app->clientMessageEvent(&e->xclient);
      }
    }
    
    break;
  }

  default:
#ifdef SHAPE
    if (e->type == shape.event_basep) {
      XShapeEvent *shape_event = (XShapeEvent *) e;

      BlackboxWindow *eWin = NULL;
      if (((eWin = searchWindow(e->xany.window)) != NULL) ||
	  (shape_event->kind != ShapeBounding))
	eWin->shapeEvent(shape_event);
    }
#endif
    break;
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


Application *Blackbox::searchApp(Window window) {
  if (validateWindow(window)) {
    Application *app;
    LinkedListIterator<ApplicationSearch> it(appSearchList);

    for (; it.current(); it++) {
      ApplicationSearch *tmp = it.current();
      if (tmp)
	if (tmp->window == window) {
	  app = tmp->data;
	  return app;
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
	  if (w->clientWindow() != win->clientWindow())
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


WorkspaceManager *Blackbox::searchWSManager(Window window) {
  if (validateWindow(window)) {
    WorkspaceManager *wsm = NULL;
    LinkedListIterator<WSManagerSearch> it(wsManagerSearchList);
    
    for (; it.current(); it++) {
      WSManagerSearch *tmp = it.current();
      
      if (tmp)
	if (tmp->window == window) {
	  wsm = tmp->data;
	  return wsm;
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


void Blackbox::saveWSManagerSearch(Window window, WorkspaceManager *data) {
  WSManagerSearch *tmp = new WSManagerSearch;
  tmp->window = window;
  tmp->data = data;
  wsManagerSearchList->insert(tmp);
}


void Blackbox::saveAppSearch(Window window, Application *data) {
  ApplicationSearch *tmp = new ApplicationSearch;
  tmp->window = window;
  tmp->data = data;
  appSearchList->insert(tmp);
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


void Blackbox::removeWSManagerSearch(Window window) {
  LinkedListIterator<WSManagerSearch> it(wsManagerSearchList);
  for (; it.current(); it++) {
    WSManagerSearch *tmp = it.current();

    if (tmp)
      if (tmp->window == window) {
	wsManagerSearchList->remove(tmp);
	delete tmp;
	break;
      }
  }
}


void Blackbox::removeAppSearch(Window window) {
  LinkedListIterator<ApplicationSearch> it(appSearchList);
  for (; it.current(); it++) {
    ApplicationSearch *tmp = it.current();

    if (tmp)
      if (tmp->window == window) {
	appSearchList->remove(tmp);
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
  shutdown = True;
}


void Blackbox::Restart(char *prog) {
  XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);

  if (prog) {
    execlp(prog, prog, NULL);
    perror(prog);
  }

  // fall back in case the above execlp doesn't work
  execvp(b_argv[0], b_argv);

  // fall back in case we can't re execvp() ourself
  Exit();
}


void Blackbox::Shutdown(Bool do_delete) {
  if (do_delete)
    delete this;
}


// *************************************************************************
// Session utility and maintainence
// *************************************************************************

void Blackbox::reassociateWindow(BlackboxWindow *w) {
  if (! w->isStuck() && w->workspace() != wsManager->currentWorkspaceID()) {
    wsManager->workspace(w->workspace())->removeWindow(w);
    wsManager->currentWorkspace()->addWindow(w);
  }
}


// *************************************************************************
// Menu loading
// *************************************************************************

void Blackbox::InitMenu(void) {
  if (rootmenu) 
    delete rootmenu;
  
  rootmenu = new Rootmenu(this);
  
  Bool defaultMenu = True;

  if (resource.menuFile) {
    FILE *menuFile = fopen(resource.menuFile, "r");

    if (menuFile) {
      if (! feof(menuFile)) {
	char line[256], tmp1[256];
	memset(line, 0, 256);
	memset(tmp1, 0, 256);

	while (fgets(line, 256, menuFile) && ! feof(menuFile)) {
	  if (line[0] != '#') {
	    int i, ri, len = strlen(line);
	    
	    for (i = 0; i < len; i++)
	      if (line[i] == '[') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ']') break;
	    
	    if (i < ri && ri > 0) {
	      strncpy(tmp1, line + i, ri - i);
	      *(tmp1 + (ri - i)) = '\0';
	      
	      if (strstr(tmp1, "begin")) {
		for (i = 0; i < len; i++)
		  if (line[i] == '(') { i++; break; }
		for (ri = len; i < len; ri--)
		  if (line[ri] == ')') { break; }
		
		char *label = 0;
		if (i < ri && ri > 0) {
		  label = new char[ri - i + 1];
		  strncpy(label, line + i, ri - i);
		  *(label + (ri - i)) = '\0';
		}
		
		rootmenu->setMenuLabel(label);
		defaultMenu = parseMenuFile(menuFile, rootmenu);
		break;
	      }
	    }
	  }
	}
      } else
	fprintf(stderr, "%s: Empty menu file", resource.menuFile);

      fclose(menuFile);
    } else
      perror(resource.menuFile);
  }
  
  if (defaultMenu) {
    rootmenu->defaultMenu();
    rootmenu->insert("xterm", B_Execute, "xterm");
    rootmenu->insert("Restart", B_Restart);
    rootmenu->insert("Exit", B_Exit);
  }
}


Bool Blackbox::parseMenuFile(FILE *file, Rootmenu *menu) {
  char line[256], tmp1[256], tmp2[256];

  while (! feof(file)) {
    memset(line, 0, 256);
    memset(tmp1, 0, 256);
    memset(tmp2, 0, 256);
    
    if (fgets(line, 256, file)) {
      if (line[0] != '#') {
	int i, ri, len = strlen(line);

	for (i = 0; i < len; i++)
	  if (line[i] == '[') { i++; break; }
	for (ri = len; ri > 0; ri--)
	  if (line[ri] == ']') break;

	if (i < ri && ri > 0) {
	  strncpy(tmp1, line + i, ri - i);
	  *(tmp1 + (ri - i)) = '\0';
	  
	  if (strstr(tmp1, "exit")) {
	    for (i = 0; i < len; i++)
	      if (line[i] == '(') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ')') break;
	    
	    if (i < ri && ri > 0) {
	      char *label = new char[ri - i + 1];
	      strncpy(label, line + i, ri - i);
	      *(label + (ri - i)) = '\0';
	      
	      menu->insert(label, B_Exit);
	    }
	  } else if (strstr(tmp1, "restart")) {
	    for (i = 0; i < len; i++)
	      if (line[i] == '(') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ')') break;
	    
	    if (i < ri && ri > 0) {
	      char *label = new char[ri - i + 1];
	      strncpy(label, line + i, ri - i);
	      *(label + (ri - i)) = '\0';
	      
	      for (i = 0; i < len; i++)
		if (line[i] == '{') { i++; break; }
	      for (ri = len; ri > 0; ri--)
		if (line[ri] == '}') break;
	      
	      char *other = 0;
	      if (i < ri && ri > 0) {
		other = new char[ri - i + 1];
		strncpy(other, line + i, ri - i);
		*(other + (ri - i)) = '\0';
	      }

	      if (other)
		menu->insert(label, B_RestartOther, other);
	      else
		menu->insert(label, B_Restart);
	    }
	  } else if (strstr(tmp1, "reconfig")) {
	    for (i = 0; i < len; i++)
	      if (line[i] == '(') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ')') break;
	    
	    if (i < ri && ri > 0) {
	      char *label = new char[ri - i + 1];
	      strncpy(label, line + i, ri - i);
	      *(label + (ri - i)) = '\0';
	      
	      for (i = 0; i < len; i++)
		if (line[i] == '{') { i++; break; }
	      for (ri = len; ri > 0; ri--)
		if (line[ri] == '}') break;
	      
	      char *exec = 0;
	      if (i < ri && ri > 0) {
		exec = new char[ri - i + 1];
		strncpy(exec, line + i, ri - i);
		*(exec + (ri - i)) = '\0';
	      }

	      if (exec)
		menu->insert(label, B_ExecReconfigure, exec);
	      else
		menu->insert(label, B_Reconfigure);
	    }
	  } else if (strstr(tmp1, "submenu")) {
	    for (i = 0; i < len; i++)
	      if (line[i] == '(') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ')') break;

	    char *label;
	    if (i < ri && ri > 0) {
	      label = new char[ri - i + 1];
	      strncpy(label, line + i, ri - i);
	      *(label + (ri - i)) = '\0';
	    } else
	      label = "(nil)";
	    
	    // this is an optional feature
	    for (i = 0; i < len; i++)
	      if (line[i] == '{') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == '}') break;
	    
	    char *title = 0;
	    if (i < ri && ri > 0) {
	      title = new char[ri - i + 1];
	      strncpy(title, line + i, ri - i);
	      *(title + (ri - i)) = '\0';
	    } else {
	      int l = strlen(label);
	      title = new char[l + 1];
	      strncpy(title, label, l + 1);
	      //	      *(title + l + 1) = '\0';
	    }
	    
	    Rootmenu *submenu = new Rootmenu(this);
	    submenu->setMenuLabel(title);
	    parseMenuFile(file, submenu);
	    submenu->Update();
	    menu->insert(label, submenu);
	  } else if (strstr(tmp1, "end")) {
	    break;
	  } else if (strstr(tmp1, "exec")) {
	    for (i = 0; i < len; i++)
	      if (line[i] == '(') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ')') break;

	    if (i < ri && ri > 0) {
	      char *label = new char[ri - i + 1];
	      strncpy(label, line + i, ri - i);
	      *(label + (ri - i)) = '\0';
   
	      for (i = 0; i < len; i++)
		if (line[i] == '{') { i++; break; }
	      for (ri = len; ri > 0; ri--)
		if (line[ri] == '}') break;
	      
	      if (i < ri && ri > 0) {
		char *command = new char[ri - i + 1];
		strncpy(command, line + i, ri - i);
		*(command + (ri - i)) = '\0';

		if (label && command)
		  menu->insert(label, B_Execute, command);
		else
		  fprintf(stderr,
                          "error: label(%s) == NULL || command(%s) == NULL\n",
			  label, command);
	      } else
		printf("error: no command string for [exec] (%s)\n", label);
	    } else
	      printf("error: no label string for [exec]\n");
	  }
	}
      }
    }
  }

  return ((menu->Count() == 0) ? True : False);
}


// *************************************************************************
// Resource loading
// *************************************************************************

void Blackbox::LoadDefaults(void) {
#define BLACKBOXAD XAPPLOADDIR##"/Blackbox.ad"
#define BLACKBOXMENUAD XAPPLOADDIR##"/BlackboxMenu.ad"
  
  resource.blackboxrc = 0;
  char *homedir = getenv("HOME"), *rcfile = new char[strlen(homedir) + 32];
  sprintf(rcfile, "%s/.blackboxrc", homedir);

  if ((resource.blackboxrc = XrmGetFileDatabase(rcfile)) == NULL)
    resource.blackboxrc = XrmGetFileDatabase(BLACKBOXAD);

  delete [] rcfile;

  XrmValue value;
  char *value_type;

  // load window config
  if (! (resource.win.decorTexture =
	 blackbox->readDatabaseTexture("associatedWindow.decorations",
				       "AssociatedWindow.Decorations")))
    resource.win.decorTexture = BImageRaised|BImageSolid|BImageBevel2;

  if (! (resource.win.handleTexture =
	 blackbox->readDatabaseTexture("associatedWindow.handle",
				       "AssociatedWindow.Handle")))
    resource.win.handleTexture = BImageRaised|BImageSolid|BImageBevel2;

  if (! (resource.win.frameTexture =
	 blackbox->readDatabaseTexture("associatedWindow.frame",
				       "AssociatedWindow.Frame")))
    resource.win.frameTexture = BImageRaised|BImageBevel2|BImageSolid;

  if (! (resource.win.buttonTexture =
	 blackbox->readDatabaseTexture("associatedWindow.button",
				       "AssociatedWindow.Button")))
    resource.win.buttonTexture = BImageRaised|BImageBevel2|BImageSolid;
  
  // button, focused and unfocused colors
  if (! blackbox->readDatabaseColor("associatedWindow.focusColor",
				    "AssociatedWindow.FocusColor",
				    &resource.win.focusColor))
    resource.win.focusColor.pixel =
      blackbox->getColor("darkblue", &resource.win.focusColor.r,
			 &resource.win.focusColor.g,
			 &resource.win.focusColor.b);
  
  if (! blackbox->readDatabaseColor("associatedWindow.unfocusColor",
				    "AssociatedWindow.UnfocusColor",
				    &resource.win.unfocusColor))
    resource.win.unfocusColor.pixel =
      blackbox->getColor("grey", &resource.win.unfocusColor.r,
			 &resource.win.unfocusColor.g,
			 &resource.win.unfocusColor.b);

  if ((resource.win.decorTexture & BImageGradient) ||
      (resource.win.buttonTexture & BImageGradient)) {
    if (! blackbox->readDatabaseColor("associatedWindow.focusToColor",
				      "AssociatedWindow.FocusToColor",
				      &resource.win.focusColorTo))
      resource.win.focusColorTo.pixel =
	blackbox->getColor("black", &resource.win.focusColorTo.r,
			   &resource.win.focusColorTo.g,
			   &resource.win.focusColorTo.b);

    if (! blackbox->readDatabaseColor("associatedWindow.unfocusToColor",
				      "AssociatedWindow.UnfocusToColor",
				      &resource.win.unfocusColorTo))
      resource.win.unfocusColorTo.pixel =
	blackbox->getColor("darkgrey", &resource.win.unfocusColorTo.r,
			   &resource.win.unfocusColorTo.g,
			   &resource.win.unfocusColorTo.b);
  }
  
  // focused and unfocused text colors
  if (! blackbox->readDatabaseColor("associatedWindow.focusTextColor",
				    "AssociatedWindow.FocusTextColor",
				    &resource.win.focusTextColor))
    resource.win.focusTextColor.pixel =
      blackbox->getColor("white", &resource.win.focusTextColor.r,
			 &resource.win.focusTextColor.g,
			 &resource.win.focusTextColor.b);
  
  if (! blackbox->readDatabaseColor("associatedWindow.unfocusTextColor",
				    "AssociatedWindow.UnfocusTextColor",
				    &resource.win.unfocusTextColor))
    resource.win.unfocusTextColor.pixel =
      blackbox->getColor("black", &resource.win.unfocusTextColor.r,
			 &resource.win.unfocusTextColor.g,
			 &resource.win.unfocusTextColor.b);
  
  if (! (blackbox->readDatabaseColor("associatedWindow.frame.color",
				     "AssociatedWindow.Frame.Color",
				     &resource.win.frameColor)))
    resource.win.frameColor.pixel =
      blackbox->getColor("grey", &resource.win.frameColor.r,
			 &resource.win.frameColor.g,
			 &resource.win.frameColor.b);

  // load menu configuration
  if (! (resource.menu.titleTexture =
	 blackbox->readDatabaseTexture("baseMenu.title", "BaseMenu.Title")))
    resource.menu.titleTexture = BImageSolid|BImageRaised|BImageBevel2;
  
  if (! blackbox->readDatabaseColor("baseMenu.title.color",
				    "BaseMenu.Title.Color",
				    &resource.menu.titleColor))
    resource.menu.titleColor.pixel =
      blackbox->getColor("darkblue", &resource.menu.titleColor.r,
			 &resource.menu.titleColor.g,
			 &resource.menu.titleColor.b);

  if (resource.menu.titleTexture & BImageGradient)
    if (! blackbox->readDatabaseColor("baseMenu.title.colorTo",
				      "BaseMenu.Title.ColorTo",
				      &resource.menu.titleColorTo))
      resource.menu.titleColorTo.pixel =
	blackbox->getColor("black", &resource.menu.titleColorTo.r,
			   &resource.menu.titleColorTo.g,
			   &resource.menu.titleColorTo.b);
  
  if (! blackbox->readDatabaseColor("baseMenu.title.textColor",
				    "BaseMenu.Title.TextColor",
				    &resource.menu.titleTextColor))
    resource.menu.titleTextColor.pixel =
      blackbox->getColor("white", &resource.menu.titleTextColor.r,
			 &resource.menu.titleTextColor.g,
			 &resource.menu.titleTextColor.b);
  
  if (! (resource.menu.frameTexture =
	 blackbox->readDatabaseTexture("baseMenu.frame", "BaseMenu.Frame")))
    resource.menu.frameTexture = BImageSolid|BImageRaised|BImageBevel2;
  
  if (! blackbox->readDatabaseColor("baseMenu.frame.color",
				    "BaseMenu.Frame.Color",
				    &resource.menu.frameColor))
    resource.menu.frameColor.pixel =
      blackbox->getColor("grey", &resource.menu.frameColor.r,
			 &resource.menu.frameColor.g,
			 &resource.menu.frameColor.b);

  if (resource.menu.frameTexture & BImageGradient)
    if (! blackbox->readDatabaseColor("baseMenu.frame.colorTo",
				      "BaseMenu.Frame.ColorTo",
				      &resource.menu.frameColorTo))
      resource.menu.frameColorTo.pixel =
	blackbox->getColor("darkgrey", &resource.menu.frameColorTo.r,
			   &resource.menu.frameColorTo.g,
			   &resource.menu.frameColorTo.b);

  if (! blackbox->readDatabaseColor("baseMenu.frame.highlightColor",
				    "BaseMenu.Frame.HighLightColor",
				    &resource.menu.highlightColor))
    resource.menu.highlightColor.pixel =
      blackbox->getColor("black", &resource.menu.highlightColor.r,
			 &resource.menu.highlightColor.g,
			 &resource.menu.highlightColor.b);

  if (! blackbox->readDatabaseColor("baseMenu.frame.textColor",
				    "BaseMenu.Frame.TextColor",
				    &resource.menu.frameTextColor))
    resource.menu.frameTextColor.pixel =
      blackbox->getColor("black", &resource.menu.frameTextColor.r,
			 &resource.menu.frameTextColor.g,
			 &resource.menu.frameTextColor.b);
  
  if (! blackbox->readDatabaseColor("baseMenu.frame.hiTextColor",
				    "BaseMenu.Frame.HiTextColor",
				    &resource.menu.hiTextColor))
    resource.menu.hiTextColor.pixel =
      blackbox->getColor("white", &resource.menu.hiTextColor.r,
			 &resource.menu.hiTextColor.g,
			 &resource.menu.hiTextColor.b);

  // toolbox main window
  if (! (resource.wsm.windowTexture =
	 blackbox->readDatabaseTexture("workspaceManager.toolbox",
				       "WorkspaceManager.Toolbox")))
    resource.wsm.windowTexture = BImageSolid|BImageRaised|BImageBevel2;

  if (! blackbox->readDatabaseColor("workspaceManager.toolbox.color",
				    "WorkspaceManager.Toolbox.Color",
				    &resource.wsm.windowColor))
    resource.wsm.windowColor.pixel =
      blackbox->getColor("grey", &resource.wsm.windowColor.r,
			 &resource.wsm.windowColor.g,
			 &resource.wsm.windowColor.b);

  if (resource.wsm.windowTexture & BImageGradient)
    if (! blackbox->readDatabaseColor("workspaceManager.toolbox.colorTo",
				     "WorkspaceManager.Toolbox.ColorTo",
				     &resource.wsm.windowColorTo))
      resource.wsm.windowColorTo.pixel =
	blackbox->getColor("darkgrey", &resource.wsm.windowColorTo.r,
			   &resource.wsm.windowColorTo.g,
			   &resource.wsm.windowColorTo.b);

  // toolbox label
  if (! (resource.wsm.labelTexture =
	 blackbox->readDatabaseTexture("workspaceManager.label",
				       "WorkspaceManager.Label")))
    resource.wsm.labelTexture = BImageSolid|BImageRaised|BImageBevel2;

  if (! blackbox->readDatabaseColor("workspaceManager.label.color",
				    "WorkspaceManager.Label.Color",
				    &resource.wsm.labelColor))
    resource.wsm.labelColor.pixel =
      blackbox->getColor("grey", &resource.wsm.labelColor.r,
			 &resource.wsm.labelColor.g,
			 &resource.wsm.labelColor.b);

  if (resource.wsm.labelTexture & BImageGradient)
    if (! blackbox->readDatabaseColor("workspaceManager.label.colorTo",
				      "WorkspaceManager.Label.ColorTo",
				      &resource.wsm.labelColorTo))
      resource.wsm.labelColorTo.pixel =
	blackbox->getColor("darkgrey", &resource.wsm.labelColorTo.r,
			   &resource.wsm.labelColorTo.g,
			   &resource.wsm.labelColorTo.b);
  
  // toolbox clock
  if (! (resource.wsm.clockTexture =
	 blackbox->readDatabaseTexture("workspaceManager.clock",
				       "WorkspaceManager.Clock")))
    resource.wsm.clockTexture = BImageSolid|BImageRaised|BImageBevel2;

  if (! blackbox->readDatabaseColor("workspaceManager.clock.color",
				    "WorkspaceManager.Clock.Color",
				    &resource.wsm.clockColor))
    resource.wsm.clockColor.pixel =
      blackbox->getColor("grey", &resource.wsm.clockColor.r,
			 &resource.wsm.clockColor.g,
			 &resource.wsm.clockColor.b);

  if (resource.wsm.clockTexture & BImageGradient)
    if (! blackbox->readDatabaseColor("workspaceManager.clock.colorTo",
				      "WorkspaceManager.Clock.ColorTo",
				      &resource.wsm.clockColorTo))
      resource.wsm.clockColorTo.pixel =
	blackbox->getColor("darkgrey", &resource.wsm.clockColorTo.r,
			   &resource.wsm.clockColorTo.g,
			   &resource.wsm.clockColorTo.b);

  // toolbox buttons
  if (! (resource.wsm.buttonTexture =
	 blackbox->readDatabaseTexture("workspaceManager.button",
				       "WorkspaceManager.Button")))
    resource.wsm.buttonTexture = BImageSolid|BImageRaised|BImageBevel2;
  
  if (! blackbox->readDatabaseColor("workspaceManager.button.color",
				    "WorkspaceManager.Button.Color",
				    &resource.wsm.buttonColor))
    resource.wsm.buttonColor.pixel =
      blackbox->getColor("grey", &resource.wsm.buttonColor.r,
			 &resource.wsm.buttonColor.g,
			 &resource.wsm.buttonColor.b);

  if (resource.wsm.buttonTexture & BImageGradient)
    if (! blackbox->readDatabaseColor("workspaceManager.button.colorTo",
				      "WorkspaceManager.Button.ColorTo",
				      &resource.wsm.buttonColorTo))
      resource.wsm.buttonColorTo.pixel =
	blackbox->getColor("darkgrey", &resource.wsm.buttonColorTo.r,
			   &resource.wsm.buttonColorTo.g,
			   &resource.wsm.buttonColorTo.b);

  // toolbox text color
  if (! blackbox->readDatabaseColor("workspaceManager.textColor",
				    "WorkspaceManager.TextColor",
				    &resource.wsm.textColor))
    resource.wsm.textColor.pixel =
      blackbox->getColor("black", &resource.wsm.textColor.r,
			 &resource.wsm.textColor.g,
			 &resource.wsm.textColor.b);

  // load icon config
  if (! (resource.icon.texture =
	 blackbox->readDatabaseTexture("clientIcon.texture",
				       "ClientIcon.Texture")))
    resource.icon.texture = BImageRaised|BImageSolid|BImageBevel1;

  if (! blackbox->readDatabaseColor("clientIcon.color", "ClientIcon.Color",
				    &resource.icon.color))
    resource. icon.color.pixel =
      blackbox->getColor("darkblue", &resource.icon.color.r,
			 &resource.icon.color.g, &resource.icon.color.b);

  if (resource.icon.texture & BImageGradient)
    if (! blackbox->readDatabaseColor("clientIcon.colorTo",
				      "ClientIcon.ColorTo",
				      &resource.icon.colorTo))
      resource.icon.colorTo.pixel =
	blackbox->getColor("black", &resource.icon.colorTo.r,
			   &resource.icon.colorTo.g, &resource.icon.colorTo.b);

  if (! blackbox->readDatabaseColor("clientIcon.textColor",
				    "ClientIcon.TextColor",
				    &resource.icon.textColor))
    resource.icon.textColor.pixel =
      blackbox->getColor("grey", &resource.icon.textColor.r,
			 &resource.icon.textColor.g,
			 &resource.icon.textColor.b);

  // load border color
  if (! (readDatabaseColor("session.borderColor", "Session.BorderColor",
			   &resource.borderColor)))
    resource.borderColor.pixel =
      getColor("black", &resource.borderColor.r, &resource.borderColor.g,
	       &resource.borderColor.b);
  
  // load session configuration parameters
  if (XrmGetResource(resource.blackboxrc,
		     "session.handleWidth",
		     "session.HandleWidth", &value_type,
		     &value)) {
    if (sscanf(value.addr, "%u", &resource.handleWidth) != 1)
      resource.handleWidth = 8;
    else
      if (resource.handleWidth > (xres / 2) ||
          resource.handleWidth == 0)
	resource.handleWidth = 8;
  } else
    resource.handleWidth = 8;

  if (XrmGetResource(resource.blackboxrc,
		     "session.bevelWidth",
		     "session.BevelWidth", &value_type,
		     &value)) {
    if (sscanf(value.addr, "%u", &resource.bevelWidth) != 1)
      resource.bevelWidth = 4;
    else
      if (resource.bevelWidth > (xres / 2) || resource.bevelWidth == 0)
	resource.bevelWidth = 4;
  } else
    resource.bevelWidth = 4;
  
  if (XrmGetResource(resource.blackboxrc,
		     "session.workspaces",
		     "Session.Workspaces", &value_type, &value)) {
    if (sscanf(value.addr, "%d", &resource.workspaces) != 1) {
      resource.workspaces = 1;
    }
  } else
    resource.workspaces = 1;

  // load session menu file
  if (XrmGetResource(resource.blackboxrc,
		     "session.menuFile",
		     "Session.MenuFile", &value_type, &value)) {
    int len = strlen(value.addr);
    resource.menuFile = new char[len + 1];
    memset(resource.menuFile, 0, len + 1);
    strncpy(resource.menuFile, value.addr, len);
  } else {
    int len = strlen(BLACKBOXMENUAD);
    resource.menuFile = new char[len + 1];
    memset(resource.menuFile, 0, len + 1);
    strncpy(resource.menuFile, BLACKBOXMENUAD, len);
  }

  if (XrmGetResource(resource.blackboxrc,
		     "session.titleJustify",
		     "Session.TitleJustify", &value_type, &value)) {
    if (! strncasecmp("leftjustify", value.addr, value.size))
      resource.justification = B_LeftJustify;
    else if (! strncasecmp("rightjustify", value.addr, value.size))
      resource.justification = B_RightJustify;
    else if (! strncasecmp("centerjustify", value.addr, value.size))
      resource.justification = B_CenterJustify;
    else
      resource.justification = B_LeftJustify;
  } else
    resource.justification = B_LeftJustify;

  if (XrmGetResource(resource.blackboxrc,
		     "session.moveStyle",
		     "Session.MoveStyle", &value_type, &value)) {
    if (! strncasecmp("opaque", value.addr, value.size))
      resource.opaqueMove = True;
    else
      resource.opaqueMove = False;
  } else
    resource.opaqueMove = False;

  if (XrmGetResource(resource.blackboxrc,
		     "session.imageDither",
		     "Session.ImageDither", &value_type,
		     &value)) {
    if (! strncasecmp("true", value.addr, value.size))
      resource.imageDither = True;
    else
      resource.imageDither = False;
  } else
    resource.imageDither = True;

  if (startup)
    if (depth == 8) {
      if (XrmGetResource(resource.blackboxrc,
			 "session.colorsPerChannel",
			 "Session.ColorsPerChannel", &value_type,
			 &value)) {
	if (sscanf(value.addr, "%d", &resource.cpc8bpp) != 1) {
	  resource.cpc8bpp = 5;
	} else {
	  if (resource.cpc8bpp < 1) resource.cpc8bpp = 1;
	  if (resource.cpc8bpp > 6) resource.cpc8bpp = 6;
	}
      }
    } else
      resource.cpc8bpp = 0;

  if (XrmGetResource(resource.blackboxrc,
		     "workspaceManager.24hourClock",
		     "WorkspaceManager.24HourClock", &value_type,
		     &value)) {
    if (! strncasecmp(value.addr, "true", value.size))
      resource.clock24hour = True;
    else
      resource.clock24hour = False;
  } else
    resource.clock24hour = False;

  const char *defaultFont = "-*-helvetica-medium-r-*-*-*-120-*-*-*-*-*-*";
  if (resource.font.title) {
    XFreeFont(display, resource.font.title);
    resource.font.title = 0;
  }

  if (XrmGetResource(resource.blackboxrc,
		     "session.titleFont",
		     "Session.TitleFont", &value_type, &value)) {
    if ((resource.font.title = XLoadQueryFont(display, value.addr)) == NULL) {
      fprintf(stderr,
	      " blackbox: couldn't load font '%s'\n"
	      "  ...  reverting to default font.", value.addr);
      if ((resource.font.title = XLoadQueryFont(display, defaultFont))
	  == NULL) {
	fprintf(stderr,
		"blackbox: couldn't load default font.  please check to\n"
		"make sure the necessary font is installed '%s'\n",
		defaultFont);
	exit(2);
      }  
    }
  } else {
    if ((resource.font.title = XLoadQueryFont(display, defaultFont)) == NULL) {
      fprintf(stderr,
	      "blackbox: couldn't load default font.  please check to\n"
	      "make sure the necessary font is installed '%s'\n", defaultFont);
      exit(2);
    }
  }

  if (resource.font.menu) {
    XFreeFont(display, resource.font.menu);
    resource.font.menu = 0;
  }

  if (XrmGetResource(resource.blackboxrc,
		     "session.menuFont",
		     "Session.MenuFont", &value_type, &value)) {
    if ((resource.font.menu = XLoadQueryFont(display, value.addr)) == NULL) {
      fprintf(stderr,
	      " blackbox: couldn't load font '%s'\n"
	      "  ...  reverting to default font.", value.addr);
      if ((resource.font.menu = XLoadQueryFont(display, defaultFont))
	  == NULL) {
	fprintf(stderr,
		"blackbox: couldn't load default font.  please check to\n"
		"make sure the necessary font is installed '%s'\n",
		defaultFont);
	exit(2);
      }  
    }
  } else {
    if ((resource.font.menu = XLoadQueryFont(display, defaultFont)) == NULL) {
      fprintf(stderr,
	      "blackbox: couldn't load default font.  please check to\n"
	      "make sure the necessary font is installed '%s'\n", defaultFont);
      exit(2);
    }
  }

  XrmDestroyDatabase(resource.blackboxrc);
}


unsigned long Blackbox::readDatabaseTexture(char *rname, char *rclass) {
  XrmValue value;
  char *value_type;
  unsigned long texture = 0;

  if (XrmGetResource(resource.blackboxrc, rname, rclass, &value_type,
		     &value)) {
    if (strstr(value.addr, "Solid"))
      texture |= BImageSolid;
    else if (strstr(value.addr, "Gradient")) {
      texture |= BImageGradient;

      if (strstr(value.addr, "Diagonal"))
	texture |= BImageDiagonal;
      else if (strstr(value.addr, "Horizontal"))
	texture |= BImageHorizontal;
      else if (strstr(value.addr, "Vertical"))
	texture |= BImageVertical;
      else
	texture |= BImageDiagonal;
    } else
      texture |= BImageSolid;

    if (strstr(value.addr, "Raised"))
      texture |= BImageRaised;
    else if (strstr(value.addr, "Sunken"))
      texture |= BImageSunken;
    else if (strstr(value.addr, "Flat"))
      texture |= BImageFlat;
    else
      texture |= BImageRaised;

    if (! (texture & BImageFlat))
      if (strstr(value.addr, "Bevel"))
	if (strstr(value.addr, "Bevel1"))
	  texture |= BImageBevel1;
	else if (strstr(value.addr, "Bevel2"))
	  texture |= BImageBevel2;
	else if (strstr(value.addr, "MotifBevel"))
	  texture |= BImageMotifBevel;
	else
	  texture |= BImageBevel1;
  }
  
  return texture;
}


Bool Blackbox::readDatabaseColor(char *rname, char *rclass, BColor *color) {
  XrmValue value;
  char *value_type;

  if (XrmGetResource(resource.blackboxrc, rname, rclass, &value_type,
		     &value)) {
    color->pixel = getColor(value.addr, &color->r, &color->g, &color->b);
    return True;
  }

  return False;
}


// *************************************************************************
// Color lookup and allocation methods
// *************************************************************************

void Blackbox::InitColor(void) {
  if (depth == 8) {
    int cpc3 = (resource.cpc8bpp * resource.cpc8bpp * resource.cpc8bpp);
    colors_8bpp = new XColor[cpc3];
    int i = 0;
    for (int r = 0; r < resource.cpc8bpp; r++)
      for (int g = 0; g < resource.cpc8bpp; g++)
	for (int b = 0; b < resource.cpc8bpp; b++) {
	  colors_8bpp[i].red = (r * 0xffff) / (resource.cpc8bpp - 1);
	  colors_8bpp[i].green = (g * 0xffff) / (resource.cpc8bpp - 1);
	  colors_8bpp[i].blue = (b * 0xffff) / (resource.cpc8bpp - 1);;
	  colors_8bpp[i++].flags = DoRed|DoGreen|DoBlue;
	}
    
    XGrabServer(display);
    
    Colormap colormap = DefaultColormap(display, DefaultScreen(display));
    for (i = 0; i < cpc3; i++)
      if (! XAllocColor(display, colormap, &colors_8bpp[i])) {
	fprintf(stderr, "couldn't alloc color %i %i %i\n", colors_8bpp[i].red,
		colors_8bpp[i].green, colors_8bpp[i].blue);		
	colors_8bpp[i].flags = 0;
      } else
	colors_8bpp[i].flags = DoRed|DoGreen|DoBlue;

    XUngrabServer(display);
  } else
    colors_8bpp = 0;
}


unsigned long Blackbox::getColor(const char *colorname,
					unsigned char *r, unsigned char *g,
					unsigned char *b)
{
  XColor color;
  XWindowAttributes attributes;
  
  XGetWindowAttributes(display, root, &attributes);
  color.pixel = 0;
  if (!XParseColor(display, attributes.colormap, colorname, &color)) {
    fprintf(stderr, "blackbox: color parse error: \"%s\"\n", colorname);
  } else if (!XAllocColor(display, attributes.colormap, &color)) {
    fprintf(stderr, "blackbox: color alloc error: \"%s\"\n", colorname);
  }
  
  if (color.red == 65535) *r = 0xff;
  else *r = (unsigned char) (color.red / 0xff);
  if (color.green == 65535) *g = 0xff;
  else *g = (unsigned char) (color.green / 0xff);
  if (color.blue == 65535) *b = 0xff;
  else *b = (unsigned char) (color.blue / 0xff);
 
  return color.pixel;
}


unsigned long Blackbox::getColor(const char *colorname) {
  XColor color;
  XWindowAttributes attributes;
  
  XGetWindowAttributes(display, root, &attributes);
  color.pixel = 0;
  if (!XParseColor(display, attributes.colormap, colorname, &color)) {
    fprintf(stderr, "blackbox: color parse error: \"%s\"\n", colorname);
  } else if (!XAllocColor(display, attributes.colormap, &color)) {
    fprintf(stderr, "blackbox: color alloc error: \"%s\"\n", colorname);
  }

  return color.pixel;
}


// *************************************************************************
// Resource reconfiguration
// *************************************************************************

void Blackbox::Reconfigure(void) {
  reconfigure = True;
}


void Blackbox::do_reconfigure(void) {
  XGrabServer(display);

  LoadDefaults();
  InitMenu();

  rootmenu->Reconfigure();
  wsManager->Reconfigure();

  XGCValues gcv;
  gcv.foreground = getColor("white");
  gcv.function = GXxor;
  gcv.line_width = 2;
  gcv.subwindow_mode = IncludeInferiors;
  XChangeGC(display, opGC, GCForeground|GCFunction|GCSubwindowMode, &gcv);
  
  XUngrabServer(display);
}
