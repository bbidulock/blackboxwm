// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// GCCache.hh for Blackbox - an X11 Window manager
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

#ifndef GCCACHE_HH
#define GCCACHE_HH

#include "BaseDisplay.hh"
#include "Color.hh"

namespace bt {

  class GCCacheItem;

  class GCCacheContext {
  public:
    void set(const bt::Color &_color, const XFontStruct * const _font,
             const int _function, const int _subwindow);
    void set(const XFontStruct * const _font);

    ~GCCacheContext(void);

  private:
    GCCacheContext(const bt::Display * const _display)
      : display(_display), gc(0), pixel(0ul), fontid(0ul),
        function(0), subwindow(0), used(false), screen(~(0u)) {}

    const bt::Display *display;
    GC gc;
    unsigned long pixel;
    unsigned long fontid;
    int function;
    int subwindow;
    bool used;
    unsigned int screen;

    GCCacheContext(const GCCacheContext &_nocopy);
    GCCacheContext &operator=(const GCCacheContext &_nocopy);

    friend class GCCache;
    friend class GCCacheItem;
  };

  class GCCacheItem {
  public:
    inline const GC &gc(void) const { return ctx->gc; }

  private:
    GCCacheItem(void) : ctx(0), count(0), hits(0), fault(false) { }

    GCCacheContext *ctx;
    unsigned int count;
    unsigned int hits;
    bool fault;

    GCCacheItem(const GCCacheItem &_nocopy);
    GCCacheItem &operator=(const GCCacheItem &_nocopy);

    friend class GCCache;
  };

  class GCCache {
  public:
    GCCache(const Display * const _display, unsigned int screen_count);
    ~GCCache(void);

    // cleans up the cache
    void purge(void);

    GCCacheItem *find(const bt::Color &_color,
                      const XFontStruct * const _font = 0,
                      int _function = GXcopy,
                      int _subwindow = ClipByChildren);
    void release(GCCacheItem *_item);

  private:
    GCCacheContext *nextContext(unsigned int _screen);
    void release(GCCacheContext *ctx);

    // this is closely modelled after the Qt GC cache, but with some of the
    // complexity stripped out
    const Display *display;

    const unsigned int context_count;
    const unsigned int cache_size;
    const unsigned int cache_buckets;
    const unsigned int cache_total_size;
    GCCacheContext **contexts;
    GCCacheItem **cache;
  };

  class Pen {
  public:
    inline Pen(const bt::Color &_color,
               const XFontStruct * const _font = 0,
               int _function = GXcopy,
               int _subwindow = ClipByChildren)
      : color(_color), font(_font), function(_function), subwindow(_subwindow),
        cache(_color.display()->gcCache()), item(0) { }
    inline ~Pen(void) { if (item) cache->release(item); }

    inline const GC &gc(void) const {
      if (! item) item = cache->find(color, font, function, subwindow);
      return item->gc();
    }

  private:
    const bt::Color &color;
    const XFontStruct *font;
    int function;
    int subwindow;

    mutable GCCache *cache;
    mutable GCCacheItem *item;
  };

} // namespace bt

#endif // GCCACHE_HH
