//
// Basemenu.cc for Blackbox - an X11 Window manager
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
#include "Rootmenu.hh"
#include "Screen.hh"

#if HAVE_STDIO_H
#  include <stdio.h>
#endif

#if STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif

#if HAVE_PROCESS_H && __EMX__
#  include <process.h>
#endif


Rootmenu::Rootmenu(Blackbox *bb, BScreen *scrn) : Basemenu(bb, scrn) {
  blackbox = bb;
  screen = scrn;
}


void Rootmenu::itemSelected(int button, int index) {
  if (button == 1) {
    BasemenuItem *item = find(index);

    if (item->function()) {
      switch (item->function()) {
      case Blackbox::B_Execute:
	if (item->exec()) {
#ifndef __EMX__
          int dslen = strlen(DisplayString(screen->getDisplay()));
	  
          char *displaystring = new char[dslen + 32];
          char *command = new char[strlen(item->exec()) + dslen + 64];
	  
          sprintf(displaystring, "%s", DisplayString(screen->getDisplay()));
          // gotta love pointer math
          sprintf(displaystring + dslen - 1, "%d", screen->getScreenNumber());
	  sprintf(command, "DISPLAY=%s exec %s &", displaystring,
		  item->exec());
	  system(command);

          delete [] displaystring;
          delete [] command;
#else
	  spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", item->exec(), NULL);
#endif
	}
	
	break;
      
      case Blackbox::B_ExecReconfigure:
	if (item->exec()) {
	  char *command = new char[strlen(item->exec()) + 1];
	  sprintf(command, "%s", item->exec());
	  system(command);
	  delete [] command;
	}
	
      case Blackbox::B_Reconfigure:
	blackbox->reconfigure();
	break;
	
      case Blackbox::B_Restart:
	blackbox->restart();
	break;
	
      case Blackbox::B_RestartOther:
	if (item->exec())
	  blackbox->restart(item->exec());

	break;
	
      case Blackbox::B_Exit:
	blackbox->exit();
	break;

      case Blackbox::B_SetStyle:
	if (item->exec()) {
	  blackbox->saveStyleFilename(item->exec());
	  blackbox->reconfigure();
	}
	
	break;
      }
      
      if (! (screen->getRootmenu()->hasUserMoved() || hasUserMoved()) &&
	  item->function() != Blackbox::B_Reconfigure &&
	  item->function() != Blackbox::B_ExecReconfigure &&
	  item->function() != Blackbox::B_SetStyle)
	screen->getRootmenu()->hide();
    }
  } else if (button == 3)
    screen->getRootmenu()->hide();
}


void Rootmenu::show(void) {
  XRaiseWindow(screen->getDisplay(), getWindowID());

  Basemenu::show();
}
