// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// WindowGroup.hh for Blackbox - an X11 Window manager
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

#ifndef __WindowGroup_hh
#define __WindowGroup_hh

#include <Util.hh>
#include <list>

class Blackbox;
class BScreen;
class BlackboxWindow;
typedef std::list<BlackboxWindow *> BlackboxWindowList;

class BWindowGroup {
private:
  Blackbox *blackbox;
  Window group;
  BlackboxWindowList windowList;
  BlackboxWindowList transientList;

public:
  BWindowGroup(Blackbox *b, Window _group);
  ~BWindowGroup(void);

  inline Window groupWindow(void) const
  { return group; }

  inline bool empty(void) const
  { return windowList.empty(); }

  void addWindow(BlackboxWindow *win);
  void removeWindow(BlackboxWindow *win);

  inline const BlackboxWindowList &windows(void) const
  { return windowList; }
  inline const BlackboxWindowList &transients(void) const
  { return transientList; }
};

#endif // __WindowGroup_hh
