// Rootmenu.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software. 
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
// DEALINGS IN THE SOFTWARE.
  
// stupid macros needed to access some functions in version 2 of the GNU C 
// library
#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "blackbox.hh"
#include "Rootmenu.hh"
#include "Screen.hh"

#ifdef    DEBUG
#  include "mem.h"
#endif // DEBUG

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS

#if defined(HAVE_PROCESS_H) && defined(__EMX__)
#  include <process.h>
#endif //   HAVE_PROCESS_H             __EMX__


Rootmenu::Rootmenu(BScreen *scrn) : Basemenu(scrn) {
#ifdef    DEBUG
  allocate(sizeof(Rootmenu), "Rootmenu.cc");
#endif // DEBUG

  screen = scrn;
  blackbox = screen->getBlackbox();
}


#ifdef    DEBUG
Rootmenu::~Rootmenu(void) {
  deallocate(sizeof(Rootmenu), "Rootmenu.cc");
}
#endif // DEBUG


void Rootmenu::itemSelected(int button, int index) {
  if (button == 1) {
    BasemenuItem *item = find(index);

    if (item->function()) {
      switch (item->function()) {
      case BScreen::Execute:
	if (item->exec()) {
#ifndef   __EMX__
          int dslen =
            strlen(DisplayString(screen->getBaseDisplay()->getXDisplay()));

#ifdef    DEBUG	  
	  allocate(sizeof(char) * (dslen + 32), "Rootmenu.cc");
#endif // dEBUG

          char *displaystring = new char[dslen + 32];

#ifdef    DEBUG
	  allocate(sizeof(char) * (strlen(item->exec()) + dslen + 64), "Rootmenu.cc");
#endif // DEBUG

          char *command = new char[strlen(item->exec()) + dslen + 64];
	  
          sprintf(displaystring, "%s",
		  DisplayString(screen->getBaseDisplay()->getXDisplay()));
          // gotta love pointer math
          sprintf(displaystring + dslen - 1, "%d", screen->getScreenNumber());
	  sprintf(command, "DISPLAY=\"%s\" exec %s &", displaystring,
		  item->exec());
	  system(command);

#ifdef    DEBUG
	  deallocate(sizeof(char) * (dslen + 32), "Rootmenu.cc");
	  deallocate(sizeof(char) * (strlen(item->exec()) + dslen + 64), "Rootmenu.cc");
#endif // DEBUG

          delete [] displaystring;
          delete [] command;
#else // !__EMX__
	  spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", item->exec(), NULL);
#endif // __EMX__
	}
	
	break;
      	
      case BScreen::Restart:
	blackbox->restart();
	break;
	
      case BScreen::RestartOther:
	if (item->exec())
	  blackbox->restart(item->exec());

	break;
	
      case BScreen::Exit:
	blackbox->shutdown();
	break;

      case BScreen::SetStyle:
	if (item->exec())
	  blackbox->saveStyleFilename(item->exec());

      case BScreen::Reconfigure:
        blackbox->reconfigure();

	return;
      }
      
      if (! (screen->getRootmenu()->isTorn() || isTorn()) &&
	  item->function() != BScreen::Reconfigure &&
	  item->function() != BScreen::SetStyle)
	hide();
    }
  }
}

