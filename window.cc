//
// window.cc for Blackbox - an X11 Window manager
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
#include "window.hh"
#include "session.hh"
#include "icon.hh"
#include "workspace.hh"

#include <X11/keysym.h>

const unsigned long CLIENT_EVENT_MASK = (StructureNotifyMask|\
					 PropertyChangeMask|\
					 EnterWindowMask|\
					 LeaveWindowMask|\
					 ColormapChangeMask|\
					 SubstructureNotifyMask|\
					 FocusChangeMask);


// *************************************************************************
// Window Menu class code
// *************************************************************************

BlackboxWindowMenu::BlackboxWindowMenu(BlackboxWindow *w, BlackboxSession *s) :
  BlackboxMenu(s)
{
  window = w;
  session = s;
  hideTitle();
  
  insert("Send To ...",
         send_to_menu = new SendToWorkspaceMenu(window, session));
  insert("Iconify", BlackboxSession::B_WindowIconify);
  if (window->resizable())
    insert("(Un)Maximize", BlackboxSession::B_WindowMaximize);

  insert("Close", BlackboxSession::B_WindowClose);
  insert("Raise", BlackboxSession::B_WindowRaise);
  insert("Lower", BlackboxSession::B_WindowLower);

  updateMenu();
}


BlackboxWindowMenu::~BlackboxWindowMenu(void)
{ delete send_to_menu; }
void BlackboxWindowMenu::titlePressed(int) { }
void BlackboxWindowMenu::titleReleased(int) { }
void BlackboxWindowMenu::itemPressed(int button, int item) {
  if (button == 1 && item == 0) {
    send_to_menu->updateMenu();
    BlackboxMenu::drawSubMenu(item);
  }
}


void BlackboxWindowMenu::itemReleased(int button, int item) {
  if (button == 1) {
    if (window->resizable())
      switch (item) {
      case 1:
	hideMenu();
	window->iconifyWindow();
	break;
	
      case 2:
	hideMenu();
	window->maximizeWindow();
	break;
	
      case 3:
	hideMenu();
	window->closeWindow();
	break;
	
      case 4:
	hideMenu();
	window->raiseWindow();
	break;
	
      case 5:
	hideMenu();
	window->lowerWindow();
	break;
      }
    else
      switch (item) {
      case 1:
	hideMenu();
	window->iconifyWindow();
	break;
	
      case 2:
	hideMenu();
	window->closeWindow();
	break;
	
      case 3:
	hideMenu();
	window->raiseWindow();
	break;
	
      case 4:
	hideMenu();
	window->lowerWindow();
	break;
      }
  }
}


void BlackboxWindowMenu::showMenu()
{ BlackboxMenu::showMenu(); window->setMenuVisible(true); }
void BlackboxWindowMenu::hideMenu() {
  send_to_menu->hideMenu();
  BlackboxMenu::hideMenu();
  window->setMenuVisible(false);
}


void BlackboxWindowMenu::moveMenu(int x, int y)
{ BlackboxMenu::moveMenu(x, y); }


// *************************************************************************
// Window send to workspace menu class code
// *************************************************************************

SendToWorkspaceMenu::SendToWorkspaceMenu(BlackboxWindow *w,
					 BlackboxSession *s)
  : BlackboxMenu(s)
{
  window = w;
  ws_manager = s->WSManager();

  hideTitle();
  updateMenu();
}


void SendToWorkspaceMenu::titlePressed(int) { }
void SendToWorkspaceMenu::titleReleased(int) { }
void SendToWorkspaceMenu::itemPressed(int, int) { }
void SendToWorkspaceMenu::itemReleased(int button, int item) {
  if (button == 1)
    if (item < ws_manager->count()) {
      if (item != ws_manager->currentWorkspaceID()) {
	ws_manager->workspace(window->workspace())->removeWindow(window);
	ws_manager->workspace(item)->addWindow(window);
	window->withdrawWindow();
	hideMenu();
      }
    } else
      updateMenu();
}


void SendToWorkspaceMenu::showMenu(void) {
  updateMenu();
  BlackboxMenu::showMenu();
}


void SendToWorkspaceMenu::hideMenu(void)
{ BlackboxMenu::hideMenu(); }
void SendToWorkspaceMenu::updateMenu(void) {
  int i, r = BlackboxMenu::count();

  if (BlackboxMenu::count() != 0)
    for (i = 0; i < r; ++i)
      BlackboxMenu::remove(0);
  
  for (i = 0; i < ws_manager->count(); ++i)
    BlackboxMenu::insert(ws_manager->workspace(i)->name());
  
  BlackboxMenu::updateMenu();
}
  

// *************************************************************************
// Window class code
// *************************************************************************

/*
  Resources allocated for each window:
  Debugger - debug
  Window - frame window, title, handle, resize handle, close button,
      iconify button, maximize button, input window
  Menu - window menu
  XPointer - client title
  Pixmaps - ftitle, utitle, fhandle, uhandle, button, pbutton
  GC - lGC, dGC, textGC
*/

