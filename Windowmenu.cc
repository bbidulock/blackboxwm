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

#define __GNU_SOURCE
#include "Windowmenu.hh"
#include "Workspace.hh"
#include "WorkspaceManager.hh"

#include "blackbox.hh"
#include "window.hh"

#include <string.h>


Windowmenu::Windowmenu(BlackboxWindow *win, Blackbox *bb) : Basemenu(bb) {
  window = win;
  wsManager = bb->workspaceManager();

  setTitleVisibility(False);
  setMovable(False);
  defaultMenu();

  sendToMenu = new SendtoWorkspaceMenu(win, bb);
  insert("Send To ...", sendToMenu);
  insert("(Un)Shade", Blackbox::B_WindowShade);
  insert("Iconify", Blackbox::B_WindowIconify);
  
  if (window->resizable())
    insert("(Un)Maximize", Blackbox::B_WindowMaximize);
  
  insert("Close", Blackbox::B_WindowClose);
  insert("Raise", Blackbox::B_WindowRaise);
  insert("Lower", Blackbox::B_WindowLower);
  
  Update();
}


Windowmenu::~Windowmenu(void) {
  delete sendToMenu;
}


void Windowmenu::itemSelected(int button, int index) {
  if (button == 1) {
    BasemenuItem *item = find(index);
    
    switch (item->Function()) {
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
      wsManager->currentWorkspace()->raiseWindow(window);
      break;
      
    case Blackbox::B_WindowLower:
      Hide();
      wsManager->currentWorkspace()->lowerWindow(window);
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
  wsManager = bb->workspaceManager();

  char *l = new char[strlen("Send To ...") + 1];
  strncpy(l, "Send To ...", strlen("Send To ...") + 1);
  setMenuLabel(l);

  setTitleVisibility(True);
  Update();
}


SendtoWorkspaceMenu::~SendtoWorkspaceMenu(void) {

}


void SendtoWorkspaceMenu::itemSelected(int button, int index) {
  if (button == 1) {
    if (index < wsManager->count())
      if (index != wsManager->currentWorkspaceID()) {
	wsManager->workspace(window->workspace())->removeWindow(window);
	wsManager->workspace(index)->addWindow(window);
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
  
  for (i = 0; i < wsManager->count(); ++i)
    insert(wsManager->workspace(i)->Name());

  Basemenu::Update();
}
