// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Color.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry <shaleh at debian.org>
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

#include "Color.hh"
#include "Display.hh"

#include <X11/Xlib.h>
#include <assert.h>
#include <stdio.h>

#include <map>

// #define COLORCACHE_DEBUG


namespace bt {

  class ColorCache {
  public:
    ColorCache(const Display &display);
    ~ColorCache(void);

    /*
      Finds a color matching the specified rgb on the given screen.

      The color is allocated if needed; otherwise it is reference
      counted and freed when no more references for the color exist.
    */
    unsigned long find(unsigned int screen, int r, int g, int b);
    /*
      Releases the specified rgb on the given screen.

      If the reference count for a particular color is zero, it will
      be freed by calling clear().
    */
    void release(unsigned int screen, int r, int g, int b);

    /*
      Clears the color cache.  All colors with a zero reference count
      are freed.

      If force is true, then all colors are freed, regardless of the
      reference count.  This is done when destroying the cache.
    */
    void clear(bool force);

  private:
    const Display &_display;

    struct RGB {
      const unsigned int screen;
      const int r, g, b;

      inline RGB(void)
        : screen(~0u), r(-1), g(-1), b(-1) { }
      inline RGB(const unsigned int s, const int x, const int y, const int z)
        : screen(s), r(x), g(y), b(z) { }
      inline RGB(const RGB &x)
        : screen(x.screen), r(x.r), g(x.g), b(x.b) { }

      inline bool operator==(const RGB &x) const
      { return screen == x.screen && r == x.r && g == x.g && b == x.b; }

      inline bool operator<(const RGB &x) const {
        const unsigned long p1 =
          (screen << 24 | r << 16 | g << 8 | b) & 0xffffffff;
        const unsigned long p2 =
          (x.screen << 24 | x.r << 16 | x.g << 8 | x.b) & 0xffffffff;
        return p1 < p2;
      }
    };
    struct PixelRef {
      const unsigned long pixel;
      unsigned int count;
      inline PixelRef(void) : pixel(0ul), count(0u) { }
      inline PixelRef(const unsigned long x) : pixel(x), count(1u) { }
    };

    typedef std::map<RGB,PixelRef> Cache;
    typedef Cache::value_type CacheItem;
    Cache cache;
  };


  static ColorCache *colorcache = 0;


  void createColorCache(const Display &display) {
    assert(colorcache == 0);
    colorcache = new ColorCache(display);
  }


  void destroyColorCache(void) {
    delete colorcache;
    colorcache = 0;
  }

} // namespace bt


bt::ColorCache::ColorCache(const Display &display) : _display(display) { }


bt::ColorCache::~ColorCache(void) { clear(true); }


unsigned long bt::ColorCache::find(unsigned int screen, int r, int g, int b) {
  if (r < 0 && r > 255) r = 0;
  if (g < 0 && g > 255) g = 0;
  if (b < 0 && b > 255) b = 0;

  // see if we have allocated this color before
  RGB rgb(screen, r, g, b);
  Cache::iterator it = cache.find(rgb);
  if (it != cache.end()) {
    // found a cached color, use it
    ++it->second.count;

#ifdef COLORCACHE_DEBUG
    fprintf(stderr, "bt::ColorCache: use %02x/%02x/%02x, count %4d\n",
            r, g, b, it->second.count);
#endif // COLORCACHE_DEBUG

    return it->second.pixel;
  }

  XColor xcol;
  xcol.red   = r | r << 8;
  xcol.green = g | g << 8;
  xcol.blue  = b | b << 8;
  xcol.pixel = 0;
  xcol.flags = DoRed | DoGreen | DoBlue;

  Colormap colormap = _display.screenInfo(screen).colormap();
  if (! XAllocColor(_display.XDisplay(), colormap, &xcol)) {
    fprintf(stderr,
            "bt::Color::pixel: cannot allocate color 'rgb:%02x/%02x/%02x'\n",
            r, g, b);
    xcol.pixel = BlackPixel(_display.XDisplay(), screen);
  }

#ifdef COLORCACHE_DEBUG
  fprintf(stderr, "bt::ColorCache: add %02x/%02x/%02x, pixel %08lx\n",
          r, g, b, xcol.pixel);
#endif // COLORCACHE_DEBUG

  cache.insert(CacheItem(rgb, PixelRef(xcol.pixel)));

  return xcol.pixel;
}


