// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// StackingList.cc for Blackbox - an X11 Window manager
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

#include "StackingList.hh"
#include "Window.hh"

#include <cassert>
#include <cstdio>


static BlackboxWindow * const zero = 0;


StackingList::StackingList(void) {
  desktop = stack.insert(stack.begin(), zero);
  below = stack.insert(desktop, zero);
  normal = stack.insert(below, zero);
  above = stack.insert(normal, zero);
  fullscreen = stack.insert(above, zero);
}


StackingList::iterator&
StackingList::findLayer(const BlackboxWindow *const win) {
  switch (win->getLayer()) {
  case LAYER_DESKTOP:
    return desktop;
  case LAYER_BELOW:
    return below;
  case LAYER_NORMAL:
    return normal;
  case LAYER_ABOVE:
    return above;
  case LAYER_FULLSCREEN:
    return fullscreen;
  }

  assert(0);
  return normal;
}

StackingList::iterator StackingList::insert(BlackboxWindow *win) {
  assert(win);

  iterator& it = findLayer(win);
  it = stack.insert(it, win);
  return it;
}


StackingList::iterator StackingList::append(BlackboxWindow *win) {
  assert(win);

  iterator& it = findLayer(win);
  if (!*it) { // empty layer
    it = stack.insert(it, win);
    return it;
  }

  // find the end of the layer (the zero pointer)
  iterator tmp = std::find(it, stack.end(), zero);
  assert(tmp != stack.end());
  tmp = stack.insert(tmp, win);
  return tmp;
}


StackingList::iterator StackingList::remove(BlackboxWindow *win) {
  assert(win);

  iterator& pos = findLayer(win);
  iterator it = std::find(pos, stack.end(), win);
  assert(it != stack.end());
  if (it == pos)
    ++pos;

  it = stack.erase(it);
  assert(stack.size() >= 5);
  return it;
}


void StackingList::raise(BlackboxWindow *win) {
  assert(win);

  iterator& pos = findLayer(win);
  iterator it = std::find(pos, stack.end(), win);
  assert(it != stack.end());
  if (it == pos)
    ++pos;

  (void) stack.erase(it);
  pos = stack.insert(pos, win);
}


void StackingList::lower(BlackboxWindow *win) {
  assert(win);

  iterator& pos = findLayer(win);
  iterator it = std::find(pos, stack.end(), win);
  assert(it != stack.end());
  if (it == pos)
    ++pos;

  (void) stack.erase(it);

  if (!*pos) { // empty layer
    pos = stack.insert(pos, win);
    return;
  }

  it = std::find(pos, stack.end(), zero);
  assert(it != stack.end());
  (void) stack.insert(it, win);
}


BlackboxWindow *StackingList::front(void) const {
  assert(stack.size() > 5);

  if (*fullscreen) return *fullscreen;
  if (*above) return *above;
  if (*normal) return *normal;
  if (*below) return *below;
  // we do not return desktop entities
  assert(0);

  // this point is never reached, but the compiler doesn't know that.
  return zero;
}


BlackboxWindow *StackingList::back(void) const {
  assert(stack.size() > 5);

  const_iterator it = desktop, _end = stack.begin();
  for (--it; it != _end; --it) {
    if (*it) return *it;
  }

  assert(0);
  return zero;
}


void StackingList::dump(void) const {
  const_iterator it = stack.begin(), end = stack.end();
  BlackboxWindow *win;
  fprintf(stderr, "Stack:\n");
  for (; it != end; ++it) {
    win = *it;
    if (win)
      fprintf(stderr, "%s: 0x%lx\n", win->getTitle(), win->getClientWindow());
    else
      fprintf(stderr, "zero\n");
  }
  fprintf(stderr, "the layers:\n");
  win = *fullscreen;
  if (win)
    fprintf(stderr, "%s: 0x%lx\n", win->getTitle(), win->getClientWindow());
  else
    fprintf(stderr, "zero\n");
  win = *above;
  if (win)
    fprintf(stderr, "%s: 0x%lx\n", win->getTitle(), win->getClientWindow());
  else
    fprintf(stderr, "zero\n");
  win = *normal;
  if (win)
    fprintf(stderr, "%s: 0x%lx\n", win->getTitle(), win->getClientWindow());
  else
    fprintf(stderr, "zero\n");
  win = *below;
  if (win)
    fprintf(stderr, "%s: 0x%lx\n", win->getTitle(), win->getClientWindow());
  else
    fprintf(stderr, "zero\n");
  win = *desktop;
  if (win)
    fprintf(stderr, "%s: 0x%lx\n", win->getTitle(), win->getClientWindow());
  else
    fprintf(stderr, "zero\n");
}
