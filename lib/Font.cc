// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
//
// Blackbox - an X11 Window manager
//
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

#include "Font.hh"

extern "C" {
#include <assert.h>
#include <ctype.h>
}

#include "BaseDisplay.hh"
#include "Color.hh"
#include "GCCache.hh"
#include "Resource.hh"
#include "i18n.hh"

static const char * const defaultFont = "fixed";


bt::Font::FontCache bt::Font::fontcache;

bt::Font::Font(const std::string &name, const bt::Display * const dpy )
  : _fontname(name), _dpy(dpy), _fontset(NULL), _font(NULL) {
  if (_dpy) load();
}


bt::Font::~Font(void) {
  if (_dpy) unload();
}


bt::Font &bt::Font::operator=(const Font &f) {
  if (_dpy) unload();

  _fontname = f._fontname;
  _dpy = f._dpy;

  if (_dpy) load();

  return *this;
}


void bt::Font::load(void) {
  if (_font || _fontset) return;
  assert(_dpy != 0);

  if (_fontname.empty())
    _fontname = defaultFont;

  // see if this font is in the cache
  FontCache::iterator it = fontcache.find(_fontname);
  if (it != fontcache.end()) {
    // found
    _fontset = it->second.fontset;
    _font = it->second.font;
    ++it->second.count;
    return;
  }

  if (bt::i18n.multibyte()) {
    // _fontset = createFontSet(_fontname.c_str());

    if (_fontset == NULL) {
      _fontname = defaultFont;
      // _fontset = createFontSet(_fontname.c_str());
    }
    assert(_fontset != NULL);
  } else {
    _font = XLoadQueryFont(display()->XDisplay(), _fontname.c_str());

    if (_font == NULL) {
      _fontname = defaultFont;
      _font = XLoadQueryFont(display()->XDisplay(), _fontname.c_str());
    }
    assert(_font != NULL);
  }

  // insert new item into the cache

  fontcache.insert(FontCacheItem(_fontname, FontRef(_fontset, _font)));
}


void bt::Font::unload(void) {
  if (!_font && !_fontset) return;
  assert(_dpy != 0);

  // see if this font is in the cache
  FontCache::iterator it = fontcache.find(_fontname);
  if (it != fontcache.end()) {
    // found
    if (--it->second.count == 0) {
      // no more references to this font, free it
      if (it->second.fontset != NULL)
        XFreeFontSet(display()->XDisplay(), it->second.fontset);
      if (it->second.font != NULL)
        XFreeFont(display()->XDisplay(), it->second.font);

      // remove from the cache
      fontcache.erase(it);
    }
  }

  _fontset = NULL;
  _font = NULL;
}


unsigned int bt::textHeight(const bt::Font &font) {
  if (bt::i18n.multibyte())
    return XExtentsOfFontSet(font.fontset())->max_ink_extent.height;
  return font.font()->ascent + font.font()->descent;
}


bt::Rect bt::textRect(const bt::Font &font, const std::string &text) {
  if (bt::i18n.multibyte()) {
    XRectangle ink, unused;
    XmbTextExtents(font.fontset(), text.c_str(), text.length(), &ink, &unused);
    return bt::Rect(0, 0, ink.width,
                    XExtentsOfFontSet(font.fontset())->max_ink_extent.height);
  }
  return bt::Rect(0, 0, XTextWidth(font.font(), text.c_str(), text.length()),
                  font.font()->ascent + font.font()->descent);
}


void bt::drawText(const bt::Font &font, const bt::Pen &pen, Window window,
                  const bt::Rect &rect, bt::Alignment alignment,
                  const std::string &text) {
  bt::Rect tr = bt::textRect(font, text);

  // align horizontally
  switch (alignment) {
  case bt::AlignRight:
    tr.setX(rect.x() + rect.width() - tr.width());
    break;

  case bt::AlignCenter:
    tr.setX(rect.x() + (rect.width() - tr.width()) / 2);
    break;

  default:
  case bt::AlignLeft:
    tr.setX(rect.x());
  }

  // align vertically (center for now)
  tr.setY(rect.y() + ((rect.height() - tr.height()) / 2));

  if (bt::i18n.multibyte()) {

  } else {
    XDrawString(font.display()->XDisplay(), window, pen.gc(),
                tr.x(), tr.y() + font.font()->ascent,
                text.c_str(), text.length());
  }
}


/*
 * Take a string and make it 'count' chars long by removing the middle
 * and replacing it with the string in 'ellide'
 *
 * FIXME: this function currently assumes that ellide is 3 chars long
 */
std::string bt::ellideText(const std::string& text, unsigned int count,
                           const char* ellide) {
  unsigned int len = text.length();
  if (len <= count)
    return text;

  std::string ret = text;

  // delta represents the amount of text to remove from the right hand side
  unsigned int delta = std::min(len - count, (count/2) - 2);

  ret.replace(ret.begin() + (count/2) - 1, ret.end() - delta, ellide);

  return ret;
}


bt::Alignment bt::alignResource(const Resource &resource,
                                const std::string &name,
                                const std::string &classname,
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
