// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Pen.cc for Blackbox - an X11 Window manager
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <stdio.h>
}

#include <algorithm>

#include "Pen.hh"
#include "Display.hh"
#include "Color.hh"
#include "Font.hh"
#include "Util.hh"


static const unsigned int cache_size    = 32u;
static const unsigned int cache_buckets =  8u;
static const unsigned int context_count = cache_size * cache_buckets;

/*
  The Pen cache is comprised of multiple arrays of cache buckets.  The
  bucket arrays are small to allow for fast lookups. A simple hash is
  used to determine which array is used.

  The 'cache_size' variable above controls the number of bucket
  arrays; the 'cache_buckets' variable controls the size of each
  bucket array.

  Each bucket stores a hit count, and the buckets are reordered at
  runtime from highest to lowest hit count.  When no suitable item can
  be found in the cache, the first bucket that is not in use will be
  used.

  A cache fault happens if all buckets are in use, at the cache will
  print a diagnostic message to 'stderr' and call abort().  Normally,
  a cache fault indicates that the cache was not big enough, and the
  variables above should be increased accordingly.

  --------------------------------------------------------------------

           >        +----+----+----+----+----+----+----+----+
           |        | 14 | 10 |  4 |  3 |  2 |  2 |  2 |  0 |
           |        +----+----+----+----+----+----+----+----+
  cache_size
           |        +----+----+----+----+----+----+----+----+
           |        |  9 |  8 |  7 |  6 |  4 |  3 |  0 |  0 |
           |        +----+----+----+----+----+----+----+----+
           |
           |        ... more bucket arrays ...
           >

                    ^------------- cache_buckets -----------^
*/


namespace bt {

  class PenCacheContext : public NoCopy {
  public:
    inline PenCacheContext(void)
      : _screen(~0u), _gc(0), _fontid(0ul), _function(0),
        _subwindow(0), _used(false) { }
    ~PenCacheContext(void);

    void set(const Color &color, const unsigned long fontid,
             const int function, const int subwindow);
    void set(const unsigned long fontid);

    unsigned int _screen;
    GC _gc;
    Color _color;
    unsigned long _fontid;
    int _function;
    int _subwindow;
    bool _used;
  };

  class PenCacheItem : public NoCopy {
  public:
    inline PenCacheItem(void) : _ctx(0), _count(0u), _hits(0u) { }
    inline const GC &gc(void) const { return _ctx->_gc; }

    PenCacheContext *_ctx;
    unsigned int _count;
    unsigned int _hits;
  };

#ifdef XFT
  class XftCacheContext : public NoCopy {
  public:
    inline XftCacheContext(void)
      : _screen(~0u), _drawable(None), _xftdraw(0), _used(false) { }
    ~XftCacheContext(void);

    void set(Drawable drawable);

    unsigned int _screen;
    Drawable _drawable;
    XftDraw *_xftdraw;
    bool _used;
  };

  class XftCacheItem : public NoCopy {
  public:
    inline XftCacheItem(void) : _ctx(0), _count(0u), _hits(0u) { }
    inline Drawable drawable(void) const { return _ctx->_drawable; }
    inline XftDraw *xftdraw(void) const { return _ctx->_xftdraw; }

    XftCacheContext *_ctx;
    unsigned int _count;
    unsigned int _hits;
  };
#endif

  class PenCache {
  public:
    PenCache(const Display &display);
    ~PenCache(void);

    // cleans up the cache
    void purge(void);

    PenCacheContext *contexts;
    PenCacheItem **cache;

    PenCacheContext *nextContext(unsigned int screen);
    void release(PenCacheContext *ctx);

    PenCacheItem *find(unsigned int screen,
                       const Color &color,
                       const unsigned long fontid,
                       int function,
                       int subwindow);
    void release(PenCacheItem *item);

#ifdef XFT
    XftCacheContext *xftcontexts;
    XftCacheItem **xftcache;

    XftCacheContext *nextXftContext(unsigned int screen);
    void release(XftCacheContext *ctx);

    XftCacheItem *findXft(unsigned int screen, Drawable drawable);
    void release(XftCacheItem *xftitem);
#endif

    const Display &_display;
    const unsigned int cache_total_size;
  };


  static PenCache *pencache = 0;


  void createPenCache(const Display &display) {
    pencache = new PenCache(display);
  }


  void destroyPenCache(void) {
    delete pencache;
    pencache = 0;
  }

} // namespace bt


bt::PenCacheContext::~PenCacheContext(void) {
  if (_gc) XFreeGC(pencache->_display.XDisplay(), _gc);
  _gc = 0;
}


