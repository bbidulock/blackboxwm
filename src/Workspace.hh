//
// workspace.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 1999 by Brad Hughes, bhughes@tcac.net
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

#ifndef   __Workspace_hh
#define   __Workspace_hh

#include <X11/Xlib.h>

class BScreen;
class Clientmenu;
class Workspace;
class BlackboxWindow;

#include "LinkedList.hh"


class Workspace {
private:
#ifdef    KDE
  Atom desktop_name_atom;
#endif // KDE
  
  BScreen *screen;
  Clientmenu *clientmenu;

  LinkedList<BlackboxWindow> *windowList;

  char *name, **label;
  int id, cascade_x, cascade_y;


protected:
  void placeWindow(BlackboxWindow *);


public:
  Workspace(BScreen *, int = 0);
  ~Workspace(void);

  BlackboxWindow *getWindow(int);

  Bool isCurrent(void);

  BScreen *getScreen(void) { return screen; }

  Clientmenu *getMenu(void) { return clientmenu; }
  
  char *getName(void) { return name; }
  char **getLabel(void) { return label; }
  
  const int addWindow(BlackboxWindow *, Bool = False);
  const int removeWindow(BlackboxWindow *);
  const int getCount(void);
  
  int getWorkspaceID(void) { return id; }
  int showAll(void);
  int hideAll(void);
  int removeAll(void);
  
  void raiseWindow(BlackboxWindow *);
  void lowerWindow(BlackboxWindow *);
  void setFocusWindow(int);
  void reconfigure();
  void update();
  void setCurrent(void);
  void setName(char *);
  void shutdown(void);

#ifdef    KDE
  void getKWMWindowRegion(int *, int *, int *, int *);
  void rereadName(void);
#endif // KDE
};


#endif // __Workspace_hh
