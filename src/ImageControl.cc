// -*- mode: C++; indent-tabs-mode: nil; -*-
// ImageControl.cc for Blackbox - an X11 Window manager
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    HAVE_CTYPE_H
#  include <ctype.h>
#endif // HAVE_CTYPE_H

#include <algorithm>

#include "blackbox.hh"
#include "i18n.hh"
#include "BaseDisplay.hh"
#include "Image.hh"

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


BImageControl::BImageControl(BaseDisplay *dpy, ScreenInfo *scrn, Bool _dither,
                             int _cpc, unsigned long cache_timeout,
                             unsigned long cmax)
{
  basedisplay = dpy;
  screeninfo = scrn;
  setDither(_dither);
  setColorsPerChannel(_cpc);

  cache_max = cmax;
#ifdef    TIMEDCACHE
  if (cache_timeout) {
    timer = new BTimer(basedisplay, this);
    timer->setTimeout(cache_timeout);
    timer->start();
  } else {
    timer = (BTimer *) 0;
  }
#endif // TIMEDCACHE

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
  XPixmapFormatValues *pmv = XListPixmapFormats(basedisplay->getXDisplay(),
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
    int i;

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

    for (i = 0; i < 256; i++) {
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
      fprintf(stderr,
	      i18n(ImageSet, ImageInvalidColormapSize,
                   "BImageControl::BImageControl: invalid colormap size %d "
                   "(%d/%d/%d) - reducing"),
	      ncolors, colors_per_channel, colors_per_channel,
	      colors_per_channel);

      colors_per_channel = (1 << screen_depth) / 3;
    }

    colors = new XColor[ncolors];
    if (! colors) {
      fprintf(stderr, i18n(ImageSet, ImageErrorAllocatingColormap,
                           "BImageControl::BImageControl: error allocating "
                           "colormap\n"));
      exit(1);
    }

    int i = 0, ii, p, r, g, b,

#ifdef ORDEREDPSEUDO
      bits = 256 / colors_per_channel;
#else // !ORDEREDPSEUDO
    bits = 255 / (colors_per_channel - 1);
#endif // ORDEREDPSEUDO

    red_bits = green_bits = blue_bits = bits;

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
      if (! XAllocColor(basedisplay->getXDisplay(), colormap, &colors[i])) {
	fprintf(stderr, i18n(ImageSet, ImageColorAllocFail,
                             "couldn't alloc color %i %i %i\n"),
		colors[i].red, colors[i].green, colors[i].blue);
	colors[i].flags = 0;
      } else {
	colors[i].flags = DoRed|DoGreen|DoBlue;
      }
    }

    XColor icolors[256];
    int incolors = (((1 << screen_depth) > 256) ? 256 : (1 << screen_depth));

    for (i = 0; i < incolors; i++)
      icolors[i].pixel = i;

    XQueryColors(basedisplay->getXDisplay(), colormap, icolors, incolors);
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

	    if (XAllocColor(basedisplay->getXDisplay(), colormap,
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
      fprintf(stderr,
              i18n(ImageSet, ImageInvalidColormapSize,
	           "BImageControl::BImageControl: invalid colormap size %d "
	            "(%d/%d/%d) - reducing"),
	      ncolors, colors_per_channel, colors_per_channel,
	      colors_per_channel);

      colors_per_channel = (1 << screen_depth) / 3;
    }

    colors = new XColor[ncolors];
    if (! colors) {
      fprintf(stderr,
              i18n(ImageSet, ImageErrorAllocatingColormap,
                 "BImageControl::BImageControl: error allocating colormap\n"));
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

      if (! XAllocColor(basedisplay->getXDisplay(), colormap,
			&colors[i])) {
	fprintf(stderr, i18n(ImageSet, ImageColorAllocFail,
			     "couldn't alloc color %i %i %i\n"),
		colors[i].red, colors[i].green, colors[i].blue);
	colors[i].flags = 0;
      } else {
	colors[i].flags = DoRed|DoGreen|DoBlue;
      }
    }

    XColor icolors[256];
    int incolors = (((1 << screen_depth) > 256) ? 256 :
		    (1 << screen_depth));

    for (i = 0; i < incolors; i++)
      icolors[i].pixel = i;

    XQueryColors(basedisplay->getXDisplay(), colormap, icolors, incolors);
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

	    if (XAllocColor(basedisplay->getXDisplay(), colormap,
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
    fprintf(stderr,
            i18n(ImageSet, ImageUnsupVisual,
                 "BImageControl::BImageControl: unsupported visual %d\n"),
	    getVisual()->c_class);
    exit(1);
  }
}


BImageControl::~BImageControl(void) {
  if (sqrt_table) {
    delete [] sqrt_table;
  }

  if (grad_xbuffer) {
    delete [] grad_xbuffer;
  }

  if (grad_ybuffer) {
    delete [] grad_ybuffer;
  }

  if (colors) {
    unsigned long *pixels = new unsigned long [ncolors];

    int i;
    for (i = 0; i < ncolors; i++)
      *(pixels + i) = (*(colors + i)).pixel;

    XFreeColors(basedisplay->getXDisplay(), colormap, pixels, ncolors, 0);

    delete [] colors;
  }

  if (!cache.empty()) {
    //#ifdef DEBUG
    fprintf(stderr, i18n(ImageSet, ImagePixmapRelease,
		         "BImageContol::~BImageControl: pixmap cache - "
	                 "releasing %d pixmaps\n"), cache.size());
    //#endif
    CacheContainer::iterator it = cache.begin();
    const CacheContainer::iterator end = cache.end();
    for (; it != end; ++it) {
      XFreePixmap(basedisplay->getXDisplay(), (*it).pixmap);
    }
  }
#ifdef    TIMEDCACHE
  if (timer) {
    timer->stop();
    delete timer;
  }
#endif // TIMEDCACHE
}


Pixmap BImageControl::searchCache(unsigned int width, unsigned int height,
      unsigned long texture,
      BColor *c1, BColor *c2) {
  if (cache.empty())
    return None;

  CacheContainer::iterator it = cache.begin();
  const CacheContainer::iterator end = cache.end();
  for (; it != end; ++it) {
    CachedImage &tmp = *it;
    if ((tmp.width == width) && (tmp.height == height) &&
        (tmp.texture == texture) && (tmp.pixel1 == c1->getPixel()))
      if (texture & BImage_Gradient) {
        if (tmp.pixel2 == c2->getPixel()) {
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


Pixmap BImageControl::renderImage(unsigned int width, unsigned int height,
      BTexture *texture) {
  if (texture->getTexture() & BImage_ParentRelative) return ParentRelative;

  Pixmap pixmap = searchCache(width, height, texture->getTexture(),
			      texture->getColor(), texture->getColorTo());
  if (pixmap) return pixmap;

  BImage image(this, width, height);
  pixmap = image.render(texture);

  if (!pixmap)
    return None;

  CachedImage tmp;

  tmp.pixmap = pixmap;
  tmp.width = width;
  tmp.height = height;
  tmp.count = 1;
  tmp.texture = texture->getTexture();
  tmp.pixel1 = texture->getColor()->getPixel();

  if (texture->getTexture() & BImage_Gradient)
    tmp.pixel2 = texture->getColorTo()->getPixel();
  else
    tmp.pixel2 = 0l;

  cache.push_back(tmp);

  if ((unsigned) cache.size() > cache_max) {
    //#ifdef    DEBUG
    fprintf(stderr, i18n(ImageSet, ImagePixmapCacheLarge,
			 "BImageControl::renderImage: cache is large, "
			 "forcing cleanout\n"));
    //#endif // DEBUG

    timeout();
  }

  return pixmap;
}


void BImageControl::removeImage(Pixmap pixmap) {
  if (!pixmap)
    return;

  CacheContainer::iterator it = cache.begin();
  const CacheContainer::iterator end = cache.end();
  for (; it != end; ++it) {
    CachedImage &tmp = *it;
    if (tmp.pixmap == pixmap && tmp.count > 0)
      tmp.count--;
  }

#ifdef    TIMEDCACHE
  if (! timer)
#endif // TIMEDCACHE
  {
    fprintf(stderr, "flushing cache in removeImage()\n");
    timeout();
  }
}


unsigned long BImageControl::getColor(const char *colorname,
				      unsigned char *r, unsigned char *g,
				      unsigned char *b)
{
  XColor color;
  color.pixel = 0;

  if (! XParseColor(basedisplay->getXDisplay(), colormap, colorname, &color))
    fprintf(stderr, "BImageControl::getColor: color parse error: \"%s\"\n",
	    colorname);
  else if (! XAllocColor(basedisplay->getXDisplay(), colormap, &color))
    fprintf(stderr, "BImageControl::getColor: color alloc error: \"%s\"\n",
	    colorname);

  if (color.red == 65535) *r = 0xff;
  else *r = (unsigned char) (color.red / 0xff);
  if (color.green == 65535) *g = 0xff;
  else *g = (unsigned char) (color.green / 0xff);
  if (color.blue == 65535) *b = 0xff;
  else *b = (unsigned char) (color.blue / 0xff);

  return color.pixel;
}


unsigned long BImageControl::getColor(const char *colorname) {
  XColor color;
  color.pixel = 0;

  if (! XParseColor(basedisplay->getXDisplay(), colormap, colorname, &color))
    fprintf(stderr, "BImageControl::getColor: color parse error: \"%s\"\n",
	    colorname);
  else if (! XAllocColor(basedisplay->getXDisplay(), colormap, &color))
    fprintf(stderr, "BImageControl::getColor: color alloc error: \"%s\"\n",
	    colorname);

  return color.pixel;
}


void BImageControl::getColorTables(unsigned char **rmt, unsigned char **gmt,
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


void BImageControl::getXColorTable(XColor **c, int *n) {
  if (c) *c = colors;
  if (n) *n = ncolors;
}


void BImageControl::getGradientBuffers(unsigned int w,
				       unsigned int h,
				       unsigned int **xbuf,
				       unsigned int **ybuf)
{
  if (w > grad_buffer_width) {
    if (grad_xbuffer) {
      delete [] grad_xbuffer;
    }

    grad_buffer_width = w;

    grad_xbuffer = new unsigned int[grad_buffer_width * 3];
  }

  if (h > grad_buffer_height) {
    if (grad_ybuffer) {
      delete [] grad_ybuffer;
    }

    grad_buffer_height = h;

    grad_ybuffer = new unsigned int[grad_buffer_height * 3];
  }

  *xbuf = grad_xbuffer;
  *ybuf = grad_ybuffer;
}


void BImageControl::installRootColormap(void) {
  Bool install = True;
  int i = 0, ncmap = 0;
  Colormap *cmaps =
    XListInstalledColormaps(basedisplay->getXDisplay(), window, &ncmap);

  if (cmaps) {
    for (i = 0; i < ncmap; i++)
      if (*(cmaps + i) == colormap)
	install = False;

    if (install)
      XInstallColormap(basedisplay->getXDisplay(), colormap);

    XFree(cmaps);
  }
}


void BImageControl::setColorsPerChannel(int cpc) {
  if (cpc < 2) cpc = 2;
  if (cpc > 6) cpc = 6;

  colors_per_channel = cpc;
}


unsigned long BImageControl::getSqrt(unsigned int x) {
  if (! sqrt_table) {
    // build sqrt table for use with elliptic gradient

    sqrt_table = new unsigned long[(256 * 256 * 2) + 1];

    for (int i = 0; i < (256 * 256 * 2); i++)
      *(sqrt_table + i) = bsqrt(i);
  }

  return (*(sqrt_table + x));
}


void BImageControl::parseTexture(BTexture *texture, char *t) {
  if ((! texture) || (! t)) return;

  int t_len = strlen(t) + 1, i;
  char *ts = new char[t_len];
  if (! ts) return;

  // convert to lower case
  for (i = 0; i < t_len; i++)
    *(ts + i) = tolower(*(t + i));

  if (strstr(ts, "parentrelative")) {
    texture->setTexture(BImage_ParentRelative);
  } else {
    texture->setTexture(0);

    if (strstr(ts, "solid"))
      texture->addTexture(BImage_Solid);
    else if (strstr(ts, "gradient")) {
      texture->addTexture(BImage_Gradient);
      if (strstr(ts, "crossdiagonal"))
	texture->addTexture(BImage_CrossDiagonal);
      else if (strstr(ts, "rectangle"))
	texture->addTexture(BImage_Rectangle);
      else if (strstr(ts, "pyramid"))
	texture->addTexture(BImage_Pyramid);
      else if (strstr(ts, "pipecross"))
	texture->addTexture(BImage_PipeCross);
      else if (strstr(ts, "elliptic"))
	texture->addTexture(BImage_Elliptic);
      else if (strstr(ts, "diagonal"))
	texture->addTexture(BImage_Diagonal);
      else if (strstr(ts, "horizontal"))
	texture->addTexture(BImage_Horizontal);
      else if (strstr(ts, "vertical"))
	texture->addTexture(BImage_Vertical);
      else
	texture->addTexture(BImage_Diagonal);
    } else {
      texture->addTexture(BImage_Solid);
    }

    if (strstr(ts, "raised"))
      texture->addTexture(BImage_Raised);
    else if (strstr(ts, "sunken"))
      texture->addTexture(BImage_Sunken);
    else if (strstr(ts, "flat"))
      texture->addTexture(BImage_Flat);
    else
      texture->addTexture(BImage_Raised);

    if (! (texture->getTexture() & BImage_Flat)) {
      if (strstr(ts, "bevel2"))
	texture->addTexture(BImage_Bevel2);
      else
	texture->addTexture(BImage_Bevel1);
    }

#ifdef    INTERLACE
    if (strstr(ts, "interlaced"))
      texture->addTexture(BImage_Interlaced);
#endif // INTERLACE
  }

  delete [] ts;
}


void BImageControl::parseColor(BColor *color, char *c) {
  if (! color) return;

  if (color->isAllocated()) {
    unsigned long pixel = color->getPixel();

    XFreeColors(basedisplay->getXDisplay(), colormap, &pixel, 1, 0);

    color->setPixel(0l);
    color->setRGB(0, 0, 0);
    color->setAllocated(False);
  }

  if (c) {
    unsigned char r, g, b;

    color->setPixel(getColor(c, &r, &g, &b));
    color->setRGB(r, g, b);
    color->setAllocated(True);
  }
}

struct ZeroRefCheck {
  bool operator()(const BImageControl::CachedImage &image) const {
    return (image.count == 0);
  }
};

void BImageControl::timeout(void) {
  fprintf(stderr, "timeout handler, %d\n", cache.size());
  cache.erase(std::remove_if(cache.begin(), cache.end(), ZeroRefCheck()),
              cache.end());
  fprintf(stderr, "timeout handler done, %d\n", cache.size());
}
