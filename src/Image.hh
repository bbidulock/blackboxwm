//
// Image.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@arn.net
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// (See the included file COPYING / GPL-2.0)
//

#ifndef __Image_hh
#define __Image_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>

// forward declarations
class Blackbox;

class BImage;
class BImageControl;

typedef struct BColor {
  unsigned char red, green, blue;
  unsigned long pixel;
};

#include "LinkedList.hh"

// bevel options
#define BImage_Flat		(1<<1)
#define BImage_Sunken		(1<<2)
#define BImage_Raised		(1<<3)

// textures
#define BImage_Solid		(1<<4)
#define BImage_Gradient		(1<<5)

// gradients
#define BImage_Horizontal	(1<<6)
#define BImage_Vertical		(1<<7)
#define BImage_Diagonal		(1<<8)

// bevel types
#define BImage_Bevel1		(1<<9)
#define BImage_Bevel2		(1<<10)

// inverted image
#define BImage_Invert		(1<<11)

// don't dither large solids
#define BImage_NoDitherSolid	(1<<12)


class BImage {
private:
  BImageControl *control;

  XColor *colors;

  BColor bg, from, to;
  int roff, goff, boff, ncolors, cpc, cpccpc;
  unsigned char *red, *green, *blue;
  unsigned int width, height;
  unsigned short *tr, *tg, *tb;


protected:
  void invert(void);

  void bevel1(Bool = True, Bool = False);
  void bevel2(Bool = True, Bool = False);

  void dgradient(void);
  void hgradient(void);
  void vgradient(void);

  void background(const BColor &);

  XImage *renderXImage(Bool = True);
  Pixmap renderPixmap(Bool = True);


public:
  BImage(BImageControl *, unsigned int, unsigned int);
  ~BImage(void);

  Pixmap render(unsigned long, const BColor &, const BColor &);
  Pixmap render_solid(unsigned long, const BColor &);
  Pixmap render_gradient(unsigned long, const BColor &, const BColor &);
};

class BImageControl {
private:
  Blackbox *blackbox;

  Colormap root_colormap;
  Display *display;
  Window window; // == blackbox->Root();
  XColor *colors;
  int colors_per_channel, ncolors, screen_number, screen_depth,
    bits_per_pixel, red_offset, green_offset, blue_offset;
  unsigned short rmask_table[256], gmask_table[256], bmask_table[256];

  unsigned int dither_buf_width;
  short *red_err, *green_err, *blue_err, *next_red_err, *next_green_err,
    *next_blue_err;

  typedef struct Cache {
    Pixmap pixmap;

    unsigned int count, width, height;
    unsigned long pixel1, pixel2, texture;
  } Cache;

  LinkedList<Cache> *cache;

protected:
  Pixmap searchCache(unsigned int, unsigned int, unsigned long, const BColor &,
                     const BColor &);

public:
  BImageControl(Blackbox *);
  ~BImageControl(void);

  // user configurable information
  Bool dither(void);

  // details
  Display *d(void) { return display; }
  Visual *v(void);
  Window drawable(void) { return window; }
  int bpp(void) { return bits_per_pixel; }
  int depth(void) { return screen_depth; }
  int screen(void) { return screen_number; }
  int colorsPerChannel(void) { return colors_per_channel; }

  unsigned long getColor(const char *);
  unsigned long getColor(const char *, unsigned char *, unsigned char *,
                         unsigned char *);
  void installRootColormap(void);

  // image cache/render requests
  Pixmap renderImage(unsigned int, unsigned int, unsigned long, const BColor &,
                     const BColor &);
  void removeImage(Pixmap);

  // dither buffers
  void getDitherBuffers(unsigned int, short **, short **, short **, short **,
			short **, short **);

  // rgb mask color lookup tables
  void getMaskTables(unsigned short **, unsigned short **, unsigned short **,
		     int *, int *, int *);

  // allocated colors
  void getColorTable(XColor **, int *);
};


#endif // __Image_hh

