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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <stdio.h>

#ifdef HAVE_STRING_H
#  include <string.h>
#endif // HAVE_STRING_H
}

#include <assert.h>

#include <functional>
#include <string>

#include "i18n.hh"
#include "blackbox.hh"
#include "Clientmenu.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Util.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Windowmenu.hh"


StackingList::StackingList(void) {
  desktop = stack.insert(stack.begin(), (BlackboxWindow*) 0);
  below = stack.insert(desktop, (BlackboxWindow*) 0);
  normal = stack.insert(below, (BlackboxWindow*) 0);
  above = stack.insert(normal, (BlackboxWindow*) 0);
  fullscreen = stack.insert(above, (BlackboxWindow*) 0);
}


StackingList::iterator&
StackingList::findLocation(const BlackboxWindow* const w) {
  if (w->getLayer() == BlackboxWindow::LAYER_NORMAL)
    return normal;
  else if (w->getLayer() == BlackboxWindow::LAYER_ABOVE)
    return above;
  else if (w->getLayer() == BlackboxWindow::LAYER_FULLSCREEN)
    return fullscreen;
  else if (w->getLayer() == BlackboxWindow::LAYER_BELOW)
    return below;
  else if (w->getLayer() == BlackboxWindow::LAYER_DESKTOP)
    return desktop;

  return normal;
}


void StackingList::insert(BlackboxWindow* w) {
  assert(w);

  StackingList::iterator& it = findLocation(w);
  it = stack.insert(it, w);
}


void StackingList::remove(BlackboxWindow* w) {
  assert(w);

  iterator& pos = findLocation(w);
  iterator it = std::find(pos, stack.end(), w);
  assert(it != stack.end());
  if (it == pos)
    ++pos;

  stack.erase(it);
  assert(stack.size() >= 5);
}


BlackboxWindow* StackingList::front(void) const {
  if (*fullscreen) return *fullscreen;
  if (*above) return *above;
  if (*normal) return *normal;
  if (*below) return *below;
  // we do not return desktop windows
  assert(0);
}


Workspace::Workspace(BScreen *scrn, unsigned int i) {
  screen = scrn;

  cascade_x = cascade_y = 32;

  id = i;

  clientmenu = new Clientmenu(this);

  lastfocus = (BlackboxWindow *) 0;

  setName(screen->getNameOfWorkspace(id));
}


void Workspace::addWindow(BlackboxWindow *w, bool place) {
  assert(w != 0);

  if (place) placeWindow(w);

  w->setWorkspace(id);
  w->setWindowNumber(windowList.size());

  stackingList.insert(w);
  windowList.push_back(w);

  std::string title = w->getTitle();
  unsigned int len = title.length();
  if (len > 60) {
    std::string::iterator lside = title.begin() + 29;
    unsigned int delta = len - 60;
    delta = (delta > 28) ? 28 : delta;
    std::string::iterator rside = title.end() - delta;
    title.replace(lside, rside, "...");
  }

  clientmenu->insert(title);
  clientmenu->update();

  raiseWindow(w);
}


unsigned int Workspace::removeWindow(BlackboxWindow *w) {
  assert(w != 0);

  stackingList.remove(w);

  // pass focus to the next appropriate window
  if ((w->isFocused() || w == lastfocus) &&
      ! screen->getBlackbox()->doShutdown()) {
    focusFallback(w);
  }

  screen->updateClientListStackingHint();

  windowList.remove(w);
  clientmenu->remove(w->getWindowNumber());
  clientmenu->update();

  BlackboxWindowList::iterator it = windowList.begin();
  const BlackboxWindowList::iterator end = windowList.end();
  unsigned int i = 0;
  for (; it != end; ++it, ++i)
    (*it)->setWindowNumber(i);

  if (i == 0)
    cascade_x = cascade_y = 32;

  return i;
}


