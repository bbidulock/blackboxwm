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
#include <assert.h>
#include <math.h>
}

#include <algorithm>

#include "BaseDisplay.hh"
#include "GCCache.hh"
#include "Image.hh"
#include "Texture.hh"


static const unsigned int maximumWidth  = 4000;
static const unsigned int maximumHeight = 3000;


bt::Image::Buffer bt::Image::buffer;
unsigned int bt::Image::global_colorsPerChannel = 4;
bool bt::Image::global_ditherEnabled = true;
bt::Image::XColorTableList bt::Image::colorTableList;


namespace bt {
  class XColorTable {
  public:
    XColorTable(Display &dpy, unsigned int screen,
                unsigned int colors_per_channel);
    ~XColorTable(void);

    void mapDither(unsigned int &red,
                   unsigned int &green,
                   unsigned int &blue,
                   unsigned int &red_error,
                   unsigned int &green_error,
                   unsigned int &blue_error);
    void map(unsigned int &red,
             unsigned int &green,
             unsigned int &blue);
    unsigned long pixel(unsigned int red,
                        unsigned int green,
                        unsigned int blue);

  private:
    Display &_dpy;
    unsigned int _screen;
    int _vclass;
    unsigned int _cpc, _cpcsq;
    int red_offset, green_offset, blue_offset;
    unsigned int red_bits, green_bits, blue_bits;

    unsigned char _red[256];
    unsigned char _green[256];
    unsigned char _blue[256];
    std::vector<XColor> colors;
  };
} // namespace bt


bt::XColorTable::XColorTable(Display &dpy, unsigned int screen,
                             unsigned int colors_per_channel)
  : _dpy(dpy), _screen(screen),
    _cpc(colors_per_channel), _cpcsq(_cpc * _cpc) {
  const ScreenInfo * const screeninfo = _dpy.screenNumber(_screen);
  _vclass = screeninfo->getVisual()->c_class;
  unsigned int depth = screeninfo->getDepth();

  red_offset = green_offset = blue_offset = 0;
  red_bits = green_bits = blue_bits = 0;

  switch (_vclass) {
  case StaticGray:
  case GrayScale: {
    unsigned int ncolors;
    if (_vclass == StaticGray)
      ncolors = 1u << depth;
    else
      ncolors = _cpc * _cpc * _cpc;

    if (ncolors > (1u << depth)) {
      _cpc = (1u << depth) / 3u;
      ncolors = _cpc * _cpc * _cpc;
    }

    if (_cpc < 2u || ncolors > (1u << depth)) {
      // invalid colormap size, reducing
      _cpc = (1u << depth) / 3u;
    }

    colors.resize(ncolors);
    red_bits = green_bits = blue_bits = 255u / (_cpc - 1);
    break;
  }

  case StaticColor:
  case PseudoColor: {
    unsigned int ncolors = _cpc * _cpc * _cpc;

    if (ncolors > (1u << depth)) {
      _cpc = (1u << depth) / 3u;
      ncolors = _cpc * _cpc * _cpc;
    }

    if (_cpc < 2u || ncolors > (1u << depth)) {
      // invalid colormap size, reducing
      _cpc = (1u << depth) / 3u;
    }

    colors.resize(ncolors);
#ifdef ORDEREDPSEUDO
    red_bits = green_bits = blue_bits = 256u / _cpc;
#else // !ORDEREDPSEUDO
    red_bits = green_bits = blue_bits = 255u / (_cpc - 1);
#endif // ORDEREDPSEUDO
    break;
  }

  case TrueColor:
  case DirectColor: {
    // compute color tables
    unsigned long red_mask = screeninfo->getVisual()->red_mask,
                green_mask = screeninfo->getVisual()->green_mask,
                 blue_mask = screeninfo->getVisual()->blue_mask;

    while (! (red_mask & 1)) { red_offset++; red_mask >>= 1; }
    while (! (green_mask & 1)) { green_offset++; green_mask >>= 1; }
    while (! (blue_mask & 1)) { blue_offset++; blue_mask >>= 1; }

    red_bits = 255u / red_mask;
    green_bits = 255u / green_mask;
    blue_bits = 255u / blue_mask;

    break;
  }
  } // switch

  // initialize color tables
  for (unsigned int i = 0; i < 256u; i++) {
    _red[i] = i / red_bits;
    _green[i] = i / green_bits;
    _blue[i] = i / blue_bits;
  }

  if (! colors.empty()) {
    // allocate color cube
    unsigned int i = 0, ii, p, r, g, b;
    for (r = 0, i = 0; r < _cpc; r++)
      for (g = 0; g < _cpc; g++)
	for (b = 0; b < _cpc; b++, i++) {
	  colors[i].red = (r * 0xffff) / (_cpc - 1);
	  colors[i].green = (g * 0xffff) / (_cpc - 1);
	  colors[i].blue = (b * 0xffff) / (_cpc - 1);;
	  colors[i].flags = DoRed|DoGreen|DoBlue;
	}

    const Colormap colormap = screeninfo->getColormap();
    for (i = 0; i < colors.size(); i++) {
      if (! XAllocColor(_dpy.XDisplay(), colormap, &colors[i]))
	colors[i].flags = 0;
      else
	colors[i].flags = DoRed|DoGreen|DoBlue;
    }

    XColor icolors[256];
    unsigned int incolors = (((1u << depth) > 256u) ? 256u : (1u << depth));

    for (i = 0; i < incolors; i++)
      icolors[i].pixel = i;

    XQueryColors(_dpy.XDisplay(), colormap, icolors, incolors);
    for (i = 0; i < colors.size(); i++) {
      if (! colors[i].flags) {
	unsigned long chk = 0xffffffff, pix, close = 0;

	p = 2;
	while (p--) {
	  for (ii = 0; ii < incolors; ii++) {
	    r = (colors[i].red - icolors[i].red) >> 8;
	    g = (colors[i].green - icolors[i].green) >> 8;
	    b = (colors[i].blue - icolors[i].blue) >> 8;
	    pix = (r * r) + (g * g) + (b * b);

	    if (pix < chk) {
	      chk = pix;
	      close = ii;
	    }

	    colors[i].red = icolors[close].red;
	    colors[i].green = icolors[close].green;
	    colors[i].blue = icolors[close].blue;

	    if (XAllocColor(_dpy.XDisplay(), colormap,
			    &colors[i])) {
	      colors[i].flags = DoRed|DoGreen|DoBlue;
	      break;
	    }
	  }
	}
      }
    }
  }
}


