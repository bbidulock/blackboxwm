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
#include "blackbox.hh"
#include "graphics.hh"
#include "menu.hh"


// *************************************************************************
// Menu creation and destruction methods
// *************************************************************************

BaseMenu::BaseMenu(Blackbox *ctrl) {
  blackbox = ctrl;
  display = blackbox->control();
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

  menu.width = menu.title_w = menu.height = menu.title_h = menu.item_w =
    blackbox->titleFont()->ascent + blackbox->titleFont()->descent + 8;
  menu.button_h = menu.title_h - 6;
  menu.button_w = menu.button_h * 2 / 3;
  menu.label = 0;
  menu.sublevels = menu.persub = menu.use_sublevels = 0;
  menu.item_h = blackbox->menuFont()->ascent + blackbox->menuFont()->descent + 4;
  
  unsigned long attrib_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWOverrideRedirect|CWCursor|CWEventMask;
  XSetWindowAttributes attrib;
  attrib.background_pixmap = None;
  attrib.background_pixel = attrib.border_pixel = blackbox->frameColor().pixel;
  attrib.override_redirect = True;
  attrib.cursor = blackbox->sessionCursor();
  attrib.event_mask = StructureNotifyMask|SubstructureNotifyMask|
    SubstructureRedirectMask|ButtonPressMask|ButtonReleaseMask|
    ButtonMotionMask|ExposureMask;

  menu.frame =
    XCreateWindow(display, blackbox->Root(), menu.x, menu.y, menu.title_w,
		  menu.height, 1, blackbox->Depth(), InputOutput,
		  blackbox->visual(), attrib_mask, &attrib);
  blackbox->saveMenuSearch(menu.frame, this);
  
  if (show_title) {
    attrib_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|CWCursor|CWEventMask;
    attrib.background_pixel = blackbox->frameColor().pixel;
    attrib.event_mask |= EnterWindowMask|LeaveWindowMask;
    menu.title =
      XCreateWindow(display, menu.frame, 0, 0, menu.title_w, menu.height, 0,
		    blackbox->Depth(), InputOutput, blackbox->visual(),
		    attrib_mask, &attrib);
    blackbox->saveMenuSearch(menu.title, this);
  } else
    menu.title = None;

  menuitems = new llist<BaseMenuItem>;
  
  XGCValues gcv;
  gcv.foreground = blackbox->menuTextColor().pixel;
  gcv.font = blackbox->titleFont()->fid;
  titleGC = XCreateGC(display, menu.frame, GCForeground|GCFont, &gcv);

  gcv.foreground = blackbox->menuItemTextColor().pixel;
  gcv.font = blackbox->menuFont()->fid;
  itemGC = XCreateGC(display, menu.frame, GCForeground|GCFont, &gcv);

  gcv.foreground = blackbox->menuPressedTextColor().pixel;
  pitemGC = XCreateGC(display, menu.frame, GCForeground|GCFont, &gcv);

  // even though this is the end of the constructor the menu is still not
  // completely created.
}


BaseMenu::~BaseMenu(void) {
  XUnmapWindow(display, menu.frame);

  int n = menuitems->count();
  for (int i = 0; i < n; ++i)
    remove(0);

  delete menuitems;

  XFreeGC(display, titleGC);
  XFreeGC(display, itemGC);
  XFreeGC(display, pitemGC);
  if (menu.title) {
    blackbox->removeMenuSearch(menu.title);
    XDestroyWindow(display, menu.title);
  }

  blackbox->removeMenuSearch(menu.frame);
  XDestroyWindow(display, menu.frame);
  if (menu.label) delete menu.label;
}


// *************************************************************************
// insertion and removal methods
// *************************************************************************

int BaseMenu::insert(char **label) {
  BaseMenuItem *item = new BaseMenuItem(createItemWindow(), label);
  menuitems->append(item);

  return menuitems->count();
}


int BaseMenu::insert(char *label) {
  BaseMenuItem *item = new BaseMenuItem(createItemWindow(), label);
  menuitems->append(item);
  
  return menuitems->count();
}


