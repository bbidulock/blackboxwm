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

#include <X11/Xlib.h>

class Rect {
public:
  Rect(void) : _x1(0), _y1(0), _x2(0), _y2(0) { }
  Rect(int __x, int __y, int __w, int __h)
  { setRect(__x, __h, __w, __h); }
  explicit Rect(const XRectangle& xrect)
  { setRect(xrect.x, xrect.y, xrect.width, xrect.height); }

  int left(void) const { return _x1; }
  int top(void) const { return _y1; }
  int right(void) const { return _x2; }
  int bottom(void) const { return _y2; }

  int x(void) const { return _x1; }
  void setX(int __x) { _x2 += __x - _x1; _x1 = __x; }

  int y(void) const { return _y1; }
  void setY(int __y) { _y2 += __y - _y1; _y1 = __y; }

  void setPos(int __x, int __y)
  { _x2 += __x - _x1; _x1 = __x; _y2 += __y - _y1; _y1 = __y; }

  int width(void) const { return _x2 - _x1 + 1; }
  void setWidth(int __w) { _x2 = __w + _x1 - 1; }

  int height(void) const { return _y2 - _y1 + 1; }
  void setHeight(int __h) { _y2 = __h + _y1 - 1; }

  void setSize(int __w, int __h)
  { setWidth(__w); setHeight(__h); }

  void setRect(int __x, int __y, int __w, int __h)
  { setPos(__x, __y); setSize(__w, __h); }

  void setCoords(int __l, int __t, int __r, int __b)
  { _x1 = __l; _y1 = __t; _x2 = __r; _y2 = __b; }

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
