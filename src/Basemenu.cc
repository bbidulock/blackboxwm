// Basemenu.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 1999 by Brad Hughes, bhughes@tcac.net
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

#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "blackbox.hh"
#include "Basemenu.hh"
#include "Screen.hh"

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS


static Basemenu *shown = (Basemenu *) 0;

Basemenu::Basemenu(Blackbox *ctrl, BScreen *scrn) {
  blackbox = ctrl;
  screen = scrn;
  image_ctrl = screen->getImageControl();
  display = blackbox->getDisplay();
  parent = (Basemenu *) 0;
  alignment = AlignDontCare;

  title_vis = movable = hidable = True;
  shifted = default_menu = moving = user_moved = visible = False;
  menu.x = menu.y = menu.x_shift = menu.y_shift = menu.x_move =
    menu.y_move = 0;
  always_highlight = which_sub = which_press = which_sbl = -1;
  indicator = screen->getBulletStyle();
  indicator_position = screen->getBulletPosition();

  menu.iframe_pixmap = menu.title_pixmap = None;

  menu.bevel_w = screen->getBevelWidth();
  menu.width = menu.title_h = menu.item_w = menu.iframe_h =
    screen->getTitleFont()->ascent + screen->getTitleFont()->descent +
    (menu.bevel_w * 2);
  menu.label = 0;
  menu.sublevels = menu.persub = menu.minsub = 0;
  menu.item_h = screen->getMenuFont()->ascent +
    screen->getMenuFont()->descent + (menu.bevel_w);
  menu.height = menu.title_h + screen->getBorderWidth() + menu.iframe_h;
  
  unsigned long attrib_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
    CWOverrideRedirect | CWEventMask;
  XSetWindowAttributes attrib;
  attrib.background_pixmap = None;
  attrib.background_pixel = attrib.border_pixel =
    screen->getBorderColor()->getPixel();
  attrib.override_redirect = True;
  attrib.event_mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
    ExposureMask;

  menu.frame =
    XCreateWindow(display, screen->getRootWindow(), menu.x, menu.y, menu.width,
		  menu.height, screen->getBorderWidth(), screen->getDepth(),
                  InputOutput, screen->getVisual(), attrib_mask, &attrib);
  blackbox->saveMenuSearch(menu.frame, this);
  
  attrib_mask = CWBackPixmap | CWBackPixel | CWBorderPixel | CWEventMask;
  attrib.background_pixel = screen->getBorderColor()->getPixel();
  attrib.event_mask |= EnterWindowMask | LeaveWindowMask;

  menu.title =
    XCreateWindow(display, menu.frame, 0, 0, menu.width, menu.height, 0,
		  screen->getDepth(), InputOutput, screen->getVisual(),
		  attrib_mask, &attrib);
  blackbox->saveMenuSearch(menu.title, this);
  
  menu.iframe = XCreateWindow(display, menu.frame, 0,
                              menu.title_h + screen->getBorderWidth(),
			      menu.width, menu.iframe_h, 0,
			      screen->getDepth(), InputOutput,
			      screen->getVisual(), attrib_mask, &attrib);
  blackbox->saveMenuSearch(menu.iframe, this);
  
  menuitems = new LinkedList<BasemenuItem>;
  
  // even though this is the end of the constructor the menu is still not
  // completely created.  items must be inserted and it must be update()'d
}


Basemenu::~Basemenu(void) {
  XUnmapWindow(display, menu.frame);

  if (shown && shown->getWindowID() == getWindowID())
    shown = (Basemenu *) 0;

  int n = menuitems->count();
  for (int i = 0; i < n; ++i)
    remove(0);
  
  delete menuitems;

  if (menu.label) delete [] menu.label;

  if (menu.title_pixmap) {
    image_ctrl->removeImage(menu.title_pixmap);
    menu.title_pixmap = None;
  }

  if (menu.iframe_pixmap) {
    image_ctrl->removeImage(menu.iframe_pixmap);
    menu.iframe_pixmap = None;
  }

  blackbox->removeMenuSearch(menu.title);
  XDestroyWindow(display, menu.title);

  blackbox->removeMenuSearch(menu.iframe);
  XDestroyWindow(display, menu.iframe);

  blackbox->removeMenuSearch(menu.frame);
  XDestroyWindow(display, menu.frame);
}


