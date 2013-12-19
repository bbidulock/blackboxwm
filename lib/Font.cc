// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Font.cc for Blackbox - an X11 Window manager
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

#include "Font.hh"
#include "Color.hh"
#include "Display.hh"
#include "Pen.hh"
#include "Resource.hh"

#include <map>
#include <vector>

#include <X11/Xlib.h>
#ifdef XFT
#  include <X11/Xft/Xft.h>
#endif

#include <assert.h>
#include <cctype>
#include <locale.h>
#include <cstdio>

// #define FONTCACHE_DEBUG


static const char * const defaultFont = "fixed";
#ifdef XFT
static const char * const defaultXftFont = "sans-serif";
#endif


namespace bt {

  class FontCache {
  public:
    FontCache(const Display &dpy);
    ~FontCache(void);

    XFontSet findFontSet(const std::string &fontsetname);
#ifdef XFT
    XftFont *findXftFont(const std::string &fontname, unsigned int screen);
#endif

    void release(const std::string &fontname, unsigned int screen);

    void clear(bool force);

    const Display &_display;
#ifdef XFT
    bool xft_initialized;
#endif

    struct FontName {
      const std::string name;
      unsigned int screen;
      inline FontName(const std::string &n, unsigned int s)
        : name(n), screen(s)
      { }
      inline bool operator<(const FontName &other) const {
        if (screen != other.screen)
          return screen < other.screen;
        return name < other.name;
      }
    };

    struct FontRef {
      XFontSet const fontset;
#ifdef XFT
      XftFont * const xftfont;
#else
      void * const xftfont; // avoid #ifdef spaghetti below...
#endif
      unsigned int count;
      inline FontRef(void)
        : fontset(0), xftfont(0), count(0u)
      { }
      inline FontRef(XFontSet const fs)
        : fontset(fs), xftfont(0), count(1u)
      { }
#ifdef XFT
      inline FontRef(XftFont * const ft)
        : fontset(0), xftfont(ft), count(1u)
      { }
#endif
    };

    typedef std::map<FontName,FontRef> Cache;
    typedef Cache::value_type CacheItem;
    Cache cache;
  };


  static FontCache *fontcache = 0;


  void createFontCache(const Display &display) {
    assert(fontcache == 0);
    fontcache = new FontCache(display);
  }


  void destroyFontCache(void) {
    delete fontcache;
    fontcache = 0;
  }


  // xlfd parser
  enum xlfd_parts {
    xp_foundry,
    xp_family,
    xp_weight,
    xp_slant,
    xp_width,
    xp_addstyle,
    xp_pixels,
    xp_points,
    xp_resx,
    xp_resy,
    xp_space,
    xp_avgwidth,
    xp_regsitry,
    xp_encoding,
    xp_count
  };


  typedef std::vector<std::string> xlfd_vector;
  xlfd_vector parse_xlfd(const std::string &xlfd) {
    std::string::const_iterator it = xlfd.begin(), end = xlfd.end(), save;
    if (it == end || !*it || *it != '-')
      return xlfd_vector();
    xlfd_vector vec(xp_count);

    int x;
    for (x = 0; x < xp_count && it != end && *it; ++x) {
      save = it;
      ++save; // skip the '-'
      while (++it != end && *it != '-')
        ;
      vec[x].assign(save, it);
    }

    if (x != xp_count)
      return xlfd_vector();
    return vec;
  }

} // namespace bt



bt::FontCache::FontCache(const Display &dpy)
  : _display(dpy)
{
#ifdef XFT
  xft_initialized = XftInit(NULL) && XftInitFtLibrary();
#endif
}


bt::FontCache::~FontCache(void)
{ clear(true); }


