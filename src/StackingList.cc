// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// StackingList.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2004 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2004
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

#include <Unicode.hh>

#include <cassert>
#include <cstdio>


static StackEntity * const zero = 0;


StackingList::StackingList(void) {
  desktop = stack.insert(stack.begin(), zero);
  below = stack.insert(desktop, zero);
  normal = stack.insert(below, zero);
  above = stack.insert(normal, zero);
  fullscreen = stack.insert(above, zero);
}


StackingList::iterator StackingList::insert(StackEntity *entity) {
  assert(entity);

  iterator& it = layer(entity->layer());
  it = stack.insert(it, entity);
  return it;
}


StackingList::iterator StackingList::append(StackEntity *entity) {
  assert(entity);

  iterator& it = layer(entity->layer());
  if (!*it) { // empty layer
    it = stack.insert(it, entity);
    return it;
  }

  // find the end of the layer (the zero pointer)
  iterator tmp = std::find(it, stack.end(), zero);
  assert(tmp != stack.end());
  tmp = stack.insert(tmp, entity);
  return tmp;
}


StackingList::iterator StackingList::remove(StackEntity *entity) {
  assert(entity);

  iterator& pos = layer(entity->layer());
  iterator it = std::find(pos, stack.end(), entity);
  assert(it != stack.end());
  if (it == pos) ++pos;
  it = stack.erase(it);
  assert(stack.size() >= 5);
  return it;
}


StackingList::iterator& StackingList::layer(Layer which) {
  switch (which) { // teehee
  case LayerNormal:
    return normal;
  case LayerFullScreen:
    return fullscreen;
  case LayerAbove:
    return above;
  case LayerBelow:
    return below;
  case LayerDesktop:
    return desktop;
  }

  assert(0);
  return normal;
}


void StackingList::changeLayer(StackEntity *entity, Layer new_layer) {
  assert(entity);

  (void) remove(entity);
  entity->setLayer(new_layer);
  (void) insert(entity);
}


void StackingList::raise(StackEntity *entity) {
  assert(entity);

  iterator& pos = layer(entity->layer());
  iterator it = std::find(pos, stack.end(), entity);
  assert(it != stack.end());
  if (it == pos)
    ++pos;

  (void) stack.erase(it);
  pos = stack.insert(pos, entity);
}


void StackingList::lower(StackEntity *entity) {
  assert(entity);

  iterator& pos = layer(entity->layer());
  iterator it = std::find(pos, stack.end(), entity);
  assert(it != stack.end());
  if (it == pos)
    ++pos;

  (void) stack.erase(it);

  if (!*pos) { // empty layer
    pos = stack.insert(pos, entity);
    return;
  }

  it = std::find(pos, stack.end(), zero);
  assert(it != stack.end());
  (void) stack.insert(it, entity);
}


StackEntity *StackingList::front(void) const {
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


StackEntity *StackingList::back(void) const {
  assert(stack.size() > 5);

  const_iterator it = desktop, _end = stack.begin();
  for (--it; it != _end; --it) {
    if (*it) return *it;
  }

  assert(0);
  return zero;
}


static void print_entity(StackEntity *entity)
{
  BlackboxWindow *win = dynamic_cast<BlackboxWindow *>(entity);
  if (win) {
    fprintf(stderr, "  0x%lx: window 0x%lx '%s'\n",
            win->windowID(), win->clientWindow(),
            bt::toLocale(win->title()).c_str());
  } else if (entity) {
    fprintf(stderr, "  0x%lx: unknown entity\n",
            entity->windowID());
  } else {
    fprintf(stderr, "  zero\n");
  }
}


void StackingList::dump(void) const {
  const_iterator it = stack.begin(), _end = stack.end();
  fprintf(stderr, "Stack:\n");
  for (; it != _end; ++it)
    print_entity(*it);

  fprintf(stderr, "Layers:\n");
  print_entity(*fullscreen);
  print_entity(*above);
  print_entity(*normal);
  print_entity(*below);
  print_entity(*desktop);
}