int Basemenu::insert(char *l, int function, char *e, int pos) {
  char *label = 0, *exec = 0;
  int llen, elen;
  
  if (l) {
    llen = strlen(l) + 1;
    label = new char[llen];
    strncpy(label, l, llen);
  }
  
  if (e) {
    elen = strlen(e) + 1;
    exec = new char[elen + 1];
    strncpy(exec, e, elen + 1);
  }
  
  BasemenuItem *item = new BasemenuItem(label, function, exec);
  menuitems->insert(item, pos);
  
  return menuitems->count();
}


int Basemenu::insert(char *l, Basemenu *submenu, int pos) {
  char *label = 0;
  int llen = strlen(l);

  if (llen) {
    label = new char[llen + 1];
    strncpy(label, l, llen + 1);
  }
  
  BasemenuItem *item = new BasemenuItem(label, submenu);
  menuitems->insert(item, pos);
  
  submenu->parent = this;
  if (submenu->title_vis) submenu->setMovable(True);
  if (! submenu->default_menu) submenu->setHidable(False);
  
  return menuitems->count();
}


int Basemenu::insert(char **ulabel, int pos) {
  BasemenuItem *item = new BasemenuItem(ulabel);
  menuitems->insert(item, pos);
  
  return menuitems->count();
}


int Basemenu::remove(int index) {
  if (index < 0 || index > menuitems->count()) return -1;

  BasemenuItem *item = menuitems->remove(index);

  if (item) {
    if ((! default_menu) && (item->submenu()) &&
	(! item->submenu()->default_menu)) {
      Basemenu *tmp = (Basemenu *) item->submenu();
      delete tmp;
    }
    
    if (item->label())
      delete [] item->label();
    
    if (item->exec())
      delete [] item->exec();
    
    delete item;
  }

  if (which_sub == index)
    which_sub = -1;
  else if (which_sub > index)
    which_sub--;

  if (always_highlight == index)
    always_highlight = -1;
  else if (always_highlight > index)
    always_highlight--;

  return menuitems->count();
}


