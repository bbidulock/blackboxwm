//
// Basemenu.cc for Blackbox - an X11 Window manager
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

#include "blackbox.hh"
#include "graphics.hh"
#include "Basemenu.hh"
#include "Rootmenu.hh"

#include <stdio.h>
#include <stdlib.h>


// *************************************************************************
// Basemenu constructor and destructor
// *************************************************************************

Basemenu::Basemenu(Blackbox *ctrl) {
  blackbox = ctrl;
  display = blackbox->control();
  parent = (Basemenu *) 0;

  title_vis = movable = True;
  shifted = default_menu = moving = user_moved = visible = False;
  menu.x = menu.y = menu.x_shift = menu.y_shift = menu.x_move =
    menu.y_move = 0;
  which_sub = which_press = which_sbl = -1;

  menu.iframe_pixmap = None;

  menu.bevel_w = blackbox->bevelWidth();
  menu.width= menu.title_h = menu.item_w = menu.iframe_h =
    blackbox->titleFont()->ascent + blackbox->titleFont()->descent +
    (menu.bevel_w * 2);
  menu.label = 0;
  menu.sublevels = menu.persub = 0;
  menu.item_h = blackbox->menuFont()->ascent +
    blackbox->menuFont()->descent + (menu.bevel_w);
  menu.height = menu.title_h + 1 + menu.iframe_h;
  
  unsigned long attrib_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWOverrideRedirect|CWCursor|CWEventMask;
  XSetWindowAttributes attrib;
  attrib.background_pixmap = None;
  attrib.background_pixel = attrib.border_pixel =
    blackbox->borderColor().pixel;
  attrib.override_redirect = True;
  attrib.cursor = blackbox->sessionCursor();
  attrib.event_mask = StructureNotifyMask|SubstructureNotifyMask|
    SubstructureRedirectMask|ButtonPressMask|ButtonReleaseMask|
    ButtonMotionMask|ExposureMask;

  menu.frame =
    XCreateWindow(display, blackbox->Root(), menu.x, menu.y, menu.width,
		  menu.height, 1, blackbox->Depth(), InputOutput,
		  blackbox->visual(), attrib_mask, &attrib);
  blackbox->saveMenuSearch(menu.frame, this);
  
  attrib_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|CWCursor|CWEventMask;
  attrib.background_pixel = blackbox->borderColor().pixel;
  attrib.event_mask |= EnterWindowMask|LeaveWindowMask;
  menu.title =
    XCreateWindow(display, menu.frame, 0, 0, menu.width, menu.height, 0,
		  blackbox->Depth(), InputOutput, blackbox->visual(),
		  attrib_mask, &attrib);
  blackbox->saveMenuSearch(menu.title, this);

  menu.iframe = XCreateWindow(display, menu.frame, 0, menu.title_h + 1,
			      menu.width, menu.iframe_h, 0,
			      blackbox->Depth(), InputOutput,
			      blackbox->visual(), attrib_mask, &attrib);
  blackbox->saveMenuSearch(menu.iframe, this);

  menuitems = new LinkedList<BasemenuItem>;
  
  XGCValues gcv;
  gcv.foreground = blackbox->mTTextColor().pixel;
  gcv.font = blackbox->titleFont()->fid;
  titleGC = XCreateGC(display, menu.frame, GCForeground|GCFont, &gcv);

  gcv.foreground = blackbox->mFTextColor().pixel;
  gcv.font = blackbox->menuFont()->fid;
  itemGC = XCreateGC(display, menu.frame, GCForeground|GCFont, &gcv);

  gcv.foreground = blackbox->mHTextColor().pixel;
  hitemGC = XCreateGC(display, menu.frame, GCForeground|GCBackground|GCFont,
		      &gcv);
  
  gcv.foreground = blackbox->mHColor().pixel;
  gcv.arc_mode = ArcChord;
  gcv.fill_style = FillSolid;
  hbgGC = XCreateGC(display, menu.frame, GCForeground|GCFillStyle|GCArcMode,
		    &gcv);

  // even though this is the end of the constructor the menu is still not
  // completely created.  items must be inserted and it must be Update()'d
}


