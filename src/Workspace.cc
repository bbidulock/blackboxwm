// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Workspace.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2003
//         Bradley T Hughes <bhughes at trolltech.com>
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

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include "i18n.hh"
#include "blackbox.hh"
#include "Clientmenu.hh"
#include "Font.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Util.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Windowmenu.hh"


void StackingList::dump(void) const {
#if 0
  StackingList::const_iterator it = stack.begin(), end = stack.end();
  BlackboxWindow *w;
  fprintf(stderr, "Stack:\n");
  for (; it != end; ++it) {
    w = *it;
    if (w)
      fprintf(stderr, "%s: 0x%lx\n", w->getTitle(), w->getClientWindow());
    else
      fprintf(stderr, "zero\n");
  }
  fprintf(stderr, "the layers:\n");
  w = *fullscreen;
  if (w)
    fprintf(stderr, "%s: 0x%lx\n", w->getTitle(), w->getClientWindow());
  else
    fprintf(stderr, "zero\n");
  w = *above;
  if (w)
    fprintf(stderr, "%s: 0x%lx\n", w->getTitle(), w->getClientWindow());
  else
    fprintf(stderr, "zero\n");
  w = *normal;
  if (w)
    fprintf(stderr, "%s: 0x%lx\n", w->getTitle(), w->getClientWindow());
  else
    fprintf(stderr, "zero\n");
  w = *below;
  if (w)
    fprintf(stderr, "%s: 0x%lx\n", w->getTitle(), w->getClientWindow());
  else
    fprintf(stderr, "zero\n");
  w = *desktop;
  if (w)
    fprintf(stderr, "%s: 0x%lx\n", w->getTitle(), w->getClientWindow());
  else
    fprintf(stderr, "zero\n");
#endif
}


StackingList::StackingList(void) {
  desktop = stack.insert(stack.begin(), (BlackboxWindow*) 0);
  below = stack.insert(desktop, (BlackboxWindow*) 0);
  normal = stack.insert(below, (BlackboxWindow*) 0);
  above = stack.insert(normal, (BlackboxWindow*) 0);
  fullscreen = stack.insert(above, (BlackboxWindow*) 0);
}


StackingList::iterator&
StackingList::findLayer(const BlackboxWindow* const w) {
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

  assert(0);
  return normal;
}


void StackingList::insert(BlackboxWindow* w) {
  assert(w);

  StackingList::iterator& it = findLayer(w);
  it = stack.insert(it, w);
}


void StackingList::append(BlackboxWindow* w) {
  assert(w);

  StackingList::iterator& it = findLayer(w);
  if (! *it) { // empty layer
    it = stack.insert(it, w);
    return;
  }

  // find the end of the layer (the zero pointer)
  const BlackboxWindow * const zero = 0;
  StackingList::iterator tmp = std::find(it, stack.end(), zero);
  assert(tmp != stack.end());
  stack.insert(tmp, w);
}


void StackingList::remove(BlackboxWindow* w) {
  assert(w);

  iterator& pos = findLayer(w);
  iterator it = std::find(pos, stack.end(), w);
  assert(it != stack.end());
  if (it == pos)
    ++pos;

  stack.erase(it);
  assert(stack.size() >= 5);
}


BlackboxWindow* StackingList::front(void) const {
  assert(stack.size() > 5);

  if (*fullscreen) return *fullscreen;
  if (*above) return *above;
  if (*normal) return *normal;
  if (*below) return *below;
  // we do not return desktop windows
  assert(0);

  // this point is never reached, but the compiler doesn't know that.
  return (BlackboxWindow*) 0;
}


BlackboxWindow* StackingList::back(void) const {
  assert(stack.size() > 5);

  WindowStack::const_iterator it = desktop, _end = stack.begin();
  for (--it; it != _end; --it) {
    if (*it)
      return *it;
  }
  assert(0);

  return (BlackboxWindow*) 0;
}


