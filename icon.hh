//
// icon.hh for Blackbox - an X11 Window manager
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

#ifndef __blackbox_icon_hh
#define __blackbox_icon_hh

// forward declaration
class IconMenu;
class BlackboxIcon;

class Blackbox;
class WorkspaceManager;
class BlackboxWindow;

#include "Basemenu.hh"
#include "LinkedList.hh"


class Iconmenu : public Basemenu {
private:
  LinkedList<BlackboxIcon> *iconList;


protected:
  virtual void itemSelected(int, int);


public:
  Iconmenu(Blackbox *);
  ~Iconmenu(void);

  int insert(BlackboxIcon *);
  int remove(BlackboxIcon *);
};


class BlackboxIcon {
private:
  Display *display;
  Window client;

  char *name;
  int icon_number;
  
  BlackboxWindow *window;
  WorkspaceManager *wsManager;


protected:


public:
  BlackboxIcon(Blackbox *, BlackboxWindow *);
  ~BlackboxIcon(void);

  char **ULabel(void) { return &name; }
  void setIconNumber(int n) { icon_number = n; }
  const int iconNumber(void) { return (const) icon_number; }
  BlackboxWindow *bWindow(void) { return window; }

  void rereadLabel(void);
};


#endif