int BaseMenu::insert(char *label, int function, char *exec) {
  int ret = 0;
  switch (function) {
  case Blackbox::B_Execute: {
    BaseMenuItem *item = new BaseMenuItem(createItemWindow(), label,
					       exec);
    menuitems->append(item);

    ret = menuitems->count();
    break; }

  case Blackbox::B_Exit:
  case Blackbox::B_Restart:
  case Blackbox::B_Reconfigure: {
    BaseMenuItem *item = new BaseMenuItem(createItemWindow(), label,
						  function);
    menuitems->append(item);

    ret = menuitems->count();
    break; }

  case Blackbox::B_RestartOther: {
    BaseMenuItem *item = new BaseMenuItem(createItemWindow(), label,
						  function, exec);
    menuitems->append(item);
    
    ret = menuitems->count();
    break; }
  
  case Blackbox::B_Shutdown:
    //    ret = insert(label, (void (*)()) blackbox->Shutdown);
    break;

  case Blackbox::B_WindowShade:
  case Blackbox::B_WindowClose:
  case Blackbox::B_WindowIconify:
  case Blackbox::B_WindowRaise:
  case Blackbox::B_WindowLower:
  case Blackbox::B_WindowMaximize:
    if (! show_title) {
      BaseMenuItem *item =
	new BaseMenuItem(createItemWindow(), label, function);
      menuitems->append(item);
      
      ret = menuitems->count();
    }
    break;
  }
  
  return ret;
}


int BaseMenu::insert(char *label, BaseMenu *submenu) {
  BaseMenuItem *item = new BaseMenuItem(createItemWindow(), label,
						submenu);
  menuitems->append(item);

  return menuitems->count();
}


int BaseMenu::remove(int index) {
  BaseMenuItem *item = menuitems->remove(index);
  blackbox->removeMenuSearch(item->window);
  XDestroyWindow(display, item->window);
  delete item;

  if (which_sub != -1)
    if ((--which_sub) == index)
      which_sub = -1;

  return menuitems->count();
}

Window BaseMenu::createItemWindow(void) {
  unsigned long attrib_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|CWCursor|
    CWEventMask;
  
  XSetWindowAttributes attrib;
  attrib.background_pixmap = None;
  attrib.background_pixel = blackbox->frameColor().pixel;
  attrib.border_pixel = None;
  attrib.override_redirect = True;
  attrib.cursor = blackbox->sessionCursor();
  attrib.event_mask = StructureNotifyMask|SubstructureNotifyMask|
    SubstructureRedirectMask|ButtonPressMask|ButtonReleaseMask|
    ButtonMotionMask|ExposureMask|EnterWindowMask|LeaveWindowMask;
  
  Window c =
    XCreateWindow(display, menu.frame, 0, (menu.item_h * menuitems->count())
		  + menu.title_h + 1, 1, menu.item_h, 0, blackbox->Depth(),
		  InputOutput, blackbox->visual(), attrib_mask, &attrib);

  blackbox->saveMenuSearch(c, this);
  return c;
}


// *************************************************************************
// Menu maintainence and utility methods
// *************************************************************************