void Basemenu::update(void) {
  menu.item_h = screen->getMenuFont()->ascent +
    screen->getMenuFont()->descent + menu.bevel_w;
  
  if (title_vis)
    menu.item_w = (menu.bevel_w * 2) +
      XTextWidth(screen->getTitleFont(),
		 ((menu.label) ? menu.label : "Blackbox Menu"),
		 strlen(((menu.label) ? menu.label : "Blackbox Menu")));
  else
    menu.item_w = 1;
  
  int ii = 0;
  LinkedListIterator<BasemenuItem> it(menuitems);
  for (; it.current(); it++) {
    BasemenuItem *itmp = it.current();

    if (itmp->u)
      ii = (menu.bevel_w * 2) + (menu.item_h * 2) +
	XTextWidth(screen->getMenuFont(), *itmp->u, strlen(*itmp->u));
    else if (itmp->l)
      ii = (menu.bevel_w * 2) + (menu.item_h * 2) +
	XTextWidth(screen->getMenuFont(), itmp->l, strlen(itmp->l));
    else
      ii = 0;
    
    menu.item_w = ((menu.item_w < (unsigned int) ii) ? ii : menu.item_w);
  }
  
  if (menuitems->count()) {
    menu.sublevels = 1;

    while (((menu.item_h * (menuitems->count() + 1) / menu.sublevels)
	    + menu.title_h + screen->getBorderWidth()) >
	   screen->getHeight())
      menu.sublevels++;

    if (menu.sublevels < menu.minsub) menu.sublevels = menu.minsub;
    
    menu.persub = menuitems->count() / menu.sublevels;
    if (menuitems->count() % menu.sublevels) menu.persub++;
  } else {
    menu.sublevels = 0;
    menu.persub = 0;
  }
  
  menu.width = (menu.sublevels * (menu.item_w));
  if (! menu.width) menu.width = menu.item_w;

  menu.title_h = screen->getTitleFont()->ascent +
    screen->getTitleFont()->descent + (menu.bevel_w * 2);

  menu.iframe_h = (menu.item_h * menu.persub);
  menu.height = ((title_vis) ? menu.title_h + screen->getBorderWidth() : 0) +
    menu.iframe_h;
  if (! menu.iframe_h) menu.iframe_h = 1;
  if (menu.height < 1) menu.height = 1;

  Pixmap tmp;
  if (title_vis) {
    tmp = menu.title_pixmap;
    menu.title_pixmap =
      image_ctrl->renderImage(menu.width, menu.title_h,
			      &(screen->getMResource()->title.texture));
    if (tmp) image_ctrl->removeImage(tmp);
    XSetWindowBackgroundPixmap(display, menu.title, menu.title_pixmap);
    XClearWindow(display, menu.title);
  }

  tmp = menu.iframe_pixmap;
  menu.iframe_pixmap =
    image_ctrl->renderImage(menu.width, menu.iframe_h,
			    &(screen->getMResource()->frame.texture));
  if (tmp) image_ctrl->removeImage(tmp);
  XSetWindowBackgroundPixmap(display, menu.iframe, menu.iframe_pixmap);
  
  XResizeWindow(display, menu.frame, menu.width, menu.height);
  
  if (title_vis)
    XResizeWindow(display, menu.title, menu.width, menu.title_h);
  
  XMoveResizeWindow(display, menu.iframe, 0,
		    ((title_vis) ? menu.title_h +
                                   screen->getBorderWidth() : 0), menu.width,
		    menu.iframe_h);
  
  XClearWindow(display, menu.frame);
  XClearWindow(display, menu.title);
  XClearWindow(display, menu.iframe);
  
  if (title_vis)
    switch (screen->getJustification()) {
    case BScreen::LeftJustify:
      {
	XDrawString(display, menu.title, screen->getMenuTitleGC(),
		    menu.bevel_w, screen->getTitleFont()->ascent +
		    menu.bevel_w,
		    ((menu.label) ? menu.label : "Blackbox Menu"),
		    strlen(((menu.label) ? menu.label : "Blackbox Menu")));
	break;
      }
      
    case BScreen::RightJustify:
      {
	int off = menu.bevel_w +
	  XTextWidth(screen->getTitleFont(),
		     ((menu.label) ? menu.label : "Blackbox Menu"),
		     strlen(((menu.label) ? menu.label :
			     "Blackbox Menu")));
	XDrawString(display, menu.title, screen->getMenuTitleGC(),
		    menu.width - off,
		    screen->getTitleFont()->ascent + menu.bevel_w,
		    ((menu.label) ? menu.label : "Blackbox Menu"),
		    strlen(((menu.label) ? menu.label : "Blackbox Menu")));
	break;
      }
      
    case BScreen::CenterJustify:
      {
	int ins = (menu.width -
		   (XTextWidth(screen->getTitleFont(),
			       ((menu.label) ? menu.label : "Blackbox Menu"),
			       strlen(((menu.label) ? menu.label :
				       "Blackbox Menu"))))) / 2;
	XDrawString(display, menu.title, screen->getMenuTitleGC(), ins,
		    screen->getTitleFont()->ascent + menu.bevel_w,
		    ((menu.label) ? menu.label : "Blackbox Menu"),
		    strlen(((menu.label) ? menu.label : "Blackbox Menu")));
	break;
      }
    }
  
  int i = 0;
  for (i = 0; i < menuitems->count(); i++)
    if (i == which_sub) {
      drawItem(i, (i == always_highlight), True, 0);
      drawSubmenu(i);
    } else
      drawItem(i, (i == always_highlight), False, 0);
  
  if (parent && visible)
    parent->drawSubmenu(parent->which_sub);
  
  XMapSubwindows(display, menu.frame);
}


void Basemenu::show(void) {
  XMapSubwindows(display, menu.frame);
  XMapWindow(display, menu.frame);
  visible = True;

  if (! parent) {
    if (shown && ! shown->hasUserMoved()) shown->hide();

    shown = this;
  }
}


void Basemenu::hide(void) {
  if (which_sub != -1) {
    BasemenuItem *tmp = menuitems->find(which_sub);
    tmp->submenu()->hide();
  }

  user_moved = visible = False;
  which_sub = which_press = which_sub = -1;

  if (parent) {
    parent->drawItem(parent->which_sub,
		     (parent->always_highlight == parent->which_sub),
		     False, True);

    if (! default_menu) hidable = False;
    parent->which_sub = -1;
  } else if (shown && shown->getWindowID() == getWindowID())
    shown = (Basemenu *) 0;

  XUnmapWindow(display, menu.frame);
}


void Basemenu::move(int x, int y) {
  menu.x = x;
  menu.y = y;
  XMoveWindow(display, menu.frame, x, y);
  if (which_sub != -1)
    drawSubmenu(which_sub);
}


