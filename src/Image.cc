//
// Image.cc for Blackbox - an X11 Window manager
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "blackbox.hh"
#include "Image.hh"

#include <stdio.h>
#include <malloc.h>

#ifdef GradientHack
#  include <math.h>
#  include <float.h>
#endif


// *************************************************************************
// Graphics engine class code
// *************************************************************************

BImage::BImage(BImageControl *c, unsigned int w, unsigned int h) {
  control = c;

  width = ((signed) w > 0) ? w : 1;
  height = ((signed) h > 0) ? h : 1;

  red = new unsigned char[width * height];
  green = new unsigned char[width * height];
  blue = new unsigned char[width * height];
}


BImage::~BImage(void) {
  if (red) delete [] red;
  if (green) delete [] green;
  if (blue) delete [] blue;
}


Pixmap BImage::render(unsigned long texture, const BColor &c1,
                      const BColor &c2)
{
  if (texture & BImage_Solid)
    return render_solid(texture, c1);
  else if (texture & BImage_Gradient)
    return render_gradient(texture, c1, c2);

  return None;
}


Pixmap BImage::render_solid(unsigned long texture, const BColor &color) {
  background(color);

  if (texture & BImage_Raised) {
    if (texture & BImage_Bevel1) {
      if (color.red == color.green && color.green == color.blue &&
          color.blue == 0)
        bevel1(True, True);
      else
        bevel1(True);
    } else if (texture & BImage_Bevel2) {
      if (color.red == color.green && color.green == color.blue &&
          color.blue == 0)
        bevel2(True, True);
      else
        bevel2(True);
    }

    if (texture & BImage_Invert)
      invert();
  } else if (texture & BImage_Sunken) {
    if (texture & BImage_Bevel1) {
      if (color.red == color.green && color.green == color.blue &&
          color.blue == 0)
        bevel1(True, True);
      else
        bevel1(True);
    } else if (texture & BImage_Bevel2) {
      if (color.red == color.green && color.green == color.blue &&
          color.blue == 0)
        bevel2(True, True);
      else
        bevel2(True);
    }

    if (! (texture & BImage_Invert))
      invert();
  }

  return renderPixmap(! (texture & BImage_NoDitherSolid));
}


Pixmap BImage::render_gradient(unsigned long texture, const BColor &color1,
                               const BColor &color2) {
  if (texture & BImage_Diagonal) {
    if (texture & BImage_Sunken) {
      from = color2;
      to = color1;
      dgradient();

      if (texture & BImage_Bevel1)
        bevel1(False);
      else if (texture & BImage_Bevel2)
        bevel2(False);

      if (! (texture & BImage_Invert))
        invert();
    } else {
      from = color1;
      to = color2;
      dgradient();

      if (texture & BImage_Bevel1)
        bevel1(False);
      else if (texture & BImage_Bevel2)
        bevel2(False);

      if (texture & BImage_Invert)
        invert();
    }
  } else if (texture & BImage_Horizontal) {
    if (texture & BImage_Sunken) {
      from = color2;
      to = color1;
      hgradient();

      if (texture & BImage_Bevel1)
        bevel1(False);
      else if (texture & BImage_Bevel2)
        bevel2(False);

      if (! (texture & BImage_Invert))
        invert();
    } else {
      from = color1;
      to = color2;
      hgradient();

      if (texture & BImage_Bevel1)
        bevel1(False);
      else if (texture & BImage_Bevel2)
        bevel2(False);

      if (texture & BImage_Invert)
        invert();
    }
  } else if (texture & BImage_Vertical) {
    if (texture & BImage_Sunken) {
      from = color2;
      to = color1;
      vgradient();

      if (texture & BImage_Bevel1)
        bevel1(False);
      else if (texture & BImage_Bevel2)
        bevel2(False);

      if (! (texture & BImage_Invert))
        invert();
    } else {
      from = color1;
      to = color2;
      vgradient();

      if (texture & BImage_Bevel1)
        bevel1(False);
      else if (texture & BImage_Bevel2)
        bevel2(False);

      if (texture & BImage_Invert)
        invert();
    }
  }

  return renderPixmap();
}


