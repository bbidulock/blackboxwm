//
// sessionmenu.cc for Blackbox - an X11 Window manager
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

#include "blackbox.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// *************************************************************************
// Session menu class code - root menu
// *************************************************************************

SessionMenu::SessionMenu(Blackbox *bb) : BaseMenu(bb) {
  blackbox = bb;
  default_menu = False;
}
  
  
SessionMenu::~SessionMenu(void) {
  int n = count();
  for (int i = 0; i < n; i++)
    remove(0);
}


int SessionMenu::remove(int index) {
  if (index >= 0 && index < count()) {
    BaseMenuItem *itmp = find(index);

    if (! default_menu) {
      if (itmp->Submenu()) {
	SessionMenu *tmp = (SessionMenu *) itmp->Submenu();
	delete tmp;
      }
      if (itmp->Label()) {
	delete [] itmp->Label();
      }
      if (itmp->Exec()) {
	delete [] itmp->Exec();
      }
    }
    
    return BaseMenu::remove(index);
  }

  return -1;
}


void SessionMenu::itemPressed(int button, int item) {
  if (button == 1 && hasSubmenu(item)) {
    if (find(item)->Submenu()->menuVisible())
      XRaiseWindow(blackbox->control(), find(item)->Submenu()->windowID());
  }
}


void SessionMenu::titlePressed(int) {
}


void SessionMenu::titleReleased(int button) {
  if (button == 3 && windowID() == blackbox->Rootmenu()->windowID())
    hideMenu();
}


void SessionMenu::itemReleased(int button, int index) {
  if (button == 1) {
    BaseMenuItem *item = find(index);
    if (item->Function()) {
      switch (item->Function()) {
      case Blackbox::B_Reconfigure:
	blackbox->Reconfigure();
	break;

      case Blackbox::B_Restart:
	blackbox->Restart();
	break;

      case Blackbox::B_RestartOther:
	blackbox->Restart(item->Exec());
	break;

      case Blackbox::B_Exit:
	blackbox->Exit();
	break;
      }

      if (! blackbox->Rootmenu()->userMoved() &&
	  item->Function() != Blackbox::B_Reconfigure)
	blackbox->Rootmenu()->hideMenu();
    } else if (item->Exec()) {
      char *command = new char[strlen(item->Exec()) + 8];
      sprintf(command, "exec %s &", item->Exec());
      system(command);
      delete [] command;
      if (! blackbox->Rootmenu()->userMoved())
	blackbox->Rootmenu()->hideMenu();
    }
  }
}
