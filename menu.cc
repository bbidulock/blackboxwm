//
// menu.cc for Blackbox - an X11 Window manager
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
#include "menu.hh"
#include "graphics.hh"
#include "session.hh"


// *************************************************************************
// Menu creation and destruction methods
// *************************************************************************
//
// allocations:
// Window menu.frame, menu.title, item windows (contexts)
// llist *menuitems
// GC itemGC, pitemGC, titleGC
// char *menu.label
//
// *************************************************************************

BlackboxMenu::BlackboxMenu(BlackboxSession *ctrl) {
  session = ctrl;
  display = session->control();
  show_title = True;
  movable = True;
  moving = False;
  user_moved = False;
  menu.x = 10;
  menu.y = 10;
  sub = visible = False;
  which_sub = -1;

  menu.item_pixmap = None;
  menu.pushed_pixmap = None;

  menu.width = menu.title_w = menu.height = menu.title_h =
    session->titleFont()->ascent + session->titleFont()->descent + 8;
  menu.button_h = menu.title_h - 6;
  menu.button_w = menu.button_h * 2 / 3;
  menu.label = 0;

  menu.item_h = session->menuFont()->ascent + session->menuFont()->descent + 4;
  
  unsigned long attrib_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWOverrideRedirect|CWCursor|CWEventMask;
  XSetWindowAttributes attrib;
  attrib.background_pixmap = None;
  attrib.background_pixel = attrib.border_pixel = session->frameColor().pixel;
  attrib.override_redirect = True;
  attrib.cursor = session->sessionCursor();
  attrib.event_mask = StructureNotifyMask|SubstructureNotifyMask|
    SubstructureRedirectMask|ButtonPressMask|ButtonReleaseMask|
    ButtonMotionMask|ExposureMask;

  menu.frame =
    XCreateWindow(display, session->Root(), menu.x, menu.y, menu.title_w,
		  menu.height, 1, session->Depth(), InputOutput,
		  session->visual(), attrib_mask, &attrib);
  session->saveMenuSearch(menu.frame, this);
  
  if (show_title) {
    attrib_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|CWCursor|CWEventMask;
    attrib.background_pixel = session->frameColor().pixel;
    attrib.event_mask |= EnterWindowMask|LeaveWindowMask;
    menu.title =
      XCreateWindow(display, menu.frame, 0, 0, menu.title_w, menu.height, 0,
		    session->Depth(), InputOutput, session->visual(),
		    attrib_mask, &attrib);
    session->saveMenuSearch(menu.title, this);
  } else
    menu.title = None;

  menuitems = new llist<BlackboxMenuItem>;
  
  XGCValues gcv;
  gcv.foreground = session->menuTextColor().pixel;
  gcv.font = session->titleFont()->fid;
  titleGC = XCreateGC(display, menu.frame, GCForeground|GCFont, &gcv);

  gcv.foreground = session->menuItemTextColor().pixel;
  gcv.font = session->menuFont()->fid;
  itemGC = XCreateGC(display, menu.frame, GCForeground|GCFont, &gcv);

  gcv.foreground = session->menuPressedTextColor().pixel;
  pitemGC = XCreateGC(display, menu.frame, GCForeground|GCFont, &gcv);

  // even though this is the end of the constructor the menu is still not
  // completely created.
}


BlackboxMenu::~BlackboxMenu(void) {
  XUnmapWindow(display, menu.frame);

  int n = menuitems->count();
  for (int i = 0; i < n; ++i)
    remove(0);

  delete menuitems;

  XFreeGC(display, titleGC);
  XFreeGC(display, itemGC);
  XFreeGC(display, pitemGC);
  if (menu.title) {
    session->removeMenuSearch(menu.title);
    XDestroyWindow(display, menu.title);
  }

  session->removeMenuSearch(menu.frame);
  XDestroyWindow(display, menu.frame);
  if (menu.label) delete menu.label;
}


// *************************************************************************
// insertion and removal methods
// *************************************************************************

int BlackboxMenu::insert(char **label) {
  BlackboxMenuItem *item = new BlackboxMenuItem(createItemWindow(), label);
  menuitems->append(item);

  return menuitems->count();
}


int BlackboxMenu::insert(char *label) {
  BlackboxMenuItem *item = new BlackboxMenuItem(createItemWindow(), label);
  menuitems->append(item);
  
  return menuitems->count();
}


