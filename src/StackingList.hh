// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// StackingList.hh for Blackbox - an X11 Window manager
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

#ifndef __StackingList_hh
#define __StackingList_hh

#include <Util.hh>

#include <list>
#include <vector>

typedef std::vector<Window> WindowStack;

class BlackboxWindow;

class StackingList {
 public:
  enum Layer {
    LayerNormal,
    LayerFullScreen,
    LayerAbove,
    LayerBelow,
    LayerDesktop
  };

  typedef std::list<BlackboxWindow *> _List;
  typedef _List::iterator iterator;
  typedef _List::reverse_iterator reverse_iterator;
  typedef _List::const_iterator const_iterator;
  typedef _List::const_reverse_iterator const_reverse_iterator;

  StackingList(void);

  iterator insert(BlackboxWindow *win);
  iterator append(BlackboxWindow *win);
  iterator remove(BlackboxWindow *win);

  iterator& layer(Layer layer);
  void changeLayer(BlackboxWindow *win, Layer new_layer);

  void raise(BlackboxWindow *win);
  void lower(BlackboxWindow *win);

  bool empty(void) const { return (stack.size() == 5); }
  _List::size_type size(void) const { return stack.size() - 5; }
  BlackboxWindow *front(void) const;
  BlackboxWindow *back(void) const;
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
  _List stack;
  iterator fullscreen, above, normal, below, desktop;
};

#endif // __StackingList_hh