bt::XColorTable::~XColorTable(void) {

}


void bt::XColorTable::mapDither(unsigned int &red,
                                unsigned int &green,
                                unsigned int &blue,
                                unsigned int &red_error,
                                unsigned int &green_error,
                                unsigned int &blue_error) {
  red_error =   red   & (red_bits - 1);
  green_error = green & (green_bits - 1);
  blue_error =  blue  & (blue_bits - 1);

  map(red, green, blue);
}


void bt::XColorTable::map(unsigned int &red,
                          unsigned int &green,
                          unsigned int &blue) {
  red = _red[red];
  green = _green[green];
  blue = _blue[blue];
}


unsigned long bt::XColorTable::pixel(unsigned int red,
                                     unsigned int green,
                                     unsigned int blue) {
  switch (_vclass) {
  case StaticGray:
  case GrayScale:
    return ((red * 30) + (green * 59) + (blue * 11)) / 100;

  case StaticColor:
  case PseudoColor:
    return (red * _cpcsq) + (green * _cpc) + blue;

  case TrueColor:
  case DirectColor:
    return ((red << red_offset) |
            (green << green_offset) |
            (blue << blue_offset));
  }

  // not reached
  return 0; // shut up compiler warning
}


bt::Image::Image(unsigned int w, unsigned int h)
  :  width(w), height(h), _dpy(0), _screen(~0u) {
  assert(width > 0  && width  < maximumWidth);
  assert(height > 0 && height < maximumHeight);

  red = new unsigned char[width * height];
  green = new unsigned char[width * height];
  blue = new unsigned char[width * height];
}


bt::Image::~Image(void) {
  delete [] red;
  delete [] green;
  delete [] blue;
}


Pixmap bt::Image::render(const bt::Texture &texture) {
  _dpy = const_cast<Display*>(texture.display());
  _screen = texture.screen();

  if (texture.texture() & bt::Texture::Parent_Relative)
    return ParentRelative;
  else if (texture.texture() & bt::Texture::Solid)
    return render_solid(texture);
  else if (texture.texture() & bt::Texture::Gradient)
    return render_gradient(texture);
  return None;
}


