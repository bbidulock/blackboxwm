//
// Image.cc for Blackbox - an X11 Window manager
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

#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "blackbox.hh"
#include "Image.hh"
#include "Screen.hh"

#ifdef    HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif // HAVE_SYS_TYPES_H

#ifndef u_int32_t
#  ifdef uint_32_t
typedef uint32_t u_int32_t;
#  else
#    ifdef __uint32_t
typedef __uint32_t u_int32_t;
#    else
typedef unsigned int u_int32_t;
#    endif
#  endif
#endif

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    HAVE_MALLOC_H
#  include <malloc.h>
#endif // HAVE_MALLOC_H


BImage::BImage(BImageControl *c, unsigned int w, unsigned int h) {
  control = c;

  width = ((signed) w > 0) ? w : 1;
  height = ((signed) h > 0) ? h : 1;

  red = new unsigned char[width * height];
  green = new unsigned char[width * height];
  blue = new unsigned char[width * height];

  cpc = control->getColorsPerChannel();
  cpccpc = cpc * cpc;
  
  control->getColorTables(&red_table, &green_table, &blue_table,
                          &red_offset, &green_offset, &blue_offset);
  
  if (control->getVisual()->c_class != TrueColor)
    control->getXColorTable(&colors, &ncolors);
}


BImage::~BImage(void) {
  if (red) delete [] red;
  if (green) delete [] green;
  if (blue) delete [] blue;
}


Pixmap BImage::render(BTexture *texture) {
  if (texture->getTexture() & BImage_Solid) {
    return render_solid(texture);
  } else if (texture->getTexture() & BImage_Gradient)
    return render_gradient(texture);
  
  return None;
}


Pixmap BImage::render_solid(BTexture *texture) {
  Pixmap pixmap = XCreatePixmap(control->getDisplay(),
				control->getDrawable(), width,
				height, control->getDepth());
  if (pixmap == None) {
    fprintf(stderr, "BImage::render_solid: error creating pixmap\n");
    return None;
  }
  
  XGCValues gcv;
  GC gc, hgc, lgc;
  
  gcv.foreground = texture->getColor()->getPixel();
  gcv.fill_style = FillSolid;
  gc = XCreateGC(control->getDisplay(), pixmap, GCForeground | GCFillStyle,
		 &gcv);
  
  gcv.foreground = texture->getHiColor()->getPixel();
  hgc = XCreateGC(control->getDisplay(), pixmap, GCForeground, &gcv);
  
  gcv.foreground = texture->getLoColor()->getPixel();
  lgc = XCreateGC(control->getDisplay(), pixmap, GCForeground, &gcv);
  
  XFillRectangle(control->getDisplay(), pixmap, gc, 0, 0, width, height);
  
  if (texture->getTexture() & BImage_Bevel1) {
    if (texture->getTexture() & BImage_Raised) {
      XDrawLine(control->getDisplay(), pixmap, lgc, 0, height - 1,
		width - 1, height - 1);
      XDrawLine(control->getDisplay(), pixmap, lgc, width - 1, height - 1,
		width - 1, 0);
      
      XDrawLine(control->getDisplay(), pixmap, hgc, 0, 0,
		width - 1, 0);
      XDrawLine(control->getDisplay(), pixmap, hgc, 0, height - 1,
		0, 0);
    } else if (texture->getTexture() & BImage_Sunken) {
      XDrawLine(control->getDisplay(), pixmap, hgc, 0, height - 1,
		width - 1, height - 1);
      XDrawLine(control->getDisplay(), pixmap, hgc, width - 1, height - 1,
		width - 1, 0);
      
      XDrawLine(control->getDisplay(), pixmap, lgc, 0, 0, width - 1, 0);
      XDrawLine(control->getDisplay(), pixmap, lgc, 0, height - 1, 0, 0);
    }
  } else if (texture->getTexture() & BImage_Bevel2) {
    if (texture->getTexture() & BImage_Raised) {
      XDrawLine(control->getDisplay(), pixmap, lgc, 1, height - 3,
		width - 3, height - 3);
      XDrawLine(control->getDisplay(), pixmap, lgc, width - 3, height - 3,
		width - 3, 1);
      
      XDrawLine(control->getDisplay(), pixmap, hgc, 1, 1,
		width - 3, 1);
      XDrawLine(control->getDisplay(), pixmap, hgc, 1, height - 3,
		1, 1);
    } else if (texture->getTexture() & BImage_Sunken) {
      XDrawLine(control->getDisplay(), pixmap, hgc, 1, height - 3,
		width - 3, height - 3);
      XDrawLine(control->getDisplay(), pixmap, hgc, width - 3, height - 3,
		width - 3, 1);
      
      XDrawLine(control->getDisplay(), pixmap, lgc, 1, 1, width - 3, 1);
      XDrawLine(control->getDisplay(), pixmap, lgc, 1, height - 3, 1, 1);
    }
  }
  
  XFreeGC(control->getDisplay(), gc);
  XFreeGC(control->getDisplay(), hgc);
  XFreeGC(control->getDisplay(), lgc);
  
  return pixmap;
}