void BaseMenu::updateMenu(void) {
  menu.item_h = blackbox->menuFont()->ascent + blackbox->menuFont()->descent + 4;
  menu.item_w = ((show_title) ?
                XTextWidth(blackbox->titleFont(),
                           ((menu.label) ? menu.label : "Blackbox Menu"),
                           strlen(((menu.label) ? menu.label :
                                   "Blackbox Menu"))) + 8 : 0);

  int ii = 0;
  llist_iterator<BaseMenuItem> it(menuitems);
  for (; it.current(); it++) {
    BaseMenuItem *itmp = it.current();
    if (itmp->ulabel || itmp->label)
      ii = XTextWidth(blackbox->menuFont(),
		      ((itmp->ulabel) ? *itmp->ulabel : itmp->label),
		      strlen((itmp->ulabel) ? *itmp->ulabel : itmp->label))
	+ 8 + menu.item_h;
    else
      ii = 0;
    menu.item_w = ((menu.item_w < (unsigned int) ii) ? ii : menu.item_w);
  }

  if (menuitems->count()) {
    if (menu.use_sublevels) {
      menu.sublevels = menu.use_sublevels;
    } else {
      menu.sublevels = 1;
      while (((menu.item_w + 1) * menuitems->count() / menu.sublevels) >
	     blackbox->XResolution())
	menu.sublevels++;
    }

    menu.persub = menuitems->count() / menu.sublevels;
    if (menuitems->count() % menu.sublevels) menu.sublevels++;
  } else {
    menu.sublevels = 0;
    menu.persub = 1;
  }

  menu.width = (menu.persub * (menu.item_w)) + (menu.persub - 1);
  menu.title_h = blackbox->titleFont()->ascent +
    blackbox->titleFont()->descent + 8;
  menu.height = ((show_title) ? menu.title_h + 1 : 0) +
    ((menu.item_h) * menu.sublevels) + (menu.sublevels - 1);

  if (show_title) {
    BImage mt_image(blackbox, menu.width, menu.title_h, blackbox->Depth(),
		    blackbox->menuColor());
    Pixmap mt_pixmap =
      mt_image.renderImage(blackbox->menuTexture(), 1, blackbox->menuColor(),
			   blackbox->menuToColor());
    
    XSetWindowBackgroundPixmap(display, menu.title, mt_pixmap);
    XClearWindow(display, menu.title);
    if (mt_pixmap) XFreePixmap(display, mt_pixmap);
  }

  BImage *mi_image = new BImage(blackbox, menu.item_w, menu.item_h,
				blackbox->Depth(), blackbox->menuItemColor());
  if (menu.item_pixmap) XFreePixmap(display, menu.item_pixmap);
  
  menu.item_pixmap = mi_image->renderImage(blackbox->menuItemTexture(), 0,
					   blackbox->menuItemColor(),
					   blackbox->menuItemToColor());
  
  if (menu.pushed_pixmap) XFreePixmap(display, menu.pushed_pixmap);
  menu.pushed_pixmap =
    mi_image->renderImage(blackbox->menuItemPressedTexture(), 0,
			  blackbox->menuItemToColor(),
			  blackbox->menuItemColor());
  delete mi_image;
  
  XResizeWindow(display, menu.frame, menu.width, menu.height);
  if (show_title) XResizeWindow(display, menu.title, menu.width,
				menu.title_h);
  XClearWindow(display, menu.frame);
  XClearWindow(display, menu.title);

  if (show_title) {
    switch (blackbox->Justification()) {
    case Blackbox::B_LeftJustify: {
      XDrawString(display, menu.title, titleGC, 3,
		  blackbox->titleFont()->ascent + 3,
		  ((menu.label) ? menu.label : "Blackbox Menu"),
		  strlen(((menu.label) ? menu.label : "Blackbox Menu")));
      break; }
    
    case Blackbox::B_RightJustify: {
      int off = XTextWidth(blackbox->titleFont(),
			   ((menu.label) ? menu.label : "Blackbox Menu"),
			   strlen(((menu.label) ? menu.label :
				   "Blackbox Menu"))) + 3;
      XDrawString(display, menu.title, titleGC, menu.width - off,
		  blackbox->titleFont()->ascent + 3,
		  ((menu.label) ? menu.label : "Blackbox Menu"),
		  strlen(((menu.label) ? menu.label : "Blackbox Menu")));
      break; }
    
    case Blackbox::B_CenterJustify: {
      int ins = (menu.width -
		 (XTextWidth(blackbox->titleFont(),
			     ((menu.label) ? menu.label : "Blackbox Menu"),
			     strlen(((menu.label) ? menu.label :
				     "Blackbox Menu"))))) / 2;
      XDrawString(display, menu.title, titleGC, ins,
		  blackbox->titleFont()->ascent + 3,
		  ((menu.label) ? menu.label : "Blackbox Menu"),
		  strlen(((menu.label) ? menu.label : "Blackbox Menu")));
      break; }
    }
  }

  it.reset();
  int i = 0, sbl = 0;
  for (; it.current(); it++, i++) {
    BaseMenuItem *itmp = it.current();
    if (itmp->sub_menu)
      itmp->sub_menu->updateMenu();

    if (i == menu.persub) { i = 0; sbl++; }

    XMoveResizeWindow(display, itmp->window, (i * (menu.item_w + 1)),
		      (sbl * (menu.item_h + 1)) +
		      ((show_title) ? menu.title_h + 1 : 0),
		      menu.item_w, menu.item_h);
    if (which_sub == i) {
      XSetWindowBackgroundPixmap(display, itmp->window, menu.pushed_pixmap);
      XClearWindow(display, itmp->window);
      XDrawString(display, itmp->window, pitemGC, 4,
		  blackbox->menuFont()->ascent + 3,
		  ((itmp->ulabel) ? *itmp->ulabel : itmp->label),
		  strlen(((itmp->ulabel) ? *itmp->ulabel : itmp->label)));
      if (itmp->sub_menu)
	XDrawRectangle(display, itmp->window, pitemGC,
		       menu.item_w - (menu.item_h - 6), 3, 3, 3);
    } else {
      XSetWindowBackgroundPixmap(display, itmp->window, menu.item_pixmap);
      XClearWindow(display, itmp->window);
      XDrawString(display, itmp->window, itemGC, 4,
		  blackbox->menuFont()->ascent + 3,
		  ((itmp->ulabel) ? *itmp->ulabel : itmp->label),
		  strlen(((itmp->ulabel) ? *itmp->ulabel : itmp->label)));
      if (itmp->sub_menu)
	XDrawRectangle(display, itmp->window, itemGC,
		       menu.item_w - (menu.item_h - 6), 3, 3, 3);
    }
  }

#ifdef SHAPE
  // this will shape the menu so that we dont have empty, left over menu item
  // spaces...

#endif

  XMapSubwindows(display, menu.frame);
  XSync(display, False);
}