void bt::PenCacheContext::set(const Color &color,
                             const unsigned long fontid,
                             const int function,
                             const int subwindow) {
  XGCValues gcv;
  _fontid = gcv.font = fontid;
  _color = color;
  gcv.foreground = _color.pixel(_screen);
  _function = gcv.function = function;
  _subwindow = gcv.subwindow_mode = subwindow;
  unsigned long mask = GCForeground | GCFunction | GCSubwindowMode;

  if (fontid) mask |= GCFont;

  XChangeGC(pencache->_display.XDisplay(), _gc, mask, &gcv);
}


void bt::PenCacheContext::set(const unsigned long fontid) {
  XGCValues gcv;
  _fontid = gcv.font = fontid;
  if (fontid) XChangeGC(pencache->_display.XDisplay(), _gc, GCFont, &gcv);
}


#ifdef XFT
bt::XftCacheContext::~XftCacheContext(void) {
  if (_xftdraw) XftDrawDestroy(_xftdraw);
  _xftdraw = 0;
}


void bt::XftCacheContext::set(Drawable drawable) {
  XftDrawChange(_xftdraw, drawable);
  _drawable = drawable;
}
#endif


bt::PenCache::PenCache(const Display &display)
  : _display(display),
    cache_total_size(context_count * _display.screenCount()) {
  unsigned int i;

  contexts = new PenCacheContext[cache_total_size];
  cache = new PenCacheItem*[cache_total_size];
  for (i = 0; i < cache_total_size; ++i) {
    cache[i] = new PenCacheItem;
  }

#ifdef XFT
  xftcontexts = new XftCacheContext[cache_total_size];
  xftcache = new XftCacheItem*[cache_total_size];
  for (i = 0; i < cache_total_size; ++i) {
    xftcache[i] = new XftCacheItem;
  }
#endif
}


bt::PenCache::~PenCache(void) {
  std::for_each(cache, cache + cache_total_size, PointerAssassin());
  delete [] cache;
  delete [] contexts;

#ifdef XFT
  std::for_each(xftcache, xftcache + cache_total_size, PointerAssassin());
  delete [] xftcache;
  delete [] xftcontexts;
#endif
}


void bt::PenCache::purge(void) {
  for (unsigned int i = 0; i < cache_total_size; ++i) {
    PenCacheItem *d = cache[ i ];

    if (d->_ctx && d->_count == 0) {
      release(d->_ctx);
      d->_ctx = 0;
    }
  }
}


bt::PenCacheContext *bt::PenCache::nextContext(unsigned int screen) {
  Window hd = pencache->_display.screenInfo(screen).rootWindow();

  PenCacheContext *c;
  unsigned int i;
  for (i = 0; i < cache_total_size; ++i) {
    c = contexts + i;

    if (! c->_gc) {
      c->_gc = XCreateGC(pencache->_display.XDisplay(), hd, 0, 0);
      c->_used = false;
      c->_screen = screen;
    }
    if (! c->_used && c->_screen == screen)
      return c;
  }

  fprintf(stderr, "bt::PenCache: context fault at %u of %u\n",
          i, cache_total_size);
  abort();
  return 0; // not reached
}


void bt::PenCache::release(PenCacheContext *ctx) {
  ctx->_used = false;
}


bt::PenCacheItem *bt::PenCache::find(unsigned int screen,
                                   const Color &color,
                                   const unsigned long fontid,
                                   int function,
                                   int subwindow) {
  int k = color.red() ^ color.green() ^ color.blue();
  k = (screen * context_count) + ((k % cache_size) * cache_buckets);
  unsigned int i = 0; // loop variable
  PenCacheItem *c = cache[ k ], *prev = 0;

  /*
    this will either loop cache_buckets times then return/abort or
    it will stop matching
  */
  while (c->_ctx &&
         (c->_ctx->_color != color || c->_ctx->_function != function ||
          c->_ctx->_subwindow != subwindow || c->_ctx->_screen != screen)) {
    if (i < (cache_buckets - 1)) {
      prev = c;
      c = cache[ ++k ];
      ++i;
      continue;
    }
    if (c->_count == 0 && c->_ctx->_screen == screen) {
      // printf("GC : k %03d hijack\n", k);
      // use this cache item
      c->_ctx->set(color, fontid, function, subwindow);
      c->_ctx->_used = true;
      c->_count = 1;
      c->_hits = 1;
      return c;
    }
    // cache fault!
    fprintf(stderr,
            "bt::PenCache: cache fault at %d\n"
            "      count: %d, screen: %d, item screen: %d\n",
            k, c->_count, screen, c->_ctx->_screen);
    abort();
  }

  if (c->_ctx) {
    // printf("GC : k %03d cache hit\n", k);
    // reuse existing context
    if (fontid && fontid != c->_ctx->_fontid)
      c->_ctx->set(fontid);
    c->_count++;
    c->_hits++;
    if (prev && c->_hits > prev->_hits) {
      cache[ k     ] = prev;
      cache[ k - 1 ] = c;
    }
  } else {
    // printf("GC : k %03d new context\n", k);
    c->_ctx = nextContext(screen);
    c->_ctx->set(color, fontid, function, subwindow);
    c->_ctx->_used = true;
    c->_count = 1;
    c->_hits = 1;
  }

  return c;
}


