// Color.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2002 Brad Hughes (bhughes@trolltech.com)
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

#ifndef COLOR_HH
#define COLOR_HH

#include <X11/Xlib.h>

#include <map>
#include <string>
using std::string;


class BColor {
public:
  BColor( int scr = -1 );
  BColor( int rr, int gg, int bb, int scr = -1 );
  BColor( const string &name, int scr = -1 );
  ~BColor();

  const string &name() const { return colorname; }

  int   red(void) const { return r; }
  int green(void) const { return g; }
  int  blue(void) const { return b; }
  void setRGB( int rr, int gg, int bb )
  {
    deallocate();
    r = rr;
    g = gg;
    b = bb;
  }

  int screen() const { return scrn; }
  void setScreen( int scr );

  bool isAllocated(void) const { return allocated; }

  bool isValid() const
  {
    return r != -1 && g != -1 && b != -1;
  }

  unsigned long pixel(void) const;

  const GC &gc() const;

  // operators
  BColor &operator=( const BColor &c );
  bool operator==( const BColor &c ) const
  {
    return ( r == c.r && b == c.b && b == c.b );
  }
  bool operator!=( const BColor &c ) const
  {
    return ( ! operator==( c ) );
  }

  static void cleanupColorCache();

private:
  void parseColorName();
  void allocate();
  void deallocate();

  bool allocated;
  int r, g, b;
  unsigned long p;
  int scrn;
  string colorname;

  // global color allocator/deallocator
  struct rgb {
    int r, g, b, screen;
    rgb() : r( -1 ), g( -1 ), b( -1 ), screen( -1 ) { }
    rgb( int x, int y, int z, int q ) : r( x ), g( y ), b( z ), screen( q ) { }
    rgb( const rgb &x ) : r( x.r ), g( x.g ), b( x.b ), screen( x.screen ) { }
    bool operator==( const rgb &x ) const
    {
      return r == x.r && g == x.g && b == x.b && screen == x.screen;
    }
    bool operator<( const rgb &x ) const
    {
      unsigned long p1, p2;
      p1 = ( r << 16 | g << 8 | b ) & 0x00ffffff;
      p2 = ( x.r << 16 | x.g << 8 | x.b) & 0x00ffffff;
      return p1 < p2;
    }
  };
  struct pixelref {
    unsigned long p;
    int count;
    pixelref() : p( 0 ), count( 0 ) { }
    pixelref( unsigned long x ) : p( x ), count( 1 ) { }
  };
  typedef std::map<rgb,pixelref> ColorCache;
  static ColorCache colorcache;
  static bool cleancache;
  static void doCacheCleanup();
};

#endif // COLOR_HH