XImage *BImage::renderXImage(Bool dither) {
  XImage *image = XCreateImage(control->d(), control->v(), control->depth(),
                               ZPixmap, 0, 0, width, height, 32, 0);
  if (! image) return 0;

  unsigned char *d = (unsigned char *) malloc(image->bytes_per_line * height);
  if (! d) return 0;

  register unsigned int x, y, r = 0, g = 0, b = 0;

  unsigned char *idata = d;
  unsigned int i, ofs;
  unsigned long pixel;
  unsigned short *tr = 0, *tg = 0, *tb = 0;

  int ncolors;
  XColor *colors;
  int cpc = control->colorsPerChannel(), cpccpc = cpc * cpc;
  
  control->getMaskTables(&tr, &tg, &tb, &roff, &goff, &boff);
  if ((! tr) || (! tg) || (! tb)) {
    XDestroyImage(image);
    return 0;
  }
  
  if (control->v()->c_class != TrueColor) {
    control->getColorTable(&colors, &ncolors);
    if ((! colors) || (! ncolors)) {
      XDestroyImage(image);
      return 0;
    }
  }

  if (control->dither() && dither) {
    short er, eg, eb, *or, *og, *ob, *nor, *nog, *nob;
    
    control->getDitherBuffers(width + 2, &or, &og, &ob, &nor, &nog, &nob);
    if ((! or) || (! og) || (! ob) || (! nor) || (! nog) || (! nob)) {
      XDestroyImage(image);
      return 0;
    }
    
    for (x = 0; x < width; x++) {
      *(or + x) = *(red + x);
      *(og + x) = *(green + x);
      *(ob + x) = *(blue + x);
    }
    
    *(or + x) = *(og + x) = *(ob + x) = 0;
    
    for (y = 0, ofs = 0; y < height; y++) {
      if (y < (height - 1)) {
	for (x = 0, i = ofs + width; x < width; x++, i++) {
	  *(nor + x) = *(red + i);
	  *(nog + x) = *(green + i);
	  *(nob + x) = *(blue + i);
	}
	
	i--;
	*(nor + x) = *(red + i);
	*(nog + x) = *(green + i);
	*(nob + x) = *(blue + i);
      }
      
      idata = d + (image->bytes_per_line * y);
      
      for (x = 0; x < width; x++) {
	if (*(or + x) > 255) *(or + x) = 255;
	else if (*(or + x) < 0) *(or + x) = 0;
	if (*(og + x) > 255) *(og + x) = 255;
	else if (*(og + x) < 0) *(og + x) = 0;
	if (*(ob + x) > 255) *(ob + x) = 255;
	else if (*(ob + x) < 0) *(ob + x) = 0;
	
	r = *(tr + *(or + x));
	g = *(tg + *(og + x));
	b = *(tb + *(ob + x));
	
	switch (control->v()->c_class) {
	case StaticColor:
	case PseudoColor:
	  pixel = (r * cpccpc) + (g * cpc) + b;
	  *idata++ = colors[pixel].pixel;

          er = *(or + x) - ((r << 8) / (cpc - 1));
          eg = *(og + x) - ((g << 8) / (cpc - 1));
          eb = *(ob + x) - ((b << 8) / (cpc - 1));

	  break;
	  
	case TrueColor:
	  pixel = (r << roff) | (g << goff) | (b << boff);
	  
	  switch (image->bits_per_pixel) {
	  case 16:
	    {
	      if (image->byte_order == MSBFirst) {
		*idata++ = (pixel >> 8);
		*idata++ = pixel;
	      } else {
		*idata++ = pixel;
		*idata++ = (pixel >> 8);
	      }
	      
	      break;
	    }
	    
	  case 24:
	    {
	      if (image->byte_order == MSBFirst) {
		*idata++ = (pixel >> 16);
		*idata++ = (pixel >> 8);
		*idata++ = pixel;
	      } else {
		*idata++ = pixel;
		*idata++ = (pixel >> 8);
		*idata++ = (pixel >> 16);
	      }
	      
	      break;
	    }
		
	  case 32:
	    {
	      if (image->byte_order == MSBFirst) {
		*idata++ = (pixel >> 24);
		*idata++ = (pixel >> 16);
		*idata++ = (pixel >> 8);
		*idata++ = pixel;
	      } else {
		*idata++ = pixel;
		*idata++ = (pixel >> 8);
		*idata++ = (pixel >> 16);
		*idata++ = (pixel >> 24);
	      }
	      
	      break;
	    }
	  }

          er = *(or + x) - ((r << 8) / (image->red_mask >> roff));
          eg = *(og + x) - ((g << 8) / (image->green_mask >> goff));
          eb = *(ob + x) - ((b << 8) / (image->blue_mask >> boff));

          break;

	default:
	  fprintf(stderr, "BImage::renderXImage: unsupported visual\n");
	  image->data = (char *) d;
	  XDestroyImage(image);
	  return 0;
	}

        r = (er >> 3) + (er >> 2);
        g = (eg >> 3) + (eg >> 2);
        b = (eb >> 3) + (eb >> 2);

	*(or + x + 1) += r;
	*(og + x + 1) += g;
	*(ob + x + 1) += b;
	
	*(nor + x) += r;
	*(nog + x) += g;
	*(nob + x) += b;
	
	*(nor + x + 1) += er - (r << 1);
	*(nog + x + 1) += eg - (g << 1);
	*(nob + x + 1) += eb - (b << 1);
      }
      
      ofs += image->width;
      
      short *t;
      t = or; or = nor; nor = t;
      t = og; og = nog; nog = t;
      t = ob; ob = nob; nob = t;
    }
  } else {
    for (y = 0, ofs = 0; y < height; y++) {
      idata = d + (image->bytes_per_line * y);
      
      for (x = 0; x < width; x++, ofs++) {
	r = *(tr + *(red + ofs));
	g = *(tg + *(green + ofs));
	b = *(tb + *(blue + ofs));
	
	switch (control->v()->c_class) {
	case StaticColor:
	case PseudoColor:
	  pixel = (r * cpccpc) + (g * cpc) + b;
	  *idata++ = colors[pixel].pixel;
	  break;
	  
	case TrueColor:
	  pixel = (r << roff) | (g << goff) | (b << boff);
	  
	  switch (image->bits_per_pixel) {
	  case 16:
	    if (image->byte_order == MSBFirst) {
	      *idata++ = (pixel >> 8);
	      *idata++ = pixel;
	    } else {
	      *idata++ = pixel;
	      *idata++ = (pixel >> 8);
	    }
	    break;
		
	  case 24:
	    if (image->byte_order == MSBFirst) {
	      *idata++ = (pixel >> 16);
	      *idata++ = (pixel >> 8);
	      *idata++ = pixel;
	    } else {
	      *idata++ = pixel;
	      *idata++ = (pixel >> 8);
	      *idata++ = (pixel >> 16);
	    }
	    break;
		
	  case 32:
	    if (image->byte_order == MSBFirst) {
	      *idata++ = (pixel >> 24);
	      *idata++ = (pixel >> 16);
	      *idata++ = (pixel >> 8);
	      *idata++ = pixel;
	    } else {
	      *idata++ = pixel;
	      *idata++ = (pixel >> 8);
	      *idata++ = (pixel >> 16);
	      *idata++ = (pixel >> 24);
	    }

	    break;
	  }

          break;

        default:
	  fprintf(stderr, "BImage::renderXImage: unsupported visual\n");
	  image->data = (char *) d;
	  XDestroyImage(image);
	  return 0; 
	}
      }
    }
  }

  image->data = (char *) d;
  return image;
}