void bt::ColorCache::release(unsigned int screen, int r, int g, int b) {
  if (r < 0 && r > 255) r = 0;
  if (g < 0 && g > 255) g = 0;
  if (b < 0 && b > 255) b = 0;

  RGB rgb(screen, r, g, b);
  Cache::iterator it = cache.find(rgb);

  assert(it != cache.end() && it->second.count > 0);
  --it->second.count;

#ifdef COLORCACHE_DEBUG
  fprintf(stderr, "bt::ColorCache: rel %02x/%02x/%02x, count %4d\n",
          r, g, b, it->second.count);
#endif // COLORCACHE_DEBUG
}


void bt::ColorCache::clear(bool force) {
  Cache::iterator it = cache.begin();
  if (it == cache.end()) return; // nothing to do

#ifdef COLORCACHE_DEBUG
  fprintf(stderr, "bt::ColorCache: clearing cache, %d entries\n",
          cache.size());
#endif // COLORCACHE_DEBUG

  unsigned long *pixels = new unsigned long[ cache.size() ];
  unsigned int screen, count;

  for (screen = 0; screen < _display.screenCount(); ++screen) {
    count = 0;
    it = cache.begin();
    while (it != cache.end()) {
      if (it->second.count != 0 && !force) {
        ++it;
        continue;
      }

#ifdef COLORCACHE_DEBUG
      fprintf(stderr, "bt::ColorCache: fre %02x/%02x/%02x, pixel %08lx\n",
              it->first.r, it->first.g, it->first.b, it->second.pixel);
#endif // COLORCACHE_DEBUG

      pixels[count++] = it->second.pixel;

      Cache::iterator r = it++;
      cache.erase(r);
    }

    if (count > 0u) {
      XFreeColors(_display.XDisplay(),
                  _display.screenInfo(screen).colormap(),
                  pixels, count, 0);
    }
  }

  delete [] pixels;

#ifdef COLORCACHE_DEBUG
  fprintf(stderr, "bt::ColorCache: cleared, %d entries remain\n",
          cache.size());
#endif // COLORCACHE_DEBUG
}


void bt::Color::clearCache(void)
{ if (colorcache) colorcache->clear(false); }


bt::Color bt::Color::namedColor(const Display &display, unsigned int screen,
                                const std::string &colorname) {
  if (colorname.empty()) {
    fprintf(stderr, "bt::Color::namedColor: empty colorname\n");
    return Color();
  }

  // get rgb values from colorname
  XColor xcol;
  xcol.red   = 0;
  xcol.green = 0;
  xcol.blue  = 0;
  xcol.pixel = 0;

  Colormap colormap = display.screenInfo(screen).colormap();
  if (! XParseColor(display.XDisplay(), colormap, colorname.c_str(), &xcol)) {
    fprintf(stderr, "bt::Color::namedColor: invalid color '%s'\n",
            colorname.c_str());
    return Color();
  }

  return Color(xcol.red >> 8, xcol.green >> 8, xcol.blue >> 8);
}


unsigned long bt::Color::pixel(unsigned int screen) const {
  if ( _screen == screen) return _pixel; // already allocated on this screen

  assert(colorcache != 0);
  // deallocate() isn't const, so we can't call it from here
  if (_screen != ~0u)
    colorcache->release(_screen, _red, _green, _blue);

  _screen = screen;
  _pixel = colorcache->find(_screen, _red, _green, _blue);
  return _pixel;
}


void bt::Color::deallocate(void) {
  if (_screen == ~0u) return; // not allocated

  assert(colorcache != 0);
  colorcache->release(_screen, _red, _green, _blue);

  _screen = ~0u;
  _pixel = 0ul;
}
