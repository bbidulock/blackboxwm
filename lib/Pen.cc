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

namespace bt {

  class GCCacheContext : public NoCopy {
  public:
    void set(const Color &color, const unsigned long fontid,
             const int function, const int subwindow);
    void set(const unsigned long fontid);

    ~GCCacheContext(void);

  private:
    GCCacheContext(const Display &display)
      : _display(display), _screen(~0u), _gc(0),
        _fontid(0ul), _function(0), _subwindow(0), _used(false) { }

    const Display &_display;
    unsigned int _screen;
    GC _gc;
    Color _color;
    unsigned long _fontid;
    int _function;
    int _subwindow;
    bool _used;

    friend class GCCache;
    friend class GCCacheItem;
  };

  class GCCacheItem : public NoCopy {
  public:
    inline const Display &display(void) const { return _ctx->_display;}
    inline const GC &gc(void) const { return _ctx->_gc; }

  private:
    GCCacheItem(void) : _ctx(0), _count(0), _hits(0), _fault(false) { }

    GCCacheContext *_ctx;
    unsigned int _count;
    unsigned int _hits;
    bool _fault;

    friend class GCCache;
  };

  class GCCache {
  public:
    GCCache(const Display &display);
    ~GCCache(void);

    // cleans up the cache
    void purge(void);

    GCCacheItem *find(unsigned int screen,
                      const Color &color,
                      const unsigned long fontid = 0,
                      int function = GXcopy,
                      int subwindow = ClipByChildren);
    void release(GCCacheItem *item);

  private:
    GCCacheContext *nextContext(unsigned int screen);
    void release(GCCacheContext *ctx);

    // this is closely modelled after the Qt GC cache, but with some of the
    // complexity stripped out
    const Display &_display;

    const unsigned int context_count;
    const unsigned int cache_size;
    const unsigned int cache_buckets;
    const unsigned int cache_total_size;
    GCCacheContext **contexts;
    GCCacheItem **cache;
  };


  static GCCache *pencache = 0;


  void createPenCache(const Display &display) {
    pencache = new GCCache(display);
  }


  void destroyPenCache(void) {
    delete pencache;
    pencache = 0;
  }

} // namespace bt


bt::GCCacheContext::~GCCacheContext(void) {
  if (_gc) XFreeGC(_display.XDisplay(), _gc);
}


void bt::GCCacheContext::set(const Color &color,
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

  XChangeGC(_display.XDisplay(), _gc, mask, &gcv);
}


void bt::GCCacheContext::set(const unsigned long fontid) {
  XGCValues gcv;
  _fontid = gcv.font = fontid;
  if (fontid) XChangeGC(_display.XDisplay(), _gc, GCFont, &gcv);
}


bt::GCCache::GCCache(const Display &display)
  : _display(display),  context_count(128u), cache_size(16u),
    cache_buckets(8u * _display.screenCount()),
    cache_total_size(cache_size * cache_buckets) {
  contexts = new GCCacheContext*[context_count];
  unsigned int i;
  for (i = 0; i < context_count; i++) {
    contexts[i] = new GCCacheContext(_display);
  }

  cache = new GCCacheItem*[cache_total_size];
  for (i = 0; i < cache_total_size; ++i) {
    cache[i] = new GCCacheItem;
  }
}


bt::GCCache::~GCCache(void) {
  std::for_each(contexts, contexts + context_count, PointerAssassin());
  std::for_each(cache, cache + cache_total_size, PointerAssassin());
  delete [] cache;
  delete [] contexts;
}


bt::GCCacheContext *bt::GCCache::nextContext(unsigned int screen) {
  Window hd = _display.screenInfo(screen).rootWindow();

  GCCacheContext *c;

  for (unsigned int i = 0; i < context_count; ++i) {
    c = contexts[i];

    if (! c->_gc) {
      c->_gc = XCreateGC(_display.XDisplay(), hd, 0, 0);
      c->_used = false;
      c->_screen = screen;
    }
    if (! c->_used && c->_screen == screen)
      return c;
  }

  fprintf(stderr, "bt::GCCache: context fault!\n");
  abort();
  return 0; // not reached
}


void bt::GCCache::release(GCCacheContext *ctx) {
  ctx->_used = false;
}


bt::GCCacheItem *bt::GCCache::find(unsigned int screen,
                                   const Color &color,
                                   const unsigned long fontid,
                                   int function,
                                   int subwindow) {
  int k = color.red() ^ color.green() ^ color.blue();
  k = (k % cache_size) * cache_buckets;
  unsigned int i = 0; // loop variable
  GCCacheItem *c = cache[ k ], *prev = 0;

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
      // use this cache item
      c->_ctx->set(color, fontid, function, subwindow);
      c->_ctx->_used = true;
      c->_count = 1;
      c->_hits = 1;
      return c;
    }
    // cache fault!
    fprintf(stderr,
            "bt::GCCache: cache fault\n"
            "      count: %d, screen: %d, item screen: %d\n",
            c->_count, screen, c->_ctx->_screen);
    abort();
  }

  if (c->_ctx) {
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
    c->_ctx = nextContext(screen);
    c->_ctx->set(color, fontid, function, subwindow);
    c->_ctx->_used = true;
    c->_count = 1;
    c->_hits = 1;
  }

  return c;
}


void bt::GCCache::release(GCCacheItem *item) {
  item->_count--;
}


void bt::GCCache::purge(void) {
  for (unsigned int i = 0; i < cache_total_size; ++i) {
    GCCacheItem *d = cache[ i ];

    if (d->_ctx && d->_count == 0) {
      release(d->_ctx);
      d->_ctx = 0;
    }
  }
}


bt::Pen::Pen(unsigned int screen, const Color &color,
             int function, int subwindow)
  : _screen(screen), _color(color), _fontid(0ul),
    _function(function), _subwindow(subwindow), _item(0) { }


bt::Pen::~Pen(void) {
  if (_item) pencache->release(_item);
  _item = 0;
}


void bt::Pen::setFont(const Font &font) {
  if (_item) pencache->release(_item);
  _item = 0;
  _fontid = font.font() ? font.font()->fid : 0ul;
}


const bt::Display &bt::Pen::display(void) const {
  if (! _item)
    _item = pencache->find(_screen, _color, _fontid, _function, _subwindow);
  return _item->display();
}


const GC &bt::Pen::gc(void) const {
  if (! _item)
    _item = pencache->find(_screen, _color, _fontid, _function, _subwindow);
  return _item->gc();
}


void bt::Pen::clearCache(void) { pencache->purge(); }
