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

#include "blackbox.hh"
#include "../version.h"
#include "../nls/blackbox-nls.hh"

#include <i18n.hh>

#include <stdio.h>


bt::I18n bt::i18n; // initialized in main


static void showHelp(int exitval) {
  // print version - this should not be localized!
  printf("Blackbox %s\n"
         "Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry\n"
         "Copyright (c) 1997 - 2000, 2002 - 2003 Bradley T Hughes\n",
         __blackbox_version);

  // print program usage and command line options
  printf(bt::i18n(mainSet, mainUsage,
                  "  -display <string>\t\tuse display connection.\n"
                  "  -single <string>\t\tmanage the default screen only\n"
                  "  -rc <string>\t\t\tuse alternate resource file.\n"
                  "  -version\t\t\tdisplay version and exit.\n"
                  "  -help\t\t\t\tdisplay this help text and exit.\n\n"));

  // some people have requested that we print out compile options
  // as well
  printf(bt::i18n(mainSet, mainCompileOptions,
                  "Compile time options:\n"
                  "  Debugging:\t\t\t%s\n"
                  "  Shape:\t\t\t%s\n\n"),
#ifdef    DEBUG
         bt::i18n(CommonSet, CommonYes, "yes"),
#else // !DEBUG
         bt::i18n(CommonSet, CommonNo, "no"),
#endif // DEBUG

#ifdef    SHAPE
         bt::i18n(CommonSet, CommonYes, "yes")
#else // !SHAPE
         bt::i18n(CommonSet, CommonNo, "no")
#endif // SHAPE

         );

  ::exit(exitval);
}

int main(int argc, char **argv) {
  const char *dpy_name = 0;
  std::string rc_file;
  bool multi_head = true;

  bt::i18n.openCatalog("blackbox.cat");

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
        fprintf(stderr,
                bt::i18n(mainSet, mainRCRequiresArg,
                                 "error: '-rc' requires and argument\n"));

        ::exit(1);
      }

      rc_file = argv[i];
    } else if (! strcmp(argv[i], "-display")) {
      // check for -display option... to run on a display other than the one
      // set by the environment variable DISPLAY

      if ((++i) >= argc) {
        fprintf(stderr,
                bt::i18n(mainSet, mainDISPLAYRequiresArg,
                                 "error: '-display' requires an argument\n"));

        ::exit(1);
      }

      dpy_name = argv[i];
      std::string dtmp = "DISPLAY=";
      dtmp += dpy_name;

      if (putenv(const_cast<char*>(dtmp.c_str()))) {
        fprintf(stderr, bt::i18n(mainSet, mainWarnDisplaySet,
                "warning: couldn't set environment variable 'DISPLAY'\n"));
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

  if (rc_file.empty()) rc_file = "~/.blackboxrc";
  rc_file = bt::expandTilde(rc_file);

  Blackbox blackbox(argv, dpy_name, rc_file, multi_head);
  blackbox.eventLoop();

  return(0);
}
