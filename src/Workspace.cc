//
// workspace.cc for Blackbox - an X11 Window manager
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
#include "Window.hh"
#include "Workspace.hh"
#include "Windowmenu.hh"

#include <stdio.h>
#include <string.h>


// *************************************************************************
// Workspace class code
// *************************************************************************

Workspace::Workspace(Toolbar *t, int i) {
  toolbar = t;

  id = i;
  stack = 0;

  windowList = new LinkedList<BlackboxWindow>;
  cMenu = new Clientmenu(toolbar->_blackbox(), this);
  cMenu->Update();
  
  toolbar->_blackbox()->nameOfWorkspace(id, &name);
  
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
  
  cMenu->insert(w->Title());
  cMenu->Update();
  
  int i, k = windowList->count();
  Window *tmp_stack = new Window[k];
  for (i = 0; i < k - 1; i++)
    *(tmp_stack + i) = *(stack + i);
  
  *(tmp_stack + i) = w->frameWindow();
  
  if (stack) delete [] stack;
  stack = tmp_stack;
  if (toolbar->currentWorkspaceID() == id)
    toolbar->stackWindows(stack, k);
  
  return w->windowNumber();
}


const int Workspace::removeWindow(BlackboxWindow *w) {
  int i = 0, ii = 0, k = windowList->count() - 1;
  Window *tmp_stack = new Window[k];

  for (i = 0; i < k + 1; i++)
    if (*(stack + i) != w->frameWindow())
      *(tmp_stack + (ii++)) = *(stack + i);
  
  if (stack) delete [] stack;
  stack = tmp_stack;

  windowList->remove((const int) w->windowNumber());
  
  cMenu->remove(w->windowNumber());
  cMenu->Update();

  if (! cMenu->Count()) cMenu->Hide();

  if (toolbar->currentWorkspaceID() == id)
    toolbar->stackWindows(stack, k);
  
  LinkedListIterator<BlackboxWindow> it(windowList);
  for (i = 0; it.current(); it++, i++)
    it.current()->setWindowNumber(i);
  
  return windowList->count();
}


void Workspace::setFocusWindow(int w) {
  if (w >= 0 && w < windowList->count())
    label = window(w)->Title();
  else
    label = 0;

  cMenu->setHighlight(w);  
  toolbar->redrawLabel(True);
}


int Workspace::showAll(void) {
  BlackboxWindow *win;

  toolbar->stackWindows(stack, windowList->count());

  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++) {
    win = it.current();
    if (! win->isIconic())
      win->deiconifyWindow();
  }
  
  return windowList->count();
}


int Workspace::hideAll(void) {
  BlackboxWindow *win;

  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++) {
    win = it.current();
    if (! win->isIconic())
      win->withdrawWindow();
  }

  if (cMenu->Visible())
    cMenu->Hide();

  return windowList->count();
} 


int Workspace::removeAll(void) {
  BlackboxWindow *win;

  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++) {
    win = it.current();
    toolbar->currentWorkspace()->addWindow(win);
    if (! win->isIconic())
      win->iconifyWindow();
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
    raiseWindow(w->TransientFor());
  } else {
    for (i = 0; i < k; i++)
      if (*(stack + i) != w->frameWindow())
        *(tmp_stack + (ii++)) = *(stack + i);
    
    *(tmp_stack + ii++) = w->frameWindow();
    
    for (i = 0; i < k; ++i)
      *(stack + i) = *(tmp_stack + i);

    delete [] tmp_stack;

    if (w->hasTransient()) {
      if (! re_enter) {
	re_enter = 1;
	raiseWindow(w->Transient());
	re_enter = 0;
      } else
	raiseWindow(w->Transient());
    }

    if (! re_enter && id == toolbar->currentWorkspaceID())
      toolbar->stackWindows(stack, k);
  }
}


void Workspace::lowerWindow(BlackboxWindow *w) {
  int i, ii = 1, k = windowList->count();
  Window *tmp_stack = new Window[k];
  static int re_enter = 0;
  
  if (w->hasTransient() && ! re_enter) {
    lowerWindow(w->Transient());
  } else {
    for (i = 0; i < k; ++i)
      if (*(stack + i) != w->frameWindow())
	*(tmp_stack + (ii++)) = *(stack + i);
    
    *(tmp_stack) = w->frameWindow();

    for (i = 0; i < k; ++i)
      *(stack + i) = *(tmp_stack + i);
    
    delete [] tmp_stack;
    
    if (w->isTransient()) {
      if (! re_enter) {
	re_enter = 1;
	lowerWindow(w->TransientFor());
	re_enter = 0;
      } else
	lowerWindow(w->TransientFor());
    }
    
    if (! re_enter && id == toolbar->currentWorkspaceID())
      toolbar->stackWindows(stack, k);
  }
}


void Workspace::Reconfigure(void) {
  cMenu->Reconfigure();

  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++)
    if (toolbar->_blackbox()->validateWindow(it.current()->clientWindow()))
      it.current()->Reconfigure();
}


BlackboxWindow *Workspace::window(int index) {
  if ((index >= 0) && (index < windowList->count()))
    return windowList->find(index);
  else
    return 0;
}


const int Workspace::Count(void) {
  return windowList->count();
}


void Workspace::Update(void) {
  cMenu->Update();
  toolbar->redrawLabel(True);
}


void Workspace::restackWindows(void) {
  toolbar->stackWindows(stack, windowList->count());
}


Bool Workspace::isCurrent(void) {
  return (id == toolbar->currentWorkspaceID());
}


void Workspace::setCurrent(void) {
  toolbar->changeWorkspaceID(id);
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
