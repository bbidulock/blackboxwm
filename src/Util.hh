// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Util.hh for Blackbox - an X11 Window manager
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

#ifndef UTIL_HH
#define UTIL_HH

#include <algorithm>
#include <string>
using std::string;

class Point
{
public:
  Point() : _x( 0 ), _y( 0 ) { }
  Point( int a, int b ) : _x( a ), _y( b ) { }
  Point( const Point &a ) : _x( a._x ), _y( a._y ) { }

  int x() const { return _x; }
  void setX( int a ) { _x = a; }

  int y() const { return _y; }
  void setY( int b ) { _y = b; }

  Point &operator=( const Point &a )
  { _x = a._x; _y = a._y; return *this; }

  Point operator+( const Point &a )
  { return Point( _x + a._x, _y + a._y ); }

  Point operator-( const Point &a )
  { return Point( _x - a._x, _y - a._y ); }

  Point &operator+=( const Point &a )
  { _x += a._x; _y += a._y; return *this; }

  Point &operator-=( const Point &a )
  { _x -= a._x; _y -= a._y; return *this; }

  bool operator==( const Point &a )
  { return _x == a._x && _y == a._y; }

  bool operator!=( const Point &a )
  { return ! operator==( a ); }

private:
  int _x, _y;
};

class Size
{
public:
  Size() : _w( 1 ), _h( 1 ) { }
  Size( int a, int b ) : _w( a ), _h( b ) { }
  Size( const Size &a ) : _w( a._w ), _h( a._h ) { }

  int width() const { return _w; }
  void setWidth( int a ) { _w = a; }

  int height() const { return _h; }
  void setHeight( int b ) { _h = b; }

  Size &operator=( const Size &a )
  { _w = a._w; _h = a._h; return *this; }

  Size operator+( const Size &a )
  { return Size( _w + a._w, _h + a._h ); }

  Size operator-( const Size &a )
  { return Size( _w - a._w, _h + a._h ); }

  Size &operator+=( const Size &a )
  { _w += a._w; _h += a._h; return *this; }

  Size &operator-=( const Size &a )
  { _w -= a._w; _h -= a._h; return *this; }

  bool operator==( const Size &a )
  { return _w == a._w && _h == a._h; }

  bool operator!=( const Size &a )
  { return ! operator==( a ); }

private:
  int _w, _h;
};


class Rect
{
public:
  Rect() : _x1( 0 ), _y1( 0 ), _x2( 0 ), _y2( 0 ) { }
  Rect( int a, int b, int c, int d ) { setRect( a, b, c, d ); }
  Rect( const Point &a, const Size &b ) { setRect( a, b ); }
  Rect( const Rect &a ) { *this = a; }

  int x() const { return _x1; }
  void setX( int a ) { _x1 = a; }

  int y() const { return _y1; }
  void setY( int b ) { _y1 = b; }

  Point pos() const { return Point( x(), y() ); }
  void setPos( int a, int b ) { _x1 = a; _y1 = b; }
  void setPos( const Point &e ) { _x1 = e.x(); _y1 = e.y(); }

  int width() const { return _x2 - _x1 + 1; }
  void setWidth( int c ) { _x2 = c + _x1 - 1; }

  int height() const { return _y2 - _y1 + 1; }
  void setHeight( int d ) { _y2 = d + _y1 - 1; }

  Size size() const { return Size( width(), height() ); }
  void setSize( int a, int b ) { setWidth( a ); setHeight( b ) ; }
  void setSize( const Size &f )
  { setWidth( f.width() ); setHeight( f.height() ) ; }

  void setRect( int a, int b, int c, int d )
  { setPos( a, b ); setSize( c, d ); }

  void setRect( const Point &p, const Size &s )
  { setPos( p ); setSize( s ); }

  bool operator==( const Rect &a )
  { return _x1 == a._x1 && _y1 == a._y1 && _x2 == a._x2 && _y2 == a._y2; }

  bool operator!=( const Rect &a )
  { return ! operator==( a ); }

  Rect &operator=( const Rect &a )
  { _x1 = a._x1; _y1 = a._y1; _x2 = a._x2; _y2 = a._y2; return *this; }
  Rect operator|( const Rect &a ) const
  {
    Rect b;
    b._x1 = std::min( _x1, a._x1 );
    b._y1 = std::min( _y1, a._y1 );
    b._x2 = std::max( _x2, a._x2 );
    b._y2 = std::max( _y2, a._y2 );
    return b;
  }
  Rect operator&( const Rect &a ) const
  {
    Rect b;
    b._x1 = std::max( _x1, a._x1 );
    b._y1 = std::max( _y1, a._y1 );
    b._x2 = std::min( _x2, a._x2 );
    b._y2 = std::min( _y2, a._y2 );
    return b;
  }
  Rect &operator|=( const Rect &a )
  { *this = *this | a; return *this; }
  Rect &operator&=( const Rect &a )
  { *this = *this & a; return *this; }

  bool contains( const Point &p ) const
  { return p.x() >= _x1 && p.x() <= _x2 &&
           p.y() >= _y1 && p.y() <= _y2; }

  bool intersects( const Rect &a ) const
  { return std::max( _x1, a._x1 ) <= std::min( _x2, a._x2 ) &&
           std::max( _y1, a._y1 ) <= std::min( _y2, a._y2 ); }

private:
  int _x1, _y1, _x2, _y2;
};

// some string functions
char* expandTilde(const char *s);
char* bstrdup(const char *);
void bexec(const string &, int = -1);

struct timeval; // forward declare to avoid the header
timeval normalizeTimeval(const timeval &tm);

#endif // UTIL_HH