BlackboxWindow::BlackboxWindow(BlackboxSession *ctrl, Window parent,
			       Window window) {
  debug = new Debugger('%');
#ifdef DEBUG
  debug->enable();
#endif
  debug->enter("% blackbox window constructor\n");
  debug->msg("creating window/contexts %lx <- %lx\n", window, parent);

  moving = False;
  resizing = False;
  shaded = False;
  maximized = False;
  visible = False;
  iconic = False;
  transient = False;
  focused = False;
  menu_visible = False;
  window_number = -1;
  
  session = ctrl;
  display = ctrl->control();
  client.window = window;
  frame.window = frame.title = frame.handle = frame.close_button =
    frame.iconify_button = frame.maximize_button = client.icon_window = 
    client.icon_pixmap = client.icon_mask = frame.button = frame.pbutton =
    None;
  client.group = client.transient_for = client.transient = 0;
  client.title = client.app_class = client.app_name = 0;
  wm_command = wm_client_machine = 0;
  icon = 0;
  
  XWindowAttributes wattrib;
  XGetWindowAttributes(display, client.window, &wattrib);
  if (wattrib.override_redirect) return;
  do_close = True;
  client.x = wattrib.x;
  client.y = wattrib.y;
  client.width = wattrib.width;
  client.height = wattrib.height;

  Window win;
  getWMHints();

  transient = False;
  do_handle = True;
  do_iconify = True;
  do_maximize = True;
  
  if (XGetTransientForHint(display, client.window, &win)) {
    if (win != client.window_group) {
      if ((client.transient_for = session->getWindow(win)) != NULL) {
	debug->msg("setting transient owner %lx\n", win);
	client.transient_for->client.transient = this;
      } else
	debug->msg("transient owner doesn't exist %lx\n", win);
      
      transient = True;
      do_iconify = True;
      do_handle = False;
      do_maximize = False; 
    }
  } else
    debug->msg("window is not a transient\n");
    
  XSizeHints sizehint;
  getWMNormalHints(&sizehint);
  if ((client.hint_flags & PMinSize) && (client.hint_flags & PMaxSize))
    if (client.max_w == client.min_w && client.max_h == client.min_h) {
      do_handle = False;
      do_maximize = False;
    }
  
  frame.handle_h = ((do_handle) ? client.height : 0);
  frame.handle_w = ((do_handle) ? 8 : 0);
  frame.title_h = session->titleFont()->ascent +
    session->titleFont()->descent + 8;
  frame.title_w = client.width + ((do_handle) ? frame.handle_w + 1 : 0);
  frame.button_h = frame.title_h - 8;
  frame.button_w = frame.button_h * 2 / 3;

  if ((session->Startup()) || client.hint_flags & (PPosition|USPosition)
      || transient) { 
    frame.x = client.x;
    frame.y = client.y;
  } else {
    static unsigned int cx = 32, cy = 32;

    if (cx > (session->XResolution() / 2)) { cx = 32; cy = 32; }
    if (cy > (session->YResolution() / 2)) { cx = 32; cy = 32; }

    frame.x = cx;
    frame.y = cy;

    cy += frame.title_h;
    cx += frame.title_h;
  }

  frame.width = frame.title_w;
  frame.height = client.height + frame.title_h + 1;
  frame.border = 1;

  frame.window = createToplevelWindow(frame.x, frame.y, frame.width,
				      frame.height, frame.border);
  XSaveContext(display, frame.window, session->winContext(), (XPointer) this);

  frame.title = createChildWindow(frame.window, 0, 0, frame.title_w,
				  frame.title_h, 0L);
  XSaveContext(display, frame.title, session->winContext(), (XPointer) this);

  if (do_handle) {
    if (session->Orientation() == BlackboxSession::B_LeftHandedUser)
      frame.handle = createChildWindow(frame.window, 0, frame.title_h + 1,
				       frame.handle_w, frame.handle_h, 0l);
    else
      frame.handle = createChildWindow(frame.window, client.width + 1,
				       frame.title_h + 1, frame.handle_w,
				       frame.handle_h, 0l);
    
    XSaveContext(display, frame.handle, session->winContext(),
		 (XPointer) this);
    
    frame.resize_handle = createChildWindow(frame.handle, 0, frame.handle_h -
					    (frame.button_h),
					    frame.handle_w,
					    frame.button_h, 0);
					    
    frame.handle_h -= frame.button_h;
    XSaveContext(display, frame.resize_handle, session->winContext(),
    		 (XPointer) this);
  }

  client.WMProtocols = session->ProtocolsAtom();
  getWMProtocols();
  createDecorations();
  associateClientWindow();
  positionButtons();
  XGrabButton(display,  1, ControlMask, frame.window, True, ButtonPressMask,
	      GrabModeAsync, GrabModeSync, frame.window, None);
  XGrabButton(display,  3, ControlMask, frame.window, True, ButtonPressMask,
	      GrabModeAsync, GrabModeSync, frame.window, None);
  
  XLowerWindow(display, client.window);
  XMapSubwindows(display, frame.title);
  if (do_handle) XMapSubwindows(display, frame.handle);
  XMapSubwindows(display, frame.window);
  
  window_menu = new BlackboxWindowMenu(this, session);
  session->addWindow(this);
  configureWindow(frame.x, frame.y, frame.width, frame.height);

  if (iconic && ! visible)
    iconifyWindow();
}


/*
  Resources deallocated for each window:
  * Debugger - debug
  * Window - frame window, title, handle, resize handle, close button,
      iconify button, maximize button, input window,
      client window (context only)
  * Icon - icon
  * Menu - window menu
  * XPointer - client title
  * Pixmaps - ftitle, utitle, fhandle, uhandle, button, pbutton
  * GC - lGC, dGC, textGC
*/

BlackboxWindow::~BlackboxWindow(void) {
  debug->msg("deleting window object %lx\n", client.window);  
  XGrabServer(display);
  
  /*
    remove window from internal window list...
    remove the icon if we're iconified
    dissociate this window from it's parent if this window is a transient
    window 
  */

  delete window_menu;
  if (icon)
    delete icon;

  session->removeWindow(this);
  if (transient && client.transient_for)
    client.transient_for->client.transient = 0;
  
  /*
    destroy struct frame subwindows first: window, title, handle,
    close_button, iconify_button, maximize_button
  */
  
  if (frame.close_button != None) {
    XDeleteContext(display, frame.close_button, session->winContext());
    XDestroyWindow(display, frame.close_button);
  }
  if (frame.iconify_button != None) {
    XDeleteContext(display, frame.iconify_button, session->winContext());
    XDestroyWindow(display, frame.iconify_button);
  }
  if (frame.maximize_button != None) {
    XDeleteContext(display, frame.maximize_button, session->winContext());
    XDestroyWindow(display, frame.maximize_button);
  }

  if (frame.title != None) {
    if (frame.ftitle) XFreePixmap(display, frame.ftitle);
    if (frame.utitle) XFreePixmap(display, frame.utitle);
    XDeleteContext(display, frame.title, session->winContext());
    XDestroyWindow(display, frame.title);
  }
  if (frame.handle != None) {
    if (frame.fhandle) XFreePixmap(display, frame.fhandle);
    if (frame.uhandle) XFreePixmap(display, frame.uhandle);
    if (frame.rhandle) XFreePixmap(display, frame.rhandle);
    XDeleteContext(display, frame.handle, session->winContext());
    XDeleteContext(display, frame.resize_handle, session->winContext());
    XDestroyWindow(display, frame.resize_handle);
    XDestroyWindow(display, frame.handle);
  }

  if (frame.button != None) XFreePixmap(display, frame.button);
  if (frame.pbutton != None) XFreePixmap(display, frame.pbutton);

  XDeleteContext(display, client.window, session->winContext());
  XDeleteContext(display, frame.window, session->winContext());
  XDestroyWindow(display, frame.window);  

  if (client.app_name) XFree(client.app_name);
  if (client.app_class) XFree(client.app_class);
  if (client.title)
    if (strcmp(client.title, "Unnamed"))
      XFree(client.title);
  if (wm_client_machine) XFree(wm_client_machine);
  if (wm_command) XFree(wm_command);

  XFreeGC(display, frame.ftextGC);
  XFreeGC(display, frame.utextGC);

  XUngrabServer(display);

  debug->msg("window object deleted\n");
  delete debug;
}


// *************************************************************************
// Window decoration code
// *************************************************************************

Window BlackboxWindow::createToplevelWindow(int x, int y, unsigned int width,
					    unsigned int height,
					    unsigned int borderwidth)
{
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWOverrideRedirect |CWCursor|CWEventMask; 
  
  attrib_create.background_pixmap = None;
  attrib_create.background_pixel = session->frameColor().pixel;
  attrib_create.border_pixel = session->frameColor().pixel;
  attrib_create.override_redirect = True;
  attrib_create.cursor = session->sessionCursor();
  attrib_create.event_mask = SubstructureRedirectMask|ButtonPressMask|
    ButtonReleaseMask|ButtonMotionMask|ExposureMask|EnterWindowMask;
  
  debug->msg("creating top level window\n");
  
  return (XCreateWindow(display, session->Root(), x, y, width, height,
			borderwidth, session->Depth(), InputOutput,
			session->visual(), create_mask,
			&attrib_create));
}


