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

#include <X11/keysym.h>

#include <string.h>
#include <stdlib.h>


static int anotherWMRunning(Display *, XErrorEvent *) {
  fprintf(stderr,
	  "blackbox: a fatal error has occurred while querying the X server\n"
	  "          make sure there is not another window manager running\n");
  exit(3);
  return(-1);
}


SessionMenu::SessionMenu(BlackboxSession *s) : BlackboxMenu(s)
{ session = s; }
int SessionMenu::insert(char *l, int f, char *e)
{ return BlackboxMenu::insert(l, f, e); }
int SessionMenu::insert(char *l, SessionMenu *s)
{ return BlackboxMenu::insert(l, s); }
void SessionMenu::showMenu(void)
{ BlackboxMenu::showMenu(); }
void SessionMenu::hideMenu(void)
{ BlackboxMenu::hideMenu(); }
void SessionMenu::moveMenu(int x, int y)
{ BlackboxMenu::moveMenu(x, y); }
void SessionMenu::updateMenu(void)
{ BlackboxMenu::updateMenu(); }
Window SessionMenu::windowID(void)
{ return BlackboxMenu::windowID(); }
void SessionMenu::itemPressed(int button, int item) {
  if (button == 1 && hasSubmenu(item)) {
    drawSubmenu(item);
    XRaiseWindow(session->control(), at(item)->Submenu()->windowID());
  }
}


void SessionMenu::titlePressed(int) { }
void SessionMenu::titleReleased(int button) {
  if (button == 3)
    if (windowID() == session->rootmenu->windowID())
      hideMenu();
}


void SessionMenu::itemReleased(int button, int index) {
  if (button == 1) {
    BlackboxMenuItem *item = at(index);
    if (item->Function()) {
      switch (item->Function()) {
      case BlackboxSession::B_Reconfigure:
	Session()->Reconfigure();
	break;

      case BlackboxSession::B_Restart:
	Session()->Restart();
	break;

      case BlackboxSession::B_RestartOther:
	blackbox->Restart(item->Exec());
	break;

      case BlackboxSession::B_Exit:
	Session()->Exit();
	break;
      }

      if (! session->rootmenu->userMoved())
	session->rootmenu->hideMenu();
    } else if (item->Exec()) {
      char *command = new char[strlen(item->Exec()) + 8];
      sprintf(command, "exec %s &", item->Exec());
      system(command);
      delete [] command;
      if (! session->rootmenu->userMoved())
	session->rootmenu->hideMenu();
    }
  }
}


void SessionMenu::drawSubmenu(int index)
{ BlackboxMenu::drawSubmenu(index); }


/*

  Resources allocated for each session:
  linked list - ilist
  Debugger - debug
  XFontStruct = resource font title, menu
  XpmAttributes - icon default attrib
  GC - opGC
  WorkspaceManager - ws_manager
  SessionMenu - rootmenu

*/

BlackboxSession::BlackboxSession(char *display_name) {
  b1Pressed = False;
  b2Pressed = False;
  b3Pressed = False;
  startup = True;

  rootmenu = 0;
  resource.font.menu = resource.font.icon = resource.font.title = 0;

  debug = new Debugger();
#ifdef DEBUG
  debug->enable();
#endif
  debug->enter("BlackboxSession::Blackbox\n");

  debug->msg("opening X server connection\n");
  if ((display = XOpenDisplay(display_name)) == NULL) {
    fprintf(stderr, "connection to X server failed\n");
    exit(2);
  }
  else
    debug->msg("opened to '%s'... beginning management\n",
	       XDisplayName(display_name));
  
  screen = DefaultScreen(display);
  root = RootWindow(display, screen);
  v = DefaultVisual(display, screen);
  depth = DefaultDepth(display, screen);

#ifdef SHAPE
  debug->msg("querying X server shape extensions\n");
  if (! (shape.extensions = XShapeQueryExtension(display, &shape.event_basep, 
						 &shape.error_basep)))
    debug->msg("server doesn't support shape extensions\n");
  else
    debug->msg("server supports shape extensions\n"
	       "   (event_basep %d, error_basep %d)\n", shape.event_basep,
	       shape.error_basep);

#else
  shape.extensions = False;
#endif

  InitScreen();
}