void Workspace::focusFallback(const BlackboxWindow *old_window) {
  BlackboxWindow *newfocus = 0;

  if (id == screen->getCurrentWorkspaceID()) {
    // The window is on the visible workspace.

    // if it's a transient, then try to focus its parent
    if (old_window && old_window->isTransient()) {
      newfocus = old_window->getTransientFor();

      if (! newfocus ||
          newfocus->isIconic() ||                  // do not focus icons
          newfocus->getWorkspaceNumber() != id ||  // or other workspaces
          ! newfocus->setInputFocus())
        newfocus = 0;
    }

    if (! newfocus) {
      BlackboxWindowList::iterator it = stackingList.begin(),
                                  end = stackingList.end();
      for (; it != end; ++it) {
        BlackboxWindow *tmp = *it;
        if (tmp && tmp->setInputFocus()) {
          // we found our new focus target
          newfocus = tmp;
          break;
        }
      }
    }

    screen->getBlackbox()->setFocusedWindow(newfocus);
  } else {
    // The window is not on the visible workspace.

    if (old_window && lastfocus == old_window) {
      // The window was the last-focus target, so we need to replace it.
      BlackboxWindow *win = (BlackboxWindow*) 0;
      if (! stackingList.empty())
        win = stackingList.front();
      setLastFocusedWindow(win);
    }
  }
}


void Workspace::removeAll(void) {
  while (! windowList.empty())
    windowList.front()->iconify();
}


/*
 * returns the number of transients for win, plus the number of transients
 * associated with each transient of win
 */
static unsigned int countTransients(const BlackboxWindow * const win) {
  BlackboxWindowList transients = win->getTransients();
  if (transients.empty()) return 0;

  unsigned int ret = transients.size();
  BlackboxWindowList::const_iterator it = transients.begin(),
    end = transients.end();
  for (; it != end; ++it)
    ret += countTransients(*it);

  return ret;
}


/*
 * puts the transients of win into the stack. windows are stacked above
 * the window before it in the stackvector being iterated, meaning
 * stack[0] is on bottom, stack[1] is above stack[0], stack[2] is above
 * stack[1], etc...
 */
void Workspace::raiseTransients(const BlackboxWindow * const win,
                                WindowStack::iterator &stack) {
  if (win->getTransients().empty()) return; // nothing to do

  // put win's transients in the stack
  BlackboxWindowList::const_iterator it, end = win->getTransients().end();
  for (it = win->getTransients().begin(); it != end; ++it) {
    BlackboxWindow *w = *it;
    *(stack++) = w->getFrameWindow();

    if (! w->isIconic()) {
      Workspace *wkspc = screen->getWorkspace(w->getWorkspaceNumber());
      wkspc->stackingList.remove(w);
      wkspc->stackingList.insert(w);
    }
  }

  // put transients of win's transients in the stack
  for (it = win->getTransients().begin(); it != end; ++it)
    raiseTransients(*it, stack);
}


void Workspace::lowerTransients(const BlackboxWindow * const win,
                                WindowStack::iterator &stack) {
  if (win->getTransients().empty()) return; // nothing to do

  // put transients of win's transients in the stack
  BlackboxWindowList::const_reverse_iterator it,
    end = win->getTransients().rend();
  for (it = win->getTransients().rbegin(); it != end; ++it)
    lowerTransients(*it, stack);

  // put win's transients in the stack
  for (it = win->getTransients().rbegin(); it != end; ++it) {
    BlackboxWindow *w = *it;
    *stack++ = w->getFrameWindow();

    if (! w->isIconic()) {
      Workspace *wkspc = screen->getWorkspace(w->getWorkspaceNumber());
      wkspc->stackingList.remove(w);
      wkspc->stackingList.insert(w);
    }
  }
}


void Workspace::raiseWindow(BlackboxWindow *w) {
  BlackboxWindow *win = w;

  // walk up the transient_for's to the window that is not a transient
  while (win->isTransient() && win->getTransientFor())
    win = win->getTransientFor();

  // get the total window count (win and all transients)
  unsigned int i = 1 + countTransients(win);

  // stack the window with all transients above
  WindowStack stack_vector(i);
  WindowStack::iterator stack = stack_vector.begin();

  *(stack++) = win->getFrameWindow();
  if (! win->isIconic()) {
    Workspace *wkspc = screen->getWorkspace(win->getWorkspaceNumber());
    wkspc->stackingList.remove(win);
    wkspc->stackingList.insert(win);
  }

  raiseTransients(win, stack);

  screen->raiseWindows(&stack_vector);
}