int BlackboxMenu::insert(char *label, int function, char *exec) {
  int ret = 0;
  switch (function) {
  case BlackboxSession::B_Execute: {
    BlackboxMenuItem *item = new BlackboxMenuItem(createItemWindow(), label,
					       exec);
    menuitems->append(item);

    ret = menuitems->count();
    break; }

  case BlackboxSession::B_Exit:
  case BlackboxSession::B_Restart:
  case BlackboxSession::B_Reconfigure: {
    BlackboxMenuItem *item = new BlackboxMenuItem(createItemWindow(), label,
						  function);
    menuitems->append(item);

    ret = menuitems->count();
    break; }

  case BlackboxSession::B_RestartOther: {
    BlackboxMenuItem *item = new BlackboxMenuItem(createItemWindow(), label,
						  function, exec);
    menuitems->append(item);
    
    ret = menuitems->count();
    break; }
  
  case BlackboxSession::B_Shutdown:
    //    ret = insert(label, (void (*)()) session->Shutdown);
    break;

  case BlackboxSession::B_WindowShade:
  case BlackboxSession::B_WindowClose:
  case BlackboxSession::B_WindowIconify:
  case BlackboxSession::B_WindowRaise:
  case BlackboxSession::B_WindowLower:
  case BlackboxSession::B_WindowMaximize:
    if (! show_title) {
      BlackboxMenuItem *item =
	new BlackboxMenuItem(createItemWindow(), label, function);
      menuitems->append(item);
      
      ret = menuitems->count();
    }
    break;
  }
  
  return ret;
}


int BlackboxMenu::insert(char *label, BlackboxMenu *submenu) {
  BlackboxMenuItem *item = new BlackboxMenuItem(createItemWindow(), label,
						submenu);
  menuitems->append(item);

  return menuitems->count();
}


int BlackboxMenu::remove(int index) {
  BlackboxMenuItem *item = menuitems->remove(index);
  session->removeMenuSearch(item->window);
  XDestroyWindow(display, item->window);
  delete item;

  if (which_sub != -1)
    if ((--which_sub) == index)
      which_sub = -1;

  return menuitems->count();
}

Window BlackboxMenu::createItemWindow(void) {
  unsigned long attrib_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|CWCursor|
    CWEventMask;
  
  XSetWindowAttributes attrib;
  attrib.background_pixmap = None;
  attrib.background_pixel = session->frameColor().pixel;
  attrib.border_pixel = None;
  attrib.override_redirect = True;
  attrib.cursor = session->sessionCursor();
  attrib.event_mask = StructureNotifyMask|SubstructureNotifyMask|
    SubstructureRedirectMask|ButtonPressMask|ButtonReleaseMask|
    ButtonMotionMask|ExposureMask|EnterWindowMask|LeaveWindowMask;
  
  Window c =
    XCreateWindow(display, menu.frame, 0, (menu.item_h * menuitems->count())
		  + menu.title_h + 1, 1, menu.item_h, 0, session->Depth(),
		  InputOutput, session->visual(), attrib_mask, &attrib);

  session->saveMenuSearch(c, this);
  return c;
}


// *************************************************************************
// Menu maintainence and utility methods
// *************************************************************************

