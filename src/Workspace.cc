// Workspace.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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

  char *tmp;
  name = (char *) 0;
  screen->getNameOfWorkspace(id, &tmp);
  setName(tmp);

  if (tmp)
    delete [] tmp;
}


Workspace::~Workspace(void) {
  delete stackingList;
  delete windowList;
  delete clientmenu;

  if (name)
    delete [] name;
}


const int Workspace::addWindow(BlackboxWindow *w, Bool place) {
  if (! w) return -1;

  if (place) placeWindow(w);

  w->setWorkspace(id);
  w->setWindowNumber(windowList->count());

  stackingList->insert(w, 0);
  windowList->insert(w);

  clientmenu->insert((const char **) w->getTitle());
  clientmenu->update();

  screen->updateNetizenWindowAdd(w->getClientWindow(), id);

  raiseWindow(w);

  return w->getWindowNumber();
}


const int Workspace::removeWindow(BlackboxWindow *w) {
  if (! w) return -1;

  stackingList->remove(w);

  if (w->isFocused()) {
    if (w->isTransient() && w->getTransientFor() &&
	w->getTransientFor()->isVisible()) {
      w->getTransientFor()->setInputFocus();
    } else if (screen->isSloppyFocus()) {
      screen->getBlackbox()->setFocusedWindow((BlackboxWindow *) 0);
    } else {
      BlackboxWindow *top = stackingList->first();
      if (! top || ! top->setInputFocus()) {
	screen->getBlackbox()->setFocusedWindow((BlackboxWindow *) 0);
	XSetInputFocus(screen->getBlackbox()->getXDisplay(),
		       screen->getToolbar()->getWindowID(),
		       RevertToParent, CurrentTime);
      }
    }
  }
  
  if (lastfocus == w)
    lastfocus = (BlackboxWindow *) 0;

  windowList->remove(w->getWindowNumber());
  clientmenu->remove(w->getWindowNumber());
  clientmenu->update();

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

  screen->getBlackbox()->grab();

  XLowerWindow(screen->getBaseDisplay()->getXDisplay(), *nstack);
  XRestackWindows(screen->getBaseDisplay()->getXDisplay(), nstack, i);

  screen->getBlackbox()->ungrab();

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
  
  clientmenu->setLabel(name);
  clientmenu->update();
}


void Workspace::shutdown(void) {
  while (windowList->count()) {
    windowList->first()->restore();
    delete windowList->first();
  }
}