Basemenu::~Basemenu(void) {
  XUnmapWindow(display, menu.frame);

  int n = menuitems->count();
  for (int i = 0; i < n; ++i)
    remove(0);
  delete menuitems;

  XFreeGC(display, titleGC);
  XFreeGC(display, itemGC);
  XFreeGC(display, hitemGC);
  XFreeGC(display, hbgGC);

  blackbox->removeMenuSearch(menu.title);
  XDestroyWindow(display, menu.title);

  blackbox->removeMenuSearch(menu.iframe);
  XDestroyWindow(display, menu.iframe);

  blackbox->removeMenuSearch(menu.frame);
  XDestroyWindow(display, menu.frame);

  if (menu.label) delete [] menu.label;
}


// *************************************************************************
// insertion and removal methods
// *************************************************************************

int Basemenu::insert(char *label, int function, char *exec) {
  int ret = 0;
  if (function)
    switch (function) {
    case Blackbox::B_WindowKill:
    case Blackbox::B_WindowStick:
    case Blackbox::B_WindowShade:
    case Blackbox::B_WindowIconify:
    case Blackbox::B_WindowMaximize:
    case Blackbox::B_WindowClose:
    case Blackbox::B_WindowRaise:
    case Blackbox::B_WindowLower:
    case Blackbox::B_Exit:
    case Blackbox::B_Restart:
    case Blackbox::B_Reconfigure: {
      BasemenuItem *item = new BasemenuItem(label, function);
      ret = menuitems->insert(item);
      
      ret = menuitems->count();
      break; }
    
    case Blackbox::B_Execute:
    case Blackbox::B_ExecReconfigure:
    case Blackbox::B_RestartOther: {
      BasemenuItem *item = new BasemenuItem(label, function, exec);
      ret = menuitems->insert(item);
      
      ret = menuitems->count();
      break; }
    }
  else
    ret = menuitems->insert(new BasemenuItem(label, 0));
  
  return ret;
}


int Basemenu::insert(char *label, Basemenu *submenu) {
  BasemenuItem *item = new BasemenuItem(label, submenu);
  menuitems->insert(item);
  submenu->parent = this;
  submenu->setMovable(False);

  return menuitems->count();
}


int Basemenu::insert(char **ulabel) {
  BasemenuItem *item = new BasemenuItem(ulabel);
  menuitems->insert(item);

  return menuitems->count();
}


int Basemenu::remove(int index) {
  BasemenuItem *item = menuitems->remove(index);

  if (! default_menu) {
    if (item->sub_menu) {
      Basemenu *tmp = (Basemenu *) item->sub_menu;
      delete tmp;
    }
    if (item->label) {
      delete [] item->label;
    }
    if (item->exec) {
      delete [] item->exec;
    }
  }
  
  delete item;

  if (which_sub != -1)
    if ((--which_sub) == index)
      which_sub = -1;

  return menuitems->count();
}


// *************************************************************************
// Menu maintainence and utility methods
// *************************************************************************

