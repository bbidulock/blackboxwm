//
// Image.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 1999 by Brad Hughes, bhughes@tcac.net
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
class BScreen;

class BImage;
class BImageControl;

typedef struct BColor {
  int allocated;
  unsigned char red, green, blue;
  unsigned long pixel;
};

typedef struct BTexture {
  BColor color, colorTo, hiColor, loColor;
  unsigned long texture;
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


class BImage {
private:
  BImageControl *control;

  XColor *colors;

  BColor from, to;
  int roff, goff, boff, ncolors, cpc, cpccpc;
  unsigned char *red, *green, *blue;
  unsigned int width, height;
  unsigned short *tr, *tg, *tb;


protected:
  Pixmap renderPixmap(void);
  
  XImage *renderXImage(void);
  
  void invert(void);
  void bevel1(void);
  void bevel2(void);
  void dgradient(void);
  void hgradient(void);
  void vgradient(void);


public:
  BImage(BImageControl *, unsigned int, unsigned int);
  ~BImage(void);

  Pixmap render(const BTexture *          /* texture */);
  Pixmap render_solid(const BTexture *    /* texture */);
  Pixmap render_gradient(const BTexture * /* texture */);
};

class BImageControl {
private:
  Blackbox *blackbox;
  BScreen *screen;

  Colormap root_colormap;
  Display *display;
  Window window;
  XColor *colors;
  int colors_per_channel, ncolors, screen_number, screen_depth,
    bits_per_pixel, red_offset, green_offset, blue_offset;
  unsigned short rmask_table[256], gmask_table[256], bmask_table[256],
    rerr_table[256], gerr_table[256], berr_table[256];

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
  BImageControl(Blackbox *, BScreen *);
  ~BImageControl(void);


  Bool doDither(void);

  BScreen *getScreen(void) { return screen; }

  Colormap getColormap(void) { return root_colormap; }

  Display *getDisplay(void) { return display; }

  Pixmap renderImage(unsigned int, unsigned int, const BTexture *);

  Visual *getVisual(void);

  Window getDrawable(void) { return window; }

  int getBitsPerPixel(void) { return bits_per_pixel; }
  int getDepth(void) { return screen_depth; }
  int getColorsPerChannel(void) { return colors_per_channel; }

  unsigned long getColor(const char *);
  unsigned long getColor(const char *, unsigned char *, unsigned char *,
                         unsigned char *);

  void installRootColormap(void);
  void removeImage(Pixmap);
  void getDitherBuffers(unsigned int,
			short **, short **, short **,
			short **, short **, short **,
			unsigned short **, unsigned short **,
			unsigned short **);
  void getMaskTables(unsigned short **, unsigned short **, unsigned short **,
		     int *, int *, int *);
  void getColorTable(XColor **, int *);
};


#endif // __Image_hh

