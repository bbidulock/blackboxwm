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
class BImageControl;

#include "LinkedList.hh"


class Basemenu {
private:
  LinkedList<BasemenuItem> *menuitems;
  Blackbox *blackbox;
  Basemenu *parent;
  BImageControl *image_ctrl;

  Bool moving, visible, movable, user_moved, default_menu, title_vis, shifted;
  Display *display;
  int which_sub, which_press, which_sbl;

  struct menu {
    Pixmap iframe_pixmap, title_pixmap;
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
  void setMenuLabel(char *n);
  virtual void Show(void);
  void Hide(void);
  void Move(int, int);
  void Update(void);
  void defaultMenu(void) { default_menu = True; }
};


class BasemenuItem {
private:
  Basemenu *s;
  char **u, *l, *e;
  int f;

  friend Basemenu;


protected:


public:
  BasemenuItem(char *lp, int fp, char *ep = 0)
    { l = lp; e = ep; s = 0; f = fp; u = 0; }

  BasemenuItem(char *lp, Basemenu *mp)
    { l = lp; s = mp; e = 0; f = 0; u = 0; }

  BasemenuItem(char **up)
    { u = up; l = e = 0; f = 0; s = 0; }

  ~BasemenuItem(void)
    { /* the item doesn't delete any data it holds */ }

  char *exec(void) { return e; }
  char *label(void) { return l; }
  char **ulabel(void) { return u; }
  int function(void) { return f; }
  Basemenu *submenu(void) { return s; }
};


#endif // __Basemenu_hh