Window BlackboxWindow::createChildWindow(Window parent, int x, int y,
					 unsigned int width,
					 unsigned int height,
					 unsigned int borderwidth)
{
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|CWCursor|
    CWEventMask;
  
  attrib_create.background_pixmap = None;
  attrib_create.background_pixel = 0l;
  attrib_create.border_pixel = 0l;
  attrib_create.cursor = session->sessionCursor();
  attrib_create.event_mask = KeyPressMask|KeyReleaseMask|ButtonPressMask|
    ButtonReleaseMask|ButtonMotionMask|ExposureMask|EnterWindowMask|LeaveWindowMask;
  
  return (XCreateWindow(display, parent, x, y, width, height, borderwidth,
			session->Depth(), InputOutput, session->visual(),
			create_mask, &attrib_create));
}


void BlackboxWindow::associateClientWindow(void) {
  Protocols.WM_DELETE_WINDOW = False;
  Protocols.WM_TAKE_FOCUS = True;

  XSetWindowAttributes attrib_set;
  attrib_set.event_mask = CLIENT_EVENT_MASK;
  attrib_set.do_not_propagate_mask = ButtonPressMask|ButtonReleaseMask;
  attrib_set.save_under = False;
  XChangeWindowAttributes(display, client.window, CWEventMask|CWDontPropagate|
			  CWSaveUnder, &attrib_set);
  XSetWindowBorderWidth(display, client.window, 0);

  if (! XFetchName(display, client.window, &client.title))
    client.title = "Unnamed";

  if (session->Orientation() == BlackboxSession::B_LeftHandedUser)
    XReparentWindow(display, client.window, frame.window,
		    ((do_handle) ? frame.handle_w + 1 : 0),
		    frame.title_h + 1);
  else
    XReparentWindow(display, client.window, frame.window, 0,
		    frame.title_h + 1);

  if (session->shapeExtensions())
    XShapeSelectInput(display, client.window, ShapeNotifyMask);

  XSaveContext(display, client.window, session->winContext(), (XPointer) this);
  createIconifyButton();
  createMaximizeButton();

  if (frame.button && frame.close_button)
    XSetWindowBackgroundPixmap(display, frame.close_button, frame.button);
  if (frame.button && frame.maximize_button)
    XSetWindowBackgroundPixmap(display, frame.maximize_button, frame.button);
  if (frame.button && frame.iconify_button)
    XSetWindowBackgroundPixmap(display, frame.iconify_button, frame.button);
}


void BlackboxWindow::createDecorations(void) {
  BImage image(session, frame.title_w, frame.title_h, session->Depth(),
	       session->focusColor());

  frame.ftitle = image.renderImage(session->frameTexture(), 1,
				   session->focusColor(),
				   session->focusToColor());
  
  frame.utitle = image.renderImage(session->frameTexture(), 1,
				   session->unfocusColor(),
				   session->unfocusToColor());
  
  XSetWindowBackgroundPixmap(display, frame.title, frame.utitle);
  XClearWindow(display, frame.title);
  
  if (do_handle) {
    BImage h_image(session, frame.handle_w, frame.handle_h, session->Depth(),
		   session->focusColor());
    
    frame.fhandle = h_image.renderImage(session->frameTexture(), 1,
					session->focusColor(),
					session->focusToColor());
    
    frame.uhandle = h_image.renderImage(session->frameTexture(), 1,
					session->unfocusColor(),
					session->unfocusToColor());
    
    XSetWindowBackground(display, frame.handle, frame.uhandle);
    XClearWindow(display, frame.handle);
    
    BImage rh_image(session, frame.handle_w, frame.button_h, session->Depth(),
		    session->buttonColor());

    frame.rhandle = rh_image.renderImage(session->buttonTexture(), 0,
					 session->buttonColor(),
					 session->buttonToColor());
     
    XSetWindowBackgroundPixmap(display, frame.resize_handle, frame.rhandle);
    XClearWindow(display, frame.resize_handle);
  }
  
  BImage button_image(session, frame.button_w, frame.button_h,
		      session->Depth(), session->buttonColor());

  frame.button = button_image.renderImage(session->buttonTexture(), 0,
					  session->buttonColor(),
					  session->buttonToColor());
  frame.pbutton =
    button_image.renderInvertedImage(session->buttonTexture(), 0,
				     session->buttonColor(),
				     session->buttonToColor());

  XGCValues gcv;
  gcv.foreground = session->unfocusTextColor().pixel;
  gcv.font = session->titleFont()->fid;
  frame.utextGC = XCreateGC(display, frame.window, GCForeground|GCBackground|
			    GCFont, &gcv);

  gcv.foreground = session->focusTextColor().pixel;
  gcv.font = session->titleFont()->fid;
  frame.ftextGC = XCreateGC(display, frame.window, GCForeground|GCBackground|
			    GCFont, &gcv);
}


void BlackboxWindow::positionButtons(void) {
  if (session->Orientation() == BlackboxSession::B_LeftHandedUser) {
    int mx = 3, my = 3;
    
    if (do_iconify && frame.iconify_button != None) {
      XMoveWindow(display, frame.iconify_button, mx, my);
      XMapWindow(display, frame.iconify_button);
      XClearWindow(display, frame.iconify_button);
      mx = mx + frame.button_w + 4;
    }
    
    if (do_maximize && frame.maximize_button != None) {
      
      XMoveWindow(display, frame.maximize_button, mx, my);
      XMapWindow(display, frame.maximize_button);
      XClearWindow(display, frame.maximize_button);
      mx = mx + frame.button_w + 4;
    }
    
    if (do_close && frame.close_button != None) {
      XMoveWindow(display, frame.close_button, mx, my);
      XMapWindow(display, frame.close_button);
      XClearWindow(display, frame.close_button);
    }
  } else {
    int mx = frame.title_w - frame.button_w - 5, my = 3;
    
    if (do_close && frame.close_button != None) {
      XMoveWindow(display, frame.close_button, mx, my);
      XMapWindow(display, frame.close_button);
      XClearWindow(display, frame.close_button);
      mx = mx - frame.button_w - 4;
    }

    if (do_maximize && frame.maximize_button != None) {
      
      XMoveWindow(display, frame.maximize_button, mx, my);
      XMapWindow(display, frame.maximize_button);
      XClearWindow(display, frame.maximize_button);
      mx = mx - frame.button_w - 4;
    }

    if (do_iconify && frame.iconify_button != None) {
      XMoveWindow(display, frame.iconify_button, mx, my);
      XMapWindow(display, frame.iconify_button);
      XClearWindow(display, frame.iconify_button);
    }
  }
}


void BlackboxWindow::createCloseButton(void) {
  if (do_close && frame.title != None && frame.close_button == None) {
    frame.close_button = createChildWindow(frame.title, 0, 0, frame.button_w,
					   frame.button_h, 1);
    if (frame.button != None)
      XSetWindowBackgroundPixmap(display, frame.close_button, frame.button);

    XSaveContext(display, frame.close_button, session->winContext(),
		 (XPointer) this);
  }
}