Pixmap BImage::render_gradient(BTexture *texture) {
  int inverted = 0;
 
#ifdef    INTERLACE
  interlaced = texture->getTexture() & BImage_Interlaced;
#endif // INTERLACE
  
  if (texture->getTexture() & BImage_Sunken) {
    from = texture->getColorTo();
    to = texture->getColor();

    if (! (texture->getTexture() & BImage_Invert)) inverted = 1;
  } else {
    from = texture->getColor();
    to = texture->getColorTo();
    
    if (texture->getTexture() & BImage_Invert) inverted = 1;
  }
  
  if (texture->getTexture() & BImage_Diagonal) dgradient();
  else if (texture->getTexture() & BImage_Horizontal) hgradient();
  else if (texture->getTexture() & BImage_Vertical) vgradient();

  if (texture->getTexture() & BImage_Bevel1) bevel1();
  else if (texture->getTexture() & BImage_Bevel2) bevel2();
  
  if (inverted) invert();
  
  Pixmap pixmap = renderPixmap();

  return pixmap;

}


XImage *BImage::renderXImage(void) {
  XImage *image = XCreateImage(control->getDisplay(), control->getVisual(),
			       control->getDepth(), ZPixmap,
			       0, 0, width, height, 32, 0);
  if (! image) {
    fprintf(stderr, "BImage::renderXImage: error creating XImage\n");
    return (XImage *) 0;
  }

  // insurance policy
  image->data = (char *) 0;

  unsigned char *d = new unsigned char[image->bytes_per_line * (height + 1)];
  if (! d) {
    fprintf(stderr,
	    "BImage::renderXImage: error allocating memory for image\n");
    XDestroyImage(image);
    return (XImage *) 0;
  }
  
  register unsigned int x, y, r = 0, g = 0, b = 0, i, offset;
 
  unsigned char *pixel_data = d, *ppixel_data = d, *pred, *pgreen, *pblue;
  unsigned long pixel;

  if ((! red_table) || (! green_table) || (! blue_table)) {
    fprintf(stderr, "BImage::renderXImage: error getting color mask tables\n");
    delete [] d;
    XDestroyImage(image);
    return (XImage *) 0;
  }
  
  if (control->getVisual()->c_class != TrueColor)
    if ((! colors) || (! ncolors)) {
      fprintf(stderr,
	      "BImage::renderXImage: error getting pseudo color tables\n");
      delete [] d;
      XDestroyImage(image);
      return (XImage *) 0;
    }

  if (control->doDither() && width > 1 && height > 1) {
    unsigned char *red_error_table, *green_error_table, *blue_error_table,
      *red_error_4_table, *green_error_4_table, *blue_error_4_table;
    short *red_buffer, *green_buffer, *blue_buffer,
      *nred_buffer, *ngreen_buffer, *nblue_buffer,
      *pred_buffer, *pgreen_buffer, *pblue_buffer;
 
    control->
      getDitherBuffers(width + 2,
                       &red_buffer, &green_buffer, &blue_buffer,
                       &nred_buffer, &ngreen_buffer, &nblue_buffer,
		       &red_error_table, &green_error_table, &blue_error_table,
		       &red_error_4_table,
		       &green_error_4_table,
		       &blue_error_4_table);
    
    if ((! red_buffer) || (! green_buffer) || (! blue_buffer) ||
        (! nred_buffer) || (! ngreen_buffer) || (! nblue_buffer) ||
	(! red_error_table) || (! green_error_table) || (! blue_error_table) ||
        (! red_error_4_table) ||
	(! green_error_4_table) ||
        (! blue_error_4_table)) {
      fprintf(stderr,
	      "BImage::renderXImage: error getting dither information\n");
      delete [] d;
      XDestroyImage(image);
      return (XImage *) 0;
    }
    
    char pbytes = image->bits_per_pixel >> 3;
    u_int32_t *pixel_data_32 = (u_int32_t *) pixel_data;
    
    x = width;
 
    pred_buffer = red_buffer;
    pgreen_buffer = green_buffer;
    pblue_buffer = blue_buffer;

    pred = red;
    pgreen = green;
    pblue = blue;

    while (x--) {
      *(pred_buffer++) = *(pred++);
      *(pgreen_buffer++) = *(pgreen++);
      *(pblue_buffer++) = *(pblue++);
    }
    
    for (y = 0, offset = 0; y < height; y++) {
      if (y < (height - 1)) {
	for (x = 0, i = offset + width; x < width; x++, i++) {
	  *(nred_buffer + x) = *(red + i);
	  *(ngreen_buffer + x) = *(green + i);
	  *(nblue_buffer + x) = *(blue + i);
	}
	
	i--;
	*(nred_buffer + x) = *(red + i);
	*(ngreen_buffer + x) = *(green + i);
	*(nblue_buffer + x) = *(blue + i);
      }
      
      for (x = 0; x < width; x++) {
	if (*(red_buffer + x) > 255) *(red_buffer + x) = 255;
	else if (*(red_buffer + x) < 0) *(red_buffer + x) = 0;
	if (*(green_buffer + x) > 255) *(green_buffer + x) = 255;
	else if (*(green_buffer + x) < 0) *(green_buffer + x) = 0;
	if (*(blue_buffer + x) > 255) *(blue_buffer + x) = 255;
	else if (*(blue_buffer + x) < 0) *(blue_buffer + x) = 0;
	
	r = *(red_table + *(red_buffer + x));
	g = *(green_table + *(green_buffer + x));
	b = *(blue_table + *(blue_buffer + x));
	
	switch (control->getVisual()->c_class) {
	case StaticColor:
	case PseudoColor:
	  pixel = (r * cpccpc) + (g * cpc) + b;
	  *pixel_data++ = colors[pixel].pixel;
	  
	  break;
	  
	case TrueColor:
	  pixel = (r << red_offset) | (g << green_offset) | (b << blue_offset);

          pixel_data_32 = (u_int32_t *) pixel_data;
          *pixel_data_32 = pixel;
          pixel_data += pbytes;
	  
	  break;
	  
	default:
	  fprintf(stderr, "BImage::renderXImage: unsupported visual\n");
	  delete [] d;
	  XDestroyImage(image);
	  return (XImage *) 0;
	}

	*(red_buffer + x + 1) += *(red_error_table + *(red_buffer + x));
	*(green_buffer + x + 1) += *(green_error_table + *(green_buffer + x));
	*(blue_buffer + x + 1) += *(blue_error_table + *(blue_buffer + x));
	
        *(nred_buffer + x) += *(red_error_table + *(red_buffer + x));
        *(ngreen_buffer + x) += *(green_error_table + *(green_buffer + x));
        *(nblue_buffer + x) += *(blue_error_table + *(blue_buffer + x));

	*(nred_buffer + x + 1) -=
          *(red_error_4_table + *(red_buffer + x));
	*(ngreen_buffer + x + 1) -=
          *(green_error_4_table + *(green_buffer + x));
	*(nblue_buffer + x + 1) -=
          *(blue_error_4_table + *(blue_buffer + x));
      }
      
      offset += image->width;
      pixel_data = (ppixel_data += image->bytes_per_line);
      
      pred_buffer = red_buffer;
      red_buffer = nred_buffer;
      nred_buffer = pred_buffer;

      pgreen_buffer = green_buffer;
      green_buffer = ngreen_buffer;
      ngreen_buffer = pgreen_buffer;

      pblue_buffer = blue_buffer;
      blue_buffer = nblue_buffer;
      nblue_buffer = pblue_buffer;
    }
  } else {
    u_int32_t *pixel_data_32 = (u_int32_t *) pixel_data;
    char pbytes = image->bits_per_pixel >> 3;

    for (y = 0, offset = 0; y < height; y++) {
      for (x = 0; x < width; x++, offset++) {
	r = *(red_table + *(red + offset));
	g = *(green_table + *(green + offset));
	b = *(blue_table + *(blue + offset));
	
	switch (control->getVisual()->c_class) {
	case StaticColor:
	case PseudoColor:
	  pixel = (r * cpccpc) + (g * cpc) + b;
	  *pixel_data++ = colors[pixel].pixel;
	  break;
	  
	case TrueColor:
	  pixel = (r << red_offset) | (g << green_offset) | (b << blue_offset);

          pixel_data_32 = (u_int32_t *) pixel_data;
          *pixel_data_32 = pixel;
          pixel_data += pbytes;

	  break;

	default:
	  fprintf(stderr, "BImage::renderXImage: unsupported visual\n");
	  delete [] d;
	  XDestroyImage(image);
	  return (XImage *) 0; 
	}
      }

      pixel_data = (ppixel_data += image->bytes_per_line);
    }
  }
  
  image->data = (char *) d;
  return image;
}


