//
// Windowmenu.cc for Blackbox - an X11 Window manager
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

#include "Windowmenu.hh"
#include "Workspace.hh"
#include "Toolbar.hh"

#include "blackbox.hh"
#include "Window.hh"

#include <string.h>


Windowmenu::Windowmenu(BlackboxWindow *win, Blackbox *bb) : Basemenu(bb) {
  window = win;
  toolbar = bb->toolbar();

  setTitleVisibility(False);
  setMovable(False);
  defaultMenu();

  sendToMenu = new SendtoWorkspaceMenu(win, bb);
  insert("Send To ...", sendToMenu);
  if (window->hasTitlebar())
    insert("(Un)Shade", Blackbox::B_WindowShade);
  if (window->isIconifiable())
    insert("Iconify", Blackbox::B_WindowIconify);
  if (window->isMaximizable())
    insert("(Un)Maximize", Blackbox::B_WindowMaximize);  
  insert("Raise", Blackbox::B_WindowRaise);
  insert("Lower", Blackbox::B_WindowLower);
  insert("(Un)Stick", Blackbox::B_WindowStick);
  if (window->isClosable())
    insert("Close", Blackbox::B_WindowClose);
  else
    insert("Kill Client", Blackbox::B_WindowKill);
  
  Update();
}


Windowmenu::~Windowmenu(void) {
  delete sendToMenu;
}


void Windowmenu::itemSelected(int button, int index) {
  if (button == 1) {
    BasemenuItem *item = find(index);
    
    switch (item->function()) {
    case Blackbox::B_WindowShade:
      Hide();
      window->shadeWindow();
      break;
      
    case Blackbox::B_WindowIconify:
      Hide();
      window->iconifyWindow();
      break;
      
    case Blackbox::B_WindowMaximize:
      Hide();
      window->maximizeWindow();
      break;
      
    case Blackbox::B_WindowClose:
      Hide();
      window->closeWindow();
      break;
      
    case Blackbox::B_WindowRaise:
      Hide();
      toolbar->workspace(window->workspace())->raiseWindow(window);
      if (window->isStuck())
	toolbar->currentWorkspace()->restackWindows();
      break;
      
    case Blackbox::B_WindowLower:
      Hide();
      toolbar->workspace(window->workspace())->lowerWindow(window);
      if (window->isStuck())
	toolbar->currentWorkspace()->restackWindows();
      break;

    case Blackbox::B_WindowStick:
      Hide();
      if (! window->isStuck()) {
	int id = window->workspace();
	window->setWorkspace(0);
	toolbar->workspace(id)->removeWindow(window);
	toolbar->workspace(0)->addWindow(window);
	window->stickWindow(True);
      } else {
	toolbar->workspace(0)->removeWindow(window);
	toolbar->currentWorkspace()->addWindow(window);
	window->stickWindow(False);
      }

      toolbar->currentWorkspace()->restackWindows();
      break;

    case Blackbox::B_WindowKill:
      Hide();
      XKillClient(toolbar->_blackbox()->control(), window->clientWindow());
      break;
    }
  }
}


void Windowmenu::Reconfigure(void) {
  sendToMenu->Reconfigure();
  
  Basemenu::Reconfigure();
}


SendtoWorkspaceMenu::SendtoWorkspaceMenu(BlackboxWindow *win, Blackbox *bb) :
  Basemenu(bb)
{
  window = win;
  toolbar = bb->toolbar();

  setTitleVisibility(False);
  setMovable(False);
  defaultMenu();
  Update();
}


void SendtoWorkspaceMenu::itemSelected(int button, int index) {
  if (button == 1) {
    if ((index + 1) <= toolbar->count())
      if ((index + 1) != toolbar->currentWorkspaceID()) {
	toolbar->workspace(window->workspace())->removeWindow(window);
	toolbar->workspace(index + 1)->addWindow(window);
	if (window->isStuck()) window->stickWindow(False);
	window->withdrawWindow();
      }
  } else
    Update();
}


void SendtoWorkspaceMenu::Update(void) {
  int i, r = Count();
  
  if (Count() != 0)
    for (i = 0; i < r; ++i)
      remove(0);
  
  for (i = 1; i < toolbar->count(); ++i)
    insert(toolbar->workspace(i)->Name());
  
  Basemenu::Update();
}


void SendtoWorkspaceMenu::Show(void) {
  Update();

  Basemenu::Show();
}
