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

#define _GNU_SOURCE

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
#include "Workspace.hh"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>


// *************************************************************************
// signal handler to allow for proper and gentle shutdown
// *************************************************************************

Blackbox *blackbox;

static void signalhandler(int i) {
  static int re_enter = 0;
  
  fprintf(stderr, "%s: toplevel:\n\t[ signal %d caught ]\n", __FILE__, i);
  if (! re_enter) {
    re_enter = 1;
    fprintf(stderr, "\t[ shutting down ]\n");
    blackbox->Shutdown(False);
  }

  fprintf(stderr, "\t[ exiting ]\n");
  abort();
}


// *************************************************************************
// X error handler to detect a running window manager
// *************************************************************************

static int anotherWMRunning(Display *d, XErrorEvent *e) {
  char errtxt[128];
  XGetErrorText(d, e->error_code, errtxt, 128);
  fprintf(stderr,
	  "blackbox: a fatal error occured while querying the X server.\n\n"
	  "X Error of failed request: %d\n"
	  "  (major/minor: %d/%d resource: 0x%08lx)\n"
	  "  %s\n"
	  " - There is another window manager already running. -\n",
	  e->error_code, e->request_code, e->minor_code, e->resourceid,
	  errtxt);
  exit(3);
  return(-1);
}


// *************************************************************************
// Blackbox class code constructor and destructor
// *************************************************************************

Blackbox::Blackbox(int argc, char **argv, char *dpy_name) {
  // install signal handlers for fatal signals
  signal(SIGSEGV, (void (*)(int)) signalhandler);
  signal(SIGTERM, (void (*)(int)) signalhandler);
  signal(SIGINT, (void (*)(int)) signalhandler);

  ::blackbox = this;
  b_argc = argc;
  b_argv = argv;

  shutdown = False;
  startup = True;
  focus_window_number = -1;

  rootmenu = 0;
  reconfWidget = 0;
  resource.font.menu = resource.font.icon = resource.font.title = 0;

  if ((display = XOpenDisplay(dpy_name)) == NULL) {
    fprintf(stderr, "%s: connection to X server failed\n", b_argv[0]);
    exit(2);
  }

  screen = DefaultScreen(display);
  root = RootWindow(display, screen);
  v = DefaultVisual(display, screen);
  depth = DefaultDepth(display, screen);
  display_name = XDisplayName(dpy_name);

  event_mask = LeaveWindowMask | EnterWindowMask | PropertyChangeMask |
    SubstructureNotifyMask | PointerMotionMask | SubstructureRedirectMask |
    ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
  
  XSetErrorHandler((XErrorHandler) anotherWMRunning);
  XSelectInput(display, root, event_mask);
  XSync(display, False);
  XSetErrorHandler((XErrorHandler) NULL);

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

  cursor.session = XCreateFontCursor(display, XC_left_ptr);
  cursor.move = XCreateFontCursor(display, XC_fleur);
  XDefineCursor(display, root, cursor.session);

  xres = WidthOfScreen(ScreenOfDisplay(display, screen));
  yres = HeightOfScreen(ScreenOfDisplay(display, screen));

  windowSearchList = new LinkedList<WindowSearch>;
  menuSearchList = new LinkedList<MenuSearch>;
  iconSearchList = new LinkedList<IconSearch>;
  wsManagerSearchList = new LinkedList<WSManagerSearch>;
  rWidgetSearchList = new LinkedList<RWidgetSearch>;

  InitColor();
  XrmInitialize();
  LoadDefaults();

  XGCValues gcv;
  gcv.foreground = toolboxTextColor().pixel;
  gcv.function = GXxor;
  gcv.line_width = 2;
  gcv.subwindow_mode = IncludeInferiors;
  gcv.font = resource.font.title->fid;
  opGC = XCreateGC(display, root, GCForeground|GCFunction|GCSubwindowMode|
		   GCFont, &gcv);

  InitMenu();
  wsManager = new WorkspaceManager(this, resource.workspaces);
  wsManager->stackWindows(0, 0);

  unsigned int nchild;
  Window r, p, *children;
  XGrabServer(display);
  XQueryTree(display, root, &r, &p, &children, &nchild);

  for (int i = 0; i < (int) nchild; ++i) {
    if (children[i] == None) continue;

    XWindowAttributes attrib;
    if (XGetWindowAttributes(display, children[i], &attrib)) {
      if ((! attrib.override_redirect) && (attrib.map_state != IsUnmapped)) {
	XSync(display, False);
	BlackboxWindow *nWin = new BlackboxWindow(this, children[i]);
		
	Atom atom;
	int foo;
	unsigned long ulfoo, nitems;
	unsigned char *state;
	XGetWindowProperty(display, nWin->clientWindow(), _XA_WM_STATE,
			   0, 3, False, _XA_WM_STATE, &atom, &foo, &nitems,
			   &ulfoo, &state);

	if (state != NULL) {
	  switch (*((unsigned long *) state)) {
	  case WithdrawnState:
	    nWin->deiconifyWindow();
            nWin->setFocusFlag(False);
	    break;
	    
	  case IconicState:
	    nWin->iconifyWindow();
	    break;
	    
	  case NormalState:
	  default:
	    nWin->deiconifyWindow();
	    nWin->setFocusFlag(False);
	    break;
	  }
	} else {
	  nWin->deiconifyWindow();
	  nWin->setFocusFlag(False);
	} 
      }
    }
  }

  while (XPending(display)) {
    XEvent foo;
    XNextEvent(display, &foo);
  }

  XSynchronize(display, False);
  XUngrabServer(display);
}