Workspace::Workspace(BScreen *scrn, unsigned int i) {
  _screen = scrn;

  cascade_x = cascade_y = 32;

  _id = i;

  clientmenu = new Clientmenu(*_screen->getBlackbox(), *_screen, _id);

  lastfocus = (BlackboxWindow *) 0;

  setName(_screen->resource().nameOfWorkspace(i));
}


void Workspace::addWindow(BlackboxWindow *w, bool place) {
  assert(! (w == 0));

  if (place) placeWindow(w);

  int wid = clientmenu->insertItem(bt::ellideText(w->getTitle(), 60, "..."));
  w->setWorkspace(_id);
  w->setWindowNumber(wid);

  stackingList.insert(w);

  raiseWindow(w);
}


void Workspace::removeWindow(BlackboxWindow *w) {
  assert(w != 0);

  stackingList.remove(w);

  // pass focus to the next appropriate window
  if ((w->isFocused() || w == lastfocus) &&
      _screen->getBlackbox()->running() ) {
    focusFallback(w);
  }

  _screen->updateClientListStackingHint();

  clientmenu->removeItem(w->getWindowNumber());

  if (stackingList.empty())
    cascade_x = cascade_y = 32;
}


void Workspace::focusFallback(const BlackboxWindow *old_window) {
  BlackboxWindow *newfocus = 0;

  if (_id == _screen->getCurrentWorkspaceID()) {
    // The window is on the visible workspace.

    // if it's a transient, then try to focus its parent
    if (old_window && old_window->isTransient()) {
      newfocus = old_window->getTransientFor();

      if (! newfocus ||
          newfocus->isIconic() ||                   // do not focus icons
          newfocus->getWorkspaceNumber() != _id ||  // or other workspaces
          ! newfocus->setInputFocus())
        newfocus = 0;
    }

    if (! newfocus) {
      StackingList::iterator it = stackingList.begin(),
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

    _screen->getBlackbox()->setFocusedWindow(newfocus);
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


void Workspace::transferWindows(Workspace& wkspc) {
  while (! stackingList.empty()) {
    BlackboxWindow* w = stackingList.back();
    wkspc.addWindow(w);
    stackingList.remove(w);
  }
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
                                WindowStack& stack) {
  if (win->getTransients().empty()) return; // nothing to do

  // put transients of win's transients in the stack
  BlackboxWindowList::const_reverse_iterator it,
    end = win->getTransients().rend();
  for (it = win->getTransients().rbegin(); it != end; ++it)
    raiseTransients(*it, stack);

  // put win's transients in the stack
  for (it = win->getTransients().rbegin(); it != end; ++it) {
    BlackboxWindow *w = *it;
    if (! w->isIconic() && w->getWorkspaceNumber() == _id) {
      stack.push_back(w->getFrameWindow());
      stackingList.remove(w);
      stackingList.insert(w);
    }
  }
}


void Workspace::lowerTransients(const BlackboxWindow * const win,
                                WindowStack& stack) {
  if (win->getTransients().empty()) return; // nothing to do

  // put transients of win's transients in the stack
  BlackboxWindowList::const_reverse_iterator it,
    end = win->getTransients().rend();
  for (it = win->getTransients().rbegin(); it != end; ++it)
    lowerTransients(*it, stack);

  // put win's transients in the stack
  for (it = win->getTransients().rbegin(); it != end; ++it) {
    BlackboxWindow *w = *it;
    if (! w->isIconic() && w->getWorkspaceNumber() == _id) {
      stack.push_back(w->getFrameWindow());
      stackingList.remove(w);
      stackingList.append(w);
    }
  }
}


void Workspace::raiseWindow(BlackboxWindow *w) {
  BlackboxWindow *win = w;

  // walk up the transient_for's to the window that is not a transient
  while (win->isTransient() && win->getTransientFor())
    win = win->getTransientFor();

  // stack the window with all transients above
  WindowStack stack_vector;
  bool layer_above = False;
  StackingList::iterator it = stackingList.findLayer(win);
  StackingList::iterator tmp = it;
  for (; tmp != stackingList.begin(); --tmp) {
    if (*tmp && it != tmp)
      break;
  }
  if (*tmp) {
    stack_vector.push_back((*tmp)->getFrameWindow());
    layer_above = True;
  }

  raiseTransients(win, stack_vector);

  if (win->getWorkspaceNumber() == _id) {
    stack_vector.push_back(win->getFrameWindow());
    stackingList.remove(win);
    stackingList.insert(win);
  }

  assert(! stack_vector.empty());
  if (! layer_above)
    XRaiseWindow(_screen->getBlackbox()->XDisplay(),
                 stack_vector.front());

  XRestackWindows(_screen->getBlackbox()->XDisplay(), &stack_vector[0],
                  stack_vector.size());
  _screen->raiseWindows(0);
}


void Workspace::lowerWindow(BlackboxWindow *w) {
  BlackboxWindow *win = w;

  // walk up the transient_for's to the window that is not a transient
  while (win->isTransient() && win->getTransientFor())
    win = win->getTransientFor();

  WindowStack stack_vector;
  bool layer_above = False;
  StackingList::iterator it = stackingList.findLayer(win);
  StackingList::iterator tmp = it;
  for (; tmp != stackingList.begin(); --tmp) {
    if (*tmp && it != tmp)
      break;
  }
  if (*tmp) {
    stack_vector.push_back((*tmp)->getFrameWindow());
    layer_above = True;
  }

  // stack the window with all transients above
  lowerTransients(win, stack_vector);

  if (! win->isIconic() && win->getWorkspaceNumber() == _id) {
    stack_vector.push_back(win->getFrameWindow());
    stackingList.remove(win);
    stackingList.append(win);
  }

  assert(! stack_vector.empty());

  if (! layer_above) {
    // ok, no layers above us, how about below?
    tmp = std::find(it, stackingList.end(), (BlackboxWindow*) 0);
    assert(tmp != stackingList.end());
    for (; tmp != stackingList.end(); ++tmp) {
      if (*tmp && it != tmp)
        break;
    }
    if (tmp != stackingList.end() && *tmp) {
      XWindowChanges changes;
      changes.sibling = (*tmp)->getFrameWindow();
      changes.stack_mode = Above;
      XConfigureWindow(_screen->getBlackbox()->XDisplay(),
                       stack_vector.front(), CWStackMode | CWSibling,
                       &changes);
    } else {
      XLowerWindow(_screen->getBlackbox()->XDisplay(),
                   stack_vector.front());
    }
  }

  XRestackWindows(_screen->getBlackbox()->XDisplay(), &stack_vector[0],
                  stack_vector.size());
  _screen->raiseWindows(0);
}


void Workspace::reconfigure(void) {
  clientmenu->reconfigure();
  StackingList::iterator it = stackingList.begin(), end = stackingList.end();
  for (; it != end; ++it)
    if (*it) (*it)->reconfigure();
}


BlackboxWindow *Workspace::window(unsigned int index) const {
  if (index < stackingList.size()) {
    StackingList::const_iterator it = stackingList.begin(),
                                end = stackingList.end();
    for (; it != end; ++it) {
      if (*it && (*it)->getWindowNumber() == index)
        return *it;
    }
  }

  return 0;
}


BlackboxWindow*
Workspace::getNextWindowInList(BlackboxWindow *w) {
  StackingList::iterator it = std::find(stackingList.begin(),
                                        stackingList.end(),
                                        w);
  assert(it != stackingList.end());   // window must be in list
  ++it;                               // next window
  if (it == stackingList.end())
    return stackingList.front();      // if we walked off the end, wrap around

  return *it;
}


BlackboxWindow* Workspace::getPrevWindowInList(BlackboxWindow *w) {
  StackingList::iterator it = std::find(stackingList.begin(),
                                        stackingList.end(),
                                        w);
  assert(it != stackingList.end()); // window must be in list
  if (it == stackingList.begin())
    return stackingList.back();     // if we walked of the front, wrap around

  return *(--it);
}


BlackboxWindow* Workspace::getTopWindowOnStack(void) const {
  assert(! stackingList.empty());
  return stackingList.front();
}


unsigned int Workspace::windowCount(void) const {
  return stackingList.size();
}


void Workspace::hide(void) {
  BlackboxWindow *focused = _screen->getBlackbox()->getFocusedWindow();
  if (focused && focused->getScreen() == _screen) {
    assert(focused->getWorkspaceNumber() == _id);

    lastfocus = focused;
  } else {
    // if no window had focus, no need to store a last focus
    lastfocus = (BlackboxWindow *) 0;
  }

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
  for (; it != end; ++it)
    if (*it) (*it)->show();

  XSync(_screen->getBlackbox()->XDisplay(), False);

  if (_screen->resource().doFocusLast()) {
    if (! (_screen->resource().isSloppyFocus() ||
           lastfocus || stackingList.empty()))
      lastfocus = stackingList.front();

    if (lastfocus)
      lastfocus->setInputFocus();
  }
}


bool Workspace::isCurrent(void) const {
  return (_id == _screen->getCurrentWorkspaceID());
}


void Workspace::setCurrent(void) {
  _screen->changeWorkspaceID(_id);
}


void Workspace::setName(const std::string& new_name) {
  std::string the_name;

  if (! new_name.empty()) {
    the_name = new_name;
  } else {
    const std::string tmp =
      bt::i18n(WorkspaceSet, WorkspaceDefaultNameFormat, "Workspace %d");
    assert(tmp.length() < 32);
    char default_name[32];
    sprintf(default_name, tmp.c_str(), _id + 1);
    the_name = default_name;
  }

  _screen->resource().saveWorkspaceName(_id, the_name);
  clientmenu->setTitle(the_name);
}


bool Workspace::smartPlacement(bt::Rect& win, const bt::Rect& availableArea) {
  // constants
  const bool row_placement =
    (_screen->resource().placementPolicy() == RowSmartPlacement);
  const bool leftright =
    (_screen->resource().rowPlacementDirection() == LeftRight);
  const bool topbottom =
    (_screen->resource().colPlacementDirection() == TopBottom);
  const bool ignore_shaded = _screen->resource().placementIgnoresShaded();

  const int border_width = _screen->resource().borderWidth();
  const int left_border   = leftright ? 0 : -border_width-1;
  const int top_border    = topbottom ? 0 : -border_width-1;
  const int right_border  = leftright ? border_width+1 : 0;
  const int bottom_border = topbottom ? border_width+1 : 0;

  StackingList::const_iterator w_it, w_end;

  /*
    build a sorted vector of x and y grid boundaries

    note: we use one vector to reduce the number of allocations
    std::vector must do... we allocate as much memory as we would need
    in the worst case scenario and work with that
  */
  std::vector<int> coords(stackingList.size() * 4 + 4);
  std::vector<int>::iterator
    x_begin = coords.begin(),
    x_end   = x_begin,
    y_begin = coords.begin() + stackingList.size() * 2 + 2,
    y_end   = y_begin;

  {
    std::vector<int>::iterator x_it = x_begin, y_it = y_begin;

    *x_it++ = availableArea.left();
    *x_it++ = availableArea.right();
    x_end += 2;

    *y_it++ = availableArea.top();
    *y_it++ = availableArea.bottom();
    y_end += 2;


    for (w_it  = stackingList.begin(), w_end = stackingList.end();
         w_it != w_end; ++w_it) {
      const BlackboxWindow* const w = *w_it;
      if (! w || (ignore_shaded && w->isShaded()))
        continue;

      *x_it++ = std::max(w->frameRect().left() + left_border,
                         availableArea.left());
      *x_it++ = std::min(w->frameRect().right() + right_border,
                         availableArea.right());
      x_end += 2;

      *y_it++ = std::max(w->frameRect().top() + top_border,
                         availableArea.top());
      *y_it++ = std::min(w->frameRect().bottom() + bottom_border,
                         availableArea.bottom());
      y_end += 2;
    }

    assert(x_end <= y_begin);
  }

  std::sort(x_begin, x_end);
  x_end = std::unique(x_begin, x_end);

  std::sort(y_begin, y_end);
  y_end = std::unique(y_begin, y_end);

  // build a distribution grid
  unsigned int gw = x_end - x_begin - 1,
               gh = y_end - y_begin - 1;
  std::bit_vector used_grid(gw * gh);
  std::fill_n(used_grid.begin(), used_grid.size(), false);

  for (w_it = stackingList.begin(), w_end = stackingList.end();
       w_it != w_end; ++w_it) {
    const BlackboxWindow* const w = *w_it;
    if (! w || (ignore_shaded && w->isShaded()))
      continue;

    const int w_left =
      std::max(w->frameRect().left() + left_border,
               availableArea.left());
    const int w_top =
      std::max(w->frameRect().top() + top_border,
               availableArea.top());
    const int w_right =
      std::min(w->frameRect().right() + right_border,
               availableArea.right());
    const int w_bottom =
      std::min(w->frameRect().bottom() + bottom_border,
               availableArea.bottom());

    // which areas of the grid are used by this window?
    std::vector<int>::iterator l_it = std::find(x_begin, x_end, w_left),
                               r_it = std::find(x_begin, x_end, w_right),
                               t_it = std::find(y_begin, y_end, w_top),
                               b_it = std::find(y_begin, y_end, w_bottom);
    assert(l_it != x_end &&
           r_it != x_end &&
           t_it != y_end &&
           b_it != y_end);

    const unsigned int left   = l_it - x_begin,
                       right  = r_it - x_begin,
                       top    = t_it - y_begin,
                       bottom = b_it - y_begin;

    for (unsigned int gy = top; gy < bottom; ++gy)
      for (unsigned int gx = left; gx < right; ++gx)
        used_grid[(gy * gw) + gx] = true;
  }

  /*
    Attempt to fit the window into any of the empty areas in the grid.
    The exact order is dependent upon the users configuration (as
    shown below).

    row placement:
    - outer -> vertical axis
    - inner -> horizontal axis

    col placement:
    - outer -> horizontal axis
    - inner -> vertical axis
  */

  int gx, gy;
  int &outer = row_placement ? gy : gx;
  int &inner = row_placement ? gx : gy;
  const int outer_delta =
    row_placement ? (topbottom ? 1 : -1) : (leftright ? 1 : -1);
  const int inner_delta =
    row_placement ? (leftright ? 1 : -1) : (topbottom ? 1 : -1);
  const int outer_begin =
    row_placement ? (topbottom ? 0 : gh) : (leftright ? 0 : gw);
  const int outer_end = row_placement ?
                        (topbottom ? static_cast<int>(gh) : -1) :
                        (leftright ? static_cast<int>(gw) : -1);
  const int inner_begin =
    row_placement ? (leftright ? 0 : gw) : (topbottom ? 0 : gh);
  const int inner_end = row_placement ?
                        (leftright ? static_cast<int>(gw) : -1) :
                        (topbottom ? static_cast<int>(gh) : -1);

  bt::Rect where;
  bool fit = false;
  for (outer = outer_begin; ! fit && outer != outer_end;
       outer += outer_delta) {
    for (inner = inner_begin; ! fit && inner != inner_end;
         inner += inner_delta) {
      // see if the window fits in a single unused area
      if (used_grid[(gy * gw) + gx]) continue;

      where.setCoords(*(x_begin + gx), *(y_begin + gy),
                      *(x_begin + gx + 1), *(y_begin + gy + 1));

      if (where.width()  >= win.width() &&
          where.height() >= win.height()) {
        fit = true;
        break;
      }

      /*
        see if we neighboring spaces are unused

        TODO: we should grid fit in the same direction as above,
        instead of always right->left and top->bottom
      */
      int gx2 = gx, gy2 = gy;

      if (win.width() > where.width()) {
        for (gx2 = gx+1; gx2 < static_cast<int>(gw); ++gx2) {
          if (used_grid[(gy * gw) + gx2]) {
            --gx2;
            break;
          }

          where.setCoords(*(x_begin + gx), *(y_begin + gy),
                          *(x_begin + gx2 + 1), *(y_begin + gy2 + 1));

          if (where.width() >= win.width()) break;
        }

        if (gx2 >= static_cast<int>(gw)) --gx2;
      }

      if (win.height() > where.height()) {
        for (gy2 = gy; gy2 < static_cast<int>(gh); ++gy2) {
          if (used_grid[(gy2 * gw) + gx]) {
            --gy2;
            break;
          }

          where.setCoords(*(x_begin + gx), *(y_begin + gy),
                          *(x_begin + gx2 + 1), *(y_begin + gy2 + 1));

          if (where.height() >= win.height()) break;
        }

        if (gy2 >= static_cast<int>(gh)) --gy2;
      }

      if (where.width()  >= win.width() &&
          where.height() >= win.height()) {
        fit = true;

        // make sure all spaces are really available
        for (int gy3 = gy; gy3 <= gy2; ++gy3) {
          for (int gx3 = gx; gx3 <= gx2; ++gx3) {
            if (used_grid[(gy3 * gw) + gx3]) {
              fit = false;
              break;
            }
          }
        }
      }
    }
  }

  if (! fit) {
    const int screen_area = availableArea.width() * availableArea.height();
    const int window_area = win.width() * win.height();
    if (window_area > screen_area / 2) {
      // center large windows instead of cascading
      win.setPos((availableArea.x() + availableArea.width() -
                  win.width()) / 2,
                 (availableArea.y() + availableArea.height() -
                  win.height()) / 2);
      return true;
    }
    return false;
  }

  // adjust the location() based on left/right and top/bottom placement
  if (! leftright) where.setX(where.right() - win.width() + 1);
  if (! topbottom) where.setY(where.bottom() - win.height() + 1);

  win.setX(where.x());
  win.setY(where.y());

  return true;
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

  if (win.right()  > availableArea.right() ||
      win.bottom() > availableArea.bottom()) {
    cascade_x = cascade_y = 32;

    cascade_x += availableArea.x();
    cascade_y += availableArea.y();

    win.setPos(cascade_x, cascade_y);
  }

  return True;
}


void Workspace::placeWindow(BlackboxWindow *win) {
  const bt::Rect &availableArea = _screen->availableArea();
  bt::Rect new_win(availableArea.x(), availableArea.y(),
                   win->frameRect().width(), win->frameRect().height());
  bool placed = False;

  switch (_screen->resource().placementPolicy()) {
  case RowSmartPlacement:
  case ColSmartPlacement:
    placed = smartPlacement(new_win, availableArea);
    break;
  default:
    break; // handled below
  } // switch

  if (placed == False) {
    cascadePlacement(new_win, availableArea);
    cascade_x += win->getTitleHeight() +
                 (_screen->resource().borderWidth() * 2);
    cascade_y += win->getTitleHeight() +
                 (_screen->resource().borderWidth() * 2);
  }

  if (new_win.right() > availableArea.right())
    new_win.setX(availableArea.left());
  if (new_win.bottom() > availableArea.bottom())
    new_win.setY(availableArea.top());

  win->configure(new_win.x(), new_win.y(), new_win.width(), new_win.height());
}


void
Workspace::updateClientListStacking(bt::Netwm::WindowList& clientList) const {
  StackingList::const_iterator it = stackingList.begin(),
    end = stackingList.end();
  for (; it != end; ++it)
    if (*it) clientList.push_back((*it)->getClientWindow());
}


const std::string& Workspace::name(void) const {
  return _screen->resource().nameOfWorkspace(_id);
}
