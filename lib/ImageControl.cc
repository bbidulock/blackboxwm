// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// ImageControl.cc for Blackbox - an X11 Window manager
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

#ifdef    HAVE_CTYPE_H
#  include <ctype.h>
#endif // HAVE_CTYPE_H

#include <X11/Xlib.h>
}

#include <algorithm>

#include "BaseDisplay.hh"
#include "Color.hh"
#include "Image.hh"
#include "Texture.hh"

static unsigned long bsqrt(unsigned long x) {
  if (x <= 0) return 0;
  if (x == 1) return 1;

  unsigned long r = x >> 1;
  unsigned long q;

  while (1) {
    q = x / r;
    if (q >= r) return r;
    r = (r + q) >> 1;
  }
}

bt::ImageControl *ctrl = 0;

bt::ImageControl::ImageControl(TimerQueueManager *app,
                               Display &_display, 
                               const ScreenInfo *scrn,
                               bool _dither, int _cpc,
                               unsigned long cache_timeout,
                               unsigned long cmax): display(_display)
{
  if (! ctrl) ctrl = this;

  screeninfo = scrn;
  setDither(_dither);
  setColorsPerChannel(_cpc);

  cache_max = cmax;
  if (cache_timeout) {
    timer = new Timer(app, this);
    timer->setTimeout(cache_timeout);
    timer->start();
  } else {
    timer = (bt::Timer *) 0;
  }

  colors = (XColor *) 0;
  ncolors = 0;

  grad_xbuffer = grad_ybuffer = (unsigned int *) 0;
  grad_buffer_width = grad_buffer_height = 0;

  sqrt_table = (unsigned long *) 0;

  screen_depth = screeninfo->getDepth();
  window = screeninfo->getRootWindow();
  screen_number = screeninfo->getScreenNumber();
  colormap = screeninfo->getColormap();

  int count;
  XPixmapFormatValues *pmv = XListPixmapFormats(display.XDisplay(),
                                                &count);
  if (pmv) {
    bits_per_pixel = 0;
    for (int i = 0; i < count; i++)
      if (pmv[i].depth == screen_depth) {
	bits_per_pixel = pmv[i].bits_per_pixel;
	break;
      }

    XFree(pmv);
  }

  if (bits_per_pixel == 0) bits_per_pixel = screen_depth;
  if (bits_per_pixel >= 24) setDither(False);

  red_offset = green_offset = blue_offset = 0;

  switch (getVisual()->c_class) {
  case TrueColor: {
    // compute color tables
    unsigned long red_mask = getVisual()->red_mask,
      green_mask = getVisual()->green_mask,
      blue_mask = getVisual()->blue_mask;

    while (! (red_mask & 1)) { red_offset++; red_mask >>= 1; }
    while (! (green_mask & 1)) { green_offset++; green_mask >>= 1; }
    while (! (blue_mask & 1)) { blue_offset++; blue_mask >>= 1; }

    red_bits = 255 / red_mask;
    green_bits = 255 / green_mask;
    blue_bits = 255 / blue_mask;

    for (int i = 0; i < 256; i++) {
      red_color_table[i] = i / red_bits;
      green_color_table[i] = i / green_bits;
      blue_color_table[i] = i / blue_bits;
    }
    break;
  }

  case PseudoColor:
  case StaticColor: {
    ncolors = colors_per_channel * colors_per_channel * colors_per_channel;

    if (ncolors > (1 << screen_depth)) {
      colors_per_channel = (1 << screen_depth) / 3;
      ncolors = colors_per_channel * colors_per_channel * colors_per_channel;
    }

    if (colors_per_channel < 2 || ncolors > (1 << screen_depth)) {
      // invalid colormap size, reducing

      colors_per_channel = (1 << screen_depth) / 3;
    }

    colors = new XColor[ncolors];
    if (! colors)
      exit(1);

    int
#ifdef ORDEREDPSEUDO
      bits = 256 / colors_per_channel;
#else // !ORDEREDPSEUDO
      bits = 255 / (colors_per_channel - 1);
#endif // ORDEREDPSEUDO

    red_bits = green_bits = blue_bits = bits;

    int i = 0, ii, p, r, g, b;

    for (i = 0; i < 256; i++)
      red_color_table[i] = green_color_table[i] = blue_color_table[i] =
	i / bits;

    for (r = 0, i = 0; r < colors_per_channel; r++)
      for (g = 0; g < colors_per_channel; g++)
	for (b = 0; b < colors_per_channel; b++, i++) {
	  colors[i].red = (r * 0xffff) / (colors_per_channel - 1);
	  colors[i].green = (g * 0xffff) / (colors_per_channel - 1);
	  colors[i].blue = (b * 0xffff) / (colors_per_channel - 1);;
	  colors[i].flags = DoRed|DoGreen|DoBlue;
	}

    for (i = 0; i < ncolors; i++) {
      if (! XAllocColor(display.XDisplay(), colormap, &colors[i]))
	colors[i].flags = 0;
      else
	colors[i].flags = DoRed|DoGreen|DoBlue;
    }

    XColor icolors[256];
    int incolors = (((1 << screen_depth) > 256) ? 256 : (1 << screen_depth));

    for (i = 0; i < incolors; i++)
      icolors[i].pixel = i;

    XQueryColors(display.XDisplay(), colormap, icolors, incolors);
    for (i = 0; i < ncolors; i++) {
      if (! colors[i].flags) {
	unsigned long chk = 0xffffffff, pixel, close = 0;

	p = 2;
	while (p--) {
	  for (ii = 0; ii < incolors; ii++) {
	    r = (colors[i].red - icolors[i].red) >> 8;
	    g = (colors[i].green - icolors[i].green) >> 8;
	    b = (colors[i].blue - icolors[i].blue) >> 8;
	    pixel = (r * r) + (g * g) + (b * b);

	    if (pixel < chk) {
	      chk = pixel;
	      close = ii;
	    }

	    colors[i].red = icolors[close].red;
	    colors[i].green = icolors[close].green;
	    colors[i].blue = icolors[close].blue;

	    if (XAllocColor(display.XDisplay(), colormap,
			    &colors[i])) {
	      colors[i].flags = DoRed|DoGreen|DoBlue;
	      break;
	    }
	  }
	}
      }
    }

    break;
  }

  case GrayScale:
  case StaticGray: {
    if (getVisual()->c_class == StaticGray) {
      ncolors = 1 << screen_depth;
    } else {
      ncolors = colors_per_channel * colors_per_channel * colors_per_channel;

      if (ncolors > (1 << screen_depth)) {
	colors_per_channel = (1 << screen_depth) / 3;
	ncolors =
	  colors_per_channel * colors_per_channel * colors_per_channel;
      }
    }

    if (colors_per_channel < 2 || ncolors > (1 << screen_depth)) {
      // invalid colormap size, reducing

      colors_per_channel = (1 << screen_depth) / 3;
    }

    colors = new XColor[ncolors];
    if (! colors) {
      exit(1);
    }

    int i = 0, ii, p, bits = 255 / (colors_per_channel - 1);
    red_bits = green_bits = blue_bits = bits;

    for (i = 0; i < 256; i++)
      red_color_table[i] = green_color_table[i] = blue_color_table[i] =
	i / bits;

    for (i = 0; i < ncolors; i++) {
      colors[i].red = (i * 0xffff) / (colors_per_channel - 1);
      colors[i].green = (i * 0xffff) / (colors_per_channel - 1);
      colors[i].blue = (i * 0xffff) / (colors_per_channel - 1);;
      colors[i].flags = DoRed|DoGreen|DoBlue;

      if (! XAllocColor(display.XDisplay(), colormap,
			&colors[i]))
	colors[i].flags = 0;
      else
	colors[i].flags = DoRed|DoGreen|DoBlue;
    }

    XColor icolors[256];
    int incolors = (((1 << screen_depth) > 256) ? 256 :
		    (1 << screen_depth));

    for (i = 0; i < incolors; i++)
      icolors[i].pixel = i;

    XQueryColors(display.XDisplay(), colormap, icolors, incolors);
    for (i = 0; i < ncolors; i++) {
      if (! colors[i].flags) {
	unsigned long chk = 0xffffffff, pixel, close = 0;

	p = 2;
	while (p--) {
	  for (ii = 0; ii < incolors; ii++) {
	    int r = (colors[i].red - icolors[i].red) >> 8;
	    int g = (colors[i].green - icolors[i].green) >> 8;
	    int b = (colors[i].blue - icolors[i].blue) >> 8;
	    pixel = (r * r) + (g * g) + (b * b);

	    if (pixel < chk) {
	      chk = pixel;
	      close = ii;
	    }

	    colors[i].red = icolors[close].red;
	    colors[i].green = icolors[close].green;
	    colors[i].blue = icolors[close].blue;

	    if (XAllocColor(display.XDisplay(), colormap,
			    &colors[i])) {
	      colors[i].flags = DoRed|DoGreen|DoBlue;
	      break;
	    }
	  }
	}
      }
    }

    break;
  }

  default:
    // unsupported visual
    exit(1);
  }
}


