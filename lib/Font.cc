// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Font.cc for Blackbox - an X11 Window manager
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

extern "C" {
#include <assert.h>
#include <ctype.h>
#include <locale.h>
#include <stdio.h>
}

#include <map>
#include <vector>

#include "Font.hh"
#include "Color.hh"
#include "Display.hh"
#include "Pen.hh"
#include "Resource.hh"
#include "i18n.hh"

// #define FONTCACHE_DEBUG

static const char * const defaultFont = "fixed";


namespace bt {

  class FontCache {
  public:
    FontCache(const Display &dpy);
    ~FontCache(void);

    XFontStruct *findFont(const std::string &fontname);
    XFontSet findFontSet(const std::string &fontsetname);

    void release(const std::string &fontname);

    void clear(bool force);

  private:
    const Display &_display;

    struct FontRef {
      XFontSet const fontset;
      XFontStruct * const font;
      unsigned int count;
      inline FontRef(void)
        : fontset(0), font(0), count(0u) { }
      inline FontRef(XFontStruct * const f)
        : fontset(0), font(f), count(1u) { }
      inline FontRef(XFontSet const fs)
        : fontset(fs), font(0), count(1u) { }
    };

    typedef std::map<std::string,FontRef> Cache;
    typedef Cache::value_type CacheItem;
    Cache cache;
  };


  static FontCache *fontcache = 0;


  void createFontCache(const Display &dpy) {
    assert(fontcache == 0);
    fontcache = new FontCache(dpy);
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
    if (it == end || ! *it || *it != '-') return xlfd_vector();
    xlfd_vector vec(xp_count);

    int x;
    for (x = 0; x < xp_count && it != end && *it; ++x) {
      save = it;
      ++save; // skip the '-'
      while (++it != end && *it != '-');
      vec[x].assign(save, it);
    }

    if (x != xp_count)
      return xlfd_vector();
    return vec;
  }

} // namespace bt



bt::FontCache::FontCache(const Display &dpy) : _display(dpy) { }


bt::FontCache::~FontCache(void) { clear(true); }


XFontStruct *bt::FontCache::findFont(const std::string &fontname) {
  if (fontname.empty()) return findFont(defaultFont);

  // see if the font is in the cache
  Cache::iterator it = cache.find(fontname);
  if (it != cache.end()) {
    // found it

#ifdef FONTCACHE_DEBUG
    fprintf(stderr, "bt::FontCache: ref font '%s'\n", fontname.c_str());
#endif // FONTCACHE_DEBUG

    ++it->second.count;
    return it->second.font;
  }

  XFontStruct *ret = XLoadQueryFont(_display.XDisplay(), fontname.c_str());
  if (ret == NULL) {
    fprintf(stderr, "bt::Font: couldn't load font '%s'\n", fontname.c_str());
    ret = XLoadQueryFont(_display.XDisplay(), defaultFont);
  }
  assert(ret != NULL);

#ifdef FONTCACHE_DEBUG
  fprintf(stderr, "bt::FontCache: add font '%s'\n", fontname.c_str());
#endif // FONTCACHE_DEBUG

  cache.insert(CacheItem(fontname, FontRef(ret)));
  return ret;
}


XFontSet bt::FontCache::findFontSet(const std::string &fontsetname) {
  if (fontsetname.empty()) return findFontSet(defaultFont);

  // see if the font is in the cache
  Cache::iterator it = cache.find(fontsetname);
  if (it != cache.end()) {
    // found it

#ifdef FONTCACHE_DEBUG
    fprintf(stderr, "bt::FontCache: ref set  '%s'\n", fontsetname.c_str());
#endif // FONTCACHE_DEBUG

    ++it->second.count;
    return it->second.fontset;
  }

  XFontSet fs;
  char **missing, *def = "-";
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

    if (missing) XFreeStringList(missing);

    if (fs) {
#ifdef FONTCACHE_DEBUG
      fprintf(stderr, "bt::FontCache: add set  '%s'\n", fontsetname.c_str());
#endif // FONTCACHE_DEBUG

      cache.insert(CacheItem(fontsetname, FontRef(fs)));
      return fs; // created fontset
    }
  }

  /*
    fontset is missing some charsets, adjust the fontlist to allow
    Xlib to automatically find the needed fonts.
  */
  xlfd_vector vec = parse_xlfd(fontsetname);
  std::string newname = fontsetname;
  if (! vec.empty()) {
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
    int x;
    for (x = 0; x < nmissing; ++x)
      fprintf(stderr, "Warning: missing charset '%s' in fontset\n",
              missing[x]);
  }
  if (missing)
    XFreeStringList(missing);

#ifdef FONTCACHE_DEBUG
  fprintf(stderr, "bt::FontCache: add set  '%s'\n", fontsetname.c_str());
#endif // FONTCACHE_DEBUG

  cache.insert(CacheItem(fontsetname, FontRef(fs)));
  return fs;
}


void bt::FontCache::release(const std::string &fontname) {
#ifdef FONTCACHE_DEBUG
  fprintf(stderr, "bt::FontCache: rel      '%s'\n", fontname.c_str());
#endif // FONTCACHE_DEBUG

  Cache::iterator it = cache.find(fontname);

  assert(it != cache.end() && it->second.count > 0);
  --it->second.count;
}


