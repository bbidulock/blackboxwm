// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// StackingList.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2005
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

#ifndef __StackingList_hh
#define __StackingList_hh

#include <Util.hh>

#include <list>
#include <vector>

class BlackboxWindow;
class StackEntity;

typedef std::list<BlackboxWindow *> BlackboxWindowList;
typedef std::list<StackEntity *> StackEntityList;
typedef std::vector<Window> WindowStack;

class StackingList {
public:
  enum Layer {
    LayerNormal,
    LayerFullScreen,
    LayerAbove,
    LayerBelow,
    LayerDesktop
  };

  typedef StackEntityList::iterator iterator;
  typedef StackEntityList::reverse_iterator reverse_iterator;
  typedef StackEntityList::const_iterator const_iterator;
  typedef StackEntityList::const_reverse_iterator const_reverse_iterator;

  StackingList(void);

  iterator insert(StackEntity *entity);
  iterator append(StackEntity *entity);
  iterator remove(StackEntity *entity);

  iterator& layer(Layer which);
  void changeLayer(StackEntity *entity, Layer new_layer);

  iterator raise(StackEntity *entity);
  iterator lower(StackEntity *entity);

  bool empty(void) const { return (stack.size() == 5); }
  StackEntityList::size_type size(void) const { return stack.size() - 5; }
  StackEntity *front(void) const;
  StackEntity *back(void) const;
  iterator begin(void) { return stack.begin(); }
  iterator end(void) { return stack.end(); }
  reverse_iterator rbegin(void) { return stack.rbegin(); }
  reverse_iterator rend(void) { return stack.rend(); }
  const_iterator begin(void) const { return stack.begin(); }
  const_iterator end(void) const { return stack.end(); }
  const_reverse_iterator rbegin(void) const { return stack.rbegin(); }
  const_reverse_iterator rend(void) const { return stack.rend(); }

  void dump(void) const;

private:
  StackEntityList stack;
  iterator fullscreen, above, normal, below, desktop;
};

class StackEntity {
private:
  StackingList::Layer _layer;
public:
  inline StackEntity() : _layer(StackingList::LayerNormal) { }
  inline void setLayer(StackingList::Layer new_layer)
  { _layer = new_layer; }
  inline StackingList::Layer layer(void) const
  { return _layer; }
  virtual Window windowID(void) const = 0;
};

#endif // __StackingList_hh
