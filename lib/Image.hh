// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Image.hh for Blackbox - an X11 Window manager
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

#ifndef   __Image_hh
#define   __Image_hh

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
}

#include <list>
#include <vector>

#include "Timer.hh"
#include "BaseDisplay.hh"
#include "Color.hh"


namespace bt {

  class Texture;
  class XColorTable;

  class Image {
  public:
    static unsigned int colorsPerChannel(void)
    { return global_colorsPerChannel; }
    static void setColorsPerChannel(unsigned int newval)
    { global_colorsPerChannel = newval > 2 ? newval < 6 ? newval : 6 : 2; }

    static bool isDitherEnabled(void)
    { return global_ditherEnabled; }
    static void setDitherEnabled(bool enabled)
    { global_ditherEnabled = enabled; }

    Image(unsigned int w, unsigned int h);
    ~Image(void);

    Pixmap render(const Texture &texture);

  private:
    unsigned char *red, *green, *blue;
    unsigned int width, height;

    void TrueColorDither(XColorTable *colortable,
                         unsigned int bit_depth,
                         int bytes_per_line,
                         unsigned char *pixel_data);
    void PseudoColorDither(XColorTable *colortable,
                           int bytes_per_line,
                           unsigned char *pixel_data);
    void OrderedPseudoColorDither(XColorTable *colortable,
                                  int bytes_per_line,
                                  unsigned char *pixel_data);

    Pixmap render_solid(const Texture &texture);
    Pixmap render_gradient(const Texture &texture);

    Pixmap renderPixmap(const Display &display, unsigned int screen);
    XImage *renderXImage(const Display &display, unsigned int screen);

    void invert(void);
    void bevel(unsigned int border_width = 0);
    void dgradient(const Color &from, const Color &to, bool interlaced);
    void egradient(const Color &from, const Color &to, bool interlaced);
    void hgradient(const Color &from, const Color &to, bool interlaced);
    void pgradient(const Color &from, const Color &to, bool interlaced);
    void rgradient(const Color &from, const Color &to, bool interlaced);
    void vgradient(const Color &from, const Color &to, bool interlaced);
    void cdgradient(const Color &from, const Color &to, bool interlaced);
    void pcgradient(const Color &from, const Color &to, bool interlaced);

    typedef std::vector<unsigned char> Buffer;
    static Buffer buffer;
    static unsigned int global_colorsPerChannel;
    static bool global_ditherEnabled;

    typedef std::vector<XColorTable*> XColorTableList;
    static XColorTableList colorTableList;
  };


  class ImageControl : public TimeoutHandler {
  public:
    struct CachedImage {
      Pixmap pixmap;

      unsigned int count, width, height;
      unsigned long pixel1, pixel2, texture;
    };

    ImageControl(TimerQueueManager *app, Display& _display,
                 const ScreenInfo *scrn,
                 bool _dither= False, int _cpc = 4,
                 unsigned long cache_timeout = 300000l,
                 unsigned long cmax = 200l);
    virtual ~ImageControl(void);

    Pixmap renderImage(unsigned int width, unsigned int height,
                       const Texture &texture);

    void installRootColormap(void);
    void removeImage(Pixmap pixmap);

    virtual void timeout(void);

  private:
    Display &display;
    Window window;
    Colormap colormap;
    Timer *timer;
    unsigned long cache_max;

    typedef std::list<CachedImage> CacheContainer;
    CacheContainer cache;

    Pixmap searchCache(const unsigned int width, const unsigned int height,
                       const unsigned long texture,
                       const Color &c1, const Color &c2);
  };

} // namespace bt

#endif // __Image_hh
