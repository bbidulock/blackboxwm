// Windowmenu.cc for Blackbox - an X11 Window manager
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

#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "blackbox.hh"
#include "Screen.hh"
#include "Window.hh"
#include "Windowmenu.hh"
#include "Workspace.hh"

#ifdef    STDC_HEADERS
#  include <string.h>
#endif // STDC_HEADERS


Windowmenu::Windowmenu(BlackboxWindow *win, Blackbox *bb) :
  Basemenu(bb, win->getScreen())
{
  window = win;
  screen = window->getScreen();

  setTitleVisibility(False);
  setMovable(False);
  defaultMenu();

  sendToMenu = new SendtoWorkspaceMenu(win, bb);
  insert("Send To ...", sendToMenu);
  if (window->hasTitlebar())
    insert("(Un)Shade", BScreen::WindowShade);
  if (window->isIconifiable())
    insert("Iconify", BScreen::WindowIconify);
  if (window->isMaximizable())
    insert("(Un)Maximize", BScreen::WindowMaximize);  
  insert("Raise", BScreen::WindowRaise);
  insert("Lower", BScreen::WindowLower);
  insert("(Un)Stick", BScreen::WindowStick);
  if (window->isClosable())
    insert("Close", BScreen::WindowClose);
  insert("Kill Client", BScreen::WindowKill);
  
  update();
}


Windowmenu::~Windowmenu(void) {
  delete sendToMenu;
}


void Windowmenu::itemSelected(int button, int index) {
  BasemenuItem *item = find(index);
    
  switch (item->function()) {
  case BScreen::WindowShade:
    hide();
    window->shade();
    break;
      
  case BScreen::WindowIconify:
    hide();
    window->iconify();
    break;
      
  case BScreen::WindowMaximize:
    hide();
    window->maximize((unsigned int) button);
    break;
      
  case BScreen::WindowClose:
    hide();
    window->close();
    break;
      
  case BScreen::WindowRaise:
    hide();
    screen->getWorkspace(window->getWorkspaceNumber())->raiseWindow(window);
    break;
    
  case BScreen::WindowLower:
    hide();
    screen->getWorkspace(window->getWorkspaceNumber())->lowerWindow(window);
    break;

  case BScreen::WindowStick:
    hide();
    window->stick();
    break;

  case BScreen::WindowKill:
    hide();
    XKillClient(screen->getDisplay()->getDisplay(), window->getClientWindow());
    break;
  }
}


void Windowmenu::reconfigure(void) {
  sendToMenu->reconfigure();
  
  Basemenu::reconfigure();
}


void Windowmenu::setClosable(void) {
  int i, n = getCount();

  for (i = 0; i < n; i++) {
    BasemenuItem *item = find(i);

    if (item && (item->function() == BScreen::WindowKill)) {
      remove(i);

      break;
    }
  }

  n = getCount();
  for (i = 0; i < n; i++) {
    BasemenuItem *item = find(i);

    if (item && (item->function() == BScreen::WindowClose)) {
      remove(i);

      break;
    }
  }

  if (window->isClosable())
    insert("Close", BScreen::WindowClose);
  insert("Kill Client", BScreen::WindowKill);
      
  update();
}


SendtoWorkspaceMenu::SendtoWorkspaceMenu(BlackboxWindow *win, Blackbox *bb) :
  Basemenu(bb, win->getScreen())
{
  window = win;
  screen = window->getScreen();

  setTitleVisibility(False);
  setMovable(False);
  defaultMenu();
  update();
}


void SendtoWorkspaceMenu::itemSelected(int button, int index) {
  if (button == 1) {
    if ((index) <= screen->getCount()) {
      if ((index) != screen->getCurrentWorkspaceID()) {	
	screen->getWorkspace(window->getWorkspaceNumber())->
	  removeWindow(window);
	screen->getWorkspace(index)->addWindow(window);
	
	if (window->isStuck()) window->stick();
	window->withdraw();
      }
    }
  } else
    update();
}


void SendtoWorkspaceMenu::update(void) {
  int i, r = getCount();
  
  if (getCount() != 0)
    for (i = 0; i < r; ++i)
      remove(0);
  
  for (i = 0; i < screen->getCount(); ++i)
    insert(screen->getWorkspace(i)->getName());
  
  Basemenu::update();
}


void SendtoWorkspaceMenu::show(void) {
  update();

  Basemenu::show();
}
