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

#ifndef __Font_hh
#define __Font_hh

extern "C" {
#include <X11/Xlib.h>
}

#include <map>
#include <string>
#include <vector>


namespace bt {

  class Display;
  class Pen;
  class Rect;
  class Resource;

  enum Alignment {
    AlignLeft = 0,
    AlignCenter,
    AlignRight
  };

  class Font {
  public:
    explicit Font(const std::string &name = std::string(),
                  const Display * const dpy = 0);
    ~Font(void);

    inline const Display *display(void) const { return _dpy; }
    void setDisplay(const Display * const dpy);

    inline const std::string& fontname(void) const { return _fontname; }

    inline XFontSet fontset(void) const { return _fontset; }
    inline XFontStruct *font(void) const { return _font; }

    Font& operator=(const Font &f);
    inline bool operator==(const Font &f)
    { return _fontname == f._fontname; }
    inline bool operator!=(const Font &f)
    { return (! operator==(f)); }

  private:
    void load(void);
    XFontSet load_fontset(const std::string &name);
    void unload(void);

    std::string _fontname;
    const Display *_dpy;
    XFontSet _fontset;
    XFontStruct *_font;

    // global font cache
    struct FontRef {
      XFontSet fontset;
      XFontStruct *font;
      unsigned int count;
      inline FontRef(void)
        : fontset(NULL), font(NULL), count(0u) { }
      inline FontRef(XFontSet fs, XFontStruct *f)
        : fontset(fs), font(f), count(1u) { }
    };
    typedef std::map<std::string,FontRef> FontCache;
    typedef FontCache::value_type FontCacheItem;
    static FontCache fontcache;

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
    static xlfd_vector parse_xlfd(const std::string &xlfd);
  };

  unsigned int textHeight(const Font &font);

  Rect textRect(const Font &font, const std::string &text);

  void drawText(const Font &font, const Pen &pen, Window window,
                const Rect &rect, Alignment alignment,
                const std::string &text);

  std::string ellideText(const std::string& text, unsigned int count,
                         const char* ellide);

  Alignment alignResource(const Resource &resource,
                          const std::string &name,
                          const std::string &classname,
                          Alignment default_align = AlignLeft);

} // namespace bt

#endif // __Font_hh
