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

#ifndef _blackbox_graphics_hh
#define _blackbox_graphics_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

typedef struct BColor {
  unsigned char r, g, b;
  unsigned long pixel;
} BColor;

#include "session.hh"


class BImage {
private:
  int depth;
  unsigned long *data;
  unsigned int width, height;

  BColor bg_color;
  BlackboxSession *session;


protected:
  void renderBevel(void);
  void renderButton(void);
  void invertImage(void);
  void renderDGradient(const BColor &, const BColor &);
  void renderHGradient(const BColor &, const BColor &);
  void renderVGradient(const BColor &, const BColor &);
  void setBackgroundColor(const BColor &);
  void setBackgroundColor(unsigned char, unsigned char, unsigned char);
  
  XImage *convertToXImage(void);
  Pixmap convertToPixmap(void);

  
public:
  BImage(BlackboxSession *, unsigned int, unsigned int, int,
	 unsigned char = 0, unsigned char = 0, unsigned char = 0);
  BImage(BlackboxSession *, unsigned int, unsigned int, int, const BColor &);
  ~BImage(void);

  Bool getPixel(unsigned int, unsigned int, unsigned long *);
  Bool getPixel(unsigned int, unsigned int, BColor &);
  Bool putPixel(unsigned int, unsigned int, unsigned long);
  Bool putPixel(unsigned int, unsigned int, const BColor &);
  
  Pixmap renderImage(int, int, const BColor &, const BColor &);
  Pixmap renderInvertedImage(int, int, const BColor &, const BColor &);
  Pixmap renderSolidImage(int, int, const BColor &);
  Pixmap renderGradientImage(int, int, const BColor &, const BColor &);
};


#endif
