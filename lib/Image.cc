// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Image.cc for Blackbox - an X11 Window manager
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <stdio.h>
}

#include <algorithm>

#include "BaseDisplay.hh"
#include "GCCache.hh"
#include "Image.hh"
#include "Texture.hh"


bt::Image::Image(bt::ImageControl *c, int w, int h) {
  control = c;

  width = (w > 0) ? w : 1;
  height = (h > 0) ? h : 1;

  red = new unsigned char[width * height];
  green = new unsigned char[width * height];
  blue = new unsigned char[width * height];

  xtable = ytable = (unsigned int *) 0;

  cpc = control->getColorsPerChannel();
  cpccpc = cpc * cpc;

  control->getColorTables(&red_table, &green_table, &blue_table,
                          &red_offset, &green_offset, &blue_offset,
                          &red_bits, &green_bits, &blue_bits);

  if (control->getVisual()->c_class != TrueColor)
    control->getXColorTable(&colors, &ncolors);
}


bt::Image::~Image(void) {
  delete [] red;
  delete [] green;
  delete [] blue;
}


Pixmap bt::Image::render(const bt::Texture &texture) {
  if (texture.texture() & bt::Texture::Parent_Relative)
    return ParentRelative;
  else if (texture.texture() & bt::Texture::Solid)
    return render_solid(texture);
  else if (texture.texture() & bt::Texture::Gradient)
    return render_gradient(texture);
  return None;
}


Pixmap bt::Image::render_solid(const bt::Texture &texture) {
  Pixmap pixmap = XCreatePixmap(control->getDisplay()->getXDisplay(),
				control->getDrawable(), width,
				height, control->getDepth());
  if (pixmap == None)
    return None;

  ::Display *display = control->getDisplay()->getXDisplay();

  bt::Pen pen(texture.color());
  bt::Pen penlight(texture.lightColor());
  bt::Pen penshadow(texture.shadowColor());

  XFillRectangle(display, pixmap, pen.gc(), 0, 0, width, height);

  if (texture.texture() & bt::Texture::Interlaced) {
    bt::Pen peninterlace(texture.colorTo());
    for (unsigned int i = 0; i < height; i += 2)
      XDrawLine(display, pixmap, peninterlace.gc(), 0, i, width, i);
  }

  if (texture.texture() & bt::Texture::Bevel1) {
    if (texture.texture() & bt::Texture::Raised) {
      XDrawLine(display, pixmap, penshadow.gc(),
                0, height - 1, width - 1, height - 1);
      XDrawLine(display, pixmap, penshadow.gc(),
                width - 1, height - 1, width - 1, 0);

      XDrawLine(display, pixmap, penlight.gc(),
                0, 0, width - 1, 0);
      XDrawLine(display, pixmap, penlight.gc(),
                0, height - 1, 0, 0);
    } else if (texture.texture() & bt::Texture::Sunken) {
      XDrawLine(display, pixmap, penlight.gc(),
                0, height - 1, width - 1, height - 1);
      XDrawLine(display, pixmap, penlight.gc(),
                width - 1, height - 1, width - 1, 0);

      XDrawLine(display, pixmap, penshadow.gc(),
                0, 0, width - 1, 0);
      XDrawLine(display, pixmap, penshadow.gc(),
                0, height - 1, 0, 0);
    }
  } else if (texture.texture() & bt::Texture::Bevel2) {
    if (texture.texture() & bt::Texture::Raised) {
      XDrawLine(display, pixmap, penshadow.gc(),
                1, height - 3, width - 3, height - 3);
      XDrawLine(display, pixmap, penshadow.gc(),
                width - 3, height - 3, width - 3, 1);

      XDrawLine(display, pixmap, penlight.gc(),
                1, 1, width - 3, 1);
      XDrawLine(display, pixmap, penlight.gc(),
                1, height - 3, 1, 1);
    } else if (texture.texture() & bt::Texture::Sunken) {
      XDrawLine(display, pixmap, penlight.gc(),
                1, height - 3, width - 3, height - 3);
      XDrawLine(display, pixmap, penlight.gc(),
                width - 3, height - 3, width - 3, 1);

      XDrawLine(display, pixmap, penshadow.gc(),
                1, 1, width - 3, 1);
      XDrawLine(display, pixmap, penshadow.gc(),
                1, height - 3, 1, 1);
    }
  }

  return pixmap;
}


Pixmap bt::Image::render_gradient(const bt::Texture &texture) {
  bool inverted = False;

  interlaced = texture.texture() & bt::Texture::Interlaced;

  if (texture.texture() & bt::Texture::Sunken) {
    from = texture.colorTo();
    to = texture.color();

    if (! (texture.texture() & bt::Texture::Invert)) inverted = True;
  } else {
    from = texture.color();
    to = texture.colorTo();

    if (texture.texture() & bt::Texture::Invert) inverted = True;
  }

  control->getGradientBuffers(width, height, &xtable, &ytable);

  if (texture.texture() & bt::Texture::Diagonal) dgradient();
  else if (texture.texture() & bt::Texture::Elliptic) egradient();
  else if (texture.texture() & bt::Texture::Horizontal) hgradient();
  else if (texture.texture() & bt::Texture::Pyramid) pgradient();
  else if (texture.texture() & bt::Texture::Rectangle) rgradient();
  else if (texture.texture() & bt::Texture::Vertical) vgradient();
  else if (texture.texture() & bt::Texture::CrossDiagonal) cdgradient();
  else if (texture.texture() & bt::Texture::PipeCross) pcgradient();

  if (texture.texture() & bt::Texture::Bevel1) bevel1();
  else if (texture.texture() & bt::Texture::Bevel2) bevel2();

  if (inverted) invert();

  return renderPixmap();
}