void Basemenu::drawSubmenu(int index) {
  if (which_sub != -1 && which_sub != index) {
    BasemenuItem *itmp = menuitems->find(which_sub);

    if (! itmp->submenu()->hasUserMoved())
      itmp->submenu()->hide();
  }
  
  if (index >= 0 && index < menuitems->count()) {
    BasemenuItem *item = menuitems->find(index);
    if (item->submenu() && visible && (! item->submenu()->hasUserMoved())) {
      drawItem(index, False, True);

      int sbl = index / menu.persub, i = index - (sbl * menu.persub),
	x = (((shifted) ? menu.x_shift : menu.x) +
	     (menu.item_w * (sbl + 1)) + screen->getBorderWidth()), y;
      
      if (alignment == AlignTop)
	y = (((shifted) ? menu.y_shift : menu.y) +
	     ((title_vis) ? menu.title_h + screen->getBorderWidth() : 0) -
	     ((item->submenu()->title_vis) ?
	      item->submenu()->menu.title_h + screen->getBorderWidth() : 0));
      else
	y = (((shifted) ? menu.y_shift : menu.y) +
	     (menu.item_h * i) +
             ((title_vis) ? menu.title_h + screen->getBorderWidth() : 0) -
	     ((item->submenu()->title_vis) ?
	      item->submenu()->menu.title_h + screen->getBorderWidth() : 0));

      if (alignment == AlignBottom &&
	  (y + item->submenu()->menu.height) > ((shifted) ? menu.y_shift :
						menu.y) + menu.height)
	y = (((shifted) ? menu.y_shift : menu.y) +
	     menu.height - item->submenu()->menu.height);
      
      if ((x + item->submenu()->getWidth()) > screen->getWidth())
	x = ((shifted) ? menu.x_shift : menu.x) -
	  item->submenu()->getWidth() - screen->getBorderWidth();
      if (x < 0) x = 0;

      if ((y + item->submenu()->getHeight()) > screen->getHeight())
	y = screen->getHeight() - item->submenu()->getHeight() -
          screen->getBorderWidth();
      if (y < 0) y = 0;
      
      item->submenu()->move(x, y);
      
      if (! item->submenu()->isVisible())
	item->submenu()->show();
      which_sub = index;
    } else
      which_sub = -1;
  }
}


Bool Basemenu::hasSubmenu(int index) {
  if ((index >= 0) && (index < menuitems->count()))
    if (menuitems->find(index)->submenu())
      return True;
    else
      return False;
  else
    return False;
}


