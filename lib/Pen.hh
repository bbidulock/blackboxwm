// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Pen.hh for Blackbox - an X11 Window manager
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

#ifndef __Pen_hh
#define __Pen_hh

#include "Util.hh"

typedef struct _XftDraw XftDraw;

namespace bt {

  // forward declarations
  class Color;
  class Display;
  class PenCacheItem;
  class XftCacheItem;

  class Pen : public NoCopy {
  public:
    static void clearCache(void);

    Pen(unsigned int screen_, const Color &color_);
    ~Pen(void);

    inline unsigned int screen(void) const
    { return _screen; }
    inline const Color &color(void) const
    { return _color; }

    void setGCFunction(int function);
    void setLineWidth(int linewidth);
    void setSubWindowMode(int subwindow);

    ::Display *XDisplay(void) const;
    const Display &display(void) const;
    const GC &gc(void) const;

    XftDraw *xftDraw(Drawable drawable) const;

  private:
    unsigned int _screen;

    const Color &_color;
    int _function;
    int _linewidth;
    int _subwindow;

    mutable PenCacheItem *_item;
    mutable XftCacheItem *_xftitem;
  };

} // namespace bt

#endif // __Pen_hh