void BlackboxMenu::updateMenu(void) {
  menu.item_h = session->menuFont()->ascent + session->menuFont()->descent + 4;
  menu.width = ((show_title) ?
                XTextWidth(session->titleFont(),
                           ((menu.label) ? menu.label : "Blackbox Menu"),
                           strlen(((menu.label) ? menu.label :
                                   "Blackbox Menu"))) + 8 : 0);

  int ii = 0;
  llist_iterator<BlackboxMenuItem> it(menuitems);
  for (; it.current(); it++) {
    BlackboxMenuItem *itmp = it.current();
    if (itmp->ulabel || itmp->label)
      ii = XTextWidth(session->menuFont(),
		      ((itmp->ulabel) ? *itmp->ulabel : itmp->label),
		      strlen((itmp->ulabel) ? *itmp->ulabel : itmp->label))
	+ 8 + menu.item_h;
    else
      ii = 0;
    menu.width = ((menu.width < (unsigned int) ii) ? ii : menu.width);
  }

  if (show_title) {
    BImage mt_image(session, menu.width, menu.title_h, session->Depth(),
		    session->menuColor());
    Pixmap mt_pixmap =
      mt_image.renderImage(session->menuTexture(), 1, session->menuColor(),
			   session->menuToColor());
    
    XSetWindowBackgroundPixmap(display, menu.title, mt_pixmap);
    XClearWindow(display, menu.title);
    if (mt_pixmap) XFreePixmap(display, mt_pixmap);
  }

  BImage *mi_image = new BImage(session, menu.width, menu.item_h,
				session->Depth(), session->menuItemColor());
  if (menu.item_pixmap) XFreePixmap(display, menu.item_pixmap);
  
  menu.item_pixmap = mi_image->renderImage(session->menuItemTexture(), 0,
					   session->menuItemColor(),
					   session->menuItemToColor());
  
  if (menu.pushed_pixmap) XFreePixmap(display, menu.pushed_pixmap);
  menu.pushed_pixmap =
    mi_image->renderImage(session->menuItemPressedTexture(), 0,
			  session->menuItemToColor(),
			  session->menuItemColor());
  delete mi_image;
  
  menu.height = ((show_title) ? menu.title_h : 0) +
    (menu.item_h * menuitems->count());
  XResizeWindow(display, menu.frame, menu.width, menu.height);
  if (show_title) XResizeWindow(display, menu.title, menu.width,
				menu.title_h);
  XClearWindow(display, menu.frame);
  XClearWindow(display, menu.title);

  if (show_title)
    XDrawString(display, menu.title, titleGC, 3,
		session->titleFont()->ascent + 3,
		((menu.label) ? menu.label : "Blackbox Menu"),
		strlen(((menu.label) ? menu.label : "Blackbox Menu")));

  it.reset();
  for (int i = 0; it.current(); it++, i++) {
    BlackboxMenuItem *itmp = it.current();
    if (itmp->sub_menu)
      itmp->sub_menu->updateMenu();

    if (which_sub == i) {
      XSetWindowBackgroundPixmap(display, itmp->window, menu.pushed_pixmap);
      XMoveResizeWindow(display, itmp->window, 0, (i * (menu.item_h - 1)) +
			((show_title) ? menu.title_h : 0) + i,
			menu.width + 8, menu.item_h);
      XClearWindow(display, itmp->window);
      XDrawString(display, itmp->window, pitemGC, 4,
		  session->menuFont()->ascent + 2,
		  ((itmp->ulabel) ? *itmp->ulabel : itmp->label),
		  strlen(((itmp->ulabel) ? *itmp->ulabel : itmp->label)));
      if (itmp->sub_menu)
	XDrawRectangle(display, itmp->window, pitemGC,
		       menu.width - (menu.item_h - 6), 3, 3, 3);
    } else {
      XSetWindowBackgroundPixmap(display, itmp->window, menu.item_pixmap);
      XMoveResizeWindow(display, itmp->window, 0, (i * (menu.item_h - 1)) +
			((show_title) ? menu.title_h : 0) + i,
			menu.width + 8, menu.item_h);
      XClearWindow(display, itmp->window);
      XDrawString(display, itmp->window, itemGC, 4,
		  session->menuFont()->ascent + 2,
		  ((itmp->ulabel) ? *itmp->ulabel : itmp->label),
		  strlen(((itmp->ulabel) ? *itmp->ulabel : itmp->label)));
      if (itmp->sub_menu)
	XDrawRectangle(display, itmp->window, itemGC,
		       menu.width - (menu.item_h - 6), 3, 3, 3);
    }
  }

  XMapSubwindows(display, menu.frame);
  XSync(display, False);
}


void BlackboxMenu::showMenu(void) {
  XMapSubwindows(display, menu.frame);
  XMapWindow(display, menu.frame);
  visible = True;
}


void BlackboxMenu::hideMenu(void) {
  if (which_sub != -1) {
    BlackboxMenuItem *tmp = menuitems->find(which_sub);
    tmp->sub_menu->hideMenu();
    XSetWindowBackgroundPixmap(display, tmp->window, menu.item_pixmap);
    XClearWindow(display, tmp->window);
  }

  user_moved = False;
  visible = False;
  which_sub = -1;
  XUnmapWindow(display, menu.frame);
}


