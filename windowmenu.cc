//
// windowmenu.cc for Blackbox - an X11 Window manager
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

#define _GNU_SOURCE
#include "window.hh"
#include "session.hh"
#include "workspace.hh"


// *************************************************************************
// Window Menu class code
// *************************************************************************
//
// allocations:
// SendToWorkspaceMenu *send_to_menu
//
// *************************************************************************

BlackboxWindowMenu::BlackboxWindowMenu(BlackboxWindow *w, BlackboxSession *s) :
  BlackboxMenu(s)
{
  window = w;
  session = s;
  hideTitle();
  setMovable(False);
  
  send_to_menu = new SendToWorkspaceMenu(window, session);
  insert("Send To ...", send_to_menu);
  insert("(Un)Shade", BlackboxSession::B_WindowShade);
  insert("Iconify", BlackboxSession::B_WindowIconify);

  if (window->resizable())
    insert("(Un)Maximize", BlackboxSession::B_WindowMaximize);

  insert("Close", BlackboxSession::B_WindowClose);
  insert("Raise", BlackboxSession::B_WindowRaise);
  insert("Lower", BlackboxSession::B_WindowLower);

  updateMenu();
}


BlackboxWindowMenu::~BlackboxWindowMenu(void)
{ delete send_to_menu; }


void BlackboxWindowMenu::Reconfigure(void) {
  BlackboxMenu::Reconfigure();
}


void BlackboxWindowMenu::titlePressed(int) { }
void BlackboxWindowMenu::titleReleased(int) { }
void BlackboxWindowMenu::itemPressed(int button, int item) {
  if (button == 1 && item == 0) {
    send_to_menu->updateMenu();
    XRaiseWindow(session->control(), send_to_menu->windowID());
    BlackboxMenu::drawSubmenu(item);
  } else if (button == 3 && item == 0) {
    send_to_menu->updateMenu();
    XRaiseWindow(session->control(), send_to_menu->windowID());
  }
}


void BlackboxWindowMenu::itemReleased(int button, int item) {
  if (button == 1) {
    if (window->resizable())
      switch (item) {
      case 1:
	hideMenu();
	window->shadeWindow();
	break;

      case 2:
	hideMenu();
	window->iconifyWindow();
	break;
	
      case 3:
	hideMenu();
	window->maximizeWindow();
	break;
	
      case 4:
	hideMenu();
	window->closeWindow();
	break;
	
      case 5:
	hideMenu();
	session->raiseWindow(window);
	break;
	
      case 6:
	hideMenu();
	session->lowerWindow(window);
	break;
      }
    else
      switch (item) {
      case 1:
	hideMenu();
	window->shadeWindow();
	break;

      case 2:
	hideMenu();
	window->iconifyWindow();
	break;
	
      case 3:
	hideMenu();
	window->closeWindow();
	break;
	
      case 4:
	hideMenu();
	session->raiseWindow(window);
	break;
	
      case 5:
	hideMenu();
	session->lowerWindow(window);
	break;
      }
  }
}


void BlackboxWindowMenu::showMenu()
{ BlackboxMenu::showMenu(); window->setMenuVisible(true); }
void BlackboxWindowMenu::hideMenu() {
  send_to_menu->hideMenu();
  BlackboxMenu::hideMenu();
  window->setMenuVisible(false);
}


void BlackboxWindowMenu::moveMenu(int x, int y)
{ BlackboxMenu::moveMenu(x, y); }


// *************************************************************************
// Window send to workspace menu class code
// *************************************************************************

SendToWorkspaceMenu::SendToWorkspaceMenu(BlackboxWindow *w,
					 BlackboxSession *s)
  : BlackboxMenu(s)
{
  window = w;
  ws_manager = s->WSManager();

  hideTitle();
  updateMenu();
}


void SendToWorkspaceMenu::titlePressed(int) { }
void SendToWorkspaceMenu::titleReleased(int) { }
void SendToWorkspaceMenu::itemPressed(int, int) { }
void SendToWorkspaceMenu::itemReleased(int button, int item) {
  if (button == 1)
    if (item < ws_manager->count()) {
      if (item != ws_manager->currentWorkspaceID()) {
	ws_manager->workspace(window->workspace())->removeWindow(window);
	ws_manager->workspace(item)->addWindow(window);
	window->withdrawWindow();
	hideMenu();
      }
    } else
      updateMenu();
}


void SendToWorkspaceMenu::showMenu(void) {
  updateMenu();
  BlackboxMenu::showMenu();
}


void SendToWorkspaceMenu::hideMenu(void)
{ BlackboxMenu::hideMenu(); }
void SendToWorkspaceMenu::updateMenu(void) {
  int i, r = BlackboxMenu::count();

  if (BlackboxMenu::count() != 0)
    for (i = 0; i < r; ++i)
      BlackboxMenu::remove(0);
  
  for (i = 0; i < ws_manager->count(); ++i)
    BlackboxMenu::insert(ws_manager->workspace(i)->name());
  
  BlackboxMenu::updateMenu();
}