static const unsigned char dither4[4][4] = {
  {0, 4, 1, 5},
  {6, 2, 7, 3},
  {1, 5, 0, 4},
  {7, 3, 6, 2}
};


/*
 * Helper function for TrueColorDither and renderXImage
 *
 * This handles the proper setting of the image data based on the image depth
 * and the machine's byte ordering
 */
static inline
void assignPixelData(unsigned int bit_depth, unsigned char **data,
		     unsigned long pixel) {
  unsigned char *pixel_data = *data;
  switch (bit_depth) {
  case  8: //  8bpp
    *pixel_data++ = pixel;
    break;

  case 16: // 16bpp LSB
    *pixel_data++ = pixel;
    *pixel_data++ = pixel >> 8;
    break;

  case 17: // 16bpp MSB
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel;
    break;

  case 24: // 24bpp LSB
    *pixel_data++ = pixel;
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel >> 16;
    break;

  case 25: // 24bpp MSB
    *pixel_data++ = pixel >> 16;
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel;
    break;

  case 32: // 32bpp LSB
    *pixel_data++ = pixel;
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel >> 16;
    *pixel_data++ = pixel >> 24;
    break;

  case 33: // 32bpp MSB
    *pixel_data++ = pixel >> 24;
    *pixel_data++ = pixel >> 16;
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel;
    break;
  }
  *data = pixel_data; // assign back so we don't lose our place
}


// algorithm: ordered dithering... many many thanks to rasterman
// (raster@rasterman.com) for telling me about this... portions of this
// code is based off of his code in Imlib
void bt::Image::TrueColorDither(unsigned int bit_depth, int bytes_per_line,
                                unsigned char *pixel_data) {
  unsigned int x, y, dithx, dithy, r, g, b, er, eg, eb, offset;
  unsigned char *ppixel_data = pixel_data;
  unsigned long pixel;

  for (y = 0, offset = 0; y < height; y++) {
    dithy = y & 0x3;

    for (x = 0; x < width; x++, offset++) {
      dithx = x & 0x3;
      r = red[offset];
      g = green[offset];
      b = blue[offset];

      er = r & (red_bits - 1);
      eg = g & (green_bits - 1);
      eb = b & (blue_bits - 1);

      r = red_table[r];
      g = green_table[g];
      b = blue_table[b];

      if ((dither4[dithy][dithx] < er) && (r < red_table[255])) r++;
      if ((dither4[dithy][dithx] < eg) && (g < green_table[255])) g++;
      if ((dither4[dithy][dithx] < eb) && (b < blue_table[255])) b++;

      pixel = (r << red_offset) | (g << green_offset) | (b << blue_offset);
      assignPixelData(bit_depth, &pixel_data, pixel);
    }

    pixel_data = (ppixel_data += bytes_per_line);
  }
}

#ifdef ORDEREDPSEUDO
const static unsigned char dither8[8][8] = {
  { 0,  32, 8,  40, 2,  34, 10, 42},
  { 48, 16, 56, 24, 50, 18, 58, 26},
  { 12, 44, 4,  36, 14, 46, 6,  38},
  { 60, 28, 52, 20, 62, 30, 54, 22},
  { 3,  35, 11, 43, 1,  33, 9,  41},
  { 51, 19, 59, 27, 49, 17, 57, 25},
  { 15, 47, 7,  39, 13, 45, 5,  37},
  { 63, 31, 55, 23, 61, 29, 53, 21}
};

void bt::Image::OrderedPseudoColorDither(int bytes_per_line,
                                         unsigned char *pixel_data) {
  unsigned int x, y, dithx, dithy, r, g, b, er, eg, eb, offset;
  unsigned long pixel;
  unsigned char *ppixel_data = pixel_data;

  for (y = 0, offset = 0; y < height; y++) {
    dithy = y & 7;

    for (x = 0; x < width; x++, offset++) {
      dithx = x & 7;

      r = red[offset];
      g = green[offset];
      b = blue[offset];

      er = r & (red_bits - 1);
      eg = g & (green_bits - 1);
      eb = b & (blue_bits - 1);

      r = red_table[r];
      g = green_table[g];
      b = blue_table[b];

      if ((dither8[dithy][dithx] < er) && (r < red_table[255])) r++;
      if ((dither8[dithy][dithx] < eg) && (g < green_table[255])) g++;
      if ((dither8[dithy][dithx] < eb) && (b < blue_table[255])) b++;

      pixel = (r * cpccpc) + (g * cpc) + b;
      *(pixel_data++) = colors[pixel].pixel;
    }

    pixel_data = (ppixel_data += bytes_per_line);
  }
}
#endif

