// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
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

#ifndef _BLACKBOX_UTIL_HH
#define _BLACKBOX_UTIL_HH

class Rect {
public:
  Rect(void) : _x1(0), _y1(0), _x2(0), _y2(0) { }
  Rect(int a, int b, int c, int d) { setRect(a, b, c, d); }

  int left(void) const { return _x1; }
  int top(void) const { return _y1; }
  int right(void) const { return _x2; }
  int bottom(void) const { return _y2; }

  int x(void) const { return _x1; }
  void setX(int a) { _x1 = a; }

  int y(void) const { return _y1; }
  void setY(int b) { _y1 = b; }

  void setPos(int a, int b) { _x1 = a; _y1 = b; }

  int width(void) const { return _x2 - _x1 + 1; }
  void setWidth(int c) { _x2 = c + _x1 - 1; }

  int height(void) const { return _y2 - _y1 + 1; }
  void setHeight(int d) { _y2 = d + _y1 - 1; }

  void setSize(int a, int b) { setWidth(a); setHeight(b) ; }

  void setRect(int a, int b, int c, int d)
  { setPos(a, b); setSize(c, d); }

  void setCoords(int a, int b, int c, int d)
  { _x1 = a; _y1 = b; _x2 = c; _y2 = d; }

  bool operator==(const Rect &a)
  { return _x1 == a._x1 && _y1 == a._y1 && _x2 == a._x2 && _y2 == a._y2; }

  bool operator!=(const Rect &a)
  { return ! operator==(a); }

  Rect operator|(const Rect &a) const;
  Rect operator&(const Rect &a) const;
  Rect &operator|=(const Rect &a)
  { *this = *this | a; return *this; }
  Rect &operator&=(const Rect &a)
  { *this = *this & a; return *this; }

  bool intersects(const Rect &a) const;

private:
  int _x1, _y1, _x2, _y2;
};

#include <string>

/* XXX: this needs autoconf help */
const unsigned int BSENTINEL = 65535;

std::string expandTilde(const std::string& s);

void bexec(const std::string& command, const std::string& displaystring);

#ifndef   HAVE_BASENAME
std::string basename(const std::string& path);
#endif

struct timeval; // forward declare to avoid the header
timeval normalizeTimeval(const timeval &tm);

struct PointerAssassin {
  template<typename T>
  inline void operator()(const T ptr) const {
    delete ptr;
  }
};

#endif