bt::ImageControl::~ImageControl(void) {
  delete [] sqrt_table;
  delete [] grad_xbuffer;
  delete [] grad_ybuffer;

  if (colors) {
    unsigned long *pixels = new unsigned long [ncolors];

    for (int i = 0; i < ncolors; i++)
      *(pixels + i) = (*(colors + i)).pixel;

    XFreeColors(display.XDisplay(), colormap, pixels, ncolors, 0);

    delete [] colors;
  }

  if (!cache.empty()) {
    CacheContainer::iterator it = cache.begin();
    const CacheContainer::iterator end = cache.end();
    for (; it != end; ++it)
      XFreePixmap(display.XDisplay(), it->pixmap);
  }
  if (timer) {
    timer->stop();
    delete timer;
  }
}


Pixmap bt::ImageControl::searchCache(const unsigned int width,
                                     const unsigned int height,
                                     const unsigned long texture,
                                     const bt::Color &c1,
                                     const bt::Color &c2) {
  if (cache.empty())
    return None;

  CacheContainer::iterator it = cache.begin();
  const CacheContainer::iterator end = cache.end();
  for (; it != end; ++it) {
    CachedImage& tmp = *it;
    if (tmp.width == width && tmp.height == height &&
        tmp.texture == texture && tmp.pixel1 == c1.pixel())
      if (texture & bt::Texture::Gradient) {
        if (tmp.pixel2 == c2.pixel()) {
          tmp.count++;
          return tmp.pixmap;
        }
      } else {
        tmp.count++;
        return tmp.pixmap;
      }
  }
  return None;
}