void Basemenu::Update(void) {
  menu.item_h = blackbox->menuFont()->ascent + blackbox->menuFont()->descent +
    menu.bevel_w;
  
  if (title_vis)
    menu.item_w = (menu.bevel_w * 2) +
      XTextWidth(blackbox->titleFont(),
		 ((menu.label) ? menu.label : "Blackbox Menu"),
		 strlen(((menu.label) ? menu.label : "Blackbox Menu")));
  else
    menu.item_w = 1;
  
  int ii = 0;
  LinkedListIterator<BasemenuItem> it(menuitems);
  for (; it.current(); it++) {
    BasemenuItem *itmp = it.current();
    if (itmp->ulabel)
      ii = (menu.bevel_w * 2) + menu.item_h +
	XTextWidth(blackbox->menuFont(), *itmp->ulabel,
		   strlen(*itmp->ulabel));
    else if (itmp->label)
      ii = (menu.bevel_w * 2) + menu.item_h +
	XTextWidth(blackbox->menuFont(), itmp->label, strlen(itmp->label));
    else
      ii = 0;
    
    menu.item_w = ((menu.item_w < (unsigned int) ii) ? ii : menu.item_w);
  }
  
  if (menuitems->count()) {
    menu.sublevels = 1;
    while ((((menu.item_h + 1) * (menuitems->count() + 1) / menu.sublevels)
	    + menu.title_h + 1) >
	   blackbox->YResolution())
      menu.sublevels++;
  
    menu.persub = menuitems->count() / menu.sublevels;
    if (menuitems->count() % menu.sublevels) menu.sublevels++;
  } else {
    menu.sublevels = 0;
    menu.persub = 0;
  }
  
  menu.width = (menu.sublevels * (menu.item_w));
  if (! menu.width) menu.width = menu.item_w;

  menu.title_h = blackbox->titleFont()->ascent +
    blackbox->titleFont()->descent + (menu.bevel_w * 2);

  menu.iframe_h = ((menu.item_h + 1) * menu.persub);
  menu.height = ((title_vis) ? menu.title_h + 1 : 0) + menu.iframe_h;
  if (! menu.iframe_h) menu.iframe_h = 1;
  if (menu.height < 1) menu.height = 1;

  if (title_vis) {
    BImage *mt_image = new BImage(blackbox, menu.width, menu.title_h,
				  blackbox->Depth());
    Pixmap mt_pixmap =
      mt_image->renderImage(blackbox->mTitleTexture(),
			    blackbox->mTColor(),
			    blackbox->mTColorTo());
    delete mt_image;
    XSetWindowBackgroundPixmap(display, menu.title, mt_pixmap);
    XClearWindow(display, menu.title);
    if (mt_pixmap) XFreePixmap(display, mt_pixmap);
  }
  
  BImage *mi_image = new BImage(blackbox, menu.width, menu.iframe_h,
				blackbox->Depth());
  if (menu.iframe_pixmap) XFreePixmap(display, menu.iframe_pixmap);
  menu.iframe_pixmap =
    mi_image->renderImage(blackbox->mFrameTexture(),
			  blackbox->mFColor(),
			  blackbox->mFColorTo());
  delete mi_image;
  XSetWindowBackgroundPixmap(display, menu.iframe, menu.iframe_pixmap);
  
  XResizeWindow(display, menu.frame, menu.width, menu.height);

  if (title_vis)
    XResizeWindow(display, menu.title, menu.width, menu.title_h);

  XMoveResizeWindow(display, menu.iframe, 0,
		    ((title_vis) ? menu.title_h + 1 : 0), menu.width,
		    menu.iframe_h);

  XClearWindow(display, menu.frame);
  XClearWindow(display, menu.title);
  XClearWindow(display, menu.iframe);

  if (title_vis)
    switch (blackbox->Justification()) {
    case Blackbox::B_LeftJustify: {
      XDrawString(display, menu.title, titleGC, menu.bevel_w,
		  blackbox->titleFont()->ascent + menu.bevel_w,
		  ((menu.label) ? menu.label : "Blackbox Menu"),
		  strlen(((menu.label) ? menu.label : "Blackbox Menu")));
      break; }
    
    case Blackbox::B_RightJustify: {
      int off = menu.bevel_w +
	XTextWidth(blackbox->titleFont(),
		   ((menu.label) ? menu.label : "Blackbox Menu"),
		   strlen(((menu.label) ? menu.label :
			   "Blackbox Menu")));
      XDrawString(display, menu.title, titleGC, menu.width - off,
		  blackbox->titleFont()->ascent + menu.bevel_w,
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
		  blackbox->titleFont()->ascent + menu.bevel_w,
		  ((menu.label) ? menu.label : "Blackbox Menu"),
		  strlen(((menu.label) ? menu.label : "Blackbox Menu")));
      break; }
    }
  
  int i = 0;
  for (i = 0; i < menuitems->count(); i++)
    drawItem(i, (i == which_sub), 0);

  XMapSubwindows(display, menu.frame);
  //  XSync(display, False);
}


void Basemenu::Show(void) {
  XMapSubwindows(display, menu.frame);
  XMapWindow(display, menu.frame);
  visible = True;
}


void Basemenu::Hide(void) {
  if (which_sub != -1) {
    BasemenuItem *tmp = menuitems->find(which_sub);
    tmp->sub_menu->Hide();
  }

  user_moved = False;
  visible = False;
  which_sub = which_press = which_sub = -1;
  XUnmapWindow(display, menu.frame);
}