void BlackboxMenu::moveMenu(int x, int y) {
  menu.x = x;
  menu.y = y;
  XMoveWindow(display, menu.frame, x, y);
  if (which_sub != -1)
    drawSubmenu(which_sub);
}


void BlackboxMenu::drawSubmenu(int index) {
  if (which_sub != -1 && which_sub != index) {
    BlackboxMenuItem *tmp = menuitems->find(which_sub);
    tmp->sub_menu->hideMenu();
    XSetWindowBackgroundPixmap(display, tmp->window,
                               menu.item_pixmap);
    XClearWindow(display, tmp->window);
    XDrawString(display, tmp->window, itemGC, 4,
                session->menuFont()->ascent + 2,
                ((tmp->ulabel) ? *tmp->ulabel : tmp->label),
                strlen(((tmp->ulabel) ? *tmp->ulabel : tmp->label)));

    if (tmp->sub_menu)
      XDrawRectangle(display, tmp->window, itemGC,
		     menu.width - (menu.item_h - 6), 3, 3, 3);
  }

  if (index >= 0 && index < menuitems->count()) {
    BlackboxMenuItem *item = menuitems->find(index);
    if (item->sub_menu) {
      int x = menu.x + menu.width + 1,
	y =  menu.y + ((show_title) ? menu.title_h + 1 : 0) +
	(index * menu.item_h) -
	((item->sub_menu->show_title) ?
	 item->sub_menu->menu.title_h + 1 : 0);

      if (x + item->sub_menu->Width() > session->XResolution())
	x -= (menu.width + item->sub_menu->Width() + 2);

      item->sub_menu->moveMenu(x, y);
      item->sub_menu->showMenu();
      item->sub_menu->visible = 3;
      sub = False;
      which_sub = index;
    } else
      which_sub = -1;
  }
}


Bool BlackboxMenu::hasSubmenu(int index) {
  if ((index >= 0) && (index < menuitems->count()))
    if (menuitems->find(index)->sub_menu)
      return True;
    else
      return False;
  else
    return False;
}


// *************************************************************************
// Menu event handling
// *************************************************************************

void BlackboxMenu::buttonPressEvent(XButtonEvent *be) {
  if (show_title && be->window == menu.title) {
    if (be->x >= 0 && be->x <= (signed) menu.width &&
	be->y >= 0 && be->y <= (signed) menu.title_h)
      titlePressed(be->button);
  } else {
    llist_iterator<BlackboxMenuItem> it(menuitems);
    for (int i = 0; it.current(); it++, i++) {
      BlackboxMenuItem *item = it.current();
      if (item->window == be->window) {
	if (be->button == 3 && item->sub_menu) {
	  if (! item->sub_menu->visible) {
	    XSetWindowBackgroundPixmap(display, be->window,
				       menu.pushed_pixmap);
	    XClearWindow(display, be->window);
	    drawSubmenu(i);
	    sub = True;
	  }
	} else {
	  XSetWindowBackgroundPixmap(display, be->window, menu.pushed_pixmap);
	  XClearWindow(display, be->window);
	}

	XDrawString(display, item->window, pitemGC, 4,
		    session->menuFont()->ascent + 2,
		    ((item->ulabel) ? *item->ulabel : item->label),
		    strlen(((item->ulabel) ? *item->ulabel : item->label)));

	if (item->sub_menu)
	  XDrawRectangle(display, item->window, pitemGC,
			 menu.width - (menu.item_h - 6), 3, 3, 3);
	
	itemPressed(be->button, i);
	break;
      }
    }
  }
}