void BlackboxWindow::createIconifyButton(void) {
  if (do_iconify && frame.title != None) {
    frame.iconify_button =
      createChildWindow(frame.title, 0, 0, frame.button_w,
		    frame.button_h, 1);
    XSaveContext(display, frame.iconify_button, session->winContext(),
		 (XPointer) this);
  }
}


void BlackboxWindow::createMaximizeButton(void) {
  if (do_maximize && frame.title != None) {
    frame.maximize_button =
      createChildWindow(frame.title, 0, 0, frame.button_w, frame.button_h, 1);
    XSaveContext(display, frame.maximize_button, session->winContext(),
		 (XPointer) this);
  }
}


// *************************************************************************
// Window protocol and ICCCM code
// *************************************************************************

Bool BlackboxWindow::getWMProtocols(void) {
  Atom *protocols;
  int num_return = 0;
  
  XGetWMProtocols(display, client.window, &protocols, &num_return);
  
  debug->msg("got %d protocols\n", num_return);
  int i;
  
  do_close = False;
  for (i = 0; i < num_return; ++i) {
    if (protocols[i] == session->DeleteAtom()) {
      debug->msg("WM_DELETE_WINDOW\n");
      do_close = True;
      Protocols.WM_DELETE_WINDOW = True;
      client.WMDelete = session->DeleteAtom();
      createCloseButton();
      positionButtons();
    } else if (protocols[i] ==  session->FocusAtom()) {
      debug->msg("WM_TAKE_FOCUS\n");
      Protocols.WM_TAKE_FOCUS = True;
    } else if (protocols[i] ==  session->StateAtom()) {
      debug->msg("WM_STATE\n");
    } else  if (protocols[i] ==  session->ColormapAtom()) {
      debug->msg("wm colormap windows\n");
    }
  }
    
  return True;
}


Bool BlackboxWindow::getWMHints(void) {
  debug->msg("getting window manager hints\n");
  XWMHints *wmhints;
  if ((wmhints = XGetWMHints(display, client.window)) == NULL) {
    visible = True;
    iconic = False;
    focus_mode = F_Passive;
    return False;
  }

  if (wmhints->flags & InputHint) {
    if (wmhints->input == True) {
      if (Protocols.WM_TAKE_FOCUS)
	focus_mode = F_LocallyActive;
      else
	focus_mode = F_Passive;
    } else {
      if (Protocols.WM_TAKE_FOCUS)
	focus_mode = F_GloballyActive;
      else
	focus_mode = F_NoInput;
    }
  } else
    focus_mode = F_Passive;
  
  if (wmhints->flags & StateHint)
    if (! session->Startup())
      switch(wmhints->initial_state) {
      case WithdrawnState:
	visible = False;
	iconic = False;
	break;
	
      case NormalState:
	visible = True;
	iconic = False;
	break;
	
      case IconicState:
	visible = False;
	iconic = True;
	break;
      }
    else {
      visible = True;
      iconic = False;
    }
  else {
    visible = True;
    iconic = False;
  } 
  
  if (wmhints->flags & IconPixmapHint) {
    client.icon_pixmap = wmhints->icon_pixmap;
    useIconWindow = False;
  }
  
  if (wmhints->flags & IconWindowHint) {
    client.icon_window = wmhints->icon_window;
    useIconWindow = True;
  }
  
  /* icon position hint would be next, but as the ICCCM says, we can ignore
     this hint, as it is the window managers function to organize and place
     icons
  */
  
  if (wmhints->flags & IconMaskHint) {
    client.icon_mask = wmhints->icon_mask;
  }
  
  if (wmhints->flags & WindowGroupHint) {
    client.window_group = wmhints->window_group;
    client.group = session->getWindow(wmhints->window_group);
  }
  
  return True;
}


Bool BlackboxWindow::getWMNormalHints(XSizeHints *hint) {
  long icccm_mask;
  if (! XGetWMNormalHints(display, client.window, hint, &icccm_mask))
    return(False);
  
  /* check to see if we're dealing with an up to date client */

  client.hint_flags = hint->flags;
  if (icccm_mask == (USPosition|USSize|PPosition|PSize|PMinSize|PMaxSize|
		     PResizeInc|PAspect))
    icccm_compliant = False;
  else
    icccm_compliant = True;
  
  if (client.hint_flags & PResizeInc) {
    client.inc_w = hint->width_inc;
    client.inc_h = hint->height_inc;
  } else
    client.inc_h = client.inc_w = 1;
  
  if (client.hint_flags & PMinSize) {
    client.min_w = hint->min_width;
    client.min_h = hint->min_height;
  } else
    client.min_h = client.min_w = 1;

  if (client.hint_flags & PBaseSize) {
    client.base_w = hint->base_width;
    client.base_h = hint->base_height;
  } else
    client.base_w = client.base_h = 0;

  if (client.hint_flags & PMaxSize) {
    client.max_w = hint->max_width;
    client.max_h = hint->max_height;
  } else {
    client.max_w = session->XResolution();
    client.max_h = session->YResolution();
  }
  
  if (client.hint_flags & PAspect) {
    client.min_ax = hint->min_aspect.x;
    client.min_ay = hint->min_aspect.y;
    client.max_ax = hint->max_aspect.x;
    client.max_ay = hint->max_aspect.y;
  }

  return True;
}


// *************************************************************************
// Window utility function code
// *************************************************************************