/*

  Resources deallocated for each session:
  linked list - ilist... the icon data is deallocated by each workspaces
      Dissociate() call

  Debugger - debug
  XFontStruct = resource font title, menu
  XpmAttributes - icon default attrib
  GC - opGC
  WorkspaceManager - ws_manager
  SessionMenu - rootmenu

*/

BlackboxSession::~BlackboxSession() {
  XSelectInput(display, root, NoEventMask);

  XFreeFont(display, resource.font.title);
  XFreeFont(display, resource.font.menu);
  XFreeFont(display, resource.font.icon);
  XFreeGC(display, opGC);
  
  delete [] resource.menuFile;
  delete rootmenu;
  delete ws_manager;
  debug->msg("closing X connection\n");
  XSync(display, 0);
  XCloseDisplay(display);
  delete debug;
}


void BlackboxSession::InitScreen(void) {
  context.workspace = XUniqueContext();
  context.window = XUniqueContext();
  context.icon = XUniqueContext();
  context.menu = XUniqueContext();

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
  XSync(display, 0);
  XSetErrorHandler((XErrorHandler) NULL);

  XModifierKeymap *defmap = XGetModifierMapping(display);
  debug->msg("shift   mod: %x %x\n", defmap->modifiermap[0],
	     defmap->modifiermap[1]);
  debug->msg("lock    mod: %x %x\n", defmap->modifiermap[2],
	     defmap->modifiermap[3]);
  debug->msg("control mod: %x %x\n", defmap->modifiermap[4],
	     defmap->modifiermap[5]);
  debug->msg("mod1    mod: %x %x\n", defmap->modifiermap[6],
	     defmap->modifiermap[7]);
  debug->msg("mod2    mod: %x %x\n", defmap->modifiermap[8],
	     defmap->modifiermap[9]);
  debug->msg("mod3    mod: %x %x\n", defmap->modifiermap[10],
	     defmap->modifiermap[11]);
  debug->msg("mod4    mod: %x %x\n", defmap->modifiermap[12],
	     defmap->modifiermap[13]);
  debug->msg("mod5    mod: %x %x\n", defmap->modifiermap[14],
	     defmap->modifiermap[15]);
  XFreeModifiermap(defmap);

  cursor.session = XCreateFontCursor(display, XC_left_ptr);
  cursor.move = XCreateFontCursor(display, XC_fleur);
  XDefineCursor(display, root, cursor.session);

  xres = WidthOfScreen(ScreenOfDisplay(display, screen));
  yres = HeightOfScreen(ScreenOfDisplay(display, screen));
    
  debug->msg("setting icon size hint\n");
  XIconSize iconsize;
  iconsize.min_width = 1;
  iconsize.min_height = 1;
  iconsize.max_width = 32;
  iconsize.max_height = 32;
  iconsize.width_inc = 1;
  iconsize.height_inc = 1;
  XSetIconSizes(display, root, &iconsize, 1);

  InitColor();
  LoadDefaults();

  XGCValues gcv;
  gcv.foreground = getColor("grey");
  gcv.function = GXxor;
  gcv.line_width = 2;
  gcv.subwindow_mode = IncludeInferiors;
  gcv.font = resource.font.title->fid;
  opGC = XCreateGC(display, root, GCForeground|GCFunction|GCSubwindowMode|
		   GCFont, &gcv);

  InitMenu();
  ws_manager = new WorkspaceManager(this, resource.workspaces);

  int i;
  unsigned int nchild;
  Window r, p, *children;
  XGrabServer(display);
  XQueryTree(display, root, &r, &p, &children, &nchild);

  debug->msg("reparenting %u existing window(s)\n", nchild);
  debug->msg("tree: %lx - %lx - %lx\n", root, r, p);

  for (i = 0; i < (int) nchild; ++i) {
    if (children[i] == None) continue;

    XWindowAttributes attrib;
    if (XGetWindowAttributes(display, children[i], &attrib)) {
      if ((! attrib.override_redirect) && (attrib.map_state != IsUnmapped)) {
	XSync(display, 0);
	BlackboxWindow *nWin = new BlackboxWindow(this, root, children[i]);
		
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
      } else
	debug->msg("do not reparent override redirect windows\n");
      
    } else
      debug->msg("couldn't get window attributes\n");
  }

  while (XPending(display)) {
    XEvent foo;
    XNextEvent(display, &foo);
  }


  ws_manager->stackWindows(NULL, 0);
  XUngrabServer(display);
}