XFontSet bt::FontCache::findFontSet(const std::string &fontsetname) {
  if (fontsetname.empty())
    return findFontSet(defaultFont);

  // see if the font is in the cache
  assert(!fontsetname.empty());
  FontName fn(fontsetname, ~0u);
  Cache::iterator it = cache.find(fn);
  if (it != cache.end()) {
    // found it

#ifdef FONTCACHE_DEBUG
    fprintf(stderr, "bt::FontCache: ref set  '%s'\n", fontsetname.c_str());
#endif // FONTCACHE_DEBUG

    ++it->second.count;
    return it->second.fontset;
  }

  XFontSet fs;
  char **missing, *def = const_cast<char *>("-");
  int nmissing;

  // load the fontset
  fs = XCreateFontSet(_display.XDisplay(), fontsetname.c_str(),
                      &missing, &nmissing, &def);
  if (fs) {
    if (nmissing) {
      // missing characters, unload and try again below
      XFreeFontSet(_display.XDisplay(), fs);
      fs = 0;
    }

    if (missing)
      XFreeStringList(missing);

    if (fs) {
#ifdef FONTCACHE_DEBUG
      fprintf(stderr, "bt::FontCache: add set  '%s'\n", fontsetname.c_str());
#endif // FONTCACHE_DEBUG

      cache.insert(CacheItem(fn, FontRef(fs)));
      return fs; // created fontset
    }
  }

  /*
    fontset is missing some charsets, adjust the fontlist to allow
    Xlib to automatically find the needed fonts.
  */
  xlfd_vector vec = parse_xlfd(fontsetname);
  std::string newname = fontsetname;
  if (!vec.empty()) {
    newname +=
      ",-*-*-" + vec[xp_weight] + "-" + vec[xp_slant] + "-*-*-" +
      vec[xp_pixels] + "-*-*-*-*-*-*-*,-*-*-*-*-*-*-" + vec[xp_pixels] +
      "-" + vec[xp_points] + "-*-*-*-*-*-*,*";
  } else {
    newname += "-*-*-*-*-*-*-*-*-*-*-*-*-*-*,*";
  }

  fs = XCreateFontSet(_display.XDisplay(), newname.c_str(),
                      &missing, &nmissing, &def);
  if (nmissing) {
    for (int x = 0; x < nmissing; ++x)
      fprintf(stderr, "Warning: missing charset '%s' in fontset\n",
              missing[x]);
  }
  if (missing)
    XFreeStringList(missing);

#ifdef FONTCACHE_DEBUG
  fprintf(stderr, "bt::FontCache: add set  '%s'\n", fontsetname.c_str());
#endif // FONTCACHE_DEBUG

  cache.insert(CacheItem(fn, FontRef(fs)));
  return fs;
}


#ifdef XFT
XftFont *bt::FontCache::findXftFont(const std::string &fontname,
                                    unsigned int screen) {
  if (!xft_initialized)
    return 0;

  if (fontname.empty())
    return findXftFont(defaultXftFont, screen);

  // see if the font is in the cache
  assert(!fontname.empty());
  FontName fn(fontname, screen);
  Cache::iterator it = cache.find(fn);
  if (it != cache.end()) {
    // found it
    assert(it->first.screen == screen);

#ifdef FONTCACHE_DEBUG
    fprintf(stderr, "bt::FontCache: %s Xft%u '%s'\n",
            it->second.xftfont ? "ref" : "skp",
            screen, fontname.c_str());
#endif // FONTCACHE_DEBUG
    ++it->second.count;
    return it->second.xftfont;
  }

  XftFont *ret = 0;
  bool use_xft = true;
  int unused = 0;
  char **list =
    XListFonts(_display.XDisplay(), fontname.c_str(), 1, &unused);
  if (list != NULL) {
    // if fontname is a valid XLFD or alias, use a fontset instead of Xft
    use_xft = false;
    XFreeFontNames(list);

#ifdef FONTCACHE_DEBUG
    fprintf(stderr, "bt::FontCache: skp Xft%u '%s'\n",
            screen, fontname.c_str());
#endif // FONTCACHE_DEBUG
  }

  if (use_xft) {
    // Xft can't do antialiasing on 8bpp very well
    std::string n = fontname;
    if (_display.screenInfo(screen).depth() <= 8)
      n += ":antialias=false";

    ret = XftFontOpenName(_display.XDisplay(), screen, n.c_str());
    if (ret == NULL) {
      // Xft will never return NULL, but it doesn't hurt to be cautious
      fprintf(stderr, "bt::Font: couldn't load Xft%u '%s'\n",
              screen, fontname.c_str());
      ret = XftFontOpenName(_display.XDisplay(), screen, defaultXftFont);
    }
    assert(ret != NULL);

#ifdef FONTCACHE_DEBUG
    fprintf(stderr, "bt::FontCache: add Xft%u '%s'\n",
            screen, fontname.c_str());
#endif // FONTCACHE_DEBUG
  }

  cache.insert(CacheItem(fn, FontRef(ret)));
  return ret;
}
#endif