void BlackboxWindow::configureWindow(int dx, int dy, unsigned int dw,
				     unsigned int dh) {
  //
  // configure our client and decoration windows according to the frame size
  // changes passed as the arguments
  //

  Bool resize, synth_event;

  debug->msg("configuring window %d,%d %dx%d\n   max w/h %d/%d\n", dx, dy,
             dw, dh, client.max_w, client.max_h);

  if (dw > (client.max_w + ((do_handle) ? frame.handle_w + 1 : 0)))
    dw = client.max_w + ((do_handle) ? frame.handle_w + 1 : 0);
  if (dh > (client.max_h + frame.title_h + 1))
    dh = client.max_h + frame.title_h + 1;
  
  if ((((int) frame.width) + dx) < 0) dx = 0;
  if ((((int) frame.title_h) + dy) < 0) dy = 0;
  
  resize = ((dw != frame.width) || (dh != frame.height));
  synth_event = ((dx != frame.x || dy != frame.y) && (! resize));

  if (resize) {
    frame.x = dx;
    frame.y = dy;
    frame.width = dw;
    frame.height = dh;
    frame.title_w = frame.width;
    frame.handle_h = dh - frame.title_h - 1;
    client.x = dx + frame.border;
    client.y = dy + frame.title_h + frame.border + 1;
    client.width = dw - ((do_handle) ? (frame.handle_w + 1) : 0);
    client.height = dh - frame.title_h - 1;

    XWindowChanges xwc;
    xwc.x = dx;
    xwc.y = dy;
    xwc.width = dw;
    xwc.height = dh;
    XGrabServer(display);
    XConfigureWindow(display, frame.window, CWX|CWY|CWWidth|CWHeight, &xwc);
    XResizeWindow(display, frame.title, frame.title_w, frame.title_h);

    if (do_handle) {
      if (session->Orientation() == BlackboxSession::B_LeftHandedUser)
	XMoveResizeWindow(display, frame.handle, 0, frame.title_h + 1,
			  frame.handle_w, frame.handle_h);
      else
	XMoveResizeWindow(display, frame.handle, client.width + 1,
			  frame.title_h + 1, frame.handle_w, frame.handle_h);

      frame.handle_h -= frame.button_h;
      if (frame.handle_h <= 0) frame.handle_h = 1;
      XMoveWindow(display, frame.resize_handle, 0, frame.handle_h);
    }

    if (session->Orientation() == BlackboxSession::B_LeftHandedUser)
      XMoveResizeWindow(display, client.window,
			((do_handle) ? frame.handle_w + 1 : 0),
			frame.title_h + 1, client.width, client.height);
    else
      XMoveResizeWindow(display, client.window, 0, frame.title_h + 1,
			client.width, client.height);

    positionButtons();

    if (frame.ftitle) XFreePixmap(display, frame.ftitle);
    if (frame.utitle) XFreePixmap(display, frame.utitle);

    BImage image(session, frame.title_w, frame.title_h, session->Depth(),
		 session->focusColor());

    frame.ftitle = image.renderImage(session->frameTexture(), 1,
				     session->focusColor(),
				     session->focusToColor());

    frame.utitle = image.renderImage(session->frameTexture(), 1,
				     session->unfocusColor(),
				     session->unfocusToColor());
    
    if (do_handle) {
      if (frame.fhandle) XFreePixmap(display, frame.fhandle);
      if (frame.uhandle) XFreePixmap(display, frame.uhandle);
      
      BImage h_image(session, frame.handle_w, frame.handle_h,
		     session->Depth(), session->focusColor());

      frame.fhandle = h_image.renderImage(session->frameTexture(), 1,
					  session->focusColor(),
					  session->focusToColor());

      frame.uhandle = h_image.renderImage(session->frameTexture(), 1,
					  session->unfocusColor(),
					  session->unfocusToColor());
    }
    
    setFocusFlag(focused);
    drawTitleWin(0, 0, 0, 0);
    drawAllButtons();
    XUngrabServer(display);
  } else {
    frame.x = dx;
    frame.y = dy;
    client.x = dx + frame.border;
    client.y = dy + frame.title_h + frame.border + 1;
   
    XWindowChanges xwc;
    xwc.x = dx;
    xwc.y = dy;
    XConfigureWindow(display, frame.window, CWX|CWY, &xwc);
  }

  if (synth_event) {
    XEvent event;
    event.type = ConfigureNotify;

    event.xconfigure.display = display;
    event.xconfigure.event = client.window;
    event.xconfigure.window = client.window;
    event.xconfigure.x = client.x;
    event.xconfigure.y = client.y;
    event.xconfigure.width = client.width;
    event.xconfigure.height = client.height;
    event.xconfigure.border_width = 0;
    event.xconfigure.above = frame.window;
    event.xconfigure.override_redirect = False;

    XSendEvent(display, client.window, False, StructureNotifyMask, &event);
  }
}


void BlackboxWindow::setFGColor(GC *gc, char *color) {
  XGCValues gcv;
  gcv.foreground = session->getColor(color);
  XChangeGC(display, *gc, GCForeground, &gcv);
}


void BlackboxWindow::setFGColor(GC *gc, unsigned long color) {
  XGCValues gcv;
  gcv.foreground = color;
  XChangeGC(display, *gc, GCForeground, &gcv);
}


Bool BlackboxWindow::setInputFocus(void) {
  switch (focus_mode) {
  case F_NoInput:
  case F_GloballyActive:
    break;
    
  case F_LocallyActive:
  case F_Passive:
    XSetWindowBackgroundPixmap(display, frame.title, frame.ftitle);
    XClearWindow(display, frame.title);

    if (do_handle) {
      XSetWindowBackgroundPixmap(display, frame.handle, frame.fhandle);
      XClearWindow(display, frame.handle);
    }

    debug->msg("setting focus to window %lx - visible(Bool) %d\n",
	       client.window, visible);
    XSetInputFocus(display, client.window, RevertToParent,
		   CurrentTime);

    drawTitleWin(0,0,0,0);
    return True;
    break;
  }

  return False;
}


void BlackboxWindow::iconifyWindow(void) {
  window_menu->hideMenu();
  menu_visible = False;

  XUnmapWindow(display, frame.window);
  visible = False;
  iconic = True;
    
  if (transient) {
    if (! client.transient_for->iconic)
      client.transient_for->iconifyWindow();
  } else
    icon = new BlackboxIcon(session, client.window);

  if (client.transient)
    if (! client.transient->iconic)
      client.transient->iconifyWindow();

  unsigned long state[2];
  state[0] = (unsigned long) IconicState;
  state[1] = (unsigned long) client.icon_pixmap;
  debug->msg("changing to iconic state %lx %d\n", client.window,
             state[0]);
  XChangeProperty(display, client.window, session->StateAtom(),
                  session->StateAtom(), 32, PropModeReplace,
		  (unsigned char *) state, 2);
}


void BlackboxWindow::deiconifyWindow(void) {
  XMapRaised(display, frame.window);
  visible = True;
  iconic = False;

  if (client.transient) client.transient->deiconifyWindow();
  
  unsigned long state[2];
  state[0] = (unsigned long) NormalState;
  state[1] = (unsigned long) None;

  debug->msg("changing to normal state %lx %d\n", client.window,
	     state[0]);
  XChangeProperty(display, client.window, session->StateAtom(),
		  session->StateAtom(), 32, PropModeReplace,
		  (unsigned char *) state, 2);
  session->reassociateWindow(this);

  if (icon) {
    delete icon;
    removeIcon();
  }
}


void BlackboxWindow::closeWindow(void) {
  XEvent ce;
  ce.xclient.type = ClientMessage;
  ce.xclient.message_type = session->ProtocolsAtom();
  ce.xclient.display = display;
  ce.xclient.window = client.window;
  ce.xclient.format = 32;
  
  ce.xclient.data.l[0] = session->DeleteAtom();
  ce.xclient.data.l[1] = CurrentTime;
  ce.xclient.data.l[2] = 0L;
  ce.xclient.data.l[3] = 0L;
  
  XSendEvent(display, client.window, False, NoEventMask, &ce);
  XFlush(display);
}


void BlackboxWindow::withdrawWindow(void) {
  XGrabServer(display);
  XSync(display, 0);
  
  unsigned long state[2];
  state[0] = (unsigned long) WithdrawnState;
  state[1] = (unsigned long) None;

  visible = False;
  XUnmapWindow(display, frame.window);
  window_menu->hideMenu();
  
  XEvent foo;
  if (! XCheckTypedWindowEvent(display, client.window, DestroyNotify,
			       &foo)) {
    debug->msg("changing to unmapped state %lx %d\n", client.window,
	       visible);
    XChangeProperty(display, client.window, session->StateAtom(),
		    session->StateAtom(), 32, PropModeReplace,
		    (unsigned char *) state, 2);
  }
  
  XUngrabServer(display);
}


int BlackboxWindow::setWindowNumber(int n) {
  window_number = n;
  return window_number;
}


