// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Workspace.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh at debian.org>
// Copyright (c) 1997 - 2000, 2002 Bradley T Hughes <bhughes at trolltech.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// stupid macros needed to access some functions in version 2 of the GNU C
// library
#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "i18n.hh"
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


Workspace::Workspace(BScreen *scrn, int i) {
  screen = scrn;

  cascade_x = cascade_y = 32;

  id = i;

  stackingList = new LinkedList<BlackboxWindow>;
  windowList = new LinkedList<BlackboxWindow>;
  clientmenu = new Clientmenu(this);

  lastfocus = (BlackboxWindow *) 0;

  name = (char *) 0;
  char *tmp = screen->getNameOfWorkspace(id);
  setName(tmp);
}


Workspace::~Workspace(void) {
  delete stackingList;
  delete windowList;
  delete clientmenu;

  if (name)
    delete [] name;
}


int Workspace::addWindow(BlackboxWindow *w, Bool place)
{
  if (! w)
    return -1;

  if (place)
    placeWindow(w);

  w->setWorkspace(id);
  w->setWindowNumber(windowList->count());

  stackingList->insert(w, 0);
  windowList->insert(w);

  clientmenu->insert(*w->getTitle());

  screen->updateNetizenWindowAdd(w->getClientWindow(), id);

  raiseWindow(w);

  return w->getWindowNumber();
}

int Workspace::removeWindow(BlackboxWindow *w)
{
  if (! w)
    return -1;

  stackingList->remove(w);

  if (w->isFocused()) {
    if (w->isTransient() && w->getTransientFor() &&
	w->getTransientFor()->isVisible()) {
      w->getTransientFor()->setInputFocus();
    } else if (screen->isSloppyFocus()) {
      Blackbox::instance()->setFocusedWindow((BlackboxWindow *) 0);
    } else {
      BlackboxWindow *top = stackingList->first();
      if (! top || ! top->setInputFocus()) {
	Blackbox::instance()->setFocusedWindow((BlackboxWindow *) 0);
	XSetInputFocus(*BaseDisplay::instance(),
		       screen->getToolbar()->getWindowID(),
		       RevertToParent, CurrentTime);
      }
    }
  }

  if (lastfocus == w)
    lastfocus = (BlackboxWindow *) 0;

  windowList->remove(w->getWindowNumber());
  clientmenu->remove(w->getWindowNumber());

  screen->updateNetizenWindowDel(w->getClientWindow());

  LinkedListIterator<BlackboxWindow> it(windowList);
  BlackboxWindow *bw = it.current();
  for (int i = 0; bw; it++, i++, bw = it.current())
    bw->setWindowNumber(i);

  return windowList->count();
}


void Workspace::showAll(void) {
  LinkedListIterator<BlackboxWindow> it(stackingList);
  for (BlackboxWindow *bw = it.current(); bw; it++, bw = it.current())
    bw->deiconify(False, False);
}


void Workspace::hideAll(void) {
  LinkedList<BlackboxWindow> lst;

  LinkedListIterator<BlackboxWindow> it(stackingList);
  for (BlackboxWindow *bw = it.current(); bw; it++, bw = it.current())
    lst.insert(bw, 0);

  LinkedListIterator<BlackboxWindow> it2(&lst);
  for (BlackboxWindow *bw = it2.current(); bw; it2++, bw = it2.current())
    if (! bw->isStuck())
      bw->withdraw();
}


void Workspace::removeAll(void) {
  LinkedListIterator<BlackboxWindow> it(windowList);
  for (BlackboxWindow *bw = it.current(); bw; it++, bw = it.current())
    bw->iconify();
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

  Window *nstack = new Window[i], *curr = nstack;
  Workspace *wkspc;

  win = bottom;
  while (True) {
    *(curr++) = win->getFrameWindow();
    screen->updateNetizenWindowRaise(win->getClientWindow());

    if (! win->isIconic()) {
      wkspc = screen->getWorkspace(win->getWorkspaceNumber());
      wkspc->stackingList->remove(win);
      wkspc->stackingList->insert(win, 0);
    }

    if (! win->hasTransient() || ! win->getTransient())
      break;

    win = win->getTransient();
  }

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

  Window *nstack = new Window[i], *curr = nstack;
  Workspace *wkspc;

  while (True) {
    *(curr++) = win->getFrameWindow();
    screen->updateNetizenWindowLower(win->getClientWindow());

    if (! win->isIconic()) {
      wkspc = screen->getWorkspace(win->getWorkspaceNumber());
      wkspc->stackingList->remove(win);
      wkspc->stackingList->insert(win);
    }

    if (! win->getTransientFor())
      break;

    win = win->getTransientFor();
  }

  XLowerWindow(*BaseDisplay::instance(), *nstack);
  XRestackWindows(*BaseDisplay::instance(), nstack, i);

  delete [] nstack;
}


void Workspace::reconfigure(void) {
  clientmenu->reconfigure();

  LinkedListIterator<BlackboxWindow> it(windowList);
  for (BlackboxWindow *bw = it.current(); bw; it++, bw = it.current()) {
    if (bw->validateClient())
      bw->reconfigure();
  }
}


BlackboxWindow *Workspace::getWindow(int index) {
  if ((index >= 0) && (index < windowList->count()))
    return windowList->find(index);
  else
    return 0;
}

void Workspace::changeName(BlackboxWindow *window)
{
  clientmenu->change(window->getWindowNumber(), *window->getTitle());
  screen->getToolbar()->redrawWindowLabel(True);
}