Pixmap BImage::renderPixmap(Bool dither) {
  Pixmap pixmap = XCreatePixmap(control->d(), control->drawable(), width,
                                height, control->depth());
  if (pixmap == None) return None;

  XImage *image = renderXImage(dither);
  if ((! image) || (! image->data)) {
    XFreePixmap(control->d(), pixmap);
    return None;
  }

  XPutImage(control->d(), pixmap,
	    DefaultGC(control->d(), control->screen()),
            image, 0, 0, 0, 0, width, height);

  XDestroyImage(image);
  return pixmap;
}


void BImage::background(const BColor &c) {
  unsigned int i, wh = width * height;

  bg = c;
  
  for (i = 0; i < wh; i++) {
    *(red + i) = c.red;
    *(green + i) = c.green;
    *(blue + i) = c.blue;
  }
}


void BImage::bevel1(Bool solid, Bool solidblack) {
  if (width > 2 && height > 2) {
    unsigned char r, g, b, rr ,gg ,bb, *pr = red, *pg = green, *pb = blue;
    unsigned int w = width, h = height - 1, wh = w * h;
    
    if (solid) {
      if (solidblack) {
	r = g = b = 0xc0;
	rr = gg = bb = 0x60;
      } else {
	r = bg.red * 3 / 2;
	if (r < bg.red) r = ~0;
	g = bg.green * 3 / 2;
	if (g < bg.green) g = ~0;
	b = bg.blue * 3 / 2;
	if (b < bg.blue) b = ~0;
	
	rr = bg.red * 3 / 4;
	if (rr > bg.red) rr = 0;
	gg = bg.green * 3 / 4;
	if (gg > bg.green) gg = 0;
	bb = bg.blue * 3 / 4;
	if (bb > bg.blue) bb = 0;
      }      

      while (--w) {
	*pr = r;
	*pg = g;
	*pb = b;
	*((pr++) + wh) = rr;
	*((pg++) + wh) = gg;
	*((pb++) + wh) = bb;
      }

      *pr = r;
      *pg = g;
      *pb = b;
      *(pr + wh) = rr;
      *(pg + wh) = gg;
      *(pb + wh) = bb;
      
      pr = red + width;
      pg = green + width;
      pb = blue + width;
      
      while (--h) {
	*pr = r;
	*pg = g;
	*pb = b;
	
	pr += width - 1;
	pg += width - 1;
	pb += width - 1;
	
	*(pr++) = rr;
	*(pg++) = gg;
	*(pb++) = bb;
      }
      
      *pr = r;
      *pg = g;
      *pb = b;
      
      pr += width - 1;
      pg += width - 1;
      pb += width - 1;
      
      *pr = rr;
      *pg = gg;
      *pb = bb;
    } else {
      while (--w) {
	r = *pr;
	rr = r * 3 / 2;
	if (rr < r) rr = ~0;
	g = *pg;
	gg = g * 3 / 2;
	if (gg < g) gg = ~0;
	b = *pb;
	bb = b * 3 / 2;
	if (bb < b) bb = ~0;

	*pr = rr;
	*pg = gg;
	*pb = bb;

	r = *(pr + wh);
	rr = r * 3 / 4;
	if (rr > r) rr = 0;
	g = *(pg + wh);
	gg = g * 3 / 4;
	if (gg > g) gg = 0;
	b = *(pb + wh);
	bb = b * 3 / 4;
	if (bb > b) bb = 0;

	*((pr++) + wh) = rr;
	*((pg++) + wh) = gg;
	*((pb++) + wh) = bb;
      }

      r = *pr;
      rr = r * 3 / 2;
      if (rr < r) r = ~0;
      g = *pg;
      gg = g * 3 / 2;
      if (gg < g) g = ~0;
      b = *pb;
      bb = b * 3 / 2;
      if (bb < b) b = ~0;

      *pr = rr;
      *pg = gg;
      *pb = bb;

      r = *(pr + wh);
      rr = r * 3 / 4;
      if (rr > r) rr = 0;
      g = *(pg + wh);
      gg = g * 3 / 4;
      if (gg > g) gg = 0;
      b = *(pb + wh);
      bb = b * 3 / 4;
      if (bb > b) bb = 0;
      
      *(pr + wh) = rr;
      *(pg + wh) = gg;
      *(pb + wh) = bb;
      
      pr = red + width;
      pg = green + width;
      pb = blue + width;
      
      while (--h) {
	r = *pr;
	rr = r * 3 / 2;
	if (rr < r) rr = ~0;
	g = *pg;
	gg = g * 3 / 2;
	if (gg < g) gg = ~0;
	b = *pb;
	bb = b * 3 / 2;
	if (bb < b) bb = ~0;
	
	*pr = rr;
	*pg = gg;
	*pb = bb;
	
	pr += width - 1;
	pg += width - 1;
	pb += width - 1;
	
	r = *pr;
	rr = r * 3 / 4;
	if (rr > r) rr = 0;
	g = *pg;
	gg = g * 3 / 4;
	if (gg > g) gg = 0;
	b = *pb;
	bb = b * 3 / 4;
	if (bb > b) bb = 0;

	*(pr++) = rr;
	*(pg++) = gg;
	*(pb++) = bb;
      }
      
      r = *pr;
      rr = r * 3 / 2;
      if (rr < r) rr = ~0;
      g = *pg;
      gg = g * 3 / 2;
      if (gg < g) gg = ~0;
      b = *pb;
      bb = b * 3 / 2;
      if (bb < b) bb = ~0;

      *pr = rr;
      *pg = gg;
      *pb = bb;
      
      pr += width - 1;
      pg += width - 1;
      pb += width - 1;
      
      r = *pr;
      rr = r * 3 / 4;
      if (rr > r) rr = 0;
      g = *pg;
      gg = g * 3 / 4;
      if (gg > g) gg = 0;
      b = *pb;
      bb = b * 3 / 4;
      if (bb > b) bb = 0;
      
      *pr = rr;
      *pg = gg;
      *pb = bb;
    }
  }
}


