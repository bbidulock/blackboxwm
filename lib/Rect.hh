// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Rect.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2005
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

#ifndef __Rect_hh
#define __Rect_hh

namespace bt {

  class Rect {
  public:
    inline Rect(void)
      : _x1(0), _y1(0), _x2(0), _y2(0)
    { }
    inline Rect(int x_, int y_, unsigned int w, unsigned int h)
      : _x1(x_), _y1(y_), _x2(w + x_ - 1), _y2(h + y_ - 1)
    { }

    inline int left(void) const
    { return _x1; }
    inline int top(void) const
    { return _y1; }
    inline int right(void) const
    { return _x2; }
    inline int bottom(void) const
    { return _y2; }

    inline int x(void) const
    { return _x1; }
    inline int y(void) const
    { return _y1; }
    void setX(int x_);
    void setY(int y_);
    void setPos(int x_, int y_);

    inline unsigned int width(void) const
    { return _x2 - _x1 + 1; }
    inline unsigned int height(void) const
    { return _y2 - _y1 + 1; }
    void setWidth(unsigned int w);
    void setHeight(unsigned int h);
    void setSize(unsigned int w, unsigned int h);

    void setRect(int x_, int y_, unsigned int w, unsigned int h);

    void setCoords(int l, int t, int r, int b);

    inline bool operator==(const Rect &a) const
    { return _x1 == a._x1 && _y1 == a._y1 && _x2 == a._x2 && _y2 == a._y2; }
    inline bool operator!=(const Rect &a) const
    { return (!operator==(a)); }

    Rect operator|(const Rect &a) const;
    Rect operator&(const Rect &a) const;
    inline Rect &operator|=(const Rect &a)
    { *this = *this | a; return *this; }
    inline Rect &operator&=(const Rect &a)
    { *this = *this & a; return *this; }

    inline bool valid(void) const
    { return _x2 > _x1 && _y2 > _y1; }

    bool intersects(const Rect &a) const;
    bool contains(int x_, int y_) const;

    Rect inside(const Rect &a) const;

  private:
    int _x1, _y1, _x2, _y2;
  };

} // namespace bt

#endif // __Rect_hh
