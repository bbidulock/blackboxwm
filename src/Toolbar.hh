//
// Toolbar.hh for Blackbox - an X11 Window manager
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

#ifndef __Toolbar_hh
#define __Toolbar_hh

#include <X11/Xlib.h>

// forward declaration
class Toolbar;

class Blackbox;
class BlackboxIcon;
class Iconmenu;
class Workspace;
class Workspacemenu;
class BImageControl;

#include "LinkedList.hh"


class Toolbar {
private:
  Bool wait_button, raised;
  Display *display;
  GC buttonGC;

  struct frame {
    Pixmap frame, label, button, pbutton, clk;
    Window window, menuButton, workspaceLabel, workspacePrev, workspaceNext,
      windowLabel, windowPrev, windowNext, clock;

    int x, y;
    unsigned int width, height, button_w, button_h, label_w, label_h,
      clock_w, clock_h, bevel_w;
  } frame;
  
  LinkedList<Workspace> *workspacesList;
  Blackbox *blackbox;
  Iconmenu *iconMenu;
  Workspace *current, *zero;
  Workspacemenu *wsMenu;

  BImageControl *image_ctrl;


protected:


public:
  Toolbar(Blackbox *, int = 1);
  ~Toolbar(void);

  Blackbox *_blackbox(void) { return blackbox; }

  int addWorkspace(void);
  int removeLastWorkspace(void);
  void changeWorkspaceID(int);
  Workspace *workspace(int);
  void addIcon(BlackboxIcon *i);
  void removeIcon(BlackboxIcon *i);
  void iconUpdate(void);
  void stackWindows(Window *, int);
  void Reconfigure(void);
  void checkClock(Bool = False);
  void redrawLabel(Bool = False);
  void readWorkspaceName(Window);

  void buttonPressEvent(XButtonEvent *);
  void buttonReleaseEvent(XButtonEvent *);
  void exposeEvent(XExposeEvent *);

  Window windowID(void) { return frame.window; }
  Workspace *currentWorkspace(void) { return current; }
  int count(void) { return workspacesList->count(); }
  int currentWorkspaceID(void);
  Bool Raised(void) { return raised; }
  unsigned int Width(void) { return frame.width; }
  unsigned int Height(void) { return frame.height; }
};


#endif // __Toolbar_hh
