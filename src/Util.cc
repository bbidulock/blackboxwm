// -*- mode: C++; indent-tabs-mode: nil; -*-
// Util.cc for Blackbox - an X11 Window manager
// Copyright (c) 2002 Sean 'Shaleh' Perry <shaleh@debian.org>
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H
#if defined(HAVE_PROCESS_H) && defined(__EMX__)
#  include <process.h>
#endif //   HAVE_PROCESS_H             __EMX__

#include "Util.hh"

using std::string;

string expandTilde(const string& s) {
  if (s[0] != '~') return s;

  string ret = getenv("HOME");
  ret += s.substr(s.find('/'));
  return ret; 
}


void bexec(const string& command, const string& displaystring) {
#ifndef    __EMX__
  if (! fork()) {
    setsid();
    putenv(const_cast<char *>(displaystring.c_str()));
    string cmd = "exec ";
    cmd += command;
    execl("/bin/sh", "/bin/sh", "-c", cmd.c_str(), NULL);
    exit(0);
  }
#else //   __EMX__
  spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", command, NULL);
#endif // !__EMX__
}


#ifndef   HAVE_BASENAME
string basename (const string& path) {
  string::size_type slash = path.rfind('/');
  if (slash == string::npos)
    return path;
  return path.substr(slash+1);
}
#endif // HAVE_BASENAME


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
