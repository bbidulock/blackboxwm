// -*- mode: C++; indent-tabs-mode: nil; -*-
// Workspace.hh for Blackbox - an X11 Window manager
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

#ifndef   __Workspace_hh
#define   __Workspace_hh

#include <X11/Xlib.h>

#include <list>
#include <string>

class BScreen;
class Clientmenu;
class Workspace;
class BlackboxWindow;

typedef std::list<BlackboxWindow*> BlackboxWindowList;

class Workspace {
private:
  BScreen *screen;
  BlackboxWindow *lastfocus;
  Clientmenu *clientmenu;

  BlackboxWindowList stackingList, windowList;

  std::string name;
  unsigned int id;
  int cascade_x, cascade_y;

  Workspace(const Workspace&);
  Workspace& operator=(const Workspace&);

protected:
  void placeWindow(BlackboxWindow *win);


public:
  Workspace(BScreen *scrn, unsigned int i = 0);
  ~Workspace(void);

  inline BScreen *getScreen(void) { return screen; }

  inline BlackboxWindow *getLastFocusedWindow(void) { return lastfocus; }
  
  inline Clientmenu *getMenu(void) { return clientmenu; }

  inline const char *getName(void) const { return name.c_str(); }

  inline const unsigned int getWorkspaceID(void) const { return id; }
  
  inline void setLastFocusedWindow(BlackboxWindow *w) { lastfocus = w; }

  BlackboxWindow *getWindow(unsigned int index);

  Bool isCurrent(void) const;
  Bool isLastWindow(const BlackboxWindow* w) const;
  
  const int addWindow(BlackboxWindow *w, Bool place = False);
  const int removeWindow(BlackboxWindow *w);
  const unsigned int getCount(void) const;

  void showAll(void);
  void hideAll(void);
  void removeAll(void);
  void raiseWindow(BlackboxWindow *w);
  void lowerWindow(BlackboxWindow *w);
  void reconfigure(void);
  void updateFocusModel(void);
  void update(void);
  void setCurrent(void);
  void setName(const char* new_name);
  void shutdown(void);
};


#endif // __Workspace_hh

