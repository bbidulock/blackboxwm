//
// workspace.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@tcac.net
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "blackbox.hh"
#include "Clientmenu.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Windowmenu.hh"

#if HAVE_STDIO_H
#  include <stdio.h>
#endif

#if STDC_HEADERS
#  include <string.h>
#endif


Workspace::Workspace(BScreen *scrn, int i) {
  screen = scrn;

  cascade_x = cascade_y = 32;

  id = i;
  stack = 0;

  windowList = new LinkedList<BlackboxWindow>;
  clientmenu = new Clientmenu(screen->getBlackbox(), this);
  clientmenu->update();
  
  screen->getNameOfWorkspace(id, &name);
  
  if (! name) {
    name = new char[32];
    if (name)
      sprintf(name, "Workspace %d", id);
  }
  
  label = 0;
}


Workspace::~Workspace(void) {
  delete windowList;
  delete clientmenu;

  if (stack) delete [] stack;
  if (name) delete [] name;
}


const int Workspace::addWindow(BlackboxWindow *w, Bool place) {
  if (place) placeWindow(w);

  w->setWorkspace(id);
  w->setWindowNumber(windowList->count());
  
  windowList->insert(w);
  
  clientmenu->insert(w->getTitle());
  clientmenu->update();
  
  int i, k = windowList->count();
  Window *tmp_stack = new Window[k];
  for (i = 0; i < k - 1; i++)
    *(tmp_stack + i) = *(stack + i);
  
  *(tmp_stack + i) = w->getFrameWindow();
  
  if (stack) delete [] stack;
  stack = tmp_stack;
  if (screen->getCurrentWorkspaceID() == id)
    screen->stackWindows(stack, k);
  
  return w->getWindowNumber();
}


const int Workspace::removeWindow(BlackboxWindow *w) {
  int i = 0, ii = 0, k = windowList->count() - 1;
  Window *tmp_stack = new Window[k];

  for (i = 0; i < k + 1; i++)
    if (*(stack + i) != w->getFrameWindow())
      *(tmp_stack + (ii++)) = *(stack + i);
  
  if (stack) delete [] stack;
  stack = tmp_stack;

  windowList->remove((const int) w->getWindowNumber());
  
  clientmenu->remove(w->getWindowNumber());
  clientmenu->update();

  if (! clientmenu->getCount()) clientmenu->hide();

  if (screen->getCurrentWorkspaceID() == id)
    screen->stackWindows(stack, k);
  
  LinkedListIterator<BlackboxWindow> it(windowList);
  for (i = 0; it.current(); it++, i++)
    it.current()->setWindowNumber(i);

  return windowList->count();
}


void Workspace::setFocusWindow(int w) {
  if (w >= 0 && w < windowList->count())
    label = getWindow(w)->getTitle();
  else
    label = 0;

  clientmenu->setHighlight(w);  
  screen->getToolbar()->redrawWindowLabel(True);
}


int Workspace::showAll(void) {
  BlackboxWindow *win;

  screen->stackWindows(stack, windowList->count());

  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++) {
    win = it.current();
    if (! win->isIconic())
      win->deiconify();
  }
  
  return windowList->count();
}


int Workspace::hideAll(void) {
  BlackboxWindow *win;

  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++) {
    win = it.current();
    if (! win->isIconic())
      win->withdraw();
  }

  if (clientmenu->isVisible())
    clientmenu->hide();

  return windowList->count();
} 


int Workspace::removeAll(void) {
  BlackboxWindow *win;

  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++) {
    win = it.current();
    screen->getCurrentWorkspace()->addWindow(win);
    if (! win->isIconic())
      win->iconify();
  }

  return windowList->count();
}


void Workspace::raiseWindow(BlackboxWindow *w) {
  int i = 0, ii = 0, k = windowList->count();
  Window *tmp_stack = new Window[k];
  static int re_enter = 0;
  
  if (w->isTransient() && ! re_enter) {
    raiseWindow(w->getTransientFor());
  } else {
    for (i = 0; i < k; i++)
      if (*(stack + i) != w->getFrameWindow())
        *(tmp_stack + (ii++)) = *(stack + i);
    
    *(tmp_stack + ii++) = w->getFrameWindow();
    
    for (i = 0; i < k; ++i)
      *(stack + i) = *(tmp_stack + i);

    delete [] tmp_stack;

    if (w->hasTransient()) {
      if (! re_enter) {
	re_enter = 1;
	raiseWindow(w->getTransient());
	re_enter = 0;
      } else
	raiseWindow(w->getTransient());
    }

    if (! re_enter && id == screen->getCurrentWorkspaceID())
      screen->stackWindows(stack, k);
  }
}