void BaseMenu::showMenu(void) {
#ifdef ANIMATIONS
  Bool do_scale = True;
  int mx = menu.x + (menu.width / 2), sx = menu.width / 16,
    sy = menu.height / 16;

  if (menu.width < 16 || menu.height < 16) {
    XMoveResizeWindow(display, menu.frame, menu.x, menu.y, menu.width,
		      menu.height);
    do_scale = False;
  } else
    XMoveResizeWindow(display, menu.frame, mx, menu.y, sx, sy);
#endif
  
  XMapSubwindows(display, menu.frame);
  XMapWindow(display, menu.frame);
  visible = True;
  
#ifdef ANIMATIONS
  if (do_scale) {
    int fx = ((sx) ? sx : 1), fy = ((sy) ? sy : 1);
    for (unsigned int i = 0; i < 16; i++, fx += sx, fy += sy)
      XMoveResizeWindow(display, menu.frame, mx - (fx / 2), menu.y, fx, fy);
    XMoveResizeWindow(display, menu.frame, menu.x, menu.y, menu.width,
		      menu.height);
  }
#endif
}


void BaseMenu::hideMenu(void) {
  if (which_sub != -1) {
    BaseMenuItem *tmp = menuitems->find(which_sub);
    tmp->sub_menu->hideMenu();
    XSetWindowBackgroundPixmap(display, tmp->window, menu.item_pixmap);
    XClearWindow(display, tmp->window);
  }

#ifdef ANIMATIONS
  int mx = menu.x + (menu.width / 2), sx = menu.width / 16,
    sy =menu.height / 16;
  
  if (menu.width > 16 || menu.height > 16) {
    int fx = menu.width, fy = menu.height;
    for (unsigned int i = 0; i < 16; i++, fx -= sx, fy -= sy)
      XMoveResizeWindow(display, menu.frame, mx - (fx / 2), menu.y, fx, fy);
  }
#endif
  
  user_moved = False;
  visible = False;
  which_sub = -1;
  XUnmapWindow(display, menu.frame);
}