void BlackboxMenu::buttonReleaseEvent(XButtonEvent *re) {
  if (show_title && re->window == menu.title) {
    if (moving) {
      XUngrabPointer(display, CurrentTime);
      moving = False;
      if (which_sub != -1)
	drawSubmenu(which_sub);
    }

    if (re->x >= 0 && re->x <= (signed) menu.width &&
	re->y >= 0 && re->y <= (signed) menu.title_h)
      titleReleased(re->button);
  } else {
    llist_iterator<BlackboxMenuItem> it(menuitems);
    for (int i = 0; it.current(); it++, i++) {
      BlackboxMenuItem *item = it.current();
      if (item->window == re->window) {
	if (re->button == 3 && item->sub_menu) {
	  if (! sub && item->sub_menu->visible == 3) {
	    item->sub_menu->hideMenu();
	    XSetWindowBackgroundPixmap(display, re->window, menu.item_pixmap);
	    XClearWindow(display, re->window);
	    XDrawString(display, item->window, itemGC, 4,
			session->menuFont()->ascent + 2,
			((item->ulabel) ? *item->ulabel : item->label),
			strlen(((item->ulabel) ? *item->ulabel :
				item->label)));
	    XDrawRectangle(display, item->window, itemGC,
			   menu.width - (menu.item_h - 6), 3, 3, 3);
	    which_sub = -1;
	  } else
	    sub = False;
	} else {
          if (which_sub != i) {
	    XSetWindowBackgroundPixmap(display, re->window, menu.item_pixmap);
	    XClearWindow(display, re->window);
	    XDrawString(display, item->window, itemGC, 4,
		        session->menuFont()->ascent + 2,
		        ((item->ulabel) ? *item->ulabel : item->label),
		        strlen(((item->ulabel) ? *item->ulabel :
				item->label)));
	    if (item->sub_menu)
	      XDrawRectangle(display, item->window, itemGC,
			     menu.width - (menu.item_h - 6), 3, 3, 3);
	  }
        }

	if (re->x > 0 && re->x < (int) menu.width &&
	    re->y > 0 && re->y < (int) menu.item_h) 
	  itemReleased(re->button, i);
	
	break;
      }
    }
  }
}


void BlackboxMenu::motionNotifyEvent(XMotionEvent *me) {
  if (me->window == menu.title && session->button1Pressed()) {
    if (movable) {
      if (! moving) {
	if (XGrabPointer(display, menu.title, False, PointerMotionMask|
			 ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
			 None, session->moveCursor(), CurrentTime)
	    == GrabSuccess) {
	  moving = True;
	  user_moved = True;
	  menu.x_move = me->x;
	  menu.y_move = me->y;
	  if (which_sub != -1)
	    drawSubmenu(which_sub);
	} else
	  moving = False;	
      } else {
	int dx = me->x_root - menu.x_move,
	  dy = me->y_root - menu.y_move;
	menu.x = dx;
	menu.y = dy;
	XMoveWindow(display, menu.frame, dx, dy);
	  if (which_sub != -1)
	    drawSubmenu(which_sub);
      }
    }
  }
}


void BlackboxMenu::exposeEvent(XExposeEvent *ee) {
  if (ee->window == menu.title) {
    XDrawString(display, menu.title, titleGC, 3,
		session->titleFont()->ascent + 3,
		((menu.label) ? menu.label : "Blackbox Menu"),
		strlen(((menu.label) ? menu.label : "Blackbox Menu")));
  } else {
    llist_iterator<BlackboxMenuItem> it(menuitems);
    for (int i = 0; it.current(); it++, i++) {
      BlackboxMenuItem *item = it.current();
      if (item->window == ee->window) {
	XDrawString(display, item->window,
		    ((i == which_sub) ? pitemGC : itemGC), 4,
		    session->menuFont()->ascent + 2,
		    ((item->ulabel) ? *item->ulabel : item->label),
		    strlen(((item->ulabel) ? *item->ulabel : item->label)));

	if (item->sub_menu)
	  XDrawRectangle(display, item->window,
			 ((i == which_sub) ? pitemGC : itemGC),
			 menu.width - (menu.item_h - 6), 3, 3, 3);

	break;
      }
    }
  }
}


// *************************************************************************
// Resource reconfiguration
// *************************************************************************

void BlackboxMenu::Reconfigure(void) {
  XGCValues gcv;
  gcv.foreground = session->menuTextColor().pixel;
  gcv.font = session->titleFont()->fid;
  XChangeGC(display, titleGC, GCForeground|GCFont, &gcv);

  gcv.foreground = session->menuItemTextColor().pixel;
  gcv.font = session->menuFont()->fid;
  XChangeGC(display, itemGC, GCForeground|GCFont, &gcv);

  gcv.foreground = session->menuPressedTextColor().pixel;
  XChangeGC(display, pitemGC, GCForeground|GCFont, &gcv);

  XSetWindowBackground(display, menu.frame, session->frameColor().pixel);
  XSetWindowBorder(display, menu.frame, session->frameColor().pixel);

  menu.item_h = session->menuFont()->ascent + session->menuFont()->descent + 4;
  updateMenu();
}
