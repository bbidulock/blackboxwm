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
#include "Toolbar.hh"

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

  return(False);
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

  root_menu = 0;
  resource.stylerc = 0;
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
  XFree(pmv);

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
  
  cursor.session = XCreateFontCursor(display, XC_left_ptr);
  cursor.move = XCreateFontCursor(display, XC_fleur);
  XDefineCursor(display, root, cursor.session);

  xres = WidthOfScreen(ScreenOfDisplay(display, screen));
  yres = HeightOfScreen(ScreenOfDisplay(display, screen));

  windowSearchList = new LinkedList<WindowSearch>;
  menuSearchList = new LinkedList<MenuSearch>;
  toolbarSearchList = new LinkedList<ToolbarSearch>;
  groupSearchList = new LinkedList<GroupSearch>;

  pixmapCacheList = new LinkedList<PixmapCache>;

  XrmInitialize();
  LoadRC();
  LoadStyle();
  InitColor();

  XGCValues gcv;
  gcv.foreground = getColor("white");
  gcv.function = GXxor;
  gcv.line_width = 2;
  gcv.subwindow_mode = IncludeInferiors;
  opGC = XCreateGC(display, root, GCForeground|GCFunction|GCSubwindowMode,
                   &gcv);

  gcv.foreground = blackbox->wResource()->decoration.utextColor.pixel;
  gcv.font = blackbox->titleFont()->fid;
  wunfocusGC = XCreateGC(display, root, GCForeground|GCBackground|
		       GCFont, &gcv);
  
  gcv.foreground = blackbox->wResource()->decoration.ftextColor.pixel;
  gcv.font = blackbox->titleFont()->fid;
  wfocusGC = XCreateGC(display, root, GCForeground|GCBackground|
		       GCFont, &gcv);

  gcv.foreground = blackbox->mResource()->title.textColor.pixel;
  gcv.font = blackbox->titleFont()->fid;
  mtitleGC = XCreateGC(display, root, GCForeground|GCFont, &gcv);

  gcv.foreground = blackbox->mResource()->frame.textColor.pixel;
  gcv.font = blackbox->menuFont()->fid;
  mframeGC = XCreateGC(display, root, GCForeground|GCFont, &gcv);

  gcv.foreground = blackbox->mResource()->frame.htextColor.pixel;
  mhiGC = XCreateGC(display, root, GCForeground|GCBackground|GCFont,
                      &gcv);

  gcv.foreground = blackbox->mResource()->frame.hcolor.pixel;
  gcv.arc_mode = ArcChord;
  gcv.fill_style = FillSolid;
  mhbgGC = XCreateGC(display, root, GCForeground|GCFillStyle|GCArcMode,
                    &gcv);

  InitMenu();
  tool_bar = new Toolbar(this, resource.workspaces);
  tool_bar->stackWindows(0, 0);
  root_menu->Update();

  unsigned int nchild;
  Window r, p, *children;
  XQueryTree(display, root, &r, &p, &children, &nchild);

  for (int i = 0; i < (int) nchild; ++i) {
    if (children[i] == None || (! validateWindow(children[i]))) continue;

    XWindowAttributes attrib;
    if (XGetWindowAttributes(display, children[i], &attrib))
      if (! attrib.override_redirect && attrib.map_state != IsUnmapped) {
	Atom atom;
	int foo;
	unsigned long ulfoo, nitems;
	unsigned char *state;

	if (XGetWindowProperty(display, children[i], _XA_WM_STATE, 0l, 2l,
			       False, _XA_WM_STATE, &atom, &foo, &nitems,
			       &ulfoo, &state) == Success)
	  if (state) {
	    BlackboxWindow *nWin = new BlackboxWindow(this, children[i]);
	    
	    XMapRequestEvent mre;
	    mre.window = children[i];
	    nWin->mapRequestEvent(&mre);
	  }
      }
  }

  if (! resource.sloppyFocus)
    XSetInputFocus(display, tool_bar->windowID(), RevertToParent, CurrentTime);
  
  XSynchronize(display, False);
  XSync(display, False);
  XUngrabServer(display);
}


Blackbox::~Blackbox(void) {
  XSelectInput(display, root, NoEventMask);

  delete [] resource.menuFile;
  delete [] resource.styleFile;
  delete root_menu;
  delete tool_bar;

  delete windowSearchList;
  delete menuSearchList;
  delete toolbarSearchList;
  delete groupSearchList;

  if (pixmapCacheList->count()) {
    printf("%s: pixmap cache: %u unreleased pixmaps.\n",
	   b_argv[0], pixmapCacheList->count());
    
    int i, n = pixmapCacheList->count();
    for (i = 0; i < n; i++) {
      PixmapCache *tmp = pixmapCacheList->first();
      XFreePixmap(display, tmp->pixmap);
      pixmapCacheList->remove(tmp);
      delete tmp;
    }
  }

  delete pixmapCacheList;
  
  if (resource.font.title) {
    XFreeFont(display, resource.font.title);
    resource.font.title = 0;
  }
  
  if (resource.font.menu) {
    XFreeFont(display, resource.font.menu);
    resource.font.menu = 0;
  }
  
  XFreeGC(display, opGC);
  XFreeGC(display, wfocusGC);
  XFreeGC(display, wunfocusGC);
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
	tool_bar->checkClock();
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

	tool_bar->checkClock();
	lastTime = time(NULL);
      }
    }
  }

  SaveRC();
}


