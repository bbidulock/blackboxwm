// Workspace.cc for Blackbox - an X11 Window manager
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

#define MIN(x,y) ((x < y) ? x : y)
#define MAX(x,y) ((x > y) ? x : y)


Workspace::Workspace(BScreen *scrn, int i) {
  screen = scrn;

  cascade_x = cascade_y = 32;

  id = i;

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

  windowList->insert(w);

  clientmenu->insert((const char **) w->getTitle());
  clientmenu->update();

  screen->updateNetizenWindowAdd(w->getClientWindow(), id);

  raiseWindow(w);

  return w->getWindowNumber();
}


const int Workspace::removeWindow(BlackboxWindow *w) {
  if (! w) return -1;

  if (w->isFocused()) {
    if (screen->isSloppyFocus())
      screen->getBlackbox()->setFocusedWindow((BlackboxWindow *) 0);
    else if (w->isTransient() && w->getTransientFor() &&
	     w->getTransientFor()->isVisible())
      w->getTransientFor()->setInputFocus();
    else {
      screen->nextFocus();
      
      if (w->isFocused()) {
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
  for (int i = 0; it.current(); it++, i++)
    it.current()->setWindowNumber(i);

  return windowList->count();
}


void Workspace::showAll(void) {
  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++)
    it.current()->deiconify(False, False);
}


void Workspace::hideAll(void) {
  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++)
    if (! it.current()->isStuck())
      it.current()->withdraw();
}


void Workspace::removeAll(void) {
  LinkedListIterator<BlackboxWindow> it(windowList);
  for (; it.current(); it++)
    it.current()->iconify();
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
    screen->updateNetizenWindowRaise(win->getClientWindow());

    win = win->getTransient();
  }

  *(nstack + (i++)) = win->getFrameWindow();
  screen->updateNetizenWindowRaise(win->getClientWindow());

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
  while (win->getTransientFor()) {
    *(nstack + (i++)) = win->getFrameWindow();
    screen->updateNetizenWindowLower(win->getClientWindow());

    win = win->getTransientFor();
  }

  *(nstack + (i++)) = win->getFrameWindow();
  screen->updateNetizenWindowLower(win->getClientWindow());

  screen->getBlackbox()->grab();

  XLowerWindow(screen->getBaseDisplay()->getXDisplay(), *nstack);
  XRestackWindows(screen->getBaseDisplay()->getXDisplay(), nstack, i);

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
    name = bstrdup(i18n->
		   getMessage(
#ifdef    NLS
			      WorkspaceSet, WorkspaceDefaultNameFormat,
#else // !NLS
			      0, 0,
#endif // NLS
			      "Workspace %d"));
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
  LinkedListIterator<BlackboxWindow> it(windowList);
  int win_w = win->getWidth() + (screen->getBorderWidth2x() * 2),
    win_h = win->getHeight() + (screen->getBorderWidth2x() * 2),
#ifdef    SLIT
    slit_x = screen->getSlit()->getX() - screen->getBorderWidth(),
    slit_y = screen->getSlit()->getY() - screen->getBorderWidth(),
    slit_w = screen->getSlit()->getWidth() +
      (screen->getBorderWidth2x() * 2),
    slit_h = screen->getSlit()->getHeight() +
      (screen->getBorderWidth2x() * 2),
#endif // SLIT
    toolbar_x = screen->getToolbar()->getX() - screen->getBorderWidth(),
    toolbar_y = screen->getToolbar()->getY() - screen->getBorderWidth(),
    toolbar_w = screen->getToolbar()->getWidth() +
      (screen->getBorderWidth2x() * 2),
    toolbar_h = screen->getToolbar()->getHeight() +
      (screen->getBorderWidth2x() * 2),
    place_x = 0, place_y = 0, change_x = 1, change_y = 1;

  if (screen->getColPlacementDirection() == BScreen::BottomTop)
    change_y = -1;
  if (screen->getRowPlacementDirection() == BScreen::RightLeft)
    change_x = -1;

  register int test_x, test_y, curr_w, curr_h;

  switch (screen->getPlacementPolicy()) {
  case BScreen::RowSmartPlacement: {
    test_y = screen->getBorderWidth() + screen->getEdgeSnapThreshold();
    if (screen->getColPlacementDirection() == BScreen::BottomTop)
      test_y = screen->getHeight() - win_h - test_y;

    while (((screen->getColPlacementDirection() == BScreen::BottomTop) ?
	    test_y > 0 :    test_y + win_h < (signed) screen->getHeight()) &&
	   ! placed) {
      test_x = screen->getBorderWidth() + screen->getEdgeSnapThreshold();
      if (screen->getRowPlacementDirection() == BScreen::RightLeft)
	test_x = screen->getWidth() - win_w - test_x;

      while (((screen->getRowPlacementDirection() == BScreen::RightLeft) ?
	      test_x > 0 : test_x + win_w < (signed) screen->getWidth()) &&
	     ! placed) {
        placed = True;

        it.reset();
        for (; it.current() && placed; it++) {
          curr_w = it.current()->getWidth() + screen->getBorderWidth2x() +
            screen->getBorderWidth2x();
          curr_h =
            ((it.current()->isShaded()) ? it.current()->getTitleHeight() :
                                          it.current()->getHeight()) +
            screen->getBorderWidth2x() + screen->getBorderWidth2x();

          if (it.current()->getXFrame() < test_x + win_w &&
              it.current()->getXFrame() + curr_w > test_x &&
              it.current()->getYFrame() < test_y + win_h &&
              it.current()->getYFrame() + curr_h > test_y)
            placed = False;
        }

        if ((toolbar_x < test_x + win_w &&
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

        test_x += change_x;
      }

      test_y += change_y;
    }

    break; }

  case BScreen::ColSmartPlacement: {
    test_x = screen->getBorderWidth() + screen->getEdgeSnapThreshold();
    if (screen->getRowPlacementDirection() == BScreen::RightLeft)
      test_x = screen->getWidth() - win_w - test_x;

    while (((screen->getRowPlacementDirection() == BScreen::RightLeft) ?
	    test_x > 0 : test_x + win_w < (signed) screen->getWidth()) &&
	   ! placed) {
      test_y = screen->getBorderWidth() + screen->getEdgeSnapThreshold();
      if (screen->getColPlacementDirection() == BScreen::BottomTop)
	test_y = screen->getHeight() - win_h - test_y;

      while (((screen->getColPlacementDirection() == BScreen::BottomTop) ?
	      test_y > 0 : test_y + win_h < (signed) screen->getHeight()) &&
	     ! placed) {
        placed = True;

        it.reset();
        for (; it.current() && placed; it++) {
          curr_w = it.current()->getWidth() + screen->getBorderWidth2x() +
            screen->getBorderWidth2x();
          curr_h =
            ((it.current()->isShaded()) ? it.current()->getTitleHeight() :
                                          it.current()->getHeight()) +
            screen->getBorderWidth2x() + screen->getBorderWidth2x();

          if (it.current()->getXFrame() < test_x + win_w &&
              it.current()->getXFrame() + curr_w > test_x &&
              it.current()->getYFrame() < test_y + win_h &&
              it.current()->getYFrame() + curr_h > test_y)
            placed = False;
        }

        if ((toolbar_x < test_x + win_w &&
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

       test_y += change_y;
     }

     test_x += change_x;
   }

   break; }
  }

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
    place_x = (screen->getWidth() - win_w) / 2;
  if (place_y + win_h > (signed) screen->getHeight())
    place_y = (screen->getHeight() - win_h) / 2;

  win->configure(place_x, place_y, win->getWidth(), win->getHeight());
}
