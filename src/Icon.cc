//
// icon.cc for Blackbox - an X11 Window manager
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "Workspace.hh"
#include "Toolbar.hh"

#include "blackbox.hh"
#include "Icon.hh"
#include "Window.hh"


// *************************************************************************
// Icon class code
// *************************************************************************

Iconmenu::Iconmenu(Blackbox *bb) : Basemenu(bb) {
  defaultMenu();
  setMovable(False);
  setTitleVisibility(False);
  Update();

  blackbox = bb;
  iconList = new LinkedList<BlackboxIcon>;
}


Iconmenu::~Iconmenu(void) {
  delete iconList;
}


int Iconmenu::insert(BlackboxIcon *icon) {
  icon->setIconNumber(iconList->count());
  int ret = iconList->insert(icon);
  Basemenu::insert(icon->ULabel());

  Basemenu::Update();

  return ret;
}


int Iconmenu::remove(BlackboxIcon *icon) {
  iconList->remove(icon->iconNumber());
  int ret = Basemenu::remove(icon->iconNumber());

  int i;
  LinkedListIterator<BlackboxIcon> it(iconList);
  for (i = 0; it.current(); it++)
    it.current()->setIconNumber(i++);

  Update();

  return ret;
}


void Iconmenu::itemSelected(int button, int item) {
  if (button == 1) {
    BlackboxWindow *window = iconList->find(item)->bWindow();
    blackbox->toolbar()->workspace(window->workspace())->raiseWindow(window);
    window->deiconifyWindow();
  }
}


// *************************************************************************
// Icon class code
// *************************************************************************

BlackboxIcon::BlackboxIcon(Blackbox *bb, BlackboxWindow *win) {
  display = bb->control();
  window = win;
  toolbar = bb->toolbar();
  client = window->clientWindow();
  icon_number = -1;

  if (! XGetIconName(display, client, &name))
    name = "Unnamed";

  toolbar->addIcon(this);
}


BlackboxIcon::~BlackboxIcon(void) {
  toolbar->removeIcon(this);

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

  toolbar->iconUpdate();
}