void Basemenu::Move(int x, int y) {
  menu.x = x;
  menu.y = y;
  XMoveWindow(display, menu.frame, x, y);
  if (which_sub != -1)
    drawSubmenu(which_sub);
}


void Basemenu::drawSubmenu(int index, Bool) {
  if (which_sub != -1 && which_sub != index) {
    BasemenuItem *tmp = menuitems->find(which_sub);
    tmp->sub_menu->Hide();

    drawItem(which_sub, False, True);
  }
  
  if (index >= 0 && index < menuitems->count()) {
    BasemenuItem *item = menuitems->find(index);
    if (item->sub_menu && visible) {
      drawItem(index, True);

      int sbl = index / menu.persub, i = index - (sbl * menu.persub),
	x = ((shifted) ? menu.x_shift : menu.x) +
	(menu.item_w * (sbl + 1)) + 1,
	y = ((shifted) ? menu.y_shift : menu.y) +
	((menu.item_h + 1) * i) + ((title_vis) ? menu.title_h + 1 : 0) -
	((item->sub_menu->title_vis) ? item->sub_menu->menu.title_h + 1 : 0);
      
      if ((x + item->sub_menu->Width()) > blackbox->XResolution())
	x = ((shifted) ? menu.x_shift : menu.x) - item->sub_menu->Width() - 1;
      
      if ((y + item->sub_menu->Height()) > blackbox->YResolution())
	y = blackbox->YResolution() - item->sub_menu->Height() - 1;

      item->sub_menu->Move(x, y);

      if (! item->sub_menu->visible)
	item->sub_menu->Show();
      item->sub_menu->visible = 3;
      which_sub = index;
    } else
      which_sub = -1;
  }
}


Bool Basemenu::hasSubmenu(int index) {
  if ((index >= 0) && (index < menuitems->count()))
    if (menuitems->find(index)->sub_menu)
      return True;
    else
      return False;
  else
    return False;
}


void Basemenu::drawItem(int index, Bool highlight, Bool clearArea) {
  if (index < 0 || index > menuitems->count()) return;

  int sbl = index / menu.persub, i = index - (sbl * menu.persub),
    ix = (sbl * menu.item_w), iy = (i * (menu.item_h + 1));
  BasemenuItem *item = menuitems->find(index);

  if (! item) return;
  
  if (clearArea) {
    XClearArea(display, menu.iframe, ix, iy, menu.item_w, menu.item_h + 1,
	       False);
  }

  if (highlight) {
    XFillArc(display, menu.iframe, hbgGC, ix + 1, iy, menu.item_h,
	     menu.item_h, 90 * 64, 180 * 64);
    XFillRectangle(display, menu.iframe, hbgGC, ix + ((menu.item_h + 1) / 2),
		   iy + 1, menu.item_w - (((menu.item_h) + 1) / 2) - 2,
		   menu.item_h - 1);
    
    if (item->ulabel)
      XDrawString(display, menu.iframe, hitemGC, ix + menu.bevel_w +
		  menu.item_h + 1, iy + blackbox->menuFont()->ascent +
		  menu.bevel_w, *item->ulabel, strlen(*item->ulabel));
    else if (item->label)
      XDrawString(display, menu.iframe, hitemGC, ix + menu.bevel_w +
		  menu.item_h + 1, iy + blackbox->menuFont()->ascent +
		  menu.bevel_w, item->label, strlen(item->label));
    
    if (item->sub_menu)
      XDrawArc(display, menu.iframe, hitemGC, ix + menu.bevel_w + 1, iy +
	       menu.bevel_w, menu.item_h - (menu.bevel_w * 2),
	       menu.item_h - (menu.bevel_w * 2), 0, 360 * 64);
  } else {
    if (item->ulabel)
      XDrawString(display, menu.iframe, itemGC, ix + menu.bevel_w +
		  menu.item_h + 1, iy + blackbox->menuFont()->ascent +
		  menu.bevel_w, *item->ulabel, strlen(*item->ulabel));
    else if (item->label)
      XDrawString(display, menu.iframe, itemGC, ix + menu.bevel_w +
		  menu.item_h + 1, iy + blackbox->menuFont()->ascent +
		  menu.bevel_w, item->label, strlen(item->label));
    
    if (item->sub_menu)
      XDrawArc(display, menu.iframe, itemGC, ix + menu.bevel_w + 1, iy + 
	       menu.bevel_w, menu.item_h - (menu.bevel_w * 2),
	       menu.item_h - (menu.bevel_w * 2), 0, 360 * 64);
  }
}


