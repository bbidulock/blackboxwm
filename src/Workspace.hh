// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Workspace.hh for Blackbox - an X11 Window manager
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

#ifndef   __Workspace_hh
#define   __Workspace_hh

extern "C" {
#include <X11/Xlib.h>
}

#include <list>
#include <vector>
#include <string>

class BScreen;
class Clientmenu;
class Workspace;
class BlackboxWindow;
class Netizen;

typedef std::list<BlackboxWindow*> BlackboxWindowList;
typedef std::list<Window> WindowList;

class StackingList {
public:
  typedef std::list<BlackboxWindow*> WindowStack;
  typedef WindowStack::iterator iterator;
  typedef WindowStack::reverse_iterator reverse_iterator;
  typedef WindowStack::const_iterator const_iterator;
  typedef WindowStack::const_reverse_iterator const_reverse_iterator;

  StackingList(void);
  void insert(BlackboxWindow* w);
  void remove(BlackboxWindow* w);

  iterator& findLayer(const BlackboxWindow* const w);

  bool empty(void) const { return (stack.size() == 5); }
  WindowStack::size_type size(void) const { return stack.size() - 5; }
  BlackboxWindow* front(void) const;
  iterator begin(void) { return stack.begin(); }
  iterator end(void) { return stack.end(); }
  reverse_iterator rbegin(void) { return stack.rbegin(); }
  reverse_iterator rend(void) { return stack.rend(); }
  const_iterator begin(void) const { return stack.begin(); }
  const_iterator end(void) const { return stack.end(); }
  const_reverse_iterator rbegin(void) const { return stack.rbegin(); }
  const_reverse_iterator rend(void) const { return stack.rend(); }

private:
  WindowStack stack;
  iterator fullscreen, above, normal, below, desktop;
};


class Workspace {
private:
  BScreen *screen;
  BlackboxWindow *lastfocus;
  Clientmenu *clientmenu;

  BlackboxWindowList windowList;
  StackingList stackingList;

  std::string name;
  unsigned int id;
  unsigned int cascade_x, cascade_y;

  Workspace(const Workspace&);
  Workspace& operator=(const Workspace&);

  void raiseTransients(const BlackboxWindow * const win,
                       WindowStack& stack);
  void lowerTransients(const BlackboxWindow * const win,
                       WindowStack& stack);

  void placeWindow(BlackboxWindow *win);
  bool cascadePlacement(bt::Rect& win, const bt::Rect& availableArea);
  bool smartPlacement(bt::Rect& win, const bt::Rect& availableArea);

public:
  Workspace(BScreen *scrn, unsigned int i = 0);

  inline BScreen *getScreen(void) { return screen; }

  inline BlackboxWindow *getLastFocusedWindow(void) { return lastfocus; }

  inline Clientmenu *getMenu(void) { return clientmenu; }

  inline const std::string& getName(void) const { return name; }

  inline unsigned int getID(void) const { return id; }

  inline void setLastFocusedWindow(BlackboxWindow *w) { lastfocus = w; }

  BlackboxWindow* getWindow(unsigned int index);
  BlackboxWindow* getNextWindowInList(BlackboxWindow *w);
  BlackboxWindow* getPrevWindowInList(BlackboxWindow *w);
  BlackboxWindow* getTopWindowOnStack(void) const;

  void focusFallback(const BlackboxWindow *old_window);

  bool isCurrent(void) const;
  bool isLastWindow(const BlackboxWindow* w) const;

  void addWindow(BlackboxWindow *w, bool place = False);
  unsigned int removeWindow(BlackboxWindow *w);
  unsigned int getCount(void) const;
  void updateClientListStacking(Netwm::WindowList& clientList) const;

  void show(void);
  void hide(void);
  void removeAll(void);
  void raiseWindow(BlackboxWindow *w);
  void lowerWindow(BlackboxWindow *w);
  void reconfigure(void);
  void setCurrent(void);
  void setName(const std::string& new_name);
};


#endif // __Workspace_hh

