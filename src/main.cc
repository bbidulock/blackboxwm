// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// main.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2003
//         Bradley T Hughes <bhughes at trolltech.com>
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

// #define PRINT_SIZES

#if defined(PRINT_SIZES)
#  include "Screen.hh"
#  include "Slit.hh"
#  include "Toolbar.hh"
#  include "Window.hh"
#endif

#include "blackbox.hh"
#include "../version.h"

#include <stdio.h>


static void showHelp(int exitval) {
  // print version - this should not be localized!
  printf("Blackbox %s\n"
         "Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry\n"
         "Copyright (c) 1997 - 2000, 2002 - 2003 Bradley T Hughes\n",
         __blackbox_version);

  // print program usage and command line options
  printf("  -display <string>\t\tuse display connection.\n"
         "  -single <string>\t\tmanage the default screen only\n"
         "  -rc <string>\t\t\tuse alternate resource file.\n"
         "  -version\t\t\tdisplay version and exit.\n"
         "  -help\t\t\t\tdisplay this help text and exit.\n\n");

  // some people have requested that we print out compile options
  // as well
  printf("Compile time options:\n"
         "  Debugging:\t\t\t%s\n"
         "  Shape:\t\t\t%s\n\n",
#ifdef    DEBUG
         "yes",
#else // !DEBUG
         "no",
#endif // DEBUG

#ifdef    SHAPE
         "yes"
#else // !SHAPE
         "no"
#endif // SHAPE
         );

  ::exit(exitval);
}

int main(int argc, char **argv) {
  const char *dpy_name = 0;
  std::string rc_file;
  bool multi_head = true;

  for (int i = 1; i < argc; ++i) {
    if (! strcmp(argv[i], "-help")) {
      showHelp(0);
    } else if (! strcmp(argv[i], "-version")) {
      // print current version string, this should not be localized!
      printf("Blackbox %s\n"
             "Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry\n"
             "Copyright (c) 1997 - 2000, 2002 - 2003 Bradley T Hughes\n",
             __blackbox_version);

      ::exit(0);
    } else if (! strcmp(argv[i], "-rc")) {
      // look for alternative rc file to use

      if ((++i) >= argc) {
        fprintf(stderr, "error: '-rc' requires and argument\n");
        ::exit(1);
      }

      rc_file = argv[i];
    } else if (! strcmp(argv[i], "-display")) {
      // check for -display option... to run on a display other than the one
      // set by the environment variable DISPLAY

      if ((++i) >= argc) {
        fprintf(stderr, "error: '-display' requires an argument\n");
        ::exit(1);
      }

      dpy_name = argv[i];
      std::string dtmp = "DISPLAY=";
      dtmp += dpy_name;

      if (putenv(const_cast<char*>(dtmp.c_str()))) {
        fprintf(stderr,
                "warning: couldn't set environment variable 'DISPLAY'\n");
        perror("putenv()");
      }
    } else if (! strcmp(argv[i], "-single")) {
      multi_head = false;
    } else { // invalid command line option
      showHelp(-1);
    }
  }

#ifdef    __EMX__
  _chdir2(getenv("X11ROOT"));
#endif // __EMX__

  if (rc_file.empty())
    rc_file = "~/.blackboxrc";
  rc_file = bt::expandTilde(rc_file);

#if defined(PRINT_SIZES)
  printf("Blackbox      : %4d bytes\n"
         "BScreen       : %4d bytes\n"
         "BlackboxWindow: %4d bytes\n"
         "Toolbar       : %4d bytes\n"
         "Slit          : %4d bytes\n",
         sizeof(Blackbox),
         sizeof(BScreen),
         sizeof(BlackboxWindow),
         sizeof(Toolbar),
         sizeof(Slit));
#endif

  Blackbox blackbox(argv, dpy_name, rc_file, multi_head);
  blackbox.run();
  return 0;
}
