// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Color.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh at debian.org>
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

#ifndef __Color_hh
#define __Color_hh

#include <string>

namespace bt {

  // forward declarations
  class Display;
  class PenCache;

  /*
    The color object.  Colors are stored in rgb format (screen
    independent).  Screen dependent pixel values can be obtained using
    the pixel() function.
  */
  class Color {
  public:
    /*
      Frees unused colors on all screens.
     */
    static void clearCache(void);

    /*
      Returns the named color on the specified display and screen.  If
      the color couldn't be found, an invalid color is returned.
    */
    static Color namedColor(const Display &display, unsigned int screen,
                            const std::string &colorname);

    explicit inline Color(int r = -1, int g = -1, int b = -1)
      : _red(r), _green(g), _blue(b),
        _screen(~0u), _pixel(0ul)
    { }
    inline Color(const Color &c)
      : _red(c._red), _green(c._green), _blue(c._blue),
        _screen(~0u), _pixel(0ul)
    { }
    inline ~Color(void)
    { deallocate(); }

    inline int   red(void) const
    { return _red; }
    inline int green(void) const
    { return _green; }
    inline int  blue(void) const
    { return _blue; }
    inline void setRGB(int r, int g, int b)
    { deallocate(); _red = r; _green = g; _blue = b; }

    unsigned long pixel(unsigned int screen) const;

    inline bool valid(void) const
    { return _red != -1 && _green != -1 && _blue != -1; }

    // operators
    inline Color &operator=(const Color &c)
    { setRGB(c._red, c._green, c._blue); return *this; }
    inline bool operator==(const Color &c) const
    { return _red == c._red && _green == c._green && _blue == c._blue; }
    inline bool operator!=(const Color &c) const
    { return !operator==(c); }

  private:
    void deallocate(void);

    int _red, _green, _blue;
    mutable unsigned int _screen;
    mutable unsigned long _pixel;

    friend class PenCache;
  };

} // namespace bt

#endif // __Color_hh
