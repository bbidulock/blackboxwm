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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "Util.hh"

#include <X11/Xatom.h>
#include <assert.h>
#if defined(__EMX__)
#  include <process.h>
#endif // __EMX__
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>


std::string bt::basename(const std::string& path) {
  std::string::size_type slash = path.rfind('/');
  if (slash == std::string::npos)
    return path;
  return path.substr(slash+1);
}


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
  spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", command.c_str(), NULL);
#endif // !__EMX__
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


std::string bt::textPropertyToString(::Display *display,
                                     ::XTextProperty& text_prop) {
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