void bt::FontCache::release(const std::string &fontname, unsigned int screen) {
  if (fontname.empty()) {
#ifdef XFT
    if (screen != ~0u)
      release(defaultXftFont, screen);
    else
#endif // XFT
      release(defaultFont, screen);
    return;
  }

#ifdef FONTCACHE_DEBUG
  fprintf(stderr, "bt::FontCache: rel      '%s'\n", fontname.c_str());
#endif // FONTCACHE_DEBUG

  assert(!fontname.empty());
  FontName fn(fontname, screen);
  Cache::iterator it = cache.find(fn);

  assert(it != cache.end() && it->second.count > 0);
  --it->second.count;
}


void bt::FontCache::clear(bool force) {
  Cache::iterator it = cache.begin();
  if (it == cache.end())
    return; // nothing to do

#ifdef FONTCACHE_DEBUG
  fprintf(stderr, "bt::FontCache: clearing cache, %u entries\n", cache.size());
#endif // FONTCACHE_DEBUG

  while (it != cache.end()) {
    if (it->second.count != 0 && !force) {
      ++it;
      continue;
    }

#ifdef FONTCACHE_DEBUG
    fprintf(stderr, "bt::FontCache: fre      '%s'\n", it->first.name.c_str());
#endif // FONTCACHE_DEBUG

    if (it->second.fontset)
      XFreeFontSet(_display.XDisplay(), it->second.fontset);
#ifdef XFT
    if (it->second.xftfont)
      XftFontClose(_display.XDisplay(), it->second.xftfont);
#endif

    Cache::iterator r = it++;
    cache.erase(r);
  }

#ifdef FONTCACHE_DEBUG
  fprintf(stderr, "bt::FontCache: cleared, %u entries remain\n", cache.size());
#endif // FONTCACHE_DEBUG
}


XFontSet bt::Font::fontSet(void) const {
  if (_fontset)
    return _fontset;

  _fontset = fontcache->findFontSet(_fontname);
  return _fontset;
}


#ifdef XFT
XftFont *bt::Font::xftFont(unsigned int screen) const {
  if (_screen != ~0u && _screen == screen)
    return _xftfont;

  _screen = screen;
  _xftfont = fontcache->findXftFont(_fontname, _screen);
  return _xftfont;
}
#endif


void bt::Font::unload(void) {
  /*
    yes, we really want to check _fontset and _xftfont separately.

    if the user has called fontSet() and xftFont(), then the _fontname
    in the cache will be counted multiple times, so we will need to
    release multiple times
  */
  if (_fontset)
    fontcache->release(_fontname, ~0u); // fontsets have no screen
  _fontset = 0;

#ifdef XFT
  if (_xftfont)
    fontcache->release(_fontname, _screen);
  _xftfont = 0;
  _screen = ~0u;
#endif
}


void bt::Font::clearCache(void)
{ fontcache->clear(false); }


unsigned int bt::textHeight(unsigned int screen, const Font &font) {
#ifdef XFT
  const XftFont * const f = font.xftFont(screen);
  if (f)
    return f->ascent + f->descent;
#endif

  return XExtentsOfFontSet(font.fontSet())->max_ink_extent.height;
}


unsigned int bt::textIndent(unsigned int screen, const Font &font) {
#ifdef XFT
  const XftFont * const f = font.xftFont(screen);
  if (f)
    return f->descent;
#endif

  XFontSetExtents *e = XExtentsOfFontSet(font.fontSet());
  return e->max_ink_extent.height + e->max_ink_extent.y;
}