void BaseMenu::moveMenu(int x, int y) {
  menu.x = x;
  menu.y = y;
  XMoveWindow(display, menu.frame, x, y);
  if (which_sub != -1)
    drawSubmenu(which_sub);
}


void BaseMenu::drawSubmenu(int index) {
  if (which_sub != -1 && which_sub != index) {
    BaseMenuItem *tmp = menuitems->find(which_sub);
    tmp->sub_menu->hideMenu();
    XSetWindowBackgroundPixmap(display, tmp->window,
                               menu.item_pixmap);
    XClearWindow(display, tmp->window);
    XDrawString(display, tmp->window, itemGC, 4,
                blackbox->menuFont()->ascent + 3,
                ((tmp->ulabel) ? *tmp->ulabel : tmp->label),
                strlen(((tmp->ulabel) ? *tmp->ulabel : tmp->label)));

    if (tmp->sub_menu)
      XDrawRectangle(display, tmp->window, itemGC,
		     menu.item_w - (menu.item_h - 6), 3, 3, 3);
  }

  if (index >= 0 && index < menuitems->count()) {
    BaseMenuItem *item = menuitems->find(index);
    if (item->sub_menu) {
      int x = menu.x + ((menu.item_w + 1) * (index % menu.persub)),
	y =  menu.y + ((show_title) ? menu.title_h + 1 : 1) +
	(((index / menu.persub) + 1) * (menu.item_h + 1));
      
      if (x + item->sub_menu->Width() > blackbox->XResolution())
	x = blackbox->XResolution() - (item->sub_menu->Width() + 2);
      
      item->sub_menu->moveMenu(x, y);

      if (! item->sub_menu->visible)
	item->sub_menu->showMenu();
      item->sub_menu->visible = 3;
      sub = False;
      which_sub = index;
    } else
      which_sub = -1;
  }
}


