//
// Basemenu.hh for Blackbox - an X11 Window manager
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

#ifndef __Basemenu_hh
#define __Basemenu_hh

#include <X11/Xlib.h>

// forward declarations
class Basemenu;
class BasemenuItem;

class Blackbox;

#include "LinkedList.hh"
#include "graphics.hh"

// base menu class... it is inherited for sessions, windows, and workspaces
class Basemenu {
private:
  LinkedList<BasemenuItem> *menuitems;
  Blackbox *blackbox;
  Basemenu *parent;

  Bool moving, visible, movable, user_moved, default_menu, title_vis, shifted;
  Display *display;
  GC titleGC, itemGC, hitemGC, hbgGC;
  int which_sub, which_press, which_sbl;

  struct menu {
    Pixmap iframe_pixmap;
    Window frame, iframe, title;

    char *label;
    int x, y, x_move, y_move, x_shift, y_shift, sublevels, persub;
    unsigned int width, height, title_h, iframe_h, item_w, item_h, bevel_w,
      bevel_h;
  } menu;

  void drawSubmenu(int, Bool = False);
  void drawItem(int, Bool = False, Bool = False);


protected:
  BasemenuItem *find(int index) { return menuitems->find(index); }
  void setTitleVisibility(Bool b) { title_vis = b; }
  void setMovable(Bool b) { movable = b; }

  virtual void itemSelected(int, int) = 0;


public:
  Basemenu(Blackbox *);
  virtual ~Basemenu(void);

  void buttonPressEvent(XButtonEvent *);
  void buttonReleaseEvent(XButtonEvent *);
  void motionNotifyEvent(XMotionEvent *);
  void enterNotifyEvent(XCrossingEvent *);
  void leaveNotifyEvent(XCrossingEvent *);
  void exposeEvent(XExposeEvent *);

  void Reconfigure(void);
  Bool hasSubmenu(int);

  int insert(char *, int = 0, char * = 0);
  int insert(char **);
  int insert(char *, Basemenu *);
  int remove(int);

  Bool userMoved(void) { return user_moved; }
  Window WindowID(void) { return menu.frame; }
  unsigned int Width(void) { return menu.width; }
  unsigned int Height(void) { return menu.height; }
  unsigned int titleHeight(void) { return menu.title_h; }
  int X(void) { return menu.x; }
  int Y(void) { return menu.y; }
  int Visible(void) { return visible; }
  const char *Label(void) const { return menu.label; }
  int Count(void) { return menuitems->count(); }
  void setMenuLabel(char *n) { menu.label = n; }
  virtual void Show(void);
  void Hide(void);
  void Move(int, int);
  void Update(void);
  void defaultMenu(void) { default_menu = True; }
};


// menu items held in the menus
class BasemenuItem {
private:
  Basemenu *sub_menu;
  char **ulabel, *label, *exec;
  int function;

  friend Basemenu;


protected:


public:
  BasemenuItem(char *l, int f, char *e = 0)
    { label = l; exec = e; sub_menu = 0; function = f; ulabel = 0; }

  BasemenuItem(char *l, Basemenu *m)
    { label = l; sub_menu = m; exec = 0; function = 0; ulabel = 0; }

  BasemenuItem(char **u)
    { ulabel = u; label = 0; exec = 0; function = 0; sub_menu = 0; }

  ~BasemenuItem(void)
    { /* the item doesn't delete any data it holds */ }

  char *Exec(void) { return exec; }
  char *Label(void) { return label; }
  char **ULabel(void) { return ulabel; }
  int Function(void) { return function; }
  Basemenu *Submenu(void) { return sub_menu; }
};


#endif // __Basemenu_hh
