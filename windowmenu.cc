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
#include "blackbox.hh"
#include "menu.hh"
#include "window.hh"
#include "workspace.hh"

#include <string.h>


// *************************************************************************
// Window Menu class code
// *************************************************************************

BlackboxWindowMenu::BlackboxWindowMenu(BlackboxWindow *w, Blackbox *bb) :
  BaseMenu(bb)
{
  window = w;
  blackbox = bb;
  hideTitle();
  setMovable(False);
  
  send_to_menu = new SendToWorkspaceMenu(window, blackbox);
  insert("Send To ...", send_to_menu);
  insert("(Un)Shade", Blackbox::B_WindowShade);
  insert("Iconify", Blackbox::B_WindowIconify);

  if (window->resizable())
    insert("(Un)Maximize", Blackbox::B_WindowMaximize);

  insert("Close", Blackbox::B_WindowClose);
  insert("Raise", Blackbox::B_WindowRaise);
  insert("Lower", Blackbox::B_WindowLower);

  useSublevels(2);
  updateMenu();
}


BlackboxWindowMenu::~BlackboxWindowMenu(void)
{ delete send_to_menu; }


void BlackboxWindowMenu::Reconfigure(void) {
  send_to_menu->Reconfigure();
  BaseMenu::Reconfigure();
}


void BlackboxWindowMenu::titlePressed(int) { }
void BlackboxWindowMenu::titleReleased(int) { }
void BlackboxWindowMenu::itemPressed(int button, int item) {
  if (button == 1 && item == 0) {
    send_to_menu->updateMenu();
    XRaiseWindow(blackbox->control(), send_to_menu->windowID());
  } else if (button == 3 && item == 0) {
    send_to_menu->updateMenu();
    XRaiseWindow(blackbox->control(), send_to_menu->windowID());
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
	blackbox->raiseWindow(window);
	break;
	
      case 6:
	hideMenu();
	blackbox->lowerWindow(window);
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
	blackbox->raiseWindow(window);
	break;
	
      case 5:
	hideMenu();
	blackbox->lowerWindow(window);
	break;
      }
  }
}


void BlackboxWindowMenu::showMenu()
{ BaseMenu::showMenu(); window->setMenuVisible(true); }
void BlackboxWindowMenu::hideMenu() {
  send_to_menu->hideMenu();
  BaseMenu::hideMenu();
  window->setMenuVisible(false);
}


void BlackboxWindowMenu::moveMenu(int x, int y)
{ BaseMenu::moveMenu(x, y); }


// *************************************************************************
// Window send to workspace menu class code
// *************************************************************************

SendToWorkspaceMenu::SendToWorkspaceMenu(BlackboxWindow *w,
					 Blackbox *bb)
  : BaseMenu(bb)
{
  window = w;
  ws_manager = bb->WSManager();

  char *l = new char[strlen("Send To ...") + 1];
  strncpy(l, "Send To ...", strlen("Send To ...") + 1);
  setMenuLabel(l);
  showTitle();
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
  BaseMenu::showMenu();
}


void SendToWorkspaceMenu::hideMenu(void)
{ BaseMenu::hideMenu(); }
void SendToWorkspaceMenu::updateMenu(void) {
  int i, r = BaseMenu::count();

  if (BaseMenu::count() != 0)
    for (i = 0; i < r; ++i)
      BaseMenu::remove(0);
  
  for (i = 0; i < ws_manager->count(); ++i)
    BaseMenu::insert(ws_manager->workspace(i)->name());
  
  BaseMenu::updateMenu();
}