void Workspace::lowerWindow(BlackboxWindow *w) {
  BlackboxWindow *win = w;

  // walk up the transient_for's to the window that is not a transient
  while (win->isTransient() && win->getTransientFor())
    win = win->getTransientFor();

  // get the total window count (win and all transients)
  unsigned int i = 1 + countTransients(win);

  // stack the window with all transients above
  WindowStack stack_vector(i);
  WindowStack::iterator stack = stack_vector.begin();

  lowerTransients(win, stack);

  *(stack++) = win->getFrameWindow();
  if (! win->isIconic()) {
    Workspace *wkspc = screen->getWorkspace(win->getWorkspaceNumber());
    wkspc->stackingList.remove(win);
    wkspc->stackingList.insert(win);
  }

  XLowerWindow(screen->getBaseDisplay()->getXDisplay(), stack_vector.front());
  XRestackWindows(screen->getBaseDisplay()->getXDisplay(),
                  &stack_vector[0], stack_vector.size());
}


void Workspace::reconfigure(void) {
  clientmenu->reconfigure();
  std::for_each(windowList.begin(), windowList.end(),
                std::mem_fun(&BlackboxWindow::reconfigure));
}


BlackboxWindow *Workspace::getWindow(unsigned int index) {
  if (index < windowList.size()) {
    BlackboxWindowList::iterator it = windowList.begin();
    while (index-- > 0)
      it = bt::next_it(it);
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


BlackboxWindow* Workspace::getPrevWindowInList(BlackboxWindow *w) {
  BlackboxWindowList::iterator it = std::find(windowList.begin(),
                                              windowList.end(),
                                              w);
  assert(it != windowList.end()); // window must be in list
  if (it == windowList.begin())
    return windowList.back();     // if we walked of the front, wrap around

  return *(--it);
}


BlackboxWindow* Workspace::getTopWindowOnStack(void) const {
  assert(! stackingList.empty());
  return stackingList.front();
}


unsigned int Workspace::getCount(void) const {
  return windowList.size();
}


void Workspace::hide(void) {
  BlackboxWindow *focused = screen->getBlackbox()->getFocusedWindow();
  if (focused && focused->getScreen() == screen) {
    assert(focused->getWorkspaceNumber() == id);

    lastfocus = focused;
  } else {
    // if no window had focus, no need to store a last focus
    lastfocus = (BlackboxWindow *) 0;
  }

  // when we switch workspaces, unfocus whatever was focused
  screen->getBlackbox()->setFocusedWindow((BlackboxWindow *) 0);

  // withdraw windows in reverse order to minimize the number of Expose events

  StackingList::reverse_iterator it = stackingList.rbegin(),
    end = stackingList.rend();
  for (; it != end; ++it) {
    BlackboxWindow *bw = *it;
    if (bw)
      bw->withdraw();
  }
}


void Workspace::show(void) {
  StackingList::iterator it = stackingList.begin(),
    end = stackingList.end();
  for (; it != end; ++it) {
    if (*it)
      (*it)->show();
  }

  XSync(screen->getBlackbox()->getXDisplay(), False);

  if (screen->doFocusLast()) {
    if (! screen->isSloppyFocus() && ! lastfocus && ! stackingList.empty())
      lastfocus = stackingList.front();

    if (lastfocus)
      lastfocus->setInputFocus();
  }
}


bool Workspace::isCurrent(void) const {
  return (id == screen->getCurrentWorkspaceID());
}


bool Workspace::isLastWindow(const BlackboxWindow* const w) const {
  return (w == windowList.back());
}


void Workspace::setCurrent(void) {
  screen->changeWorkspaceID(id);
}


void Workspace::setName(const std::string& new_name) {
  if (! new_name.empty()) {
    name = new_name;
  } else {
    std::string tmp =
      bt::i18n(WorkspaceSet, WorkspaceDefaultNameFormat, "Workspace %d");
    assert(tmp.length() < 32);
    char default_name[32];
    sprintf(default_name, tmp.c_str(), id + 1);
    name = default_name;
  }

  clientmenu->setLabel(name);
  clientmenu->update();
}


/*
 * Calculate free space available for window placement.
 */
typedef std::vector<bt::Rect> rectList;

static rectList calcSpace(const bt::Rect &win, const rectList &spaces) {
  bt::Rect isect, extra;
  rectList result;
  rectList::const_iterator siter, end = spaces.end();
  for (siter = spaces.begin(); siter != end; ++siter) {
    const bt::Rect &curr = *siter;

    if(! win.intersects(curr)) {
      result.push_back(curr);
      continue;
    }

    /* Use an intersection of win and curr to determine the space around
     * curr that we can use.
     *
     * NOTE: the spaces calculated can overlap.
     */
    isect = curr & win;

    // left
    extra.setCoords(curr.left(), curr.top(),
                    isect.left() - 1, curr.bottom());
    if (extra.valid()) result.push_back(extra);

    // top
    extra.setCoords(curr.left(), curr.top(),
                    curr.right(), isect.top() - 1);
    if (extra.valid()) result.push_back(extra);

    // right
    extra.setCoords(isect.right() + 1, curr.top(),
                    curr.right(), curr.bottom());
    if (extra.valid()) result.push_back(extra);

    // bottom
    extra.setCoords(curr.left(), isect.bottom() + 1,
                    curr.right(), curr.bottom());
    if (extra.valid()) result.push_back(extra);
  }
  return result;
}


static bool rowRLBT(const bt::Rect &first, const bt::Rect &second) {
  if (first.bottom() == second.bottom())
    return first.right() > second.right();
  return first.bottom() > second.bottom();
}

static bool rowRLTB(const bt::Rect &first, const bt::Rect &second) {
  if (first.y() == second.y())
    return first.right() > second.right();
  return first.y() < second.y();
}

static bool rowLRBT(const bt::Rect &first, const bt::Rect &second) {
  if (first.bottom() == second.bottom())
    return first.x() < second.x();
  return first.bottom() > second.bottom();
}

static bool rowLRTB(const bt::Rect &first, const bt::Rect &second) {
  if (first.y() == second.y())
    return first.x() < second.x();
  return first.y() < second.y();
}

static bool colLRTB(const bt::Rect &first, const bt::Rect &second) {
  if (first.x() == second.x())
    return first.y() < second.y();
  return first.x() < second.x();
}

static bool colLRBT(const bt::Rect &first, const bt::Rect &second) {
  if (first.x() == second.x())
    return first.bottom() > second.bottom();
  return first.x() < second.x();
}

static bool colRLTB(const bt::Rect &first, const bt::Rect &second) {
  if (first.right() == second.right())
    return first.y() < second.y();
  return first.right() > second.right();
}

static bool colRLBT(const bt::Rect &first, const bt::Rect &second) {
  if (first.right() == second.right())
    return first.bottom() > second.bottom();
  return first.right() > second.right();
}


bool Workspace::smartPlacement(bt::Rect& win, const bt::Rect& availableArea) {
  rectList spaces;
  spaces.push_back(availableArea); //initially the entire screen is free

  //Find Free Spaces
  BlackboxWindowList::const_iterator wit = windowList.begin(),
    end = windowList.end();
  bt::Rect tmp;
  for (; wit != end; ++wit) {
    const BlackboxWindow* const curr = *wit;
    if (curr->isShaded()) continue;

    tmp.setRect(curr->frameRect().x(), curr->frameRect().y(),
                curr->frameRect().width() + screen->getBorderWidth(),
                curr->frameRect().height() + screen->getBorderWidth());

    spaces = calcSpace(tmp, spaces);
  }

  if (screen->getPlacementPolicy() == BScreen::RowSmartPlacement) {
    if(screen->getRowPlacementDirection() == BScreen::LeftRight) {
      if(screen->getColPlacementDirection() == BScreen::TopBottom)
        std::sort(spaces.begin(), spaces.end(), rowLRTB);
      else
        std::sort(spaces.begin(), spaces.end(), rowLRBT);
    } else {
      if(screen->getColPlacementDirection() == BScreen::TopBottom)
        std::sort(spaces.begin(), spaces.end(), rowRLTB);
      else
        std::sort(spaces.begin(), spaces.end(), rowRLBT);
    }
  } else {
    if(screen->getColPlacementDirection() == BScreen::TopBottom) {
      if(screen->getRowPlacementDirection() == BScreen::LeftRight)
        std::sort(spaces.begin(), spaces.end(), colLRTB);
      else
        std::sort(spaces.begin(), spaces.end(), colRLTB);
    } else {
      if(screen->getRowPlacementDirection() == BScreen::LeftRight)
        std::sort(spaces.begin(), spaces.end(), colLRBT);
      else
        std::sort(spaces.begin(), spaces.end(), colRLBT);
    }
  }

  rectList::const_iterator sit = spaces.begin(), spaces_end = spaces.end();
  for(; sit != spaces_end; ++sit) {
    if (sit->width() >= win.width() && sit->height() >= win.height())
      break;
  }

  if (sit == spaces_end)
    return False;

  //set new position based on the empty space found
  const bt::Rect& where = *sit;
  win.setX(where.x());
  win.setY(where.y());

  // adjust the location() based on left/right and top/bottom placement
  if (screen->getPlacementPolicy() == BScreen::RowSmartPlacement) {
    if (screen->getRowPlacementDirection() == BScreen::RightLeft)
      win.setX(where.right() - win.width());
    if (screen->getColPlacementDirection() == BScreen::BottomTop)
      win.setY(where.bottom() - win.height());
  } else {
    if (screen->getColPlacementDirection() == BScreen::BottomTop)
      win.setY(win.y() + where.height() - win.height());
    if (screen->getRowPlacementDirection() == BScreen::RightLeft)
      win.setX(win.x() + where.width() - win.width());
  }
  return True;
}


bool Workspace::cascadePlacement(bt::Rect &win,
                                 const bt::Rect &availableArea) {
  if (cascade_x > (availableArea.width() / 2) ||
      cascade_y > (availableArea.height() / 2))
    cascade_x = cascade_y = 32;

  if (cascade_x == 32) {
    cascade_x += availableArea.x();
    cascade_y += availableArea.y();
  }

  win.setPos(cascade_x, cascade_y);

  return True;
}


void Workspace::placeWindow(BlackboxWindow *win) {
  bt::Rect availableArea(screen->availableArea()),
    new_win(availableArea.x(), availableArea.y(),
            win->frameRect().width(), win->frameRect().height());
  bool placed = False;

  switch (screen->getPlacementPolicy()) {
  case BScreen::RowSmartPlacement:
  case BScreen::ColSmartPlacement:
    placed = smartPlacement(new_win, availableArea);
    break;
  default:
    break; // handled below
  } // switch

  if (placed == False) {
    cascadePlacement(new_win, availableArea);
    cascade_x += win->getTitleHeight() + (screen->getBorderWidth() * 2);
    cascade_y += win->getTitleHeight() + (screen->getBorderWidth() * 2);
  }

  if (new_win.right() > availableArea.right())
    new_win.setX(availableArea.left());
  if (new_win.bottom() > availableArea.bottom())
    new_win.setY(availableArea.top());
  win->configure(new_win.x(), new_win.y(), new_win.width(), new_win.height());
}


void Workspace::updateClientListStacking(Netwm::WindowList& clientList) const {
  StackingList::const_iterator it = stackingList.begin(),
    end = stackingList.end();
  for (; it != end; ++it) {
    if (*it)
      clientList.push_back((*it)->getClientWindow());
  }
}
