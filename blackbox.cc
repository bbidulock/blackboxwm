//
// blackbox.cc for Blackbox - an X11 Window manager
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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>

#include "blackbox.hh"
#include "session.hh"

#include <stdlib.h>
#include <unistd.h>

// global variable to allow access to restart and shutdown member functions
Blackbox *blackbox;


/*

  Resources allocated in the Blackbox constructor:

    Debugger *debug
    llist *session_list
    BlackboxSession *session

*/

Blackbox::Blackbox(int argc, char **argv) {
  b_argc = argc;
  b_argv = argv;

  char *session_display = NULL;
  ::blackbox = this;
  //
  //  Startup our window management session.
  //

  debug = new Debugger('!');

#ifdef DEBUG
  debug->enable();
#endif

  debug->enter("Blackbox::Blackbox\n");


  session_list = new llist<BlackboxSession>;
  //
  //  Scan the command line for a list of servers to manage.
  //

  int i;
  for (i = 1; i < argc; ++i) {
    if (! strcmp(argv[i], "-display")) {
      //
      //  In this case, we have been started by xinit or xdm to manage a single
      //  X session.  Open the single connection and start the event loop.
      //

      if (++i >= argc) {
	printf("error: '-display' requires and argument\n");
	exit(1);
      }
      
      debug->msg("beginning single display management '%s'\n", argv[i]);
      session_display = argv[i];

      //
      // Once the specified display is open... set the environment variable
      // for DISPLAY... xinit (and xdm?) do this already... but when future
      // multi X session management is finished... we'll need this set for
      // each separate X session.
      //
      if (! setenv("DISPLAY", session_display, 1)) {
	fprintf(stderr, "couldn't set environment variable DISPLAY\n");
	perror("setenv()");
      }
    } else if (! strcmp(argv[i], "-version")) {
      printf("Blackbox %s : (c) 1997, 1998 Brad Hughes\n\n",
             _blackbox_version);
      exit(0);
    } else if (! strcmp(argv[i], "-help")) {
      printf("Blackbox %s : (c) 1997, 1998 Brad Hughes\n",
             _blackbox_version);
      printf("\n"
             "  -display <string>\tuse display connection.\n"
	     "  -version\t\tdisplay version and exit.\n"
             "  -help\t\t\tdisplay this help text and exit.\n\n");
      exit(0);
    }
  }

  //
  // At the moment, blackbox can only manage one X server connection... this
  // will hopefully change with the advent of multiple threads and placing one
  // event loop in each thread.
  //

  session = new BlackboxSession(session_display);
  session_list->insert(session);
}


/*

  Resources deallocated:

  Debugger  *debug
  llist *session_list

  BlackboxSession *session NOT deallocated... memleak?

*/

Blackbox::~Blackbox(void) {
  session_list->remove(session);
  delete session;
  delete session_list;
  delete debug;
}


void Blackbox::EventLoop(void) {
  //
  // When multiple X sessions are supported... this function will change to
  // create new threads of execution and start an session->EventLoop() in
  // each thread.
  //
  // Note:  as stated above... one thread will be created for each X session
  // to be managed;
  //

  session->EventLoop();
}


void Blackbox::Restart(char *prog) {
  //
  // This function is just a quick "fix"
  // It is also subject to change when multithreads are incorporated...
  //

  if (prog)
    execlp(prog, prog, NULL);
  else {
    delete session;
    execvp(b_argv[0], b_argv);
  }
}