Pixmap bt::ImageControl::renderImage(unsigned int width, unsigned int height,
                                     const bt::Texture &texture) {
  if (texture.texture() & bt::Texture::Parent_Relative) return ParentRelative;

  Pixmap pixmap = searchCache(width, height, texture.texture(),
			      texture.color(), texture.colorTo());
  if (pixmap) return pixmap;

  bt::Image image(this, width, height);
  pixmap = image.render(texture);

  if (!pixmap)
    return None;

  CachedImage tmp;

  tmp.pixmap = pixmap;
  tmp.width = width;
  tmp.height = height;
  tmp.count = 1;
  tmp.texture = texture.texture();
  tmp.pixel1 = texture.color().pixel();

  if (texture.texture() & bt::Texture::Gradient)
    tmp.pixel2 = texture.colorTo().pixel();
  else
    tmp.pixel2 = 0l;

  cache.push_back(tmp);

  if (cache.size() > cache_max)
    timeout();

  return pixmap;
}


void bt::ImageControl::removeImage(Pixmap pixmap) {
  if (!pixmap)
    return;

  CacheContainer::iterator it = cache.begin();
  const CacheContainer::iterator end = cache.end();
  for (; it != end; ++it) {
    CachedImage &tmp = *it;
    if (tmp.pixmap == pixmap && tmp.count > 0)
      tmp.count--;
  }

  if (! timer)
    timeout();
}


void bt::ImageControl::getColorTables(unsigned char **rmt,
                                      unsigned char **gmt,
                                      unsigned char **bmt,
                                      int *roff, int *goff, int *boff,
                                      int *rbit, int *gbit, int *bbit) {
  if (rmt) *rmt = red_color_table;
  if (gmt) *gmt = green_color_table;
  if (bmt) *bmt = blue_color_table;

  if (roff) *roff = red_offset;
  if (goff) *goff = green_offset;
  if (boff) *boff = blue_offset;

  if (rbit) *rbit = red_bits;
  if (gbit) *gbit = green_bits;
  if (bbit) *bbit = blue_bits;
}


void bt::ImageControl::getXColorTable(XColor **c, int *n) {
  if (c) *c = colors;
  if (n) *n = ncolors;
}


void bt::ImageControl::getGradientBuffers(unsigned int w,
				       unsigned int h,
				       unsigned int **xbuf,
				       unsigned int **ybuf) {
  if (w > grad_buffer_width) {
    if (grad_xbuffer)
      delete [] grad_xbuffer;

    grad_buffer_width = w;

    grad_xbuffer = new unsigned int[grad_buffer_width * 3];
  }

  if (h > grad_buffer_height) {
    if (grad_ybuffer)
      delete [] grad_ybuffer;

    grad_buffer_height = h;

    grad_ybuffer = new unsigned int[grad_buffer_height * 3];
  }

  *xbuf = grad_xbuffer;
  *ybuf = grad_ybuffer;
}


void bt::ImageControl::installRootColormap(void) {
  int ncmap = 0;
  Colormap *cmaps =
    XListInstalledColormaps(display.XDisplay(), window, &ncmap);

  if (cmaps) {
    bool install = True;
    for (int i = 0; i < ncmap; i++)
      if (*(cmaps + i) == colormap)
	install = False;

    if (install)
      XInstallColormap(display.XDisplay(), colormap);

    XFree(cmaps);
  }
}


void bt::ImageControl::setColorsPerChannel(int cpc) {
  if (cpc < 2) cpc = 2;
  if (cpc > 6) cpc = 6;

  colors_per_channel = cpc;
}


unsigned long bt::ImageControl::getSqrt(unsigned int x) {
  if (! sqrt_table) {
    // build sqrt table for use with elliptic gradient

    sqrt_table = new unsigned long[(256 * 256 * 2) + 1];

    for (int i = 0; i < (256 * 256 * 2); i++)
      *(sqrt_table + i) = bsqrt(i);
  }

  return (*(sqrt_table + x));
}


struct ZeroRefCheck {
  inline bool operator()(const bt::ImageControl::CachedImage &image) const {
    return (image.count == 0);
  }
};

struct CacheCleaner {
  ::Display *display;
  ZeroRefCheck ref_check;
  CacheCleaner(::Display *d): display(d) {}
  inline void operator()(const bt::ImageControl::CachedImage& image) const {
    if (ref_check(image))
      XFreePixmap(display, image.pixmap);
  }
};


void bt::ImageControl::timeout(void) {
  CacheCleaner cleaner(display.XDisplay());
  std::for_each(cache.begin(), cache.end(), cleaner);
  cache.remove_if(cleaner.ref_check);
}