void BImage::bevel2(Bool solid, Bool solidblack) {
  if (width > 4 && height > 4) {
    unsigned char r, g, b, rr ,gg ,bb, *pr = red + width + 1,
      *pg = green + width + 1, *pb = blue + width + 1;
    unsigned int w = width - 2, h = height - 1, wh = width * (height - 3);
    
    if (solid) {
      if (solidblack) {
	r = g = b = 0xc0;
	rr = gg = bb = 0x60;
      } else {
	r = bg.red * 3 / 2;
	if (r < bg.red) r = ~0;
	g = bg.green * 3 / 2;
	if (g < bg.green) g = ~0;
	b = bg.blue * 3 / 2;
	if (b < bg.blue) b = ~0;
	
	rr = bg.red * 3 / 4;
	if (rr > bg.red) rr = 0;
	gg = bg.green * 3 / 4;
	if (gg > bg.green) gg = 0;
	bb = bg.blue * 3 / 4;
	if (bb > bg.blue) bb = 0;
      }      

      while (--w) {
	*pr = r;
	*pg = g;
	*pb = b;
	*((pr++) + wh) = rr;
	*((pg++) + wh) = gg;
	*((pb++) + wh) = bb;
      }
      
      pr = red + width;
      pg = green + width;
      pb = blue + width;
      
      while (--h) {
	*(++pr) = r;
	*(++pg) = g;
	*(++pb) = b;
	
	pr += width - 3;
	pg += width - 3;
	pb += width - 3;
	
	*(pr++) = rr;
	*(pg++) = gg;
	*(pb++) = bb;

        pr++; pg++; pb++;
      }
    } else {
      while (--w) {
	r = *pr;
	rr = r * 3 / 2;
	if (rr < r) rr = ~0;
	g = *pg;
	gg = g * 3 / 2;
	if (gg < g) gg = ~0;
	b = *pb;
	bb = b * 3 / 2;
	if (bb < b) bb = ~0;

	*pr = rr;
	*pg = gg;
	*pb = bb;

	r = *(pr + wh);
	rr = r * 3 / 4;
	if (rr > r) rr = 0;
	g = *(pg + wh);
	gg = g * 3 / 4;
	if (gg > g) gg = 0;
	b = *(pb + wh);
	bb = b * 3 / 4;
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
	rr = r * 3 / 2;
	if (rr < r) rr = ~0;
	g = *pg;
	gg = g * 3 / 2;
	if (gg < g) gg = ~0;
	b = *pb;
	bb = b * 3 / 2;
	if (bb < b) bb = ~0;
	
	*(++pr) = rr;
	*(++pg) = gg;
	*(++pb) = bb;
	
	pr += width - 3;
	pg += width - 3;
	pb += width - 3;
	
	r = *pr;
	rr = r * 3 / 4;
	if (rr > r) rr = 0;
	g = *pg;
	gg = g * 3 / 4;
	if (gg > g) gg = 0;
	b = *pb;
	bb = b * 3 / 4;
	if (bb > b) bb = 0;

	*(pr++) = rr;
	*(pg++) = gg;
	*(pb++) = bb;

        pr++; pg++; pb++;
      }
    }
  }
}