Pixmap bt::Image::render_solid(const bt::Texture &texture) {
  const ScreenInfo * const screeninfo = _dpy->screenNumber(_screen);
  Pixmap pixmap = XCreatePixmap(_dpy->XDisplay(), screeninfo->getRootWindow(),
                                width, height, screeninfo->getDepth());
  if (pixmap == None)
    return None;

  ::Display *display = _dpy->XDisplay();

  bt::Pen pen(texture.color());
  bt::Pen penlight(texture.lightColor());
  bt::Pen penshadow(texture.shadowColor());

  XFillRectangle(display, pixmap, pen.gc(), 0, 0, width, height);

  unsigned int bw = 0;
  if (texture.texture() & bt::Texture::Border) {
    bt::Pen penborder(texture.borderColor());
    bw = texture.borderWidth();

    for (unsigned int i = 0; i < bw; ++i)
      XDrawRectangle(display, pixmap, penborder.gc(),
                     i, i, width - (i * 2) - 1, height - (i * 2) - 1);
  }

  if (texture.texture() & bt::Texture::Interlaced) {
    bt::Pen peninterlace(texture.colorTo());
    for (unsigned int i = bw; i < height - (bw * 2); i += 2)
      XDrawLine(display, pixmap, peninterlace.gc(),
                bw, i, width - (bw * 2), i);
  }

  if (texture.texture() & bt::Texture::Raised) {
    XDrawLine(display, pixmap, penshadow.gc(),
              bw, height - bw - 1, width - bw - 1, height - bw - 1);
    XDrawLine(display, pixmap, penshadow.gc(),
              width - bw - 1, height - bw - 1, width - bw - 1, bw);

    XDrawLine(display, pixmap, penlight.gc(),
              bw, bw, width - bw - 1, bw);
    XDrawLine(display, pixmap, penlight.gc(),
              bw, height - bw - 1, bw, bw);
  } else if (texture.texture() & bt::Texture::Sunken) {
    XDrawLine(display, pixmap, penlight.gc(),
              bw, height - bw - 1, width - bw - 1, height - bw - 1);
    XDrawLine(display, pixmap, penlight.gc(),
              width - bw - 1, height - bw - 1, width - bw - 1, bw);

    XDrawLine(display, pixmap, penshadow.gc(),
              bw, bw, width - bw - 1, bw);
    XDrawLine(display, pixmap, penshadow.gc(),
              bw, height - bw - 1, bw, bw);
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

  if (texture.texture() & bt::Texture::Diagonal) dgradient();
  else if (texture.texture() & bt::Texture::Elliptic) egradient();
  else if (texture.texture() & bt::Texture::Horizontal) hgradient();
  else if (texture.texture() & bt::Texture::Pyramid) pgradient();
  else if (texture.texture() & bt::Texture::Rectangle) rgradient();
  else if (texture.texture() & bt::Texture::Vertical) vgradient();
  else if (texture.texture() & bt::Texture::CrossDiagonal) cdgradient();
  else if (texture.texture() & bt::Texture::PipeCross) pcgradient();

  if (texture.texture() & (bt::Texture::Sunken | bt::Texture::Raised))
    bevel(texture.borderWidth());

  if (inverted) invert();

  Pixmap pixmap = renderPixmap();

  unsigned int bw = 0;
  if (texture.texture() & bt::Texture::Border) {
    bt::Pen penborder(texture.borderColor());
    bw = texture.borderWidth();

    for (unsigned int i = 0; i < bw; ++i)
      XDrawRectangle(_dpy->XDisplay(), pixmap, penborder.gc(),
                     i, i, width - (i * 2) - 1, height - (i * 2) - 1);
  }

  return pixmap;
}


/*
 * Ordered dither tables
 */
static const unsigned char dither4[4][4] = {
  { 0, 4, 1, 5 },
  { 6, 2, 7, 3 },
  { 1, 5, 0, 4 },
  { 7, 3, 6, 2 }
};
static const unsigned char dither8[8][8] = {
  { 0,  32, 8,  40, 2,  34, 10, 42 },
  { 48, 16, 56, 24, 50, 18, 58, 26 },
  { 12, 44, 4,  36, 14, 46, 6,  38 },
  { 60, 28, 52, 20, 62, 30, 54, 22 },
  { 3,  35, 11, 43, 1,  33, 9,  41 },
  { 51, 19, 59, 27, 49, 17, 57, 25 },
  { 15, 47, 7,  39, 13, 45, 5,  37 },
  { 63, 31, 55, 23, 61, 29, 53, 21 }
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
    pixel_data[0] = pixel;
    ++pixel_data;
    break;

  case 16: // 16bpp LSB
    pixel_data[0] = pixel;
    pixel_data[1] = pixel >> 8;
    pixel_data += 2;
    break;

  case 17: // 16bpp MSB
    pixel_data[0] = pixel >> 8;
    pixel_data[1] = pixel;
    pixel_data += 2;
    break;

  case 24: // 24bpp LSB
    pixel_data[0] = pixel;
    pixel_data[1] = pixel >> 8;
    pixel_data[2] = pixel >> 16;
    pixel_data += 3;
    break;

  case 25: // 24bpp MSB
    pixel_data[0] = pixel >> 16;
    pixel_data[1] = pixel >> 8;
    pixel_data[2] = pixel;
    pixel_data += 3;
    break;

  case 32: // 32bpp LSB
    pixel_data[0] = pixel;
    pixel_data[1] = pixel >> 8;
    pixel_data[2] = pixel >> 16;
    pixel_data[3] = pixel >> 24;
    pixel_data += 4;
    break;

  case 33: // 32bpp MSB
    pixel_data[0] = pixel >> 24;
    pixel_data[1] = pixel >> 16;
    pixel_data[2] = pixel >> 8;
    pixel_data[3] = pixel;
    pixel_data += 4;
    break;
  }
  *data = pixel_data; // assign back so we don't lose our place
}


