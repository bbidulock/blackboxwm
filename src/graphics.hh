//
// graphics.hh for Blackbox - an X11 Window manager
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

#ifndef __blackbox_graphics_hh
#define __blackbox_graphics_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>

// forward declarations
class BImage;

typedef struct BColor {
  unsigned char r, g, b;
  unsigned long pixel;
} BColor;

class Blackbox;

// bevel options
#define BImageFlat            (1<<1)   // no bevel
#define BImageSunken          (1<<2)   // bevel resembles unpressed button
#define BImageRaised          (1<<3)   // bevel resembels pressed button

// texture types
#define BImageSolid           (1<<4)   // solid color image
#define BImageGradient        (1<<5)   // varied color image

// gradient types
#define BImageHorizontal      (1<<6)   // gradient varies on x axis
#define BImageVertical        (1<<7)   // gradient varies on y axis
#define BImageDiagonal        (1<<8)   // gradient varies on x-y axes

// bevel types
#define BImageBevel1          (1<<9)   // bevel on edge of image
#define BImageBevel2          (1<<10)  // bevel inset one pixel from image edge
#define BImageMotifBevel      (1<<11)  // Motif-like bevel (2 pixels wide)

// for inverting images
#define BImageInverted        (1<<12)  // for pressed buttons and the like...
                                       // currently only used internally
// for turning off dithering on solid images
#define BImageNoDitherSolid   (1<<13)  // to speed up certain window functions
                                       // currently only used internally

// image class that does basic rendering functions for blackbox
class BImage {
private:
  int depth;
  unsigned long *data;
  unsigned int width, height;
  unsigned short *or, *og, *ob, *nor, *nog, *nob;

  BColor bg_color;
  Blackbox *blackbox;


protected:
  void renderBevel1(Bool = True, Bool = False);
  void renderBevel2(Bool = True, Bool = False);
  void invertImage(void);
  void renderDGradient(const BColor &, const BColor &);
  void renderHGradient(const BColor &, const BColor &);
  void renderVGradient(const BColor &, const BColor &);
  void setBackgroundColor(const BColor &);
  void setBackgroundColor(unsigned char, unsigned char, unsigned char);
  
  XImage *convertToXImage(Bool = true);
  Pixmap convertToPixmap(Bool = true);

  
public:
  BImage(Blackbox *, unsigned int, unsigned int, int);
  ~BImage(void);

  Bool getPixel(unsigned int, unsigned int, unsigned long *);
  Bool getPixel(unsigned int, unsigned int, BColor &);
  Bool putPixel(unsigned int, unsigned int, unsigned long);
  Bool putPixel(unsigned int, unsigned int, const BColor &);
  
  Pixmap renderImage(unsigned long, const BColor &, const BColor &);
  Pixmap renderSolidImage(unsigned long, const BColor &);
  Pixmap renderGradientImage(unsigned long, const BColor &, const BColor &);
};


#endif