void bt::Image::PseudoColorDither(int bytes_per_line,
                                  unsigned char *pixel_data) {
  short *terr,
    *rerr = new short[width + 2],
    *gerr = new short[width + 2],
    *berr = new short[width + 2],
    *nrerr = new short[width + 2],
    *ngerr = new short[width + 2],
    *nberr = new short[width + 2];

  int rr, gg, bb, rer, ger, ber;
  int dd = 255 / control->getColorsPerChannel();
  unsigned int x, y, r, g, b, offset;
  unsigned long pixel;
  unsigned char *ppixel_data = pixel_data;

  for (x = 0; x < width; x++) {
    *(rerr + x) = *(red + x);
    *(gerr + x) = *(green + x);
    *(berr + x) = *(blue + x);
  }

  *(rerr + x) = *(gerr + x) = *(berr + x) = 0;

  for (y = 0, offset = 0; y < height; y++) {
    if (y < (height - 1)) {
      int i = offset + width;
      for (x = 0; x < width; x++, i++) {
	*(nrerr + x) = *(red + i);
	*(ngerr + x) = *(green + i);
	*(nberr + x) = *(blue + i);
      }

      *(nrerr + x) = *(red + (--i));
      *(ngerr + x) = *(green + i);
      *(nberr + x) = *(blue + i);
    }

    for (x = 0; x < width; x++) {
      rr = rerr[x];
      gg = gerr[x];
      bb = berr[x];

      if (rr > 255) rr = 255; else if (rr < 0) rr = 0;
      if (gg > 255) gg = 255; else if (gg < 0) gg = 0;
      if (bb > 255) bb = 255; else if (bb < 0) bb = 0;

      r = red_table[rr];
      g = green_table[gg];
      b = blue_table[bb];

      rer = rerr[x] - r*dd;
      ger = gerr[x] - g*dd;
      ber = berr[x] - b*dd;

      pixel = (r * cpccpc) + (g * cpc) + b;
      *pixel_data++ = colors[pixel].pixel;

      r = rer >> 1;
      g = ger >> 1;
      b = ber >> 1;
      rerr[x+1] += r;
      gerr[x+1] += g;
      berr[x+1] += b;
      nrerr[x] += r;
      ngerr[x] += g;
      nberr[x] += b;
    }

    offset += width;

    pixel_data = (ppixel_data += bytes_per_line);

    terr = rerr;
    rerr = nrerr;
    nrerr = terr;

    terr = gerr;
    gerr = ngerr;
    ngerr = terr;

    terr = berr;
    berr = nberr;
    nberr = terr;
  }

  delete [] rerr;
  delete [] gerr;
  delete [] berr;
  delete [] nrerr;
  delete [] ngerr;
  delete [] nberr;
}


XImage *bt::Image::renderXImage(void) {
  XImage *image =
    XCreateImage(control->getDisplay()->getXDisplay(),
                 control->getVisual(), control->getDepth(), ZPixmap, 0, 0,
                 width, height, 32, 0);

  if (! image)
    return (XImage *) 0;

  // insurance policy
  image->data = (char *) 0;

  unsigned char *d = new unsigned char[image->bytes_per_line * (height + 1)];

  unsigned int o = image->bits_per_pixel +
    ((image->byte_order == MSBFirst) ? 1 : 0);

  bool unsupported = False;

  if (control->doDither() && width > 1 && height > 1) {
    switch (control->getVisual()->c_class) {
    case TrueColor:
      TrueColorDither(o, image->bytes_per_line, d);
      break;

    case StaticColor:
    case PseudoColor: {
#ifdef ORDEREDPSEUDO
      OrderedPseudoColorDither(image->bytes_per_line, d);
#else
      PseudoColorDither(image->bytes_per_line, d);
#endif
      break;
    }

    default:
      unsupported = True;
    }
  } else {
    unsigned int x, y, r, g, b, offset;
    unsigned char *pixel_data = d, *ppixel_data = d;
    unsigned long pixel;

    switch (control->getVisual()->c_class) {
    case StaticColor:
    case PseudoColor:
      for (y = 0, offset = 0; y < height; ++y) {
        for (x = 0; x < width; ++x, ++offset) {
  	  r = red_table[red[offset]];
          g = green_table[green[offset]];
	  b = blue_table[blue[offset]];

	  pixel = (r * cpccpc) + (g * cpc) + b;
	  *pixel_data++ = colors[pixel].pixel;
        }

        pixel_data = (ppixel_data += image->bytes_per_line);
      }

      break;

    case TrueColor:
      for (y = 0, offset = 0; y < height; y++) {
        for (x = 0; x < width; x++, offset++) {
	  r = red_table[red[offset]];
	  g = green_table[green[offset]];
	  b = blue_table[blue[offset]];

	  pixel = (r << red_offset) | (g << green_offset) | (b << blue_offset);
	  assignPixelData(o, &pixel_data, pixel);
        }

        pixel_data = (ppixel_data += image->bytes_per_line);
      }

      break;

    case StaticGray:
    case GrayScale:
      for (y = 0, offset = 0; y < height; y++) {
	for (x = 0; x < width; x++, offset++) {
	  r = *(red_table + *(red + offset));
	  g = *(green_table + *(green + offset));
	  b = *(blue_table + *(blue + offset));

	  g = ((r * 30) + (g * 59) + (b * 11)) / 100;
	  *pixel_data++ = colors[g].pixel;
	}

	pixel_data = (ppixel_data += image->bytes_per_line);
      }

      break;

    default:
      unsupported = True;
    }
  }

  if (unsupported) {
    delete [] d;
    XDestroyImage(image);
    return (XImage *) 0;
  }

  image->data = (char *) d;

  return image;
}


