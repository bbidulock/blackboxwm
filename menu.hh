//
// menu.hh for Blackbox - an X11 Window manager
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

#ifndef _blackbox_menu_hh
#define _blackbox_menu_hh

#include <X11/Xlib.h>

#include "llist.hh"


// forward declarations
class BlackboxMenu;
class BlackboxMenuItem;
class BlackboxSession;


// base menu class... it is inherited for sessions, windows, and workspaces
class BlackboxMenu {
private:
  llist<BlackboxMenuItem> *menuitems;
  BlackboxSession *session;

  Bool moving, show_title, visible, sub, movable, user_moved;
  Display *display;
  GC titleGC, itemGC, pitemGC;
  
  int which_sub;

  struct menu {
    char *label;
    int x, y, x_move, y_move, sublevels, persub, use_sublevels;
    unsigned width, height,
      title_w, title_h,
      button_w, button_h, item_h, item_w;
    Pixmap item_pixmap, pushed_pixmap;
    Window frame, title;
  } menu;


protected:
  virtual void itemPressed(int, int) = 0;
  virtual void itemReleased(int, int) = 0;
  virtual void titlePressed(int) = 0;
  virtual void titleReleased(int) = 0;

  void drawSubmenu(int);
  Window createItemWindow(void);
  int remove(int);

  void showTitle(void) { show_title = True; }
  void hideTitle(void) { show_title = False; }
  BlackboxMenuItem *find(int index) { return menuitems->find(index); }


public:
  BlackboxMenu(BlackboxSession *);
  virtual ~BlackboxMenu(void);

  void buttonPressEvent(XButtonEvent *);
  void buttonReleaseEvent(XButtonEvent *);
  void motionNotifyEvent(XMotionEvent *);
  void exposeEvent(XExposeEvent *);
  void Reconfigure(void);
  Bool hasSubmenu(int);

  int insert(char *);
  int insert(char **);
  int insert(char *, int, char * = 0);
  int insert(char *, BlackboxMenu *);

  Window windowID(void) { return menu.frame; }
  unsigned int Width(void) { return menu.width; }
  unsigned int Height(void) { return menu.height; }
  unsigned int titleHeight(void) { return menu.title_h; }
  int X(void) { return menu.x; }
  int Y(void) { return menu.y; }
  int menuVisible(void) { return visible; }
  const char *label(void) const { return menu.label; }
  int count(void) { return menuitems->count(); }
  Bool userMoved(void) { return user_moved; }
  void setMenuLabel(char *n) { menu.label = n; }
  void setMovable(Bool b) { movable = b; }

  void showMenu(void);
  void hideMenu(void);
  void moveMenu(int, int);
  void updateMenu(void);
  void useSublevels(int s) { menu.use_sublevels = s; }
};


// menu items held in the menus
class BlackboxMenuItem {
private:
  Window window;
  BlackboxMenu *sub_menu;
  char **ulabel, *label, *exec;
  int function;

  friend BlackboxMenu;


protected:


public:
  BlackboxMenuItem(Window w, char **u)
    { window = w; ulabel = u; label = 0; exec = 0; sub_menu = 0;
      function = 0; }
  
  BlackboxMenuItem(Window w, char *l, int f)
    { window = w; ulabel = 0; label = l; exec = 0; sub_menu = 0;
      function = f; }
  
  BlackboxMenuItem(Window w, char *l, char *e)
    { window = w; ulabel = 0; label = l; exec = e; sub_menu = 0;
      function = 0; }

  BlackboxMenuItem(Window w, char *l, int f, char *e)
    { window = w; ulabel = 0; label = l; exec = e; sub_menu = 0;
      function = f; }

  BlackboxMenuItem(Window w, char *l)
    { window = w; ulabel = 0; label = l; exec = 0; sub_menu = 0;
      function = 0; }

  BlackboxMenuItem(Window w, char *l, BlackboxMenu *m)
    { window = w; ulabel = 0; label = l; sub_menu = m; exec = 0;
      function = 0; }

  ~BlackboxMenuItem(void)
    { /* the item doesn't delete any data it holds */ }

  char *Exec(void) { return exec; }
  char *Label(void) { return label; }
  char **ULabel(void) { return ulabel; }
  int Function(void) { return function; }
  BlackboxMenu *Submenu(void) { return sub_menu; }
};


#endif