// algorithm: ordered dithering... many many thanks to rasterman
// (raster@rasterman.com) for telling me about this... portions of this
// code is based off of his code in Imlib
void bt::Image::TrueColorDither(XColorTable *colortable,
                                unsigned int bit_depth,
                                int bytes_per_line,
                                unsigned char *pixel_data) {
  unsigned int x, y, dithx, dithy, r, g, b, er, eg, eb, offset;
  unsigned char *ppixel_data = pixel_data;
  unsigned int maxr = 255, maxg = 255, maxb = 255;

  colortable->map(maxr, maxg, maxb);

  for (y = 0, offset = 0; y < height; ++y) {
    dithy = y & 0x3;

    for (x = 0; x < width; ++x, ++offset) {
      dithx = x & 0x3;

      r = red[offset];
      g = green[offset];
      b = blue[offset];

      colortable->mapDither(r, g, b, er, eg, eb);

      if ((dither4[dithy][dithx] < er) && (r < maxr)) r++;
      if ((dither4[dithy][dithx] < eg) && (g < maxg)) g++;
      if ((dither4[dithy][dithx] < eb) && (b < maxb)) b++;

      assignPixelData(bit_depth, &pixel_data, colortable->pixel(r, g, b));
    }

    pixel_data = (ppixel_data += bytes_per_line);
  }
}


void bt::Image::OrderedPseudoColorDither(XColorTable *colortable,
                                         int bytes_per_line,
                                         unsigned char *pixel_data) {
  unsigned int x, y, dithx, dithy, r, g, b, er, eg, eb, offset;
  unsigned char *ppixel_data = pixel_data;
  unsigned int maxr = 255, maxg = 255, maxb = 255;

  colortable->map(maxr, maxg, maxb);

  for (y = 0, offset = 0; y < height; y++) {
    dithy = y & 7;

    for (x = 0; x < width; x++, offset++) {
      dithx = x & 7;

      r = red[offset];
      g = green[offset];
      b = blue[offset];

      colortable->mapDither(r, g, b, er, eg, eb);

      if ((dither8[dithy][dithx] < er) && (r < maxr)) r++;
      if ((dither8[dithy][dithx] < eg) && (g < maxg)) g++;
      if ((dither8[dithy][dithx] < eb) && (b < maxb)) b++;

      *(pixel_data++) = colortable->pixel(r, g, b);
    }

    pixel_data = (ppixel_data += bytes_per_line);
  }
}


void bt::Image::PseudoColorDither(XColorTable *colortable,
                                  int bytes_per_line,
                                  unsigned char *pixel_data) {
  short *terr,
    *rerr = new short[width + 2],
    *gerr = new short[width + 2],
    *berr = new short[width + 2],
    *nrerr = new short[width + 2],
    *ngerr = new short[width + 2],
    *nberr = new short[width + 2];

  int rr, gg, bb, rer, ger, ber;
  int dd = 255 / (colorsPerChannel() - 1);
  unsigned int x, y, r, g, b, offset;
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

      colortable->map(r, g, b);

      rer = rerr[x] - r*dd;
      ger = gerr[x] - g*dd;
      ber = berr[x] - b*dd;

      *pixel_data++ = colortable->pixel(r, g, b);

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
  // get the colortable for the screen. if necessary, we will create one.
  if (colorTableList.empty())
    colorTableList.resize(_dpy->screenCount(), 0);

  if (! colorTableList[_screen])
    colorTableList[_screen] =
      new XColorTable(*_dpy, _screen, colorsPerChannel());

  XColorTable *colortable = colorTableList[_screen];

  // create XImage
  const ScreenInfo * const screeninfo = _dpy->screenNumber(_screen);
  XImage *image =
    XCreateImage(_dpy->XDisplay(), screeninfo->getVisual(),
                 screeninfo->getDepth(), ZPixmap,
                 0, 0, width, height, 32, 0);

  if (! image)
    return (XImage *) 0;

  // insurance policy
  image->data = (char *) 0;

  buffer.reserve(image->bytes_per_line * (height + 1));
  unsigned char *d = &buffer[0];

  unsigned int o = image->bits_per_pixel +
                   ((image->byte_order == MSBFirst) ? 1 : 0);


  if ( isDitherEnabled() && width > 1 && height > 1) {
    switch (screeninfo->getVisual()->c_class) {
    case TrueColor:
    case DirectColor:
      TrueColorDither(colortable, o, image->bytes_per_line, d);
      break;

    case StaticColor:
    case PseudoColor: {
#ifdef ORDEREDPSEUDO
      OrderedPseudoColorDither(colortable, image->bytes_per_line, d);
#else
      PseudoColorDither(colortable, image->bytes_per_line, d);
#endif
      break;
    }

    default: {
      // unsupported visual...
      image->data = NULL;
      XDestroyImage(image);
      return (XImage *) 0;
    }
    } // switch
  } else {
    unsigned int x, y, offset, r, g, b;
    unsigned char *pixel_data = d, *ppixel_data = d;

    for (y = 0, offset = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++offset) {
        r = red[offset];
        g = green[offset];
        b = blue[offset];

        colortable->map(r, g, b);
        assignPixelData(o, &pixel_data, colortable->pixel(r, g, b));
      }

      pixel_data = (ppixel_data += image->bytes_per_line);
    }
  }

  image->data = (char *) d;
  return image;
}