Pixmap bt::Image::renderPixmap(void) {
  Pixmap pixmap =
    XCreatePixmap(control->getDisplay()->getXDisplay(),
                  control->getDrawable(), width, height, control->getDepth());

  if (pixmap == None)
    return None;

  XImage *image = renderXImage();

  if (! image) {
    XFreePixmap(control->getDisplay()->getXDisplay(), pixmap);
    return None;
  }

  if (! image->data) {
    XDestroyImage(image);
    XFreePixmap(control->getDisplay()->getXDisplay(), pixmap);
    return None;
  }

  XPutImage(control->getDisplay()->getXDisplay(), pixmap,
	    DefaultGC(control->getDisplay()->getXDisplay(),
		      control->getScreenInfo()->getScreenNumber()),
            image, 0, 0, 0, 0, width, height);

  if (image->data) {
    delete [] image->data;
    image->data = NULL;
  }

  XDestroyImage(image);

  return pixmap;
}


void bt::Image::bevel1(void) {
  if (width > 2 && height > 2) {
    unsigned char *pr = red, *pg = green, *pb = blue;

    register unsigned char r, g, b, rr ,gg ,bb;
    register unsigned int w = width, h = height - 1, wh = w * h;

    while (--w) {
      r = *pr;
      rr = r + (r >> 1);
      if (rr < r) rr = ~0;
      g = *pg;
      gg = g + (g >> 1);
      if (gg < g) gg = ~0;
      b = *pb;
      bb = b + (b >> 1);
      if (bb < b) bb = ~0;

      *pr = rr;
      *pg = gg;
      *pb = bb;

      r = *(pr + wh);
      rr = (r >> 2) + (r >> 1);
      if (rr > r) rr = 0;
      g = *(pg + wh);
      gg = (g >> 2) + (g >> 1);
      if (gg > g) gg = 0;
      b = *(pb + wh);
      bb = (b >> 2) + (b >> 1);
      if (bb > b) bb = 0;

      *((pr++) + wh) = rr;
      *((pg++) + wh) = gg;
      *((pb++) + wh) = bb;
    }

    r = *pr;
    rr = r + (r >> 1);
    if (rr < r) rr = ~0;
    g = *pg;
    gg = g + (g >> 1);
    if (gg < g) gg = ~0;
    b = *pb;
    bb = b + (b >> 1);
    if (bb < b) bb = ~0;

    *pr = rr;
    *pg = gg;
    *pb = bb;

    r = *(pr + wh);
    rr = (r >> 2) + (r >> 1);
    if (rr > r) rr = 0;
    g = *(pg + wh);
    gg = (g >> 2) + (g >> 1);
    if (gg > g) gg = 0;
    b = *(pb + wh);
    bb = (b >> 2) + (b >> 1);
    if (bb > b) bb = 0;

    *(pr + wh) = rr;
    *(pg + wh) = gg;
    *(pb + wh) = bb;

    pr = red + width;
    pg = green + width;
    pb = blue + width;

    while (--h) {
      r = *pr;
      rr = r + (r >> 1);
      if (rr < r) rr = ~0;
      g = *pg;
      gg = g + (g >> 1);
      if (gg < g) gg = ~0;
      b = *pb;
      bb = b + (b >> 1);
      if (bb < b) bb = ~0;

      *pr = rr;
      *pg = gg;
      *pb = bb;

      pr += width - 1;
      pg += width - 1;
      pb += width - 1;

      r = *pr;
      rr = (r >> 2) + (r >> 1);
      if (rr > r) rr = 0;
      g = *pg;
      gg = (g >> 2) + (g >> 1);
      if (gg > g) gg = 0;
      b = *pb;
      bb = (b >> 2) + (b >> 1);
      if (bb > b) bb = 0;

      *(pr++) = rr;
      *(pg++) = gg;
      *(pb++) = bb;
    }

    r = *pr;
    rr = r + (r >> 1);
    if (rr < r) rr = ~0;
    g = *pg;
    gg = g + (g >> 1);
    if (gg < g) gg = ~0;
    b = *pb;
    bb = b + (b >> 1);
    if (bb < b) bb = ~0;

    *pr = rr;
    *pg = gg;
    *pb = bb;

    pr += width - 1;
    pg += width - 1;
    pb += width - 1;

    r = *pr;
    rr = (r >> 2) + (r >> 1);
    if (rr > r) rr = 0;
    g = *pg;
    gg = (g >> 2) + (g >> 1);
    if (gg > g) gg = 0;
    b = *pb;
    bb = (b >> 2) + (b >> 1);
    if (bb > b) bb = 0;

    *pr = rr;
    *pg = gg;
    *pb = bb;
  }
}


