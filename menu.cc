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

#include <stdlib.h>


/*

  Resources allocated for each menu
  Window - menu frame, title,
  linked list - menuitems
  GC - titleGC
  dynamic number of menu items

*/

BlackboxMenu::BlackboxMenu(BlackboxSession *ctrl)
{
  debug = new Debugger('*');
  session = ctrl;
  display = session->control();
  show_title = True;
  movable = True;
  moving = False;
  menu.x = 10;
  menu.y = 10;
  sub = visible = False;
  which_sub = -1;

#ifdef DEBUG
  debug->enable();
#endif
  debug->enter("BlackboxMenu constructor\n");

  menu.item_pixmap = None;
  menu.pushed_pixmap = None;

  debug->msg("calculating menu size\n");
  menu.height = menu.title_h = session->titleFont()->ascent +
    session->titleFont()->descent + 8;
  menu.button_h = menu.title_h - 6;
  menu.button_w = menu.button_h * 2 / 3;
  menu.label = "Blackbox";
  menu.width = menu.title_w = XTextWidth(session->titleFont(), menu.label,
			    strlen(menu.label)) + 8;
  menu.item_h = session->menuFont()->ascent +
    session->menuFont()->descent + 4;
  
  debug->msg("creating menu frame\n");
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
  XSaveContext(display, menu.frame, session->menuContext(), (XPointer) this);
  
  if (show_title) {
    debug->msg("creating menu title\n");
    attrib_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|CWCursor|CWEventMask;
    attrib.background_pixel = session->frameColor().pixel;
    attrib.event_mask |= EnterWindowMask|LeaveWindowMask;
    menu.title =
      XCreateWindow(display, menu.frame, 0, 0, menu.title_w, menu.height, 0,
		    session->Depth(), InputOutput, session->visual(),
		    attrib_mask, &attrib);
    XSaveContext(display, menu.title, session->menuContext(),
		 (XPointer) this);
  } else
    menu.title = None;

  debug->msg("allocing menu items linked list\n");
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

  itemContext = XUniqueContext();

  //
  // even though this is the end of the constructor the menu is still not
  // completely created.
  //
}



/*

  Resources deallocated for each menu
  Window - menu frame, title,
  linked list - menuitems
  GC - titleGC
  dynamic number of menu items - windows and contexts... then the actual item
      itself is deleted

      the linked list destroys the internal node... not the data contained in
          the node

*/

BlackboxMenu::~BlackboxMenu(void) {
  debug->msg("destroying menu\n");
  XUnmapWindow(display, menu.frame);

  int i;
  for (i = 0; i < menuitems->count(); ++i) {
    BlackboxMenuItem *itmp = menuitems->at(i);
    XDeleteContext(display, itmp->window, session->menuContext());
    XDeleteContext(display, itmp->window, itemContext);
    XDestroyWindow(display, itmp->window);
    delete itmp;
  }

  delete menuitems;

  XFreeGC(display, titleGC);
  XFreeGC(display, itemGC);
  XFreeGC(display, pitemGC);
  if (show_title) {
    XDeleteContext(display, menu.title, session->menuContext());
    XDestroyWindow(display, menu.title);
  }
  XDeleteContext(display, menu.frame, session->menuContext());
  XDestroyWindow(display, menu.frame);
  delete debug;
}


int BlackboxMenu::insert(char **label) {
  debug->msg("inserting char**\n");
  BlackboxMenuItem *item = new BlackboxMenuItem(createItemWindow(), label);
  XSaveContext(display, item->window, itemContext, (XPointer) item);
  menuitems->append(item);

  return menuitems->count();
}


int BlackboxMenu::insert(char *label) {
  debug->msg("inserting char*\n");
  BlackboxMenuItem *item = new BlackboxMenuItem(createItemWindow(), label);
  XSaveContext(display, item->window, itemContext, (XPointer) item);
  menuitems->append(item);
  
  return menuitems->count();
}


