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

// *************************************************************************
// Workspace class code
// *************************************************************************

Workspace::Workspace(Toolbar *tbar, BScreen *scrn, int i) {
  toolbar = tbar;
  screen = scrn;

  id = i;
  stack = 0;

  windowList = new LinkedList<BlackboxWindow>;
  cMenu = new Clientmenu(screen->getBlackbox(), this);
  cMenu->update();
  
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
  delete cMenu;

  if (stack) delete [] stack;
  if (name) delete [] name;
}


const int Workspace::addWindow(BlackboxWindow *w) {
  w->setWorkspace(id);
  w->setWindowNumber(windowList->count());
  
  windowList->insert(w);
  
  cMenu->insert(w->getTitle());
  cMenu->update();
  
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
  
  cMenu->remove(w->getWindowNumber());
  cMenu->update();

  if (! cMenu->getCount()) cMenu->hide();

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

  cMenu->setHighlight(w);  
  toolbar->redrawWindowLabel(True);
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

  if (cMenu->isVisible())
    cMenu->hide();

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
  // this just arranges the windows... and then passes it to the workspace
  // manager... which adds this stack to it's own stack... and the workspace
  // manager then tells the X server to restack the windows

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
  cMenu->reconfigure();

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
  cMenu->update();
  toolbar->redrawWindowLabel(True);
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
