// -*- mode: C++; indent-tabs-mode: nil; -*-
// Workspace.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh@debian.org>
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef HAVE_STRING_H
#  include <string.h>
#endif // HAVE_STRING_H
}

#include "i18n.hh"
#include "blackbox.hh"
#include "Clientmenu.hh"
#include "Netizen.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Util.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Windowmenu.hh"

#include <functional>

using std::string;


Workspace::Workspace(BScreen *scrn, unsigned int i) {
  screen = scrn;

  cascade_x = cascade_y = 32;

  id = i;

  clientmenu = new Clientmenu(this);

  lastfocus = (BlackboxWindow *) 0;

  setName(screen->getNameOfWorkspace(id));
}


/*
 * nothing to do here, BScreen actually controls all of the resources
 */
Workspace::~Workspace(void) {}


void Workspace::addWindow(BlackboxWindow *w, Bool place) {
  assert(w != 0);

  if (place) placeWindow(w);

  w->setWorkspace(id);
  w->setWindowNumber(windowList.size());

  stackingList.push_front(w);
  windowList.push_back(w);

  clientmenu->insert(w->getTitle());
  clientmenu->update();

  screen->updateNetizenWindowAdd(w->getClientWindow(), id);

  raiseWindow(w);
}


unsigned int Workspace::removeWindow(BlackboxWindow *w) {
  assert(w != 0);

  stackingList.remove(w);

  if (w->isFocused()) {
    BlackboxWindow *newfocus = 0;
    if (w->isTransient()) newfocus = w->getTransientFor();
    if (! newfocus && ! stackingList.empty()) newfocus = stackingList.front();
    if (! newfocus || ! newfocus->setInputFocus()) {
      screen->getBlackbox()->setFocusedWindow(0);
    }
  }

  if (lastfocus == w)
    lastfocus = (BlackboxWindow *) 0;

  windowList.remove(w);
  clientmenu->remove(w->getWindowNumber());
  clientmenu->update();

  screen->updateNetizenWindowDel(w->getClientWindow());

  BlackboxWindowList::iterator it = windowList.begin();
  const BlackboxWindowList::iterator end = windowList.end();
  unsigned int i = 0;
  for (; it != end; ++it, ++i)
    (*it)->setWindowNumber(i);

  if (i == 0)
    cascade_x = cascade_y = 32;

  return i;
}


void Workspace::showAll(void) {
  std::for_each(stackingList.begin(), stackingList.end(),
                std::mem_fun(&BlackboxWindow::show));
}


void Workspace::hideAll(void) {
  // XXX: why is the order they are withdrawn important?

  /* make a temporary list in reverse order */
  BlackboxWindowList lst(stackingList.rbegin(), stackingList.rend());

  BlackboxWindowList::iterator it = lst.begin();
  const BlackboxWindowList::iterator end = lst.end();
  for (; it != end; ++it) {
    BlackboxWindow *bw = *it;
    if (! bw->isStuck())
      bw->withdraw();
  }
}


void Workspace::removeAll(void) {
  while (! windowList.empty())
    windowList.front()->iconify();
}


void Workspace::raiseWindow(BlackboxWindow *w) {
  BlackboxWindow *win = (BlackboxWindow *) 0, *bottom = w;

  while (bottom->isTransient()) {
    BlackboxWindow *bw = bottom->getTransientFor();
    if (! bw) break;
    bottom = bw;
  }

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
      wkspc->stackingList.remove(win);
      wkspc->stackingList.push_front(win);
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

  while (bottom->isTransient()) {
    BlackboxWindow *bw = bottom->getTransientFor();
    if (! bw) break;
    bottom = bw;
  }

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
      wkspc->stackingList.remove(win);
      wkspc->stackingList.push_back(win);
    }

    win = win->getTransientFor();
    if (! win)
      break;
  }

  XLowerWindow(screen->getBaseDisplay()->getXDisplay(), *nstack);
  XRestackWindows(screen->getBaseDisplay()->getXDisplay(), nstack, i);

  delete [] nstack;
}


void Workspace::reconfigure(void) {
  clientmenu->reconfigure();

  BlackboxWindowList::iterator it = windowList.begin();
  const BlackboxWindowList::iterator end = windowList.end();
  for (; it != end; ++it) {
    BlackboxWindow *bw = *it;
    if (bw->validateClient())
      bw->reconfigure();
  }
}


BlackboxWindow *Workspace::getWindow(unsigned int index) {
  if (index < windowList.size()) {
    BlackboxWindowList::iterator it = windowList.begin();
    for(; index > 0; --index, ++it); /* increment to index */
    return *it;
  }
  return 0;
}


BlackboxWindow*
Workspace::getNextWindowInList(BlackboxWindow *w) {
  BlackboxWindowList::iterator it = std::find(windowList.begin(),
                                              windowList.end(),
                                              w);
  assert(it != windowList.end());   // window must be in list
  ++it;                             // next window
  if (it == windowList.end())
    return windowList.front();      // if we walked off the end, wrap around

  return *it;
}


