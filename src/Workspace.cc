//
// workspace.cc for Blackbox - an X11 Window manager
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

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "blackbox.hh"
#include "Clientmenu.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Windowmenu.hh"

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <string.h>
#endif // STDC_HEADERS

#define MIN(x,y) ((x < y) ? x : y)
#define MAX(x,y) ((x > y) ? x : y)


Workspace::Workspace(BScreen *scrn, int i) {
  screen = scrn;

  cascade_x = cascade_y = 32;

  id = i;

  windowList = new LinkedList<BlackboxWindow>;
  clientmenu = new Clientmenu(screen->getBlackbox(), this);
  //  clientmenu->update();

#ifdef    KDE
  char t[1024];
  sprintf(t, "KWM_DESKTOP_NAME_%d", id + 1);
  
  desktop_name_atom = XInternAtom(screen->getDisplay(), t, False);
#endif // KDE
  
  char *tmp;  
  name = (char *) 0;
  screen->getNameOfWorkspace(id, &tmp);
  setName(tmp);
  
  if (tmp) delete [] tmp;
  
  label = 0;
}


Workspace::~Workspace(void) {
  while (windowList->count())
    delete windowList->remove(0);
  
  delete windowList;
  delete clientmenu;

  if (name) delete [] name;
}


const int Workspace::addWindow(BlackboxWindow *w, Bool place) {
  if (place) placeWindow(w);

  w->setWorkspace(id);
  w->setWindowNumber(windowList->count());
  
  windowList->insert(w);
  
  clientmenu->insert(w->getTitle());
  clientmenu->update();

  raiseWindow(w);
  
  return w->getWindowNumber();
}


const int Workspace::removeWindow(BlackboxWindow *w) {
  windowList->remove((const int) w->getWindowNumber());
  
  if (clientmenu->getHighlight() == w->getWindowNumber())
    setFocusWindow(-1);

  clientmenu->remove(w->getWindowNumber());
  clientmenu->update();

  if (! clientmenu->getCount()) clientmenu->hide();

  LinkedListIterator<BlackboxWindow> it(windowList);
  for (int i = 0; it.current(); it++, i++)
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
  
  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++) {
    win = it.current();
    if (! win->isIconic())
      win->deiconify(False);
  }
  
  return windowList->count();
}


int Workspace::hideAll(void) {
  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++) {
    BlackboxWindow *win = it.current();
    if ((! win->isIconic()) && (! win->isStuck()))
      win->withdraw();
  }

  if (clientmenu->isVisible())
    clientmenu->hide();

  return windowList->count();
} 


int Workspace::removeAll(void) {
  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++) {
    BlackboxWindow *win = it.current();
    screen->getCurrentWorkspace()->addWindow(win);
    if (! win->isIconic())
      win->iconify();
  }

  return windowList->count();
}


void Workspace::raiseWindow(BlackboxWindow *w) {
  BlackboxWindow *win = (BlackboxWindow *) 0, *bottom = w;
  
  while (bottom->isTransient() && bottom->getTransientFor())
    bottom = bottom->getTransientFor();
  
  int i = 1;
  win = bottom;
  while (win->hasTransient() && win->getTransient()) {
    win = win->getTransient();
    
    i++;
  }
  
  Window *nstack = new Window[i];
  
  i = 0;
  win = bottom;
  while (win->hasTransient() && win->getTransient()) {
    *(nstack + (i++)) = win->getFrameWindow();
    win = win->getTransient();
  }
  
  *(nstack + (i++)) = win->getFrameWindow();
  
  screen->raiseWindows(nstack, i);
  
  delete [] nstack;
}


void Workspace::lowerWindow(BlackboxWindow *w) {
  BlackboxWindow *win = (BlackboxWindow *) 0, *bottom = w;
  
  while (bottom->isTransient() && bottom->getTransientFor())
    bottom = bottom->getTransientFor();
  
  int i = 1;
  win = bottom;
  while (win->hasTransient() && win->getTransient()) {
    win = win->getTransient();
    
    i++;
  }
  
  Window *nstack = new Window[i];
  
  i = 0;
  win = bottom;
  while (win->hasTransient() && win->getTransient()) {
    *(nstack + (i++)) = win->getFrameWindow();
    win = win->getTransient();
  }
  
  *(nstack + (i++)) = win->getFrameWindow();
  
  screen->getBlackbox()->grab();
  
  XLowerWindow(screen->getDisplay(), *nstack);
  XRestackWindows(screen->getDisplay(), nstack, i);
  
  screen->getBlackbox()->ungrab();
  
  delete [] nstack;
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
    sprintf(name, "%s", new_name);
  } else {
    if (name) delete [] name;
    
    name = new char[32];
    if (name)
      sprintf(name, "Workspace %d", id + 1);
  }

  clientmenu->setLabel(name);
  clientmenu->update();