Bool BaseMenu::hasSubmenu(int index) {
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

void BaseMenu::buttonPressEvent(XButtonEvent *be) {
  if (show_title && be->window == menu.title) {
    if (be->x >= 0 && be->x <= (signed) menu.width &&
	be->y >= 0 && be->y <= (signed) menu.title_h)
      titlePressed(be->button);
  } else {
    llist_iterator<BaseMenuItem> it(menuitems);
    for (int i = 0; it.current(); it++, i++) {
      BaseMenuItem *item = it.current();
      if (item->window == be->window) {
	if (item->sub_menu) {
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
		    blackbox->menuFont()->ascent + 3,
		    ((item->ulabel) ? *item->ulabel : item->label),
		    strlen(((item->ulabel) ? *item->ulabel : item->label)));

	if (item->sub_menu)
	  XDrawRectangle(display, item->window, pitemGC,
			 menu.item_w - (menu.item_h - 6), 3, 3, 3);
	
	itemPressed(be->button, i);
	break;
      }
    }
  }
}


void BaseMenu::buttonReleaseEvent(XButtonEvent *re) {
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
    llist_iterator<BaseMenuItem> it(menuitems);
    for (int i = 0; it.current(); it++, i++) {
      BaseMenuItem *item = it.current();
      if (item->window == re->window) {
	if (item->sub_menu) {
	  if (! sub && item->sub_menu->visible == 3) {
	    item->sub_menu->hideMenu();
	    XSetWindowBackgroundPixmap(display, re->window, menu.item_pixmap);
	    XClearWindow(display, re->window);
	    XDrawString(display, item->window, itemGC, 4,
			blackbox->menuFont()->ascent + 3,
			((item->ulabel) ? *item->ulabel : item->label),
			strlen(((item->ulabel) ? *item->ulabel :
				item->label)));
	    XDrawRectangle(display, item->window, itemGC,
			   menu.item_w - (menu.item_h - 6), 3, 3, 3);
	    which_sub = -1;
	  } else
	    sub = False;
	} else {
          if (which_sub != i) {
	    XSetWindowBackgroundPixmap(display, re->window, menu.item_pixmap);
	    XClearWindow(display, re->window);
	    XDrawString(display, item->window, itemGC, 4,
		        blackbox->menuFont()->ascent + 3,
		        ((item->ulabel) ? *item->ulabel : item->label),
		        strlen(((item->ulabel) ? *item->ulabel :
				item->label)));
	    if (item->sub_menu)
	      XDrawRectangle(display, item->window, itemGC,
			     menu.item_w - (menu.item_h - 6), 3, 3, 3);
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


void BaseMenu::motionNotifyEvent(XMotionEvent *me) {
  if (me->window == menu.title && (me->state & Button1Mask)) {
    if (movable) {
      if (! moving) {
	if (XGrabPointer(display, menu.title, False, PointerMotionMask|
			 ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
			 None, blackbox->moveCursor(), CurrentTime)
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


void BaseMenu::exposeEvent(XExposeEvent *ee) {
  if (ee->window == menu.title) {
    switch (blackbox->Justification()) {
    case Blackbox::B_LeftJustify: {
      XDrawString(display, menu.title, titleGC, 3,
		  blackbox->titleFont()->ascent + 3,
		  ((menu.label) ? menu.label : "Blackbox Menu"),
		  strlen(((menu.label) ? menu.label : "Blackbox Menu")));
      break; }

    case Blackbox::B_RightJustify: {
      int off = XTextWidth(blackbox->titleFont(),
			   ((menu.label) ? menu.label : "Blackbox Menu"),
			   strlen(((menu.label) ? menu.label :
				   "Blackbox Menu"))) + 3;
      XDrawString(display, menu.title, titleGC, menu.width - off,
		  blackbox->titleFont()->ascent + 3,
		  ((menu.label) ? menu.label : "Blackbox Menu"),
		  strlen(((menu.label) ? menu.label : "Blackbox Menu")));
      break; }

    case Blackbox::B_CenterJustify: {
      int ins = (menu.width -
		 (XTextWidth(blackbox->titleFont(),
			     ((menu.label) ? menu.label : "Blackbox Menu"),
			     strlen(((menu.label) ? menu.label :
				     "Blackbox Menu"))))) / 2;
      XDrawString(display, menu.title, titleGC, ins,
		  blackbox->titleFont()->ascent + 3,
		  ((menu.label) ? menu.label : "Blackbox Menu"),
		  strlen(((menu.label) ? menu.label : "Blackbox Menu")));
      break; }
    }
  } else {
    llist_iterator<BaseMenuItem> it(menuitems);
    for (int i = 0; it.current(); it++, i++) {
      BaseMenuItem *item = it.current();
      if (item->window == ee->window) {
	XDrawString(display, item->window,
		    ((i == which_sub) ? pitemGC : itemGC), 4,
		    blackbox->menuFont()->ascent + 3,
		    ((item->ulabel) ? *item->ulabel : item->label),
		    strlen(((item->ulabel) ? *item->ulabel : item->label)));

	if (item->sub_menu)
	  XDrawRectangle(display, item->window,
			 ((i == which_sub) ? pitemGC : itemGC),
			 menu.item_w - (menu.item_h - 6), 3, 3, 3);

	break;
      }
    }
  }
}


// *************************************************************************
// Resource reconfiguration
// *************************************************************************

void BaseMenu::Reconfigure(void) {
  XGCValues gcv;
  gcv.foreground = blackbox->menuTextColor().pixel;
  gcv.font = blackbox->titleFont()->fid;
  XChangeGC(display, titleGC, GCForeground|GCFont, &gcv);

  gcv.foreground = blackbox->menuItemTextColor().pixel;
  gcv.font = blackbox->menuFont()->fid;
  XChangeGC(display, itemGC, GCForeground|GCFont, &gcv);

  gcv.foreground = blackbox->menuPressedTextColor().pixel;
  XChangeGC(display, pitemGC, GCForeground|GCFont, &gcv);

  XSetWindowBackground(display, menu.frame, blackbox->frameColor().pixel);
  XSetWindowBorder(display, menu.frame, blackbox->frameColor().pixel);

  menu.item_h = blackbox->menuFont()->ascent + blackbox->menuFont()->descent + 4;
  updateMenu();
}
