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
#include "blackbox.hh"
#include "session.hh"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// global variable to allow access to restart and shutdown member functions
Blackbox *blackbox;


// *************************************************************************
// signal handler to allow for proper and gentle shutdown
// *************************************************************************

static void signalhandler(int i) {
  static int re_enter = 0;
  
  fprintf(stderr, "%s: toplevel:\n\t[ signal %d caught ]\n", __FILE__, i);
  if (! re_enter) {
    re_enter = 1;
    fprintf(stderr, "\t[ shutting down ]\n");
    blackbox->Shutdown(False);
  }

  fprintf(stderr, "\t[ exiting ]\n");
  abort();
}


// *************************************************************************
// Blackbox class code
// *************************************************************************
//
// allocations:
// llist *session_list
// BlackboxSession * - one for each managed display
//
// *************************************************************************

Blackbox::Blackbox(int argc, char **argv) {
  // install signal handlers for fatal signals
  signal(SIGSEGV, (void (*)(int)) signalhandler);
  signal(SIGTERM, (void (*)(int)) signalhandler);
  signal(SIGINT, (void (*)(int)) signalhandler);

  // store program arguments (used in restart)
  b_argc = argc;
  b_argv = argv;

  // the initial display is set to NULL... and Xlib will read the environment
  // variable DISPLAY to determine with which X server to establish the session
  char *session_display = NULL;
  ::blackbox = this;
  session_list = new llist<BlackboxSession>;

  // scan the command line for a list of servers to manage.
  int i;
  for (i = 1; i < argc; ++i) {
    if (! strcmp(argv[i], "-display")) {
      // use display specified on the command line
      if (++i >= argc) {
	printf("error: '-display' requires and argument\n");
	exit(1);
      }
      
      session_display = argv[i];
      // set the environment variable DISPLAY
      if (setenv("DISPLAY", session_display, 1)) {
	fprintf(stderr, "couldn't set environment variable DISPLAY\n");
	perror("setenv()");
      }
    } else if (! strcmp(argv[i], "-version")) {
      // print current version string
      printf("Blackbox %s : (c) 1997, 1998 Brad Hughes\n\n",
             _blackbox_version);
      exit(0);
    } else if (! strcmp(argv[i], "-help")) {
      // print program options
      printf("Blackbox %s : (c) 1997, 1998 Brad Hughes\n",
             _blackbox_version);
      printf("\n"
             "  -display <string>\tuse display connection.\n"
	     "  -version\t\tdisplay version and exit.\n"
             "  -help\t\t\tdisplay this help text and exit.\n\n");
      exit(0);
    }
  }

  // At the moment, blackbox can only manage one X server connection... this
  // will hopefully change with the advent of multiple threads and placing one
  // event loop in each thread.
  session_list->insert(new BlackboxSession(session_display));
}


Blackbox::~Blackbox(void) {
  delete session_list;
}


void Blackbox::EventLoop(void) {
  // When multiple X sessions are supported... this function will change to
  // create new threads of execution and start an session->EventLoop() in
  // each thread.
  //
  // Note:  as stated above... one thread will be created for each X session
  // to be managed;
  session_list->at(0)->EventLoop();
}


void Blackbox::Restart(char *prog) {
  // This function is just a quick "fix"
  // It is also subject to change when multithreads are incorporated.
  if (prog) {
    for (int i = 0; i < session_list->count(); ++i)
      session_list->at(i)->Dissociate();

    execlp(prog, prog, NULL);
  } else {
    for (int i = 0; i < session_list->count(); ++i)
      session_list->at(i)->Dissociate();

    execvp(b_argv[0], b_argv);
  }
}


void Blackbox::Shutdown(Bool do_delete) {
  // end management for all sessions and quit
  for (int i = 0; i < session_list->count(); ++i) {
    BlackboxSession *tmp = session_list->at(i);
    tmp->Dissociate();
    delete tmp;
  }

  if (do_delete)
    delete this;
}