#ifdef    KDE
  if (desktop_name_atom) {
    XChangeProperty(screen->getDisplay(), screen->getRootWindow(),
                    desktop_name_atom, XA_STRING, 8, PropModeReplace,
                    (unsigned char *) name, strlen(name) + 1);
    
    screen->sendToKWMModules(screen->getBlackbox()->
			     getKWMModuleDesktopNameChangeAtom(), (XID) id);
  }
#endif // KDE
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
      LinkedListIterator<BlackboxWindow> it(windowList);

      register int x_origin = 0, y_origin = 0, lap_x, lap_y, test_x, test_y;
      long best_cover = 0, cover = 0;
      unsigned int best_x = 0, best_y = 0, test_w = 0, test_h = 0,
        win_w = win->getWidth() + 2, win_h = win->getHeight() + 2,
        max_x = (unsigned) screen->getXRes(),
        max_y = (unsigned) (screen->getYRes() -
                            screen->getToolbar()->getHeight() - 1);

#ifdef    KDE
      int junk;
      
      getKWMWindowRegion(&x_origin, &y_origin, &junk, &junk);
#endif // KDE
      
      // smarterplacement from Dyon Balding... modified by Brad Hughes
     
      test_y = y_origin;

      while (((test_y + win_h) < max_y) && (! done)) {
	test_x = x_origin;
	
	while (((test_x + win_w) < max_x) && (! done)) {
	  done = True;
          cover = 0;
	  
	  for (; it.current(); it++) {
	    if ((! it.current()->isStuck()) && (! it.current()->isVisible()) &&
		(! it.current()->isShaded() || (it.current()->isIconic())))
	      continue;
	    
	    test_w = it.current()->getXFrame() + it.current()->getWidth() + 2;
	    test_h = it.current()->getYFrame() + it.current()->getHeight() + 2;
	    
	    lap_x =MIN(test_x + win_w, test_w) -
	      MAX(test_x, it.current()->getXFrame());
	    lap_y = MIN(test_y + win_h, test_h) -
	      MAX(test_y, it.current()->getYFrame());
	    
	    if (lap_x < 0)
	      lap_x = 0;
	    if (lap_y < 0)
	      lap_y = 0;
	    
	    cover += (lap_x * lap_y);
	    if (lap_x && lap_y)
	      done = False;
	  }

          it.reset();
	  
	  if (done) {
	    place_x = test_x;
	    place_y = test_y;
	    break;
	  }

	  if ((cover < best_cover) || (! best_cover)) {
	    best_x = test_x;
	    best_y = test_y;
	    best_cover = cover;
	  }
	  
	  test_x += 2;
	}
	
	test_y += 2;
      }
      
      if (done)
	break;
      
      place_x = best_x;
      place_y = best_y;
      
      break;
    }
    
  case BScreen::CascadePlacement:
  default:
    if (((unsigned) cascade_x > (screen->getXRes() / 2)) ||
	((unsigned) cascade_y > (screen->getYRes() / 2)))
      cascade_x = cascade_y = 32;
    
    
#ifdef    KDE
    int x_origin = 0, y_origin = 0, junk = 0;
    getKWMWindowRegion(&x_origin, &y_origin, &junk, &junk);
    
    place_x = cascade_x + x_origin;
    place_y = cascade_y + y_origin;
#else  // KDE
    place_x = cascade_x;
    place_y = cascade_y;
#endif // KDE
    
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


#ifdef    KDE
void Workspace::getKWMWindowRegion(int *x1, int *y1, int *x2, int *y2) {
  Atom ajunk;

  int ijunk;
  unsigned long uljunk, *data = (unsigned long *) 0;

  if (XGetWindowProperty(screen->getDisplay(), screen->getRootWindow(),
                         screen->getBlackbox()->getKWMWindowRegion1Atom(),
                         0l, 4l, False,
                         screen->getBlackbox()->getKWMWindowRegion1Atom(),
                         &ajunk, &ijunk, &uljunk, &uljunk,
                         (unsigned char **) &data) == Success && data) {
    *x1 = (int) data[0];
    *y1 = (int) data[1];
    *x2 = (int) data[2];
    *y2 = (int) data[3];

    XFree((char *) data);
  }
}


void Workspace::rereadName(void) {
  Atom ajunk;
  
  char *new_name = 0;
  int ijunk;
  unsigned long uljunk;
  
  if (XGetWindowProperty(screen->getDisplay(), screen->getRootWindow(),
			 desktop_name_atom, 0l, ~0l, False, XA_STRING,
			 &ajunk, &ijunk, &uljunk, &uljunk,
			 (unsigned char **) &new_name) == Success &&
      new_name) {
    if (strcmp(new_name, name)) {
      setName(new_name);
      screen->getToolbar()->redrawWorkspaceLabel(True);
    }
    
    XFree((char *) new_name);
  }
}
#endif // KDE