// *************************************************************************
// Menu event handling
// *************************************************************************

void Basemenu::buttonPressEvent(XButtonEvent *be) {
  if (be->window == menu.title) {
    if (be->x >= 0 && be->x <= (signed) menu.width &&
	be->y >= 0 && be->y <= (signed) menu.title_h) {
    }
  } else if (be->window == menu.iframe) {
    int sbl = (be->x / menu.item_w), i = (be->y / (menu.item_h + 1));
    int w = (sbl * menu.persub) + i;

    if (w < menuitems->count() && w >= 0) {
      which_press = i;
      which_sbl = sbl;

      BasemenuItem *item = menuitems->find(w);

      if (item->sub_menu)
	drawSubmenu(w);
      else
	drawItem(w, True);
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
      if (re->button == 3 && menu.frame == blackbox->Menu()->WindowID())
	Hide();
  } else if (re->window == menu.iframe &&
	     re->x >= 0 && re->x < (signed) menu.width &&
	     re->y >= 0 && re->y < (signed) menu.iframe_h) {
    int sbl = (re->x / menu.item_w), i = (re->y / (menu.item_h + 1)),
      ix = sbl * menu.item_w, iy = i * (menu.item_h + 1),
      w = (sbl * menu.persub) + i,
      p = (which_sbl * menu.persub) + which_press;

    if (w < menuitems->count() && w >= 0) {
      if (p != which_sub)
	drawItem(p, False, True);
      
      if  (p == w) {
	if (re->x > ix && re->x < (signed) (ix + menu.item_w) &&
	    re->y > iy && re->y < (signed) (iy + menu.item_h)) {
	  itemSelected(re->button, w);
	}
      }
    } else
      drawItem(p, True);
  } else
    XUngrabPointer(display, CurrentTime);
}


void Basemenu::motionNotifyEvent(XMotionEvent *me) {
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
  } else if ((! (me->state & Button1Mask)) && me->window == menu.iframe &&
	     me->x >= 0 && me->x < (signed) menu.width &&
	     me->y >= 0 && me->y < (signed) menu.iframe_h) {
    int sbl = (me->x / menu.item_w), i = (me->y / (menu.item_h + 1)),
      w = (sbl * menu.persub) + i;
    
    if ((i != which_press || sbl != which_sbl) &&
	(w < menuitems->count() && w >= 0)) {
      if (which_press != -1 && which_sbl != -1) {
	int p = (which_sbl * menu.persub) + which_press;
	BasemenuItem *item = menuitems->find(p);
	
	drawItem(p, False, True);
	if (item->sub_menu)
	  if (item->sub_menu->Visible()) {
	    item->sub_menu->Hide();
	    which_sub = -1;  
	  }
      }
      
      which_press = i;
      which_sbl = sbl;
      
      BasemenuItem *itmp = menuitems->find(w);
      
      if (itmp->sub_menu)
	drawSubmenu(w);
      else
	drawItem(w, True);
    }
  }
}


