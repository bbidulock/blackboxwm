// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// PixmapCache.hh for Blackbox - an X11 Window manager
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

#ifndef __PixmapCache_hh
#define __PixmapCache_hh

namespace bt {

  // forward declarations
  class Texture;

  class PixmapCache {
  public:
    // maximum amount of memory to store in server side pixmaps (in kb)
    // defaults to 2mb
    static unsigned long cacheLimit(void);
    // modify the above
    static void setCacheLimit(unsigned long limit);

    // current amount of memory used to store server side pixmaps (in kb)
    static unsigned long memoryUsage(void);

    static unsigned long find(unsigned int screen,
                              const Texture &texture,
                              unsigned int width, unsigned int height,
                              unsigned long old_pixmap = 0ul);
    static void release(unsigned long pixmap);

    static void clearCache(void);

  private:
    PixmapCache(); // static only interface
  };

} // namespace bt

#endif // __PixmapCache_hh