Blackbox::~Blackbox(void) {
  XSelectInput(display, root, NoEventMask);

  delete [] resource.menuFile;
  delete rootmenu;
  delete wsManager;

  delete windowSearchList;
  delete iconSearchList;
  delete menuSearchList;
  delete wsManagerSearchList;
  delete rWidgetSearchList;
  
  if (resource.font.title) XFreeFont(display, resource.font.title);
  if (resource.font.menu) XFreeFont(display, resource.font.menu);
  if (resource.font.icon) XFreeFont(display, resource.font.icon);
  XFreeGC(display, opGC);
  
  XSync(display, False);
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

  while (! shutdown) {
    if (reconfigure) {
      do_reconfigure();
      reconfigure = False;
    } else if (XPending(display)) {
      XEvent e;
      XNextEvent(display, &e);
      ProcessEvent(&e);
    } else {
      // put a wait on the network file descriptor for the X connection...
      // this saves blackbox from eating all available cpu
      fd_set rfds;
      FD_ZERO(&rfds);
      FD_SET(xfd, &rfds);
      
      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 5000;
      
      select(xfd + 1, &rfds, 0, 0, &tv);
      wsManager->checkClock();
    }
  }
    
  Dissociate();
}


void Blackbox::ProcessEvent(XEvent *e) {
  switch (e->type) {
  case ButtonPress: {
    BlackboxWindow *bWin = NULL;
    BlackboxIcon *bIcon = NULL;
    Basemenu *rMenu = NULL;
    ReconfigureWidget *rWidget = NULL;
    WorkspaceManager *wsMan = NULL;
    
    if ((bWin = searchWindow(e->xbutton.window)) != NULL) {
      bWin->buttonPressEvent(&e->xbutton);
    } else if ((bIcon = searchIcon(e->xbutton.window)) != NULL) {
      bIcon->buttonPressEvent(&e->xbutton);
    } else if ((rMenu = searchMenu(e->xbutton.window)) != NULL) {
      rMenu->buttonPressEvent(&e->xbutton);
    } else if ((wsMan = searchWSManager(e->xbutton.window)) != NULL) {
      wsMan->buttonPressEvent(&e->xbutton);
    } else if ((rWidget = searchRWidget(e->xbutton.window)) != NULL) {
      rWidget->buttonPressEvent(&e->xbutton);
    } else if (e->xbutton.window == root && e->xbutton.button == 3) {
      int mx = e->xbutton.x_root - (rootmenu->Width() / 2);
      rootmenu->Move(((mx > 0) ? mx : 0), e->xbutton.y_root -
			 (rootmenu->titleHeight() / 2));
      
      if (! rootmenu->Visible())
	rootmenu->Show();
    }
    
    break;
  }
  
  case ButtonRelease: {
    BlackboxWindow *bWin = NULL;
    BlackboxIcon *bIcon = NULL;
    Basemenu *rMenu = NULL;
    ReconfigureWidget *rWidget = NULL;
    WorkspaceManager *wsMan = NULL;

    if ((bWin = searchWindow(e->xbutton.window)) != NULL)
      bWin->buttonReleaseEvent(&e->xbutton);
    else if ((bIcon = searchIcon(e->xbutton.window)) != NULL)
      bIcon->buttonReleaseEvent(&e->xbutton);
    else if ((rMenu = searchMenu(e->xbutton.window)) != NULL)
      rMenu->buttonReleaseEvent(&e->xbutton);
    else if ((wsMan = searchWSManager(e->xbutton.window)) != NULL)
      wsMan->buttonReleaseEvent(&e->xbutton);
    else if ((rWidget = searchRWidget(e->xbutton.window)) != NULL)
      rWidget->buttonReleaseEvent(&e->xbutton);
    
    break;
  }
  
  case ConfigureRequest: {
    BlackboxWindow *cWin = searchWindow(e->xconfigurerequest.window);
    if (cWin != NULL)
      cWin->configureRequestEvent(&e->xconfigurerequest);
    else {
      /* configure a window we haven't mapped yet */
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
    
  case MapRequest: {
    BlackboxWindow *rWin = searchWindow(e->xmaprequest.window);
    if (rWin == NULL)
	rWin = new BlackboxWindow(this, e->xmaprequest.window);
    
    rWin->mapRequestEvent(&e->xmaprequest);
    
      break;
  }
  
  case MapNotify: {
    BlackboxWindow *mWin = searchWindow(e->xmap.window);
    if (mWin != NULL)
      mWin->mapNotifyEvent(&e->xmap);
    
    break;
  }
  
  case UnmapNotify: {
    BlackboxWindow *uWin = searchWindow(e->xunmap.window);
    if (uWin != NULL)
      uWin->unmapNotifyEvent(&e->xunmap);
    
    break;
  }
  
  case DestroyNotify: {
    BlackboxWindow *dWin = searchWindow(e->xdestroywindow.window);
    if (dWin != NULL)
      dWin->destroyNotifyEvent(&e->xdestroywindow);
    
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
      if (e->xproperty.atom == XA_RESOURCE_MANAGER &&
	  e->xproperty.window == root) {
	if (resource.prompt_reconfigure) {
	  reconfWidget = new ReconfigureWidget(this);
	  reconfWidget->Show();
	} else
	  Reconfigure();
      } else {
	BlackboxWindow *pWin = searchWindow(e->xproperty.window);
	if (pWin != NULL)
	  pWin->propertyNotifyEvent(e->xproperty.atom);
      }
    }

    break;
  }
  
  case EnterNotify: {
    BlackboxWindow *eWin = NULL;
    Basemenu *eMenu = NULL;
    BlackboxIcon *eIcon = NULL;
    
    
    if ((eWin = searchWindow(e->xcrossing.window)) != NULL) {
      XGrabServer(display);
      
      XSync(display, False);
      XEvent event;
      if (!XCheckTypedWindowEvent(display, eWin->clientWindow(),
				 UnmapNotify, &event)) {
	if ((! eWin->isFocused()) && eWin->isVisible()) {
	  eWin->setInputFocus();
	}
      } else
	ProcessEvent(&event);
      
      XUngrabServer(display);
    } else if ((eMenu = searchMenu(e->xcrossing.window)) != NULL)
      eMenu->enterNotifyEvent(&e->xcrossing);
    else if ((eIcon = searchIcon(e->xcrossing.window)) != NULL)
      eIcon->enterNotifyEvent(&e->xcrossing);
    
    break;
  }
  
  case LeaveNotify: {
    Basemenu *lMenu = NULL;
    BlackboxIcon *lIcon = NULL;

    if ((lMenu = searchMenu(e->xcrossing.window)) != NULL)
      lMenu->leaveNotifyEvent(&e->xcrossing);
    if ((lIcon = searchIcon(e->xcrossing.window)) != NULL)
      lIcon->leaveNotifyEvent(&e->xcrossing);
    
    break;
  }
  
  case Expose: {
    BlackboxWindow *eWin = NULL;
    BlackboxIcon *eIcon = NULL;
    Basemenu *eMenu = NULL;
    ReconfigureWidget *rWidget = NULL;
    WorkspaceManager *wsMan = NULL;

    if ((eWin = searchWindow(e->xexpose.window)) != NULL)
      eWin->exposeEvent(&e->xexpose);
    else if ((eIcon = searchIcon(e->xexpose.window)) != NULL)
      eIcon->exposeEvent(&e->xexpose);
    else if ((eMenu = searchMenu(e->xexpose.window)) != NULL)
      eMenu->exposeEvent(&e->xexpose);
    else if ((wsMan = searchWSManager(e->xexpose.window)) != NULL)
      wsMan->exposeEvent(&e->xexpose);
    else if ((rWidget = searchRWidget(e->xexpose.window)) != NULL)
      rWidget->exposeEvent(&e->xexpose);
    
    break;
  } 
  
  case FocusIn: {
    BlackboxWindow *iWin = searchWindow(e->xfocus.window);

    if (iWin != NULL) {
      iWin->setFocusFlag(True);
      focus_window_number = iWin->windowNumber();
    }

    if ((e->xfocus.mode == NotifyNormal) &&
	(e->xfocus.detail == NotifyPointer)) {
      if (focus_window_number != -1) {
	BlackboxWindow *tmp =
	  wsManager->currentWorkspace()->window(focus_window_number);
        if (tmp)
	  if ((tmp->workspace() == wsManager->currentWorkspaceID()) &&
	      tmp->isVisible())
	    tmp->setInputFocus();
      }
    }

    break;
  }
  
  case FocusOut: {
    BlackboxWindow *oWin = searchWindow(e->xfocus.window);

    if (oWin != NULL && e->xfocus.mode == NotifyNormal)
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
	if (wsManager->currentWorkspaceID() > 0)
	  wsManager->
	    changeWorkspaceID(wsManager->currentWorkspaceID() - 1);
	else
	  wsManager->changeWorkspaceID(wsManager->count() - 1);
      } else if (XKeycodeToKeysym(display, e->xkey.keycode, 0) == XK_Right){
	if (wsManager->currentWorkspaceID() != wsManager->count() - 1)
	  wsManager->
	    changeWorkspaceID(wsManager->currentWorkspaceID() + 1);
	else
	  wsManager->changeWorkspaceID(0);
      }
    }

    break;
  }

  case KeyRelease: {
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

BlackboxWindow *Blackbox::searchWindow(Window window) {
  XEvent event;
  if (! XCheckTypedWindowEvent(display, window, DestroyNotify, &event)) {
    BlackboxWindow *win = NULL;
    LinkedListIterator<WindowSearch> it(windowSearchList);

    for (; it.current(); it++) {
      WindowSearch *tmp = it.current();
      if (tmp)
	if (tmp->window == window) {
	  win = tmp->data;
	  break;
	}
    }

    return win;
  } else
    ProcessEvent(&event);
  
  return NULL;
}


Basemenu *Blackbox::searchMenu(Window window) {
  XEvent event;

  if (! XCheckTypedWindowEvent(display, window, DestroyNotify, &event)) {
    Basemenu *menu = NULL;
    LinkedListIterator<MenuSearch> it(menuSearchList);

    for (; it.current(); it++) {
      MenuSearch *tmp = it.current();

      if (tmp)
	if (tmp->window == window) {
	  menu = tmp->data;
	  break;
	}
    }

    return menu;
  } else
    ProcessEvent(&event);

  return NULL;
}


BlackboxIcon *Blackbox::searchIcon(Window window) {
  XEvent event;

  if (! XCheckTypedWindowEvent(display, window, DestroyNotify, &event)) {
    BlackboxIcon *icon = NULL;
    LinkedListIterator<IconSearch> it(iconSearchList);

    for (; it.current(); it++) {
      IconSearch *tmp = it.current();

      if (tmp)
	if (tmp->window == window) {
	  icon = tmp->data;
	  break;
	}
    }

    return icon;
  } else
    ProcessEvent(&event);
  
  return NULL;
}


WorkspaceManager *Blackbox::searchWSManager(Window window) {
  XEvent event;

  if (! XCheckTypedWindowEvent(display, window, DestroyNotify, &event)) {
    WorkspaceManager *wsm = NULL;
    LinkedListIterator<WSManagerSearch> it(wsManagerSearchList);

    for (; it.current(); it++) {
      WSManagerSearch *tmp = it.current();

      if (tmp)
	if (tmp->window == window) {
	  wsm = tmp->data;
	  break;
	}
    }
    
    return wsm;
  } else
    ProcessEvent(&event);
  
  return NULL;
}


ReconfigureWidget *Blackbox::searchRWidget(Window window) {
  XEvent event;

  if (! XCheckTypedWindowEvent(display, window, DestroyNotify, &event)) {
    ReconfigureWidget *rWidget = NULL;
    LinkedListIterator<RWidgetSearch> it(rWidgetSearchList);

    for (; it.current(); it++) {
      RWidgetSearch *tmp = it.current();
      
      if (tmp)
	if (tmp->window == window) {
	  rWidget = tmp->data;
	  break;
	}
    }
    
    return rWidget;
  } else
    ProcessEvent(&event);
  
  return NULL;
}


void Blackbox::saveWindowSearch(Window window, BlackboxWindow *data) {
  WindowSearch *tmp = new WindowSearch;
  tmp->window = window;
  tmp->data = data;
  windowSearchList->insert(tmp);
}


void Blackbox::saveMenuSearch(Window window, Basemenu *data) {
  MenuSearch *tmp = new MenuSearch;
  tmp->window = window;
  tmp->data = data;
  menuSearchList->insert(tmp);
}


void Blackbox::saveIconSearch(Window window, BlackboxIcon *data) {
  IconSearch *tmp = new IconSearch;
  tmp->window = window;
  tmp->data = data;
  iconSearchList->insert(tmp);
}


void Blackbox::saveWSManagerSearch(Window window,
					  WorkspaceManager *data) {
  WSManagerSearch *tmp = new WSManagerSearch;
  tmp->window = window;
  tmp->data = data;
  wsManagerSearchList->insert(tmp);
}


void Blackbox::saveRWidgetSearch(Window window, ReconfigureWidget *data) {
  RWidgetSearch *tmp = new RWidgetSearch;
  tmp->window = window;
  tmp->data = data;
  rWidgetSearchList->insert(tmp);
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


void Blackbox::removeIconSearch(Window window) {
  LinkedListIterator<IconSearch> it(iconSearchList);
  for (; it.current(); it++) {
    IconSearch *tmp = it.current();

    if (tmp)
      if (tmp->window == window) {
	iconSearchList->remove(tmp);
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


void Blackbox::removeRWidgetSearch(Window window) {
  LinkedListIterator<RWidgetSearch> it(rWidgetSearchList);
  for (; it.current(); it++) {
    RWidgetSearch *tmp = it.current();

    if (tmp)
      if (tmp->window == window) {
	rWidgetSearchList->remove(tmp);
	delete tmp;
	break;
      }
  }
}


// *************************************************************************
// Exit, Shutdown and Restart methods
// *************************************************************************

void Blackbox::Dissociate(void) {
  wsManager->DissociateAll();
}


void Blackbox::Exit(void) {
  XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);
  shutdown = True;
}


void Blackbox::Restart(char *prog) {
  Dissociate();
  XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);

  if (prog)
    execlp(prog, prog, NULL);
  else
    execvp(b_argv[0], b_argv);
}


void Blackbox::Shutdown(Bool do_delete) {
  Dissociate();
  if (do_delete)
    delete this;
}


// *************************************************************************
// Session utility and maintainence
// *************************************************************************

void Blackbox::addWindow(BlackboxWindow *w) {
  wsManager->currentWorkspace()->addWindow(w);
}


void Blackbox::removeWindow(BlackboxWindow *w) {
  wsManager->workspace(w->workspace())->removeWindow(w);
}


void Blackbox::reassociateWindow(BlackboxWindow *w) {
  if (w->workspace() != wsManager->currentWorkspaceID()) {
    wsManager->workspace(w->workspace())->removeWindow(w);
    wsManager->currentWorkspace()->addWindow(w);
  }
}


void Blackbox::raiseWindow(BlackboxWindow *w) {
  if (w->workspace() == wsManager->currentWorkspaceID())
    wsManager->currentWorkspace()->raiseWindow(w);
}


void Blackbox::lowerWindow(BlackboxWindow *w) {
  if (w->workspace() == wsManager->currentWorkspaceID())
    wsManager->currentWorkspace()->lowerWindow(w);
}


// *************************************************************************
// Menu loading
// *************************************************************************

void Blackbox::InitMenu(void) {
  if (rootmenu) {
    int n = rootmenu->Count();
    for (int i = 0; i < n; i++)
      rootmenu->remove(0);
  } else
    rootmenu = new Rootmenu(this);
  
  Bool default_menu = True;

  if (resource.menuFile) {
    XrmDatabase menu_db = XrmGetFileDatabase(resource.menuFile);
    
    if (menu_db) {
      XrmValue value;
      char *value_type;
      
      if (XrmGetResource(menu_db, "blackbox.session.rootMenu.topLevelMenu",
			 "Blackbox.Session.RootMenu.TopLevelMenu", &value_type,
			 &value)) {
	default_menu = False;
	readMenuDatabase(rootmenu, &value, &menu_db);
      } else
	fprintf(stderr, "Error reading menu database file %s, please check "
		"your configuration\n", resource.menuFile);
      
      XrmDestroyDatabase(menu_db);
    }
  }

  if (default_menu) {
    rootmenu->defaultMenu();
    rootmenu->insert("xterm", B_Execute, "xterm");
    rootmenu->insert("Restart", B_Restart);
    rootmenu->insert("Exit", B_Exit);
    
    // currently unimplemented
    //    rootmenu->insert("Shutdown", B_Shutdown);
  }

  rootmenu->Update();
}


void Blackbox::readMenuDatabase(Rootmenu *menu,
				XrmValue *menu_resource,
				XrmDatabase *menu_database) {
  char *rnstring = new char[menu_resource->size + 40],
    *rcstring = new char[menu_resource->size + 40];

  XrmValue value;
  char *value_type;

  // check for menu label
  sprintf(rnstring, "blackbox.session.%s.label", menu_resource->addr);
  sprintf(rcstring, "Blackbox.Session.%s.Label", menu_resource->addr);
  if (XrmGetResource(*menu_database, rnstring, rcstring, &value_type,
		     &value)) {
    char *menulabel = new char[value.size];
    strncpy(menulabel, value.addr, value.size);
    menu->setMenuLabel(menulabel);
  }

  // now... check for menu items
  int nitems = 0;
  sprintf(rnstring, "blackbox.session.%s.totalItems", menu_resource->addr);
  sprintf(rcstring, "Blackbox.Session.%s.TotalItems", menu_resource->addr);
  if (XrmGetResource(*menu_database, rnstring, rcstring, &value_type,
		     &value)) {
    if (sscanf(value.addr, "%d", &nitems) != 1)
      nitems = 0;
  }

  // read menu items... if they exist
  if (nitems > 0) {
    if (nitems > 100) nitems = 100; // limit number of items in the menu

    for (int i = 0; i < nitems; i++) {
      Rootmenu *submenu = 0;
      char *label = 0, *exec = 0;

      // look for label
      sprintf(rnstring, "blackbox.session.%s.item%d.label",
	      menu_resource->addr, i + 1);
      sprintf(rcstring, "Blackbox.Session.%s.Item%d.Label",
	      menu_resource->addr, i + 1);
      if (XrmGetResource(*menu_database, rnstring, rcstring, &value_type,
			 &value)) {
	label = new char[value.size];
	strncpy(label, value.addr, value.size);
      }

      sprintf(rnstring, "blackbox.session.%s.item%d.exec",
	      menu_resource->addr, i + 1);
      sprintf(rcstring, "Blackbox.Session.%s.Item%d.Exec",
	      menu_resource->addr, i + 1);
      if (XrmGetResource(*menu_database, rnstring, rcstring, &value_type,
			 &value)) {
	exec = new char[value.size];
	strncpy(exec, value.addr, value.size);
      }

      if (label && exec) {
	// have label and executable string... insert the menu item
	menu->insert(label, B_Execute, exec);
      } else if (label) {
	// check the label for a builtin function... like restart, reconfig
	// or exit
	char *restart = 0;
	
	sprintf(rnstring, "blackbox.session.%s.item%d.restart",
		menu_resource->addr, i + 1);
	sprintf(rcstring, "Blackbox.Session.%s.Item%d.Restart",
		menu_resource->addr, i + 1);
	if (XrmGetResource(*menu_database, rnstring, rcstring, &value_type,
			   &value)) {
	  if (! strncasecmp(value.addr, "restart", value.size)) {
	    // builtin restart function
	    menu->insert(label, B_Restart);
	  } else {
	    // restart a different window manager
	    restart = new char[value.size];
	    strncpy(restart, value.addr, value.size);
	    menu->insert(label, B_RestartOther, restart);
	  }
	} else {
	  sprintf(rnstring, "blackbox.session.%s.item%d.reconfig",
		  menu_resource->addr, i + 1);
	  sprintf(rcstring, "Blackbox.Session.%s.Item%d.Reconfig",
		  menu_resource->addr, i + 1);
	  if (XrmGetResource(*menu_database, rnstring, rcstring, &value_type,
			     &value)) {
	    menu->insert(label, B_Reconfigure);
	  } else {
	    sprintf(rnstring, "blackbox.session.%s.item%d.exit",
		    menu_resource->addr, i + 1);
	    sprintf(rcstring, "Blackbox.Session.%s.Item%d.Exit",
		    menu_resource->addr, i + 1);
	    if (XrmGetResource(*menu_database, rnstring, rcstring,
			       &value_type, &value)) {
	      menu->insert(label, B_Exit);
	    } else {
	      // all else fails... check for a submenu
	      sprintf(rnstring, "blackbox.session.%s.item%d.submenu",
		      menu_resource->addr, i + 1);
	      sprintf(rcstring, "Blackbox.Session.%s.Item%d.submenu",
		      menu_resource->addr, i + 1);
	      if (XrmGetResource(*menu_database, rnstring, rcstring,
				 &value_type, &value)) {
		if (strncasecmp(value.addr, menu_resource->addr,
				  value.size)) {
		  submenu = new Rootmenu(this);
		  menu->insert(label, submenu);
		  readMenuDatabase(submenu, &value, menu_database);
		  submenu->Update();
		} else
		  fprintf(stderr, "%s: %d recursive menu read\n"
			  "  please check your configuration\n", value.addr,
			  i + 1);
		} else
		  fprintf(stderr, "no item action found for item%d - please "
			  "  please check your configuration\n", i + 1);
	      }
	    
	  }
	}
      } else
	fprintf(stderr, "menu %s: no defined label for item%d - skipping\n"
		"  please check your configuration\n", menu_resource->addr,
		i + 1);
    }
  } else
    fprintf(stderr, "menu %s: number of menu items not defined\n"
	    "please check your configuration\n", menu_resource->addr);
  
  delete [] rnstring;
  delete [] rcstring;
}


// *************************************************************************
// Resource loading
// *************************************************************************

void Blackbox::LoadDefaults(void) {
#define BLACKBOXAD XAPPLOADDIR##"/Blackbox.ad"
#define BLACKBOXMENUAD XLIBDIR##"/BlackboxMenu.ad"

  XGrabServer(display);
  
  resource.blackboxrc = NULL;
  char *homedir = getenv("HOME"), *rcfile = new char[strlen(homedir) + 32];
  sprintf(rcfile, "%s/.blackboxrc", homedir);

  if ((resource.blackboxrc = XrmGetFileDatabase(rcfile)) == NULL)
    resource.blackboxrc = XrmGetFileDatabase(BLACKBOXAD);

  XrmValue value;
  char *value_type;

  // load textures
  if (! (resource.texture.toolbox =
	 readDatabaseTexture("blackbox.toolboxTexture",
			     "Blackbox.ToolboxTexture")))
    resource.texture.toolbox = BImageRaised|BImageBevel2|BImageSolid;
  
  if (! (resource.texture.window =
	 readDatabaseTexture("blackbox.window.windowTexture",
			     "Blackbox.Window.WindowTexture")))
    resource.texture.window = BImageRaised|BImageBevel2|BImageSolid;

  if (! (resource.texture.button =
	 readDatabaseTexture("blackbox.window.buttonTexture",
			     "Blackbox.Window.ButtonTexture")))
    resource.texture.button = BImageRaised|BImageBevel1|BImageSolid;
  
  if (! (resource.texture.menu =
	 readDatabaseTexture("blackbox.menu.menuTexture",
			     "Blackbox.Menu.MenuTexture")))
    resource.texture.button = BImageFlat|BImageSolid;

  if (! (resource.texture.imenu =
	 readDatabaseTexture("blackbox.menu.menuItemTexture",
			     "Blackbox.Menu.MenuItemTexture")))
    resource.texture.imenu = BImageRaised|BImageBevel1|BImageSolid;

  // load colors
  if (! (resource.color.frame.pixel =
    readDatabaseColor("blackbox.session.frameColor",
		      "Blackbox.Session.FrameColor",
		      &resource.color.frame.r, &resource.color.frame.g,
		      &resource.color.frame.b)))
    resource.color.frame.pixel =
      getColor("black", &resource.color.frame.r, &resource.color.frame.g,
	       &resource.color.frame.b);
  
  if (! (resource.color.toolbox.pixel =
	 readDatabaseColor("blackbox.session.toolboxColor",
			   "Blackbox.Session.ToolboxColor",
			   &resource.color.toolbox.r,
			   &resource.color.toolbox.g,
			   &resource.color.toolbox.b)))
    resource.color.toolbox.pixel =
      getColor("grey", &resource.color.toolbox.r, &resource.color.toolbox.g,
	       &resource.color.toolbox.b);
  
  if (! (resource.color.toolbox_to.pixel =
	 readDatabaseColor("blackbox.session.toolboxToColor",
			   "Blackbox.Session.ToolboxToColor",
			   &resource.color.toolbox_to.r,
			   &resource.color.toolbox_to.g,
			   &resource.color.toolbox_to.b)))
    resource.color.toolbox_to.pixel =
      getColor("black", &resource.color.toolbox_to.r,
	       &resource.color.toolbox_to.g, &resource.color.toolbox_to.b);
  
  if (! (resource.color.itext.pixel =
	 readDatabaseColor("blackbox.session.iconTextColor",
			   "Blackbox.Session.IconTextColor",
			   &resource.color.itext.r, &resource.color.itext.g,
			   &resource.color.itext.b)))
    resource.color.itext.pixel =
      getColor("black", &resource.color.itext.r, &resource.color.itext.g,
	       &resource.color.itext.b);

  if (! (resource.color.ttext.pixel =
	 readDatabaseColor("blackbox.session.toolboxTextColor",
			   "Blackbox.Session.ToolboxTextColor",
			   &resource.color.ttext.r, &resource.color.ttext.g,
			   &resource.color.ttext.b)))
    resource.color.ttext.pixel =
      getColor("black", &resource.color.ttext.r, &resource.color.ttext.g,
	       &resource.color.ttext.b);


  // load session configuration parameters
  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.session.workspaces",
		     "Blackbox.Session.Workspaces", &value_type, &value)) {
    if (sscanf(value.addr, "%d", &resource.workspaces) != 1) {
      resource.workspaces = 1;
    }
  } else
    resource.workspaces = 1;
  
  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.session.orientation",
		     "Blackbox.Session.Orientation", &value_type, &value)) {
    if (! strcasecmp(value.addr, "lefthanded"))
      resource.orientation = B_LeftHandedUser;
    else if (! strcasecmp(value.addr, "righthanded"))
      resource.orientation = B_RightHandedUser;
  } else
    resource.orientation = B_RightHandedUser;

  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.session.reconfigurePrompt",
		     "Blackbox.Session.ReconfigurePrompt",
		     &value_type, &value)) {
    if (! strcasecmp(value.addr, "no"))
      resource.prompt_reconfigure = False;
    else
      resource.prompt_reconfigure = True;
  } else
    resource.prompt_reconfigure = True;

  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.menu.menuFile",
		     "Blackbox.Menu.MenuFile", &value_type, &value)) {
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
  
  // load client window configuration
  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.window.focusColor",
		     "Blackbox.Window.FocusColor", &value_type, &value))
    resource.color.focus.pixel =
      getColor(value.addr, &resource.color.focus.r, &resource.color.focus.g,
	       &resource.color.focus.b);
  else
    resource.color.focus.pixel =
      getColor("darkgrey", &resource.color.focus.r, &resource.color.focus.g,
	       &resource.color.focus.b);

  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.window.focusToColor",
		     "Blackbox.Window.FocusToColor", &value_type, &value))
    resource.color.focus_to.pixel =
      getColor(value.addr, &resource.color.focus_to.r,
	       &resource.color.focus_to.g, &resource.color.focus_to.b);
  else
    resource.color.focus_to.pixel =
      getColor("black", &resource.color.focus_to.r,
	       &resource.color.focus_to.g, &resource.color.focus_to.b);

  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.window.unfocusColor",
		     "Blackbox.Window.UnfocusColor", &value_type, &value))
    resource.color.unfocus.pixel =
      getColor(value.addr, &resource.color.unfocus.r,
	       &resource.color.unfocus.g, &resource.color.unfocus.b);
  else
    resource.color.unfocus.pixel =
      getColor("black", &resource.color.unfocus.r,
	       &resource.color.unfocus.g, &resource.color.unfocus.b);
  
  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.window.unfocusToColor",
		     "Blackbox.Window.UnfocusToColor", &value_type, &value))
    resource.color.unfocus_to.pixel =
      getColor(value.addr, &resource.color.unfocus_to.r,
	       &resource.color.unfocus_to.g, &resource.color.unfocus_to.b);
  else
    resource.color.unfocus_to.pixel =
      getColor("black", &resource.color.unfocus_to.r,
	       &resource.color.unfocus_to.g, &resource.color.unfocus_to.b);
  
  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.window.buttonColor",
		     "Blackbox.Window.ButtonColor", &value_type, &value))
    resource.color.button.pixel =
      getColor(value.addr, &resource.color.button.r,
	       &resource.color.button.g, &resource.color.button.b);
  else
    resource.color.button.pixel =
      getColor("grey" , &resource.color.button.r,
	       &resource.color.button.g, &resource.color.button.b);
  
  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.window.buttonToColor",
		     "Blackbox.Window.ButtonToColor", &value_type, &value))
    resource.color.button_to.pixel =
      getColor(value.addr, &resource.color.button_to.r,
	       &resource.color.button_to.g, &resource.color.button_to.b);
  else
    resource.color.button_to.pixel =
      getColor("black", &resource.color.button_to.r,
	       &resource.color.button_to.g, &resource.color.button_to.b);

  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.window.focusTextColor",
		     "Blackbox.Window.FocusTextColor", &value_type, &value))
    resource.color.ftext.pixel =
      getColor(value.addr, &resource.color.ftext.r, &resource.color.ftext.g,
	       &resource.color.ftext.b);
  else
    resource.color.ftext.pixel =
      getColor("white", &resource.color.ftext.r, &resource.color.ftext.g,
	       &resource.color.ftext.b);

  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.window.unfocusTextColor",
		     "Blackbox.Window.UnfocusTextColor", &value_type, &value))
    resource.color.utext.pixel =
      getColor(value.addr, &resource.color.utext.r, &resource.color.utext.g,
	       &resource.color.utext.b);
  else
    resource.color.utext.pixel =
      getColor("darkgrey", &resource.color.utext.r, &resource.color.utext.g,
	       &resource.color.utext.b);
  
  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.window.titleJustify",
		     "Blackbox.Window.TitleJustify", &value_type, &value)) {
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

  // load menu configuration

  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.menu.menuColor",
		     "Blackbox.Menu.MenuColor", &value_type, &value))
    resource.color.menu.pixel =
      getColor(value.addr, &resource.color.menu.r,
	       &resource.color.menu.g, &resource.color.menu.b);
  else
    resource.color.menu.pixel =
      getColor("darkgrey", &resource.color.menu.r,
	       &resource.color.menu.g, &resource.color.menu.b);
  
  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.menu.menuToColor",
		     "Blackbox.Menu.MenuToColor", &value_type, &value))
    resource.color.menu_to.pixel =
      getColor(value.addr, &resource.color.menu_to.r,
	       &resource.color.menu_to.g, &resource.color.menu_to.b);
  else
    resource.color.menu_to.pixel =
      getColor("black", &resource.color.menu_to.r,
	       &resource.color.menu_to.g, &resource.color.menu_to.b);
  
  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.menu.menuItemColor",
		     "Blackbox.Menu.MenuItemColor", &value_type, &value))
    resource.color.imenu.pixel =
      getColor(value.addr, &resource.color.imenu.r,
	       &resource.color.imenu.g, &resource.color.imenu.b);
  else
    resource.color.imenu.pixel =
      getColor("black", &resource.color.imenu.r,
	       &resource.color.imenu.g, &resource.color.imenu.b);
  
  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.menu.menuItemToColor",
		     "Blackbox.Menu.MenuItemToColor", &value_type, &value))
    resource.color.imenu_to.pixel =
      getColor(value.addr, &resource.color.imenu_to.r,
	       &resource.color.imenu_to.g, &resource.color.imenu_to.b);
  else
    resource.color.imenu_to.pixel =
      getColor("grey", &resource.color.imenu_to.r,
	       &resource.color.imenu_to.g, &resource.color.imenu_to.b);

  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.menu.menuHighlightColor",
		     "Blackbox.Menu.MenuHighlightColor", &value_type, &value))
    resource.color.hmenu.pixel =
      getColor(value.addr, &resource.color.hmenu.r,
	       &resource.color.hmenu.g, &resource.color.hmenu.b);
  else
    resource.color.hmenu.pixel =
      getColor("black", &resource.color.hmenu.r,
	       &resource.color.hmenu.g, &resource.color.hmenu.b);
  
  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.menu.menuTextColor",
		     "Blackbox.Menu.MenuTextColor", &value_type, &value))
    resource.color.mtext.pixel =
      getColor(value.addr, &resource.color.mtext.r, &resource.color.mtext.g,
	       &resource.color.mtext.b);
  else
    resource.color.mtext.pixel =
      getColor("white", &resource.color.mtext.r, &resource.color.mtext.g,
	       &resource.color.mtext.b);

  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.menu.menuItemTextColor",
		     "Blackbox.Menu.MenuItemTextColor", &value_type,
		     &value))
    resource.color.mitext.pixel =
      getColor(value.addr, &resource.color.mitext.r, &resource.color.mitext.g,
	       &resource.color.mitext.b);
  else
    resource.color.mitext.pixel =
      getColor("grey", &resource.color.mitext.r, &resource.color.mitext.g,
	       &resource.color.mitext.b);

  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.menu.menuHighlightTextColor",
		     "Blackbox.Menu.MenuHighlightTextColor", &value_type,
		     &value))
    resource.color.htext.pixel =
      getColor(value.addr, &resource.color.htext.r, &resource.color.htext.g,
	       &resource.color.htext.b);
  else
    resource.color.htext.pixel =
      getColor("white", &resource.color.htext.r, &resource.color.htext.g,
	       &resource.color.htext.b);
  
  // load font resources
  const char *defaultFont = "-*-charter-medium-r-*-*-*-120-*-*-*-*-*-*";
  if (resource.font.title) XFreeFont(display, resource.font.title);
  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.session.titleFont",
		     "Blackbox.Session.TitleFont", &value_type, &value)) {
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

  if (resource.font.menu) XFreeFont(display, resource.font.menu);
  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.session.menuFont",
		     "Blackbox.Session.MenuFont", &value_type, &value)) {
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

  if (resource.font.icon) XFreeFont(display, resource.font.icon);
  if (XrmGetResource(resource.blackboxrc,
		     "blackbox.session.iconFont",
		     "Blackbox.Session.IconFont", &value_type, &value)) {
    if ((resource.font.icon = XLoadQueryFont(display, value.addr)) == NULL) {
      fprintf(stderr,
	      " blackbox: couldn't load font '%s'\n"
	      "  ...  reverting to default font.", value.addr);
      if ((resource.font.icon = XLoadQueryFont(display, defaultFont))
	  == NULL) {
	fprintf(stderr,
		"blackbox: couldn't load default font.  please check to\n"
		"make sure the necessary font is installed '%s'\n",
		defaultFont);
	exit(2);
      }  
    }
  } else {
    if ((resource.font.icon = XLoadQueryFont(display, defaultFont)) == NULL) {
      fprintf(stderr,
	      "blackbox: couldn't load default font.  please check to\n"
	      "make sure the necessary font is installed '%s'\n", defaultFont);
      exit(2);
    }
  }

  XrmDestroyDatabase(resource.blackboxrc);
  XUngrabServer(display);
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


unsigned long Blackbox::readDatabaseColor(char *rname, char *rclass,
					  unsigned char *r, unsigned char *g,
					  unsigned char *b) {
  XrmValue value;
  char *value_type;
  unsigned long pixel = 0;

  if (XrmGetResource(resource.blackboxrc, rname, rclass, &value_type,
		     &value)) {
    pixel = getColor(value.addr, r, g, b);
  } else
    *r = *g = *b = (unsigned char) 0;
  
  return pixel;
}


// *************************************************************************
// Color lookup and allocation methods
// *************************************************************************

void Blackbox::InitColor(void) {
  if (depth == 8) {
    colors_8bpp = new XColor[125];
    int i = 0;
    for (int r = 0; r < 5; r++)
      for (int g = 0; g < 5; g++)
	for (int b = 0; b < 5; b++) {
	  colors_8bpp[i].red = (r * 0xffff) / 4;
	  colors_8bpp[i].green = (g * 0xffff) / 4;
	  colors_8bpp[i].blue = (b * 0xffff) / 4;
	  colors_8bpp[i++].flags = DoRed|DoGreen|DoBlue;
	}
    
    XGrabServer(display);
    
    Colormap colormap = DefaultColormap(display, DefaultScreen(display));
    for (i = 0; i < 125; i++)
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
  
  //  wsManager->currentWorkspace()->hideAll();
  InitMenu();
  rootmenu->Reconfigure();
  wsManager->Reconfigure();
  
  XGCValues gcv;
  gcv.foreground = getColor("white");
  gcv.function = GXxor;
  gcv.line_width = 2;
  gcv.subwindow_mode = IncludeInferiors;
  gcv.font = resource.font.title->fid;
  XChangeGC(display, opGC, GCForeground|GCFunction|GCSubwindowMode|GCFont,
	    &gcv);
  
  //  wsManager->currentWorkspace()->showAll();
  
  XUngrabServer(display);
}