void Blackbox::ProcessEvent(XEvent *e) {
  switch (e->type) {
  case ButtonPress: {
    BlackboxWindow *bWin = NULL;
    Basemenu *rMenu = NULL;
    Toolbar *tbar = NULL;
    
    if ((bWin = searchWindow(e->xbutton.window)) != NULL) {
      bWin->buttonPressEvent(&e->xbutton);
    } else if ((rMenu = searchMenu(e->xbutton.window)) != NULL) {
      rMenu->buttonPressEvent(&e->xbutton);
    } else if ((tbar = searchToolbar(e->xbutton.window)) != NULL) {
      tbar->buttonPressEvent(&e->xbutton);
    } else if (e->xbutton.window == root && e->xbutton.button == 3) {
      int mx = e->xbutton.x_root - (root_menu->Width() / 2),
	my = e->xbutton.y_root - (root_menu->titleHeight() / 2);

      if (mx < 0) mx = 0;
      if (my < 0) my = 0;
      if (mx + root_menu->Width() > xres) mx = xres - root_menu->Width() - 1;
      if (my + root_menu->Height() > yres) my = yres - root_menu->Height() - 1;
      root_menu->Move(mx, my);
      
      if (! root_menu->Visible())
	root_menu->Show();
    }
    
    break;
  }
  
  case ButtonRelease: {
    BlackboxWindow *bWin = NULL;
    Basemenu *rMenu = NULL;
    Toolbar *tbar = NULL;

    if ((bWin = searchWindow(e->xbutton.window)) != NULL)
      bWin->buttonReleaseEvent(&e->xbutton);
    else if ((rMenu = searchMenu(e->xbutton.window)) != NULL)
      rMenu->buttonReleaseEvent(&e->xbutton);
    else if ((tbar = searchToolbar(e->xbutton.window)) != NULL)
      tbar->buttonReleaseEvent(&e->xbutton);
    
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

    if ((dWin = searchWindow(e->xdestroywindow.window)) != NULL)
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
      BlackboxWindow *pWin = searchWindow(e->xproperty.window);
      
      if (pWin != NULL)
	pWin->propertyNotifyEvent(e->xproperty.atom);
    }
    
    break;
  }
  
  case EnterNotify: {
    BlackboxWindow *eWin = NULL;
    Basemenu *eMenu = NULL;
        
    if (resource.sloppyFocus &&
	(eWin = searchWindow(e->xcrossing.window)) != NULL) {
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
    Toolbar *tbar = NULL;

    if ((eWin = searchWindow(e->xexpose.window)) != NULL)
      eWin->exposeEvent(&e->xexpose);
    else if ((eMenu = searchMenu(e->xexpose.window)) != NULL)
      eMenu->exposeEvent(&e->xexpose);
    else if ((tbar = searchToolbar(e->xexpose.window)) != NULL)
      tbar->exposeEvent(&e->xexpose);
    
    break;
  } 
  
  case FocusIn: {
    BlackboxWindow *iWin = searchWindow(e->xfocus.window);

    if ((iWin != NULL) && (e->xfocus.mode != NotifyGrab) &&
	(e->xfocus.mode != NotifyUngrab)) {
      iWin->setFocusFlag(True);
      focus_window_number = iWin->windowNumber();
      tool_bar->currentWorkspace()->setFocusWindow(focus_window_number);
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
      if (resource.sloppyFocus)
	XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);
      else
	XSetInputFocus(display, tool_bar->windowID(), RevertToParent,
		       CurrentTime);

      focus_window_number = -1;
      tool_bar->currentWorkspace()->setFocusWindow(-1);
    }

    break;
  }
  
  case KeyPress: {
    if (e->xkey.state & Mod1Mask) {
      if (XKeycodeToKeysym(display, e->xkey.keycode, 0) == XK_Tab) {
	nextFocus();
      }
    } else if (e->xkey.state & ControlMask) {
      if (XKeycodeToKeysym(display, e->xkey.keycode, 0) == XK_Left){
	if (tool_bar->currentWorkspaceID() > 1)
	  tool_bar->changeWorkspaceID(tool_bar->currentWorkspaceID() - 1);
	else
	  tool_bar->changeWorkspaceID(tool_bar->count() - 1);
      } else if (XKeycodeToKeysym(display, e->xkey.keycode, 0) == XK_Right){
	if (tool_bar->currentWorkspaceID() != tool_bar->count() - 1)
	  tool_bar->
	    changeWorkspaceID(tool_bar->currentWorkspaceID() + 1);
	else
	  tool_bar->changeWorkspaceID(1);
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
  shutdown = True;
}


void Blackbox::Restart(char *prog) {
  XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);
  SaveRC();
 
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


void Blackbox::SaveRC(void) {
  XrmDatabase new_blackboxrc = 0;
  char rc_string[1024], style[1024];
  char *homedir = getenv("HOME"), *rcfile = new char[strlen(homedir) + 32];
  sprintf(rcfile, "%s/.blackboxrc", homedir);
  sprintf(style, "%s", resource.styleFile);

  LoadRC();

  // these are the dynamic resources...
  sprintf(rc_string, "session.workspaces:  %d", tool_bar->count() - 1);
  XrmPutLineResource(&new_blackboxrc, rc_string);
  sprintf(rc_string, "session.toolbarRaised:  %s",
	  ((tool_bar->Raised()) ? "True" : "False"));
  XrmPutLineResource(&new_blackboxrc, rc_string);
  sprintf(rc_string, "session.styleFile:  %s", style);
  XrmPutLineResource(&new_blackboxrc, rc_string);

  // these are static, but may not be saved in the users .blackboxrc,
  // writing these resources will allow the user to edit them at a later
  // time... but loading the defaults before saving allows us to rewrite the
  // users changes...
  sprintf(rc_string, "session.colorsPerChannel:  %d", resource.cpc8bpp);
  XrmPutLineResource(&new_blackboxrc, rc_string);
  sprintf(rc_string, "session.clockFormat:  %d",
	  ((resource.clock24hour) ? 24 : 12));
  XrmPutLineResource(&new_blackboxrc, rc_string);
  sprintf(rc_string, "session.imageDither:  %s",
          ((resource.imageDither) ? "True" : "False"));
  XrmPutLineResource(&new_blackboxrc, rc_string);
  sprintf(rc_string, "session.menuFile:  %s", resource.menuFile);
  XrmPutLineResource(&new_blackboxrc, rc_string);
  sprintf(rc_string, "session.focusModel:  %s",
	  ((resource.sloppyFocus) ? "SloppyFocus" : "ClickToFocus"));
  XrmPutLineResource(&new_blackboxrc, rc_string);

  XrmPutFileDatabase(new_blackboxrc, rcfile);
  XrmDestroyDatabase(new_blackboxrc);

  delete [] rcfile;
}


// *************************************************************************
// Session utility and maintainence
// *************************************************************************

void Blackbox::reassociateWindow(BlackboxWindow *w) {
  if (! w->isStuck() && w->workspace() != tool_bar->currentWorkspaceID()) {
    tool_bar->workspace(w->workspace())->removeWindow(w);
    tool_bar->currentWorkspace()->addWindow(w);
  }
}


void Blackbox::nextFocus(void) { 
  if ((tool_bar->currentWorkspace()->Count() > 1) &&
      (focus_window_number >= 0)) {
    BlackboxWindow *next, *current =
      tool_bar->currentWorkspace()->window(focus_window_number);
    
    int next_window_number, level = 0;
    do {
      do {
	next_window_number =
	  ((focus_window_number + (++level)) <
	   tool_bar->currentWorkspace()->Count())
	  ? focus_window_number + level : 0;
	next =
	  tool_bar->currentWorkspace()->window(next_window_number);
      } while (next->isIconic());
    } while ((! next->setInputFocus()) &&
	     (next_window_number != focus_window_number));
    
    if (next_window_number != focus_window_number) {
      current->setFocusFlag(False);
      tool_bar->currentWorkspace()->raiseWindow(next);
    }
  } else if (tool_bar->currentWorkspace()->Count() >= 1) {
    tool_bar->currentWorkspace()->window(0)->setInputFocus();
  }
}


void Blackbox::prevFocus(void) {
  if ((tool_bar->currentWorkspace()->Count() > 1) &&
      (focus_window_number >= 0)) {
    BlackboxWindow *prev, *current =
      tool_bar->currentWorkspace()->window(focus_window_number);
    
    int prev_window_number, level = 0;
    do {
      do {
	prev_window_number =
	  ((focus_window_number - (++level)) >= 0)
	  ? focus_window_number - level :
	  tool_bar->currentWorkspace()->Count() - 1;
	prev =
	  tool_bar->currentWorkspace()->window(prev_window_number);
      } while (prev->isIconic());
    } while ((! prev->setInputFocus()) &&
	     (prev_window_number != focus_window_number));
    
    if (prev_window_number != focus_window_number) {
      current->setFocusFlag(False);
      tool_bar->currentWorkspace()->raiseWindow(prev);
    }
  } else if (tool_bar->currentWorkspace()->Count() >= 1)
    tool_bar->currentWorkspace()->window(0)->setInputFocus();
}
 

// *************************************************************************
// Menu loading
// *************************************************************************

void Blackbox::InitMenu(void) {
  if (root_menu) {
    int i, n = root_menu->Count();
    for (i = 0; i < n; i++)
      root_menu->remove(0);
  } else
    root_menu = new Rootmenu(this);
  
  Bool defaultMenu = True;

  if (resource.menuFile) {
    FILE *menuFile = fopen(resource.menuFile, "r");

    if (menuFile) {
      if (! feof(menuFile)) {
	char line[1024], tmp1[1024];
	memset(line, 0, 1024);
	memset(tmp1, 0, 1024);

	while (fgets(line, 1024, menuFile) && ! feof(menuFile)) {
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
		
		char label[1024];
		if (i < ri && ri > 0) {
		  strncpy(label, line + i, ri - i);
		  *(label + (ri - i)) = '\0';
		} else
                  label[0] = '\0';
		
		root_menu->setMenuLabel(label);
		defaultMenu = parseMenuFile(menuFile, root_menu);
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
    root_menu->defaultMenu();
    root_menu->insert("xterm", B_Execute, "xterm");
    root_menu->insert("Restart", B_Restart);
    root_menu->insert("Exit", B_Exit);
  }
}


Bool Blackbox::parseMenuFile(FILE *file, Rootmenu *menu) {
  char line[512], tmp1[512], tmp2[512];

  while (! feof(file)) {
    memset(line, 0, 512);
    memset(tmp1, 0, 512);
    memset(tmp2, 0, 512);
    
    if (fgets(line, 512, file)) {
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
	    
	    char title[512];
	    if (i < ri && ri > 0) {
	      strncpy(title, line + i, ri - i);
	      *(title + (ri - i)) = '\0';
	    } else {
	      int l = strlen(label);
	      strncpy(title, label, l + 1);
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
		  fprintf(stderr, "%s: [exec] error: label(%s) == NULL || "
			  "command(%s) == NULL\n", b_argv[0], label, command);
	      } else
		fprintf(stderr, "%s: [exec] error: no command string (%s)\n",
			b_argv[0], label);
	    } else
	      fprintf(stderr, "%s: [exec] error: no label string\n",
		      b_argv[0]);
	  } else if (strstr(tmp1, "include")) {
	    for (i = 0; i < len; i++)
	      if (line[i] == '(') { i++; break; }
	    for (ri = len; ri > 0; ri--)
	      if (line[ri] == ')') break;

	    if (i < ri && ri > 0) {
	      char *newfile = new char[ri - i + 1];
	      strncpy(newfile, line + i, ri - i);
	      *(newfile + (ri - i)) = '\0';
	      
	      if (newfile) {
		FILE *submenufile = fopen(newfile, "r");
		
		if (submenufile) {
		  if (! feof(submenufile)) {
		    parseMenuFile(submenufile, menu);
		    fclose(submenufile);
		  }
		} else
		  perror(newfile);

		delete [] newfile;
	      } else
		fprintf(stderr, "%s: [include] error: newfile(%s) == NULL\n",
			b_argv[0], newfile);
	    }
	  } else if (strstr(tmp1, "style")) {
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
		char *style = new char[ri - i + 1];
		strncpy(style, line + i, ri - i);
		*(style + (ri - i)) = '\0';

		if (label && style)
		  menu->insert(label, B_SetStyle, style);
		else
		  fprintf(stderr, "%s: [style] error: label(%s) == NULL || "
			  "style(%s) == NULL\n", b_argv[0], label, style);
	      } else
		fprintf(stderr, "%s: [style] error: no style filename (%s)\n",
			b_argv[0], label);
	    } else
	      fprintf(stderr, "%s: [style] error: no label string\n",
		      b_argv[0]);	    

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

#define BLACKBOXAD XAPPLOADDIR##"/Blackbox"
#define BLACKBOXMENUAD XAPPLOADDIR##"/Blackbox-menu"
#define BLACKBOXSTYLEAD XAPPLOADDIR##"/Blackbox-style"

void Blackbox::LoadRC(void) {  
  XrmDatabase database = 0;
  char *homedir = getenv("HOME"), *rcfile = new char[strlen(homedir) + 32];
  sprintf(rcfile, "%s/.blackboxrc", homedir);

  if ((database = XrmGetFileDatabase(rcfile)) == NULL)
    database = XrmGetFileDatabase(BLACKBOXAD);

  delete [] rcfile;

  XrmValue value;
  char *value_type;
  
  if (XrmGetResource(database,
		     "session.workspaces",
		     "Session.Workspaces", &value_type, &value)) {
    if (sscanf(value.addr, "%d", &resource.workspaces) != 1) {
      resource.workspaces = 1;
    }
  } else
    resource.workspaces = 1;

  if (resource.menuFile) delete [] resource.menuFile;
  if (XrmGetResource(database,
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

  if (XrmGetResource(database,
		     "session.imageDither",
		     "Session.ImageDither", &value_type,
		     &value)) {
    if (! strncasecmp("true", value.addr, value.size))
      resource.imageDither = True;
    else
      resource.imageDither = False;
  } else
    resource.imageDither = True;

  if (XrmGetResource(database,
		     "session.colorsPerChannel",
		     "Session.ColorsPerChannel", &value_type,
		     &value)) {
    if (sscanf(value.addr, "%d", &resource.cpc8bpp) != 1) {
      resource.cpc8bpp = 5;
    } else {
      if (resource.cpc8bpp < 1) resource.cpc8bpp = 1;
      if (resource.cpc8bpp > 6) resource.cpc8bpp = 6;
    }
  } else
    resource.cpc8bpp = 5;
  
  if (XrmGetResource(database,
		     "session.clockFormat",
		     "Session.ClockFormat", &value_type,
		     &value)) {
    int clock;
    if (sscanf(value.addr, "%d", &clock) != 1)
      resource.clock24hour = False;
    else if (clock == 24)
      resource.clock24hour = True;
    else
      resource.clock24hour = False;
  } else
    resource.clock24hour = False;

  if (XrmGetResource(database, "session.toolbarRaised",
		     "Session.ToolbarRaised", &value_type, &value)) {
    if (! strncasecmp(value.addr, "true", value.size))
      resource.toolbarRaised = True;
    else
      resource.toolbarRaised = False;
  } else
    resource.toolbarRaised = False;

  if (XrmGetResource(database, "session.focusModel", "Session.FocusModel",
		     &value_type, &value)) {
    if (! strncasecmp(value.addr, "clicktofocus", value.size))
      resource.sloppyFocus = False;
    else
      resource.sloppyFocus = True;
  } else
    resource.sloppyFocus = True;

  if (resource.styleFile) delete [] resource.styleFile;
  if (XrmGetResource(database, "session.styleFile", "Session.StyleFile",
		     &value_type, &value)) {
    int len = strlen(value.addr);
    resource.styleFile = new char[len + 1];
    memset(resource.styleFile, 0, len + 1);
    strncpy(resource.styleFile, value.addr, len);
  } else {
    int len = strlen(BLACKBOXSTYLEAD);
    resource.styleFile = new char[len + 1];
    memset(resource.styleFile, 0, len + 1);
    strncpy(resource.styleFile, BLACKBOXSTYLEAD, len);
  }	     

  XrmDestroyDatabase(database);
}


void Blackbox::LoadStyle(void) {
  resource.stylerc = 0;
  if ((resource.stylerc = XrmGetFileDatabase(resource.styleFile)) == NULL)
    resource.stylerc = XrmGetFileDatabase(BLACKBOXSTYLEAD);

  XrmValue value;
  char *value_type;

  // load window config
  if (! (resource.wres.decoration.ftexture =
	 blackbox->readDatabaseTexture("window.focus",
				       "Window.Focus")))
    resource.wres.decoration.ftexture = BImageRaised|BImageSolid|BImageBevel2;
  
  if (! (resource.wres.decoration.utexture =
	 blackbox->readDatabaseTexture("window.unfocus",
				       "Window.Unfocus")))
    resource.wres.decoration.utexture = BImageRaised|BImageSolid|BImageBevel2;
  

  if (! (resource.wres.handle.texture =
	 blackbox->readDatabaseTexture("window.handle",
				       "Window.Handle")))
    resource.wres.handle.texture = BImageRaised|BImageSolid|BImageBevel2;

  if (! (resource.wres.frame.texture =
	 blackbox->readDatabaseTexture("window.frame",
				       "Window.Frame")))
    resource.wres.frame.texture = BImageRaised|BImageBevel2|BImageSolid;

  if (! (resource.wres.button.texture =
	 blackbox->readDatabaseTexture("window.button",
				       "Window.Button")))
    resource.wres.button.texture = BImageRaised|BImageBevel2|BImageSolid;
  
  if (! (resource.wres.button.ptexture =
	 blackbox->readDatabaseTexture("window.button.pressed",
				       "Window.Button.Pressed")))
    resource.wres.button.ptexture = resource.wres.button.texture |
      BImageInverted;

  if (resource.wres.button.ptexture == BImageInverted)
    resource.wres.button.ptexture = resource.wres.button.texture |
      BImageInverted;
  
  // button, focused and unfocused colors
  if (! blackbox->readDatabaseColor("window.focus.color",
				    "Window.Focus.Color",
				    &resource.wres.decoration.fcolor))
    resource.wres.decoration.fcolor.pixel =
      blackbox->getColor("darkblue", &resource.wres.decoration.fcolor.r,
			 &resource.wres.decoration.fcolor.g,
			 &resource.wres.decoration.fcolor.b);
  
  if (! blackbox->readDatabaseColor("window.unfocus.color",
				    "Window.Unfocus.Color",
				    &resource.wres.decoration.ucolor))
    resource.wres.decoration.ucolor.pixel =
      blackbox->getColor("grey", &resource.wres.decoration.ucolor.r,
			 &resource.wres.decoration.ucolor.g,
			 &resource.wres.decoration.ucolor.b);

  if ((resource.wres.decoration.ftexture & BImageGradient) ||
      (resource.wres.button.texture & BImageGradient))
    if (! blackbox->readDatabaseColor("window.focus.colorTo",
				      "Window.Focus.ColorTo",
				      &resource.wres.decoration.fcolorTo))
      resource.wres.decoration.fcolorTo.pixel =
	blackbox->getColor("black", &resource.wres.decoration.fcolorTo.r,
			   &resource.wres.decoration.fcolorTo.g,
			   &resource.wres.decoration.fcolorTo.b);

  if ((resource.wres.decoration.utexture & BImageGradient) ||
      (resource.wres.button.texture & BImageGradient))
    if (! blackbox->readDatabaseColor("window.unfocus.colorTo",
				      "Window.Unfocus.ColorTo",
				      &resource.wres.decoration.ucolorTo))
      resource.wres.decoration.ucolorTo.pixel =
	blackbox->getColor("darkgrey", &resource.wres.decoration.ucolorTo.r,
			   &resource.wres.decoration.ucolorTo.g,
			   &resource.wres.decoration.ucolorTo.b);

  if (! blackbox->readDatabaseColor("window.handle.color",
				    "Window.Handle.Color",
				    &resource.wres.handle.color))
    resource.wres.handle.color.pixel =
      blackbox->getColor("grey", &resource.wres.handle.color.r,
			 &resource.wres.handle.color.g,
			 &resource.wres.handle.color.b);

  if (resource.wres.handle.texture & BImageGradient)
    if (! blackbox->readDatabaseColor("window.handle.colorTo",
				      "Window.Handle.ColorTo",
				      &resource.wres.handle.colorTo))
      resource.wres.handle.colorTo.pixel =
	blackbox->getColor("grey", &resource.wres.handle.colorTo.r,
			   &resource.wres.handle.colorTo.g,
			   &resource.wres.handle.colorTo.b);

  if (! blackbox->readDatabaseColor("window.button.pressed.color",
				    "Window.Button.Pressed.Color",
				    &resource.wres.button.pressed))
    resource.wres.button.pressed.pixel =
      blackbox->getColor("grey", &resource.wres.button.pressed.r,
			 &resource.wres.button.pressed.g,
			 &resource.wres.button.pressed.b);

  if (resource.wres.button.ptexture & BImageGradient)
    if (! blackbox->readDatabaseColor("window.button.pressed.colorTo",
				      "Window.Button.Pressed.ColorTo",
				      &resource.wres.button.pressedTo))
      resource.wres.button.pressedTo.pixel =
	blackbox->getColor("grey", &resource.wres.button.pressedTo.r,
			   &resource.wres.button.pressedTo.g,
			   &resource.wres.button.pressedTo.b);
  
  // focused and unfocused text colors
  if (! blackbox->readDatabaseColor("window.focus.textColor",
				    "Window.Focus.TextColor",
				    &resource.wres.decoration.ftextColor))
    resource.wres.decoration.ftextColor.pixel =
      blackbox->getColor("white", &resource.wres.decoration.ftextColor.r,
			 &resource.wres.decoration.ftextColor.g,
			 &resource.wres.decoration.ftextColor.b);
  
  if (! blackbox->readDatabaseColor("window.unfocus.textColor",
				    "Window.Unfocus.TextColor",
				    &resource.wres.decoration.utextColor))
    resource.wres.decoration.utextColor.pixel =
      blackbox->getColor("black", &resource.wres.decoration.utextColor.r,
			 &resource.wres.decoration.utextColor.g,
			 &resource.wres.decoration.utextColor.b);
  
  if (! (blackbox->readDatabaseColor("window.frame.color",
				     "Window.Frame.Color",
				     &resource.wres.frame.color)))
    resource.wres.frame.color.pixel =
      blackbox->getColor("grey", &resource.wres.frame.color.r,
			 &resource.wres.frame.color.g,
			 &resource.wres.frame.color.b);

  // load menu configuration
  if (! (resource.mres.title.texture =
	 blackbox->readDatabaseTexture("menu.title", "Menu.Title")))
    resource.mres.title.texture = BImageSolid|BImageRaised|BImageBevel2;
  
  if (! blackbox->readDatabaseColor("menu.title.color",
				    "Menu.Title.Color",
				    &resource.mres.title.color))
    resource.mres.title.color.pixel =
      blackbox->getColor("darkblue", &resource.mres.title.color.r,
			 &resource.mres.title.color.g,
			 &resource.mres.title.color.b);

  if (resource.mres.title.texture & BImageGradient)
    if (! blackbox->readDatabaseColor("menu.title.colorTo",
				      "Menu.Title.ColorTo",
				      &resource.mres.title.colorTo))
      resource.mres.title.colorTo.pixel =
	blackbox->getColor("black", &resource.mres.title.colorTo.r,
			   &resource.mres.title.colorTo.g,
			   &resource.mres.title.colorTo.b);
  
  if (! blackbox->readDatabaseColor("menu.title.textColor",
				    "Menu.Title.TextColor",
				    &resource.mres.title.textColor))
    resource.mres.title.textColor.pixel =
      blackbox->getColor("white", &resource.mres.title.textColor.r,
			 &resource.mres.title.textColor.g,
			 &resource.mres.title.textColor.b);
  
  if (! (resource.mres.frame.texture =
	 blackbox->readDatabaseTexture("menu.frame", "Menu.Frame")))
    resource.mres.frame.texture = BImageSolid|BImageRaised|BImageBevel2;
  
  if (! blackbox->readDatabaseColor("menu.frame.color",
				    "Menu.Frame.Color",
				    &resource.mres.frame.color))
    resource.mres.frame.color.pixel =
      blackbox->getColor("grey", &resource.mres.frame.color.r,
			 &resource.mres.frame.color.g,
			 &resource.mres.frame.color.b);

  if (resource.mres.frame.texture & BImageGradient)
    if (! blackbox->readDatabaseColor("menu.frame.colorTo",
				      "Menu.Frame.ColorTo",
				      &resource.mres.frame.colorTo))
      resource.mres.frame.colorTo.pixel =
	blackbox->getColor("darkgrey", &resource.mres.frame.colorTo.r,
			   &resource.mres.frame.colorTo.g,
			   &resource.mres.frame.colorTo.b);

  if (! blackbox->readDatabaseColor("menu.frame.highlightColor",
				    "Menu.Frame.HighLightColor",
				    &resource.mres.frame.hcolor))
    resource.mres.frame.hcolor.pixel =
      blackbox->getColor("black", &resource.mres.frame.hcolor.r,
			 &resource.mres.frame.hcolor.g,
			 &resource.mres.frame.hcolor.b);

  if (! blackbox->readDatabaseColor("menu.frame.textColor",
				    "Menu.Frame.TextColor",
				    &resource.mres.frame.textColor))
    resource.mres.frame.textColor.pixel =
      blackbox->getColor("black", &resource.mres.frame.textColor.r,
			 &resource.mres.frame.textColor.g,
			 &resource.mres.frame.textColor.b);
  
  if (! blackbox->readDatabaseColor("menu.frame.hiTextColor",
				    "Menu.Frame.HiTextColor",
				    &resource.mres.frame.htextColor))
    resource.mres.frame.htextColor.pixel =
      blackbox->getColor("white", &resource.mres.frame.htextColor.r,
			 &resource.mres.frame.htextColor.g,
			 &resource.mres.frame.htextColor.b);

  // toolbar configuration
  if (! (resource.tres.toolbar.texture =
	 blackbox->readDatabaseTexture("toolbar", "Toolbar")))
    resource.tres.toolbar.texture = BImageSolid|BImageRaised|BImageBevel2;

  if (! blackbox->readDatabaseColor("toolbar.color",
				    "Toolbar.Color",
				    &resource.tres.toolbar.color))
    resource.tres.toolbar.color.pixel =
      blackbox->getColor("grey", &resource.tres.toolbar.color.r,
			 &resource.tres.toolbar.color.g,
			 &resource.tres.toolbar.color.b);

  if (resource.tres.toolbar.texture & BImageGradient)
    if (! blackbox->readDatabaseColor("toolbar.colorTo",
				      "Toolbar.ColorTo",
				      &resource.tres.toolbar.colorTo))
      resource.tres.toolbar.colorTo.pixel =
	blackbox->getColor("darkgrey", &resource.tres.toolbar.colorTo.r,
			   &resource.tres.toolbar.colorTo.g,
			   &resource.tres.toolbar.colorTo.b);

  if (! (resource.tres.label.texture =
	 blackbox->readDatabaseTexture("toolbar.label",
				       "Toolbar.Label")))
    resource.tres.label.texture = BImageSolid|BImageRaised|BImageBevel2;

  if (! blackbox->readDatabaseColor("toolbar.label.color",
				    "Toolbar.Label.Color",
				    &resource.tres.label.color))
    resource.tres.label.color.pixel =
      blackbox->getColor("grey", &resource.tres.label.color.r,
			 &resource.tres.label.color.g,
			 &resource.tres.label.color.b);

  if (resource.tres.label.texture & BImageGradient)
    if (! blackbox->readDatabaseColor("toolbar.label.colorTo",
				      "Toolbar.Label.ColorTo",
				      &resource.tres.label.colorTo))
      resource.tres.label.colorTo.pixel =
	blackbox->getColor("darkgrey", &resource.tres.label.colorTo.r,
			   &resource.tres.label.colorTo.g,
			   &resource.tres.label.colorTo.b);
  
  if (! (resource.tres.clock.texture =
	 blackbox->readDatabaseTexture("toolbar.clock",
				       "Toolbar.Clock")))
    resource.tres.clock.texture = BImageSolid|BImageRaised|BImageBevel2;

  if (! blackbox->readDatabaseColor("toolbar.clock.color",
				    "Toolbar.Clock.Color",
				    &resource.tres.clock.color))
    resource.tres.clock.color.pixel =
      blackbox->getColor("grey", &resource.tres.clock.color.r,
			 &resource.tres.clock.color.g,
			 &resource.tres.clock.color.b);

  if (resource.tres.clock.texture & BImageGradient)
    if (! blackbox->readDatabaseColor("toolbar.clock.colorTo",
				      "Toolbar.Clock.ColorTo",
				      &resource.tres.clock.colorTo))
      resource.tres.clock.colorTo.pixel =
	blackbox->getColor("darkgrey", &resource.tres.clock.colorTo.r,
			   &resource.tres.clock.colorTo.g,
			   &resource.tres.clock.colorTo.b);

  if (! (resource.tres.button.texture =
	 blackbox->readDatabaseTexture("toolbar.button",
				       "Toolbar.Button")))
    resource.tres.button.texture = BImageSolid|BImageRaised|BImageBevel2;

  if (! (resource.tres.button.ptexture =
	 blackbox->readDatabaseTexture("toolbar.button.pressed",
				       "Toolbar.Button.Pressed")))
    resource.tres.button.ptexture = resource.tres.button.texture |
      BImageInverted;

  if (resource.tres.button.ptexture == BImageInverted)
    resource.tres.button.ptexture = resource.tres.button.texture |
      BImageInverted;
 
  if (! blackbox->readDatabaseColor("toolbar.button.color",
				    "Toolbar.Button.Color",
				    &resource.tres.button.color))
    resource.tres.button.color.pixel =
      blackbox->getColor("grey", &resource.tres.button.color.r,
			 &resource.tres.button.color.g,
			 &resource.tres.button.color.b);

  if (! blackbox->readDatabaseColor("toolbar.button.pressed.color",
				    "Toolbar.Button.Pressed.Color",
				    &resource.tres.button.pressed))
    resource.tres.button.pressed.pixel =
      blackbox->getColor("darkgrey", &resource.tres.button.pressed.r,
			 &resource.tres.button.pressed.g,
			 &resource.tres.button.pressed.b);
  
  if (resource.tres.button.texture & BImageGradient)
    if (! blackbox->readDatabaseColor("toolbar.button.colorTo",
				      "Toolbar.Button.ColorTo",
				      &resource.tres.button.colorTo))
      resource.tres.button.colorTo.pixel =
	blackbox->getColor("darkgrey", &resource.tres.button.colorTo.r,
			   &resource.tres.button.colorTo.g,
			   &resource.tres.button.colorTo.b);

  if (resource.tres.button.ptexture & BImageGradient)
    if (! blackbox->readDatabaseColor("toolbar.button.pressed.colorTo",
				      "Toolbar.Button.Pressed.ColorTo",
				      &resource.tres.button.pressedTo))
      resource.tres.button.pressedTo.pixel =
	blackbox->getColor("grey", &resource.tres.button.pressedTo.r,
			   &resource.tres.button.pressedTo.g,
			   &resource.tres.button.pressedTo.b);

  if (! blackbox->readDatabaseColor("toolbar.textColor",
				    "Toolbar.TextColor",
				    &resource.tres.toolbar.textColor))
    resource.tres.toolbar.textColor.pixel =
      blackbox->getColor("black", &resource.tres.toolbar.textColor.r,
			 &resource.tres.toolbar.textColor.g,
			 &resource.tres.toolbar.textColor.b);

  // load border color
  if (! (readDatabaseColor("borderColor", "BorderColor",
			   &resource.borderColor)))
    resource.borderColor.pixel =
      getColor("black", &resource.borderColor.r, &resource.borderColor.g,
	       &resource.borderColor.b);

  // load border and handle widths
  if (XrmGetResource(resource.stylerc,
		     "handleWidth",
		     "HandleWidth", &value_type,
		     &value)) {
    if (sscanf(value.addr, "%u", &resource.handleWidth) != 1)
      resource.handleWidth = 8;
    else
      if (resource.handleWidth > (xres / 2) ||
          resource.handleWidth == 0)
	resource.handleWidth = 8;
  } else
    resource.handleWidth = 8;

  if (XrmGetResource(resource.stylerc,
		     "bevelWidth",
		     "BevelWidth", &value_type,
		     &value)) {
    if (sscanf(value.addr, "%u", &resource.bevelWidth) != 1)
      resource.bevelWidth = 4;
    else
      if (resource.bevelWidth > (xres / 2) || resource.bevelWidth == 0)
	resource.bevelWidth = 4;
  } else
    resource.bevelWidth = 4;

  if (XrmGetResource(resource.stylerc,
		     "titleJustify",
		     "TitleJustify", &value_type, &value)) {
    if (! strncasecmp("leftjustify", value.addr, value.size))
      resource.justify = B_LeftJustify;
    else if (! strncasecmp("rightjustify", value.addr, value.size))
      resource.justify = B_RightJustify;
    else if (! strncasecmp("centerjustify", value.addr, value.size))
      resource.justify = B_CenterJustify;
    else
      resource.justify = B_LeftJustify;
  } else
    resource.justify = B_LeftJustify;

  if (XrmGetResource(resource.stylerc,
		     "menuJustify",
		     "MenuJustify", &value_type, &value)) {
    if (! strncasecmp("leftjustify", value.addr, value.size))
      resource.menu_justify = B_LeftJustify;
    else if (! strncasecmp("rightjustify", value.addr, value.size))
      resource.menu_justify = B_RightJustify;
    else if (! strncasecmp("centerjustify", value.addr, value.size))
      resource.menu_justify = B_CenterJustify;
    else
      resource.menu_justify = B_LeftJustify;
  } else
    resource.menu_justify = B_LeftJustify;

  if (XrmGetResource(resource.stylerc,
		     "moveStyle",
		     "MoveStyle", &value_type, &value)) {
    if (! strncasecmp("opaque", value.addr, value.size))
      resource.opaqueMove = True;
    else
      resource.opaqueMove = False;
  } else
    resource.opaqueMove = False;

  const char *defaultFont = "-*-helvetica-medium-r-*-*-*-120-*-*-*-*-*-*";
  if (resource.font.title) {
    XFreeFont(display, resource.font.title);
    resource.font.title = 0;
  }

  if (XrmGetResource(resource.stylerc,
		     "titleFont",
		     "TitleFont", &value_type, &value)) {
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

  if (XrmGetResource(resource.stylerc,
		     "menuFont",
		     "MenuFont", &value_type, &value)) {
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

  XrmDestroyDatabase(resource.stylerc);
}

unsigned long Blackbox::readDatabaseTexture(char *rname, char *rclass) {
  XrmValue value;
  char *value_type;
  unsigned long texture = 0;

  if (XrmGetResource(resource.stylerc, rname, rclass, &value_type,
		     &value)) {
    if (strstr(value.addr, "Inverted")) {
      texture |= BImageInverted;
    } else {
      if (strstr(value.addr, "Solid")) {
	texture |= BImageSolid;
      } else if (strstr(value.addr, "Gradient")) {
	texture |= BImageGradient;
	
	if (strstr(value.addr, "Diagonal")) {
	  texture |= BImageDiagonal;
	} else if (strstr(value.addr, "Horizontal")) {
	  texture |= BImageHorizontal;
	} else if (strstr(value.addr, "Vertical")) {
	  texture |= BImageVertical;
	} else
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
  }
  
  return texture;
}


Bool Blackbox::readDatabaseColor(char *rname, char *rclass, BColor *color) {
  XrmValue value;
  char *value_type;

  if (XrmGetResource(resource.stylerc, rname, rclass, &value_type,
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

  LoadStyle();

  XGCValues gcv;
  gcv.foreground = getColor("white");
  gcv.function = GXxor;
  gcv.line_width = 2;
  gcv.subwindow_mode = IncludeInferiors;
  XChangeGC(display, opGC, GCForeground|GCFunction|GCSubwindowMode, &gcv);

  gcv.foreground = blackbox->wResource()->decoration.utextColor.pixel;
  gcv.font = blackbox->titleFont()->fid;
  XChangeGC(display, wunfocusGC, GCForeground|GCBackground|
            GCFont, &gcv);

  gcv.foreground = blackbox->wResource()->decoration.ftextColor.pixel;
  gcv.font = blackbox->titleFont()->fid;
  XChangeGC(display, wfocusGC, GCForeground|GCBackground|
            GCFont, &gcv);

  gcv.foreground = blackbox->mResource()->title.textColor.pixel;
  gcv.font = blackbox->titleFont()->fid;
  XChangeGC(display, mtitleGC, GCForeground|GCFont, &gcv);

  gcv.foreground = blackbox->mResource()->frame.textColor.pixel;
  gcv.font = blackbox->menuFont()->fid;
  XChangeGC(display, mframeGC, GCForeground|GCFont, &gcv);

  gcv.foreground = blackbox->mResource()->frame.htextColor.pixel;
  XChangeGC(display, mhiGC, GCForeground|GCBackground|GCFont,
                      &gcv);

  gcv.foreground = blackbox->mResource()->frame.hcolor.pixel;
  gcv.arc_mode = ArcChord;
  gcv.fill_style = FillSolid;
  XChangeGC(display, mhbgGC, GCForeground|GCFillStyle|GCArcMode, &gcv);

  InitMenu();

  root_menu->Reconfigure();
  tool_bar->Reconfigure();

  XUngrabServer(display);
}


void Blackbox::setStyle(char *filename) {
  if (resource.styleFile) delete [] resource.styleFile;

  resource.styleFile = new char[strlen(filename) + 1];
  sprintf(resource.styleFile, "%s", filename);
}


// *************************************************************************
// pixmap cache control
// *************************************************************************

Pixmap Blackbox::renderImage(unsigned int width, unsigned int height,
			     unsigned long texture, const BColor &from,
			     const BColor &to) {
  if (pixmapCacheList->count() != 0) {
    LinkedListIterator<PixmapCache> it(pixmapCacheList);

    for (; it.current(); it++) {
      if ((it.current()->width == width) &&
	  (it.current()->height == height) &&
	  (it.current()->texture == texture) &&
	  (it.current()->pixel1 == from.pixel))
	if (texture & BImageGradient) {
	  if (it.current()->pixel2 == to.pixel) {
	    it.current()->count++;
	    return it.current()->pixmap;
	  }
	} else {
	  it.current()->count++;
	  return it.current()->pixmap;
	}
    }
  }

  BImage image(this, width, height, depth);
  Pixmap pixmap = image.renderImage(texture, from, to);
  
  if (pixmap) {
    PixmapCache *tmp = new PixmapCache;

    tmp->pixmap = pixmap;
    tmp->width = width;
    tmp->height = height;
    tmp->count = 1;
    tmp->texture = texture;
    tmp->pixel1 = from.pixel;

    if (texture & BImageGradient)
      tmp->pixel2 = to.pixel;
    else
      tmp->pixel2 = 0l;

    pixmapCacheList->insert(tmp);
    return pixmap;
  }

  return None;
}



Pixmap Blackbox::renderSolidImage(unsigned int width, unsigned int height,
				  unsigned long texture, const BColor &color) {
  if (pixmapCacheList->count() != 0) {
    LinkedListIterator<PixmapCache> it(pixmapCacheList);
    
    for (; it.current(); it++) {
      if ((it.current()->width == width) &&
	  (it.current()->height == height) &&
	  (it.current()->texture == texture) &&
	  (it.current()->pixel1 == color.pixel)) {
	it.current()->count++;
	return it.current()->pixmap;
      }
    }
  }

  BImage image(this, width, height, depth);
  Pixmap pixmap = image.renderSolidImage(texture, color);
  
  if (pixmap) {
    PixmapCache *tmp = new PixmapCache;

    tmp->pixmap = pixmap;
    tmp->width = width;
    tmp->height = height;
    tmp->count = 1;
    tmp->texture = texture;
    tmp->pixel1 = color.pixel;
    tmp->pixel2 = 0l;

    pixmapCacheList->insert(tmp);
    return pixmap;
  }

  return None;
}


void Blackbox::removeImage(Pixmap pixmap) {
  if (pixmap) {
    LinkedListIterator<PixmapCache> it(pixmapCacheList);

    for (; it.current(); it++) {
      if (it.current()->pixmap == pixmap) {
	PixmapCache *tmp = it.current();
	tmp->count--;
	  
	if (! tmp->count) {	  
	  XFreePixmap(display, tmp->pixmap);
	  pixmapCacheList->remove(tmp);
	  delete tmp;
	}
	
	return;
      }
    }
  }
}