void BlackboxSession::EventLoop(void) {
  shutdown = False;
  startup = False;

#ifdef DEBUG
  XSynchronize(display, True);
#endif

  for (; (! shutdown);) {
    XEvent e;
    XNextEvent(display, &e);

    ProcessEvent(&e);
  }

  Dissociate();
}


void BlackboxSession::InitMenu(void) {
  if (rootmenu) delete rootmenu;
  rootmenu = new SessionMenu(this);

  char *line = new char[121], *label = new char[41], *command = new char[81];
  FILE *menu_file = fopen(resource.menuFile, "r");
  if (menu_file != NULL) {
    memset(line, 0, 121);
    memset(label, 0, 41);
    memset(command, 0, 80);
    fgets(line, 120, menu_file);
    int i, ri, len = strlen(line);

    for (i = 0; i < len; ++i)
      if (line[i] == '[') { ++i; break; }
    for (ri = len; ri > 0; --ri)
      if (line[ri] == ']') break;
      
    char *c;
    if (i < ri && ri > 0) {
      c = new char[ri - i + 1];
      strncpy(c, line + i, ri - i);
      *(c + (ri - i)) = '\0';
      
      if (! strcasecmp(c, "begin")) {
	for (i = 0; i < len; ++i)
	  if (line[i] == '(') { ++i; break; }
	for (ri = len; ri > 0; --ri)
	  if (line[ri] == ')') break;
	
	char *l;
	if (i < ri && ri > 0) {
	  l = new char[ri - i + 1];
	  strncpy(l, line + i, ri - i);
	  *(l + (ri - i)) = '\0';
	} else
	  l = "(nil)";
	
	rootmenu->setMenuLabel(l);
	parseSubMenu(menu_file, rootmenu);

	if (rootmenu->count() == 0) {
	  rootmenu->insert("Restart", B_Restart);
	  rootmenu->insert("Exit", B_Exit);
	}
      } else {
	rootmenu->insert("Restart", B_Restart);
	rootmenu->insert("Exit", B_Exit);
      }
    } else {
      rootmenu->insert("Restart", B_Restart);
      rootmenu->insert("Exit", B_Exit);
    }
  } else {
    // no menu file... fall back on default
    perror(resource.menuFile);
    rootmenu->insert("Restart", B_Restart);
    rootmenu->insert("Exit", B_Exit);
  }

  delete [] command;
  delete [] line;
  delete [] label;
  rootmenu->updateMenu();
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


void BlackboxSession::ProcessEvent(XEvent *e) {
  switch (e->type) {
  case ButtonPress: {
    BlackboxWindow *bWin = NULL;
    BlackboxIcon *bIcon = NULL;
    BlackboxMenu *bMenu = NULL;
    WorkspaceManager *wsMan = NULL;

    if (e->xbutton.state == Mod1Mask && e->xbutton.button == 1) {
      if (ws_manager->currentWorkspaceID() > 0)
	ws_manager->changeWorkspaceID(ws_manager->currentWorkspaceID() - 1);
      else
	ws_manager->changeWorkspaceID(ws_manager->count() - 1);
    } else if (e->xbutton.state == Mod1Mask && e->xbutton.button == 3) {
      if (ws_manager->currentWorkspaceID() != ws_manager->count() - 1)
	ws_manager->changeWorkspaceID(ws_manager->currentWorkspaceID() + 1);
      else
	ws_manager->changeWorkspaceID(0);
    } else {
      switch (e->xbutton.button) {
      case 1: b1Pressed = True; break;
      case 2: b2Pressed = True; break;
      case 3: b3Pressed = True; break;
      }
      
      debug->msg("button press %u %lx %u (control mask %u)\n",
                 e->xbutton.button, e->xbutton.window, e->xbutton.state,
                 ControlMask);
      
      if ((bWin = getWindow(e->xbutton.window)) != NULL) {
	bWin->buttonPressEvent(&e->xbutton);
      } else if ((bIcon = getIcon(e->xbutton.window)) != NULL) {
	bIcon->buttonPressEvent(&e->xbutton);
      } else if ((bMenu = getMenu(e->xbutton.window)) != NULL) {
	bMenu->buttonPressEvent(&e->xbutton);
      } else if ((wsMan = getWSManager(e->xbutton.window)) != NULL) {
	wsMan->buttonPressEvent(&e->xbutton);
      } else if (e->xbutton.window == root && e->xbutton.button == 3) {
	if (! rootmenu->menuVisible()) {
	  rootmenu->moveMenu(e->xbutton.x_root - (rootmenu->Width() / 2),
			     e->xbutton.y_root -
			     (rootmenu->titleHeight() / 2));
	  rootmenu->showMenu();
	}
      }
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

    if ((bWin = getWindow(e->xbutton.window)) != NULL)
      bWin->buttonReleaseEvent(&e->xbutton);
    else if ((bIcon = getIcon(e->xbutton.window)) != NULL)
      bIcon->buttonReleaseEvent(&e->xbutton);
    else if ((bMenu = getMenu(e->xbutton.window)) != NULL)
      bMenu->buttonReleaseEvent(&e->xbutton);
    else if ((wsMan = getWSManager(e->xbutton.window)) != NULL)
      wsMan->buttonReleaseEvent(&e->xbutton);
    
    break;
  }
  
  case ConfigureRequest: {
    BlackboxWindow *cWin = getWindow(e->xconfigurerequest.window);
    if (cWin != NULL)
      cWin->configureRequestEvent(&e->xconfigurerequest);
    else {
      /* configure a window we haven't mapped yet */
      debug->msg("configuring unmapped window\n");
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
    BlackboxWindow *rWin = getWindow(e->xmaprequest.window);
    if (rWin == NULL)
	rWin = new BlackboxWindow(this, root, e->xmaprequest.window);
    
    rWin->mapRequestEvent(&e->xmaprequest);
    
      break;
  }
  
  case MapNotify: {
    debug->msg("map notify\n");
    BlackboxWindow *mWin = getWindow(e->xmap.window);
    if (mWin != NULL)
      mWin->mapNotifyEvent(&e->xmap);
    
    break;
  }
  
  case UnmapNotify: {
    debug->msg("unmap notify\n");
    BlackboxWindow *uWin = getWindow(e->xunmap.window);
    if (uWin != NULL)
      uWin->unmapNotifyEvent(&e->xunmap);
    
    break;
  }
  
  case DestroyNotify: {
    BlackboxWindow *dWin = getWindow(e->xdestroywindow.window);
    if (dWin != NULL)
      dWin->destroyNotifyEvent(&e->xdestroywindow);
    
    break;
  }
  
  case MotionNotify: {
    BlackboxWindow *mWin = NULL;
    BlackboxMenu *mMenu = NULL;
    if ((mWin = getWindow(e->xmotion.window)) != NULL)
      mWin->motionNotifyEvent(&e->xmotion);
    else if ((mMenu = getMenu(e->xmotion.window)) != NULL)
      mMenu->motionNotifyEvent(&e->xmotion);
    
    break;
  }
  
  case PropertyNotify: {
    char *atomname = XGetAtomName(display, e->xproperty.atom);
    debug->msg("property notify %s\n", atomname);
    XFree(atomname);

    if (e->xproperty.atom == XA_RESOURCE_MANAGER &&
	e->xproperty.window == root) {
      Reconfigure();
    } else if (e->xproperty.state != PropertyDelete) {
      BlackboxWindow *pWin = getWindow(e->xproperty.window);
      if (pWin != NULL)
        pWin->propertyNotifyEvent(e->xproperty.atom);
    }

    break;
  }
  
  case EnterNotify: {
    BlackboxWindow *fWin = NULL;
    BlackboxIcon *fIcon = NULL;
    
    if ((fWin = getWindow(e->xcrossing.window)) != NULL) {
      XGrabServer(display);
      
      XSync(display, 0);
      XEvent foo;
      if (XCheckTypedWindowEvent(display, fWin->clientWindow(),
				 UnmapNotify, &foo)) {
	debug->msg("unmap notify event enqueued for window %lx\n"
		   "processing unmap event\n",
		   fWin->clientWindow());
	ProcessEvent(&foo);
      } else if ((! fWin->isFocused()) && fWin->isVisible()) {
	fWin->setInputFocus();
      }
      
      XUngrabServer(display);
    } else if ((fIcon = getIcon(e->xcrossing.window)) != NULL)
      fIcon->enterNotifyEvent(&e->xcrossing);
    
    break;
  }
  
  case LeaveNotify: {
    BlackboxIcon *lIcon = NULL;
    if ((lIcon = getIcon(e->xcrossing.window)) != NULL)
      lIcon->leaveNotifyEvent(&e->xcrossing);
    
    break;
  }
  
  case Expose: {
    BlackboxWindow *eWin = NULL;
    BlackboxIcon *eIcon = NULL;
    BlackboxMenu *eMenu = NULL;
    WorkspaceManager *wsMan = NULL;
    if ((eWin = getWindow(e->xexpose.window)) != NULL)
      eWin->exposeEvent(&e->xexpose);
    else if ((eIcon = getIcon(e->xexpose.window)) != NULL)
      eIcon->exposeEvent(&e->xexpose);
    else if ((eMenu = getMenu(e->xexpose.window)) != NULL)
      eMenu->exposeEvent(&e->xexpose);
    else if ((wsMan = getWSManager(e->xexpose.window)) != NULL)
      wsMan->exposeEvent(&e->xexpose);
    
    break;
  } 
  
  case FocusIn: {
    BlackboxWindow *iWin = getWindow(e->xfocus.window);
    if (iWin != NULL)
      iWin->setFocusFlag(True);

    break;
  }
  
  case FocusOut: {
    BlackboxWindow *oWin = getWindow(e->xfocus.window);
    if (oWin != NULL && e->xfocus.mode == NotifyNormal)
      oWin->setFocusFlag(False);
    
    break;
  }
  
  case KeyPress: {
    printf("key press state %x keycode %x\n", e->xkey.state,
	       e->xkey.keycode);
    break;
  }

  case KeyRelease: {
    printf("key release state %x keycode %x\n", e->xkey.state,
	       e->xkey.keycode);
    break;
  }

  default:
    debug->msg("event %d: %lx\n", e->type, e->xany.window);
    break;
  }
}


BlackboxWindow *BlackboxSession::getWindow(Window window) {
  BlackboxWindow *win = NULL;
  XEvent foo;

  if (XCheckTypedWindowEvent(display, window, DestroyNotify, &foo))
    ProcessEvent(&foo);
  else if (XFindContext(display, window, context.window, (XPointer *) &win)
	   != XCNOENT)
    return win;
  
  return NULL;
}


BlackboxIcon *BlackboxSession::getIcon(Window window) {
  BlackboxIcon *icon = NULL;
  XEvent foo;
  
  if (XCheckTypedWindowEvent(display, window, DestroyNotify, &foo))
    ProcessEvent(&foo);
  else if (XFindContext(display, window, context.icon, (XPointer *) &icon)
	   != XCNOENT)
    return icon;
  
  return NULL;
}


BlackboxMenu *BlackboxSession::getMenu(Window window) {
  BlackboxMenu *menu = NULL;
  XEvent foo;
  
  if (XCheckTypedWindowEvent(display, window, DestroyNotify, &foo))
    ProcessEvent(&foo);
  else if (XFindContext(display, window, context.menu, (XPointer *) &menu)
	   != XCNOENT)
    return menu;
  
  return NULL;
}


WorkspaceManager *BlackboxSession::getWSManager(Window window) {
  WorkspaceManager *man = NULL;
  XEvent foo;
  
  if (XCheckTypedWindowEvent(display, window, DestroyNotify, &foo))
    ProcessEvent(&foo);
  else if (XFindContext(display, window, context.workspace, (XPointer *) &man)
	   != XCNOENT)
    return man;
  
  return NULL;
}  


void BlackboxSession::arrangeIcons(void)
{ ws_manager->arrangeIcons(); }
void BlackboxSession::addIcon(BlackboxIcon *i)
{ ws_manager->addIcon(i); }
void BlackboxSession::removeIcon(BlackboxIcon *i)
{ ws_manager->removeIcon(i); }


void BlackboxSession::Dissociate(void) {
  debug->msg("shutting down blackbox...\n"
	     "discontinuing management of %d workspace(s)\n",
	     ws_manager->count());
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


void BlackboxSession::LoadDefaults(void) {
  //
  // blackbox defaults...
  //

  XGrabServer(display);
  debug->msg("initializing Xrm\n");
  XrmInitialize();
  
  debug->msg("retrieving databases\n");
#define BLACKBOXAD XAPPLOADDIR##"/BlackboxAD"
  XrmDatabase blackbox_database = XrmGetDatabase(display),
    default_database = NULL, resource_database = NULL;
  
  char **rmstringlist;
  XTextProperty xtext;
  if (XGetTextProperty(display, root, &xtext, XA_RESOURCE_MANAGER)) {
    int n;
    XTextPropertyToStringList(&xtext, &rmstringlist, &n);
  
    if (rmstringlist[0] != NULL) {
      debug->msg("loading resource manager database\n");
      resource_database = XrmGetStringDatabase(rmstringlist[0]);
      XFreeStringList(rmstringlist);
    }
  }
    
  if (resource_database != NULL) {
    debug->msg("combining resource manager database\n");
    XrmCombineDatabase(resource_database, &blackbox_database, True);
  } else
    default_database = XrmGetFileDatabase(BLACKBOXAD);

  debug->msg("checking for combination of databases\n");
  if (default_database != NULL && blackbox_database == NULL) {
    debug->msg("combining databases %p %p\n", default_database,
	       blackbox_database);
    XrmCombineDatabase(default_database, &blackbox_database, False);
  }

  XrmValue value;
  char *value_type;

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
		     "blackbox.session.windowTexture",
		     "Blackbox.Session.WindowTexture", &value_type, &value)) {
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
		     "blackbox.session.focusTextColor",
		     "Blackbox.Session.FocusTextColor", &value_type, &value))
    resource.color.ftext.pixel =
      getColor(value.addr, &resource.color.ftext.r, &resource.color.ftext.g,
	       &resource.color.ftext.b);
  else
    resource.color.ftext.pixel =
      getColor("white", &resource.color.ftext.r, &resource.color.ftext.g,
	       &resource.color.ftext.b);

  if (XrmGetResource(blackbox_database,
		     "blackbox.session.unfocusTextColor",
		     "Blackbox.Session.UnfocusTextColor", &value_type, &value))
    resource.color.utext.pixel =
      getColor(value.addr, &resource.color.utext.r, &resource.color.utext.g,
	       &resource.color.utext.b);
  else
    resource.color.utext.pixel =
      getColor("darkgrey", &resource.color.utext.r, &resource.color.utext.g,
	       &resource.color.utext.b);

  if (XrmGetResource(blackbox_database,
		     "blackbox.session.menuTextColor",
		     "Blackbox.Session.MenuTextColor", &value_type, &value))
    resource.color.mtext.pixel =
      getColor(value.addr, &resource.color.mtext.r, &resource.color.mtext.g,
	       &resource.color.mtext.b);
  else
    resource.color.mtext.pixel =
      getColor("white", &resource.color.mtext.r, &resource.color.mtext.g,
	       &resource.color.mtext.b);

  if (XrmGetResource(blackbox_database,
		     "blackbox.session.menuItemTextColor",
		     "Blackbox.Session.MenuItemTextColor", &value_type,
		     &value))
    resource.color.mitext.pixel =
      getColor(value.addr, &resource.color.mitext.r, &resource.color.mitext.g,
	       &resource.color.mitext.b);
  else
    resource.color.mitext.pixel =
      getColor("grey", &resource.color.mitext.r, &resource.color.mitext.g,
	       &resource.color.mitext.b);

  if (XrmGetResource(blackbox_database,
		     "blackbox.session.menuPressedTextColor",
		     "Blackbox.Session.MenuPressedTextColor", &value_type,
		     &value))
    resource.color.ptext.pixel =
      getColor(value.addr, &resource.color.ptext.r, &resource.color.ptext.g,
	       &resource.color.ptext.b);
  else
    resource.color.ptext.pixel =
      getColor("darkgrey", &resource.color.ptext.r, &resource.color.ptext.g,
	       &resource.color.ptext.b);

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
		     "blackbox.session.menuFile",
		     "Blackbox.Session.MenuFile", &value_type, &value)) {
    int len = strlen(value.addr);
    resource.menuFile = new char[len + 1];
    memset(resource.menuFile, 0, len + 1);
    strncpy(resource.menuFile, value.addr, len);
  } else {
#define BLACKBOXMENUAD XLIBDIR##"/Blackbox/MenuAD"
    int len = strlen(BLACKBOXMENUAD);
    resource.menuFile = new char[len + 1];
    memset(resource.menuFile, 0, len + 1);
    strncpy(resource.menuFile, BLACKBOXMENUAD, len);
  }
  
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
    debug->msg("loading default font\n");
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
    debug->msg("loading default font\n");
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
    debug->msg("loading default font\n");
    if ((resource.font.icon = XLoadQueryFont(display, defaultFont)) == NULL) {
      fprintf(stderr,
	      "blackbox: couldn't load default font.  please check to\n"
	      "make sure the necessary font is installed '%s'\n", defaultFont);
      exit(2);
    }
  }

  debug->msg("destroying databases\n");
  XrmDestroyDatabase(blackbox_database);
  XUngrabServer(display);
}


