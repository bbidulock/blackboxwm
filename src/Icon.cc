//
// icon.cc for Blackbox - an X11 Window manager
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "blackbox.hh"
#include "Icon.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"


Iconmenu::Iconmenu(Blackbox *bb, BScreen *scrn) : Basemenu(bb, scrn) {
  defaultMenu();
  setMovable(False);
  setHidable(True);
  setTitleVisibility(False);
  update();
  
  blackbox = bb;
  screen = scrn;
  iconList = new LinkedList<BlackboxIcon>;
}


Iconmenu::~Iconmenu(void) {
  delete iconList;
}


int Iconmenu::insert(BlackboxIcon *icon) {
  icon->setIconNumber(iconList->count());
  int ret = iconList->insert(icon);
  Basemenu::insert(icon->getULabel());

  Basemenu::update();

  return ret;
}


int Iconmenu::remove(BlackboxIcon *icon) {
  iconList->remove(icon->getIconNumber());
  int ret = Basemenu::remove(icon->getIconNumber());

  int i;
  LinkedListIterator<BlackboxIcon> it(iconList);
  for (i = 0; it.current(); it++)
    it.current()->setIconNumber(i++);

  update();

  if (! getCount()) hide();

  return ret;
}


void Iconmenu::itemSelected(int button, int item) {
  if (button == 1) {
    BlackboxWindow *window = iconList->find(item)->getWindow();
    screen->getWorkspace(window->getWorkspaceNumber())->raiseWindow(window);
    window->deiconify();

    screen->getWorkspacemenu()->hide();
  }
}


BlackboxIcon::BlackboxIcon(Blackbox *bb, BlackboxWindow *win) {
  display = bb->getDisplay();
  window = win;
  screen = window->getScreen();
  client = window->getClientWindow();
  icon_number = -1;

  if (! XGetIconName(display, client, &name))
    name = "Unnamed";

  screen->addIcon(this);
}


BlackboxIcon::~BlackboxIcon(void) {
  screen->removeIcon(this);

  if (name)
    if (strcmp(name, "Unnamed"))
      XFree(name);
}


void BlackboxIcon::rereadLabel(void) {
  if (name)
    if (strcmp(name, "Unnamed"))
      XFree(name);
  if (! XGetIconName(display, client, &name))
    name = "Unnamed";

  screen->iconUpdate();
}