bt::Rect bt::textRect(unsigned int screen, const Font &font,
                      const bt::ustring &text) {
  const unsigned int indent = textIndent(screen, font);

#ifdef XFT
  XftFont * const f = font.xftFont(screen);
  if (f) {
    XGlyphInfo xgi;
    XftTextExtents32(fontcache->_display.XDisplay(), f,
                     reinterpret_cast<const FcChar32 *>(text.data()),
                     text.length(), &xgi);
    return Rect(xgi.x, 0, xgi.width - xgi.x + (indent * 2),
                f->ascent + f->descent);
  }
#endif

  const std::string str = toLocale(text);
  XRectangle ink, unused;
  XmbTextExtents(font.fontSet(), str.c_str(), str.length(), &ink, &unused);
  return Rect(ink.x, 0, ink.width - ink.x + (indent * 2),
              XExtentsOfFontSet(font.fontSet())->max_ink_extent.height);
}


void bt::drawText(const Font &font, const Pen &pen,
                  Drawable drawable, const Rect &rect,
                  Alignment alignment, const bt::ustring &text) {
  Rect tr = textRect(pen.screen(), font, text);
  unsigned int indent = textIndent(pen.screen(), font);

  // align vertically (center for now)
  tr.setY(rect.y() + ((rect.height() - tr.height()) / 2));

  // align horizontally
  switch (alignment) {
  case AlignRight:
    tr.setX(rect.x() + rect.width() - tr.width() - 1);
    break;

  case AlignCenter:
    tr.setX(rect.x() + (rect.width() - tr.width()) / 2);
    break;

  default:
  case AlignLeft:
    tr.setX(rect.x());
  }

#if 0
  // draws the rect 'tr' in red... useful for debugging text placement
  Pen red(pen.screen(), Color(255, 0, 0));
  XDrawRectangle(red.XDisplay(), drawable, red.gc(),
                 tr.x(), tr.y(), tr.width(), tr.height());
#endif

#ifdef XFT
  XftFont * const f = font.xftFont(pen.screen());
  if (f) {
    XftColor col;
    col.color.red   = pen.color().red()   | pen.color().red()   << 8;
    col.color.green = pen.color().green() | pen.color().green() << 8;
    col.color.blue  = pen.color().blue()  | pen.color().blue()  << 8;
    col.color.alpha = 0xffff;
    col.pixel = pen.color().pixel(pen.screen());

    XftDrawString32(pen.xftDraw(drawable), &col, f,
                    tr.x() + indent, tr.y() + f->ascent,
                    reinterpret_cast<const FcChar32 *>(text.data()),
                    text.length());
    return;
  }
#endif

  const std::string str = toLocale(text);
  XmbDrawString(pen.XDisplay(), drawable, font.fontSet(), pen.gc(),
                tr.x() + indent,
                tr.y() - XExtentsOfFontSet(font.fontSet())->max_ink_extent.y,
                str.c_str(), str.length());
}


bt::ustring bt::ellideText(const bt::ustring &text, size_t count,
                           const bt::ustring &ellide) {
  const bt::ustring::size_type len = text.length();
  if (len <= count)
    return text;

  assert(ellide.length() < (count / 2));

  bt::ustring ret = text;
  return ret.replace(ret.begin() + (count / 2) - (ellide.length() / 2),
                     ret.end() - (count / 2) + ((ellide.length() / 2) + 1),
                     ellide);
}


bt::ustring bt::ellideText(const bt::ustring &text,
                           unsigned int max_width,
                           const bt::ustring &ellide,
                           unsigned int screen,
                           const bt::Font &font) {
  bt::ustring visible = text;
  bt::Rect r = bt::textRect(screen, font, visible);

  if (r.width() > max_width) {
    const int min_c = (ellide.length() * 3) - 1;
    int c = visible.length();
    while (--c > min_c && r.width() > max_width) {
      visible = bt::ellideText(text, c, ellide);
      r = bt::textRect(screen, font, visible);
    }
    if (c <= min_c)
      visible = ellide; // couldn't ellide enough
  }

  return visible;
}


bt::Alignment bt::alignResource(const Resource &resource,
                                const char* name, const char* classname,
                                Alignment default_align) {
  std::string res = tolower(resource.read(name, classname));
  // we use find since res could have spaces and such things in the string
  if (res.find("left")   != std::string::npos)
    return AlignLeft;
  if (res.find("center") != std::string::npos)
    return AlignCenter;
  if (res.find("right")  != std::string::npos)
    return AlignRight;
  return default_align;
}