Pixmap BImage::renderPixmap(void) {
  Pixmap pixmap = XCreatePixmap(control->getDisplay(),
				control->getDrawable(), width,
                                height, control->getDepth());
  if (pixmap == None) {
    fprintf(stderr, "BImage::renderPixmap: error creating pixmap\n");
    return None;
  }
 
  XImage *image = renderXImage();

  if (! image) {
    XFreePixmap(control->getDisplay(), pixmap);
    return None;
  } else if (! image->data) {
    XDestroyImage(image);
    XFreePixmap(control->getDisplay(), pixmap);
    return None;
  }
  
  XPutImage(control->getDisplay(), pixmap,
	    DefaultGC(control->getDisplay(),
		      control->getScreen()->getScreenNumber()),
            image, 0, 0, 0, 0, width, height);

  if (image->data) { delete [] image->data; image->data = NULL; }
  XDestroyImage(image);

  return pixmap;
}


void BImage::bevel1(void) {
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


void BImage::bevel2(void) {
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


void BImage::invert(void) {
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


void BImage::dgradient(void) {
  float yr = 0.0, yg = 0.0, yb = 0.0, drx, dgx, dbx, dry, dgy, dby,
    xr = (float) from->getRed(),
    xg = (float) from->getGreen(),
    xb = (float) from->getBlue();
  unsigned char *pr = red, *pg = green, *pb = blue,
    xtable[width][3], ytable[height][3];
  unsigned int w = width * 2, h = height * 2;
  
  register unsigned int x, y;

  dry = drx = (float) (to->getRed() - from->getRed());
  dgy = dgx = (float) (to->getGreen() - from->getGreen());
  dby = dbx = (float) (to->getBlue() - from->getBlue());

  // Create X table
  drx /= w;
  dgx /= w;
  dbx /= w;
  
  for (x = 0; x < width; x++) {   
    xtable[x][0] = (unsigned char) (xr);
    xtable[x][1] = (unsigned char) (xg);
    xtable[x][2] = (unsigned char) (xb);

    xr += drx;
    xg += dgx;
    xb += dbx;
  }
  
  // Create Y table
  dry /= h;
  dgy /= h;
  dby /= h;
  
  for (y = 0; y < height; y++) {   
    ytable[y][0] = ((unsigned char) yr);
    ytable[y][1] = ((unsigned char) yg);
    ytable[y][2] = ((unsigned char) yb);

    yr += dry;
    yg += dgy;
    yb += dby;
  }
  
  // Combine tables to create gradient

#ifdef    INTERLACE
  if (! interlaced) {
#endif // INTERLACE

    // normal dgradient
    for (y = 0; y < height; y++) {
      for (x = 0; x < width; x++) {
        *(pr++) = xtable[x][0] + ytable[y][0];
        *(pg++) = xtable[x][1] + ytable[y][1];
        *(pb++) = xtable[x][2] + ytable[y][2];
      }
    }

#ifdef    INTERLACE
  } else {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (y = 0; y < height; y++) {
      for (x = 0; x < width; x++) {
        if (y & 1) {
          channel = xtable[x][0] + ytable[y][0];
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = xtable[x][1] + ytable[y][1];
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = xtable[x][2] + ytable[y][2];
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        } else {
          channel = xtable[x][0] + ytable[y][0];
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = xtable[x][1] + ytable[y][1];
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = xtable[x][2] + ytable[y][2];
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
#endif // INTERLACE
  
}


void BImage::hgradient(void) {
  float drx, dgx, dbx,
    xr = (float) from->getRed(),
    xg = (float) from->getGreen(),
    xb = (float) from->getBlue();
  unsigned char *pr = red, *pg = green, *pb = blue;
  
  register unsigned int x, y;
  
  drx = (float) (to->getRed() - from->getRed());
  dgx = (float) (to->getGreen() - from->getGreen());
  dbx = (float) (to->getBlue() - from->getBlue());
  
  drx /= width;
  dgx /= width;
  dbx /= width;
  
#ifdef    INTERLACE
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
#endif // INTERLACE
    
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
    
#ifdef    INTERLACE
  }
#endif // INTERLACE
  
}


void BImage::vgradient(void) {
  float dry, dgy, dby,
    yr = (float) from->getRed(),
    yg = (float) from->getGreen(),
    yb = (float) from->getBlue();
  unsigned char *pr = red, *pg = green, *pb = blue;

  register unsigned int y;

  dry = (float) (to->getRed() - from->getRed());
  dgy = (float) (to->getGreen() - from->getGreen());
  dby = (float) (to->getBlue() - from->getBlue());
  
  dry /= height;
  dgy /= height;
  dby /= height;
 
#ifdef    INTERLACE
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
#endif // INTERLACE
    
    // normal vgradient
    for (y = 0; y < height; y++, pr += width, pg += width, pb += width) {
      memset(pr, (unsigned char) yr, width);
      memset(pg, (unsigned char) yg, width);
      memset(pb, (unsigned char) yb, width);
    
      yr += dry;
      yg += dgy;
      yb += dby;
    }

#ifdef    INTERLACE
  }
#endif // INTERLACE
  
}


BImageControl::BImageControl(Blackbox *bb, BScreen *scrn) {
  blackbox = bb;
  screen = scrn;
  display = screen->getDisplay();
  screen_depth = screen->getDepth();
  window = screen->getRootWindow();
  screen_number = screen->getScreenNumber();

  colors = (XColor *) 0;
  colors_per_channel = ncolors = 0;
  
  int count;
  XPixmapFormatValues *pmv = XListPixmapFormats(display, &count);
  root_colormap = DefaultColormap(display, screen_number);
  
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

  red_offset = green_offset = blue_offset = 0;

  switch (screen->getVisual()->c_class) {
  case TrueColor:
    {
      int i, red_bits, green_bits, blue_bits;

      // compute color tables
      unsigned long red_mask = screen->getVisual()->red_mask,
        green_mask = screen->getVisual()->green_mask,
        blue_mask = screen->getVisual()->blue_mask;

      while (! (red_mask & 1)) { red_offset++; red_mask >>= 1; }
      while (! (green_mask & 1)) { green_offset++; green_mask >>= 1; }
      while (! (blue_mask & 1)) { blue_offset++; blue_mask >>= 1; }

      red_bits = 255 / red_mask;
      green_bits = 255 / green_mask;
      blue_bits = 255 / blue_mask;

      for (i = 0; i < 256; i++) {
	red_color_table[i] = i / red_bits;
	red_error_table[i] = (i % red_bits) * 3 / 4;
	red_error38_table[i] = red_error_table[i] / 4;

        green_color_table[i] = i / green_bits;
        green_error_table[i] = (i % green_bits) * 3 / 4;
        green_error38_table[i] = green_error_table[i] / 4;

        blue_color_table[i] = i / blue_bits;
        blue_error_table[i] = (i % blue_bits) * 3 / 4;
        blue_error38_table[i] = blue_error_table[i] / 4;
      }
    }

    break;
    
  case PseudoColor:
  case StaticColor:
    {
      colors_per_channel = blackbox->getColorsPerChannel();
      ncolors = colors_per_channel * colors_per_channel * colors_per_channel;
      
      if (ncolors > (1 << screen_depth)) {
	colors_per_channel = (1 << screen_depth) / 3;
	ncolors = colors_per_channel * colors_per_channel * colors_per_channel;
      }
      
      if (colors_per_channel < 2 || ncolors > (1 << screen_depth)) {
	fprintf(stderr,
		"BImageControl::BImageControl: invalid colormap size %d "
		"(%d/%d/%d) - reducing",
                ncolors, colors_per_channel, colors_per_channel,
                colors_per_channel);

        colors_per_channel = (1 << screen_depth) / 3; 
      }
      
      colors = new XColor[ncolors];
      if (! colors) {
	fprintf(stderr, "BImageControl::BImageControl: error allocating "
		"colormap\n");
	exit(1);
      }

      int i = 0, ii, p, r, g, b, bits = 255 / (colors_per_channel - 1);
      for (i = 0; i < 256; i++) {
	red_color_table[i] = green_color_table[i] = blue_color_table[i] =
	  i / bits;
	red_error_table[i] = green_error_table[i] = blue_error_table[i] =
	  (i % bits) * 3 / 4;
        red_error38_table[i] = green_error38_table[i] = blue_error38_table[i] =
          red_error_table[i] / 4;
      }
      
      for (r = 0, i = 0; r < colors_per_channel; r++)
	for (g = 0; g < colors_per_channel; g++)
	  for (b = 0; b < colors_per_channel; b++, i++) {
	    colors[i].red = (r * 0xffff) / (colors_per_channel - 1);
	    colors[i].green = (g * 0xffff) / (colors_per_channel - 1);
	    colors[i].blue = (b * 0xffff) / (colors_per_channel - 1);;
	    colors[i].flags = DoRed|DoGreen|DoBlue;
	  }
      
      blackbox->grab();
      
      for (i = 0; i < ncolors; i++)
	if (! XAllocColor(display, root_colormap, &colors[i])) {
	  fprintf(stderr, "couldn't alloc color %i %i %i\n", colors[i].red,
		  colors[i].green, colors[i].blue);
	  colors[i].flags = 0;
	} else
	  colors[i].flags = DoRed|DoGreen|DoBlue;
      
      blackbox->ungrab();
      
      XColor icolors[256];
      int incolors = (((1 << screen_depth) > 256) ? 256 : (1 << screen_depth));
      
      for (i = 0; i < incolors; i++)
	icolors[i].pixel = i;
      
      XQueryColors(display, root_colormap, icolors, incolors);
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
	      
	      if (XAllocColor(display, root_colormap, &colors[i])) {
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
	    screen->getVisual()->c_class);
    exit(1);
  }
  
  cache = new LinkedList<Cache>;
  
  dither_buffer_width = 0;
  red_buffer = green_buffer = blue_buffer =
    next_red_buffer = next_green_buffer = next_blue_buffer = (short *) 0;
}


BImageControl::~BImageControl(void) {
  if (red_buffer) delete [] red_buffer;
  if (green_buffer) delete [] green_buffer;
  if (blue_buffer) delete [] blue_buffer;
  if (next_red_buffer) delete [] next_red_buffer;
  if (next_green_buffer) delete [] next_green_buffer;
  if (next_blue_buffer) delete [] next_blue_buffer;

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
    fprintf(stderr, "BImageContol::~BImageControl: pixmap cache - "
	    "releasing %d pixmaps\n", n);
    
    for (i = 0; i < n; i++) {
      Cache *tmp = cache->first();
      XFreePixmap(display, tmp->pixmap);
      cache->remove(tmp);
      delete tmp;
    }
  }
  
  delete cache;
}


Bool BImageControl::doDither(void) {
  return blackbox->hasImageDither();
}


Visual *BImageControl::getVisual(void) {
  return screen->getVisual();
}


Pixmap BImageControl::searchCache(unsigned int width, unsigned int height,
				  unsigned long texture,
				  BColor *c1, BColor *c2) {
  if (cache->count()) {
    LinkedListIterator<Cache> it(cache);
    
    for (; it.current(); it++) {
      if ((it.current()->width == width) &&
	  (it.current()->height == height) &&
	  (it.current()->texture == texture) &&
	  (it.current()->pixel1 == c1->getPixel()))
	if (texture & BImage_Gradient) {
	  if (it.current()->pixel2 == c2->getPixel()) {
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
				  BTexture *texture) {
  Pixmap pixmap = searchCache(width, height, texture->getTexture(),
			      texture->getColor(), texture->getColorTo());
  if (pixmap) return pixmap;
  
  BImage image(this, width, height);
  pixmap = image.render(texture);
  
  if (pixmap) {
    Cache *tmp = new Cache;
    
    tmp->pixmap = pixmap;
    tmp->width = width;
    tmp->height = height;
    tmp->count = 1;
    tmp->texture = texture->getTexture();
    tmp->pixel1 = texture->getColor()->getPixel();
    
    if (texture->getTexture() & BImage_Gradient)
      tmp->pixel2 = texture->getColorTo()->getPixel();
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
  color.pixel = 0;
  
  if (! XParseColor(display, DefaultColormap(display, screen_number),
		    colorname, &color)) {
    fprintf(stderr, "BImageControl::getColor: color parse error: \"%s\"\n",
	    colorname);
  } else if (! XAllocColor(display, DefaultColormap(display, screen_number),
			   &color)) {
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
  color.pixel = 0;

  if (! XParseColor(display, DefaultColormap(display, screen_number),
		    colorname, &color)) {
    fprintf(stderr, "BImageControl::getColor: color parse error: \"%s\"\n",
	    colorname);
  } else if (! XAllocColor(display, DefaultColormap(display, screen_number),
			   &color)) {
    fprintf(stderr, "BImageControl::getColor: color alloc error: \"%s\"\n",
	    colorname);
  }
  
  return color.pixel;
}


void BImageControl::getColorTables(unsigned char **rmt, unsigned char **gmt,
				   unsigned char **bmt,
				   int *roff, int *goff, int *boff) {
  if (rmt) *rmt = red_color_table;
  if (gmt) *gmt = green_color_table;
  if (bmt) *bmt = blue_color_table;

  if (roff) *roff = red_offset;
  if (goff) *goff = green_offset;
  if (boff) *boff = blue_offset;
}


void BImageControl::getXColorTable(XColor **c, int *n) {
  if (c) *c = colors;
  if (n) *n = ncolors;
}


void BImageControl::getDitherBuffers(unsigned int w,
				     short **r, short **g, short **b,
				     short **nr, short **ng, short **nb,
				     unsigned char **ret,
				     unsigned char **get,
				     unsigned char **bet,
				     unsigned char **ret38,
				     unsigned char **get38,
				     unsigned char **bet38) {
  if (w > dither_buffer_width) {
    if (red_buffer) delete [] red_buffer;
    if (green_buffer) delete [] green_buffer;
    if (blue_buffer) delete [] blue_buffer;
    if (next_red_buffer) delete [] next_red_buffer;
    if (next_green_buffer) delete [] next_green_buffer;
    if (next_blue_buffer) delete [] next_blue_buffer;
    
    dither_buffer_width = w;
    
    red_buffer = new short[dither_buffer_width];
    green_buffer = new short[dither_buffer_width];
    blue_buffer = new short[dither_buffer_width];
    next_red_buffer = new short[dither_buffer_width];
    next_green_buffer = new short[dither_buffer_width];
    next_blue_buffer = new short[dither_buffer_width];
  }

  *r = red_buffer;
  *g = green_buffer;
  *b = blue_buffer;
  *nr = next_red_buffer;
  *ng = next_green_buffer;
  *nb = next_blue_buffer;
  *ret = red_error_table;
  *get = green_error_table;
  *bet = blue_error_table;
  *ret38 = red_error38_table;
  *get38 = green_error38_table;
  *bet38 = blue_error38_table;
}


void BImageControl::installRootColormap(void) {
  blackbox->grab();
  
  Bool install = True;
  int i = 0, ncmap = 0;
  Colormap *cmaps = XListInstalledColormaps(display, window, &ncmap);
  
  if (cmaps) {
    for (i = 0; i < ncmap; i++)
      if (*(cmaps + i) == root_colormap)
	install = False;
    
    if (install)
      XInstallColormap(display, root_colormap);
    
    XFree(cmaps);
  }
  
  blackbox->ungrab();
}