void Basemenu::drawItem(int index, Bool frame, Bool highlight, Bool clear) {
  if (index < 0 || index > menuitems->count()) return;
  
  int sbl = index / menu.persub, i = index - (sbl * menu.persub),
    ix = (sbl * menu.item_w), iy = (i * menu.item_h), tx = 0;
  BasemenuItem *item = menuitems->find(index);
  
  if (! item) return;
  
  switch(screen->getMenuJustification()) {
  case BScreen::LeftJustify:
    tx = ix + menu.bevel_w + menu.item_h + 1;
    break;

  case BScreen::RightJustify:
    tx = ix + menu.item_w -
      (menu.item_h + menu.bevel_w +
       XTextWidth(screen->getMenuFont(),
		  ((item->ulabel()) ? *item->ulabel() : item->label()),
		  strlen((item->ulabel()) ? *item->ulabel() : item->label())));
    break;

  case BScreen::CenterJustify:
    tx = ix + ((menu.item_w + 1 -
		XTextWidth(screen->getMenuFont(),
			   ((item->ulabel()) ? *item->ulabel() :
			    item->label()),
			   strlen((item->ulabel()) ? *item->ulabel() :
				  item->label()))) / 2);
    break;
  }
  
  if (clear) {
    XClearArea(display, menu.iframe, ix, iy, menu.item_w, menu.item_h,
	       False);
  }

  GC gc = (highlight || frame) ? screen->getMenuHiGC() :
       screen->getMenuFrameGC(),
     tgc = (highlight) ? screen->getMenuHiGC() : screen->getMenuFrameGC();

  int fx = ix, hw = menu.item_h / 2, qw = menu.item_h / 4;
    
  if (indicator_position == Right)
    fx += (menu.item_w - menu.item_h - menu.bevel_w);

  if (highlight) {
    if (indicator == Round) {
      XFillArc(display, menu.iframe, screen->getMenuHiBGGC(), ix + 1, iy + 1,
               menu.item_h - 1, menu.item_h - 2, 90 * 64, 180 * 64);
      XFillArc(display, menu.iframe, screen->getMenuHiBGGC(),
               ix + menu.item_w - menu.item_h, iy + 1, menu.item_h - 1,
               menu.item_h - 2, 270 * 64, 180 * 64);
      XFillRectangle(display, menu.iframe, screen->getMenuHiBGGC(), ix +
                     (menu.item_h / 2), iy + 1, menu.item_w - menu.item_h,
                     menu.item_h - 1);    
    } else
      XFillRectangle(display, menu.iframe, screen->getMenuHiBGGC(),
                     ix + 1, iy + 1, menu.item_w - 2, menu.item_h - 2);
   
  } else if (frame) {
    if (indicator == Round)
      XFillArc(display, menu.iframe, screen->getMenuHiBGGC(), fx + 1, iy + 1,
               menu.item_h - 1, menu.item_h - 2, 0, 360 * 64);
    else
      XFillRectangle(display, menu.iframe, screen->getMenuHiBGGC(),
                     fx + qw, iy + qw, hw + 1, hw + 1);
  }
  
  if (item->ulabel())
    XDrawString(display, menu.iframe, tgc, tx,
                iy + screen->getMenuFont()->ascent + (menu.bevel_w / 2) + 1,
                *item->ulabel(), strlen(*item->ulabel()));
  else if (item->label())
    XDrawString(display, menu.iframe, tgc, tx,
                iy + screen->getMenuFont()->ascent + (menu.bevel_w / 2) + 1,
                item->label(), strlen(item->label()));
    
  if (item->submenu()) {
    switch (indicator) {
    case Empty:
      break;

    case Round:
    default:
      XDrawArc(display, menu.iframe, gc, fx + qw + 1, iy + qw + 1,
               hw, hw, 0, 360 * 64);
      break;

    case Square:
      XDrawRectangle(display, menu.iframe, gc, fx + qw, iy + qw, hw, hw);
      break;
                                  
    case Triangle:
      XPoint tri[3]; 

      if (indicator_position == Right) {
        tri[0].x = fx + hw - 2; tri[0].y = iy + hw - 2;
        tri[1].x = 4; tri[1].y = 2;
        tri[2].x = -4; tri[2].y = 2;
      } else {
        tri[0].x = fx + hw - 2; tri[0].y = iy + hw;
        tri[1].x = 4; tri[1].y = 2; 
        tri[2].x = 0; tri[2].y = -4;
      }

      XFillPolygon(display, menu.iframe, gc, tri, 3, Convex,
                   CoordModePrevious);
      break;

    case Diamond:
      XPoint dia[4];

      dia[0].x = fx + hw - 3; dia[0].y = iy + hw;
      dia[1].x = 3; dia[1].y = -3;
      dia[2].x = 3; dia[2].y = 3;
      dia[3].x = -3; dia[3].y = 3;

      XFillPolygon(display, menu.iframe, gc, dia, 4, Convex,
                   CoordModePrevious);
      break;
    }
  }
}


void Basemenu::setLabel(char *l) {
  if (menu.label) delete [] menu.label;

  if (l) {
    int mlen = strlen(l) + 1;
    menu.label = new char[mlen];
    strncpy(menu.label, l, mlen);
  } else
    menu.label = 0;
}


void Basemenu::setHighlight(int index) {
  if (always_highlight != -1)
    drawItem(always_highlight, False, (which_sub == always_highlight), True);
  
  if (index >= 0 && index < menuitems->count()) {
    always_highlight = index; 
    drawItem(always_highlight, True, (which_sub == always_highlight), True);
  } else
    always_highlight = -1;
}


void Basemenu::buttonPressEvent(XButtonEvent *be) {
  if (be->window == menu.iframe) {
    int sbl = (be->x / menu.item_w), i = (be->y / menu.item_h);
    int w = (sbl * menu.persub) + i;

    if (w < menuitems->count() && w >= 0) {
      which_press = i;
      which_sbl = sbl;

      BasemenuItem *item = menuitems->find(w);

      if (item->submenu())
	drawSubmenu(w);
      else
	drawItem(w, False, True);
    }
  }
}


