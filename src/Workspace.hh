//
// workspace.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@arn.net
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// (See the included file COPYING / GPL-2.0)
//

#ifndef __blackbox_workspace_hh
#define __blackbox_workspace_hh

#include <X11/Xlib.h>

// forward declaration
class Workspace;

#include "Clientmenu.hh"
#include "LinkedList.hh"
#include "Toolbar.hh"
#include "Window.hh"


class Workspace {
private:
  Toolbar *toolbar;

  Clientmenu *cMenu;
  LinkedList<BlackboxWindow> *windowList;

  Window *stack;
  char *name, **label;
  int id;


protected:


public:
  Workspace(Toolbar *, int = 0);
  ~Workspace(void);

  Bool isCurrent(void);
  void setCurrent(void);
  const int addWindow(BlackboxWindow *);
  const int removeWindow(BlackboxWindow *);
  BlackboxWindow *window(int);
  const int Count(void);
  int showAll(void);
  int hideAll(void);
  int removeAll(void);
  void raiseWindow(BlackboxWindow *);
  void lowerWindow(BlackboxWindow *);
  void restackWindows(void);
  void setFocusWindow(int);
  void Reconfigure();
  void Update();

  Window *windowStack(void) { return stack; }
  int workspaceID(void) { return id; }
  char *Name(void) { return name; }
  char **Label(void) { return label; }
  Clientmenu *Menu(void) { return cMenu; }
};


#endif
