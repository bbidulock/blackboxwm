//
// Image.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@tcac.net
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "blackbox.hh"
#include "Image.hh"
#include "Screen.hh"

#ifdef HAVE_STDIO_H
#  include <stdio.h>
#endif

#ifdef HAVE_MALLOC_H
#  include <malloc.h>
#else
#  ifdef STDC_HEADERS
#    include <stdlib.h>
#  endif
#endif


BImage::BImage(BImageControl *c, unsigned int w, unsigned int h) {
  control = c;

  width = ((signed) w > 0) ? w : 1;
  height = ((signed) h > 0) ? h : 1;

  red = new unsigned char[width * height];
  green = new unsigned char[width * height];
  blue = new unsigned char[width * height];

  cpc = control->getColorsPerChannel();
  cpccpc = cpc * cpc;
  
  control->getMaskTables(&tr, &tg, &tb, &roff, &goff, &boff);
  
  if (control->getVisual()->c_class != TrueColor)
    control->getColorTable(&colors, &ncolors);
}


BImage::~BImage(void) {
  if (red) delete [] red;
  if (green) delete [] green;
  if (blue) delete [] blue;
}


Pixmap BImage::render(const BTexture *texture)
{
  if (texture->texture & BImage_Solid) {
    return render_solid(texture);
  } else if (texture->texture & BImage_Gradient)
    return render_gradient(texture);
  
  return None;
}


