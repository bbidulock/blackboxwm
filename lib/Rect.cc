// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Rect.cc for Blackbox - an X11 Window manager
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

#include "Rect.hh"

#include <algorithm>


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


void bt::Rect::setWidth(unsigned int w)
{ _x2 = w + _x1 - 1; }


void bt::Rect::setHeight(unsigned int h)
{ _y2 = h + _y1 - 1; }


void bt::Rect::setSize(unsigned int w, unsigned int h) {
  _x2 = w + _x1 - 1;
  _y2 = h + _y1 - 1;
}


void bt::Rect::setRect(int x_, int y_, unsigned int w, unsigned int h)
{ *this = bt::Rect(x_, y_, w, h); }


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