void bt::Image::bevel2(void) {
  if (width > 4 && height > 4) {
    unsigned char r, g, b, rr ,gg ,bb, *pr = red + width + 1,
      *pg = green + width + 1, *pb = blue + width + 1;
    unsigned int w = width - 2, h = height - 1, wh = width * (height - 3);

    while (--w) {
      r = *pr;
      rr = r + (r >> 1);
      if (rr < r) rr = ~0;
      g = *pg;
      gg = g + (g >> 1);
      if (gg < g) gg = ~0;
      b = *pb;
      bb = b + (b >> 1);
      if (bb < b) bb = ~0;

      *pr = rr;
      *pg = gg;
      *pb = bb;

      r = *(pr + wh);
      rr = (r >> 2) + (r >> 1);
      if (rr > r) rr = 0;
      g = *(pg + wh);
      gg = (g >> 2) + (g >> 1);
      if (gg > g) gg = 0;
      b = *(pb + wh);
      bb = (b >> 2) + (b >> 1);
      if (bb > b) bb = 0;

      *((pr++) + wh) = rr;
      *((pg++) + wh) = gg;
      *((pb++) + wh) = bb;
    }

    pr = red + width;
    pg = green + width;
    pb = blue + width;

    while (--h) {
      r = *pr;
      rr = r + (r >> 1);
      if (rr < r) rr = ~0;
      g = *pg;
      gg = g + (g >> 1);
      if (gg < g) gg = ~0;
      b = *pb;
      bb = b + (b >> 1);
      if (bb < b) bb = ~0;

      *(++pr) = rr;
      *(++pg) = gg;
      *(++pb) = bb;

      pr += width - 3;
      pg += width - 3;
      pb += width - 3;

      r = *pr;
      rr = (r >> 2) + (r >> 1);
      if (rr > r) rr = 0;
      g = *pg;
      gg = (g >> 2) + (g >> 1);
      if (gg > g) gg = 0;
      b = *pb;
      bb = (b >> 2) + (b >> 1);
      if (bb > b) bb = 0;

      *(pr++) = rr;
      *(pg++) = gg;
      *(pb++) = bb;

      pr++; pg++; pb++;
    }
  }
}


void bt::Image::invert(void) {
  register unsigned int i, j, wh = (width * height) - 1;
  unsigned char tmp;

  for (i = 0, j = wh; j > i; j--, i++) {
    tmp = *(red + j);
    *(red + j) = *(red + i);
    *(red + i) = tmp;

    tmp = *(green + j);
    *(green + j) = *(green + i);
    *(green + i) = tmp;

    tmp = *(blue + j);
    *(blue + j) = *(blue + i);
    *(blue + i) = tmp;
  }
}


