//
// WorkspaceManager.hh for Blackbox - an X11 Window manager
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

#ifndef __WorkspaceManager_hh
#define __WorkspaceManager_hh

#include <X11/Xlib.h>

// forward declaration
class WorkspaceManager;

class Blackbox;
class BlackboxIcon;
class Iconmenu;
class Workspace;
class Workspacemenu;

#include "LinkedList.hh"
#include "graphics.hh"


class WorkspaceManager {
private:
  Bool waitb1, waitb3, waiti;
  Display *display;
  GC buttonGC;

  struct frame {
    Pixmap button, pbutton, ibutton, pibutton;
    Window base, window, workspaceDock, fButton, bButton, iconButton, clock;

    int x, y;
    unsigned int width, height, button_w, button_h, wsd_w, wsd_h, ib_w, ib_h,
      clock_w, clock_h, bevel_w;
  } frame;
  
  LinkedList<Workspace> *workspacesList;
  Blackbox *blackbox;
  Iconmenu *iconMenu;
  Workspace *current, *zero;
  Workspacemenu *wsMenu;


protected:


public:
  WorkspaceManager(Blackbox *, int = 1);
  ~WorkspaceManager(void);

  Blackbox *_blackbox(void) { return blackbox; }

  int addWorkspace(void);
  int removeLastWorkspace(void);
  void changeWorkspaceID(int);
  Workspace *workspace(int);
  void DissociateAll(void);
  void addIcon(BlackboxIcon *i);
  void removeIcon(BlackboxIcon *i);
  void iconUpdate(void);
  void stackWindows(Window *, int);
  void Reconfigure(void);
  void checkClock(Bool = False);
  void redrawWSD(Bool = False);

  void buttonPressEvent(XButtonEvent *);
  void buttonReleaseEvent(XButtonEvent *);
  void exposeEvent(XExposeEvent *);
  Window windowID(void) { return frame.window; }
  Window wsdWindowID(void) { return frame.workspaceDock; }
  Workspace *currentWorkspace(void) { return current; }
  int count(void) { return workspacesList->count(); }
  int currentWorkspaceID(void);

  unsigned int Width(void) { return frame.width; }
  unsigned int buttonWidth(void) { return frame.button_w; }
  unsigned int Height(void) { return frame.height; }
};


#endif // __WorkspaceManager_hh