void BImage::invert(void) {
  unsigned int wh = width * height;
  unsigned char *r, *g, *b, *rr, *gg, *bb,
    *new_r = new unsigned char[wh],
    *new_g = new unsigned char[wh],
    *new_b = new unsigned char[wh];

  if ((! new_r) || (! new_g) || (! new_b)) return;

  rr = new_r;
  gg = new_g;
  bb = new_b;

  r = red + wh - 1;
  g = green + wh - 1;
  b = blue +wh - 1;

  while (--wh) {
    *(rr++) = *(r--);
    *(gg++) = *(g--);
    *(bb++) = *(b--);
  }

  *rr = *r;
  *gg = *g;
  *bb = *b;
  
  delete [] red;
  delete [] green;
  delete [] blue;

  red = new_r;
  green = new_g;
  blue = new_b;
}


void BImage::dgradient(void) {
  float fr, fg, fb, tr, tg, tb, dr, dg, db,
    w = (float) width, h = (float) height,
    dx, dy, xr, xg, xb, yr, yg, yb;
  
  unsigned int x, y, off;
  
  fr = (float) from.red;
  fg = (float) from.green;
  fb = (float) from.blue;
  
  tr = (float) to.red;
  tg = (float) to.green;
  tb = (float) to.blue;
  
  dr = tr - fr;
  dg = tg - fg;
  db = tb - fb;

  for (y = 0, off = 0; y < height; y++) {
#ifdef GradientHack
    dy = sin((y / h) * M_PI_2);
#else
    dy = y / h;
#endif
    
    yr = (dr * dy) + fr;
    yg = (dg * dy) + fg;
    yb = (db * dy) + fb;

    for (x = 0; x < width; x++, off++) {
#ifdef GradientHack
      dx = cos((x / w) * M_PI_2);
#else
      dx = x / w;
#endif
      
      xr = (dr * dx) + fr;
      xg = (dg * dx) + fg;
      xb = (db * dx) + fb;
      
      *(red + off) = (unsigned char) ((xr + yr) / 2);
      *(green + off) = (unsigned char) ((xg + yg) / 2);
      *(blue + off) = (unsigned char) ((xb + yb) / 2);
    }
  }
}


