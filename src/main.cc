//
// main.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 1999 by Brad Hughes, bhughes@tcac.net
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

#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "blackbox.hh"

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#endif // STDC_HEADERS

#ifdef    HAVE_LOCALE_H
#  include <locale.h>
#endif // HAVE_LOCALE_H

#ifdef    HAVE_UNISTD_H
#include <sys/types.h>
#endif // HAVE_UNISTD_H

#ifdef    HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

#ifndef   MAXPATHLEN
#define   MAXPATHLEN 255
#endif // MAXPATHLEN


int main(int argc, char **argv) {
  char *session_display = (char *) 0;
  
#ifdef    HAVE_SETLOCALE
  setlocale(LC_ALL, "");
#endif // HAVE_SETLOCALE

  int i;
  for (i = 1; i < argc; ++i) {
    // check for -display option... to run on a display other than the one
    // set by the environment variable DISPLAY
    if (! strcmp(argv[i], "-display")) {
      if ((++i) >= argc) {
	fprintf(stderr, "error: '-display' requires an argument\n");

	::exit(1);
      }
      
      session_display = argv[i];
      char dtmp[MAXPATHLEN];
      sprintf(dtmp, "DISPLAY=%s", session_display);
      
      if (putenv(dtmp)) {
	fprintf(stderr,
		"warning: couldn't set environment variable 'DISPLAY'\n");
	perror("putenv()");
      }
    } else if (! strcmp(argv[i], "-version")) {
      // print current version string
      printf("Blackbox %s : (c) 1997 - 1999 Brad Hughes\n\n",
             __blackbox_version);

      ::exit(0);
    } else if (! strcmp(argv[i], "-help")) {
      // print program usage and command line options
      printf("Blackbox %s : (c) 1997 - 1999 Brad Hughes\n\n"
             "  -display <string>\tuse display connection.\n"
	     "  -version\t\tdisplay version and exit.\n"
             "  -help\t\t\tdisplay this help text and exit.\n\n",
	     __blackbox_version);

      ::exit(0);
    }
  }

#ifdef    __EMX__
  _chdir2(getenv("X11ROOT"));
#endif // __EMX__

  Blackbox blackbox(argc, argv, session_display);
  blackbox.eventLoop();
  
  return(0);
}