int BlackboxWindow::setWorkspace(int n) {
  workspace_number = n;
  return workspace_number;
}


void BlackboxWindow::raiseWindow(void) {
  XGrabServer(display);

  XRaiseWindow(display, frame.window);
  if (client.transient)
    client.transient->raiseWindow();
  if (menu_visible)
    XRaiseWindow(display, window_menu->windowID());

  XUngrabServer(display);
}


void BlackboxWindow::lowerWindow(void) {
  XGrabServer(display);

  if (client.transient)
    client.transient->lowerWindow();
  if (menu_visible)
    XLowerWindow(display, window_menu->windowID());
  XLowerWindow(display, frame.window);

  XUngrabServer(display);
}


void BlackboxWindow::maximizeWindow(void) {
  XGrabServer(display);

  static int unmaximize = 0, px, py;
  static unsigned int pw, ph;

  if (! unmaximize) {
    int dx, dy;
    unsigned int dw, dh;

    px = frame.x;
    py = frame.y;
    pw = frame.width;
    ph = frame.height;

    dw = session->XResolution() - session->WSManager()->Width() -
      ((do_handle) ? frame.handle_w + 1 : 0) - frame.border;
    dw -= client.base_w;
    dw -= (dw % client.inc_w);
    dw -= client.inc_w;
    dw += ((do_handle) ? frame.handle_w + 1 : 0) + frame.border +
      client.base_w;
    
    dx = (((session->XResolution() - session->WSManager()->Width()) - dw) / 2)
      + session->WSManager()->Width();

    dh = session->YResolution() - frame.title_h - 1 - frame.border;
    dh -= client.base_h;
    dh -= (dh % client.inc_h);
    dh -= client.inc_h;
    dh += frame.title_h + 1 + frame.border + client.base_h;
    
    dy = ((session->YResolution()) - dh) / 2;

    unmaximize = 1;
    configureWindow(dx, dy, dw, dh);
    raiseWindow();
  } else {
    configureWindow(px, py, pw, ph);
    unmaximize = 0;
  }

  XUngrabServer(display);
}

// *************************************************************************
// Window drawing code
// *************************************************************************

void BlackboxWindow::drawTitleWin(int /* ax */, int /* ay */, int /* aw */,
				  int /* ah */) {  
  int dx = frame.button_w + 4;
  
  if (session->Orientation() == BlackboxSession::B_LeftHandedUser) {
    if (do_close) dx += frame.button_w + 4;
    if (do_maximize) dx += frame.button_w + 4;
    if (do_iconify) dx += frame.button_w + 4;
  }
  
  XDrawString(display, frame.title,
	      ((focused) ? frame.ftextGC : frame.utextGC), dx,
	      session->titleFont()->ascent + 4, client.title,
	      strlen(client.title));
}


void BlackboxWindow::drawAllButtons(void) {
  if (frame.iconify_button) drawIconifyButton(False);
  if (frame.maximize_button) drawMaximizeButton(False);
  if (frame.close_button) drawCloseButton(False);
}


void BlackboxWindow::drawIconifyButton(Bool pressed) {
  if (! pressed && frame.button) {
    XSetWindowBackgroundPixmap(display, frame.iconify_button, frame.button);
    XClearWindow(display, frame.iconify_button);
  } else if (frame.pbutton) {
    XSetWindowBackgroundPixmap(display, frame.iconify_button, frame.pbutton);
    XClearWindow(display, frame.iconify_button);
  }

  XPoint pts[4];
  pts[0].x = (frame.button_w - 5) / 2;
  pts[0].y = frame.button_h - ((frame.button_h - 6) / 2) - 1;

  pts[1].x = 5; pts[1].y = 0;
  pts[2].x = 0; pts[2].y = -1;
  pts[3].x = -5; pts[3].y = 0;
  
  XDrawLines(display, frame.iconify_button,
	     ((focused) ? frame.ftextGC : frame.utextGC), pts, 4,
	     CoordModePrevious);
}


void BlackboxWindow::drawMaximizeButton(Bool pressed) {
  if (! pressed && frame.button) {
    XSetWindowBackgroundPixmap(display, frame.maximize_button, frame.button);
    XClearWindow(display, frame.maximize_button);
  } else if (frame.pbutton) {
    XSetWindowBackgroundPixmap(display, frame.maximize_button, frame.pbutton);
    XClearWindow(display, frame.maximize_button);
  }

  XPoint pts[7];
  pts[0].x = (frame.button_w - 5) / 2;
  pts[0].y = (frame.button_h - 6) / 2;

  pts[1].x = 5; pts[1].y = 0;
  pts[2].x = 0; pts[2].y = 1;
  pts[3].x = -5; pts[3].y = 0;
  pts[4].x = 0; pts[4].y = 5;
  pts[5].x = 5; pts[5].y = 0;
  pts[6].x = 0; pts[6].y = -4;

  XDrawLines(display, frame.maximize_button,
	     ((focused) ? frame.ftextGC : frame.utextGC), pts, 7,
	     CoordModePrevious);
}


void BlackboxWindow::drawCloseButton(Bool pressed) {
  if (! pressed && frame.button) {
    XSetWindowBackgroundPixmap(display, frame.close_button, frame.button);
    XClearWindow(display, frame.close_button);
  } else if (frame.pbutton) {
    XSetWindowBackgroundPixmap(display, frame.close_button, frame.pbutton);
    XClearWindow(display, frame.close_button);
  }

  XPoint pts[18];
  pts[0].x = (frame.button_w - 5) / 2;
  pts[0].y = (frame.button_h - 6) / 2;
  pts[1].x = 1; pts[1].y = 0;
  pts[2].x = 3; pts[2].y = 0;
  pts[3].x = 1; pts[3].y = 0;
  pts[4].x = -4; pts[4].y = 1;
  pts[5].x = 3; pts[5].y = 0;
  pts[6].x = -2; pts[6].y = 1;
  pts[7].x = 1; pts[7].y = 0;
  pts[8].x = -1; pts[8].y = 1;
  pts[9].x = 1; pts[9].y = 0;
  pts[10].x = -1; pts[10].y = 1;
  pts[11].x = 1; pts[11].y = 0;
  pts[12].x = -2; pts[12].y = 1;
  pts[13].x = 3; pts[13].y = 0;
  pts[14].x = -4; pts[14].y = 1;
  pts[15].x = 1; pts[15].y = 0;
  pts[16].x = 3; pts[16].y = 0;
  pts[17].x = 1; pts[17].y = 0;

  XDrawPoints(display, frame.close_button,
	      ((focused) ? frame.ftextGC : frame.utextGC), pts, 18,
	      CoordModePrevious);
}


// *************************************************************************
// Window event code
// *************************************************************************

void BlackboxWindow::mapRequestEvent(XMapRequestEvent *re) {
  if (re->window == client.window) {
    if (visible && ! iconic) {
      debug->msg("changing to mapped (normal) state\n");
      XGrabServer(display);
      
      unsigned long state[2];
      state[0] = (unsigned long) NormalState;
      state[1] = (unsigned long) None;
      XChangeProperty(display, client.window, session->StateAtom(),
		      session->StateAtom(), 32, PropModeReplace,
		      (unsigned char *) state, 2);

      setFocusFlag(false);
      XMapSubwindows(display, frame.window);
      XMapRaised(display, frame.window);
      XUngrabServer(display);
    }
  }
}


