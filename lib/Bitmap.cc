// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Bitmap.cc for Blackbox - an X11 Window manager
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

#include "Bitmap.hh"
#include "Display.hh"
#include "Pen.hh"

#include <X11/Xlib.h>

#include <assert.h>


static const int left_width  = 9;
static const int left_height = 9;
static const unsigned char left_bits[] =
  { 0x00, 0x00, 0x40, 0x00, 0x60, 0x00, 0x70, 0x00, 0x78,
    0x00, 0x70, 0x00, 0x60, 0x00, 0x40, 0x00, 0x00, 0x00 };

static const int right_width  = 9;
static const int right_height = 9;
static const unsigned char right_bits[] =
  { 0x00, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x1c, 0x00, 0x3c,
    0x00, 0x1c, 0x00, 0x0c, 0x00, 0x04, 0x00, 0x00, 0x00 };

static const int up_width  = 9;
static const int up_height = 9;
static const unsigned char up_bits[] =
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x38,
    0x00, 0x7c, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const int down_width  = 9;
static const int down_height = 9;
static const unsigned char down_bits[] =
  { 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x7c, 0x00, 0x38,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static const int check_width  = 9;
static const int check_height = 9;
static const unsigned char check_bits[] =
  { 0x00, 0x00, 0x80, 0x00, 0xc0, 0x00, 0xe2, 0x00, 0x76,
    0x00, 0x3e, 0x00, 0x1c, 0x00, 0x08, 0x00, 0x00, 0x00 };


namespace bt {

  class BitmapLoader {
  public:
    inline BitmapLoader(const Display &display)
      : _display(display)
    { }

    ::Drawable load(unsigned int screen, const unsigned char *data,
                    int width, int height);
    void unload(::Drawable &drawable);

  private:
    const Display &_display;
  };


  static BitmapLoader *loader = 0;


  enum StandardBitmaps {
    LeftArrow,
    RightArrow,
    UpArrow,
    DownArrow,
    CheckMark,
    NStandardBitmaps
  };
  static Bitmap *standard_bitmaps[NStandardBitmaps];


  void createBitmapLoader(const Display &display) {
    assert(loader == 0);
    loader = new BitmapLoader(display);

    for (unsigned int i = 0; i < NStandardBitmaps; ++i)
      standard_bitmaps[i] = new Bitmap[display.screenCount()];
  }


  void destroyBitmapLoader(void) {
    for (unsigned int i = 0; i < NStandardBitmaps; ++i)
      delete [] standard_bitmaps[i];

    delete loader;
    loader = 0;
  }

} // namespace bt


::Drawable bt::BitmapLoader::load(unsigned int screen,
                                  const unsigned char *data,
                                  int width, int height) {
  const bt::ScreenInfo &screeninfo = _display.screenInfo(screen);
  return XCreateBitmapFromData(_display.XDisplay(), screeninfo.rootWindow(),
                               reinterpret_cast<const char *>(data),
                               width, height);
}


void bt::BitmapLoader::unload(::Drawable &drawable) {
  if (drawable)
    XFreePixmap(_display.XDisplay(), drawable);
  drawable = 0;
}


void bt::drawBitmap(const bt::Bitmap &bitmap, const bt::Pen &pen,
                    ::Drawable drawable, const bt::Rect &rect) {
  assert(bitmap.screen() == pen.screen());

  const int x = rect.x() + (rect.width() - bitmap.width()) / 2;
  const int y = rect.y() + (rect.height() - bitmap.height()) / 2;

  XSetClipMask(pen.XDisplay(), pen.gc(), bitmap.drawable());
  XSetClipOrigin(pen.XDisplay(), pen.gc(), x, y);
  XFillRectangle(pen.XDisplay(), drawable, pen.gc(),
                 x, y, bitmap.width(), bitmap.height());
  XSetClipOrigin(pen.XDisplay(), pen.gc(), 0, 0);
  XSetClipMask(pen.XDisplay(), pen.gc(), None);
}

bt::Bitmap::Bitmap(unsigned int scr, const unsigned char *data,
                   unsigned int w, unsigned int h)
  : _screen(~0u), _drawable(0ul), _width(0u), _height(0u)
{ load(scr, data, w, h); }


bt::Bitmap::~Bitmap(void)
{ loader->unload(_drawable); }


bool bt::Bitmap::load(unsigned int scr, const unsigned char *data,
                      unsigned int w, unsigned int h) {
  loader->unload(_drawable);

  _drawable = loader->load(scr, data, w, h);

  if (!_drawable) {
    _screen = ~0u;
    _width = _height = 0;
    return false;
  }

  _screen = scr;
  _width = w;
  _height = h;
  return true;
}


const bt::Bitmap &bt::Bitmap::leftArrow(unsigned int screen) {
  Bitmap &bitmap = standard_bitmaps[LeftArrow][screen];
  if (!bitmap.drawable())
    bitmap.load(screen, left_bits, left_width, left_height);
  return bitmap;
}


const bt::Bitmap &bt::Bitmap::rightArrow(unsigned int screen) {
  Bitmap &bitmap = standard_bitmaps[RightArrow][screen];
  if (!bitmap.drawable())
    bitmap.load(screen, right_bits, right_width, right_height);
  return bitmap;
}


const bt::Bitmap &bt::Bitmap::upArrow(unsigned int screen) {
  Bitmap &bitmap = standard_bitmaps[UpArrow][screen];
  if (!bitmap.drawable())
    bitmap.load(screen, up_bits, up_width, up_height);
  return bitmap;
}


const bt::Bitmap &bt::Bitmap::downArrow(unsigned int screen) {
  Bitmap &bitmap = standard_bitmaps[DownArrow][screen];
  if (!bitmap.drawable())
    bitmap.load(screen, down_bits, down_width, down_height);
  return bitmap;
}


const bt::Bitmap &bt::Bitmap::checkMark(unsigned int screen) {
  Bitmap &bitmap = standard_bitmaps[CheckMark][screen];
  if (!bitmap.drawable())
    bitmap.load(screen, check_bits, check_width, check_height);
  return bitmap;
}
