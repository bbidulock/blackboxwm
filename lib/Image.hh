// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Image.hh for Blackbox - an X11 Window manager
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

#ifndef   __Image_hh
#define   __Image_hh

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
}

#include <list>
#include <vector>

#include "Color.hh"


namespace bt {

  class Display;
  class ScreenInfo;
  class Texture;
  class XColorTable;

  enum DitherMode { NoDither, OrderedDither, FloydSteinbergDither };

  struct RGB {
    unsigned int red      : 8;
    unsigned int green    : 8;
    unsigned int blue     : 8;
    unsigned int reserved : 8;
  };

  class Image {
  public:
    static unsigned int colorsPerChannel(void)
    { return global_colorsPerChannel; }
    static void setColorsPerChannel(unsigned int newval)
    { global_colorsPerChannel = newval > 2 ? newval < 6 ? newval : 6 : 2; }

    static DitherMode ditherMode(void)
    { return global_ditherMode; }
    static void setDitherMode(DitherMode dithermode)
    { global_ditherMode = dithermode; }

    Image(unsigned int w, unsigned int h);
    ~Image(void);

    Pixmap render(const Display &display, unsigned int screen,
                  const Texture &texture);

  private:
    RGB *data;
    unsigned int width, height;

    void OrderedDither(XColorTable *colortable,
                       unsigned int bit_depth,
                       unsigned int bytes_per_line,
                       unsigned char *pixel_data);
    void FloydSteinbergDither(XColorTable *colortable,
                              unsigned int bit_depth,
                              unsigned int bytes_per_line,
                              unsigned char *pixel_data);

    Pixmap renderPixmap(const Display &display, unsigned int screen);

    void bevel(unsigned int border_width = 0);
    void dgradient(const Color &from, const Color &to, bool interlaced);
    void egradient(const Color &from, const Color &to, bool interlaced);
    void hgradient(const Color &from, const Color &to, bool interlaced);
    void pgradient(const Color &from, const Color &to, bool interlaced);
    void rgradient(const Color &from, const Color &to, bool interlaced);
    void vgradient(const Color &from, const Color &to, bool interlaced);
    void cdgradient(const Color &from, const Color &to, bool interlaced);
    void pcgradient(const Color &from, const Color &to, bool interlaced);

    static unsigned int global_colorsPerChannel;
    static DitherMode global_ditherMode;
  };

} // namespace bt

#endif // __Image_hh
