// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// PixmapCache.cc for Blackbox - an X11 Window manager
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


#include "PixmapCache.hh"
#include "Display.hh"
#include "Image.hh"
#include "Texture.hh"

#include <X11/Xlib.h>
#include <assert.h>
#include <stdio.h>

#include <algorithm>
#include <list>

// #define PIXMAPCACHE_DEBUG


namespace bt {

  class RealPixmapCache {
  public:
    RealPixmapCache(const Display &display);
    ~RealPixmapCache(void);

    Pixmap find(unsigned int screen,
                const Texture &texture,
                unsigned int width, unsigned int height,
                Pixmap old_pixmap = 0ul);
    void release(Pixmap pixmap);

    void clear(bool force);

  private:
    struct CacheItem {
      const Texture texture;
      const unsigned int screen;
      const unsigned int width;
      const unsigned int height;
      Pixmap pixmap;
      unsigned int count;

      inline CacheItem(void)
        : screen(~0u), width(0u), height(0u),
          pixmap(0ul), count(0u) { }
      inline CacheItem(const unsigned int s, const Texture &t,
                       const unsigned int w, const unsigned int h)
        : texture(t), screen(s), width(w), height(h),
          pixmap(0ul), count(1u) { }

      inline bool operator==(const CacheItem &x) const {
        return texture == x.texture &&
                screen == x.screen &&
                 width == x.width &&
                height == x.height;
      }
    };

    struct PixmapMatch {
      PixmapMatch(Pixmap p): pixmap(p) {}
      bool operator()(const RealPixmapCache::CacheItem& item) const
      { return item.pixmap == pixmap; }

      const Pixmap pixmap;
    };

    const Display &_display;

    typedef std::list<CacheItem> Cache;
    Cache cache;
  };

  static RealPixmapCache *realpixmapcache = 0;
  static unsigned long maxmem_usage = 1048576ul; // 1mb default
  static unsigned long mem_usage = 0ul;

  void createPixmapCache(const Display &display) {
    assert(realpixmapcache == 0);
    realpixmapcache = new RealPixmapCache(display);
  }


  void destroyPixmapCache(void) {
    delete realpixmapcache;
    realpixmapcache = 0;

    assert(mem_usage == 0ul);
  }

} // namespace bt


bt::RealPixmapCache::RealPixmapCache(const Display &display)
  : _display(display) { }


bt::RealPixmapCache::~RealPixmapCache(void) { clear(true); }


Pixmap bt::RealPixmapCache::find(unsigned int screen,
                                 const Texture &texture,
                                 unsigned int width, unsigned int height,
                                 Pixmap old_pixmap) {
  release(old_pixmap);

  if (texture.texture() == (Texture::Flat | Texture::Solid))
    return None;

  if (texture.texture() == Texture::Parent_Relative)
    return ParentRelative;

  Pixmap p;
  // find one in the cache
  CacheItem item(screen, texture, width, height);
  Cache::iterator it = std::find(cache.begin(), cache.end(), item);

  if (it != cache.end()) {
    // found
    ++(it->count);

    p = it->pixmap;

#ifdef PIXMAPCACHE_DEBUG
    fprintf(stderr, "bt::PixmapCache: use %08lx %4dx%4d, count %4d\n",
            it->pixmap, width, height, it->count);
#endif // PIXMAPCACHE_DEBUG
  } else {
    Image image(width, height);
    p = image.render(_display, screen, texture);

    if (p) {
      item.pixmap = p;

#ifdef PIXMAPCACHE_DEBUG
      fprintf(stderr,
              "bt::PixmapCache: add %08lx %4dx%4d\n"
              "                 mem %8ld max %8ld\n",
              p, width, height, mem_usage, maxmem_usage);
#endif // PIXMAPCACHE_DEBUG

      cache.push_front(item);

      // keep track of memory usage server side
      const unsigned long mem =
        ( ( width * height ) * (_display.screenInfo(screen).depth() / 8 ) );
      mem_usage += mem;
      if (mem_usage > maxmem_usage)
        clear(false);

      if (mem_usage > maxmem_usage) {
        fprintf(stderr,
                "bt::PixmapCache: maximum size (%lu kb) exceeded\n"
                "bt::PixmapCache: current size: %lu kb\n",
                maxmem_usage / 1024, mem_usage / 1024);
      }
    }
  }

  return p;
}


void bt::RealPixmapCache::release(Pixmap pixmap) {
  if (! pixmap || pixmap == ParentRelative) return;

  Cache::iterator it = std::find_if(cache.begin(), cache.end(),
                                    PixmapMatch(pixmap));
  assert(it != cache.end() && it->count > 0);

  // decrement the refcount
  --(it->count);

#ifdef PIXMAPCACHE_DEBUG
  fprintf(stderr, "bt::PixmapCache: rel %08lx %4dx%4d, count %4d\n",
          it->pixmap, it->width, it->height, it->count);
#endif // PIXMAPCACHE_DEBUG
}


void bt::RealPixmapCache::clear(bool force) {
  if (cache.empty()) return; // nothing to do

#ifdef PIXMAPCACHE_DEBUG
  fprintf(stderr, "bt::PixmapCache: clearing cache, %d entries\n",
          cache.size());
#endif // PIXMAPCACHE_DEBUG

  Cache::iterator it = cache.begin();
  while (it != cache.end()) {
    if (it->count != 0 && ! force) {
#ifdef PIXMAPCACHE_DEBUG
      fprintf(stderr, "bt::PixmapCache: skp %08lx %4dx%4d, count %4d\n",
              it->pixmap, it->width, it->height, it->count);
#endif // PIXMAPCACHE_DEBUG

      ++it;
      continue;
    }

#ifdef PIXMAPCACHE_DEBUG
    fprintf(stderr, "bt::PixmapCache: fre %08lx %4dx%4d\n",
            it->pixmap, it->width, it->height);
#endif // PIXMAPCACHE_DEBUG

    // keep track of memory usage server side
    const unsigned long mem =
      ( ( it->width * it->height ) *
        (_display.screenInfo(it->screen).depth() / 8 ) );
    assert(mem <= mem_usage);
    mem_usage -= mem;

    // free pixmap
    XFreePixmap(_display.XDisplay(), it->pixmap);

    // remove from cache
    it = cache.erase(it);
  }

#ifdef PIXMAPCACHE_DEBUG
  fprintf(stderr,
          "bt::PixmapCache: cleared, %d entries remain\n"
          "                 mem %8ld max %8ld\n",
          cache.size(), mem_usage, maxmem_usage);
#endif // PIXMAPCACHE_DEBUG
}


unsigned long bt::PixmapCache::cacheLimit(void) {
  return maxmem_usage / 1024;
}


void bt::PixmapCache::setCacheLimit(unsigned long limit) {
  maxmem_usage = limit * 1024;
}


unsigned long bt::PixmapCache::memoryUsage(void) {
  return mem_usage / 1024;
}


Pixmap bt::PixmapCache::find(unsigned int screen,
                             const Texture &texture,
                             unsigned int width, unsigned int height,
                             Pixmap old_pixmap) {
  return realpixmapcache->find(screen, texture, width, height, old_pixmap);
}


void bt::PixmapCache::release(Pixmap pixmap) {
    realpixmapcache->release(pixmap);
}


void bt::PixmapCache::clearCache(void) {
  realpixmapcache->clear(false);
}