void Basemenu::buttonReleaseEvent(XButtonEvent *re) {
  if (re->window == menu.title) {
    if (moving) {
      XUngrabPointer(display, CurrentTime);
      moving = False;
      if (which_sub != -1)
	drawSubmenu(which_sub);
    }

    if (re->x >= 0 && re->x <= (signed) menu.width &&
	re->y >= 0 && re->y <= (signed) menu.title_h)
      if (re->button == 3 && hidable)
	hide();
  } else if (re->window == menu.iframe &&
	     re->x >= 0 && re->x < (signed) menu.width &&
	     re->y >= 0 && re->y < (signed) menu.iframe_h) {
    if (re->button == 3 && hidable) {
      hide();
    } else {
      int sbl = (re->x / menu.item_w), i = (re->y / menu.item_h),
        ix = sbl * menu.item_w, iy = i * menu.item_h,
        w = (sbl * menu.persub) + i,
        p = (which_sbl * menu.persub) + which_press;

      if (w < menuitems->count() && w >= 0) {
	drawItem(p, (p == always_highlight), (p == which_sub), True);
	
        if  (p == w) {
	  if (re->x > ix && re->x < (signed) (ix + menu.item_w) &&
	      re->y > iy && re->y < (signed) (iy + menu.item_h)) {
	    itemSelected(re->button, w);
	  }
        }
      } else
        drawItem(p, False, True);
    }
  } else
    XUngrabPointer(display, CurrentTime);
}