void bt::FontCache::clear(bool force) {
  Cache::iterator it = cache.begin();
  if (it == cache.end()) return; // nothing to do

#ifdef FONTCACHE_DEBUG
  fprintf(stderr, "bt::FontCache: clearing cache, %d entries\n", cache.size());
#endif // FONTCACHE_DEBUG

  while (it != cache.end()) {
    if (it->second.count != 0 && !force) {
      ++it;
      continue;
    }

#ifdef FONTCACHE_DEBUG
    fprintf(stderr, "bt::FontCache: fre      '%s'\n", it->first.c_str());
#endif // FONTCACHE_DEBUG

    if (it->second.font)
      XFreeFont(_display.XDisplay(), it->second.font);
    if (it->second.fontset)
      XFreeFontSet(_display.XDisplay(), it->second.fontset);

    Cache::iterator r = it++;
    cache.erase(r);
  }

#ifdef FONTCACHE_DEBUG
  fprintf(stderr, "bt::FontCache: cleared, %d entries remain\n", cache.size());
#endif // FONTCACHE_DEBUG
}


bt::Font::Font(const std::string &name)
  : _fontname(name), _fontset(0), _font(0) { }


bt::Font::~Font(void) {
  unload();
}


XFontSet bt::Font::fontset(void) const {
  if (_fontset) return _fontset;

  _fontset = fontcache->findFontSet(_fontname);
  return _fontset;
}


XFontStruct *bt::Font::font(void) const {
  if (_font) return _font;

  _font = fontcache->findFont(_fontname);
  return _font;
}


void bt::Font::unload(void) {
  /*
    yes, we really want to check _fontset and _font separately.

    if the user has called both font() and fontset(), then the _fontname
    in the cache will be counted twice, so we will need to release twice
  */
  if (_fontset) fontcache->release(_fontname);
  if (_font) fontcache->release(_fontname);
  _fontset = 0;
  _font = 0;
}


void bt::Font::clearCache(void) {
  fontcache->clear(false);
}


unsigned int bt::textHeight(const Font &font) {
  if (i18n.multibyte())
    return XExtentsOfFontSet(font.fontset())->max_ink_extent.height;
  return font.font()->ascent + font.font()->descent;
}


bt::Rect bt::textRect(const Font &font, const std::string &text) {
  if (i18n.multibyte()) {
    XRectangle ink, unused;
    XmbTextExtents(font.fontset(), text.c_str(), text.length(), &ink, &unused);
    return Rect(0, 0, ink.width,
                    XExtentsOfFontSet(font.fontset())->max_ink_extent.height);
  }
  return Rect(0, 0, XTextWidth(font.font(), text.c_str(), text.length()),
                  font.font()->ascent + font.font()->descent);
}


void bt::drawText(const Font &font, Pen &pen, Window window,
                  const Rect &rect, Alignment alignment,
                  const std::string &text) {
  Rect tr = textRect(font, text);

  // align vertically (center for now)
  tr.setY(rect.y() + ((rect.height() - tr.height()) / 2));

  // align horizontally
  switch (alignment) {
  case AlignRight:
    tr.setX(rect.x() + rect.width() - tr.width());
    break;

  case AlignCenter:
    tr.setX(rect.x() + (rect.width() - tr.width()) / 2);
    break;

  default:
  case AlignLeft:
    tr.setX(rect.x());
  }

  // set the font on the pen
  pen.setFont(font);

  ::Display *xdpy = pen.display().XDisplay();
  if (i18n.multibyte()) {
    XmbDrawString(xdpy, window, font.fontset(), pen.gc(), tr.x(),
                  tr.y() - XExtentsOfFontSet(font.fontset())->max_ink_extent.y,
                  text.c_str(), text.length());
  } else {
    XDrawString(xdpy, window, pen.gc(), tr.x(), tr.y() + font.font()->ascent,
                text.c_str(), text.length());
  }
}


/*
 * Take a string and make it 'count' chars long by removing the middle
 * and replacing it with the string in 'ellide'
 */
std::string bt::ellideText(const std::string& text, size_t count,
                           const char* ellide) {
  const std::string::size_type len = text.length();
  if (len <= count)
    return text;

  const size_t ellide_len = strlen(ellide);
  assert(ellide_len < (count / 2));

  std::string ret = text;

  return ret.replace(ret.begin() + (count / 2) - (ellide_len / 2),
                     ret.end() - (count / 2) + ((ellide_len / 2) + 1), ellide);
}


bt::Alignment bt::alignResource(const Resource &resource,
                                const char* name, const char* classname,
                                Alignment default_align) {
  std::string res = resource.read(name, classname);

  // convert to lowercase
  std::string::iterator it, end;
  for (it = res.begin(), end = res.end(); it != end; ++it)
    *it = tolower(*it);

  // we use find since res could have spaces and such things in the string
  if (res.find("left")   != std::string::npos) return AlignLeft;
  if (res.find("center") != std::string::npos) return AlignCenter;
  if (res.find("right")  != std::string::npos) return AlignRight;
  return default_align;
}