void Workspace::placeWindow(BlackboxWindow *win) {
  Bool placed = False;

  const int win_w = win->getWidth() + (screen->getBorderWidth() * 4),
    win_h = win->getHeight() + (screen->getBorderWidth() * 4),
#ifdef    SLIT
    slit_x = screen->getSlit()->getX() - screen->getBorderWidth(),
    slit_y = screen->getSlit()->getY() - screen->getBorderWidth(),
    slit_w = screen->getSlit()->getWidth() +
      (screen->getBorderWidth() * 4),
    slit_h = screen->getSlit()->getHeight() +
      (screen->getBorderWidth() * 4),
#endif // SLIT
    toolbar_x = screen->getToolbar()->getX() - screen->getBorderWidth(),
    toolbar_y = screen->getToolbar()->getY() - screen->getBorderWidth(),
    toolbar_w = screen->getToolbar()->getWidth() +
      (screen->getBorderWidth() * 4),
    toolbar_h = screen->getToolbar()->getHeight() + 
      (screen->getBorderWidth() * 4),
    start_pos = screen->getBorderWidth() + screen->getEdgeSnapThreshold(),
    change_y =
      ((screen->getColPlacementDirection() == BScreen::TopBottom) ? 1 : -1),
    change_x =
      ((screen->getRowPlacementDirection() == BScreen::LeftRight) ? 1 : -1),
    delta_x = 8, delta_y = 8;

  int test_x, test_y, place_x = 0, place_y = 0;
  LinkedListIterator<BlackboxWindow> it(windowList);

  switch (screen->getPlacementPolicy()) {
  case BScreen::RowSmartPlacement: {
    test_y = (screen->getColPlacementDirection() == BScreen::TopBottom) ?
      start_pos : screen->getHeight() - win_h - start_pos;

    while (!placed &&
	   ((screen->getColPlacementDirection() == BScreen::BottomTop) ?
	    test_y > 0 : test_y + win_h < (signed) screen->getHeight())) {
      test_x = (screen->getRowPlacementDirection() == BScreen::LeftRight) ?
	start_pos : screen->getWidth() - win_w - start_pos;

      while (!placed &&
	     ((screen->getRowPlacementDirection() == BScreen::RightLeft) ?
	      test_x > 0 : test_x + win_w < (signed) screen->getWidth())) {
        placed = True;

        it.reset();
        for (BlackboxWindow *curr = it.current(); placed && curr;
	     it++, curr = it.current()) {
          int curr_w = curr->getWidth() + (screen->getBorderWidth() * 4);
          int curr_h =
	    ((curr->isShaded()) ? curr->getTitleHeight() : curr->getHeight()) +
            (screen->getBorderWidth() * 4);
	  
          if (curr->getXFrame() < test_x + win_w &&
              curr->getXFrame() + curr_w > test_x &&
              curr->getYFrame() < test_y + win_h &&
              curr->getYFrame() + curr_h > test_y) {
            placed = False;
	  }
        }

        if (placed &&
	    (toolbar_x < test_x + win_w &&
             toolbar_x + toolbar_w > test_x &&
             toolbar_y < test_y + win_h &&
             toolbar_y + toolbar_h > test_y)
#ifdef    SLIT
             ||
            (slit_x < test_x + win_w &&
             slit_x + slit_w > test_x &&
             slit_y < test_y + win_h &&
             slit_y + slit_h > test_y)
#endif // SLIT
	    )
          placed = False;

        if (placed) {
          place_x = test_x;
          place_y = test_y;

          break;
        }

	test_x += (change_x * delta_x);
      }

      test_y += (change_y * delta_y);
    }

    break;
  }

  case BScreen::ColSmartPlacement: {
    test_x = (screen->getRowPlacementDirection() == BScreen::LeftRight) ?
      start_pos : screen->getWidth() - win_w - start_pos;

    while (!placed &&
	   ((screen->getRowPlacementDirection() == BScreen::RightLeft) ?
	    test_x > 0 : test_x + win_w < (signed) screen->getWidth())) {
      test_y = (screen->getColPlacementDirection() == BScreen::TopBottom) ?
	start_pos : screen->getHeight() - win_h - start_pos;
      
      while (!placed &&
	     ((screen->getColPlacementDirection() == BScreen::BottomTop) ?
	      test_y > 0 : test_y + win_h < (signed) screen->getHeight())) {
        placed = True;

        it.reset();
        for (BlackboxWindow *curr = it.current(); placed && curr;
	     it++, curr = it.current()) {
          int curr_w = curr->getWidth() + (screen->getBorderWidth() * 4);
          int curr_h =
            ((curr->isShaded()) ? curr->getTitleHeight() : curr->getHeight()) +
            (screen->getBorderWidth() * 4);

          if (curr->getXFrame() < test_x + win_w &&
              curr->getXFrame() + curr_w > test_x &&
              curr->getYFrame() < test_y + win_h &&
              curr->getYFrame() + curr_h > test_y) {
            placed = False;
	  }
        }

        if (placed &&
	    (toolbar_x < test_x + win_w &&
	     toolbar_x + toolbar_w > test_x &&
	     toolbar_y < test_y + win_h &&
	     toolbar_y + toolbar_h > test_y)
#ifdef    SLIT
	    ||
	    (slit_x < test_x + win_w &&
	     slit_x + slit_w > test_x &&
	     slit_y < test_y + win_h &&
	     slit_y + slit_h > test_y)
#endif // SLIT
	    )
	  placed = False;

	if (placed) {
	  place_x = test_x;
	  place_y = test_y;

	  break;
	}

	test_y += (change_y * delta_y);
      }

      test_x += (change_x * delta_x);
    }

    break;
  }
  } // switch

  if (! placed) {
    if (((unsigned) cascade_x > (screen->getWidth() / 2)) ||
	((unsigned) cascade_y > (screen->getHeight() / 2)))
      cascade_x = cascade_y = 32;

    place_x = cascade_x;
    place_y = cascade_y;

    cascade_x += win->getTitleHeight();
    cascade_y += win->getTitleHeight();
  }
  
  if (place_x + win_w > (signed) screen->getWidth())
    place_x = (((signed) screen->getWidth()) - win_w) / 2;
  if (place_y + win_h > (signed) screen->getHeight())
    place_y = (((signed) screen->getHeight()) - win_h) / 2;

  win->configure(place_x, place_y, win->getWidth(), win->getHeight());
}