BlackboxWindow*
Workspace::getPrevWindowInList(BlackboxWindow *w) {
  BlackboxWindowList::iterator it = std::find(windowList.begin(),
                                              windowList.end(),
                                              w);
  assert(it != windowList.end()); // window must be in list
  if (it == windowList.begin())
    return windowList.back();     // if we walked of the front, wrap around

  return *(--it);
}


BlackboxWindow* Workspace::getTopWindowOnStack(void) const {
  return stackingList.front();
}


void Workspace::sendWindowList(Netizen &n) {
  BlackboxWindowList::iterator it = windowList.begin(),
    end = windowList.end();
  for(; it != end; ++it)
    n.sendWindowAdd((*it)->getClientWindow(), getID());
}


unsigned int Workspace::getCount(void) const {
  return windowList.size();
}


Bool Workspace::isCurrent(void) const {
  return (id == screen->getCurrentWorkspaceID());
}


Bool Workspace::isLastWindow(const BlackboxWindow* const w) const {
  return (w == windowList.back());
}

void Workspace::setCurrent(void) {
  screen->changeWorkspaceID(id);
}


void Workspace::setName(const string& new_name) {
  if (! new_name.empty()) {
    name = new_name;
  } else {
    string tmp =i18n(WorkspaceSet, WorkspaceDefaultNameFormat, "Workspace %d");
    assert(tmp.length() < 32);
    char default_name[32];
    sprintf(default_name, tmp.c_str(), id + 1);
    name = default_name;
  }

  clientmenu->setLabel(name);
  clientmenu->update();
}


void Workspace::placeWindow(BlackboxWindow *win) {
  /* we need to compensate for being shifted by x,y,
   * otherwise windows will not be allowed to reach the screen edge
   * it is easier to deal here than adjust everywhere else
   */
  XRectangle availableArea = screen->availableArea();
  availableArea.width += availableArea.x;
  availableArea.height += availableArea.y;

  const int win_w = win->getWidth() + (screen->getBorderWidth() * 4),
    win_h = win->getHeight() + (screen->getBorderWidth() * 4),
    change_y =
      ((screen->getColPlacementDirection() == BScreen::TopBottom) ? 1 : -1),
    change_x =
      ((screen->getRowPlacementDirection() == BScreen::LeftRight) ? 1 : -1),
    delta_x = 8, delta_y = 8;

  Bool placed = False;
  int place_x = availableArea.x, place_y = availableArea.y;

  switch (screen->getPlacementPolicy()) {
  case BScreen::RowSmartPlacement: {
    place_y = (screen->getColPlacementDirection() == BScreen::TopBottom) ?
      availableArea.y : availableArea.height - win_h;

    while (!placed &&
           ((screen->getColPlacementDirection() == BScreen::BottomTop) ?
            place_y > 0 : place_y + win_h < (signed) availableArea.height)) {
      place_x = (screen->getRowPlacementDirection() == BScreen::LeftRight) ?
        availableArea.x : availableArea.width - win_w;

      while (!placed &&
             ((screen->getRowPlacementDirection() == BScreen::RightLeft) ?
              place_x > 0 : place_x + win_w < (signed) availableArea.width)) {
        placed = True;

        const int win_right = place_x + win_w,
          win_bottom = place_y + win_h;

        BlackboxWindowList::iterator it = windowList.begin();
        const BlackboxWindowList::iterator end = windowList.end();
        for (; placed && it != end; ++it) {
          const BlackboxWindow* const curr = *it;
          const int curr_w = curr->getWidth() + (screen->getBorderWidth() * 4);
          const int curr_h =
            ((curr->isShaded()) ?
             curr->getTitleHeight() :
             curr->getHeight()) + (screen->getBorderWidth() * 4);

          if (curr->getXFrame() < win_right &&
              curr->getXFrame() + curr_w > place_x &&
              curr->getYFrame() < win_bottom &&
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
      availableArea.x : availableArea.width - win_w;
    while (!placed &&
           ((screen->getRowPlacementDirection() == BScreen::RightLeft) ?
            place_x > 0 : place_x + win_w < (signed) availableArea.width)) {
      place_y = (screen->getColPlacementDirection() == BScreen::TopBottom) ?
        availableArea.y : availableArea.height - win_h;

      while (!placed &&
             ((screen->getColPlacementDirection() == BScreen::BottomTop) ?
              place_y > 0 : place_y + win_h < (signed) availableArea.height)){
        placed = True;

        const int win_right = place_x + win_w,
          win_bottom = place_y + win_h;

        BlackboxWindowList::iterator it = windowList.begin();
        const BlackboxWindowList::iterator end = windowList.end();
        for (; placed && it != end; ++it) {
          const BlackboxWindow* const curr = *it;
          const int curr_w = curr->getWidth() + (screen->getBorderWidth() * 4);
          const int curr_h =
            ((curr->isShaded()) ?
             curr->getTitleHeight() :
             curr->getHeight()) + (screen->getBorderWidth() * 4);

          if (curr->getXFrame() < win_right &&
              curr->getXFrame() + curr_w > place_x &&
              curr->getYFrame() < win_bottom &&
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

    if (cascade_x == 32) {
      cascade_x += availableArea.x;
      cascade_y += availableArea.y;
    }
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
