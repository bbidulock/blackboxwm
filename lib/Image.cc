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
#include <stdio.h>
}

#include <algorithm>

#include "BaseDisplay.hh"
#include "Image.hh"
#include "Pen.hh"
#include "Texture.hh"

// #define COLORTABLE_DEBUG

static const unsigned int maximumWidth  = 8000;
static const unsigned int maximumHeight = 6000;


bt::Image::Buffer bt::Image::buffer;
unsigned int bt::Image::global_colorsPerChannel = 6;
bool bt::Image::global_ditherEnabled = true;
bt::Image::XColorTableList bt::Image::colorTableList;


namespace bt {

  class XColorTable {
  public:
    XColorTable(const Display &dpy, unsigned int screen,
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
    const Display &_dpy;
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


bt::XColorTable::XColorTable(const Display &dpy, unsigned int screen,
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
    red_bits = green_bits = blue_bits = 255u / (_cpc - 1);
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
  unsigned int i;
  for (i = 0; i < 256u; i++) {
    _red[i]   = i / red_bits;
    _green[i] = i / green_bits;
    _blue[i]  = i / blue_bits;
  }

  if (! colors.empty()) {
    // query existing colormap
    const Colormap colormap = screeninfo->getColormap();
    XColor icolors[256];
    unsigned int incolors = (((1u << depth) > 256u) ? 256u : (1u << depth));
    for (i = 0; i < incolors; i++) icolors[i].pixel = i;

    XQueryColors(_dpy.XDisplay(), colormap, icolors, incolors);

#ifdef COLORTABLE_DEBUG
    for (i = 0; i < incolors; ++i) {
      fprintf(stderr, "query %3d: %02x/%02x/%02x %d/%d/%d flags %x\n", i,
              (icolors[i].red   >> 8),
              (icolors[i].green >> 8),
              (icolors[i].blue  >> 8),
              (icolors[i].red   >> 8) / red_bits,
              (icolors[i].green >> 8) / green_bits,
              (icolors[i].blue  >> 8) / blue_bits,
              icolors[i].flags);
    }
#endif // COLORTABLE_DEBUG

    // create color cube
    unsigned int ii, p, r, g, b;
    for (r = 0, i = 0; r < _cpc; r++) {
      for (g = 0; g < _cpc; g++) {
     	for (b = 0; b < _cpc; b++, i++) {
     	  colors[i].red   = (r * 0xffff) / (_cpc - 1);
     	  colors[i].green = (g * 0xffff) / (_cpc - 1);
     	  colors[i].blue  = (b * 0xffff) / (_cpc - 1);
     	  colors[i].flags = DoRed|DoGreen|DoBlue;

#ifdef COLORTABLE_DEBUG
          fprintf(stderr, "req   %3d: %02x/%02x/%02x %1d/%1d/%1d\n", i,
                  (colors[i].red   >> 8),
                  (colors[i].green >> 8),
                  (colors[i].blue  >> 8),
                  (colors[i].red   >> 8) / red_bits,
                  (colors[i].green >> 8) / green_bits,
                  (colors[i].blue  >> 8) / blue_bits);
#endif // COLORTABLE_DEBUG
     	}
      }
    }

    // for missing colors, find the closest color in the existing colormap
    for (i = 0; i < colors.size(); i++) {
      unsigned long chk = 0xffffffff, pix, close = 0;

      p = 2;
      while (p--) {
        for (ii = 0; ii < incolors; ii++) {
          r = (colors[i].red   - icolors[ii].red)   >> 8;
          g = (colors[i].green - icolors[ii].green) >> 8;
          b = (colors[i].blue  - icolors[ii].blue)  >> 8;
          pix = (r * r) + (g * g) + (b * b);

          if (pix < chk) {
            chk = pix;
            close = ii;
          }
        }

        colors[i].red = icolors[close].red;
        colors[i].green = icolors[close].green;
        colors[i].blue = icolors[close].blue;

        if (XAllocColor(_dpy.XDisplay(), colormap,
                        &colors[i])) {

#ifdef COLORTABLE_DEBUG
          fprintf(stderr, "close %3d: %02x/%02x/%02x %1d/%1d/%1d (%d)\n", i,
                  (colors[i].red   >> 8),
                  (colors[i].green >> 8),
                  (colors[i].blue  >> 8),
                  (colors[i].red   >> 8) / red_bits,
                  (colors[i].green >> 8) / green_bits,
                  (colors[i].blue  >> 8) / blue_bits,
                  colors[i].pixel);
#endif // COLORTABLE_DEBUG

          colors[i].flags = DoRed|DoGreen|DoBlue;
          break;
        }
      }
    }
  }
}


bt::XColorTable::~XColorTable(void) {
  if (! colors.empty()) {
    const ScreenInfo * const screeninfo = _dpy.screenNumber(_screen);
    const Colormap colormap = screeninfo->getColormap();
    std::vector<unsigned long> pixvals(colors.size());
    std::vector<unsigned long>::iterator pt = pixvals.begin();
    std::vector<XColor>::const_iterator xt = colors.begin();
    for (; xt != colors.end() && pt != pixvals.end(); ++xt, ++pt)
      *pt = xt->pixel;

    XFreeColors(_dpy.XDisplay(), colormap, &pixvals[0], pixvals.size(), 0);

    pixvals.clear();
    colors.clear();
  }
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
  red   = _red  [red  ];
  green = _green[green];
  blue  = _blue [blue ];
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
    return colors[(red * _cpcsq) + (green * _cpc) + blue].pixel;

  case TrueColor:
  case DirectColor:
    return ((red   << red_offset) |
            (green << green_offset) |
            (blue  << blue_offset));
  }

  // not reached
  return 0; // shut up compiler warning
}


bt::Image::Image(unsigned int w, unsigned int h)
  :  width(w), height(h) {
  assert(width > 0  && width  < maximumWidth);
  assert(height > 0 && height < maximumHeight);

  red   = new unsigned char[width * height];
  green = new unsigned char[width * height];
  blue  = new unsigned char[width * height];
}


bt::Image::~Image(void) {
  delete [] red;
  delete [] green;
  delete [] blue;
}


Pixmap bt::Image::render(const Display &display, unsigned int screen,
                         const bt::Texture &texture) {
  if (texture.texture() & bt::Texture::Parent_Relative)
    return ParentRelative;
  else if (texture.texture() & bt::Texture::Solid)
    return render_solid(display, screen, texture);
  else if (texture.texture() & bt::Texture::Gradient)
    return render_gradient(display, screen, texture);
  return None;
}


Pixmap bt::Image::render_solid(const Display &display, unsigned int screen,
                               const bt::Texture &texture) {
  const ScreenInfo * const screeninfo = display.screenNumber(screen);

  Pixmap pixmap =
    XCreatePixmap(display.XDisplay(), screeninfo->getRootWindow(),
                  width, height, screeninfo->getDepth());
  if (pixmap == None)
    return None;

  Pen pen(screen, texture.color());
  Pen penlight(screen, texture.lightColor());
  Pen penshadow(screen, texture.shadowColor());

  XFillRectangle(display.XDisplay(), pixmap, pen.gc(), 0, 0, width, height);

  unsigned int bw = 0;
  if (texture.texture() & bt::Texture::Border) {
    Pen penborder(screen, texture.borderColor());
    bw = texture.borderWidth();

    for (unsigned int i = 0; i < bw; ++i)
      XDrawRectangle(display.XDisplay(), pixmap, penborder.gc(),
                     i, i, width - (i * 2) - 1, height - (i * 2) - 1);
  }

  if (texture.texture() & bt::Texture::Interlaced) {
    Pen peninterlace(screen, texture.colorTo());
    for (unsigned int i = bw; i < height - (bw * 2); i += 2)
      XDrawLine(display.XDisplay(), pixmap, peninterlace.gc(),
                bw, i, width - (bw * 2), i);
  }

  if (texture.texture() & bt::Texture::Raised) {
    XDrawLine(display.XDisplay(), pixmap, penshadow.gc(),
              bw, height - bw - 1, width - bw - 1, height - bw - 1);
    XDrawLine(display.XDisplay(), pixmap, penshadow.gc(),
              width - bw - 1, height - bw - 1, width - bw - 1, bw);

    XDrawLine(display.XDisplay(), pixmap, penlight.gc(),
              bw, bw, width - bw - 1, bw);
    XDrawLine(display.XDisplay(), pixmap, penlight.gc(),
              bw, height - bw - 1, bw, bw);
  } else if (texture.texture() & bt::Texture::Sunken) {
    XDrawLine(display.XDisplay(), pixmap, penlight.gc(),
              bw, height - bw - 1, width - bw - 1, height - bw - 1);
    XDrawLine(display.XDisplay(), pixmap, penlight.gc(),
              width - bw - 1, height - bw - 1, width - bw - 1, bw);

    XDrawLine(display.XDisplay(), pixmap, penshadow.gc(),
              bw, bw, width - bw - 1, bw);
    XDrawLine(display.XDisplay(), pixmap, penshadow.gc(),
              bw, height - bw - 1, bw, bw);
  }

  return pixmap;
}


Pixmap bt::Image::render_gradient(const Display &display, unsigned int screen,
                                  const bt::Texture &texture) {
  Color from, to;
  bool inverted = false;
  bool interlaced = texture.texture() & bt::Texture::Interlaced;

  if (texture.texture() & bt::Texture::Sunken) {
    from = texture.colorTo();
    to = texture.color();

    if (! (texture.texture() & bt::Texture::Invert))
      inverted = true;
  } else {
    from = texture.color();
    to = texture.colorTo();

    if (texture.texture() & bt::Texture::Invert)
      inverted = true;
  }

  if (texture.texture() & bt::Texture::Diagonal)
    dgradient(from, to, interlaced);
  else if (texture.texture() & bt::Texture::Elliptic)
    egradient(from, to, interlaced);
  else if (texture.texture() & bt::Texture::Horizontal)
    hgradient(from, to, interlaced);
  else if (texture.texture() & bt::Texture::Pyramid)
    pgradient(from, to, interlaced);
  else if (texture.texture() & bt::Texture::Rectangle)
    rgradient(from, to, interlaced);
  else if (texture.texture() & bt::Texture::Vertical)
    vgradient(from, to, interlaced);
  else if (texture.texture() & bt::Texture::CrossDiagonal)
    cdgradient(from, to, interlaced);
  else if (texture.texture() & bt::Texture::PipeCross)
    pcgradient(from, to, interlaced);

  if (texture.texture() & (bt::Texture::Sunken | bt::Texture::Raised))
    bevel(texture.borderWidth());

  if (inverted) invert();

  Pixmap pixmap = renderPixmap(display, screen);

  unsigned int bw = 0;
  if (texture.texture() & bt::Texture::Border) {
    Pen penborder(screen, texture.borderColor());
    bw = texture.borderWidth();

    for (unsigned int i = 0; i < bw; ++i)
      XDrawRectangle(display.XDisplay(), pixmap, penborder.gc(),
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
static const unsigned int dither16[16][16] = {
  {     0, 49152, 12288, 61440,  3072, 52224, 15360, 64512,
      768, 49920, 13056, 62208,  3840, 52992, 16128, 65280 },
  { 32768, 16384, 45056, 28672, 35840, 19456, 48128, 31744,
    33536, 17152, 45824, 29440, 36608, 20224, 48896, 32512 },
  {  8192, 57344,  4096, 53248, 11264, 60416,  7168, 56320,
     8960, 58112,  4864, 54016, 12032, 61184,  7936, 57088 },
  { 40960, 24576, 36864, 20480, 44032, 27648, 39936, 23552,
    41728, 25344, 37632, 21248, 44800, 28416, 40704, 24320 },
  {  2048, 51200, 14336, 63488,  1024, 50176, 13312, 62464,
     2816, 51968, 15104, 64256,  1792, 50944, 14080, 63232 },
  { 34816, 18432, 47104, 30720, 33792, 17408, 46080, 29696,
    35584, 19200, 47872, 31488, 34560, 18176, 46848, 30464 },
  { 10240, 59392,  6144, 55296,  9216, 58368,  5120, 54272,
    11008, 60160,  6912, 56064,  9984, 59136,  5888, 55040 },
  { 43008, 26624, 38912, 22528, 41984, 25600, 37888, 21504,
    43776, 27392, 39680, 23296, 42752, 26368, 38656, 22272 },
  {   512, 49664, 12800, 61952,  3584, 52736, 15872, 65024,
      256, 49408, 12544, 61696,  3328, 52480, 15616, 64768 },
  { 33280, 16896, 45568, 29184, 36352, 19968, 48640, 32256,
    33024, 16640, 45312, 28928, 36096, 19712, 48384, 32000 },
  {  8704, 57856,  4608, 53760, 11776, 60928,  7680, 56832,
     8448, 57600,  4352, 53504, 11520, 60672,  7424, 56576 },
  { 41472, 25088, 37376, 20992, 44544, 28160, 40448, 24064,
    41216, 24832, 37120, 20736, 44288, 27904, 40192, 23808 },
  {  2560, 51712, 14848, 64000,  1536, 50688, 13824, 62976,
     2304, 51456, 14592, 63744,  1280, 50432, 13568, 62720 },
  { 35328, 18944, 47616, 31232, 34304, 17920, 46592, 30208,
    35072, 18688, 47360, 30976, 34048, 17664, 46336, 29952 },
  { 10752, 59904,  6656, 55808,  9728, 58880,  5632, 54784,
    10496, 59648,  6400, 55552,  9472, 58624,  5376, 54528 },
  { 43520, 27136, 39424, 23040, 42496, 26112, 38400, 22016,
    43264, 26880, 39168, 22784, 42240, 25856, 38144, 21760 }
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
    dithy = y & 3;

    for (x = 0; x < width; ++x, ++offset) {
      dithx = x & 3;

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
  unsigned int x, y, dithx, dithy, r, g, b, error, offset;
  unsigned int cpc = colorsPerChannel() - 1;
  unsigned char *ppixel_data = pixel_data;

  for (y = 0, offset = 0; y < height; y++) {
    dithy = y & 15;

    for (x = 0; x < width; x++, offset++) {
      dithx = x & 15;

      error = dither16[dithy][dithx];

      r = (((256 * cpc + cpc + 1) * red  [offset] + error) / 65536);
      g = (((256 * cpc + cpc + 1) * green[offset] + error) / 65536);
      b = (((256 * cpc + cpc + 1) * blue [offset] + error) / 65536);

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

      r =   red[offset + x];
      g = green[offset + x];
      b =  blue[offset + x];

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


XImage *bt::Image::renderXImage(const Display &display, unsigned int screen) {
  // get the colortable for the screen. if necessary, we will create one.
  if (colorTableList.empty())
    colorTableList.resize(display.screenCount(), 0);

  if (! colorTableList[screen])
    colorTableList[screen] =
      new XColorTable(display, screen, colorsPerChannel());

  XColorTable *colortable = colorTableList[screen];

  // create XImage
  const ScreenInfo * const screeninfo = display.screenNumber(screen);
  XImage *image =
    XCreateImage(display.XDisplay(), screeninfo->getVisual(),
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

  if ( isDitherEnabled() && screeninfo->getDepth() < 24 &&
       width > 1 && height > 1) {
    switch (screeninfo->getVisual()->c_class) {
    case TrueColor:
    case DirectColor:
      TrueColorDither(colortable, o, image->bytes_per_line, d);
      break;

    case StaticColor:
    case PseudoColor: {
      // PseudoColorDither(colortable, image->bytes_per_line, d);
      OrderedPseudoColorDither(colortable, image->bytes_per_line, d);
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


Pixmap bt::Image::renderPixmap(const Display &display, unsigned int screen) {
  const ScreenInfo * const screeninfo = display.screenNumber(screen);
  Pixmap pixmap =
    XCreatePixmap(display.XDisplay(), screeninfo->getRootWindow(),
                  width, height, screeninfo->getDepth());

  if (pixmap == None)
    return None;

  XImage *image = renderXImage(display, screen);

  if (! image) {
    XFreePixmap(display.XDisplay(), pixmap);
    return None;
  }

  if (! image->data) {
    XDestroyImage(image);
    XFreePixmap(display.XDisplay(), pixmap);
    return None;
  }

  Pen pen(screen, Color(0, 0, 0));
  XPutImage(display.XDisplay(), pixmap, pen.gc(), image,
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


void bt::Image::dgradient(const Color &from, const Color &to,
                          bool interlaced) {
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


void bt::Image::hgradient(const Color &from, const Color &to,
                          bool interlaced) {
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


void bt::Image::vgradient(const Color &from, const Color &to,
                          bool interlaced) {
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


void bt::Image::pgradient(const Color &from, const Color &to,
                          bool interlaced) {
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


void bt::Image::rgradient(const Color &from, const Color &to,
                          bool interlaced) {
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


void bt::Image::egradient(const Color &from, const Color &to,
                          bool interlaced) {
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


void bt::Image::pcgradient(const Color &from, const Color &to,
                           bool interlaced) {
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


void bt::Image::cdgradient(const Color &from, const Color &to,
                           bool interlaced) {
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

  xt[x][0] = static_cast<unsigned char>(xr);
  xt[x][1] = static_cast<unsigned char>(xg);
  xt[x][2] = static_cast<unsigned char>(xb);

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