void Basemenu::exposeEvent(XExposeEvent *ee) {
  if (ee->window == menu.title) {
    switch (blackbox->Justification()) {
    case Blackbox::B_LeftJustify: {
      XDrawString(display, menu.title, titleGC, menu.bevel_w,
		  blackbox->titleFont()->ascent + menu.bevel_w,
		  ((menu.label) ? menu.label : "Blackbox Menu"),
		  strlen(((menu.label) ? menu.label : "Blackbox Menu")));
      break; }

    case Blackbox::B_RightJustify: {
      int off = XTextWidth(blackbox->titleFont(),
			   ((menu.label) ? menu.label : "Blackbox Menu"),
			   strlen(((menu.label) ? menu.label :
				   "Blackbox Menu"))) + menu.bevel_w;
      XDrawString(display, menu.title, titleGC, menu.width - off,
		  blackbox->titleFont()->ascent + menu.bevel_w,
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
		  blackbox->titleFont()->ascent + menu.bevel_w,
		  ((menu.label) ? menu.label : "Blackbox Menu"),
		  strlen(((menu.label) ? menu.label : "Blackbox Menu")));
      break; }
    }
  } else if (ee->window == menu.iframe) {
    LinkedListIterator<BasemenuItem> it(menuitems);

    // this is a compilicated algorithm... lets do it step by step...
    // first... we see in which sub level the expose starts... and how many
    // items down in that sublevel

    int sbl = (ee->x / menu.item_w), id = (ee->y / (menu.item_h + 1)),
      // next... figure out how many sublevels over the redraw spans
      sbl_d = ((ee->x + ee->width) / menu.item_w),
      // then we see how many items down to redraw
      id_d = ((ee->y + ee->height) / (menu.item_h + 1));

    if (id_d > menu.persub) id_d = menu.persub;

    // draw the sublevels and the number of items the exposure spans
    int i, ii;
    for (i = sbl; i <= sbl_d; i++) {
      // set the iterator to the first item in the sublevel needing redrawing
      it.set(id + (i * menu.persub));
      for (ii = id; ii <= id_d && it.current(); it++, ii++) {
	int index = ii + (i * menu.persub);
	// redraw the item
	if (which_sub == index)
	  drawItem(index, True);
	else
	  drawItem(index);
      }
    }
  }
}


void Basemenu::enterNotifyEvent(XCrossingEvent *ce) {
  if (ce->window == menu.iframe) {
    XGrabPointer(display, menu.iframe, False, PointerMotionMask|
		 LeaveWindowMask|ButtonPressMask|ButtonReleaseMask,
		 GrabModeAsync, GrabModeAsync,  None,
		 blackbox->sessionCursor(), CurrentTime);
    
    menu.x_shift = menu.x, menu.y_shift = menu.y;
    if (menu.x + menu.width > blackbox->XResolution()) {
      menu.x_shift = blackbox->XResolution() - menu.width - 1;
      shifted = True;
    }

    if (menu.y + menu.height > blackbox->YResolution()) {
      menu.y_shift = blackbox->YResolution() - menu.height - 1;
      shifted = True;
    }

    if (shifted)
      XMoveWindow(display, menu.frame, menu.x_shift, menu.y_shift);

    if (which_sub != -1) {
      BasemenuItem *tmp = menuitems->find(which_sub);
      if (tmp->sub_menu->Visible()) {
	int sbl = (ce->x / menu.item_w), i = (ce->y / (menu.item_h + 1)),
	  w = (sbl * menu.persub) + i;

	if (w != which_sub) {
	  tmp->sub_menu->Hide();
	  
	  drawItem(which_sub, False, True);
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
      
      if (p != which_sub)
	drawItem(p, False, True);
      
      which_sbl = which_press = -1;
    }
    
    if (! (ce->state & Button1Mask))
      XUngrabPointer(display, CurrentTime);

    if (shifted) {
      XMoveWindow(display, menu.frame, menu.x, menu.y);
      shifted = False;
    }
  }
}


// *************************************************************************
// Resource reconfiguration
// *************************************************************************

void Basemenu::Reconfigure(void) {
  XGCValues gcv;
  gcv.foreground = blackbox->mTTextColor().pixel;
  gcv.font = blackbox->titleFont()->fid;
  XChangeGC(display, titleGC, GCForeground|GCFont, &gcv);

  gcv.foreground = blackbox->mFTextColor().pixel;
  gcv.font = blackbox->menuFont()->fid;
  XChangeGC(display, itemGC, GCForeground|GCFont, &gcv);

  gcv.foreground = blackbox->mHTextColor().pixel;
  XChangeGC(display, hitemGC, GCForeground|GCBackground|GCFont, &gcv);

  gcv.foreground = blackbox->mHColor().pixel;
  gcv.arc_mode = ArcChord;
  gcv.fill_style = FillSolid;
  XChangeGC(display, hbgGC, GCForeground|GCFillStyle|GCArcMode,
	    &gcv);

  XSetWindowBackground(display, menu.frame, blackbox->borderColor().pixel);
  XSetWindowBorder(display, menu.frame, blackbox->borderColor().pixel);

  menu.bevel_w = blackbox->bevelWidth();
  Update();
}
