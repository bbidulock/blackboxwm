//
// session.cc for Blackbox - an X11 Window manager
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

#include "session.hh"
#include "window.hh"
#include "blackbox.hh"
#include "workspace.hh"
#include "icon.hh"

#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <string.h>
#include <sys/time.h>


static int anotherWMRunning(Display *, XErrorEvent *) {
  fprintf(stderr,
	  "blackbox: a fatal error has occurred while querying the X server\n"
	  "          make sure there is not another window manager running\n");
  exit(3);
  return(-1);
}


// *************************************************************************
// Session startup code
// *************************************************************************
//
// allocations:
// Display *display
// XFontStruct *resource.font.{menu,icon,title}
// SessionMenu *rootmenu
// GC opGC
// WorkspaceManager *ws_manager
// char *resource.menuFile
// XColors *colors_8bpp  (for 8bpp displays) %% need to free at session end %%
//
// *************************************************************************

BlackboxSession::BlackboxSession(char *display_name) {
  b1Pressed = False;
  b2Pressed = False;
  b3Pressed = False;
  shutdown = False;
  startup = True;
  focus_window_number = -1;

  rootmenu = 0;
  resource.font.menu = resource.font.icon = resource.font.title = 0;

  if ((display = XOpenDisplay(display_name)) == NULL) {
    fprintf(stderr, "connection to X server failed\n");
    exit(2);
  }
  
  screen = DefaultScreen(display);
  root = RootWindow(display, screen);
  v = DefaultVisual(display, screen);
  depth = DefaultDepth(display, screen);

#ifdef SHAPE
  shape.extensions = XShapeQueryExtension(display, &shape.event_basep,
					  &shape.error_basep);
#else
  shape.extensions = False;
#endif

  window_search_list = new llist<WindowSearch>;
  menu_search_list = new llist<MenuSearch>;
  icon_search_list = new llist<IconSearch>;
  wsmanager_search_list = new llist<WSManagerSearch>;

  _XA_WM_COLORMAP_WINDOWS = XInternAtom(display, "WM_COLORMAP_WINDOWS", False);
  _XA_WM_PROTOCOLS = XInternAtom(display, "WM_PROTOCOLS", False);
  _XA_WM_STATE = XInternAtom(display, "WM_STATE", False);
  _XA_WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
  _XA_WM_TAKE_FOCUS = XInternAtom(display, "WM_TAKE_FOCUS", False);

  event_mask = LeaveWindowMask | EnterWindowMask | PropertyChangeMask |
    SubstructureNotifyMask | PointerMotionMask | SubstructureRedirectMask |
    ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
  
  XSetErrorHandler((XErrorHandler) anotherWMRunning);
  XSelectInput(display, root, event_mask);
  XSync(display, False);
  XSetErrorHandler((XErrorHandler) NULL);

  //  XSynchronize(display, True);
  cursor.session = XCreateFontCursor(display, XC_left_ptr);
  cursor.move = XCreateFontCursor(display, XC_fleur);
  XDefineCursor(display, root, cursor.session);

  xres = WidthOfScreen(ScreenOfDisplay(display, screen));
  yres = HeightOfScreen(ScreenOfDisplay(display, screen));

  InitScreen();
}


BlackboxSession::~BlackboxSession() {
  XSelectInput(display, root, NoEventMask);

  delete ReconfigureDialog.dialog;
  XDestroyWindow(display, ReconfigureDialog.yes_button);
  XDestroyWindow(display, ReconfigureDialog.no_button);
  XDestroyWindow(display, ReconfigureDialog.text_window);
  XDestroyWindow(display, ReconfigureDialog.window);
  XFreeGC(display, ReconfigureDialog.dialogGC);

  delete [] resource.menuFile;
  delete rootmenu;
  delete ws_manager;

  delete window_search_list;
  delete icon_search_list;
  delete menu_search_list;
  delete wsmanager_search_list;

  if (resource.font.title) XFreeFont(display, resource.font.title);
  if (resource.font.menu) XFreeFont(display, resource.font.menu);
  if (resource.font.icon) XFreeFont(display, resource.font.icon);
  XFreeGC(display, opGC);
  
  XSync(display, False);
  XCloseDisplay(display);
}


void BlackboxSession::InitScreen(void) {
  InitColor();
  XrmInitialize();
  LoadDefaults();

  XGCValues gcv;
  gcv.foreground = getColor("white");
  gcv.function = GXxor;
  gcv.line_width = 2;
  gcv.subwindow_mode = IncludeInferiors;
  gcv.font = resource.font.title->fid;
  opGC = XCreateGC(display, root, GCForeground|GCFunction|GCSubwindowMode|
		   GCFont, &gcv);

  InitMenu();
  ws_manager = new WorkspaceManager(this, resource.workspaces);
  ws_manager->stackWindows(0, 0);

  createAutoConfigDialog();
  ReconfigureDialog.dialog->withdrawWindow();
  ws_manager->workspace(0)->removeWindow(ReconfigureDialog.dialog);
  ReconfigureDialog.visible = False;

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


// *************************************************************************
// Event handling/dispatching methods
// *************************************************************************

void BlackboxSession::EventLoop(void) {
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
      tv.tv_usec = 1000;
      
      select(xfd + 1, &rfds, 0, 0, &tv);
      ws_manager->checkClock();
    }
  }
    
  Dissociate();
}