int BlackboxMenu::insert(char *label, int function, char *exec) {
  debug->msg("inserting char*, int, char*\n");
  int ret = 0;

  switch (function) {
  case BlackboxSession::B_Execute: {
    BlackboxMenuItem *item = new BlackboxMenuItem(createItemWindow(), label,
					       exec);
    XSaveContext(display, item->window, itemContext, (XPointer) item);
    menuitems->append(item);

    ret = menuitems->count();
    break; }

  case BlackboxSession::B_Exit:
  case BlackboxSession::B_Restart: {
    BlackboxMenuItem *item = new BlackboxMenuItem(createItemWindow(), label,
						  function);
    XSaveContext(display, item->window, itemContext, (XPointer) item);
    menuitems->append(item);

    ret = menuitems->count();
    break; }

  case BlackboxSession::B_Shutdown:
    //    ret = insert(label, (void (*)()) session->Shutdown);
    break;

  case BlackboxSession::B_WindowClose:
  case BlackboxSession::B_WindowIconify:
  case BlackboxSession::B_WindowRaise:
  case BlackboxSession::B_WindowLower:
  case BlackboxSession::B_WindowMaximize:
    if (! show_title) {
      BlackboxMenuItem *item =
	new BlackboxMenuItem(createItemWindow(), label, function);
      XSaveContext(display, item->window, itemContext, (XPointer) item);
      menuitems->append(item);
      
      ret = menuitems->count();
    }
    break;
  }
  
  return ret;
}


int BlackboxMenu::insert(char *label, BlackboxMenu *submenu) {
  debug->msg("inserting sub menu\n");
  BlackboxMenuItem *item = new BlackboxMenuItem(createItemWindow(), label,
						submenu);
  menuitems->append(item);

  return menuitems->count();
}


int BlackboxMenu::remove(char *) {
  // i hope i never need this function
  return 0;
}


int BlackboxMenu::remove(int index) {
  debug->msg("removing item %d\n", index);
  BlackboxMenuItem *item = menuitems->at(index);
  menuitems->remove(index);

  XDeleteContext(display, item->window, itemContext);
  XDestroyWindow(display, item->window);
  delete item;

  return menuitems->count();
}


void BlackboxMenu::updateMenu(void) {
  debug->msg("updating menu\n");
  int i = 0, ii = 0;
  BlackboxMenuItem *itmp = NULL;

  debug->msg("recalculating menu size\n");
  menu.width =  XTextWidth(session->titleFont(), menu.label,
                           strlen(menu.label)) + 8;
  for (i = 0; i < menuitems->count(); ++i) {
    itmp = menuitems->at(i);
    ii = XTextWidth(session->menuFont(),
                    ((itmp->ulabel) ? *itmp->ulabel : itmp->label),
                    strlen((itmp->ulabel) ? *itmp->ulabel : itmp->label)) + 8;
    menu.width = ((menu.width < (unsigned int) ii) ? ii : menu.width);
  }

  if (show_title) {
    debug->msg("rendering title image\n");
    BImage mt_image(session, menu.width, menu.title_h, session->Depth(),
		    session->menuColor());
    Pixmap mt_pixmap =
      mt_image.renderImage(session->menuTexture(), 1, session->menuColor(),
			   session->menuToColor());
    
    debug->msg("blitting title image\n");
    XSetWindowBackgroundPixmap(display, menu.title, mt_pixmap);
    XClearWindow(display, menu.title);
    debug->msg("freeing title image resources and drawing label\n");
    if (mt_pixmap) XFreePixmap(display, mt_pixmap);
    XDrawString(display, menu.title, titleGC, 3, session->titleFont()->ascent
		+ 3, menu.label , strlen(menu.label));
  }

  debug->msg("rendering item images\n");
  BImage mi_image(session, menu.width, menu.item_h, session->Depth(),
		  session->menuItemColor());  
  if (menu.item_pixmap) XFreePixmap(display, menu.item_pixmap);

  menu.item_pixmap = mi_image.renderImage(session->menuItemTexture(), 0,
					  session->menuItemColor(),
					  session->menuItemToColor());
  
  debug->msg("rendering pushed item image\n");
  if (menu.pushed_pixmap) XFreePixmap(display, menu.pushed_pixmap);
  menu.pushed_pixmap =
    mi_image.renderImage(session->menuItemPressedTexture(), 0,
			 session->menuItemToColor(),
			 session->menuItemColor());

  XGrabServer(display);
  menu.height = ((show_title) ? menu.title_h : 0) +
    (menu.item_h * menuitems->count());
  debug->msg("resizing menu\n");
  XResizeWindow(display, menu.frame, menu.width, menu.height);
  if (show_title) XResizeWindow(display, menu.title, menu.width,
				menu.title_h);
  XClearWindow(display, menu.frame);
  XClearWindow(display, menu.title);

  debug->msg("drawing menu title and item labels\n");
  XDrawString(display, menu.title, titleGC, 3, session->titleFont()->ascent
	      + 3, menu.label, strlen(menu.label));

  for (i = 0; i < menuitems->count(); ++i) {
    itmp = menuitems->at(i);
    if (which_sub == i)
      XSetWindowBackgroundPixmap(display, itmp->window, menu.pushed_pixmap);
    else
      XSetWindowBackgroundPixmap(display, itmp->window, menu.item_pixmap);

    XMoveResizeWindow(display, itmp->window, 0, (i * (menu.item_h - 1)) +
		      ((show_title) ? menu.title_h : 0) + i,
		      menu.width + 8, menu.item_h);
    XClearWindow(display, itmp->window);
    XDrawString(display, itmp->window, itemGC, 4,
		session->menuFont()->ascent + 2,
		((itmp->ulabel) ? *itmp->ulabel : itmp->label),
		strlen(((itmp->ulabel) ? *itmp->ulabel : itmp->label)));
  }

  XMapSubwindows(display, menu.frame);
  XFlush(display);
  XSync(display, 0);
  XUngrabServer(display);
}


