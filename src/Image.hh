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

#ifndef   __Image_hh
#define   __Image_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>

class BaseDisplay;
class ScreenInfo;
class BImage;
class BImageControl;


class BColor {
private:
  int allocated;
  unsigned char red, green, blue;
  unsigned long pixel;

public:
  BColor(char r = 0, char g = 0, char b = 0)
    { red = r; green = g; blue = b; pixel = 0; allocated = 0; }

  int isAllocated(void) { return allocated; }

  unsigned char getRed(void) { return red; }
  unsigned char getGreen(void) { return green; }
  unsigned char getBlue(void) { return blue; }

  unsigned long getPixel(void) { return pixel; }

  void setAllocated(int a)
    { allocated = a; }
  void setRGB(char r, char g, char b)
    { red = r; green = g; blue = b; }
  void setPixel(unsigned long p)
    { pixel = p; }
};


class BTexture {
private:
  BColor color, colorTo, hiColor, loColor;
  unsigned long texture;

public:
  BTexture(void) { texture = 0; }

  BColor *getColor(void) { return &color; }
  BColor *getColorTo(void) { return &colorTo; }
  BColor *getHiColor(void) { return &hiColor; }
  BColor *getLoColor(void) { return &loColor; }

  unsigned long getTexture(void) { return texture; }
  
  void setTexture(unsigned long t) { texture = t; }
  void addTexture(unsigned long t) { texture |= t; }
};


#include "LinkedList.hh"

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

#ifdef    INTERLACE
// fake interlaced image
#  define BImage_Interlaced	(1l<<17)
#endif // INTERLACE


class BImage {
private:
  BImageControl *control;

#ifdef    INTERLACE
  Bool interlaced;
#endif // INTERLACE
  
  XColor *colors;

  BColor *from, *to;
  int red_offset, green_offset, blue_offset, ncolors, cpc, cpccpc;
  unsigned char *red, *green, *blue, *red_table, *green_table, *blue_table;
  unsigned int width, height, *xtable, *ytable;


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
  BImage(BImageControl *, unsigned int, unsigned int);
  ~BImage(void);

  Pixmap render(BTexture *);
  Pixmap render_solid(BTexture *);
  Pixmap render_gradient(BTexture *);
};

class BImageControl {
private:
  Bool dither;
  BaseDisplay *display;
  ScreenInfo *screeninfo;

  Colormap root_colormap;
  Window window;
  XColor *colors;
  int colors_per_channel, ncolors, screen_number, screen_depth,
    bits_per_pixel, red_offset, green_offset, blue_offset;
  unsigned char red_color_table[256], green_color_table[256],
    blue_color_table[256], red_error_table[256], green_error_table[256],
    blue_error_table[256], red_error38_table[256], green_error38_table[256],
    blue_error38_table[256];
  unsigned long *sqrt_table;
  
  short *red_buffer, *green_buffer, *blue_buffer, *next_red_buffer,
    *next_green_buffer, *next_blue_buffer;
  unsigned int dither_buffer_width;
  
  typedef struct Cache {
    Pixmap pixmap;

    unsigned int count, width, height;
    unsigned long pixel1, pixel2, texture;
  } Cache;

  LinkedList<Cache> *cache;


protected:
  Pixmap searchCache(unsigned int, unsigned int, unsigned long, BColor *,
                     BColor *);


public:
  BImageControl(BaseDisplay *, ScreenInfo *, Bool = False, int = 4);
  ~BImageControl(void);
  
  Bool doDither(void) { return dither; }
  
  ScreenInfo *getScreenInfo(void) { return screeninfo; }

  Colormap getColormap(void) { return root_colormap; }

  BaseDisplay *getDisplay(void) { return display; }

  Pixmap renderImage(unsigned int, unsigned int, BTexture *);

  Visual *getVisual(void);

  Window getDrawable(void) { return window; }

  int getBitsPerPixel(void) { return bits_per_pixel; }
  int getDepth(void) { return screen_depth; }
  int getColorsPerChannel(void) { return colors_per_channel; }

  unsigned long getColor(const char *);
  unsigned long getColor(const char *, unsigned char *, unsigned char *,
                         unsigned char *);
  unsigned long getSqrt(unsigned int);

  void installRootColormap(void);
  void removeImage(Pixmap);
  void getDitherBuffers(unsigned int,
			short **, short **, short **,
			short **, short **, short **,
			unsigned char **, unsigned char **, unsigned char **,
			unsigned char **, unsigned char **, unsigned char **);
  void getColorTables(unsigned char **, unsigned char **, unsigned char **,
		      int *, int *, int *);
  void getXColorTable(XColor **, int *);
  void setDither(Bool d) { dither = d; }
  void setColorsPerChannel(int);
  void parseTexture(BTexture *, char *);
  void parseColor(BColor *, char * = 0);
};


#endif // __Image_hh