void Workspace::lowerWindow(BlackboxWindow *w) {
  int i, ii = 1, k = windowList->count();
  Window *tmp_stack = new Window[k];
  static int re_enter = 0;
  
  if (w->hasTransient() && ! re_enter) {
    lowerWindow(w->getTransient());
  } else {
    for (i = 0; i < k; ++i)
      if (*(stack + i) != w->getFrameWindow())
	*(tmp_stack + (ii++)) = *(stack + i);
    
    *(tmp_stack) = w->getFrameWindow();

    for (i = 0; i < k; ++i)
      *(stack + i) = *(tmp_stack + i);
    
    delete [] tmp_stack;
    
    if (w->isTransient()) {
      if (! re_enter) {
	re_enter = 1;
	lowerWindow(w->getTransientFor());
	re_enter = 0;
      } else
	lowerWindow(w->getTransientFor());
    }
    
    if (! re_enter && id == screen->getCurrentWorkspaceID())
      screen->stackWindows(stack, k);
  }
}


void Workspace::reconfigure(void) {
  clientmenu->reconfigure();

  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++)
    if (it.current()->validateClient())
      it.current()->reconfigure();
}


BlackboxWindow *Workspace::getWindow(int index) {
  if ((index >= 0) && (index < windowList->count()))
    return windowList->find(index);
  else
    return 0;
}


const int Workspace::getCount(void) {
  return windowList->count();
}


void Workspace::update(void) {
  clientmenu->update();
  screen->getToolbar()->redrawWindowLabel(True);
}


void Workspace::restackWindows(void) {
  screen->stackWindows(stack, windowList->count());
}


Bool Workspace::isCurrent(void) {
  return (id == screen->getCurrentWorkspaceID());
}


void Workspace::setCurrent(void) {
  screen->changeWorkspaceID(id);
}


void Workspace::setName(char *new_name) {
  if (new_name) {
    if (name) delete [] name;

    int len = strlen(new_name) + 1;
    name = new char[len];
    strncpy(name, new_name, len);
  } else {
    if (name) delete [] name;
    
    name = new char[32];
    if (name)
      sprintf(name, "Workspace %d", id);
  }
}


void Workspace::shutdown(void) {
  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++)
    it.current()->restore();
}


void Workspace::placeWindow(BlackboxWindow *win) {
  int place_x = 0, place_y = 0;

  switch (screen->getPlacementPolicy()) {
  case BScreen::SmartPlacement:
    {
      Bool done = False;
      int test_x = 0, test_y = 0, test_w = 0, test_h = 0,
	extra = screen->getBevelWidth() * 2;
      
      test_y = extra;
      
      // adaptation from Window Maker's smart window placement
      
      while (((test_y + win->getHeight()) <
	      (unsigned) (screen->getYRes() -
			  screen->getToolbar()->getHeight() - 1)) &&
	     (! done)) {
	test_x = extra;
	
	while (((test_x + win->getWidth()) < (unsigned) screen->getXRes()) &&
	       (! done)) {
	  done = True;
	  
	  LinkedListIterator<BlackboxWindow> it(windowList);
	  for (; it.current() && done; it++) {
	    if (it.current()->isShaded())
	      test_h = it.current()->getTitleHeight();
	    else
	      test_h = it.current()->getHeight() + extra;
	    
	    if ((it.current()->getXFrame() <
		 (signed) (test_x + it.current()->getWidth())) &&
		((it.current()->getXFrame() + test_w) > test_x) &&
		(it.current()->getYFrame() <
		 (signed) (test_y + it.current()->getHeight())) &&
		((it.current()->getYFrame() + test_h) > test_y) &&
		(it.current()->isVisible() ||
		 (it.current()->isShaded() && (! it.current()->isIconic()))))
	      done = False;
	  }

	  it.reset();
	  for (; it.current() && done; it++) {
	    test_w = it.current()->getWidth();
	    
	    if (it.current()->isShaded())
	      test_h = it.current()->getTitleHeight();
	    else
	      test_h = it.current()->getHeight() + extra;
	    
	    if ((it.current()->getXFrame() <
		 (signed) (test_x + it.current()->getWidth())) &&
		((it.current()->getXFrame() + test_w) > test_x) &&
		(it.current()->getYFrame() <
		 (signed) (test_y + it.current()->getHeight())) &&
		((it.current()->getYFrame() + test_h) > test_y) &&
		(it.current()->isVisible() ||
		 (it.current()->isShaded() && (! it.current()->isIconic()))))
	      done = False;
	  }
	  
	  if (done) {
	    place_x = test_x;
	    place_y = test_y;
	    break;
	  }
	  
	  test_x += extra;
	}
	
	test_y += extra;
      }
      
      if (done)
	break;
    }
    
  case BScreen::CascadePlacement:
  default:
    if (((unsigned) cascade_x > (screen->getXRes() / 2)) ||
	((unsigned) cascade_y > (screen->getYRes() / 2)))
      cascade_x = cascade_y = 32;
    
    place_x = cascade_x;
    place_y = cascade_y;
    
    cascade_x += win->getTitleHeight();
    cascade_y += win->getTitleHeight();
    
    break;
  }
  
  if (place_x + win->getWidth() > screen->getXRes())
    place_x = (screen->getXRes() - win->getWidth()) / 2;
  if (place_y + win->getHeight() >
      (screen->getYRes() - screen->getToolbar()->getHeight() - 1))
    place_y = ((screen->getYRes() - screen->getToolbar()->getHeight() - 1) -
	       win->getHeight()) / 2;
  
  win->configure(place_x, place_y, win->getWidth(), win->getHeight());
}
