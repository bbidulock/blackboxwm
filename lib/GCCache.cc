// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// GCCache.cc for Blackbox - an X11 Window manager
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <stdio.h>
}

#include "GCCache.hh"
#include "BaseDisplay.hh"
#include "Color.hh"
#include "Util.hh"


bt::GCCacheContext::~GCCacheContext(void) {
  if (gc)
    XFreeGC(display->XDisplay(), gc);
}


void bt::GCCacheContext::set(const bt::Color &_color,
                             const XFontStruct * const _font,
                             const int _function, const int _subwindow) {
  XGCValues gcv;
  pixel = gcv.foreground = _color.pixel();
  function = gcv.function = _function;
  subwindow = gcv.subwindow_mode = _subwindow;
  unsigned long mask = GCForeground | GCFunction | GCSubwindowMode;

  if (_font) {
    fontid = gcv.font = _font->fid;
    mask |= GCFont;
  } else {
    fontid = 0;
  }

  XChangeGC(display->XDisplay(), gc, mask, &gcv);
}


void bt::GCCacheContext::set(const XFontStruct * const _font) {
  if (! _font) {
    fontid = 0;
    return;
  }

  XGCValues gcv;
  fontid = gcv.font = _font->fid;
  XChangeGC(display->XDisplay(), gc, GCFont, &gcv);
}


bt::GCCache::GCCache(const bt::Display * const _display,
                     unsigned int screen_count)
  : display(_display),  context_count(128u),
    cache_size(16u), cache_buckets(8u * screen_count),
    cache_total_size(cache_size * cache_buckets) {

  contexts = new bt::GCCacheContext*[context_count];
  unsigned int i;
  for (i = 0; i < context_count; i++) {
    contexts[i] = new bt::GCCacheContext(display);
  }

  cache = new bt::GCCacheItem*[cache_total_size];
  for (i = 0; i < cache_total_size; ++i) {
    cache[i] = new bt::GCCacheItem;
  }
}


bt::GCCache::~GCCache(void) {
  std::for_each(contexts, contexts + context_count, bt::PointerAssassin());
  std::for_each(cache, cache + cache_total_size, bt::PointerAssassin());
  delete [] cache;
  delete [] contexts;
}


bt::GCCacheContext *bt::GCCache::nextContext(unsigned int scr) {
  Window hd = display->screenNumber(scr)->getRootWindow();

  bt::GCCacheContext *c;

  for (unsigned int i = 0; i < context_count; ++i) {
    c = contexts[i];

    if (! c->gc) {
      c->gc = XCreateGC(display->XDisplay(), hd, 0, 0);
      c->used = false;
      c->screen = scr;
    }
    if (! c->used && c->screen == scr)
      return c;
  }

  fprintf(stderr, "bt::GCCache: context fault!\n");
  abort();
  return (bt::GCCacheContext*) 0; // not reached
}


void bt::GCCache::release(bt::GCCacheContext *ctx) {
  ctx->used = false;
}


bt::GCCacheItem *bt::GCCache::find(const bt::Color &_color,
                                   const XFontStruct * const _font,
                                   int _function,
                                   int _subwindow) {
  const unsigned long pixel = _color.pixel();
  const unsigned int screen = _color.screen();
  const int key = _color.red() ^ _color.green() ^ _color.blue();
  int k = (key % cache_size) * cache_buckets;
  unsigned int i = 0; // loop variable
  bt::GCCacheItem *c = cache[ k ], *prev = 0;

  /*
    this will either loop cache_buckets times then return/abort or
    it will stop matching
  */
  while (c->ctx &&
         (c->ctx->pixel != pixel || c->ctx->function != _function ||
          c->ctx->subwindow != _subwindow || c->ctx->screen != screen)) {
    if (i < (cache_buckets - 1)) {
      prev = c;
      c = cache[ ++k ];
      ++i;
      continue;
    }
    if (c->count == 0 && c->ctx->screen == screen) {
      // use this cache item
      c->ctx->set(_color, _font, _function, _subwindow);
      c->ctx->used = true;
      c->count = 1;
      c->hits = 1;
      return c;
    }
    // cache fault!
    fprintf(stderr,
            "bt::GCCache: cache fault\n"
            "      count: %d, screen: %d, item screen: %d\n",
            c->count, screen, c->ctx->screen);
    abort();
  }

  if (c->ctx) {
    // reuse existing context
    if (_font && _font->fid && _font->fid != c->ctx->fontid)
      c->ctx->set(_font);
    c->count++;
    c->hits++;
    if (prev && c->hits > prev->hits) {
      cache[ k     ] = prev;
      cache[ k - 1 ] = c;
    }
  } else {
    c->ctx = nextContext(screen);
    c->ctx->set(_color, _font, _function, _subwindow);
    c->ctx->used = true;
    c->count = 1;
    c->hits = 1;
  }

  return c;
}


void bt::GCCache::release(bt::GCCacheItem *_item) {
  _item->count--;
}


void bt::GCCache::purge(void) {
  for (unsigned int i = 0; i < cache_total_size; ++i) {
    bt::GCCacheItem *d = cache[ i ];

    if (d->ctx && d->count == 0) {
      release(d->ctx);
      d->ctx = 0;
    }
  }
}
