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

#include "Timer.hh"
#include "BaseDisplay.hh"
#include "Color.hh"


namespace bt {

  class Texture;
  class ImageControl;

  class Image {
  private:
    ImageControl *control;
    bool interlaced;
    XColor *colors;

    Color from, to;
    int red_offset, green_offset, blue_offset, red_bits, green_bits, blue_bits,
      ncolors, cpc, cpccpc;
    unsigned char *red, *green, *blue, *red_table, *green_table, *blue_table;
    unsigned int width, height;

    void TrueColorDither(unsigned int bit_depth, int bytes_per_line,
                         unsigned char *pixel_data);
    void PseudoColorDither(int bytes_per_line, unsigned char *pixel_data);
#ifdef ORDEREDPSEUDO
    void OrderedPseudoColorDither(int bytes_per_line, unsigned char *pixel_data);
#endif

    Pixmap renderPixmap(void);
    Pixmap render_solid(const Texture &texture);
    Pixmap render_gradient(const Texture &texture);

    XImage *renderXImage(void);

    void invert(void);
    void bevel(unsigned int border_width = 0);
    void dgradient(void);
    void egradient(void);
    void hgradient(void);
    void pgradient(void);
    void rgradient(void);
    void vgradient(void);
    void cdgradient(void);
    void pcgradient(void);


  public:
    // take signed ints so we can catch improper sizes
    Image(ImageControl *c, int w, int h);
    ~Image(void);

    Pixmap render(const Texture &texture);
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

    inline Display& getDisplay(void) const { return display; }

    inline bool doDither(void) { return dither; }

    inline const ScreenInfo *getScreenInfo(void) { return screeninfo; }

    inline Window getDrawable(void) const { return window; }

    inline Visual *getVisual(void) { return screeninfo->getVisual(); }

    inline int getBitsPerPixel(void) const { return bits_per_pixel; }
    inline int getDepth(void) const { return screen_depth; }
    inline int getColorsPerChannel(void) const
    { return colors_per_channel; }

    Pixmap renderImage(unsigned int width, unsigned int height,
                       const Texture &texture);

    void installRootColormap(void);
    void removeImage(Pixmap pixmap);
    void getColorTables(unsigned char **rmt, unsigned char **gmt,
                        unsigned char **bmt,
                        int *roff, int *goff, int *boff,
                        int *rbit, int *gbit, int *bbit);
    void getXColorTable(XColor **c, int *n);
    void setDither(bool d) { dither = d; }
    void setColorsPerChannel(int cpc);

    virtual void timeout(void);

  private:
    bool dither;
    Display& display;
    const ScreenInfo *screeninfo;
    Timer *timer;

    Colormap colormap;

    Window window;
    XColor *colors;
    int colors_per_channel, ncolors, screen_number, screen_depth,
      bits_per_pixel, red_offset, green_offset, blue_offset,
      red_bits, green_bits, blue_bits;
    unsigned char red_color_table[256], green_color_table[256],
      blue_color_table[256];
    unsigned long cache_max;

    typedef std::list<CachedImage> CacheContainer;
    CacheContainer cache;

    Pixmap searchCache(const unsigned int width, const unsigned int height,
                       const unsigned long texture,
                       const Color &c1, const Color &c2);
  };

} // namespace bt

#endif // __Image_hh