Bool Workspace::isCurrent(void) {
  return (id == screen->getCurrentWorkspaceID());
}


Bool Workspace::isLastWindow(BlackboxWindow *w) {
  return (w == windowList->last());
}

void Workspace::setCurrent(void) {
  screen->changeWorkspaceID(id);
}


void Workspace::setName(char *new_name) {
  if (name)
    delete [] name;

  if (new_name) {
    name = bstrdup(new_name);
  } else {
    name = new char[128];
    sprintf(name, i18n->getMessage(WorkspaceSet, WorkspaceDefaultNameFormat,
				   "Workspace %d"), id + 1);
  }

  clientmenu->setTitle(name);
  clientmenu->showTitle();
}


void Workspace::shutdown(void) {
  while (windowList->count()) {
    windowList->first()->restore();
    delete windowList->first();
  }
}

void Workspace::placeWindow(BlackboxWindow *win) {
  Bool placed = False;

  XRectangle availableArea = screen->availableArea();

  const int win_w = win->getWidth() + (screen->style()->borderWidth() * 4),
    win_h = win->getHeight() + (screen->style()->borderWidth() * 4),
    start_pos_x = availableArea.x, start_pos_y = availableArea.y,
    change_y =
      ((screen->getColPlacementDirection() == BScreen::TopBottom) ? 1 : -1),
    change_x =
      ((screen->getRowPlacementDirection() == BScreen::LeftRight) ? 1 : -1),
    delta_x = 8, delta_y = 8;

  int place_x = start_pos_x, place_y = start_pos_y;
  LinkedListIterator<BlackboxWindow> it(windowList);

  switch (screen->getPlacementPolicy()) {
  case BScreen::RowSmartPlacement: {
    place_y = (screen->getColPlacementDirection() == BScreen::TopBottom) ?
      start_pos_y : availableArea.height - win_h - start_pos_y;

    while (!placed &&
	   ((screen->getColPlacementDirection() == BScreen::BottomTop) ?
	    place_y > 0 : place_y + win_h < (signed) availableArea.height)) {
      place_x = (screen->getRowPlacementDirection() == BScreen::LeftRight) ?
	start_pos_x : availableArea.width - win_w - start_pos_x;

      while (!placed &&
	     ((screen->getRowPlacementDirection() == BScreen::RightLeft) ?
	      place_x > 0 : place_x + win_w < (signed) availableArea.width)) {
        placed = True;

        it.reset();
        for (BlackboxWindow *curr = it.current(); placed && curr;
	     it++, curr = it.current()) {
          int curr_w = curr->getWidth() + (screen->style()->borderWidth() * 4);
          int curr_h =
	    ((curr->isShaded()) ? curr->getTitleHeight() : curr->getHeight()) +
            (screen->style()->borderWidth() * 4);

          if (curr->getXFrame() < place_x + win_w &&
              curr->getXFrame() + curr_w > place_x &&
              curr->getYFrame() < place_y + win_h &&
              curr->getYFrame() + curr_h > place_y) {
            placed = False;
	  }
        }

        if (! placed)
	  place_x += (change_x * delta_x);
      }

      if (! placed)
	place_y += (change_y * delta_y);
    }

    break;
  }

  case BScreen::ColSmartPlacement: {
    place_x = (screen->getRowPlacementDirection() == BScreen::LeftRight) ?
      start_pos_x : availableArea.width - win_w - start_pos_x;

    while (!placed &&
	   ((screen->getRowPlacementDirection() == BScreen::RightLeft) ?
	    place_x > 0 : place_x + win_w < (signed) availableArea.width)) {
      place_y = (screen->getColPlacementDirection() == BScreen::TopBottom) ?
	start_pos_y : availableArea.height - win_h - start_pos_y;

      while (!placed &&
	     ((screen->getColPlacementDirection() == BScreen::BottomTop) ?
	      place_y > 0 : place_y + win_h < (signed) availableArea.height)) {
        placed = True;

        it.reset();
        for (BlackboxWindow *curr = it.current(); placed && curr;
	     it++, curr = it.current()) {
          int curr_w = curr->getWidth() + (screen->style()->borderWidth() * 4);
          int curr_h =
            ((curr->isShaded()) ? curr->getTitleHeight() : curr->getHeight()) +
            (screen->style()->borderWidth() * 4);

          if (curr->getXFrame() < place_x + win_w &&
              curr->getXFrame() + curr_w > place_x &&
              curr->getYFrame() < place_y + win_h &&
              curr->getYFrame() + curr_h > place_y) {
            placed = False;
	  }
        }

	if (! placed)
	  place_y += (change_y * delta_y);
      }

      if (! placed)
	place_x += (change_x * delta_x);
    }

    break;
  }
  } // switch

  if (! placed) {
    if (((unsigned) cascade_x > (availableArea.width / (unsigned) 2)) ||
	((unsigned) cascade_y > (availableArea.height / (unsigned) 2)))
      cascade_x = cascade_y = 32;

    place_x = cascade_x;
    place_y = cascade_y;

    cascade_x += win->getTitleHeight();
    cascade_y += win->getTitleHeight();
  }

  if (place_x + win_w > (signed) availableArea.width)
    place_x = (((signed) availableArea.width) - win_w) / 2;
  if (place_y + win_h > (signed) availableArea.height)
    place_y = (((signed) availableArea.height) - win_h) / 2;

  win->configure(place_x, place_y, win->getWidth(), win->getHeight());
}
