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


void bt::Rect::setX(int x_) {
  _x2 += x_ - _x1;
  _x1 = x_;
}


void bt::Rect::setY(int y_)
{
  _y2 += y_ - _y1;
  _y1 = y_;
}


void bt::Rect::setPos(int x_, int y_) {
  _x2 += x_ - _x1;
  _x1 = x_;
  _y2 += y_ - _y1;
  _y1 = y_;
}


void bt::Rect::setWidth(unsigned int w) {
  _x2 = w + _x1 - 1;
}


void bt::Rect::setHeight(unsigned int h) {
  _y2 = h + _y1 - 1;
}


void bt::Rect::setSize(unsigned int w, unsigned int h) {
  _x2 = w + _x1 - 1;
  _y2 = h + _y1 - 1;
}


void bt::Rect::setRect(int x_, int y_, unsigned int w, unsigned int h) {
  *this = bt::Rect(x_, y_, w, h);
}


void bt::Rect::setCoords(int l, int t, int r, int b) {
  _x1 = l;
  _y1 = t;
  _x2 = r;
  _y2 = b;
}


bt::Rect bt::Rect::operator|(const bt::Rect &a) const {
  bt::Rect b;

  b._x1 = std::min(_x1, a._x1);
  b._y1 = std::min(_y1, a._y1);
  b._x2 = std::max(_x2, a._x2);
  b._y2 = std::max(_y2, a._y2);

  return b;
}


bt::Rect bt::Rect::operator&(const bt::Rect &a) const {
  bt::Rect b;

  b._x1 = std::max(_x1, a._x1);
  b._y1 = std::max(_y1, a._y1);
  b._x2 = std::min(_x2, a._x2);
  b._y2 = std::min(_y2, a._y2);

  return b;
}


bool bt::Rect::intersects(const bt::Rect &a) const {
  return std::max(_x1, a._x1) <= std::min(_x2, a._x2) &&
         std::max(_y1, a._y1) <= std::min(_y2, a._y2);
}


bool bt::Rect::contains(int x_, int y_) const {
  return x_ >= _x1 && x_ <= _x2 &&
         y_ >= _y1 && y_ <= _y2;
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
    setsid();
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
      ret = (char *) text_prop.value;
    } else {
      text_prop.nitems = strlen((char *) text_prop.value);

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