void BlackboxSession::updateWorkspace(int w) {
  ws_manager->workspace(w)->updateMenu();
}


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

  debug->msg("color %s - rgb: %d %d %d -scaled %d %d %d\n", colorname,
	     color.red, color.green, color.blue, *r, *g, *b);
  
  return color.pixel;
}


void BlackboxSession::parseSubMenu(FILE *menu_file, SessionMenu *menu) {
  char *line = new char[121], *label = new char[41], *command = new char[81];

  debug->msg("parsing menu -%s-\n", menu->label());
  if (! feof(menu_file)) {
    while (! feof(menu_file)) {
      memset(line, 0, 121);
      memset(label, 0, 41);
      memset(command, 0, 80);
      fgets(line, 120, menu_file);
      int len = strlen(line);
      debug->msg("%s-new line -%s-\n", menu->label(), line);

      int i, ri;
      for (i = 0; i < len; ++i)
	if (line[i] == '\"') { ++i; break; }
      for (ri = len; ri > 0; --ri)
	if (line[ri] == '\"') break;
      
      debug->msg("%s-checking for command: %i %i\n", menu->label(), i, ri);
      char *l;
      if (i < ri && ri > 0) {
	l = new char[ri - i + 1];
	strncpy(l, line + i, ri - i);
	*(l + (ri - i)) = '\0';
	
	for (i = 0; i < len; ++i)
	  if (line[i] == '(') { ++i; break; }
	for (ri = len; ri > 0; --ri)
	  if (line[ri] == ')') break;
	
	char *c;
	if (i < ri && ri > 0) {
	  c = new char[ri - i + 1];
	  strncpy(c, line + i, ri - i);
	  *(c + (ri - i)) = '\0';
	} else
	  c = 0;
	
	if (c) {
	  debug->msg("%s-inserting command -%s %s-\n", menu->label(), l, c);
	  if (! strncasecmp(c, "Reconfigure", 11)) {
	    menu->insert(l, B_Reconfigure);
	    delete [] c;
	  } else if (! strncasecmp(c, "Restart", 7)) {
	    menu->insert(l, B_Restart);
	    delete [] c;
	  } else if (! strncasecmp(c, "Exit", 4)) {
	    menu->insert(l, B_Exit);
	    delete [] c;
	  } else if (! strncasecmp(c, "Shutdown", 8)) {
	    menu->insert(l, B_Shutdown);
	    delete [] c;
	  } else
	    menu->insert(l, B_Execute, c);
	} else
	  fprintf(stderr, "error in menu file... label must have command\n"
		  "  ex:  \"label\" (command)\n");
      } else {
	debug->msg("%s-checking for submenu/window manager entry\n",
		   menu->label());
	for (i = 0; i < len; ++i)
	  if (line[i] == '[') { ++i; break; }
	for (ri = len; ri > 0; --ri)
	  if (line[ri] == ']') break;
      
	char *c;
	if (i < ri && ri > 0) {
	  c = new char[ri - i + 1];
	  strncpy(c, line + i, ri - i);
	  *(c + (ri - i)) = '\0';
	  
	  if (! strcasecmp(c, "begin")) {
	    SessionMenu *newmenu = new SessionMenu(this);

	    for (i = 0; i < len; ++i)
	      if (line[i] == '(') { ++i; break; }
	    for (ri = len; ri > 0; --ri)
	      if (line[ri] == ')') break;
	
	    char *l;
	    if (i < ri && ri > 0) {
	      l = new char[ri - i + 1];
	      strncpy(l, line + i, ri - i);
	      *(l + (ri - i)) = '\0';
	    } else
	      l = "(nil)";
	    
	    delete [] c;
	    newmenu->setMenuLabel(l);
	    parseSubMenu(menu_file, newmenu);
	    debug->msg("%s-inserting submenu -%s-\n", menu->label(), l);
	    menu->insert(l, newmenu);
	    newmenu->setMovable(False);
	    XRaiseWindow(display, newmenu->windowID());
	  } else if (! strcasecmp(c, "end")) {
	    delete [] c;
	    debug->msg("%s-end of submenu\n", menu->label());
	    break;
	  } else if (! strcasecmp(c, "restart")) {
	    
	    for (i = 0; i < len; ++i)
	      if (line[i] == '(') { ++i; break; }
	    for (ri = len; ri > 0; --ri)
	      if (line[ri] == ')') break;
	
	    char *l;
	    if (i < ri && ri > 0) {
	      l = new char[ri - i + 1];
	      strncpy(l, line + i, ri - i);
	      *(l + (ri - i)) = '\0';
	    } else
	      l = "(nil)";

	    for (i = 0; i < len; ++i)
	      if (line[i] == '{') { ++i; break; }
	    for (ri = len; ri > 0; --ri)
	      if (line[ri] == '}') break;
	
	    char *e;
	    if (i < ri && ri > 0) {
	      e = new char[ri - i + 1];
	      strncpy(e, line + i, ri - i);
	      *(e + (ri - i)) = '\0';
	    } else
	      e = 0;

	    delete [] c;
	    if (e)
	      menu->insert(l, B_RestartOther, e);
	  }
	}
      }
    }
  }

  delete [] command;
  delete [] line;
  delete [] label;
  menu->updateMenu();

  debug->msg("%s-end of parse\n", menu->label());
}


void BlackboxSession::Reconfigure(void) {
  LoadDefaults();

  XGCValues gcv;
  gcv.foreground = getColor("white");
  gcv.function = GXxor;
  gcv.line_width = 2;
  gcv.subwindow_mode = IncludeInferiors;
  gcv.font = resource.font.title->fid;
  XChangeGC(display, opGC, GCForeground|GCFunction|GCSubwindowMode|GCFont,
	    &gcv);

  Bool m = rootmenu->menuVisible();
  int x = rootmenu->X(), y = rootmenu->Y();
  rootmenu->hideMenu();
  InitMenu();
  if (m) {
    rootmenu->moveMenu(x, y);
    rootmenu->showMenu();
  }

  ws_manager->Reconfigure();
}