void bt::PenCache::release(PenCacheItem *item) {
  --item->_count;
}


#ifdef XFT
bt::XftCacheContext *bt::PenCache::nextXftContext(unsigned int screen) {
  const ScreenInfo &screeninfo = _display.screenInfo(screen);

  XftCacheContext *c;
  unsigned int i;
  for (i = 0; i < cache_total_size; ++i) {
    c = xftcontexts + i;

    if (! c->_xftdraw) {
      c->_xftdraw =
        XftDrawCreate(_display.XDisplay(), screeninfo.rootWindow(),
                      screeninfo.visual(), screeninfo.colormap());
      c->_used = false;
      c->_screen = screen;
    }
    if (! c->_used && c->_screen == screen)
      return c;
  }

  fprintf(stderr, "bt::PenCache: Xft context fault at %u of %u\n",
          i, cache_total_size);
  abort();
  return 0; // not reached
}


void bt::PenCache::release(XftCacheContext *context) {
  context->_used = false;
}


bt::XftCacheItem *bt::PenCache::findXft(unsigned int screen, Drawable drawable)
{
  int k = (screen * context_count) + ((drawable % cache_size) * cache_buckets);
  unsigned int i = 0; // loop variable
  XftCacheItem *c = xftcache[ k ], *prev = 0;

  /*
    this will either loop cache_buckets times then return/abort or
    it will stop matching
  */
  while (c->_ctx &&
         (c->_ctx->_drawable != drawable || c->_ctx->_screen != screen)) {
    if (i < (cache_buckets - 1)) {
      prev = c;
      c = xftcache[ ++k ];
      ++i;
      continue;
    }
    if (c->_count == 0 && c->_ctx->_screen == screen) {
      // printf("Xft: k %03d hijack\n", k);
      // use this cache item
      if (drawable != c->_ctx->_drawable)
        c->_ctx->set(drawable);
      c->_ctx->_used = true;
      c->_count = 1;
      c->_hits = 1;
      return c;
    }
    // cache fault... try
    fprintf(stderr,
            "bt::PenCache: Xft cache fault at %d\n"
            "      count: %d, screen: %d, item screen: %d\n",
            k, c->_count, screen, c->_ctx->_screen);
    abort();
  }

  if (c->_ctx) {
    // printf("Xft: k %03d cache hit\n", k);
    // reuse existing context
    if (drawable != c->_ctx->_drawable)
      c->_ctx->set(drawable);
    c->_count++;
    c->_hits++;
    if (prev && c->_hits > prev->_hits) {
      xftcache[ k     ] = prev;
      xftcache[ k - 1 ] = c;
    }
  } else {
    // printf("Xft: k %03d new context\n", k);
    c->_ctx = nextXftContext(screen);
    c->_ctx->set(drawable);
    c->_ctx->_used = true;
    c->_count = 1;
    c->_hits = 1;
  }

  return c;
}


void bt::PenCache::release(XftCacheItem *xftitem) {
  --xftitem->_count;
}
#endif


bt::Pen::Pen(unsigned int screen_, const Color &color_,
             int function, int subwindow)
  : _screen(screen_), _color(color_), _fontid(0ul),
    _function(function), _subwindow(subwindow), _item(0)
#ifdef XFT
    , _xftitem(0)
#endif
{ }


bt::Pen::~Pen(void) {
  if (_item) pencache->release(_item);
  _item = 0;

#ifdef XFT
  if (_xftitem) pencache->release(_xftitem);
  _xftitem = 0;
#endif
}


void bt::Pen::setFont(const Font &font) {
  if (_item) pencache->release(_item);
  _item = 0;

  _fontid = font.font() ? font.font()->fid : 0ul;
}


const bt::Display &bt::Pen::display(void) const {
  return pencache->_display;
}


const GC &bt::Pen::gc(void) const {
  if (! _item)
    _item = pencache->find(_screen, _color, _fontid, _function, _subwindow);
  return _item->gc();
}


#ifdef XFT
XftDraw *bt::Pen::xftDraw(Drawable drawable) const {
  if (_xftitem && _xftitem->drawable() != drawable) {
    pencache->release(_xftitem);
    _xftitem = 0;
  }
  if (! _xftitem) {
    _xftitem = pencache->findXft(_screen, drawable);
  }
  return _xftitem->xftdraw();
}
#endif

void bt::Pen::clearCache(void) { pencache->purge(); }