void Basemenu::motionNotifyEvent(XMotionEvent *me) {
  if (me->window == menu.title && (me->state & Button1Mask)) {
    if (movable) {
      if (! moving) {
	if (XGrabPointer(display, menu.title, False, PointerMotionMask|
			 ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
			 None, blackbox->getMoveCursor(), CurrentTime)
	    == GrabSuccess) {
	  moving = hidable = user_moved = True;

	  if (parent) {
	    parent->drawItem(parent->which_sub,
			     (parent->which_sub == parent->always_highlight),
			     False, True);
	    parent->which_sub = -1;
	  }
	  
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
  } else if ((! (me->state & Button1Mask)) && me->window == menu.iframe &&
	     me->x >= 0 && me->x < (signed) menu.width &&
	     me->y >= 0 && me->y < (signed) menu.iframe_h) {
    int sbl = (me->x / menu.item_w), i = (me->y / menu.item_h),
      w = (sbl * menu.persub) + i;
    
    if ((i != which_press || sbl != which_sbl) &&
	(w < menuitems->count() && w >= 0)) {
      if (which_press != -1 && which_sbl != -1) {
	int p = (which_sbl * menu.persub) + which_press;
	BasemenuItem *item = menuitems->find(p);
	
	drawItem(p, (p == always_highlight), False, True);
	if (item->submenu())
	  if (item->submenu()->isVisible() &&
	      (! item->submenu()->hasUserMoved())) {
	    item->submenu()->hide();
	    which_sub = -1;  
	  }
      }
      
      which_press = i;
      which_sbl = sbl;
      
      BasemenuItem *itmp = menuitems->find(w);
      
      if (itmp->submenu())
	drawSubmenu(w);
      else
	drawItem(w, False, True);
    }
  }
}


void Basemenu::exposeEvent(XExposeEvent *ee) {
  if (ee->window == menu.title) {
    switch (screen->getJustification()) {
    case BScreen::LeftJustify:
      {
	XDrawString(display, menu.title, screen->getMenuTitleGC(),
		    menu.bevel_w, screen->getTitleFont()->ascent +
		    menu.bevel_w,
		    ((menu.label) ? menu.label : "Blackbox Menu"),
		    strlen(((menu.label) ? menu.label : "Blackbox Menu")));
	break;
      }

    case BScreen::RightJustify:
      {
	int off = XTextWidth(screen->getTitleFont(),
			     ((menu.label) ? menu.label : "Blackbox Menu"),
			     strlen(((menu.label) ? menu.label :
				     "Blackbox Menu"))) + menu.bevel_w;
	XDrawString(display, menu.title, screen->getMenuTitleGC(),
		    menu.width - off,
		    screen->getTitleFont()->ascent + menu.bevel_w,
		    ((menu.label) ? menu.label : "Blackbox Menu"),
		    strlen(((menu.label) ? menu.label : "Blackbox Menu")));
	break;
      }
      
    case BScreen::CenterJustify:
      {
	int ins = (menu.width -
		   (XTextWidth(screen->getTitleFont(),
			       ((menu.label) ? menu.label : "Blackbox Menu"),
			       strlen(((menu.label) ? menu.label :
				       "Blackbox Menu"))))) / 2;
	XDrawString(display, menu.title, screen->getMenuTitleGC(), ins,
		    screen->getTitleFont()->ascent + menu.bevel_w,
		    ((menu.label) ? menu.label : "Blackbox Menu"),
		    strlen(((menu.label) ? menu.label : "Blackbox Menu")));
	break;
      }
    }
  } else if (ee->window == menu.iframe) {
    LinkedListIterator<BasemenuItem> it(menuitems);
    
    // this is a compilicated algorithm... lets do it step by step...
    // first... we see in which sub level the expose starts... and how many
    // items down in that sublevel
    
    int sbl = (ee->x / menu.item_w), id = (ee->y / menu.item_h),
      // next... figure out how many sublevels over the redraw spans
      sbl_d = ((ee->x + ee->width) / menu.item_w),
      // then we see how many items down to redraw
      id_d = ((ee->y + ee->height) / menu.item_h);
    
    if (id_d > menu.persub) id_d = menu.persub;
    
    // draw the sublevels and the number of items the exposure spans
    int i, ii;
    for (i = sbl; i <= sbl_d; i++) {
      // set the iterator to the first item in the sublevel needing redrawing
      it.set(id + (i * menu.persub));
      for (ii = id; ii <= id_d && it.current(); it++, ii++) {
	int index = ii + (i * menu.persub);
	// redraw the item
	drawItem(index, (always_highlight == index), (which_sub == index));
      }
    }
  }
}


void Basemenu::enterNotifyEvent(XCrossingEvent *ce) {
  if (ce->window == menu.iframe) {
    XGrabPointer(display, menu.iframe, False, PointerMotionMask|
		 LeaveWindowMask|ButtonPressMask|ButtonReleaseMask,
		 GrabModeAsync, GrabModeAsync,  None,
		 blackbox->getSessionCursor(), CurrentTime);
    
    menu.x_shift = menu.x, menu.y_shift = menu.y;
    if (menu.x + menu.width > screen->getWidth()) {
      menu.x_shift = screen->getWidth() - menu.width - screen->getBorderWidth();
      shifted = True;
    } else if (menu.x < 0) {
      menu.x_shift = 0;
      shifted = True;
    }
    
    if (menu.y + menu.height > screen->getHeight()) {
      menu.y_shift = screen->getHeight() - menu.height -
        screen->getBorderWidth();
      shifted = True;
    } else if (menu.y + (signed) menu.title_h < 0) {
      menu.y_shift = 0;
      shifted = True;
    }
    
    if (shifted)
      XMoveWindow(display, menu.frame, menu.x_shift, menu.y_shift);
    
    if (which_sub != -1) {
      BasemenuItem *tmp = menuitems->find(which_sub);
      if (tmp->submenu()->isVisible()) {
	int sbl = (ce->x / menu.item_w), i = (ce->y / menu.item_h),
	  w = (sbl * menu.persub) + i;
	
	if (w != which_sub && (! tmp->submenu()->hasUserMoved())) {
	  tmp->submenu()->hide();
	  
	  drawItem(which_sub, (w == always_highlight), False, True);
	  which_sub = -1;
	}
      }
    }
  }
}


void Basemenu::leaveNotifyEvent(XCrossingEvent *ce) {
  if (ce->window == menu.iframe) {
    if (which_press != -1 && which_sbl != -1 && menuitems->count() > 0) {
      int p = (which_sbl * menu.persub) + which_press;      
      
      
      drawItem(p, (p == always_highlight), (p == which_sub), True);
      
      which_sbl = which_press = -1;
    }
    
    if (shifted) {
      XMoveWindow(display, menu.frame, menu.x, menu.y);
      shifted = False;
    }

    if (! (ce->state & Button1Mask))
      XUngrabPointer(display, CurrentTime);
  }
}


void Basemenu::reconfigure(void) {
  XSetWindowBackground(display, menu.frame,
		       screen->getBorderColor()->getPixel());
  XSetWindowBorder(display, menu.frame,
		   screen->getBorderColor()->getPixel());
  XSetWindowBorderWidth(display, menu.frame, screen->getBorderWidth());

  menu.bevel_w = screen->getBevelWidth();
  indicator_position = screen->getBulletPosition();
  indicator = screen->getBulletStyle();
  update();
}
