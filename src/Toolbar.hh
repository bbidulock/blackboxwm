//
// Toolbar.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@tcac.net
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
class BScreen;

#include "LinkedList.hh"


class Toolbar {
private:
  Bool wait_button, raised, editing;
  Display *display;
  GC buttonGC;

  struct frame {
    Pixmap frame, label, button, pbutton, clk, reading;
    Window window, menuButton, workspaceLabel, workspacePrev, workspaceNext,
      windowLabel, windowPrev, windowNext, clock;

    int x, y;
    unsigned int width, height, button_w, button_h, label_w, label_h,
      clock_w, clock_h, bevel_w;
  } frame;
  
  Blackbox *blackbox;
  BImageControl *image_ctrl;
  BScreen *screen;
  
  char *new_workspace_name, *new_name_pos;


protected:


public:
  Toolbar(Blackbox *, BScreen *);
  ~Toolbar(void);

  //  Blackbox *getBlackbox(void) { return blackbox; }
  
  Bool isRaised(void)      { return raised; }
  Window getWindowID(void) { return frame.window; }
  
  unsigned int getWidth(void)  { return frame.width; }
  unsigned int getHeight(void) { return frame.height; }
  unsigned int getX(void)   { return frame.x; }
  unsigned int getY(void)   { return frame.y; }

  void buttonPressEvent(XButtonEvent *);
  void buttonReleaseEvent(XButtonEvent *);
  void exposeEvent(XExposeEvent *);
  void keyPressEvent(XKeyEvent *);

  void redrawWindowLabel(Bool = False);
  void redrawWorkspaceLabel(Bool = False);
  void redrawMenuButton(Bool = False, Bool = False);
  void redrawPrevWorkspaceButton(Bool = False, Bool = False);
  void redrawNextWorkspaceButton(Bool = False, Bool = False);
  void redrawPrevWindowButton(Bool = False, Bool = False);
  void redrawNextWindowButton(Bool = False, Bool = False);
  void reconfigure(void);

#ifdef    HAVE_STRFTIME
  void checkClock(Bool = False);
#else //  HAVE_STRFTIME
  void checkClock(Bool = False, Bool = False);
#endif // HAVE_STRFTIME
};


#endif // __Toolbar_hh
