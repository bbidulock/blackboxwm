// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Bitmap.hh for Blackbox - an X11 Window manager
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

#ifndef __Bitmap_hh
#define __Bitmap_hh

#include "Util.hh"

namespace bt {

  // forward declarations
  class Bitmap;
  class Display;
  class Pen;
  class Rect;

  /*
    Draws the specified bitmap with the specified pen on the specified
    drawable in the specified area.  Note: the bitmap will be drawn
    using the color of the pen.
   */
  void drawBitmap(const Bitmap &bitmap, const Pen &pen,
                  ::Drawable drawable, const Rect &rect);

  /*
    The bitmap object.  You can create custom bitmaps from raw data,
    or use one of 5 standard bitmaps.
   */
  class Bitmap : public NoCopy {
  public:
    // standard bitmaps
    static const Bitmap &leftArrow(unsigned int screen);
    static const Bitmap &rightArrow(unsigned int screen);
    static const Bitmap &upArrow(unsigned int screen);
    static const Bitmap &downArrow(unsigned int screen);
    static const Bitmap &checkMark(unsigned int screen);

    inline Bitmap(void)
      : _screen(~0u), _drawable(0ul), _width(0u), _height(0u)
    { }
    Bitmap(unsigned int scr, const unsigned char *data,
           unsigned int w, unsigned int h);
    ~Bitmap(void);

    bool load(unsigned int scr, const unsigned char *data,
              unsigned int w, unsigned int h);

    inline unsigned int screen(void) const
    { return _screen; }
    inline ::Drawable drawable(void) const
    { return _drawable; }
    inline unsigned int width(void) const
    { return _width; }
    inline unsigned int height(void) const
    { return _height; }

  private:
    unsigned int _screen;
    ::Drawable _drawable;
    unsigned int _width, _height;
  };

} // namespace bt

#endif // __Bitmap_hh
