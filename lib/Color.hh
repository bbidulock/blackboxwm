// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Color.hh for Blackbox - an X11 Window manager
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

#ifndef COLOR_HH
#define COLOR_HH

#include <map>
#include <string>


namespace bt {

  class Display;
  class ColorCache;

  class Color {
  public:
    static void clearColorCache(void);

    static Color namedColor(const Display &display, unsigned int screen,
                            const std::string &colorname);

    Color(int r = -1, int g = -1, int b = -1);
    Color(const Color &c);
    ~Color(void);

    int   red(void) const { return _red; }
    int green(void) const { return _green; }
    int  blue(void) const { return _blue; }
    void setRGB(int r, int g, int b) {
      deallocate();
      _red   = r;
      _green = g;
      _blue  = b;
    }

    unsigned long pixel(const Display &display, unsigned int screen) const;

    bool allocated(void) const { return _screen != ~0u; }
    bool valid(void) const
    { return _red != -1 && _green != -1 && _blue != -1; }

    // operators
    Color &operator=(const Color &c)
    { setRGB(c._red, c._green, c._blue); return *this; }
    bool operator==(const Color &c) const
    { return _red == c._red && _green == c._green && _blue == c._blue; }
    bool operator!=(const Color &c) const
    { return !operator==(c); }

  private:
    void deallocate(void);

    int _red, _green, _blue;
    mutable unsigned int _screen;
    mutable unsigned long _pixel;

    static ColorCache *colorcache;
  };

} // namespace bt

#endif // COLOR_HH