void BlackboxSession::ProcessEvent(XEvent *e) {
  switch (e->type) {
  case ButtonPress: {
    BlackboxWindow *bWin = NULL;
    BlackboxIcon *bIcon = NULL;
    BlackboxMenu *bMenu = NULL;
    WorkspaceManager *wsMan = NULL;

    switch (e->xbutton.button) {
    case 1: b1Pressed = True; break;
    case 2: b2Pressed = True; break;
    case 3: b3Pressed = True; break;
    }
    
    if (e->xbutton.window == ReconfigureDialog.yes_button) {
      ReconfigureDialog.dialog->withdrawWindow();
      ws_manager->workspace(ReconfigureDialog.dialog->workspace())->
	removeWindow(ReconfigureDialog.dialog);
      ReconfigureDialog.visible = False;
      Reconfigure();
    } else if (e->xbutton.window == ReconfigureDialog.no_button) {
      ReconfigureDialog.dialog->withdrawWindow();
      ws_manager->workspace(ReconfigureDialog.dialog->workspace())->
	removeWindow(ReconfigureDialog.dialog);
      ReconfigureDialog.visible = False;
    } else if ((bWin = searchWindow(e->xbutton.window)) != NULL) {
      bWin->buttonPressEvent(&e->xbutton);
    } else if ((bIcon = searchIcon(e->xbutton.window)) != NULL) {
      bIcon->buttonPressEvent(&e->xbutton);
    } else if ((bMenu = searchMenu(e->xbutton.window)) != NULL) {
      bMenu->buttonPressEvent(&e->xbutton);
    } else if ((wsMan = searchWSManager(e->xbutton.window)) != NULL) {
      wsMan->buttonPressEvent(&e->xbutton);
    } else if (e->xbutton.window == root && e->xbutton.button == 3) {
      rootmenu->moveMenu(e->xbutton.x_root - (rootmenu->Width() / 2),
			 e->xbutton.y_root -
			 (rootmenu->titleHeight() / 2));
      
      if (! rootmenu->menuVisible())
	rootmenu->showMenu();
    }
    
    break;
  }
  
  case ButtonRelease: {
    BlackboxWindow *bWin = NULL;
    BlackboxIcon *bIcon = NULL;
    BlackboxMenu *bMenu = NULL;
    WorkspaceManager *wsMan = NULL;

    switch (e->xbutton.button) {
    case 1: b1Pressed = False; break;
    case 2: b2Pressed = False; break;
    case 3: b3Pressed = False; break;
    }

    if ((bWin = searchWindow(e->xbutton.window)) != NULL)
      bWin->buttonReleaseEvent(&e->xbutton);
    else if ((bIcon = searchIcon(e->xbutton.window)) != NULL)
      bIcon->buttonReleaseEvent(&e->xbutton);
    else if ((bMenu = searchMenu(e->xbutton.window)) != NULL)
      bMenu->buttonReleaseEvent(&e->xbutton);
    else if ((wsMan = searchWSManager(e->xbutton.window)) != NULL)
      wsMan->buttonReleaseEvent(&e->xbutton);
    
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
    BlackboxMenu *mMenu = NULL;
    if ((mWin = searchWindow(e->xmotion.window)) != NULL)
      mWin->motionNotifyEvent(&e->xmotion);
    else if ((mMenu = searchMenu(e->xmotion.window)) != NULL)
      mMenu->motionNotifyEvent(&e->xmotion);
    
    break;
  }
  
  case PropertyNotify: {
    if (e->xproperty.state != PropertyDelete) {
      if (e->xproperty.atom == XA_RESOURCE_MANAGER &&
	  e->xproperty.window == root) {
	if (resource.prompt_reconfigure) {
	  if (! ReconfigureDialog.visible) {
	    ws_manager->currentWorkspace()->
	      addWindow(ReconfigureDialog.dialog);
	    ReconfigureDialog.dialog->deiconifyWindow();
	    raiseWindow(ReconfigureDialog.dialog);
	    ReconfigureDialog.visible = True;
	  } else {
	    ws_manager->changeWorkspaceID(ReconfigureDialog.dialog->
					  workspace());
	    raiseWindow(ReconfigureDialog.dialog);
	  }
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
    BlackboxWindow *fWin = NULL;
    BlackboxIcon *fIcon = NULL;
    
    if ((fWin = searchWindow(e->xcrossing.window)) != NULL) {
      XGrabServer(display);
      
      XSync(display, False);
      XEvent event;
      if (!XCheckTypedWindowEvent(display, fWin->clientWindow(),
				 UnmapNotify, &event)) {
	if ((! fWin->isFocused()) && fWin->isVisible()) {
	  fWin->setInputFocus();
	}
      } else
	ProcessEvent(&event);
      
      XUngrabServer(display);
    } else if ((fIcon = searchIcon(e->xcrossing.window)) != NULL)
      fIcon->enterNotifyEvent(&e->xcrossing);
    
    break;
  }
  
  case LeaveNotify: {
    BlackboxIcon *lIcon = NULL;
    if ((lIcon = searchIcon(e->xcrossing.window)) != NULL)
      lIcon->leaveNotifyEvent(&e->xcrossing);
    
    break;
  }
  
  case Expose: {
    BlackboxWindow *eWin = NULL;
    BlackboxIcon *eIcon = NULL;
    BlackboxMenu *eMenu = NULL;
    WorkspaceManager *wsMan = NULL;
    if ((eWin = searchWindow(e->xexpose.window)) != NULL)
      eWin->exposeEvent(&e->xexpose);
    else if ((eIcon = searchIcon(e->xexpose.window)) != NULL)
      eIcon->exposeEvent(&e->xexpose);
    else if ((eMenu = searchMenu(e->xexpose.window)) != NULL)
      eMenu->exposeEvent(&e->xexpose);
    else if ((wsMan = searchWSManager(e->xexpose.window)) != NULL)
      wsMan->exposeEvent(&e->xexpose);
    else if (e->xexpose.window == ReconfigureDialog.text_window)
      for (int i = 0; i < 6; i++)
	XDrawString(display, ReconfigureDialog.text_window,
		    ReconfigureDialog.dialogGC, 3,
		    (ReconfigureDialog.line_h - 3) * (i + 1),
		    ReconfigureDialog.DialogText[i],
		    strlen(ReconfigureDialog.DialogText[i]));
    else if (e->xexpose.window == ReconfigureDialog.yes_button)
      XDrawString(display, ReconfigureDialog.yes_button,
		  ReconfigureDialog.dialogGC, 3, ReconfigureDialog.line_h - 3,
		  "Yes", 3);
    else if (e->xexpose.window == ReconfigureDialog.no_button)
      XDrawString(display, ReconfigureDialog.no_button,
		  ReconfigureDialog.dialogGC, 3, ReconfigureDialog.line_h - 3,
		  "No", 2);
    
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
	  ws_manager->currentWorkspace()->window(focus_window_number);
	if (tmp->isVisible())
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
	if ((ws_manager->currentWorkspace()->count() > 1) &&
	    (focus_window_number >= 0)) {
	  BlackboxWindow *next, *current =
	    ws_manager->currentWorkspace()->window(focus_window_number);
	  
	  int next_window_number, level = 0;
	  do {
	    do {
	      next_window_number =
		((focus_window_number + (++level)) <
		 ws_manager->currentWorkspace()->count())
		? focus_window_number + level : 0;
	      next =
		ws_manager->currentWorkspace()->window(next_window_number);
	    } while (next->isIconic());
	  } while ((! next->setInputFocus()) &&
		   (next_window_number != focus_window_number));
	  
	  if (next_window_number != focus_window_number) {
	    current->setFocusFlag(False);
	    ws_manager->currentWorkspace()->raiseWindow(next);
	  }
	} else if (ws_manager->currentWorkspace()->count() >= 1) {
	  ws_manager->currentWorkspace()->window(0)->setInputFocus();
	}
      }
    } else if (e->xkey.state & ControlMask) {
      if (XKeycodeToKeysym(display, e->xkey.keycode, 0) == XK_Left){
	if (ws_manager->currentWorkspaceID() > 0)
	  ws_manager->
	    changeWorkspaceID(ws_manager->currentWorkspaceID() - 1);
	else
	  ws_manager->changeWorkspaceID(ws_manager->count() - 1);
      } else if (XKeycodeToKeysym(display, e->xkey.keycode, 0) == XK_Right){
	if (ws_manager->currentWorkspaceID() != ws_manager->count() - 1)
	  ws_manager->
	    changeWorkspaceID(ws_manager->currentWorkspaceID() + 1);
	else
	  ws_manager->changeWorkspaceID(0);
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

BlackboxWindow *BlackboxSession::searchWindow(Window window) {
  XEvent event;
  if (! XCheckTypedWindowEvent(display, window, DestroyNotify, &event)) {
    BlackboxWindow *win = NULL;
    llist_iterator<WindowSearch> it(window_search_list);

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


BlackboxMenu *BlackboxSession::searchMenu(Window window) {
  XEvent event;

  if (! XCheckTypedWindowEvent(display, window, DestroyNotify, &event)) {
    BlackboxMenu *menu = NULL;
    llist_iterator<MenuSearch> it(menu_search_list);

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


BlackboxIcon *BlackboxSession::searchIcon(Window window) {
  XEvent event;

  if (! XCheckTypedWindowEvent(display, window, DestroyNotify, &event)) {
    BlackboxIcon *icon = NULL;
    llist_iterator<IconSearch> it(icon_search_list);

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


WorkspaceManager *BlackboxSession::searchWSManager(Window window) {
  XEvent event;

  if (! XCheckTypedWindowEvent(display, window, DestroyNotify, &event)) {
    WorkspaceManager *wsm = NULL;
    llist_iterator<WSManagerSearch> it(wsmanager_search_list);

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


void BlackboxSession::saveWindowSearch(Window window, BlackboxWindow *data) {
  WindowSearch *tmp = new WindowSearch;
  tmp->window = window;
  tmp->data = data;
  window_search_list->insert(tmp);
}


void BlackboxSession::saveMenuSearch(Window window, BlackboxMenu *data) {
  MenuSearch *tmp = new MenuSearch;
  tmp->window = window;
  tmp->data = data;
  menu_search_list->insert(tmp);
}


void BlackboxSession::saveIconSearch(Window window, BlackboxIcon *data) {
  IconSearch *tmp = new IconSearch;
  tmp->window = window;
  tmp->data = data;
  icon_search_list->insert(tmp);
}


void BlackboxSession::saveWSManagerSearch(Window window,
					  WorkspaceManager *data) {
  WSManagerSearch *tmp = new WSManagerSearch;
  tmp->window = window;
  tmp->data = data;
  wsmanager_search_list->insert(tmp);
}


void BlackboxSession::removeWindowSearch(Window window) {
  llist_iterator<WindowSearch> it(window_search_list);
  for (; it.current(); it++) {
    WindowSearch *tmp = it.current();

    if (tmp)
      if (tmp->window == window) {
	window_search_list->remove(tmp);
	delete tmp;
	break;
      }
  }
}


void BlackboxSession::removeMenuSearch(Window window) {
  llist_iterator<MenuSearch> it(menu_search_list);
  for (; it.current(); it++) {
    MenuSearch *tmp = it.current();

    if (tmp)
      if (tmp->window == window) {
	menu_search_list->remove(tmp);
	delete tmp;
	break;
      }
  }
}


void BlackboxSession::removeIconSearch(Window window) {
  llist_iterator<IconSearch> it(icon_search_list);
  for (; it.current(); it++) {
    IconSearch *tmp = it.current();

    if (tmp)
      if (tmp->window == window) {
	icon_search_list->remove(tmp);
	delete tmp;
	break;
      }
  }
}


void BlackboxSession::removeWSManagerSearch(Window window) {
  llist_iterator<WSManagerSearch> it(wsmanager_search_list);
  for (; it.current(); it++) {
    WSManagerSearch *tmp = it.current();

    if (tmp)
      if (tmp->window == window) {
	wsmanager_search_list->remove(tmp);
	delete tmp;
	break;
      }
  }
}


// *************************************************************************
// Exit, Shutdown and Restart methods
// *************************************************************************

void BlackboxSession::Dissociate(void) {
  ws_manager->DissociateAll();
}


void BlackboxSession::Restart(void) {
  Dissociate();
  XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);
  blackbox->Restart();
}


void BlackboxSession::Exit(void) {
  XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);
  shutdown = True;
}


// *************************************************************************
// Session utility and maintainence
// *************************************************************************

void BlackboxSession::addWindow(BlackboxWindow *w)
{ ws_manager->currentWorkspace()->addWindow(w); }


void BlackboxSession::removeWindow(BlackboxWindow *w)
{ ws_manager->workspace(w->workspace())->removeWindow(w); }


void BlackboxSession::reassociateWindow(BlackboxWindow *w) {
  if (w->workspace() != ws_manager->currentWorkspaceID()) {
    ws_manager->workspace(w->workspace())->removeWindow(w);
    ws_manager->currentWorkspace()->addWindow(w);
  }
}


void BlackboxSession::raiseWindow(BlackboxWindow *w) {
  if (w->workspace() == ws_manager->currentWorkspaceID())
    ws_manager->currentWorkspace()->raiseWindow(w);
}


void BlackboxSession::lowerWindow(BlackboxWindow *w) {
  if (w->workspace() == ws_manager->currentWorkspaceID())
    ws_manager->currentWorkspace()->lowerWindow(w);
}


// *************************************************************************
// Menu loading
// *************************************************************************

void BlackboxSession::InitMenu(void) {
  if (rootmenu) {
    int n = rootmenu->count();
    for (int i = 0; i < n; i++)
      rootmenu->remove(0);
  } else
    rootmenu = new SessionMenu(this);
  
  Bool default_menu = True;

  if (resource.menuFile) {
    XrmDatabase menu_db = XrmGetFileDatabase(resource.menuFile);
    
    if (menu_db) {
      XrmValue value;
      char *value_type;
      
      if (XrmGetResource(menu_db, "blackbox.session.rootMenu.topLevelMenu",
			 "Blackbox.Session.rootMenu.TopLevelMenu", &value_type,
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

  rootmenu->updateMenu();
}


void BlackboxSession::readMenuDatabase(SessionMenu *menu,
				       XrmValue *menu_resource,
				       XrmDatabase *menu_database) {
  char *rnstring = new char[menu_resource->size + 35],
    *rcstring = new char[menu_resource->size + 35];

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
    if (nitems > 50) nitems = 50; // limit number of items in the menu

    for (int i = 0; i < nitems; i++) {
      SessionMenu *submenu = 0;
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
	    if (XrmGetResource(*menu_database, rnstring, rcstring, &value_type,
			       &value)) {
	      menu->insert(label, B_Exit);
	    } else {
	      // all else fails... check for a submenu
	      sprintf(rnstring, "blackbox.session.%s.item%d.submenu",
		      menu_resource->addr, i + 1);
	      sprintf(rcstring, "Blackbox.Session.%s.Item%d.submenu",
		      menu_resource->addr, i + 1);
	      if (XrmGetResource(*menu_database, rnstring, rcstring,
				 &value_type, &value)) {
		submenu = new SessionMenu(this);
		menu->insert(label, submenu);
		readMenuDatabase(submenu, &value, menu_database);
		submenu->updateMenu();
	      } else
		fprintf(stderr, "no item action found for item%d - please "
			"check your configuration\n", i + 1);
	    }
	  }
	}
      } else
	fprintf(stderr, "menu %s: no defined label for item%d - skipping\n"
		"please check your configuration\n", menu_resource->addr,
		i + 1);
    }
  } else
    fprintf(stderr, "menu %s: number of menu items not defined\n"
	    "please check your configuration\n", menu_resource->addr);
}


// *************************************************************************
// Resource loading
// *************************************************************************

void BlackboxSession::LoadDefaults(void) {
#define BLACKBOXAD XAPPLOADDIR##"/Blackbox.ad"
#define BLACKBOXMENUAD XLIBDIR##"/BlackboxMenu.ad"

  XGrabServer(display);
  
  char *config_file = (char *) 0;
  XrmDatabase blackbox_database = NULL, resource_database = NULL;
  
  XTextProperty resource_manager_string;
  if (XGetTextProperty(display, root, &resource_manager_string,
		       XA_RESOURCE_MANAGER)) {
    resource_database =
      XrmGetStringDatabase((const char *) resource_manager_string.value);
    
    XrmValue value;
    char *value_type;
    if (XrmGetResource(resource_database,
		       "blackbox.session.configurationFile",
		       "Blackbox.Sessoin.ConfigurationFile",
		       &value_type, &value)) {
      int clen = strlen(value.addr);
      config_file = new char[clen + 1];
      strncpy(config_file, value.addr, clen + 1);
    }

    XrmDestroyDatabase(resource_database);
  }
  
  if (config_file)
    blackbox_database = XrmGetFileDatabase(config_file);
  else
    blackbox_database = XrmGetFileDatabase(BLACKBOXAD);

  XrmValue value;
  char *value_type;

  // load our session configuration
  if (XrmGetResource(blackbox_database,
		     "blackbox.session.toolboxTexture",
		     "Blackbox.Session.ToolboxTexture", &value_type, &value)) {
    if ((! strcasecmp(value.addr, "solid")) ||
	(! strcasecmp(value.addr, "solidraised")))
      resource.texture.toolbox = B_TextureRSolid;
    else if (! strcasecmp(value.addr, "solidsunken"))
      resource.texture.toolbox = B_TextureSSolid;
    else if (! strcasecmp(value.addr, "solidflat"))
      resource.texture.toolbox = B_TextureFSolid;
    else if ((! strcasecmp(value.addr, "dgradient")) ||
	     (! strcasecmp(value.addr, "dgradientraised")))
      resource.texture.toolbox = B_TextureRDGradient;
    else if (! strcasecmp(value.addr, "dgradientsunken"))
      resource.texture.toolbox = B_TextureSDGradient;
    else if (! strcasecmp(value.addr, "dgradientflat"))
      resource.texture.toolbox = B_TextureFDGradient;
    else if ((! strcasecmp(value.addr, "hgradient")) ||
	     (! strcasecmp(value.addr, "hgradientraised")))
      resource.texture.toolbox = B_TextureRHGradient;
    else if (! strcasecmp(value.addr, "hgradientsunken"))
      resource.texture.toolbox = B_TextureSHGradient;
    else if (! strcasecmp(value.addr, "hgradientflat"))
      resource.texture.toolbox = B_TextureFHGradient;
    else if ((! strcasecmp(value.addr, "vgradient")) ||
	     (! strcasecmp(value.addr, "vgradientraised")))
      resource.texture.toolbox = B_TextureRVGradient;
    else if (! strcasecmp(value.addr, "vgradientsunken"))
      resource.texture.toolbox = B_TextureSVGradient;
    else if (! strcasecmp(value.addr, "vgradientflat"))
      resource.texture.toolbox = B_TextureFVGradient;
    else
      resource.texture.toolbox = B_TextureRSolid;
  } else
    resource.texture.toolbox = B_TextureRSolid;

  if (XrmGetResource(blackbox_database,
		     "blackbox.session.frameColor",
		     "Blackbox.Session.FrameColor", &value_type, &value))
    resource.color.frame.pixel =
      getColor(value.addr, &resource.color.frame.r, &resource.color.frame.g,
	       &resource.color.frame.b);
  else
    resource.color.frame.pixel =
      getColor("black", &resource.color.frame.r, &resource.color.frame.g,
	       &resource.color.frame.b);

  if (XrmGetResource(blackbox_database,
		     "blackbox.session.toolboxColor",
		     "Blackbox.Session.ToolboxColor", &value_type, &value))
    resource.color.toolbox.pixel =
      getColor(value.addr, &resource.color.toolbox.r,
	       &resource.color.toolbox.g, &resource.color.toolbox.b);
  else
    resource.color.toolbox.pixel =
      getColor("grey", &resource.color.toolbox.r, &resource.color.toolbox.g,
	       &resource.color.toolbox.b);
  
  if (XrmGetResource(blackbox_database,
		     "blackbox.session.toolboxToColor",
		     "Blackbox.Session.ToolboxToColor", &value_type, &value))
    resource.color.toolbox_to.pixel =
      getColor(value.addr, &resource.color.toolbox_to.r,
	       &resource.color.toolbox_to.g, &resource.color.toolbox_to.b);
  else
    resource.color.toolbox_to.pixel =
      getColor("black", &resource.color.toolbox_to.r,
	       &resource.color.toolbox_to.g, &resource.color.toolbox_to.b);


  if (XrmGetResource(blackbox_database,
		     "blackbox.session.iconTextColor",
		     "Blackbox.Session.IconTextColor", &value_type,
		     &value))
    resource.color.itext.pixel =
      getColor(value.addr, &resource.color.itext.r, &resource.color.itext.g,
	       &resource.color.itext.b);
  else
    resource.color.itext.pixel =
      getColor("black", &resource.color.itext.r, &resource.color.itext.g,
	       &resource.color.itext.b);

  if (XrmGetResource(blackbox_database,
		     "blackbox.session.toolboxTextColor",
		     "Blackbox.Session.ToolboxTextColor", &value_type,
		     &value))
    resource.color.ttext.pixel =
      getColor(value.addr, &resource.color.ttext.r, &resource.color.ttext.g,
	       &resource.color.ttext.b);
  else
    resource.color.ttext.pixel =
      getColor("black", &resource.color.ttext.r, &resource.color.ttext.g,
	       &resource.color.ttext.b);

  if (XrmGetResource(blackbox_database,
		     "blackbox.session.workspaces",
		     "Blackbox.Session.Workspaces", &value_type, &value)) {
    if (sscanf(value.addr, "%d", &resource.workspaces) != 1) {
      resource.workspaces = 1;
    }
  } else
    resource.workspaces = 1;
  
  if (XrmGetResource(blackbox_database,
		     "blackbox.session.orientation",
		     "Blackbox.Session.Orientation", &value_type, &value)) {
    if (! strcasecmp(value.addr, "lefthanded"))
      resource.orientation = B_LeftHandedUser;
    else if (! strcasecmp(value.addr, "righthanded"))
      resource.orientation = B_RightHandedUser;
  } else
    resource.orientation = B_RightHandedUser;

  if (XrmGetResource(blackbox_database,
		     "blackbox.session.reconfigurePrompt",
		     "Blackbox.Session.ReconfigurePrompt",
		     &value_type, &value)) {
    if (! strcasecmp(value.addr, "no"))
      resource.prompt_reconfigure = False;
    else
      resource.prompt_reconfigure = True;
  } else
    resource.prompt_reconfigure = True;
  
  // load client window configuration
  if (XrmGetResource(blackbox_database,
		     "blackbox.window.windowTexture",
		     "Blackbox.Window.WindowTexture", &value_type, &value)) {
    if ((! strcasecmp(value.addr, "solid")) ||
	(! strcasecmp(value.addr, "solidraised")))
      resource.texture.window = B_TextureRSolid;
    else if (! strcasecmp(value.addr, "solidsunken"))
      resource.texture.window = B_TextureSSolid;
    else if (! strcasecmp(value.addr, "solidflat"))
      resource.texture.window = B_TextureFSolid;
    else if ((! strcasecmp(value.addr, "dgradient")) ||
	     (! strcasecmp(value.addr, "dgradientraised")))
      resource.texture.window = B_TextureRDGradient;
    else if (! strcasecmp(value.addr, "dgradientsunken"))
      resource.texture.window = B_TextureSDGradient;
    else if (! strcasecmp(value.addr, "dgradientflat"))
      resource.texture.window = B_TextureFDGradient;
    else if ((! strcasecmp(value.addr, "hgradient")) ||
	     (! strcasecmp(value.addr, "hgradientraised")))
      resource.texture.window = B_TextureRHGradient;
    else if (! strcasecmp(value.addr, "hgradientsunken"))
      resource.texture.window = B_TextureSHGradient;
    else if (! strcasecmp(value.addr, "hgradientflat"))
      resource.texture.window = B_TextureFHGradient;
    else if ((! strcasecmp(value.addr, "vgradient")) ||
	     (! strcasecmp(value.addr, "vgradientraised")))
      resource.texture.window = B_TextureRVGradient;
    else if (! strcasecmp(value.addr, "vgradientsunken"))
      resource.texture.window = B_TextureSVGradient;
    else if (! strcasecmp(value.addr, "vgradientflat"))
      resource.texture.window = B_TextureFVGradient;
    else
      resource.texture.window = B_TextureRSolid;
  } else
    resource.texture.window = B_TextureRSolid;

  if (XrmGetResource(blackbox_database,
		     "blackbox.window.buttonTexture",
		     "Blackbox.Window.ButtonTexture", &value_type, &value)) {
    if ((! strcasecmp(value.addr, "solid")) ||
	(! strcasecmp(value.addr, "solidraised")))
      resource.texture.button = B_TextureRSolid;
    else if (! strcasecmp(value.addr, "solidsunken"))
      resource.texture.button = B_TextureSSolid;
    else if (! strcasecmp(value.addr, "solidflat"))
      resource.texture.button = B_TextureFSolid;
    else if ((! strcasecmp(value.addr, "dgradient")) ||
	     (! strcasecmp(value.addr, "dgradientraised")))
      resource.texture.button = B_TextureRDGradient;
    else if (! strcasecmp(value.addr, "dgradientsunken"))
      resource.texture.button = B_TextureSDGradient;
    else if (! strcasecmp(value.addr, "dgradientflat"))
      resource.texture.button = B_TextureFDGradient;
    else if ((! strcasecmp(value.addr, "hgradient")) ||
	     (! strcasecmp(value.addr, "hgradientraised")))
      resource.texture.button = B_TextureRHGradient;
    else if (! strcasecmp(value.addr, "hgradientsunken"))
      resource.texture.button = B_TextureSHGradient;
    else if (! strcasecmp(value.addr, "hgradientflat"))
      resource.texture.button = B_TextureFHGradient;
    else if ((! strcasecmp(value.addr, "vgradient")) ||
	     (! strcasecmp(value.addr, "vgradientraised")))
      resource.texture.button = B_TextureRVGradient;
    else if (! strcasecmp(value.addr, "vgradientsunken"))
      resource.texture.button = B_TextureSVGradient;
    else if (! strcasecmp(value.addr, "vgradientflat"))
      resource.texture.button = B_TextureFVGradient;
    else
      resource.texture.button = B_TextureRSolid;
  } else
    resource.texture.button = B_TextureRSolid;

  if (XrmGetResource(blackbox_database,
		     "blackbox.window.focusColor",
		     "Blackbox.Window.FocusColor", &value_type, &value))
    resource.color.focus.pixel =
      getColor(value.addr, &resource.color.focus.r, &resource.color.focus.g,
	       &resource.color.focus.b);
  else
    resource.color.focus.pixel =
      getColor("darkgrey", &resource.color.focus.r, &resource.color.focus.g,
	       &resource.color.focus.b);

  if (XrmGetResource(blackbox_database,
		     "blackbox.window.focusToColor",
		     "Blackbox.Window.FocusToColor", &value_type, &value))
    resource.color.focus_to.pixel =
      getColor(value.addr, &resource.color.focus_to.r,
	       &resource.color.focus_to.g, &resource.color.focus_to.b);
  else
    resource.color.focus_to.pixel =
      getColor("black", &resource.color.focus_to.r,
	       &resource.color.focus_to.g, &resource.color.focus_to.b);

  if (XrmGetResource(blackbox_database,
		     "blackbox.window.unfocusColor",
		     "Blackbox.Window.UnfocusColor", &value_type, &value))
    resource.color.unfocus.pixel =
      getColor(value.addr, &resource.color.unfocus.r,
	       &resource.color.unfocus.g, &resource.color.unfocus.b);
  else
    resource.color.unfocus.pixel =
      getColor("black", &resource.color.unfocus.r,
	       &resource.color.unfocus.g, &resource.color.unfocus.b);
  
  if (XrmGetResource(blackbox_database,
		     "blackbox.window.unfocusToColor",
		     "Blackbox.Window.UnfocusToColor", &value_type, &value))
    resource.color.unfocus_to.pixel =
      getColor(value.addr, &resource.color.unfocus_to.r,
	       &resource.color.unfocus_to.g, &resource.color.unfocus_to.b);
  else
    resource.color.unfocus_to.pixel =
      getColor("black", &resource.color.unfocus_to.r,
	       &resource.color.unfocus_to.g, &resource.color.unfocus_to.b);
  
  if (XrmGetResource(blackbox_database,
		     "blackbox.window.buttonColor",
		     "Blackbox.Window.ButtonColor", &value_type, &value))
    resource.color.button.pixel =
      getColor(value.addr, &resource.color.button.r,
	       &resource.color.button.g, &resource.color.button.b);
  else
    resource.color.button.pixel =
      getColor("grey" , &resource.color.button.r,
	       &resource.color.button.g, &resource.color.button.b);
  
  if (XrmGetResource(blackbox_database,
		     "blackbox.window.buttonToColor",
		     "Blackbox.Window.ButtonToColor", &value_type, &value))
    resource.color.button_to.pixel =
      getColor(value.addr, &resource.color.button_to.r,
	       &resource.color.button_to.g, &resource.color.button_to.b);
  else
    resource.color.button_to.pixel =
      getColor("black", &resource.color.button_to.r,
	       &resource.color.button_to.g, &resource.color.button_to.b);

  if (XrmGetResource(blackbox_database,
		     "blackbox.window.focusTextColor",
		     "Blackbox.Window.FocusTextColor", &value_type, &value))
    resource.color.ftext.pixel =
      getColor(value.addr, &resource.color.ftext.r, &resource.color.ftext.g,
	       &resource.color.ftext.b);
  else
    resource.color.ftext.pixel =
      getColor("white", &resource.color.ftext.r, &resource.color.ftext.g,
	       &resource.color.ftext.b);

  if (XrmGetResource(blackbox_database,
		     "blackbox.window.unfocusTextColor",
		     "Blackbox.Window.UnfocusTextColor", &value_type, &value))
    resource.color.utext.pixel =
      getColor(value.addr, &resource.color.utext.r, &resource.color.utext.g,
	       &resource.color.utext.b);
  else
    resource.color.utext.pixel =
      getColor("darkgrey", &resource.color.utext.r, &resource.color.utext.g,
	       &resource.color.utext.b);
  
  if (XrmGetResource(blackbox_database,
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
  if (XrmGetResource(blackbox_database,
		     "blackbox.menu.menuTexture",
		     "Blackbox.Menu.MenuTexture", &value_type, &value)) {
    if ((! strcasecmp(value.addr, "solid")) ||
	(! strcasecmp(value.addr, "solidraised")))
      resource.texture.menu = B_TextureRSolid;
    else if (! strcasecmp(value.addr, "solidsunken"))
      resource.texture.menu = B_TextureSSolid;
    else if (! strcasecmp(value.addr, "solidflat"))
      resource.texture.menu = B_TextureFSolid;
    else if ((! strcasecmp(value.addr, "dgradient")) ||
	     (! strcasecmp(value.addr, "dgradientraised")))
      resource.texture.menu = B_TextureRDGradient;
    else if (! strcasecmp(value.addr, "dgradientsunken"))
      resource.texture.menu = B_TextureSDGradient;
    else if (! strcasecmp(value.addr, "dgradientflat"))
      resource.texture.menu = B_TextureFDGradient;
    else if ((! strcasecmp(value.addr, "hgradient")) ||
	     (! strcasecmp(value.addr, "hgradientraised")))
      resource.texture.menu = B_TextureRHGradient;
    else if (! strcasecmp(value.addr, "hgradientsunken"))
      resource.texture.menu = B_TextureSHGradient;
    else if (! strcasecmp(value.addr, "hgradientflat"))
      resource.texture.menu = B_TextureFHGradient;
    else if ((! strcasecmp(value.addr, "vgradient")) ||
	     (! strcasecmp(value.addr, "vgradientraised")))
      resource.texture.menu = B_TextureRVGradient;
    else if (! strcasecmp(value.addr, "vgradientsunken"))
      resource.texture.menu = B_TextureSVGradient;
    else if (! strcasecmp(value.addr, "vgradientflat"))
      resource.texture.menu = B_TextureFVGradient;
    else
      resource.texture.menu = B_TextureRSolid;
  } else
    resource.texture.menu = B_TextureRSolid;

  if (XrmGetResource(blackbox_database,
		     "blackbox.menu.menuItemPressedTexture",
		     "Blackbox.Menu.MenuItemPressedTexture", &value_type,
		     &value)) {
    if ((! strcasecmp(value.addr, "solid")) ||
	(! strcasecmp(value.addr, "solidraised")))
      resource.texture.pimenu = B_TextureRSolid;
    else if (! strcasecmp(value.addr, "solidsunken"))
      resource.texture.pimenu = B_TextureSSolid;
    else if (! strcasecmp(value.addr, "solidflat"))
      resource.texture.pimenu = B_TextureFSolid;
    else if ((! strcasecmp(value.addr, "dgradient")) ||
	     (! strcasecmp(value.addr, "dgradientraised")))
      resource.texture.pimenu = B_TextureRDGradient;
    else if (! strcasecmp(value.addr, "dgradientsunken"))
      resource.texture.pimenu = B_TextureSDGradient;
    else if (! strcasecmp(value.addr, "dgradientflat"))
      resource.texture.pimenu = B_TextureFDGradient;
    else if ((! strcasecmp(value.addr, "hgradient")) ||
	     (! strcasecmp(value.addr, "hgradientraised")))
      resource.texture.pimenu = B_TextureRHGradient;
    else if (! strcasecmp(value.addr, "hgradientsunken"))
      resource.texture.pimenu = B_TextureSHGradient;
    else if (! strcasecmp(value.addr, "hgradientflat"))
      resource.texture.pimenu = B_TextureFHGradient;
    else if ((! strcasecmp(value.addr, "vgradient")) ||
	     (! strcasecmp(value.addr, "vgradientraised")))
      resource.texture.pimenu = B_TextureRVGradient;
    else if (! strcasecmp(value.addr, "vgradientsunken"))
      resource.texture.pimenu = B_TextureSVGradient;
    else if (! strcasecmp(value.addr, "vgradientflat"))
      resource.texture.pimenu = B_TextureFVGradient;
    else
      resource.texture.pimenu = B_TextureFSolid;
  } else
    resource.texture.pimenu = B_TextureFSolid;

  if (XrmGetResource(blackbox_database,
		     "blackbox.menu.menuItemTexture",
		     "Blackbox.Menu.MenuItemTexture", &value_type, &value)) {
    if ((! strcasecmp(value.addr, "solid")) ||
	(! strcasecmp(value.addr, "solidraised")))
      resource.texture.imenu = B_TextureRSolid;
    else if (! strcasecmp(value.addr, "solidsunken"))
      resource.texture.imenu = B_TextureSSolid;
    else if (! strcasecmp(value.addr, "solidflat"))
      resource.texture.imenu = B_TextureFSolid;
    else if ((! strcasecmp(value.addr, "dgradient")) ||
	     (! strcasecmp(value.addr, "dgradientraised")))
      resource.texture.imenu = B_TextureRDGradient;
    else if (! strcasecmp(value.addr, "dgradientsunken"))
      resource.texture.imenu = B_TextureSDGradient;
    else if (! strcasecmp(value.addr, "dgradientflat"))
      resource.texture.imenu = B_TextureFDGradient;
    else if ((! strcasecmp(value.addr, "hgradient")) ||
	     (! strcasecmp(value.addr, "hgradientraised")))
      resource.texture.imenu = B_TextureRHGradient;
    else if (! strcasecmp(value.addr, "hgradientsunken"))
      resource.texture.imenu = B_TextureSHGradient;
    else if (! strcasecmp(value.addr, "hgradientflat"))
      resource.texture.imenu = B_TextureFHGradient;
    else if ((! strcasecmp(value.addr, "vgradient")) ||
	     (! strcasecmp(value.addr, "vgradientraised")))
      resource.texture.imenu = B_TextureRVGradient;
    else if (! strcasecmp(value.addr, "vgradientsunken"))
      resource.texture.imenu = B_TextureSVGradient;
    else if (! strcasecmp(value.addr, "vgradientflat"))
      resource.texture.imenu = B_TextureFVGradient;
    else
      resource.texture.imenu = B_TextureRSolid;
  } else
    resource.texture.imenu = B_TextureRSolid;

  if (XrmGetResource(blackbox_database,
		     "blackbox.menu.menuColor",
		     "Blackbox.Menu.MenuColor", &value_type, &value))
    resource.color.menu.pixel =
      getColor(value.addr, &resource.color.menu.r,
	       &resource.color.menu.g, &resource.color.menu.b);
  else
    resource.color.menu.pixel =
      getColor("darkgrey", &resource.color.menu.r,
	       &resource.color.menu.g, &resource.color.menu.b);
  
  if (XrmGetResource(blackbox_database,
		     "blackbox.menu.menuToColor",
		     "Blackbox.Menu.MenuToColor", &value_type, &value))
    resource.color.menu_to.pixel =
      getColor(value.addr, &resource.color.menu_to.r,
	       &resource.color.menu_to.g, &resource.color.menu_to.b);
  else
    resource.color.menu_to.pixel =
      getColor("black", &resource.color.menu_to.r,
	       &resource.color.menu_to.g, &resource.color.menu_to.b);
  
  if (XrmGetResource(blackbox_database,
		     "blackbox.menu.menuItemColor",
		     "Blackbox.Menu.MenuItemColor", &value_type, &value))
    resource.color.imenu.pixel =
      getColor(value.addr, &resource.color.imenu.r,
	       &resource.color.imenu.g, &resource.color.imenu.b);
  else
    resource.color.imenu.pixel =
      getColor("black", &resource.color.imenu.r,
	       &resource.color.imenu.g, &resource.color.imenu.b);
  
  if (XrmGetResource(blackbox_database,
		     "blackbox.menu.menuItemToColor",
		     "Blackbox.Menu.MenuItemToColor", &value_type, &value))
    resource.color.imenu_to.pixel =
      getColor(value.addr, &resource.color.imenu_to.r,
	       &resource.color.imenu_to.g, &resource.color.imenu_to.b);
  else
    resource.color.imenu_to.pixel =
      getColor("grey", &resource.color.imenu_to.r,
	       &resource.color.imenu_to.g, &resource.color.imenu_to.b);
  
  if (XrmGetResource(blackbox_database,
		     "blackbox.menu.menuTextColor",
		     "Blackbox.Menu.MenuTextColor", &value_type, &value))
    resource.color.mtext.pixel =
      getColor(value.addr, &resource.color.mtext.r, &resource.color.mtext.g,
	       &resource.color.mtext.b);
  else
    resource.color.mtext.pixel =
      getColor("white", &resource.color.mtext.r, &resource.color.mtext.g,
	       &resource.color.mtext.b);

  if (XrmGetResource(blackbox_database,
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

  if (XrmGetResource(blackbox_database,
		     "blackbox.menu.menuPressedTextColor",
		     "Blackbox.Menu.MenuPressedTextColor", &value_type,
		     &value))
    resource.color.ptext.pixel =
      getColor(value.addr, &resource.color.ptext.r, &resource.color.ptext.g,
	       &resource.color.ptext.b);
  else
    resource.color.ptext.pixel =
      getColor("darkgrey", &resource.color.ptext.r, &resource.color.ptext.g,
	       &resource.color.ptext.b);
  
  if (XrmGetResource(blackbox_database,
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
  
  // load font resources
  const char *defaultFont = "-*-charter-medium-r-*-*-*-120-*-*-*-*-*-*";
  if (resource.font.title) XFreeFont(display, resource.font.title);
  if (XrmGetResource(blackbox_database,
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
  if (XrmGetResource(blackbox_database,
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
  if (XrmGetResource(blackbox_database,
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

  XrmDestroyDatabase(blackbox_database);
  XUngrabServer(display);
}


void BlackboxSession::updateWorkspace(int w) {
  ws_manager->workspace(w)->updateMenu();
}


// *************************************************************************
// Color lookup and allocation methods
// *************************************************************************

void BlackboxSession::InitColor(void) {
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


unsigned long BlackboxSession::getColor(const char *colorname,
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


unsigned long BlackboxSession::getColor(const char *colorname) {
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

void BlackboxSession::Reconfigure(void) {
  reconfigure = True;
}


void BlackboxSession::do_reconfigure(void) {
  if (! ReconfigureDialog.visible) {
    XGrabServer(display);
    LoadDefaults();

    ws_manager->currentWorkspace()->hideAll();
    InitMenu();
    rootmenu->Reconfigure();

    XGCValues gcv;
    gcv.foreground = getColor("white");
    gcv.function = GXxor;
    gcv.line_width = 2;
    gcv.subwindow_mode = IncludeInferiors;
    gcv.font = resource.font.title->fid;
    XChangeGC(display, opGC, GCForeground|GCFunction|GCSubwindowMode|GCFont,
	      &gcv);
    
    ws_manager->Reconfigure();

    ReconfigureDialog.dialog->Reconfigure();
    XSetWindowBackground(display, ReconfigureDialog.window,
			 toolboxColor().pixel);
    XSetWindowBackground(display, ReconfigureDialog.text_window,
			 toolboxColor().pixel);
    XSetWindowBackground(display, ReconfigureDialog.yes_button,
			 toolboxColor().pixel);
    XSetWindowBackground(display, ReconfigureDialog.no_button,
			 toolboxColor().pixel);
    XSetWindowBorder(display, ReconfigureDialog.yes_button,
		     toolboxTextColor().pixel);
    XSetWindowBorder(display, ReconfigureDialog.no_button,
		     toolboxTextColor().pixel);
    
    XGCValues dgcv;
    dgcv.font = titleFont()->fid;
    dgcv.foreground = toolboxTextColor().pixel;
    XChangeGC(display, ReconfigureDialog.dialogGC, GCFont|GCForeground, &dgcv);
    
    ws_manager->currentWorkspace()->showAll();

    XUngrabServer(display);
  }
}


void BlackboxSession::createAutoConfigDialog(void) {
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWOverrideRedirect |CWCursor|CWEventMask; 
  
  attrib_create.background_pixmap = None;
  attrib_create.background_pixel = toolboxColor().pixel;
  attrib_create.border_pixel = toolboxTextColor().pixel;
  attrib_create.override_redirect = False;
  attrib_create.cursor = sessionCursor();
  attrib_create.event_mask = SubstructureRedirectMask|ButtonPressMask|
    ButtonReleaseMask|ButtonMotionMask|ExposureMask|EnterWindowMask;
  
  ReconfigureDialog.DialogText[0] =
    "Blackbox has the ability to perform an automatic reconfiguration, but";
  ReconfigureDialog.DialogText[1] =
    "this capability has proven unreliable in the past.  Choose \"Yes\" to";
  ReconfigureDialog.DialogText[2] =
    "allow Blackbox to reconfigure itself.  WARNING:  This can cause Blackbox";
  ReconfigureDialog.DialogText[3] =
    "to dump core and abort.  NOTE:  Previous bugs have been attributed to ";
  ReconfigureDialog.DialogText[4] =
    "over-optimization at compile time.  Choose \"No\" and you can either";
  ReconfigureDialog.DialogText[5] =
    "restart or choose the Reconfigure option from your root menu.";

  ReconfigureDialog.line_h = titleFont()->ascent + titleFont()->descent + 6;
  ReconfigureDialog.text_w = 0;
  ReconfigureDialog.text_h = ReconfigureDialog.line_h * 6;
 
  for (int i = 0; i < 6; i++) {
    int tmp =
      XTextWidth(titleFont(), ReconfigureDialog.DialogText[i],
		 strlen(ReconfigureDialog.DialogText[i])) + 6;
    ReconfigureDialog.text_w = ((ReconfigureDialog.text_w < tmp) ? tmp :
				ReconfigureDialog.text_w);
  }

  ReconfigureDialog.window =
    XCreateWindow(display, root, 200, 200, ReconfigureDialog.text_w + 10,
		  ReconfigureDialog.text_h + (ReconfigureDialog.line_h * 3),
		  0, depth, InputOutput, v, create_mask, &attrib_create);
  XStoreName(display, ReconfigureDialog.window,
	     "Perform Auto-Reconfiguration?");

  ReconfigureDialog.text_window =
    XCreateWindow(display, ReconfigureDialog.window, 5, 5,
		  ReconfigureDialog.text_w, ReconfigureDialog.text_h, 0,
		  depth, InputOutput, v, create_mask, &attrib_create);

  ReconfigureDialog.yes_button = 
    XCreateWindow(display, ReconfigureDialog.window, 5,
		  ReconfigureDialog.text_h + ReconfigureDialog.line_h,
		  (ReconfigureDialog.text_w / 2) - 10,
		  ReconfigureDialog.line_h, 1, depth, InputOutput, v,
		  create_mask, &attrib_create);

  ReconfigureDialog.no_button = 
    XCreateWindow(display, ReconfigureDialog.window,
		  (ReconfigureDialog.text_w / 2) + 10,
		  ReconfigureDialog.text_h + ReconfigureDialog.line_h,
		  (ReconfigureDialog.text_w / 2) - 10,
		  ReconfigureDialog.line_h, 1, depth, InputOutput, v,
		  create_mask, &attrib_create);

  XGCValues gcv;
  gcv.font = titleFont()->fid;
  gcv.foreground = toolboxTextColor().pixel;
  ReconfigureDialog.dialogGC = XCreateGC(display,
					 ReconfigureDialog.text_window,
					 GCFont|GCForeground, &gcv);

  ReconfigureDialog.dialog = new BlackboxWindow(this,
						ReconfigureDialog.window,
						True);

  XMapSubwindows(display, ReconfigureDialog.window);
  XMapWindow(display, ReconfigureDialog.window);
}
