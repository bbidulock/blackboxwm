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
#include <stdio.h>
#include <assert.h>
}

#include "BaseDisplay.hh"
#include "Color.hh"
#include "GCCache.hh"
#include "i18n.hh"

static const char * const defaultFont = "fixed";


bt::Font::Font(const std::string &name, const bt::Display * const dpy )
  : _fontname(name), _dpy(dpy), _fontset(NULL), _font(NULL)
{
  load();
}


bt::Font::~Font(void) {
  unload();
}


bt::Font &bt::Font::operator=(const Font &f)
{
  // unload the old font
  unload();

  // copy the font name and display
  _fontname = f._fontname;
  _dpy = f._dpy;

  // load the new font
  load();

  return *this;
}


void bt::Font::load(void) {
  if (! display()) return;

  bool load_default = true;

  if (bt::i18n.multibyte()) {
    if (! fontname().empty()) {
      // _fontset = createFontSet(fontname().c_str());
      if (_fontset != NULL)
        load_default = false;
    }

    if (load_default) {
      // _fontset = createFontSet(defaultFont);
      assert(_fontset != NULL);
    }
  } else {
    if (! fontname().empty()) {
      fprintf(stderr, "loading font '%s'\n", fontname().c_str());
      _font = XLoadQueryFont(display()->getXDisplay(), fontname().c_str());
      if (_font != NULL)
        load_default = false;
    }

    if (load_default) {
      fprintf(stderr, "loading default font '%s'\n", defaultFont);
      _font = XLoadQueryFont(display()->getXDisplay(), defaultFont);
      assert(_font != NULL);
    }
  }
}


void bt::Font::unload(void) {
  if (! display()) return;

  if (_fontset != NULL)
    XFreeFontSet(display()->getXDisplay(), _fontset);
  _fontset = NULL;

  if (_font != NULL)
    XFreeFont(display()->getXDisplay(), _font);
  _font = NULL;
}


unsigned int bt::textHeight(const bt::Font &font) {
  if (bt::i18n.multibyte())
    return XExtentsOfFontSet(font.fontset())->max_ink_extent.height;
  return font.font()->ascent + font.font()->descent;
}


bt::Rect bt::textRect(const bt::Font &font, const std::string &text) {
  if (bt::i18n.multibyte()) {
    XRectangle ink, logical;
    XmbTextExtents(font.fontset(), text.c_str(), text.length(),
                   &ink, &logical);
    return bt::Rect(0, 0, ink.width,
                    XExtentsOfFontSet(font.fontset())->max_ink_extent.height);
  }
  return bt::Rect(0, 0,
                  XTextWidth(font.font(), text.c_str(), text.length()),
                  font.font()->ascent + font.font()->descent);
}


void bt::drawText(const bt::Font &font, Window window, const bt::Rect &rect,
                  bt::Alignment alignment, const std::string &text) {
  const bt::Rect tr = bt::textRect(font, text);

  // align horizontally
  int x = 0;
  switch (alignment) {
  case bt::AlignRight:
    x += rect.width() - tr.width();
    break;
  case bt::AlignCenter:
    x += (rect.width() - tr.width()) / 2;
    break;

  case bt::AlignLeft:
  default:
    break;
  }

  if (bt::i18n.multibyte()) {



  } else {
    bt::Pen pen(bt::Color(0, 0, 0, font.display()), font.font());
    // align vertically (center for now)
    int y = (rect.height() + font.font()->ascent) / 2;
    XDrawString(font.display()->getXDisplay(), window, pen.gc(), x, y,
                text.c_str(), text.length());
  }
}
