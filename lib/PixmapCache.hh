// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// PixmapCache.hh for Blackbox - an X11 Window manager
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

#ifndef __PixmapCache_hh
#define __PixmapCache_hh

#include "Util.hh"

namespace bt {

  // forward declarations
  class Texture;

  class PixmapCache : public NoCopy {
  public:
    /*
      Returns the cache limit in kilobytes.  The default is one
      megabyte (1024 kilobytes).

      When this limit is reached, the cache will automatically free
      unused pixmaps to reduce the amount of server side memory used.

      NOTE: This limit is not a hard limit.  The cache will never free
      a pixmap that is in use.  This means it is possible that the
      PixmapCache memory usage can exceed this limit.
    */
    static unsigned long cacheLimit(void);

    /*
      Set the cache limit to the specified value (in kilobytes).
    */
    static void setCacheLimit(unsigned long limit);

    /*
      Returns the current amount of memory in kilobytes used by the X
      server for the pixmaps in the cache.
    */
    static unsigned long memoryUsage(void);

    /*
      Returns a pixmap matching the specified texture and size on the
      specified screen.  The pixmap will be rendered if necessary.

      If old_pixmap is non-zero, PixmapCache::release(old_pixmap) will
      be called immediately after finding the requested pixmap.
    */
    static Pixmap find(unsigned int screen,
                       const Texture &texture,
                       unsigned int width, unsigned int height,
                       Pixmap old_pixmap = 0ul);

    /*
      Indicates that the specified pixmap is no longer needed.
      Pixmaps contained in the cache are reference counted, so you
      should call this function once for each time you call
      PixmapCache::find().
    */
    static void release(Pixmap pixmap);

    /*
      Free all unused pixmaps in the cache.
    */
    static void clearCache(void);

  private:
    // static only interface
    PixmapCache();
  };

} // namespace bt

#endif // __PixmapCache_hh
