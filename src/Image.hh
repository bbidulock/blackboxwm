// -*- mode: C++; indent-tabs-mode: nil; -*-
// Image.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <list>

#include "Timer.hh"
#include "BaseDisplay.hh"

class BImage;
class BImageControl;


// bevel options
#define BImage_Flat		(1l<<1)
#define BImage_Sunken		(1l<<2)
#define BImage_Raised		(1l<<3)

// textures
#define BImage_Solid		(1l<<4)
#define BImage_Gradient		(1l<<5)

// gradients
#define BImage_Horizontal	(1l<<6)
#define BImage_Vertical		(1l<<7)
#define BImage_Diagonal		(1l<<8)
#define BImage_CrossDiagonal	(1l<<9)
#define BImage_Rectangle	(1l<<10)
#define BImage_Pyramid		(1l<<11)
#define BImage_PipeCross	(1l<<12)
#define BImage_Elliptic		(1l<<13)

// bevel types
#define BImage_Bevel1		(1l<<14)
#define BImage_Bevel2		(1l<<15)

// inverted image
#define BImage_Invert		(1l<<16)

// parent relative image
#define BImage_ParentRelative   (1l<<17)

#ifdef    INTERLACE
// fake interlaced image
#  define BImage_Interlaced	(1l<<18)
#endif // INTERLACE

class BColor {
private:
  int allocated;
  unsigned char red, green, blue;
  unsigned long pixel;

public:
  BColor(char r = 0, char g = 0, char b = 0)
    { red = r; green = g; blue = b; pixel = 0; allocated = 0; }

  inline const int isAllocated(void) const { return allocated; }

  inline const unsigned char getRed(void) const { return red; }
  inline const unsigned char getGreen(void) const { return green; }
  inline const unsigned char getBlue(void) const { return blue; }

  inline const unsigned long getPixel(void) const { return pixel; }

  inline void setAllocated(int a) { allocated = a; }
  inline void setRGB(char r, char g, char b) { red = r; green = g; blue = b; }
  inline void setPixel(unsigned long p) { pixel = p; }
};


class BTexture {
private:
  BColor color, colorTo, hiColor, loColor;
  unsigned long texture;

public:
  BTexture(void) { texture = 0; }

  inline BColor *getColor(void) { return &color; }
  inline BColor *getColorTo(void) { return &colorTo; }
  inline BColor *getHiColor(void) { return &hiColor; }
  inline BColor *getLoColor(void) { return &loColor; }

  inline const unsigned long getTexture(void) const { return texture; }

  inline void setTexture(unsigned long t) { texture = t; }
  inline void addTexture(unsigned long t) { texture |= t; }
};


class BImage {
private:
  BImageControl *control;

#ifdef    INTERLACE
  Bool interlaced;
#endif // INTERLACE

  XColor *colors;

  BColor *from, *to;
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

protected:
  Pixmap renderPixmap(void);

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
  BImage(BImageControl *c, unsigned int w, unsigned int h);
  ~BImage(void);

  Pixmap render(BTexture *texture);
  Pixmap render_solid(BTexture *texture);
  Pixmap render_gradient(BTexture *texture);
};


class BImageControl : public TimeoutHandler {
public:
  struct CachedImage {
    Pixmap pixmap;

    unsigned int count, width, height;
    unsigned long pixel1, pixel2, texture;
  };

  BImageControl(BaseDisplay *dpy, const ScreenInfo *scrn,
                Bool _dither= False, int _cpc = 4,
                unsigned long cache_timeout = 300000l,
                unsigned long cmax = 200l);
  virtual ~BImageControl(void);

  inline BaseDisplay *getBaseDisplay(void) const { return basedisplay; }

  inline const Bool doDither(void) { return dither; }

  inline const ScreenInfo *getScreenInfo(void) { return screeninfo; }

  inline const Window getDrawable(void) const { return window; }

  inline Visual *getVisual(void) { return screeninfo->getVisual(); }

  inline const int getBitsPerPixel(void) const { return bits_per_pixel; }
  inline const int getDepth(void) const { return screen_depth; }
  inline const int getColorsPerChannel(void) const
    { return colors_per_channel; }

  unsigned long getColor(const char *colorname);
  unsigned long getColor(const char *colorname, unsigned char *r,
                         unsigned char *g, unsigned char *b);
  unsigned long getSqrt(unsigned int x);

  Pixmap renderImage(unsigned int width, unsigned int height,
                     BTexture *texture);

  void installRootColormap(void);
  void removeImage(Pixmap pixmap);
  void getColorTables(unsigned char **rmt, unsigned char **gmt,
                      unsigned char **bmt,
                      int *roff, int *goff, int *boff,
                      int *rbit, int *gbit, int *bbit);
  void getXColorTable(XColor **c, int *n);
  void getGradientBuffers(unsigned int w, unsigned int h,
                          unsigned int **xbuf, unsigned int **ybuf);
  void setDither(Bool d) { dither = d; }
  void setColorsPerChannel(int cpc);
  void parseTexture(BTexture *texture, char *t);
  void parseColor(BColor *color, char *c = 0);

  virtual void timeout(void);

private:
  Bool dither;
  BaseDisplay *basedisplay;
  const ScreenInfo *screeninfo;
#ifdef    TIMEDCACHE
  BTimer *timer;
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

protected:
  Pixmap searchCache(unsigned int width, unsigned int height,
                     unsigned long texture, BColor *c1, BColor *c2);
};


#endif // __Image_hh

