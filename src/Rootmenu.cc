//
// Basemenu.cc for Blackbox - an X11 Window manager
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

#include "Rootmenu.hh"
#include "blackbox.hh"

#include <stdio.h>
#include <stdlib.h>
#ifdef __EMX__
#include <process.h>
#endif


Rootmenu::Rootmenu(Blackbox *bb) : Basemenu(bb) {
  blackbox = bb;
}


Rootmenu::~Rootmenu(void) {

}


void Rootmenu::itemSelected(int button, int index) {
  if (button == 1) {
    BasemenuItem *item = find(index);

    if (item->Function()) {
      switch (item->Function()) {
      case Blackbox::B_Execute: {
	if (item->Exec()) {
	  char *command = new char[strlen(item->Exec()) + 8];
#ifndef __EMX__
	  sprintf(command, "exec %s &", item->Exec());
	  system(command);
#else
	  sprintf(command, "%s", item->Exec());
	  spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", command, NULL);
#endif
	  delete [] command;
	}
	
	break; }
      
      case Blackbox::B_ExecReconfigure:
	if (item->Exec()) {
	  char *command = new char[strlen(item->Exec()) + 1];
	  sprintf(command, "%s", item->Exec());
	  system(command);
	  delete [] command;
	}
	
      case Blackbox::B_Reconfigure:
	blackbox->Reconfigure();
	break;
	
      case Blackbox::B_Restart:
	blackbox->Restart();
	break;
	
      case Blackbox::B_RestartOther:
	if (item->Exec())
	  blackbox->Restart(item->Exec());

	break;
	
      case Blackbox::B_Exit:
	blackbox->Exit();
	break;
      }
      
      if (! blackbox->Menu()->userMoved() &&
	  item->Function() != Blackbox::B_Reconfigure &&
	  item->Function() != Blackbox::B_ExecReconfigure)
	blackbox->Menu()->Hide();
    }
  }
}