Pixmap BImage::render_solid(const BTexture *texture) {
  Pixmap pixmap = XCreatePixmap(control->getDisplay(),
				control->getDrawable(), width,
				height, control->getDepth());
  if (pixmap == None) {
    fprintf(stderr, "BImage::render_solid: error creating pixmap\n");
    return None;
  }
  
  XGCValues gcv;
  GC gc, hgc, lgc;
  
  gcv.foreground = texture->color.pixel;
  gcv.fill_style = FillSolid;
  gc = XCreateGC(control->getDisplay(), pixmap, GCForeground | GCFillStyle, &gcv);
  
  gcv.foreground = texture->hiColor.pixel;
  hgc = XCreateGC(control->getDisplay(), pixmap, GCForeground, &gcv);
  
  gcv.foreground = texture->loColor.pixel;
  lgc = XCreateGC(control->getDisplay(), pixmap, GCForeground, &gcv);
  
  XFillRectangle(control->getDisplay(), pixmap, gc, 0, 0, width, height);
  
  if (texture->texture & BImage_Bevel1) {
    if (texture->texture & BImage_Raised) {
      XDrawLine(control->getDisplay(), pixmap, lgc, 0, height - 1,
		width - 1, height - 1);
      XDrawLine(control->getDisplay(), pixmap, lgc, width - 1, height - 1,
		width - 1, 0);
      
      XDrawLine(control->getDisplay(), pixmap, hgc, 0, 0,
		width - 1, 0);
      XDrawLine(control->getDisplay(), pixmap, hgc, 0, height - 1,
		0, 0);
    } else if (texture->texture & BImage_Sunken) {
      XDrawLine(control->getDisplay(), pixmap, hgc, 0, height - 1,
		width - 1, height - 1);
      XDrawLine(control->getDisplay(), pixmap, hgc, width - 1, height - 1,
		width - 1, 0);
      
      XDrawLine(control->getDisplay(), pixmap, lgc, 0, 0, width - 1, 0);
      XDrawLine(control->getDisplay(), pixmap, lgc, 0, height - 1, 0, 0);
    }
  } else if (texture->texture & BImage_Bevel2) {
    if (texture->texture & BImage_Raised) {
      XDrawLine(control->getDisplay(), pixmap, lgc, 1, height - 3,
		width - 3, height - 3);
      XDrawLine(control->getDisplay(), pixmap, lgc, width - 3, height - 3,
		width - 3, 1);
      
      XDrawLine(control->getDisplay(), pixmap, hgc, 1, 1,
		width - 3, 1);
      XDrawLine(control->getDisplay(), pixmap, hgc, 1, height - 3,
		1, 1);
    } else if (texture->texture & BImage_Sunken) {
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


Pixmap BImage::render_gradient(const BTexture *texture) {
  int inverted = 0;
  
  if (texture->texture & BImage_Sunken) {
    from = texture->colorTo;
    to = texture->color;

    if (! (texture->texture & BImage_Invert)) inverted = 1;
  } else {
    from = texture->color;
    to = texture->colorTo;
    
    if (texture>texture & BImage_Invert) inverted = 1;
  }
  
  if (texture->texture & BImage_Diagonal) dgradient();
  else if (texture->texture & BImage_Horizontal) hgradient();
  else if (texture->texture & BImage_Vertical) vgradient();

  if (texture->texture & BImage_Bevel1) bevel1();
  else if (texture->texture & BImage_Bevel2) bevel2();
  
  if (inverted) invert();
  
  return renderPixmap();
}


XImage *BImage::renderXImage(void) {
  XImage *image = XCreateImage(control->getDisplay(), control->getVisual(),
			       control->getDepth(), ZPixmap,
			       0, 0, width, height, 32, 0);
  if (! image) {
    fprintf(stderr, "BImage::renderXImage: error creating XImage\n");
    return 0;
  }

  unsigned char *d = (unsigned char *) malloc(image->bytes_per_line * height);
  if (! d) {
    fprintf(stderr,
	    "BImage::renderXImage: error allocating memory for image\n");
    XDestroyImage(image);
    return 0;
  }
  
  register unsigned int x, y, r = 0, g = 0, b = 0, i, off;
  
  unsigned char *idata = d, *pd = d, *pr, *pg, *pb;
  unsigned long pixel;

  if ((! tr) || (! tg) || (! tb)) {
    fprintf(stderr, "BImage::renderXImage: error getting color mask tables\n");
    XDestroyImage(image);
    return 0;
  }
  
  if (control->getVisual()->c_class != TrueColor)
    if ((! colors) || (! ncolors)) {
      fprintf(stderr,
	      "BImage::renderXImage: error getting pseudo color tables\n");
      XDestroyImage(image);
      return 0;
    }

  if (control->doDither()) {
    short er, eg, eb, *or, *og, *ob, *nor, *nog, *nob, *por, *pog, *pob;
    unsigned short *ort, *ogt, *obt;
    
    control->getDitherBuffers(width + 2, &or, &og, &ob, &nor, &nog, &nob,
			      &ort, &ogt, &obt);
    if ((! or) || (! og) || (! ob) || (! nor) || (! nog) || (! nob) ||
	(! ort) || (! ogt) || (! obt)) {
      fprintf(stderr,
	      "BImage::renderXImage: error getting dither information\n");
      XDestroyImage(image);
      return 0;
    }
    
    x = width;

    por = or;
    pog = og;
    pob = ob;

    pr = red;
    pg = green;
    pb = blue;

    while (x--) {
      *(por++) = *(pr++);
      *(pog++) = *(pg++);
      *(pob++) = *(pb++);
    }
    
    *por = *pog = *pob = 0;
    
    for (y = 0, off = 0; y < height; y++) {
      if (y < (height - 1)) {
	for (x = 0, i = off + width; x < width; x++, i++) {
	  *(nor + x) = *(red + i);
	  *(nog + x) = *(green + i);
	  *(nob + x) = *(blue + i);
	}
	
	i--;
	*(nor + x) = *(red + i);
	*(nog + x) = *(green + i);
	*(nob + x) = *(blue + i);
      }
      
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
	
	switch (control->getVisual()->c_class) {
	case StaticColor:
	case PseudoColor:
	  pixel = (r * cpccpc) + (g * cpc) + b;
	  *idata++ = colors[pixel].pixel;
	  
	  break;
	  
	case TrueColor:
	  pixel = (r << roff) | (g << goff) | (b << boff);
	  
	  switch (image->byte_order) {
	  case MSBFirst:
	    {
	      switch (image->bits_per_pixel) {
	      case 32: *(idata++) = (pixel >> 24);
	      case 24: *(idata++) = (pixel >> 16);
	      case 16: *(idata++) = (pixel >> 8);
	      default: *(idata++) = (pixel);
	      }

	      break;
	    }

	  case LSBFirst:
	    {
	      *(idata++) = pixel;
	      
	      switch (image->bits_per_pixel) {
	      case 32:
		*(idata++) = (pixel >> 8);
		*(idata++) = (pixel >> 16);
		*(idata++) = (pixel >> 24);
		break;

	      case 24:
		*(idata++) = (pixel >> 8);
		*(idata++) = (pixel >> 16);
		break;
		
	      case 16:
		*(idata++) = (pixel >> 8);
		break;
	      }

	      break;
	    }
	  }
	  
          break;
	  
	default:
	  fprintf(stderr, "BImage::renderXImage: unsupported visual\n");
	  image->data = (char *) d;
	  XDestroyImage(image);
	  return 0;
	}

	er = *(or + x) - *(ort + *(or + x));
	eg = *(og + x) - *(ogt + *(og + x));
	eb = *(ob + x) - *(obt + *(ob + x));
	
	*(or + x + 1) += er;
	*(og + x + 1) += eg;
	*(ob + x + 1) += eb;
	
	*(nor + x) += er;
	*(nog + x) += eg;
	*(nob + x) += eb;
	
	*(nor + x + 1) -= (er >> 1) + (er >> 2);
	*(nog + x + 1) -= (eg >> 1) + (eg >> 2);
	*(nob + x + 1) -= (eb >> 1) + (eb >> 2);
      }
      
      off += image->width;
      idata = (pd += image->bytes_per_line);
      
      por = or; or = nor; nor = por;
      pog = og; og = nog; nog = pog;
      pob = ob; ob = nob; nob = pob;
    }
  } else {
    for (y = 0, off = 0; y < height; y++) {
      for (x = 0; x < width; x++, off++) {
	r = *(tr + *(red + off));
	g = *(tg + *(green + off));
	b = *(tb + *(blue + off));
	
	switch (control->getVisual()->c_class) {
	case StaticColor:
	case PseudoColor:
	  pixel = (r * cpccpc) + (g * cpc) + b;
	  *idata++ = colors[pixel].pixel;
	  break;
	  
	case TrueColor:
	  pixel = (r << roff) | (g << goff) | (b << boff);
	  
	  switch (image->byte_order) {
	  case MSBFirst:
	    {
	      switch (image->bits_per_pixel) {
	      case 32: *(idata++) = (pixel >> 24);
	      case 24: *(idata++) = (pixel >> 16);
	      case 16: *(idata++) = (pixel >> 8);
	      default: *(idata++) = (pixel);
	      }
	      
	      break;
	    }
	    
	  case LSBFirst:
	    {
	      *(idata++) = pixel;
	      
	      switch (image->bits_per_pixel) {
	      case 32:
		*(idata++) = (pixel >> 8);
		*(idata++) = (pixel >> 16);
		*(idata++) = (pixel >> 24);
		break;
		
	      case 24:
		*(idata++) = (pixel >> 8);
		*(idata++) = (pixel >> 16);
		break;
		
	      case 16:
		*(idata++) = (pixel >> 8);
		break;
	      }
	      
	      break;
	    }
	  }
	  
	  break;
	  
	default:
	  fprintf(stderr, "BImage::renderXImage: unsupported visual\n");
	  image->data = (char *) d;
	  XDestroyImage(image);
	  return 0; 
	}
      }

      idata = (pd += image->bytes_per_line);
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
  }
  
  XPutImage(control->getDisplay(), pixmap,
	    DefaultGC(control->getDisplay(),
		      control->getScreen()->getScreenNumber()),
            image, 0, 0, 0, 0, width, height);
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
  float fr = (float) from.red,
    fg = (float) from.green,
    fb = (float) from.blue,
    tr = (float) to.red,
    tg = (float) to.green,
    tb = (float) to.blue,
    w = (float) (width * 2),
    h = (float) (height * 2),
    yr = 0.0,
    yg = 0.0,
    yb = 0.0,
    xr, xg, xb, drx, dgx, dbx, dry, dgy, dby;
  unsigned char *pr = red, *pg = green, *pb = blue;
  
  register unsigned int x, y;
  
  drx = dry = (tr - fr);
  dgx = dgy = (tg - fg);
  dbx = dby = (tb - fb);

  drx /= w;
  dgx /= w;
  dbx /= w;

  dry /= h;
  dgy /= h;
  dby /= h;

  for (y = 0; y < height; y++) {
    xr = fr + yr;
    xg = fg + yg;
    xb = fb + yb;
    
    for (x = 0; x < width; x++) {   
      *(pr++) = (unsigned char) (xr);
      *(pg++) = (unsigned char) (xg);
      *(pb++) = (unsigned char) (xb);
      
      xr += drx;
      xg += dgx;
      xb += dbx;
    }
    
    yr += dry;
    yg += dgy;
    yb += dby;
  }
}


void BImage::hgradient(void) {
  float fr, fg, fb, tr, tg, tb, drx, dgx, dbx, xr, xg, xb, w = (float) width;
  unsigned char *pr, *pg, *pb, *rr, *gg, *bb;

  register unsigned int x, y;

  xr = fr = (float) from.red;
  xg = fg = (float) from.green;
  xb = fb = (float) from.blue;

  tr = (float) to.red;
  tg = (float) to.green;
  tb = (float) to.blue;

  drx = (tr - fr) / w;
  dgx = (tg - fg) / w;
  dbx = (tb - fb) / w;

  pr = red;
  pg = green;
  pb = blue;

  // this renders one line of the hgradient
  for (x = 0; x < width; x++) {
    *(pr++) = (unsigned char) (xr);
    *(pg++) = (unsigned char) (xg);
    *(pb++) = (unsigned char) (xb);
    
    xr += drx;
    xg += dgx;
    xb += dbx;
  }

  // and this copies to the rest of the image
  for (y = 1; y < height; y++) {
    rr = red;
    gg = green;
    bb = blue;
    
    for (x = 0; x < width; x++) {
      *(pr++) = *(rr++);
      *(pg++) = *(gg++);
      *(pb++) = *(bb++);
    }
  }
}


void BImage::vgradient(void) {
  float fr, fg, fb, tr, tg, tb, dry, dgy, dby, yr, yg, yb,
    h = (float) height;
  unsigned char *pr = red, *pg = green, *pb = blue;
  
  register unsigned char r, g, b;
  register unsigned int x, y;
  
  yr = fr = (float) from.red;
  yg = fg = (float) from.green;
  yb = fb = (float) from.blue;

  tr = (float) to.red;
  tg = (float) to.green;
  tb = (float) to.blue;

  dry = (tr - fr) / h;
  dgy = (tg - fg) / h;
  dby = (tb - fb) / h;

  for (y = 0; y < height; y++) {
    yr += dry;
    yg += dgy;
    yb += dby;
      
    r = (unsigned char) (yr);
    g = (unsigned char) (yg);
    b = (unsigned char) (yb);
    
    for (x = 0; x < width; x++) {
      *(pr++) = r;
      *(pg++) = g;
      *(pb++) = b;
    }
  }
}


BImageControl::BImageControl(Blackbox *bb, BScreen *scrn) {
  blackbox = bb;
  screen = scrn;
  display = screen->getDisplay();
  screen_depth = screen->getDepth();
  window = screen->getRootWindow();
  screen_number = screen->getScreenNumber();

  colors = 0;
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
  }
  
  if (bits_per_pixel == 0) bits_per_pixel = screen_depth;
  if (pmv) XFree(pmv);

  red_offset = green_offset = blue_offset = 0;

  switch (screen->getVisual()->c_class) {
  case TrueColor:
    {
      int i;

      // compute tables for red channel
      unsigned long mask = screen->getVisual()->red_mask, emask = mask;
      while (! (mask & 1)) { red_offset++; mask >>= 1; }
      for (i = 0; i < 256; i++) {
	rmask_table[i] = (i * mask + 0x7f) / 0xff;
	rerr_table[i] = ((rmask_table[i] << 8) / (emask >> red_offset));
      }
      
      // compute tables for green channel
      emask = mask = screen->getVisual()->green_mask;
      while (! (mask & 1)) { green_offset++; mask >>= 1; }
      for (i = 0; i < 256; i++) {
	gmask_table[i] = (i * mask + 0x7f) / 0xff;
	gerr_table[i] = ((gmask_table[i] << 8) / (emask >> green_offset));
      }
      
      // compute tables for blue channel
      emask = mask = screen->getVisual()->blue_mask;
      while (! (mask & 1)) { blue_offset++; mask >>= 1; }
      for (i = 0; i < 256; i++) {
	bmask_table[i] = (i * mask + 0x7f) / 0xff;
	berr_table[i] = ((bmask_table[i] << 8) / (emask >> blue_offset));
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
	rerr_table[i] = gerr_table[i] = berr_table[i] =
	  ((rmask_table[i] << 8) / (mask));
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
				  const BTexture *texture)
{
  Pixmap pixmap = searchCache(width, height, texture->texture,
			      texture->color, texture->colorTo);
  if (pixmap) return pixmap;
  
  BImage image(this, width, height);
  pixmap = image.render(texture);
  
  if (pixmap) {
    Cache *tmp = new Cache;

    tmp->pixmap = pixmap;
    tmp->width = width;
    tmp->height = height;
    tmp->count = 1;
    tmp->texture = texture->texture;
    tmp->pixel1 = texture->color.pixel;

    if (texture->texture & BImage_Gradient)
      tmp->pixel2 = texture->colorTo.pixel;
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


void BImageControl::getDitherBuffers(unsigned int w,
				     short **r, short **g, short **b,
				     short **nr, short **ng, short **nb,
				     unsigned short **ret,
				     unsigned short **get,
				     unsigned short **bet) {
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
  *ret = rerr_table;
  *get = gerr_table;
  *bet = berr_table;
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
