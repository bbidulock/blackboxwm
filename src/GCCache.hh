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

#include <X11/Xlib.h>

#include "BaseDisplay.hh"
#include "Color.hh"

class BGCCache {
private:
  class Context {
  public:
    Context(BaseDisplay *_display)
      : display(_display), gc(0), pixel(0ul), fontid(0ul),
        function(0), subwindow(0), used(false), screen(~(0u))
    {
    }

    void set(const BColor &_color, const XFontStruct * const _font,
             const int _function, const int _subwindow);
    void set(const XFontStruct * const _font);

    BaseDisplay *display;
    GC gc;
    unsigned long pixel;
    unsigned long fontid;
    int function;
    int subwindow;
    bool used;
    unsigned int screen;

  private:
    Context(const Context &_nocopy);
    Context &operator=(const Context &_nocopy);
  };

  Context *nextContext(unsigned int _screen);
  void release(Context *ctx);

public:
  BGCCache(BaseDisplay *_display);
  ~BGCCache(void);

  // cleans up the cache
  void purge(void);

  class Item {
  public:
    Item(void) : ctx(0), count(0), hits(0), fault(false) { }
    inline const GC &gc(void) const { return ctx->gc; }

    Context *ctx;
    int count;
    int hits;
    bool fault;

  private:
    Item(const Item &_nocopy);
    Item &operator=(const Item &_nocopy);
  };

  Item *find(const BColor &_color, const XFontStruct * const _font = 0,
             int _function = GXcopy, int _subwindow = ClipByChildren);
  void release(Item *_item);

private:
  // this is closely modelled after the Qt GC cache, but with some of the
  // complexity stripped out
  BaseDisplay *display;
  const int context_count;
  Context *contexts;
  const int cache_size;
  const int cache_buckets;
  Item **cache;
};



class BPen {
public:
  inline BPen(const BColor &_color,  const XFontStruct * const _font = 0,
              int _function = GXcopy, int _subwindow = ClipByChildren)
    : color(_color), font(_font), function(_function), subwindow(_subwindow),
      cache(0), item(0) { }
  inline ~BPen(void) { if (item) cache->release(item); }

  inline const GC &gc() const {
    if (! cache) cache = color.display()->gcCache();
    if (! item) item = cache->find(color, font, function, subwindow);
    return item->gc();
  }

private:
  const BColor &color;
  const XFontStruct *font;
  int function;
  int subwindow;

  mutable BGCCache *cache;
  mutable BGCCache::Item *item;
};


#endif // GCCACHE_HH
