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
}

class BImageControl;

class BImage {
private:
  BImageControl *control;
  bool interlaced;
  XColor *colors;

  bt::Color from, to;
  int red_offset, green_offset, blue_offset, red_bits, green_bits, blue_bits,
    ncolors, cpc, cpccpc;
  unsigned char *red, *green, *blue, *red_table, *green_table, *blue_table;
  unsigned int width, height, *xtable, *ytable;

  void TrueColorDither(unsigned int bit_depth, int bytes_per_line,
		       unsigned char *pixel_data);
  void PseudoColorDither(int bytes_per_line, unsigned char *pixel_data);
#ifdef ORDEREDPSEUDO
  void OrderedPseudoColorDither(int bytes_per_line, unsigned char *pixel_data);
#endif

  Pixmap renderPixmap(void);
  Pixmap render_solid(const bt::Texture &texture);
  Pixmap render_gradient(const bt::Texture &texture);

  XImage *renderXImage(void);

  void invert(void);
  void bevel1(void);
  void bevel2(void);
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
  BImage(BImageControl *c, int w, int h);
  ~BImage(void);

  Pixmap render(const bt::Texture &texture);
};


class BImageControl : public bt::TimeoutHandler {
public:
  struct CachedImage {
    Pixmap pixmap;

    unsigned int count, width, height;
    unsigned long pixel1, pixel2, texture;
  };

  BImageControl(bt::Display *dpy, const bt::ScreenInfo *scrn,
                bool _dither= False, int _cpc = 4,
                unsigned long cache_timeout = 300000l,
                unsigned long cmax = 200l);
  virtual ~BImageControl(void);

  inline bt::Display *getDisplay(void) const { return display; }

  inline bool doDither(void) { return dither; }

  inline const bt::ScreenInfo *getScreenInfo(void) { return screeninfo; }

  inline Window getDrawable(void) const { return window; }

  inline Visual *getVisual(void) { return screeninfo->getVisual(); }

  inline int getBitsPerPixel(void) const { return bits_per_pixel; }
  inline int getDepth(void) const { return screen_depth; }
  inline int getColorsPerChannel(void) const
    { return colors_per_channel; }

  unsigned long getSqrt(unsigned int x);

  Pixmap renderImage(unsigned int width, unsigned int height,
                     const bt::Texture &texture);

  void installRootColormap(void);
  void removeImage(Pixmap pixmap);
  void getColorTables(unsigned char **rmt, unsigned char **gmt,
                      unsigned char **bmt,
                      int *roff, int *goff, int *boff,
                      int *rbit, int *gbit, int *bbit);
  void getXColorTable(XColor **c, int *n);
  void getGradientBuffers(unsigned int w, unsigned int h,
                          unsigned int **xbuf, unsigned int **ybuf);
  void setDither(bool d) { dither = d; }
  void setColorsPerChannel(int cpc);

  virtual void timeout(void);

private:
  bool dither;
  bt::Display *display;
  const bt::ScreenInfo *screeninfo;
#ifdef    TIMEDCACHE
  bt::Timer *timer;
#endif // TIMEDCACHE

  Colormap colormap;

  Window window;
  XColor *colors;
  int colors_per_channel, ncolors, screen_number, screen_depth,
    bits_per_pixel, red_offset, green_offset, blue_offset,
    red_bits, green_bits, blue_bits;
  unsigned char red_color_table[256], green_color_table[256],
    blue_color_table[256];
  unsigned int *grad_xbuffer, *grad_ybuffer, grad_buffer_width,
    grad_buffer_height;
  unsigned long *sqrt_table, cache_max;

  typedef std::list<CachedImage> CacheContainer;
  CacheContainer cache;

  Pixmap searchCache(const unsigned int width, const unsigned int height,
                     const unsigned long texture,
                     const bt::Color &c1, const bt::Color &c2);
};


#endif // __Image_hh