void BlackboxWindow::mapNotifyEvent(XMapEvent *ne) {
  if (ne->window == client.window && (! ne->override_redirect)) {
    if (visible && ! iconic) {
      debug->msg("map notify for client - setting visible attribute\n");
      XGrabServer(display);
      
      unsigned long state[2];
      state[0] = (unsigned long) NormalState;
      state[1] = (unsigned long) None;
      XChangeProperty(display, client.window, session->StateAtom(),
		      session->StateAtom(), 32, PropModeReplace,
		      (unsigned char *) state, 2);
      
      visible = True;
      iconic = False;
      XMapSubwindows(display, frame.window);
      XMapWindow(display, frame.window);
      XUngrabServer(display);
    }
  }
}


void BlackboxWindow::unmapNotifyEvent(XUnmapEvent *ue) {
  if (ue->window == client.window) {
    XGrabServer(display);
    XSync(display, 0);

    unsigned long state[2];
    state[0] = (unsigned long) ((iconic) ? IconicState : WithdrawnState);
    state[1] = (unsigned long) None;

    visible = False;
    XUnmapWindow(display, frame.window);

    XEvent foo;
    if (! XCheckTypedWindowEvent(display, client.window, DestroyNotify,
				 &foo)) {
      XReparentWindow(display, client.window, session->Root(), client.x,
		      client.y);
      
      debug->msg("changing to unmapped state %lx %d\n", client.window,
		 visible);
      XChangeProperty(display, client.window, session->StateAtom(),
		      session->StateAtom(), 32, PropModeReplace,
		      (unsigned char *) state, 2);
    }
    
    XUngrabServer(display);
    delete this;
  }
}


void BlackboxWindow::destroyNotifyEvent(XDestroyWindowEvent *de) {
  if (de->window == client.window)
    delete this;
}


void BlackboxWindow::propertyNotifyEvent(Atom atom) {
  XTextProperty tprop;
  switch(atom) {
  case XA_WM_CLASS:
    debug->msg("wm class\n");
    if (client.app_name) XFree(client.app_name);
    if (client.app_class) XFree(client.app_class);
    XClassHint classhint;
    if (XGetClassHint(display, client.window, &classhint)) {
      client.app_name = classhint.res_name;
      client.app_class = classhint.res_class;
    }
    
    break;
  
  case XA_WM_CLIENT_MACHINE:
    debug->msg("wm client machine\n");
    if (XGetTextProperty(display, client.window, &tprop,
			 XA_WM_CLIENT_MACHINE)) {
      int n;
      XTextPropertyToStringList(&tprop, &wm_client_machine, &n);
    }

    break;
    
  case XA_WM_COMMAND:
    debug->msg("wm command\n");
    if (XGetTextProperty(display, client.window, &tprop, XA_WM_COMMAND)) {
      int n;
      XTextPropertyToStringList(&tprop, &wm_client_machine, &n);
    }
    
    break;
    
  case XA_WM_HINTS:
    debug->msg("wm hints\n");
    getWMHints();
    break;
	  
  case XA_WM_ICON_NAME:
    debug->msg("wm icon name\n");
    if (icon) icon->rereadLabel();
    break;
    
  case XA_WM_ICON_SIZE:
    debug->msg("wm icon size\n");
    break;
    
  case XA_WM_NAME:
    debug->msg ("wm name\n");
    if (client.title)
      if (strcmp(client.title, "Unnamed"))
	XFree(client.title);
    if (! XFetchName(display, client.window, &client.title))
      client.title = 0;
    XClearWindow(display, frame.title);
    debug->msg("new window title: (%s)\n", client.title);
    drawTitleWin(0, 0, frame.title_w, frame.title_h);
    session->updateWorkspace(workspace_number);
    break;
    
  case XA_WM_NORMAL_HINTS: {
    XSizeHints sizehint;
    getWMNormalHints(&sizehint);
    debug->msg("wm normal hints - %u %u %u %u\n", sizehint.width,
               sizehint.height, sizehint.max_width, sizehint.max_height);

    break;
  }
    
  case XA_WM_TRANSIENT_FOR:
    debug->msg("wm transient for\n");
    break;
    
  default:
    if (atom == session->ProtocolsAtom()) {
      debug->msg("checking protocols\n");
      getWMProtocols();
    }
  }
}


void BlackboxWindow::exposeEvent(XExposeEvent *ee) {
  if (ee->window == frame.title)
    drawTitleWin(ee->x, ee->y, ee->width, ee->height);
  else if (do_close && frame.close_button == ee->window)
    drawCloseButton(False);
  else if (do_maximize && frame.maximize_button == ee->window)
    drawMaximizeButton(False);
  else if (do_iconify && frame.iconify_button == ee->window)
    drawIconifyButton(False);
}


void BlackboxWindow::configureRequestEvent(XConfigureRequestEvent *cr) {  
  debug->msg("configure request %d %d %u x %u - frame %d %d\n"
             "     to %d %d %u %u\n", client.x, client.y, client.width,
             client.height, frame.x, frame.y, cr->x, cr->y, cr->width,
             cr->height);

  if (cr->window == client.window) {
    int cx, cy;
    unsigned int cw, ch;
    
    if (cr->value_mask & CWX) cx = cr->x - frame.border;
    else cx = frame.x;
    if (cr->value_mask & CWY) cy = cr->y - frame.border;
    else cy = frame.y;
    if (cr->value_mask & CWWidth)
      cw = cr->width + ((do_handle) ? frame.handle_w + 1 : 0);
    else cw = frame.width;
    if (cr->value_mask & CWHeight) ch = cr->height + frame.title_h + 1;
    else ch = frame.height;
    
    configureWindow(cx, cy, cw, ch);
  }
}


void BlackboxWindow::buttonPressEvent(XButtonEvent *be) {
  if (session->button1Pressed()) {
    if (frame.title == be->window || frame.handle == be->window)
      raiseWindow();
    else if (frame.iconify_button == be->window)
      drawIconifyButton(True);
    else if (frame.maximize_button == be->window)
      drawMaximizeButton(True);
    else if (frame.close_button == be->window)
      drawCloseButton(True);
    else if (frame.resize_handle == be->window)
      raiseWindow();
  } else if (session->button2Pressed()) {
    if (frame.title == be->window || frame.handle == be->window) {
      lowerWindow();
    }
  } else if (session->button3Pressed()) {
    if (frame.title == be->window) {
      if (! menu_visible) {
	window_menu->moveMenu(be->x_root - (window_menu->Width() / 2),
			      frame.y + frame.title_h);
	window_menu->showMenu();
      } else
	window_menu->hideMenu();
    }
  }
}


