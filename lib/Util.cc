// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Util.cc for Blackbox - an X11 Window manager
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <X11/Xatom.h>

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

#include <assert.h>
}

#include <algorithm>

#include "Util.hh"


#ifndef   HAVE_BASENAME
std::string basename (const std::string& path) {
  std::string::size_type slash = path.rfind('/');
  if (slash == std::string::npos)
    return path;
  return path.substr(slash+1);
}
#endif // HAVE_BASENAME


std::string bt::expandTilde(const std::string& s) {
  if (s[0] != '~') return s;

  const char* const home = getenv("HOME");
  if (home == NULL) return s;

  return std::string(home + s.substr(s.find('/')));
}


void bt::bexec(const std::string& command, const std::string& displaystring) {
#ifndef    __EMX__
  if (! fork()) {
#ifndef __QNXTO__ // apparently, setsid interferes with signals on QNX
    setsid();
#endif
    int ret = putenv(const_cast<char *>(displaystring.c_str()));
    assert(ret != -1);
    std::string cmd = "exec ";
    cmd += command;
    ret = execl("/bin/sh", "/bin/sh", "-c", cmd.c_str(), NULL);
    exit(ret);
  }
#else //   __EMX__
  spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", command, NULL);
#endif // !__EMX__
}


std::string bt::textPropertyToString(Display *display,
                                     XTextProperty& text_prop) {
  std::string ret;

  if (text_prop.value && text_prop.nitems > 0) {
    if (text_prop.encoding == XA_STRING) {
      ret = reinterpret_cast<char *>(text_prop.value);
    } else {
      text_prop.nitems = strlen(reinterpret_cast<char *>(text_prop.value));

      char **list;
      int num;
      if (XmbTextPropertyToTextList(display, &text_prop,
                                    &list, &num) == Success &&
          num > 0 && *list) {
        ret = *list;
        XFreeStringList(list);
      }
    }
  }

  return ret;
}


::timeval bt::normalizeTimeval(const ::timeval &tm) {
  ::timeval ret = tm;

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


std::string bt::itostring(unsigned long i) {
  if (i == 0)
    return std::string("0");

  const char nums[] = "0123456789";

  std::string tmp;
  for (; i > 0; i /= 10)
    tmp.insert(tmp.begin(), nums[i%10]);
  return tmp;
}


std::string bt::itostring(long i) {
  std::string tmp = bt::itostring(static_cast<unsigned long>(abs(i)));
  if (i < 0)
    tmp.insert(tmp.begin(), '-');
  return tmp;
}


std::string bt::itostring(unsigned int i) {
  return bt::itostring(static_cast<unsigned long>(i));
}


std::string bt::itostring(int i) {
  return bt::itostring(static_cast<long>(i));
}


std::string bt::itostring(unsigned short i) {
  return bt::itostring(static_cast<unsigned long>(i));
}


std::string bt::itostring(short i) {
  return bt::itostring(static_cast<long>(i));
}