Pixmap bt::Image::renderPixmap(void) {
  const ScreenInfo * const screeninfo = _dpy->screenNumber(_screen);
  Pixmap pixmap =
    XCreatePixmap(_dpy->XDisplay(), screeninfo->getRootWindow(),
                  width, height, screeninfo->getDepth());

  if (pixmap == None)
    return None;

  XImage *image = renderXImage();

  if (! image) {
    XFreePixmap(_dpy->XDisplay(), pixmap);
    return None;
  }

  if (! image->data) {
    XDestroyImage(image);
    XFreePixmap(_dpy->XDisplay(), pixmap);
    return None;
  }

  Pen pen(Color(0, 0, 0, _dpy, _screen));
  XPutImage(_dpy->XDisplay(), pixmap, pen.gc(), image,
            0, 0, 0, 0, width, height);

  image->data = NULL;
  XDestroyImage(image);

  return pixmap;
}


void bt::Image::bevel(unsigned int border_width) {
  if (width < 2 || height < 2 ||
      width  < (border_width * 4) || height < (border_width * 4))
    return;

  unsigned char *pr = red   + (border_width * width) + border_width;
  unsigned char *pg = green + (border_width * width) + border_width;
  unsigned char *pb = blue  + (border_width * width) + border_width;
  unsigned int w = width - (border_width * 2);
  unsigned int h = height - (border_width * 2) - 2;
  unsigned char rr, gg, bb;

  // top of the bevel
  do {
    rr = *pr + (*pr >> 1);
    gg = *pg + (*pg >> 1);
    bb = *pb + (*pb >> 1);

    if (rr < *pr) rr = ~0;
    if (gg < *pg) gg = ~0;
    if (bb < *pb) bb = ~0;

    *pr = rr;
    *pg = gg;
    *pb = bb;

    ++pr;
    ++pg;
    ++pb;
  } while (--w);

  pr += border_width + border_width;
  pg += border_width + border_width;
  pb += border_width + border_width;

  w = width - (border_width * 2);

  // left and right of the bevel
  do {
    rr = *pr + (*pr >> 1);
    gg = *pg + (*pg >> 1);
    bb = *pb + (*pb >> 1);

    if (rr < *pr) rr = ~0;
    if (gg < *pg) gg = ~0;
    if (bb < *pb) bb = ~0;

    *pr = rr;
    *pg = gg;
    *pb = bb;

    pr += w - 1;
    pg += w - 1;
    pb += w - 1;

    rr = (*pr >> 2) + (*pr >> 1);
    gg = (*pg >> 2) + (*pg >> 1);
    bb = (*pb >> 2) + (*pb >> 1);

    if (rr > *pr) rr = 0;
    if (gg > *pg) gg = 0;
    if (bb > *pb) bb = 0;

    *pr++ = rr;
    *pg++ = gg;
    *pb++ = bb;

    pr += border_width + border_width;
    pg += border_width + border_width;
    pb += border_width + border_width;
  } while (--h);

  w = width - (border_width * 2);

  // bottom of the bevel
  do {
    rr = (*pr >> 2) + (*pr >> 1);
    gg = (*pg >> 2) + (*pg >> 1);
    bb = (*pb >> 2) + (*pb >> 1);

    if (rr > *pr) rr = 0;
    if (gg > *pg) gg = 0;
    if (bb > *pb) bb = 0;

    *pr = rr;
    *pg = gg;
    *pb = bb;

    ++pr;
    ++pg;
    ++pb;
  } while (--w);
}


void bt::Image::invert(void) {
  unsigned int i, j, wh = (width * height) - 1;
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

  double drx, dgx, dbx, dry, dgy, dby;
  double yr = 0.0, yg = 0.0, yb = 0.0,
         xr = static_cast<double>(from.red()),
         xg = static_cast<double>(from.green()),
         xb = static_cast<double>(from.blue());

  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int w = width * 2, h = height * 2;
  unsigned int xt[maximumWidth][3], yt[maximumHeight][3];

  unsigned int x, y;

  dry = drx = static_cast<double>(to.red()   - from.red());
  dgy = dgx = static_cast<double>(to.green() - from.green());
  dby = dbx = static_cast<double>(to.blue()  - from.blue());

  // Create X table
  drx /= w;
  dgx /= w;
  dbx /= w;

  for (x = 0; x < width; ++x) {
    xt[x][0] = static_cast<unsigned char>(xr);
    xt[x][1] = static_cast<unsigned char>(xg);
    xt[x][2] = static_cast<unsigned char>(xb);

    xr += drx;
    xg += dgx;
    xb += dbx;
  }

  // Create Y table
  dry /= h;
  dgy /= h;
  dby /= h;

  for (y = 0; y < height; ++y) {
    yt[y][0] = static_cast<unsigned char>(yr);
    yt[y][1] = static_cast<unsigned char>(yg);
    yt[y][2] = static_cast<unsigned char>(yb);

    yr += dry;
    yg += dgy;
    yb += dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal dgradient
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
        *pr = xt[x][0] + yt[y][0];
        *pg = xt[x][1] + yt[y][1];
        *pb = xt[x][2] + yt[y][2];
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
      *pr = xt[x][0] + yt[y][0];
      *pg = xt[x][1] + yt[y][1];
      *pb = xt[x][2] + yt[y][2];

      if (y & 1) {
        *pr = (*pr >> 1) + (*pr >> 2);
        *pg = (*pg >> 1) + (*pg >> 2);
        *pb = (*pb >> 1) + (*pb >> 2);
      }
    }
  }
}