void BlackboxWindow::buttonReleaseEvent(XButtonEvent *re) {
  if (re->button == 1) {
    if (re->window == frame.title && moving) {
      debug->msg("ending window move %d, %d\n", client.x, client.y);
      configureWindow(frame.x, frame.y, frame.width, frame.height);
      moving = False;
      XUngrabPointer(display, CurrentTime);
    } else if (resizing) {
      debug->msg("ending window resize\n");

      XDrawLine(display, session->Root(), session->GCOperations(),
		frame.x + 1, frame.y + frame.title_h, frame.x_resize + 1,
		frame.y + frame.title_h);
      XDrawLine(display, session->Root(), session->GCOperations(),
		frame.x_resize - frame.handle_w, frame.y + frame.title_h + 1,
		frame.x_resize - frame.handle_w, frame.y_resize + 1);
      XDrawLine(display, session->Root(), session->GCOperations(),
		frame.x + 1, frame.y + 1, frame.x_resize + 1, frame.y + 1);
      XDrawLine(display, session->Root(), session->GCOperations(),
		frame.x_resize + 1, frame.y + 1, frame.x_resize + 1,
		frame.y_resize + 1);
      XDrawLine(display, session->Root(), session->GCOperations(),
		frame.x + 1, frame.y_resize + 1, frame.x_resize + 1,
		frame.y_resize + 1);
      XDrawLine(display, session->Root(), session->GCOperations(),
		frame.x + 1, frame.y + 1, frame.x + 1, frame.y_resize + 1);
      
      int dw = frame.x_resize - frame.x + 1,
	dh = frame.y_resize - frame.y + 1;
      
      debug->msg("resizing window %d %d %u %u\n", frame.x, frame.y, dw, dh);
      configureWindow(frame.x, frame.y, dw, dh);
      
      resizing = False;
      XUngrabPointer(display, CurrentTime);
    } else if (re->window == frame.iconify_button) {
      if ((re->x >= 0) && ((unsigned int) re->x <= frame.button_w) &&
	  (re->y >= 0) && ((unsigned int) re->y <= frame.button_h)) {
	iconifyWindow();
      }	else
	drawIconifyButton(False);
    } else if (re->window == frame.maximize_button) {
      if ((re->x >= 0) && ((unsigned int) re->x <= frame.button_w) &&
	  (re->y >= 0) && ((unsigned int) re->y <= frame.button_h)) {
	maximizeWindow();
      } else
	drawMaximizeButton(False);
    } else if (re->window == frame.close_button) {
      if ((re->x >= 0) && ((unsigned int) re->x <= frame.button_w) &&
	  (re->y >= 0) && ((unsigned int) re->y <= frame.button_h)) {
	closeWindow();
      } else
	drawCloseButton(False);
    }
  }
}


void BlackboxWindow::motionNotifyEvent(XMotionEvent *me) {
  if (frame.title == me->window) {
    if (session->button1Pressed()) {
      if (! moving) {
	if (menu_visible) {
	  window_menu->hideMenu();
	}

	if (XGrabPointer(display, frame.title, False, PointerMotionMask|
			 ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
			 None, session->moveCursor(), CurrentTime)
	    == GrabSuccess) {
	  moving = True;
	  client.x_move = me->x;
	  client.y_move = me->y;
	  debug->msg("starting window move %lx %d, %d\n", me->window,
			       me->x, me->y);
	} else {
	  moving =False;
	  debug->msg("couldn't grab pointer for window move\n");
	}
      } else {
	int dx = me->x_root - client.x_move,
	  dy = me->y_root - client.y_move;

	configureWindow(dx, dy, frame.width, frame.height);
      }
    }
  } else if (me->window == frame.resize_handle) {
    if (session->button1Pressed()) {
      if (! resizing) {
	if (XGrabPointer(display, frame.resize_handle, False,
			 PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
			 GrabModeAsync, None, None, CurrentTime)
	    == GrabSuccess) {
	  debug->msg("starting window resize\n");
	  resizing = True;

	  frame.x_resize = frame.x + frame.width;
	  frame.y_resize = frame.y + frame.height;

	  XDrawLine(display, session->Root(), session->GCOperations(),
		    frame.x + 1, frame.y + frame.title_h, frame.x_resize + 1,
		    frame.y + frame.title_h);
	  XDrawLine(display, session->Root(), session->GCOperations(),
		    frame.x_resize - frame.handle_w,
		    frame.y + frame.title_h + 1,
		    frame.x_resize - frame.handle_w, frame.y_resize + 1);
	  XDrawLine(display, session->Root(), session->GCOperations(),
		    frame.x + 1, frame.y + 1, frame.x_resize + 1, frame.y + 1);
	  XDrawLine(display, session->Root(), session->GCOperations(),
		    frame.x_resize + 1, frame.y + 1, frame.x_resize + 1,
		    frame.y_resize + 1);
	  XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y_resize + 1, frame.x_resize + 1,
		    frame.y_resize + 1);
	  XDrawLine(display, session->Root(), session->GCOperations(),
		    frame.x + 1, frame.y + 1, frame.x + 1, frame.y_resize + 1);
	} else {
	  resizing = False;
	  debug->msg("couldn't grab pointer for window resize\n");
	}
      } else if (resizing) {
	int dx, dy;

	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y + frame.title_h, frame.x_resize + 1,
		  frame.y + frame.title_h);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x_resize - frame.handle_w, frame.y + frame.title_h + 1,
		  frame.x_resize - frame.handle_w, frame.y_resize + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y + 1, frame.x_resize + 1, frame.y + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x_resize + 1, frame.y + 1, frame.x_resize + 1,
		  frame.y_resize + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y_resize + 1, frame.x_resize + 1,
		  frame.y_resize + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y + 1, frame.x + 1, frame.y_resize + 1);
	
	if (! client.hint_flags & PResizeInc) {
	  debug->msg("resizing by pointer position\n");
	  frame.x_resize = me->x_root;
	  frame.y_resize = me->y_root;
	} else {
	  debug->msg("resizing by increment\n");

	  // calculate the width of the client window...
	  dx = me->x_root - frame.x + 1;
	  dy = me->y_root - frame.y + 1;

	  // conform the width to the size specified by the size hints of the
	  //   client window...
	  dx -= ((dx % client.inc_w) - client.base_w);
	  dy -= ((dy % client.inc_h) - client.base_h);

          if (dx < (signed) client.min_w) dx = client.min_w;
          if (dy < (signed) client.min_h) dy = client.min_h;

	  // add the appropriate frame decoration sizes before drawing the
	  //   resizing lines...
	  frame.x_resize = dx + frame.handle_w;
	  frame.y_resize = dy + frame.title_h;
	  frame.x_resize += frame.x;
	  frame.y_resize += frame.y;
	}

	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y + frame.title_h, frame.x_resize + 1,
		  frame.y + frame.title_h);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x_resize - frame.handle_w, frame.y + frame.title_h + 1,
		  frame.x_resize - frame.handle_w, frame.y_resize + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y + 1, frame.x_resize + 1, frame.y + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x_resize + 1, frame.y + 1, frame.x_resize + 1,
		  frame.y_resize + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y_resize + 1, frame.x_resize + 1,
		  frame.y_resize + 1);
	XDrawLine(display, session->Root(), session->GCOperations(),
		  frame.x + 1, frame.y + 1, frame.x + 1, frame.y_resize + 1);
      }
    }
  }
}