void BlackboxMenu::showMenu(void) {
  debug->msg("showing menu\n");
  XGrabServer(display);
  XMapSubwindows(display, menu.frame);
  XMapRaised(display, menu.frame);
  visible = True;
  XUngrabServer(display);
}


void BlackboxMenu::raiseMenu(void) {
  debug->msg("raising menu\n");
  if (menu.x < 0)
    menu.x = 0;
  if ((unsigned) menu.x >= (session->XResolution() - menu.width))
    menu.x = session->XResolution() - menu.width;
	
  if (menu.y < 0)
    menu.y = 0;
  if ((unsigned) menu.y >= (session->YResolution() - menu.height))
    menu.y = session->YResolution() - menu.height - 1;

  moveMenu(menu.x, menu.y);
  XRaiseWindow(display, menu.frame);
}


void BlackboxMenu::buttonPressEvent(XButtonEvent *be) {
  debug->msg("menu button press\n");
  if (show_title && be->window == menu.title) {
    if (be->button == 1) XRaiseWindow(display, menu.frame);
    else if (be->button == 2) XLowerWindow(display, menu.frame);
    titlePressed(be->button);
  } else {
    int i;

    for (i = 0; i < menuitems->count(); ++i) {
      BlackboxMenuItem *item = menuitems->at(i);
      if (item->window == be->window) {
	if (be->button == 3 && item->sub_menu) {
	  if (! item->sub_menu->visible) {
	    if (which_sub != -1) {
	      BlackboxMenuItem *tmp = menuitems->at(which_sub);
	      tmp->sub_menu->hideMenu();
	      XSetWindowBackgroundPixmap(display, tmp->window,
					 menu.item_pixmap);
	      XClearWindow(display, tmp->window);
	      XDrawString(display, tmp->window, itemGC, 4,
			  session->menuFont()->ascent + 2,
			  ((tmp->ulabel) ? *tmp->ulabel : tmp->label),
			  strlen(((tmp->ulabel) ? *tmp->ulabel : tmp->label)));
	    }
	    
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
	
	itemPressed(be->button, i);
	break;
      }
    }
  }
}


void BlackboxMenu::buttonReleaseEvent(XButtonEvent *re) {
  debug->msg("menu button release\n");
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
    int i;

    for (i = 0; i < menuitems->count(); ++i) {
      BlackboxMenuItem *item = menuitems->at(i);
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
  debug->msg("menu motion notify\n");
  if (me->window == menu.title && session->button1Pressed()) {
    if (movable) {
      if (! moving) {
	if (XGrabPointer(display, menu.title, False, PointerMotionMask|
			 ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
			 None, session->moveCursor(), CurrentTime)
	    == GrabSuccess) {
	  moving = 1;
	  menu.x_move = me->x;
	  menu.y_move = me->y;
	  if (which_sub != -1)
	  menuitems->at(which_sub)->sub_menu->hideMenu();
	} else
	  moving = False;	
      } else {
	int dx = me->x_root - menu.x_move,
	  dy = me->y_root - menu.y_move;
	menu.x = dx;
	menu.y = dy;
	XMoveWindow(display, menu.frame, dx, dy);
      }
    }
  }
}


void BlackboxMenu::exposeEvent(XExposeEvent *ee) {
  debug->msg("expose... redrawing menu title and item labels\n");
  if (ee->window == menu.title) {
    XDrawString(display, menu.title, titleGC, 3, session->titleFont()->ascent
		+ 3, menu.label, strlen(menu.label));
  } else {
    int i;

    for (i = 0; i < menuitems->count(); ++i) {
      BlackboxMenuItem *item = menuitems->at(i);
      if (item->window == ee->window) {
	XDrawString(display, item->window, itemGC, 4,
		    session->menuFont()->ascent + 2,
		    ((item->ulabel) ? *item->ulabel : item->label),
		    strlen(((item->ulabel) ? *item->ulabel : item->label)));

	break;
      }
    }
  }
}


Window BlackboxMenu::createItemWindow(void) {
  debug->msg("creating item window\n");
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

  XSaveContext(display, c, session->menuContext(), (XPointer) this);
  return c;
}


void BlackboxMenu::hideMenu(void) {
  debug->msg("hiding menu\n");
  XGrabServer(display);
  XUnmapWindow(display, menu.frame);
  visible = False;
  if (which_sub != -1) {
    BlackboxMenuItem *tmp = menuitems->at(which_sub);
    tmp->sub_menu->hideMenu();
    XSetWindowBackgroundPixmap(display, tmp->window, menu.item_pixmap);
    XClearWindow(display, tmp->window);
  }
  which_sub = -1;
  XUngrabServer(display);
}


void BlackboxMenu::moveMenu(int x, int y) {
  debug->msg("moving menu\n");
  XGrabServer(display);
  menu.x = x;
  menu.y = y;
  XMoveWindow(display, menu.frame, x, y);
  XUngrabServer(display);
}


Window BlackboxMenu::windowID(void) { return menu.frame; }
void BlackboxMenu::setMenuLabel(char *n) { menu.label = n; }
void BlackboxMenu::showTitle(void) { show_title = True; }
void BlackboxMenu::hideTitle(void) { show_title = False; }
void BlackboxMenu::setMovable(Bool b) { movable = b; }
BlackboxMenuItem *BlackboxMenu::at(int i) { return menuitems->at(i); }
BlackboxSession *BlackboxMenu::Session(void) { return session; }


void BlackboxMenu::drawSubmenu(int index) {
  if (index >= 0 && index < menuitems->count()) {
    BlackboxMenuItem *item = menuitems->at(index);
    item->sub_menu->moveMenu(menu.x + menu.width + 1,
			     menu.y + ((show_title) ? menu.title_h + 1 : 0) +
			     (index * menu.item_h) -
			     ((item->sub_menu->show_title) ?
			      item->sub_menu->menu.title_h + 1 : 0));
    item->sub_menu->showMenu();
    item->sub_menu->visible = 3;
    sub = False;
    which_sub = index;
  }
}


Bool BlackboxMenu::hasSubmenu(int index) {
  if ((index >= 0) && (index < menuitems->count()))
    if (menuitems->at(index)->sub_menu)
      return True;
    else
      return False;
  else
    return False;
}