void bt::Image::hgradient(void) {
  double drx, dgx, dbx,
    xr = static_cast<double>(from.red()),
    xg = static_cast<double>(from.green()),
    xb = static_cast<double>(from.blue());
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int total = width * (height - 2);
  unsigned int x;

  drx = static_cast<double>(to.red()   - from.red());
  dgx = static_cast<double>(to.green() - from.green());
  dbx = static_cast<double>(to.blue()  - from.blue());

  drx /= width;
  dgx /= width;
  dbx /= width;

  // second line
  if (interlaced && height > 1) {
    // interlacing effect

    // first line
    for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
      *pr = static_cast<unsigned char>(xr);
      *pg = static_cast<unsigned char>(xg);
      *pb = static_cast<unsigned char>(xb);

      xr += drx;
      xg += dgx;
      xb += dbx;
    }

    // second line
    xr = static_cast<double>(from.red()),
    xg = static_cast<double>(from.green()),
    xb = static_cast<double>(from.blue());

    for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
      *pr = static_cast<unsigned char>(xr);
      *pg = static_cast<unsigned char>(xg);
      *pb = static_cast<unsigned char>(xb);

      *pr = (*pr >> 1) + (*pr >> 2);
      *pg = (*pg >> 1) + (*pg >> 2);
      *pb = (*pb >> 1) + (*pb >> 2);

      xr += drx;
      xg += dgx;
      xb += dbx;
    }
  } else {
    // first line
    for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
      *pr = static_cast<unsigned char>(xr);
      *pg = static_cast<unsigned char>(xg);
      *pb = static_cast<unsigned char>(xb);

      xr += drx;
      xg += dgx;
      xb += dbx;
    }

    if (height > 1) {
      // second line
      memcpy(pr, red, width);
      memcpy(pg, green, width);
      memcpy(pb, blue, width);

      pr += width;
      pg += width;
      pb += width;
    }
  }

  // rest of the gradient
  for (x = 0; x < total; ++x) {
    pr[x] = red[x];
    pg[x] = green[x];
    pb[x] = blue[x];
  }
}