void BImage::hgradient(void) {
  float fr, fg, fb, tr, tg, tb, dr, dg, db, dx, xr, xg, xb, w = (float) width;
  unsigned char *r = new unsigned char[width],
    *g = new unsigned char [width],
    *b = new unsigned char [width];

  unsigned int x, y, off;

  if ((! r) || (! g) || (! b)) return;
  
  fr = (float) from.red;
  fg = (float) from.green;
  fb = (float) from.blue;

  tr = (float) to.red;
  tg = (float) to.green;
  tb = (float) to.blue;

  dr = tr - fr;
  dg = tg - fg;
  db = tb - fb;


  // this renders one line of the hgradient to a buffer...
  for (x = 0; x < width; x++) {
#ifdef GradientHack
    dx = cos((x / w) * M_PI_2);
#else
    dx = x / w;
#endif
    
    xr = (dr * dx) + fr;
    xg = (dg * dx) + fg;
    xb = (db * dx) + fb;
      
    *(r + x) = (unsigned char) (xr);
    *(g + x) = (unsigned char) (xg);
    *(b + x) = (unsigned char) (xb);
  }

  // and this copies the buffer to the image...
  for (y = 0, off = 0; y < height; y++)
    for (x = 0; x < width; x++, off++) {
      *(red + off) = *(r + x);
      *(green + off) = *(g + x);
      *(blue + off ) = *(b + x);
    }
  

  delete [] r;
  delete [] g;
  delete [] b;
}


void BImage::vgradient(void) {
  float fr, fg, fb, tr, tg, tb, dr, dg, db, dy, yr, yg, yb,
    h = (float) height;
  unsigned char r, g, b;
  unsigned int x, y, off;
  
  fr = (float) from.red;
  fg = (float) from.green;
  fb = (float) from.blue;

  tr = (float) to.red;
  tg = (float) to.green;
  tb = (float) to.blue;

  dr = tr - fr;
  dg = tg - fg;
  db = tb - fb;

  for (y = 0, off = 0; y < height; y++) {
#ifdef GradientHack
    dy = sin((y/ h) * M_PI_2);
#else
    dy = y / h;
#endif
    
    yr = (dr * dy) + fr;
    yg = (dg * dy) + fg;
    yb = (db * dy) + fb;
      
    r = (unsigned char) (yr);
    g = (unsigned char) (yg);
    b = (unsigned char) (yb);
    
    for (x = 0; x < width; x++, off++) {
      *(red + off) = r;
      *(green + off) = g;
      *(blue + off) = b;
    }
  }
}


// *************************************************************************
// Image control class code
// *************************************************************************