void bt::Image::dgradient(void) {
  // diagonal gradient code was written by Mike Cole <mike@mydot.com>
  // modified for interlacing by Brad Hughes

  float drx, dgx, dbx, dry, dgy, dby, yr = 0.0, yg = 0.0, yb = 0.0,
    xr = (float) from.red(),
    xg = (float) from.green(),
    xb = (float) from.blue();
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int w = width * 2, h = height * 2, *xt = xtable, *yt = ytable;

  register unsigned int x, y;

  dry = drx = (float) (to.red() - from.red());
  dgy = dgx = (float) (to.green() - from.green());
  dby = dbx = (float) (to.blue() - from.blue());

  // Create X table
  drx /= w;
  dgx /= w;
  dbx /= w;

  for (x = 0; x < width; x++) {
    *(xt++) = (unsigned char) (xr);
    *(xt++) = (unsigned char) (xg);
    *(xt++) = (unsigned char) (xb);

    xr += drx;
    xg += dgx;
    xb += dbx;
  }

  // Create Y table
  dry /= h;
  dgy /= h;
  dby /= h;

  for (y = 0; y < height; y++) {
    *(yt++) = ((unsigned char) yr);
    *(yt++) = ((unsigned char) yg);
    *(yt++) = ((unsigned char) yb);

    yr += dry;
    yg += dgy;
    yb += dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal dgradient
    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        *(pr++) = *(xt++) + *(yt);
        *(pg++) = *(xt++) + *(yt + 1);
        *(pb++) = *(xt++) + *(yt + 2);
      }
    }
  } else {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        if (y & 1) {
          channel = *(xt++) + *(yt);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = *(xt++) + *(yt + 1);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = *(xt++) + *(yt + 2);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        } else {
          channel = *(xt++) + *(yt);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = *(xt++) + *(yt + 1);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = *(xt++) + *(yt + 2);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
}


void bt::Image::hgradient(void) {
  float drx, dgx, dbx,
    xr = (float) from.red(),
    xg = (float) from.green(),
    xb = (float) from.blue();
  unsigned char *pr = red, *pg = green, *pb = blue;

  register unsigned int x, y;

  drx = (float) (to.red() - from.red());
  dgx = (float) (to.green() - from.green());
  dbx = (float) (to.blue() - from.blue());

  drx /= width;
  dgx /= width;
  dbx /= width;

  if (interlaced && height > 2) {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (x = 0; x < width; x++, pr++, pg++, pb++) {
      channel = (unsigned char) xr;
      channel2 = (channel >> 1) + (channel >> 2);
      if (channel2 > channel) channel2 = 0;
      *pr = channel2;

      channel = (unsigned char) xg;
      channel2 = (channel >> 1) + (channel >> 2);
      if (channel2 > channel) channel2 = 0;
      *pg = channel2;

      channel = (unsigned char) xb;
      channel2 = (channel >> 1) + (channel >> 2);
      if (channel2 > channel) channel2 = 0;
      *pb = channel2;


      channel = (unsigned char) xr;
      channel2 = channel + (channel >> 3);
      if (channel2 < channel) channel2 = ~0;
      *(pr + width) = channel2;

      channel = (unsigned char) xg;
      channel2 = channel + (channel >> 3);
      if (channel2 < channel) channel2 = ~0;
      *(pg + width) = channel2;

      channel = (unsigned char) xb;
      channel2 = channel + (channel >> 3);
      if (channel2 < channel) channel2 = ~0;
      *(pb + width) = channel2;

      xr += drx;
      xg += dgx;
      xb += dbx;
    }

    pr += width;
    pg += width;
    pb += width;

    int offset;

    for (y = 2; y < height; y++, pr += width, pg += width, pb += width) {
      if (y & 1) offset = width; else offset = 0;

      memcpy(pr, (red + offset), width);
      memcpy(pg, (green + offset), width);
      memcpy(pb, (blue + offset), width);
    }
  } else {
    // normal hgradient
    for (x = 0; x < width; x++) {
      *(pr++) = (unsigned char) (xr);
      *(pg++) = (unsigned char) (xg);
      *(pb++) = (unsigned char) (xb);

      xr += drx;
      xg += dgx;
      xb += dbx;
    }

    for (y = 1; y < height; y++, pr += width, pg += width, pb += width) {
      memcpy(pr, red, width);
      memcpy(pg, green, width);
      memcpy(pb, blue, width);
    }
  }
}


void bt::Image::vgradient(void) {
  float dry, dgy, dby,
    yr = (float) from.red(),
    yg = (float) from.green(),
    yb = (float) from.blue();
  unsigned char *pr = red, *pg = green, *pb = blue;

  register unsigned int y;

  dry = (float) (to.red() - from.red());
  dgy = (float) (to.green() - from.green());
  dby = (float) (to.blue() - from.blue());

  dry /= height;
  dgy /= height;
  dby /= height;

  if (interlaced) {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (y = 0; y < height; y++, pr += width, pg += width, pb += width) {
      if (y & 1) {
        channel = (unsigned char) yr;
        channel2 = (channel >> 1) + (channel >> 2);
        if (channel2 > channel) channel2 = 0;
        memset(pr, channel2, width);

        channel = (unsigned char) yg;
        channel2 = (channel >> 1) + (channel >> 2);
        if (channel2 > channel) channel2 = 0;
        memset(pg, channel2, width);

        channel = (unsigned char) yb;
        channel2 = (channel >> 1) + (channel >> 2);
        if (channel2 > channel) channel2 = 0;
        memset(pb, channel2, width);
      } else {
        channel = (unsigned char) yr;
        channel2 = channel + (channel >> 3);
        if (channel2 < channel) channel2 = ~0;
        memset(pr, channel2, width);

        channel = (unsigned char) yg;
        channel2 = channel + (channel >> 3);
        if (channel2 < channel) channel2 = ~0;
        memset(pg, channel2, width);

        channel = (unsigned char) yb;
        channel2 = channel + (channel >> 3);
        if (channel2 < channel) channel2 = ~0;
        memset(pb, channel2, width);
      }

      yr += dry;
      yg += dgy;
      yb += dby;
    }
  } else {
    // normal vgradient
    for (y = 0; y < height; y++, pr += width, pg += width, pb += width) {
      memset(pr, (unsigned char) yr, width);
      memset(pg, (unsigned char) yg, width);
      memset(pb, (unsigned char) yb, width);

      yr += dry;
      yg += dgy;
      yb += dby;
    }
  }
}


void bt::Image::pgradient(void) {
  // pyramid gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  float yr, yg, yb, drx, dgx, dbx, dry, dgy, dby,
    xr, xg, xb;
  int rsign, gsign, bsign;
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int tr = to.red(), tg = to.green(), tb = to.blue(),
    *xt = xtable, *yt = ytable;

  register unsigned int x, y;

  dry = drx = (float) (to.red() - from.red());
  dgy = dgx = (float) (to.green() - from.green());
  dby = dbx = (float) (to.blue() - from.blue());

  rsign = (drx < 0) ? -1 : 1;
  gsign = (dgx < 0) ? -1 : 1;
  bsign = (dbx < 0) ? -1 : 1;

  xr = yr = (drx / 2);
  xg = yg = (dgx / 2);
  xb = yb = (dbx / 2);

  // Create X table
  drx /= width;
  dgx /= width;
  dbx /= width;

  for (x = 0; x < width; x++) {
    *(xt++) = (unsigned char) ((xr < 0) ? -xr : xr);
    *(xt++) = (unsigned char) ((xg < 0) ? -xg : xg);
    *(xt++) = (unsigned char) ((xb < 0) ? -xb : xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; y++) {
    *(yt++) = ((unsigned char) ((yr < 0) ? -yr : yr));
    *(yt++) = ((unsigned char) ((yg < 0) ? -yg : yg));
    *(yt++) = ((unsigned char) ((yb < 0) ? -yb : yb));

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal pgradient
    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        *(pr++) = (unsigned char) (tr - (rsign * (*(xt++) + *(yt))));
        *(pg++) = (unsigned char) (tg - (gsign * (*(xt++) + *(yt + 1))));
        *(pb++) = (unsigned char) (tb - (bsign * (*(xt++) + *(yt + 2))));
      }
    }
  } else {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        if (y & 1) {
          channel = (unsigned char) (tr - (rsign * (*(xt++) + *(yt))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = (unsigned char) (tg - (gsign * (*(xt++) + *(yt + 1))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = (unsigned char) (tb - (bsign * (*(xt++) + *(yt + 2))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        } else {
          channel = (unsigned char) (tr - (rsign * (*(xt++) + *(yt))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = (unsigned char) (tg - (gsign * (*(xt++) + *(yt + 1))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = (unsigned char) (tb - (bsign * (*(xt++) + *(yt + 2))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
}


void bt::Image::rgradient(void) {
  // rectangle gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  float drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
  int rsign, gsign, bsign;
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int tr = to.red(), tg = to.green(), tb = to.blue(),
    *xt = xtable, *yt = ytable;

  register unsigned int x, y;

  dry = drx = (float) (to.red() - from.red());
  dgy = dgx = (float) (to.green() - from.green());
  dby = dbx = (float) (to.blue() - from.blue());

  rsign = (drx < 0) ? -2 : 2;
  gsign = (dgx < 0) ? -2 : 2;
  bsign = (dbx < 0) ? -2 : 2;

  xr = yr = (drx / 2);
  xg = yg = (dgx / 2);
  xb = yb = (dbx / 2);

  // Create X table
  drx /= width;
  dgx /= width;
  dbx /= width;

  for (x = 0; x < width; x++) {
    *(xt++) = (unsigned char) ((xr < 0) ? -xr : xr);
    *(xt++) = (unsigned char) ((xg < 0) ? -xg : xg);
    *(xt++) = (unsigned char) ((xb < 0) ? -xb : xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; y++) {
    *(yt++) = ((unsigned char) ((yr < 0) ? -yr : yr));
    *(yt++) = ((unsigned char) ((yg < 0) ? -yg : yg));
    *(yt++) = ((unsigned char) ((yb < 0) ? -yb : yb));

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal rgradient
    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        *(pr++) = (unsigned char) (tr - (rsign * std::max(*(xt++), *(yt))));
        *(pg++) = (unsigned char) (tg - (gsign * std::max(*(xt++),
                                                          *(yt + 1))));
        *(pb++) = (unsigned char) (tb - (bsign * std::max(*(xt++),
                                                          *(yt + 2))));
      }
    }
  } else {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        if (y & 1) {
          channel = (unsigned char) (tr - (rsign * std::max(*(xt++), *(yt))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = (unsigned char) (tg - (gsign * std::max(*(xt++),
                                                            *(yt + 1))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = (unsigned char) (tb - (bsign * std::max(*(xt++),
                                                            *(yt + 2))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        } else {
          channel = (unsigned char) (tr - (rsign * std::max(*(xt++), *(yt))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = (unsigned char) (tg - (gsign * std::max(*(xt++),
                                                            *(yt + 1))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = (unsigned char) (tb - (bsign * std::max(*(xt++),
                                                            *(yt + 2))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
}


void bt::Image::egradient(void) {
  // elliptic gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  float drx, dgx, dbx, dry, dgy, dby, yr, yg, yb, xr, xg, xb;
  int rsign, gsign, bsign;
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int *xt = xtable, *yt = ytable,
    tr = (unsigned long) to.red(),
    tg = (unsigned long) to.green(),
    tb = (unsigned long) to.blue();

  register unsigned int x, y;

  dry = drx = (float) (to.red() - from.red());
  dgy = dgx = (float) (to.green() - from.green());
  dby = dbx = (float) (to.blue() - from.blue());

  rsign = (drx < 0) ? -1 : 1;
  gsign = (dgx < 0) ? -1 : 1;
  bsign = (dbx < 0) ? -1 : 1;

  xr = yr = (drx / 2);
  xg = yg = (dgx / 2);
  xb = yb = (dbx / 2);

  // Create X table
  drx /= width;
  dgx /= width;
  dbx /= width;

  for (x = 0; x < width; x++) {
    *(xt++) = (unsigned long) (xr * xr);
    *(xt++) = (unsigned long) (xg * xg);
    *(xt++) = (unsigned long) (xb * xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; y++) {
    *(yt++) = (unsigned long) (yr * yr);
    *(yt++) = (unsigned long) (yg * yg);
    *(yt++) = (unsigned long) (yb * yb);

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal egradient
    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        *(pr++) = (unsigned char)
          (tr - (rsign * control->getSqrt(*(xt++) + *(yt))));
        *(pg++) = (unsigned char)
          (tg - (gsign * control->getSqrt(*(xt++) + *(yt + 1))));
        *(pb++) = (unsigned char)
          (tb - (bsign * control->getSqrt(*(xt++) + *(yt + 2))));
      }
    }
  } else {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        if (y & 1) {
          channel = (unsigned char)
            (tr - (rsign * control->getSqrt(*(xt++) + *(yt))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = (unsigned char)
            (tg - (gsign * control->getSqrt(*(xt++) + *(yt + 1))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = (unsigned char)
            (tb - (bsign * control->getSqrt(*(xt++) + *(yt + 2))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        } else {
          channel = (unsigned char)
            (tr - (rsign * control->getSqrt(*(xt++) + *(yt))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = (unsigned char)
          (tg - (gsign * control->getSqrt(*(xt++) + *(yt + 1))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = (unsigned char)
            (tb - (bsign * control->getSqrt(*(xt++) + *(yt + 2))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
}


void bt::Image::pcgradient(void) {
  // pipe cross gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  float drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
  int rsign, gsign, bsign;
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int *xt = xtable, *yt = ytable,
    tr = to.red(),
    tg = to.green(),
    tb = to.blue();

  register unsigned int x, y;

  dry = drx = (float) (to.red() - from.red());
  dgy = dgx = (float) (to.green() - from.green());
  dby = dbx = (float) (to.blue() - from.blue());

  rsign = (drx < 0) ? -2 : 2;
  gsign = (dgx < 0) ? -2 : 2;
  bsign = (dbx < 0) ? -2 : 2;

  xr = yr = (drx / 2);
  xg = yg = (dgx / 2);
  xb = yb = (dbx / 2);

  // Create X table
  drx /= width;
  dgx /= width;
  dbx /= width;

  for (x = 0; x < width; x++) {
    *(xt++) = (unsigned char) ((xr < 0) ? -xr : xr);
    *(xt++) = (unsigned char) ((xg < 0) ? -xg : xg);
    *(xt++) = (unsigned char) ((xb < 0) ? -xb : xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; y++) {
    *(yt++) = ((unsigned char) ((yr < 0) ? -yr : yr));
    *(yt++) = ((unsigned char) ((yg < 0) ? -yg : yg));
    *(yt++) = ((unsigned char) ((yb < 0) ? -yb : yb));

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal pcgradient
    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        *(pr++) = (unsigned char) (tr - (rsign * std::min(*(xt++), *(yt))));
        *(pg++) = (unsigned char) (tg - (gsign * std::min(*(xt++),
                                                          *(yt + 1))));
        *(pb++) = (unsigned char) (tb - (bsign * std::min(*(xt++),
                                                          *(yt + 2))));
      }
    }
  } else {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        if (y & 1) {
          channel = (unsigned char) (tr - (rsign * std::min(*(xt++), *(yt))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = (unsigned char) (tg - (bsign * std::min(*(xt++),
                                                            *(yt + 1))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = (unsigned char) (tb - (gsign * std::min(*(xt++),
                                                            *(yt + 2))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        } else {
          channel = (unsigned char) (tr - (rsign * std::min(*(xt++), *(yt))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = (unsigned char) (tg - (gsign * std::min(*(xt++),
                                                            *(yt + 1))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = (unsigned char) (tb - (bsign * std::min(*(xt++),
                                                            *(yt + 2))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
}


void bt::Image::cdgradient(void) {
  // cross diagonal gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  float drx, dgx, dbx, dry, dgy, dby, yr = 0.0, yg = 0.0, yb = 0.0,
    xr = (float) from.red(),
    xg = (float) from.green(),
    xb = (float) from.blue();
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int w = width * 2, h = height * 2, *xt, *yt;

  register unsigned int x, y;

  dry = drx = (float) (to.red() - from.red());
  dgy = dgx = (float) (to.green() - from.green());
  dby = dbx = (float) (to.blue() - from.blue());

  // Create X table
  drx /= w;
  dgx /= w;
  dbx /= w;

  for (xt = (xtable + (width * 3) - 1), x = 0; x < width; x++) {
    *(xt--) = (unsigned char) xb;
    *(xt--) = (unsigned char) xg;
    *(xt--) = (unsigned char) xr;

    xr += drx;
    xg += dgx;
    xb += dbx;
  }

  // Create Y table
  dry /= h;
  dgy /= h;
  dby /= h;

  for (yt = ytable, y = 0; y < height; y++) {
    *(yt++) = (unsigned char) yr;
    *(yt++) = (unsigned char) yg;
    *(yt++) = (unsigned char) yb;

    yr += dry;
    yg += dgy;
    yb += dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal cdgradient
    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        *(pr++) = *(xt++) + *(yt);
        *(pg++) = *(xt++) + *(yt + 1);
        *(pb++) = *(xt++) + *(yt + 2);
      }
    }
  } else {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (yt = ytable, y = 0; y < height; y++, yt += 3) {
      for (xt = xtable, x = 0; x < width; x++) {
        if (y & 1) {
          channel = *(xt++) + *(yt);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = *(xt++) + *(yt + 1);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = *(xt++) + *(yt + 2);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        } else {
          channel = *(xt++) + *(yt);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = *(xt++) + *(yt + 1);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = *(xt++) + *(yt + 2);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
}
