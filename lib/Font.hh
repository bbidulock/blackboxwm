// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Font.hh for Blackbox - an X11 Window manager
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

#ifndef __Font_hh
#define __Font_hh

#include "Util.hh"

#include <string>

namespace bt {

  // forward declarations
  class Display;
  class Font;
  class Pen;
  class Rect;
  class Resource;

  enum Alignment {
    AlignLeft,
    AlignCenter,
    AlignRight
  };

  unsigned int textHeight(unsigned int screen, const Font &font);

  Rect textRect(unsigned int screen, const Font &font,
                const std::string &text);

  void drawText(const Font &font, const Pen &pen,
                Drawable drawable, const Rect &rect,
                Alignment alignment, const std::string &text);

  std::string ellideText(const std::string& text, size_t count,
                         const char* ellide);

  Alignment alignResource(const Resource &resource,
                          const char* name, const char* classname,
                          Alignment default_align = AlignLeft);

  class Font {
  public:
    static void clearCache(void);

    explicit Font(const std::string &name = std::string());
    ~Font(void);

    const std::string& fontName(void) const { return _fontname; }
    void setFontName(const std::string &new_fontname)
    { unload(); _fontname = new_fontname; }

    XFontSet fontSet(void) const;
    XftFont *xftFont(unsigned int screen) const;

    Font& operator=(const Font &f)
    { setFontName(f.fontName()); return *this; }
    bool operator==(const Font &f) const
    { return _fontname == f._fontname; }
    bool operator!=(const Font &f) const
    { return (! operator==(f)); }

  private:
    void unload(void);

    std::string _fontname;
    mutable XFontSet _fontset;
    mutable XftFont *_xftfont;
    mutable unsigned int _screen; // only used for Xft
  };

} // namespace bt

#endif // __Font_hh