BImageControl::BImageControl(Blackbox *bb) {
  blackbox = bb;
  display = blackbox->control();
  screen_depth = blackbox->Depth();
  window = blackbox->Root();
  screen_number = blackbox->Screen();

  colors = 0;
  colors_per_channel = ncolors = 0;
  
  int count;
  XPixmapFormatValues *pmv = XListPixmapFormats(display, &count);
  
  bits_per_pixel = 0;
  for (int i = 0; i < count; i++)
    if (pmv[i].depth == screen_depth) {
      bits_per_pixel = pmv[i].bits_per_pixel;
      break;
    }
  
  if (bits_per_pixel == 0) bits_per_pixel = screen_depth;
  XFree(pmv);

  // calculate the rgb offsets for rendering images in TrueColor
  red_offset = green_offset = blue_offset = 0;

  switch (blackbox->visual()->c_class) {
  case TrueColor:
    {
      int i;
      unsigned long mask = blackbox->visual()->red_mask;
      while (! (mask & 1)) { red_offset++; mask >>= 1; }
      for (i = 0; i < 256; i++)
	rmask_table[i] = (i * mask + 0x7f) / 0xff;
      
      mask = blackbox->visual()->green_mask;
      while (! (mask & 1)) { green_offset++; mask >>= 1; }
      for (i = 0; i < 256; i++)
	gmask_table[i] = (i * mask + 0x7f) / 0xff;
      
      mask = blackbox->visual()->blue_mask;
      while (! (mask & 1)) { blue_offset++; mask >>= 1; }
      for (i = 0; i < 256; i++)
	bmask_table[i] = (i * mask + 0x7f) / 0xff;
    }
    
    break;
    
  case PseudoColor:
  case StaticColor:
    {
      colors_per_channel = blackbox->colorsPerChannel();
      ncolors = colors_per_channel * colors_per_channel * colors_per_channel;
      
      if (ncolors > (1 << screen_depth)) {
	colors_per_channel = (1 << screen_depth) / 3;
	ncolors = colors_per_channel * colors_per_channel * colors_per_channel;
      }
      
      if (colors_per_channel < 2 || ncolors > (1 << screen_depth)) {
	fprintf(stderr,
		"BImageControl::BImageControl: invalid colormap size %d "
		"(%d/%d/%d)", ncolors, colors_per_channel, colors_per_channel,
		colors_per_channel);
	exit(1);
      }
      
      colors = new XColor[ncolors];
      if (! colors) {
	fprintf(stderr, "BImageControl::BImageControl: error allocating "
		"colormap\n");
	exit(1);
      }

      int i = 0, ii, p, r, g, b;
      unsigned long mask = colors_per_channel - 1;
      for (i = 0; i < 256; i++) {
	rmask_table[i] = gmask_table[i] = bmask_table[i] =
	  (i * mask + 0x7f) / 0xff;	
      }
      
      for (int r = 0, i = 0; r < colors_per_channel; r++)
	for (int g = 0; g < colors_per_channel; g++)
	  for (int b = 0; b < colors_per_channel; b++, i++) {
	    colors[i].red = (r * 0xffff) / (colors_per_channel - 1);
	    colors[i].green = (g * 0xffff) / (colors_per_channel - 1);
	    colors[i].blue = (b * 0xffff) / (colors_per_channel - 1);;
	    colors[i].flags = DoRed|DoGreen|DoBlue;
	  }
      
      XGrabServer(display);
      
      Colormap colormap = DefaultColormap(display, screen_number);
      for (i = 0; i < ncolors; i++)
	if (! XAllocColor(display, colormap, &colors[i])) {
	  fprintf(stderr, "couldn't alloc color %i %i %i\n", colors[i].red,
		  colors[i].green, colors[i].blue);
	  colors[i].flags = 0;
	} else
	  colors[i].flags = DoRed|DoGreen|DoBlue;
      
      XUngrabServer(display);
      
      XColor icolors[256];
      int incolors = (((1 << screen_depth) > 256) ? 256 : (1 << screen_depth));
      
      for (i = 0; i < incolors; i++)
	icolors[i].pixel = i;
      
      XQueryColors(display, colormap, icolors, incolors);
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
	      
	      if (XAllocColor(display, colormap, &colors[i])) {
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
  case StaticGray:
    fprintf(stderr, "BImageControl::BImageControl: GrayScale/StaticGray "
                    "visual unsupported\n");
    break;
    
  default:
    fprintf(stderr, "BImageControl::BImageControl: unsupported visual %d\n",
	    blackbox->visual()->c_class);
    exit(1);
  }
  
  cache = new LinkedList<Cache>;
  
  dither_buf_width = 0;
  red_err = green_err = blue_err = next_red_err = next_green_err =
    next_blue_err = 0;
}


BImageControl::~BImageControl(void) {
  if (red_err) delete [] red_err;
  if (green_err) delete [] green_err;
  if (blue_err) delete [] blue_err;
  if (next_red_err) delete [] next_red_err;
  if (next_green_err) delete [] next_green_err;
  if (next_blue_err) delete [] next_blue_err;

  if (colors) {
    unsigned long *pixels = new unsigned long [ncolors];

    int i;
    for (i = 0; i < ncolors; i++)
      *(pixels + i) = (*(colors + i)).pixel;

    XFreeColors(display, DefaultColormap(display, screen_number),
		pixels, ncolors, 0);

    delete [] colors;
  }

  if (cache->count()) {
    int i, n = cache->count();
    fprintf(stderr, "BImageContol::~BImageControl: pixmap cache - %d "
                    "unreleased pixmaps\n", n);

    for (i = 0; i < n; i++) {
      Cache *tmp = cache->first();
      XFreePixmap(display, tmp->pixmap);
      cache->remove(tmp);
      delete tmp;
    }
  }

  delete cache;
}


Bool BImageControl::dither(void) {
  return blackbox->imageDither();
}


Visual *BImageControl::v(void) {
  return blackbox->visual();
}


// *************************************************************************
// pixmap cache control
// *************************************************************************

Pixmap BImageControl::searchCache(unsigned int width, unsigned int height,
				  unsigned long texture, const BColor &c1,
				  const BColor &c2) {
  if (cache->count()) {
    LinkedListIterator<Cache> it(cache);

    for (; it.current(); it++) {
      if ((it.current()->width == width) &&
	  (it.current()->height == height) &&
	  (it.current()->texture == texture) &&
	  (it.current()->pixel1 == c1.pixel))
	if (texture & BImage_Gradient) {
	  if (it.current()->pixel2 == c2.pixel) {
	    it.current()->count++;
	    return it.current()->pixmap;
	  }
	} else {
	  it.current()->count++;
	  return it.current()->pixmap;
	}
    }
  }

  return None;
}

Pixmap BImageControl::renderImage(unsigned int width, unsigned int height,
			     unsigned long texture, const BColor &c1,
			     const BColor &c2) {
  Pixmap pixmap = searchCache(width, height, texture, c1, c2);
  if (pixmap) return pixmap;

  BImage image(this, width, height);
  pixmap = image.render(texture, c1, c2);
  
  if (pixmap) {
    Cache *tmp = new Cache;

    tmp->pixmap = pixmap;
    tmp->width = width;
    tmp->height = height;
    tmp->count = 1;
    tmp->texture = texture;
    tmp->pixel1 = c1.pixel;

    if (texture & BImage_Gradient)
      tmp->pixel2 = c2.pixel;
    else
      tmp->pixel2 = 0l;

    cache->insert(tmp);

    return pixmap;
  }
  
  return None;
}


void BImageControl::removeImage(Pixmap pixmap) {
  if (pixmap) {
    LinkedListIterator<Cache> it(cache);

    for (; it.current(); it++) {
      if (it.current()->pixmap == pixmap) {
	Cache *tmp = it.current();
	tmp->count--;
	  
	if (! tmp->count) {	  
	  XFreePixmap(display, tmp->pixmap);
	  cache->remove(tmp);
	  delete tmp;
	}
	
	return;
      }
    }
  }
}


unsigned long BImageControl::getColor(const char *colorname,
				      unsigned char *r, unsigned char *g,
				      unsigned char *b)
{
  XColor color;
  XWindowAttributes attributes;
  
  XGetWindowAttributes(display, window, &attributes);
  color.pixel = 0;
  if (!XParseColor(display, attributes.colormap, colorname, &color)) {
    fprintf(stderr, "BImageControl::getColor: color parse error: \"%s\"\n",
	    colorname);
  } else if (!XAllocColor(display, attributes.colormap, &color)) {
    fprintf(stderr, "BImageControl::getColor: color alloc error: \"%s\"\n",
	    colorname);
  }
  
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
  XWindowAttributes attributes;
  
  XGetWindowAttributes(display, window, &attributes);
  color.pixel = 0;
  if (!XParseColor(display, attributes.colormap, colorname, &color)) {
    fprintf(stderr, "BImageControl::getColor: color parse error: \"%s\"\n",
	    colorname);
  } else if (!XAllocColor(display, attributes.colormap, &color)) {
    fprintf(stderr, "BImageControl::getColor: color alloc error: \"%s\"\n",
	    colorname);
  }

  return color.pixel;
}


void BImageControl::getMaskTables(unsigned short **rmt, unsigned short **gmt,
				  unsigned short **bmt,
				  int *roff, int *goff, int *boff) {
  if (rmt) *rmt = rmask_table;
  if (gmt) *gmt = gmask_table;
  if (bmt) *bmt = bmask_table;

  if (roff) *roff = red_offset;
  if (goff) *goff = green_offset;
  if (boff) *boff = blue_offset;
}


void BImageControl::getColorTable(XColor **c, int *n) {
  if (c) *c = colors;
  if (n) *n = ncolors;
}


void BImageControl::getDitherBuffers(unsigned int w, short **r, short **g,
				     short **b, short **nr, short **ng,
				     short **nb) {
  if (w > dither_buf_width) {
    if (red_err) delete [] red_err;
    if (green_err) delete [] green_err;
    if (blue_err) delete [] blue_err;
    if (next_red_err) delete [] next_red_err;
    if (next_green_err) delete [] next_green_err;
    if (next_blue_err) delete [] next_blue_err;

    dither_buf_width = w;
    
    red_err = new short[dither_buf_width];
    green_err = new short[dither_buf_width];
    blue_err = new short[dither_buf_width];
    next_red_err = new short[dither_buf_width];
    next_green_err = new short[dither_buf_width];
    next_blue_err = new short[dither_buf_width];
  }

  *r = red_err;
  *g = green_err;
  *b = blue_err;
  *nr = next_red_err;
  *ng = next_green_err;
  *nb = next_blue_err;
}