void bt::Image::vgradient(void) {
  double dry, dgy, dby,
    yr = static_cast<double>(from.red()),
    yg = static_cast<double>(from.green()),
    yb = static_cast<double>(from.blue());
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int y;

  dry = (double) (to.red() - from.red());
  dgy = (double) (to.green() - from.green());
  dby = (double) (to.blue() - from.blue());

  dry /= height;
  dgy /= height;
  dby /= height;

  if (interlaced) {
    // faked interlacing effect
    for (y = 0; y < height; y++, pr += width, pg += width, pb += width) {
      *pr = static_cast<unsigned char>(yr);
      *pg = static_cast<unsigned char>(yg);
      *pb = static_cast<unsigned char>(yb);

      if (y & 1) {
        *pr = (*pr >> 1) + (*pr >> 2);
        *pg = (*pg >> 1) + (*pg >> 2);
        *pb = (*pb >> 1) + (*pb >> 2);
      }

      memset(pr, *pr, width);
      memset(pg, *pg, width);
      memset(pb, *pb, width);

      yr += dry;
      yg += dgy;
      yb += dby;
    }
  } else {
    // normal vgradient
    for (y = 0; y < height; y++, pr += width, pg += width, pb += width) {
      *pr = static_cast<unsigned char>(yr);
      *pg = static_cast<unsigned char>(yg);
      *pb = static_cast<unsigned char>(yb);

      memset(pr, *pr, width);
      memset(pg, *pg, width);
      memset(pb, *pb, width);

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

  double yr, yg, yb, drx, dgx, dbx, dry, dgy, dby, xr, xg, xb;
  int rsign, gsign, bsign;
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int tr = to.red(), tg = to.green(), tb = to.blue();
  unsigned int xt[maximumWidth][3], yt[maximumHeight][3];
  unsigned int x, y;

  dry = drx = static_cast<double>(to.red()   - from.red());
  dgy = dgx = static_cast<double>(to.green() - from.green());
  dby = dbx = static_cast<double>(to.blue()  - from.blue());

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

  for (x = 0; x < width; ++x) {
    xt[x][0] = static_cast<unsigned char>(fabs(xr));
    xt[x][1] = static_cast<unsigned char>(fabs(xg));
    xt[x][2] = static_cast<unsigned char>(fabs(xb));

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; ++y) {
    yt[y][0] = static_cast<unsigned char>(fabs(yr));
    yt[y][1] = static_cast<unsigned char>(fabs(yg));
    yt[y][2] = static_cast<unsigned char>(fabs(yb));

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal pgradient
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
        *pr = static_cast<unsigned char>(tr - (rsign * (xt[x][0] + yt[y][0])));
        *pg = static_cast<unsigned char>(tg - (gsign * (xt[x][1] + yt[y][1])));
        *pb = static_cast<unsigned char>(tb - (bsign * (xt[x][2] + yt[y][2])));
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
      *pr = static_cast<unsigned char>(tr - (rsign * (xt[x][0] + yt[y][0])));
      *pg = static_cast<unsigned char>(tg - (gsign * (xt[x][1] + yt[y][1])));
      *pb = static_cast<unsigned char>(tb - (bsign * (xt[x][2] + yt[y][2])));

      if (y & 1) {
        *pr = (*pr >> 1) + (*pr >> 2);
        *pg = (*pg >> 1) + (*pg >> 2);
        *pb = (*pb >> 1) + (*pb >> 2);
      }
    }
  }
}


void bt::Image::rgradient(void) {
  // rectangle gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  double drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
  int rsign, gsign, bsign;
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int tr = to.red(), tg = to.green(), tb = to.blue();
  unsigned int xt[maximumWidth][3], yt[maximumHeight][3];
  unsigned int x, y;

  dry = drx = static_cast<double>(to.red()   - from.red());
  dgy = dgx = static_cast<double>(to.green() - from.green());
  dby = dbx = static_cast<double>(to.blue()  - from.blue());

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

  for (x = 0; x < width; ++x) {
    xt[x][0] = static_cast<unsigned char>(fabs(xr));
    xt[x][1] = static_cast<unsigned char>(fabs(xg));
    xt[x][2] = static_cast<unsigned char>(fabs(xb));

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; ++y) {
    yt[y][0] = static_cast<unsigned char>(fabs(yr));
    yt[y][1] = static_cast<unsigned char>(fabs(yg));
    yt[y][2] = static_cast<unsigned char>(fabs(yb));

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal rgradient
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
        *pr = static_cast<unsigned char>(tr - (rsign *
                                               std::max(xt[x][0], yt[y][0])));
        *pg = static_cast<unsigned char>(tg - (gsign *
                                               std::max(xt[x][1], yt[y][1])));
        *pb = static_cast<unsigned char>(tb - (bsign *
                                               std::max(xt[x][2], yt[y][2])));
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
      *pr = static_cast<unsigned char>(tr - (rsign *
                                             std::max(xt[x][0], yt[y][0])));
      *pg = static_cast<unsigned char>(tg - (gsign *
                                             std::max(xt[x][1], yt[y][1])));
      *pb = static_cast<unsigned char>(tb - (bsign *
                                             std::max(xt[x][2], yt[y][2])));

      if (y & 1) {
        *pr = (*pr >> 1) + (*pr >> 2);
        *pg = (*pg >> 1) + (*pg >> 2);
        *pb = (*pb >> 1) + (*pb >> 2);
      }
    }
  }
}


void bt::Image::egradient(void) {
  // elliptic gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  double drx, dgx, dbx, dry, dgy, dby, yr, yg, yb, xr, xg, xb;
  int rsign, gsign, bsign;
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int tr = to.red(), tg = to.green(), tb = to.blue();
  unsigned int xt[maximumWidth][3], yt[maximumHeight][3];
  unsigned int x, y;

  dry = drx = (double) (to.red() - from.red());
  dgy = dgx = (double) (to.green() - from.green());
  dby = dbx = (double) (to.blue() - from.blue());

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
    xt[x][0] = static_cast<unsigned int>(xr * xr);
    xt[x][1] = static_cast<unsigned int>(xg * xg);
    xt[x][2] = static_cast<unsigned int>(xb * xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; y++) {
    yt[y][0] = static_cast<unsigned int>(yr * yr);
    yt[y][1] = static_cast<unsigned int>(yg * yg);
    yt[y][2] = static_cast<unsigned int>(yb * yb);

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal egradient
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
        *pr = static_cast<unsigned char>
              (tr - (rsign * static_cast<int>(sqrt(xt[x][0] + yt[y][0]))));
        *pg = static_cast<unsigned char>
              (tg - (gsign * static_cast<int>(sqrt(xt[x][1] + yt[y][1]))));
        *pb = static_cast<unsigned char>
              (tb - (bsign * static_cast<int>(sqrt(xt[x][2] + yt[y][2]))));
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
      *pr = static_cast<unsigned char>
            (tr - (rsign * static_cast<int>(sqrt(xt[x][0] + yt[y][0]))));
      *pg = static_cast<unsigned char>
            (tg - (gsign * static_cast<int>(sqrt(xt[x][1] + yt[y][1]))));
      *pb = static_cast<unsigned char>
            (tb - (bsign * static_cast<int>(sqrt(xt[x][2] + yt[y][2]))));

      if (y & 1) {
        *pr = (*pr >> 1) + (*pr >> 2);
        *pg = (*pg >> 1) + (*pg >> 2);
        *pb = (*pb >> 1) + (*pb >> 2);
      }
    }
  }
}


