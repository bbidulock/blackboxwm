// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Util.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh at debian.org>
// Copyright (c) 1997 - 2000, 2002 Bradley T Hughes <bhughes at trolltech.com>
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

#include "Util.hh"

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_STRING_H
#  include <string.h> // C string
#endif // HAVE_STRING_H

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif // HAVE_STDLIB_H

#ifdef    TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else // !TIME_WITH_SYS_TIME
#  ifdef    HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else // !HAVE_SYS_TIME_H
#    include <time.h>
#  endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME

#ifdef    HAVE_UNISTD_H
#  include <sys/types.h>
#  include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#include <string>   // C++ string
using std::string;


char* expandTilde(const char* s) {
  if (s[0] != '~') return bstrdup(s);

  string ret = getenv("HOME");
  char* path = strchr(s, '/');
  ret += path;
  return bstrdup(ret.c_str());
}

char* bstrdup(const char *s) {
  const size_t len = strlen(s) + 1;
  char *n = new char[len];
  strcpy(n, s);
  return n;
}

void bexec(const string &command, int screen)
{
  if (command.empty()) {
    fprintf(stderr, "bexec: command not specified.\n");
    return;
  }

  // setup DISPLAY environment variable
  string displaystring = getenv("DISPLAY");
  if (displaystring.empty()) {
    fprintf(stderr, "bexec: DISPLAY not set.\n");
    return;
  }
  displaystring = "DISPLAY=" + displaystring;
  if (screen != -1) {
    unsigned int dot = displaystring.rfind('.');
    if (dot != string::npos)
      displaystring.resize(dot);
    char screennumber[32];
    sprintf(screennumber, ".%d", screen);
    displaystring += screennumber;
  }

  // setup command to execute
  string cmd = "exec ";
  cmd += command;

  if (! fork()) {
    setsid();
    putenv(displaystring.c_str());
    execl("/bin/sh", "/bin/sh", "-c", cmd.c_str(), NULL);
    perror("execl");
    exit(0);
  }
}

timeval normalizeTimeval(const timeval &tm) {
  timeval ret = tm;
  while (ret.tv_usec < 0) {
    if (ret.tv_sec > 0) {
      --ret.tv_sec;
      ret.tv_usec += 1000000;
    } else {
      ret.tv_usec = 0;
    }
  }
  if (ret.tv_usec >= 1000000) {
    ret.tv_sec += ret.tv_usec / 1000000;
    ret.tv_usec %= 1000000;
  }

  if (ret.tv_sec < 0) ret.tv_sec = 0;
  return ret;
}
