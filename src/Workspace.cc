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

#include <stdio.h>


// *************************************************************************
// Workspace class code
// *************************************************************************

Workspace::Workspace(WorkspaceManager *m, int i) {
  wsManager = m;

  id = i;
  stack = 0;

  windowList = new LinkedList<BlackboxWindow>;
  cMenu = new Clientmenu(m->_blackbox(), this);
  cMenu->Update();

  name = new char[32];
  label = new char[32];
  sprintf(name, "Workspace %d", id);
  sprintf(label, "0 window(s)");
}


Workspace::~Workspace(void) {
  delete windowList;
  delete cMenu;

  delete [] name;
  delete [] label;
}


const int Workspace::addWindow(BlackboxWindow *w) {
  w->setWorkspace(id);
  w->setWindowNumber(windowList->count());

  windowList->insert(w);
  sprintf(label, "%d window(s)", windowList->count());
  wsManager->redrawWSD(True);

  cMenu->insert(w->Title());
  cMenu->Update();

  Window *tmp_stack = new Window[windowList->count()];
  int i;
  for (i = 0; i < (windowList->count() - 1); ++i)
    *(tmp_stack + i) = *(stack + i);

  *(tmp_stack + i) = w->frameWindow();

  delete [] stack;
  stack = tmp_stack;
  if (wsManager->currentWorkspaceID() == id)
    wsManager->stackWindows(stack, windowList->count());

  return w->windowNumber();
}


const int Workspace::removeWindow(BlackboxWindow *w) {
  Window *tmp_stack = new Window[windowList->count() - 1];

  int i = 0, ii = 0;
  for (i = 0; i < windowList->count(); ++i)
    if (*(stack + i) != w->frameWindow())
      *(tmp_stack + (ii++)) = *(stack + i);

  delete [] stack;
  stack = tmp_stack;

  windowList->remove((const int) w->windowNumber());
  sprintf(label, "%d window(s)", windowList->count());
  wsManager->redrawWSD(True);

  cMenu->remove(w->windowNumber());
  cMenu->Update();

  if (wsManager->currentWorkspaceID() == id)
    wsManager->stackWindows(stack, windowList->count());

  LinkedListIterator<BlackboxWindow> it(windowList);
  for (i = 0; it.current(); it++, i++)
    it.current()->setWindowNumber(i);

  return windowList->count();
}


int Workspace::showAll(void) {
  BlackboxWindow *win;

  wsManager->stackWindows(stack, windowList->count());

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
    wsManager->currentWorkspace()->addWindow(win);
    if (! win->isIconic())
      win->iconifyWindow();
  }

  return windowList->count();
}


void Workspace::Dissociate(void) {
  Display *display = wsManager->_blackbox()->control();

  XGrabServer(display);
  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++) {
    if (wsManager->_blackbox()->validateWindow(it.current()->clientWindow())) {
      XUnmapWindow(display, it.current()->frameWindow());
      XReparentWindow(display, it.current()->clientWindow(),
		      wsManager->_blackbox()->Root(), it.current()->XClient(),
		      it.current()->YClient());
      XMoveResizeWindow(display, it.current()->clientWindow(),
			it.current()->XClient(),
			it.current()->YClient(),
			it.current()->clientWidth(),
			it.current()->clientHeight());
      XMapWindow(display, it.current()->clientWindow());
    }
  }
  
  XSync(display, False);
  XUngrabServer(display);
}


void Workspace::raiseWindow(BlackboxWindow *w) {
  // this just arranges the windows... and then passes it to the workspace
  // manager... which adds this stack to it's own stack... and the workspace
  // manager then tells the X server to restack the windows

  Window *tmp_stack = new Window[windowList->count()];
  static int re_enter = 0;
  int i = 0, ii = 0;

  if (w->isTransient() && ! re_enter) {
    raiseWindow(w->TransientFor());
  } else {
    for (i = 0; i < windowList->count(); ++i)
      if (*(stack + i) != w->frameWindow())
        *(tmp_stack + (ii++)) = *(stack + i);
  
    *(tmp_stack + windowList->count() - 1) = w->frameWindow();
  
    for (i = 0; i < windowList->count(); ++i)
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

    if (! re_enter && id == wsManager->currentWorkspaceID())
      wsManager->stackWindows(stack, windowList->count());
  }
}


void Workspace::lowerWindow(BlackboxWindow *w) {
  Window *tmp_stack = new Window[windowList->count()];
  int i, ii = 1;
  static int re_enter = 0;
  
  if (w->hasTransient() && ! re_enter) {
    lowerWindow(w->Transient());
  } else {
    for (i = 0; i < windowList->count(); ++i)
      if (*(stack + i) != w->frameWindow())
	*(tmp_stack + (ii++)) = *(stack + i);
    
    *(tmp_stack) = w->frameWindow();
    
    for (i = 0; i < windowList->count(); ++i)
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
    
    if (! re_enter && id == wsManager->currentWorkspaceID())
      wsManager->stackWindows(stack, windowList->count());
  }
}


void Workspace::Reconfigure(void) {
  cMenu->Reconfigure();

  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++)
    if (wsManager->_blackbox()->validateWindow(it.current()->clientWindow()))
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
}


void Workspace::restackWindows(void) {
  wsManager->stackWindows(stack, windowList->count());
}