void bt::Image::pcgradient(void) {
  // pipe cross gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  double drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
  int rsign, gsign, bsign;
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int tr = to.red(), tg = to.green(), tb = to.blue();
  unsigned int xt[maximumWidth][3], yt[maximumHeight][3];
  unsigned int x, y;

  dry = drx = static_cast<double>(to.red()   - from.red());
  dgy = dgx = static_cast<double>(to.green() - from.green());
  dby = dbx = static_cast<double>(to.blue()  - from.blue());

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

  for (x = 0; x < width; ++x) {
    xt[x][0] = static_cast<unsigned char>(fabs(xr));
    xt[x][1] = static_cast<unsigned char>(fabs(xg));
    xt[x][2] = static_cast<unsigned char>(fabs(xb));

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  for (y = 0; y < height; ++y) {
    yt[y][0] = static_cast<unsigned char>(fabs(yr));
    yt[y][1] = static_cast<unsigned char>(fabs(yg));
    yt[y][2] = static_cast<unsigned char>(fabs(yb));

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal rgradient
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
        *pr = static_cast<unsigned char>(tr - (rsign *
                                               std::min(xt[x][0], yt[y][0])));
        *pg = static_cast<unsigned char>(tg - (gsign *
                                               std::min(xt[x][1], yt[y][1])));
        *pb = static_cast<unsigned char>(tb - (bsign *
                                               std::min(xt[x][2], yt[y][2])));
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
      *pr = static_cast<unsigned char>(tr - (rsign *
                                             std::min(xt[x][0], yt[y][0])));
      *pg = static_cast<unsigned char>(tg - (gsign *
                                             std::min(xt[x][1], yt[y][1])));
      *pb = static_cast<unsigned char>(tb - (bsign *
                                             std::min(xt[x][2], yt[y][2])));

      if (y & 1) {
        *pr = (*pr >> 1) + (*pr >> 2);
        *pg = (*pg >> 1) + (*pg >> 2);
        *pb = (*pb >> 1) + (*pb >> 2);
      }
    }
  }
}


void bt::Image::cdgradient(void) {
  // cross diagonal gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  double drx, dgx, dbx, dry, dgy, dby;
  double yr = 0.0, yg = 0.0, yb = 0.0,
         xr = (double) from.red(),
         xg = (double) from.green(),
         xb = (double) from.blue();
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int w = width * 2, h = height * 2;
  unsigned int xt[maximumWidth][3], yt[maximumHeight][3];
  unsigned int x, y;

  dry = drx = (double) (to.red() - from.red());
  dgy = dgx = (double) (to.green() - from.green());
  dby = dbx = (double) (to.blue() - from.blue());

  // Create X table
  drx /= w;
  dgx /= w;
  dbx /= w;

  for (x = width - 1; x != 0; --x) {
    xt[x][0] = static_cast<unsigned char>(xr);
    xt[x][1] = static_cast<unsigned char>(xg);
    xt[x][2] = static_cast<unsigned char>(xb);

    xr += drx;
    xg += dgx;
    xb += dbx;
  }

  // Create Y table
  dry /= h;
  dgy /= h;
  dby /= h;

  for (y = 0; y < height; ++y) {
    yt[y][0] = static_cast<unsigned char>(yr);
    yt[y][1] = static_cast<unsigned char>(yg);
    yt[y][2] = static_cast<unsigned char>(yb);

    yr += dry;
    yg += dgy;
    yb += dby;
  }

  // Combine tables to create gradient

  if (! interlaced) {
    // normal dgradient
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
        *pr = xt[x][0] + yt[y][0];
        *pg = xt[x][1] + yt[y][1];
        *pb = xt[x][2] + yt[y][2];
      }
    }
    return;
  }

  // interlacing effect
  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x, ++pr, ++pg, ++pb) {
      *pr = xt[x][0] + yt[y][0];
      *pg = xt[x][1] + yt[y][1];
      *pb = xt[x][2] + yt[y][2];

      if (y & 1) {
        *pr = (*pr >> 1) + (*pr >> 2);
        *pg = (*pg >> 1) + (*pg >> 2);
        *pb = (*pb >> 1) + (*pb >> 2);
      }
    }
  }
}
