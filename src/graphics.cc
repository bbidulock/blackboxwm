//
// graphics.cc for Blackbox - an X11 Window manager
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
#include "graphics.hh"

#include <malloc.h>

#ifdef GradientHack
#  include <math.h>
#  include <float.h>
#endif


// *************************************************************************
// Graphics engine class code
// *************************************************************************

BImage::BImage(Blackbox *bb, unsigned int w, unsigned int h, int d)
{
  blackbox = bb;
  width = ((signed) w > 0) ? w : 1;
  height = ((signed) h > 0) ? h : 1;
  depth = d;

  data = new unsigned long [width * height];

  if (blackbox->imageDither()) {
    or = new unsigned short[width + 2];
    og = new unsigned short[width + 2];
    ob = new unsigned short[width + 2];

    nor = new unsigned short[width + 2];
    nog = new unsigned short[width + 2];
    nob = new unsigned short[width + 2];
  } else
    or = og = ob = nor = nog = nob = 0;
}


BImage::~BImage(void) {
  if (data) delete [] data;

  if (or) delete [] or;
  if (og) delete [] og;
  if (ob) delete [] ob;
  if (nor) delete [] nor;
  if (nog) delete [] nog;
  if (nob) delete [] nob;
}


Pixmap BImage::renderImage(unsigned long texture, const BColor &color1,
			   const BColor &color2)
{
  if (texture & BImageSolid)
    return renderSolidImage(texture, color1);
  else if (texture & BImageGradient)
    return renderGradientImage(texture, color1, color2);
  
  return None;
}


Pixmap BImage::renderInvertedImage(unsigned long texture,
				   const BColor &color1, const BColor &color2)
{
  texture |= BImageInverted;
  if (texture & BImageSolid)
    return renderSolidImage(texture, color1);
  else if (texture & BImageGradient)
    return renderGradientImage(texture, color1, color2);
  
  return None;
}


Pixmap BImage::renderSolidImage(unsigned long texture, const BColor &color) {
  bg_color = color;
  setBackgroundColor(color);

  if (texture & BImageRaised) {
    if (texture & BImageBevel1) {
      if (color.r == color.g && color.g == color.b && color.b == 0)
	renderBevel1(True, True);
      else
	renderBevel1(True);
    } else if (texture & BImageBevel2) {
      if (color.r == color.g && color.g == color.b && color.b == 0)
	renderBevel2(True, True);
      else
	renderBevel2(True);
    }

    if (texture & BImageInverted)
      invertImage();
  } else if (texture & BImageSunken) {
    if (texture & BImageBevel1) {
      if (color.r == color.g && color.g == color.b && color.b == 0)
	renderBevel1(True, True);
      else
	renderBevel1(True);
    } else if (texture & BImageBevel2) {
      if (color.r == color.g && color.g == color.b && color.b == 0)
	renderBevel2(True, True);
      else
	renderBevel2(True);
    }

    if (! (texture & BImageInverted))
      invertImage();
  }
  
  return convertToPixmap(! (texture & BImageNoDitherSolid));
}


Pixmap BImage::renderGradientImage(unsigned long texture, const BColor &from,
				   const BColor &to)
{
  bg_color = from;

  if (texture & BImageDiagonal) {    
    if (texture & BImageSunken) {
      renderDGradient(to, from);
      
      if (texture & BImageBevel1)
	renderBevel1(False);
      else if (texture & BImageBevel2)
	renderBevel2(False);

      if (! (texture & BImageInverted))
	invertImage();
    } else {
      renderDGradient(from, to);
      
      if (texture & BImageBevel1)
	renderBevel1(False);
      else if (texture & BImageBevel2)
	renderBevel2(False);
      
      if (texture & BImageInverted)
	invertImage();
    }
  } else if (texture & BImageHorizontal) {
    if (texture & BImageSunken) {
      renderHGradient(to, from);
      
      if (texture & BImageBevel1)
	renderBevel1(False);
      else if (texture & BImageBevel2)
	renderBevel2(False);

      if (! (texture & BImageInverted))
	invertImage();
    } else {
      renderHGradient(from, to);
      
      if (texture & BImageBevel1)
	renderBevel1(False);
      else if (texture & BImageBevel2)
	renderBevel2(False);
      
      if (texture & BImageInverted)
	invertImage();
    }
  } else if (texture & BImageVertical) {
      if (texture & BImageSunken) {
      renderVGradient(to, from);
      
      if (texture & BImageBevel1)
	renderBevel1(False);
      else if (texture & BImageBevel2)
	renderBevel2(False);

      if (! (texture & BImageInverted))
	invertImage();
    } else {
      renderVGradient(from, to);
      
      if (texture & BImageBevel1)
	renderBevel1(False);
      else if (texture & BImageBevel2)
	renderBevel2(False);
      
      if (texture & BImageInverted)
	invertImage();
    }
  }

  return convertToPixmap();
}


Bool BImage::getPixel(unsigned int x, unsigned int y, BColor &color) {
  if (x > width || y > height) {
    color.pixel = color.r = color.g = color.b = 0;
    return False;
  }

  unsigned int xy = x * y;
  unsigned long *p = data +xy;

  color.pixel = *p;
  color.r = (((*p) >> 16) & 0xff);
  color.g = (((*p) >> 8) & 0xff);
  color.b = ((*p) & 0xff);

  return True;
}


Bool BImage::getPixel(unsigned int x, unsigned int y, unsigned long *pixel) {
  // NOTE:  pixel will be returned a TrueColor (32 bit) pixel value.

  if (x > width || y > height) {
    *pixel = 0;
    return False;
  }
  
  unsigned int xy = x * y;
  *pixel = *(data + xy);
  return True;
}


Bool BImage::putPixel(unsigned int x, unsigned int y, const BColor &color) {
  if (x > width || y > height) return False;
  
  unsigned int xy = x * y;
  *(data + xy) = (color.r << 16) + (color.g << 8) + color.b;
  return True;
}


Bool BImage::putPixel(unsigned int x, unsigned int y, unsigned long pixel) {
  // NOTE:  pixel must be a TrueColor (32 bit) pixel value.

  if (x > width || y > height) return False;

  unsigned int xy = x * y; 
  *(data + xy) = pixel;
  return True;
}



XImage *BImage::convertToXImage(Bool dither) {
  int bpp = blackbox->BitsPerPixel();

  // since XDestroyImage frees the image data... we malloc() it so that we
  // don't possibly introduce problems with free()'ing space that was new'd
  char *d = (char *) malloc((width * height * (bpp / 8)) * sizeof(char));
  if (! d) return 0;

  XImage *image = XCreateImage(blackbox->control(), blackbox->visual(), depth,
			       ZPixmap, 0, d, width, height, 8, 0);
  if (image == NULL) return 0;
  
  unsigned char r = 0, g = 0, b = 0;
  unsigned int wh = width * height;
  unsigned long *p = data;
    
  // hmm... this is a quick fix (read: UGLY HACK) for 15bpp displays.  Since
  // 15bpp servers still use 16 bits per pixels, the only different between
  // the two is how the rgb elements are shifted.  Hopefully I will find a way
  // to remedy this
  if (depth == 15) bpp = 15;

  switch (bpp) {
  case 32: {
    unsigned char *im = (unsigned char *) image->data;
    
    if (image->byte_order == LSBFirst) {
      while (--wh) {
	*(im++) = (unsigned char) (*p);
	*(im++) = (unsigned char) ((*p) >> 8);
	*(im++) = (unsigned char) ((*(p++)) >> 16);
	*(im++) = (unsigned char) 0;
      }
      
      *(im++) = (unsigned char) (*p);
      *(im++) = (unsigned char) ((*p) >> 8);
      *(im++) = (unsigned char) ((*p) >> 16);
      *(im) = (unsigned char) 0;
    } else if (image->byte_order == MSBFirst) {
      while (--wh) {
        *(im++) = (unsigned char) 0;
	*(im++) = (unsigned char) ((*p) >> 16);
	*(im++) = (unsigned char) ((*p) >> 8);; 
	*(im++) = (unsigned char) (*(p++));
      }
      
      *(im++) = (unsigned char) 0;
      *(im++) = (unsigned char) ((*p) >> 16);
      *(im++) = (unsigned char) ((*p) >> 8);
      *(im) = (unsigned char) (*p);
    }
    
    break; }

  case 24: {
    unsigned char *im = (unsigned char *) image->data;

    if (image->byte_order == LSBFirst) {
      while (--wh) {
	*(im++) = (unsigned char) (*p);
	*(im++) = (unsigned char) ((*p) >> 8);
	*(im++) = (unsigned char) ((*(p++)) >> 16);
      }
      
      *(im++) = (unsigned char) (*p);
      *(im++) = (unsigned char) ((*p) >> 8);
      *(im) = (unsigned char) ((*p) >> 16);
    } else if (image->byte_order == MSBFirst) {
      while (--wh) {
	*(im++) = (unsigned char) ((*p) >> 16);
	*(im++) = (unsigned char) ((*p) >> 8);
	*(im++) = (unsigned char) (*(p++));
      }

      *(im++) = (unsigned char) ((*p) >> 16);
      *(im++) = (unsigned char) ((*p) >> 8);
      *(im) = (unsigned char) (*p);
    }
    break; }

  case 16: {
    unsigned short *im = (unsigned short *) image->data;

    if (blackbox->imageDither() && dither) {
      // lets dither the image while blitting to the XImage
      unsigned int i, x, y, ofs, w2 = width * 2;
      unsigned short *tor, *tog, *tob, er, eg, eb, sr, sg, sb;
      
      if ((! or) || (! og) || (! ob) || (! nor) || (! nog) || (! nob)) {
	XDestroyImage(image);
	return 0;
      }
      
      tor = or; tog = og; tob = ob;
      
      for (i = 0; i < width; i++) {
	*(tor++) = ((*(data + i)) & 0xff0000) >> 16;
	*(tog++) = ((*(data + i)) & 0xff00) >> 8;
	*(tob++) = ((*(data + i)) & 0xff);
      }
      
      ofs = 0;
      for (y = 0; y < height; y++) {
	if (y < (height - 1)) {
	  tor = nor; tog = nog; tob = nob;
	  
	  for (i = ofs + width; i < ofs + w2; i++) {
	    *(tor++) = ((*(data + i)) & 0xff0000) >> 16;
	    *(tog++) = ((*(data + i)) & 0xff00) >> 8;
	    *(tob++) = ((*(data + i)) & 0xff);
	  }
	  
	  *tor = ((*(data + i - 1)) & 0xff0000) >> 16;
	  *tog = ((*(data + i - 1)) & 0xff00) >> 8;
	  *tob = ((*(data + i - 1)) & 0xff);
	}
	
	for (x = 0; x < width; x++) {
	  if (*(or + x) > 255) *(or + x) = 255;
	  if (*(og + x) > 255) *(og + x) = 255;
	  if (*(ob + x) > 255) *(ob + x) = 255;
	  
	  sr = *(or + x);
	  sg = *(og + x);
	  sb = *(ob + x);
	  
	  sr = ((sr << 8) & image->red_mask);
	  sg = ((sg << 3) & image->green_mask);
	  sb = ((sb >> 3) & image->blue_mask);
	  
	  *(im++) = sr|sg|sb;
	  
	  er = *(or + x) - (sr >> 8);
	  eg = *(og + x) - (sg >> 3);
	  eb = *(ob + x) - (sb << 3);
	  
	  *(or + x + 1) += ((er >> 3) + (er >> 2));
	  *(og + x + 1) += ((eg >> 3) + (eg >> 2));
	  *(ob + x + 1) += ((eb >> 3) + (eb >> 2));
	  
	  *(nor + x) += ((er >> 3) + (er >> 2));
	  *(nog + x) += ((eg >> 1) + (eg >> 2));
	  *(nob + x) += ((eb >> 1) + (eg >> 2));
	  
	  *(nor + x + 1) += (er >> 2);
	  *(nog + x + 1) += (eg >> 2);
	  *(nob + x + 1) += (eb >> 2);
	}
	
	ofs += width;
	
	tor = or; tog = og; tob = ob;
	or = nor; og = nog; ob = nob;
	nor = tor; nog = tog; nob = tob;
      }
    } else {
      while (--wh) {
	r = (unsigned char) (((*p) & 0xff0000) >> 16);
	g = (unsigned char) (((*p) & 0xff00) >> 8);
	b = (unsigned char) (*(p++) & 0xff);
	*(im++) = (((b >> 3) & image->blue_mask) |
		   ((g << 3) & image->green_mask) |
		   ((r << 8) & image->red_mask)); 
      }
      
      r = (unsigned char) (((*p) & 0xff0000) >> 16);
      g = (unsigned char) (((*p) & 0xff00) >> 8);
      b = (unsigned char) (*(p++) & 0xff);
      *im = (((b >> 3) & image->blue_mask) |
	     ((g << 3) & image->green_mask) |
	     ((r << 8) & image->red_mask));
    }

    break; }

  case 15: {
    unsigned short *im = (unsigned short *) image->data;

    if (blackbox->imageDither() && dither) {
      // lets dither the image while blitting to the XImage
      unsigned int i, x, y, ofs, w2 = (width * 2);
      unsigned short *tor, *tog, *tob, er, eg, eb, sr, sg, sb;
      
      if ((! or) || (! og) || (! ob) || (! nor) || (! nog) || (! nob)) {
	XDestroyImage(image);
	return 0;
      }

      tor = or; tog = og; tob = ob;
      
      for (i = 0; i < width; i++) {
	*(tor++) = ((*(data + i)) & 0xff0000) >> 16;
	*(tog++) = ((*(data + i)) & 0xff00) >> 8;
	*(tob++) = ((*(data + i)) & 0xff);
      }

      ofs = 0;
      for (y = 0; y < height; y++) {
	if (y < (height - 1)) {
	  tor = nor; tog = nog; tob = nob;
	  
	  for (i = ofs + width; i < ofs + w2; i++) {
	    *(tor++) = ((*(data + i)) & 0xff0000) >> 16;
	    *(tog++) = ((*(data + i)) & 0xff00) >> 8;
	    *(tob++) = ((*(data + i)) & 0xff);
	  }
	  
	  *tor = ((*(data + i - 1)) & 0xff0000) >> 16;
	  *tog = ((*(data + i - 1)) & 0xff00) >> 8;
	  *tob = ((*(data + i - 1)) & 0xff);
	}

	for (x = 0; x < width; x++) {
	  if (*(or + x) > 255) *(or + x) = 255;
	  if (*(og + x) > 255) *(og + x) = 255;
	  if (*(ob + x) > 255) *(ob + x) = 255;
	  
	  sr = *(or + x);
	  sg = *(og + x);
	  sb = *(ob + x);

	  sr = ((sr << 7) & image->red_mask);
	  sg = ((sg << 2) & image->green_mask);
	  sb = ((sb >> 3) & image->blue_mask);
	  
	  *(im++) = sr|sg|sb;

	  er = *(or + x) - (sr >> 7);
	  eg = *(og + x) - (sg >> 2);
	  eb = *(ob + x) - (sb << 3);

	  *(or + x + 1) += ((er >> 3) + (er >> 2));
	  *(og + x + 1) += ((eg >> 3) + (eg >> 2));
	  *(ob + x + 1) += ((eb >> 3) + (eb >> 2));
	  
	  *(nor + x) += ((er >> 3) + (er >> 2));
	  *(nog + x) += ((eg >> 1) + (eg >> 2));
	  *(nob + x) += ((eb >> 1) + (eg >> 2));
	  
	  *(nor + x + 1) += (er >> 2);
	  *(nog + x + 1) += (eg >> 2);
	  *(nob + x + 1) += (eb >> 2);
	}
	ofs += width;
	
	tor = or; tog = og; tob = ob;
	or = nor; og = nog; ob = nob;
	nor = tor; nog = tog; nob = tob;
      }
    } else {
      while (--wh) {
	r = (unsigned char) (((*p) & 0xff0000) >> 16);
	g = (unsigned char) (((*p) & 0xff00) >> 8);
	b = (unsigned char) (*(p++) & 0xff);
	*(im++) = (((b >> 3) & image->blue_mask) |
		   ((g << 2) & image->green_mask) |
		   ((r << 7) & image->red_mask));
      }
      
      r = (unsigned char) (((*p) & 0xff0000) >> 16);
      g = (unsigned char) (((*p) & 0xff00) >> 8);
      b = (unsigned char) (*(p++) & 0xff);
      *im = (((b >> 3) & image->blue_mask) |
	     ((g << 2) & image->green_mask) |
	     ((r << 7) & image->red_mask));
    }
    
    break; }

  case 8: {
    int rr, gg, bb, cpc = blackbox->cpc8bpp(), cpc2 = cpc * cpc;
    unsigned char *im = (unsigned char *) image->data;

    if (blackbox->imageDither() && dither) {
      int d = 0xff / blackbox->cpc8bpp();
      unsigned int i, x, y, ofs, w2 = (width * 2);
      unsigned short *tor, *tog, *tob, er, eg, eb;

      if ((! or) || (! og) || (! ob) || (! nor) || (! nog) || (! nob)) {
	XDestroyImage(image);
	return 0;
      }

      tor = or; tog = og; tob = ob;
      
      for (i = 0; i < width; i++) {
	*(tor++) = ((*(data + i)) & 0xff0000) >> 16;
	*(tog++) = ((*(data + i)) & 0xff00) >> 8;
	*(tob++) = ((*(data + i)) & 0xff);
      }

      *tor = *tog = *tob = 0;
      ofs = 0;
      for (y = 0; y < height; y++) {
	if (y < (height - 1)) {
	  tor = nor; tog = nog; tob = nob;
	  
	  for (i = ofs + width; i < ofs + w2; i++) {
	    *(tor++) = ((*(data + i)) & 0xff0000) >> 16;
	    *(tog++) = ((*(data + i)) & 0xff00) >> 8;
	    *(tob++) = ((*(data + i)) & 0xff);
	  }
	  
	  *tor = *tog = *tob = 0;

	  *tor = ((*(data + i - 1)) & 0xff0000) >> 16;
	  *tog = ((*(data + i - 1)) & 0xff00) >> 8;
	  *tob = ((*(data + i - 1)) & 0xff);
	}

	for (x = 0; x < width; x++) {
	  rr = (*(or + x) * cpc) / 0xff;
	  gg = (*(og + x) * cpc) / 0xff;
	  bb = (*(ob + x) * cpc) / 0xff;

	  if (rr < 0) rr = 0;
	  else if (rr > (cpc - 1)) rr = (cpc - 1);
	  if (gg < 0) gg = 0;
	  else if (gg > (cpc - 1)) gg = (cpc - 1);
	  if (bb < 0) bb = 0;
	  else if (bb > (cpc - 1)) bb = (cpc - 1);
	  
	  *(im++) =
	    blackbox->Colors8bpp()[(rr * cpc2) + (gg * cpc) + bb].pixel;
	  
	  er = *(or + x) - (rr * d);
	  eg = *(og + x) - (gg * d);
	  eb = *(ob + x) - (bb * d);
	  
	  *(or + x + 1) += ((er >> 3) + (er >> 2));
	  *(og + x + 1) += ((eg >> 3) + (eg >> 2));
	  *(ob + x + 1) += ((eb >> 3) + (eb >> 2));
	  
	  *(nor + x) += ((er >> 3) + (er >> 2));
	  *(nog + x) += ((eg >> 1) + (eg >> 2));
	  *(nob + x) += ((eb >> 1) + (eg >> 2));
	  
	  *(nor + x + 1) += (er >> 2);
	  *(nog + x + 1) += (eg >> 2);
	  *(nob + x + 1) += (eb >> 2);
	}
	ofs += width;
	
	tor = or; tog = og; tob = ob;
	or = nor; og = nog; ob = nob;
	nor = tor; nog = tog; nob = tob;
      }
    } else {
      while (--wh) {
	r = (unsigned char) (((*p) & 0xff0000) >> 16);
	g = (unsigned char) (((*p) & 0xff00) >> 8);
	b = (unsigned char) ((*(p++)) & 0xff);
	
	rr = (r * cpc) / 0xff;
	gg = (g * cpc) / 0xff;
	bb = (b * cpc) / 0xff;
	
	if (rr < 0) rr = 0;
	else if (rr > (cpc - 1)) rr = (cpc - 1);
	if (gg < 0) gg = 0;
	else if (gg > (cpc - 1)) gg = (cpc - 1);
	if (bb < 0) bb = 0;
	else if (bb > (cpc - 1)) bb = (cpc - 1);
	*(im++) = blackbox->Colors8bpp()[(rr * cpc2) + (gg * cpc) + bb].pixel;
      }
      
      r = (unsigned char) (((*p) & 0xff0000) >> 16);
      g = (unsigned char) (((*p) & 0xff00) >> 8);
      b = (unsigned char) ((*(p++)) & 0xff);
      
      rr = (r * cpc) / 0xff;
      gg = (g * cpc) / 0xff;
      bb = (b * cpc) / 0xff;
      
      if (rr < 0) rr = 0;
      else if (rr > (cpc - 1)) rr = (cpc - 1);
      if (gg < 0) gg = 0;
      else if (gg > (cpc - 1)) gg = (cpc - 1);
      if (bb < 0) bb = 0;
      else if (bb > (cpc - 1)) bb = (cpc - 1);
      *(im) = blackbox->Colors8bpp()[(rr * cpc2) + (gg * cpc) + bb].pixel;
    }

    break; }
  }

  return image;
}


Pixmap BImage::convertToPixmap(Bool dither) {
  Pixmap pixmap = XCreatePixmap(blackbox->control(), blackbox->Root(), width,
				height,	(unsigned) depth);
  
  if (pixmap == None) return None;
  
  XImage *img = convertToXImage(dither);

  if (! img) {
    XFreePixmap(blackbox->control(), pixmap);
    return None;
  }

  XPutImage(blackbox->control(), pixmap,
	    DefaultGC(blackbox->control(),
		      DefaultScreen(blackbox->control())),
	    img, 0, 0, 0, 0, width, height);
  
  XDestroyImage(img);
  return pixmap;
}


void BImage::setBackgroundColor(const BColor &c) {
  unsigned int wh = width * height;
  unsigned long *p = data;

  bg_color.pixel = ((c.r << 16) | (c.g << 8) | (c.b));
  
  while (--wh) *(p++) = bg_color.pixel;
  *p = bg_color.pixel;
}


void BImage::renderBevel1(Bool solid, Bool solidblack) {
  if (width > 2 && height > 2) {
    unsigned long pix0 = 0, pix1 = 0;
    unsigned char r, g, b, rr ,gg ,bb;
    
    unsigned int w = width, h = height - 1, wh = w * h;
    unsigned long *p = data;
    
    if (solid) {
      if (solidblack) {
	pix0 = 0xc0c0c0;
	pix1 = 0x606060;
      } else {
	rr = bg_color.r * 3 / 2;
	if (rr < bg_color.r) rr = ~0;
	gg = bg_color.g * 3 / 2;
	if (gg < bg_color.g) gg = ~0;
	bb = bg_color.b * 3 / 2;
	if (bb < bg_color.b) bb = ~0;
	pix0 = ((rr << 16) | (gg << 8) | (bb));
	
	rr = bg_color.r * 3 / 4;
	if (rr > bg_color.r) rr = 0;
	gg = bg_color.g * 3 / 4;
	if (gg > bg_color.g) gg = 0;
	bb = bg_color.b * 3 / 4;
	if (bb > bg_color.b) bb = 0;
	pix1 = ((rr << 16) | (gg << 8) | (bb));
      }      

      while (--w) {
	*p = pix0;
	*((p++) + wh) = pix1;
      }

      *p = pix0;
      *(p + wh) = pix1;
      
      p = data + width;
      while (--h) {
	*p = pix0;
	p += (width - 1);
	*(p++) = pix1;
      }

      *p = pix0;
      p += (width - 1);
      *p = pix1;
    } else {
      while (--w) {
	r = (unsigned char) (((*p) & 0xff0000) >> 16);
	rr = r * 3 / 2;
	if (rr < r) rr = ~0;
	g = (unsigned char) (((*p) & 0x00ff00) >> 8);
	gg = g * 3 / 2;
	if (gg < g) gg = ~0;
	b = (unsigned char) ((*p) & 0x0000ff);
	bb = b * 3 / 2;
	if (bb < b) bb = ~0;
	pix0 = ((rr << 16) | (gg << 8) | (bb));
	
	r = (unsigned char) (((*(p + wh)) & 0xff0000) >> 16);
	rr = r * 3 / 4;
	if (rr > (unsigned char) r) rr = 0;
	g = (unsigned char) (((*(p + wh)) & 0x00ff00) >> 8);
	gg = g * 3 / 4;
	if (gg > (unsigned char) g) gg = 0;
	b = (unsigned char) ((*(p + wh)) & 0x0000ff);
	bb = b * 3 / 4;
	if (bb > (unsigned char) b) bb = 0;
	pix1 = ((rr << 16) | (gg << 8) | (bb));

	*p = pix0;
	*((p++) + wh) = pix1;
      }
      
      r = (unsigned char) (((*p) & 0xff0000) >> 16);
      rr = r * 3 / 2;
      if (rr < r) rr = ~0;
      g = (unsigned char) (((*p) & 0x00ff00) >> 8);
      gg = g * 3 / 2;
      if (gg < g) gg = ~0;
      b = (unsigned char) ((*p) & 0x0000ff);
      bb = b * 3 / 2;
      if (bb < b) bb = ~0;
      pix0 = ((rr << 16) | (gg << 8) | (bb));
      
      r = (unsigned char) (((*(p + wh)) & 0xff0000) >> 16);
      rr = r * 3 / 4;
      if (rr > (unsigned char) r) rr = 0;
      g = (unsigned char) (((*(p + wh)) & 0x00ff00) >> 8);
      gg = g * 3 / 4;
      if (gg > (unsigned char) g) gg = 0;
      b = (unsigned char) ((*(p + wh)) & 0x0000ff);
      bb = b * 3 / 4;
      if (bb > (unsigned char) b) bb = 0;
      pix1 = ((rr << 16) | (gg << 8) | (bb));
      
      *p = pix0;
      *(p + wh) = pix1;
      
      p = data + width;
      while (--h) {
	r = (unsigned char) (((*p) & 0xff0000) >> 16);
	rr = r * 3 / 2;
	if (rr < r) rr = ~0;
	g = (unsigned char) (((*p) & 0x00ff00) >> 8);
	gg = g * 3 / 2;
	if (gg < g) gg = ~0;
	b = (unsigned char) ((*p) & 0x0000ff);
	bb = b * 3 / 2;
	if (bb < b) bb = ~0;
	pix0 = ((rr << 16) | (gg << 8) | (bb));
	
	r = (unsigned char) (((*(p + (width - 1))) & 0xff0000) >> 16);
	rr = r * 3 / 4;
	if (rr > (unsigned char) r) rr = 0;
	g = (unsigned char) (((*(p + (width - 1))) & 0x00ff00) >> 8);
	gg = g * 3 / 4;
	if (gg > (unsigned char) g) gg = 0;
	b = (unsigned char) ((*(p + (width - 1))) & 0x0000ff);
	bb = b * 3 / 4;
	if (bb > (unsigned char) b) bb = 0;
	pix1 = ((rr << 16) | (gg << 8) | (bb));

	*p = pix0;
	p += (width - 1);
	*(p++) = pix1;
      }

      r = (unsigned char) (((*p) & 0xff0000) >> 16);
      rr = r * 3 / 2;
      if (rr < r) rr = ~0;
      g = (unsigned char) (((*p) & 0x00ff00) >> 8);
      gg = g * 3 / 2;
      if (gg < g) gg = ~0;
      b = (unsigned char) ((*p) & 0x0000ff);
      bb = b * 3 / 2;
      if (bb < b) bb = ~0;
      pix0 = ((rr << 16) | (gg << 8) | (bb));
      
      r = (unsigned char) (((*(p + (width - 1))) & 0xff0000) >> 16);
      rr = r * 3 / 4;
      if (rr > (unsigned char) r) rr = 0;
      g = (unsigned char) (((*(p + (width - 1))) & 0x00ff00) >> 8);
      gg = g * 3 / 4;
      if (gg > (unsigned char) g) gg = 0;
      b = (unsigned char) ((*(p + (width - 1))) & 0x0000ff);
      bb = b * 3 / 4;
      if (bb > (unsigned char) b) bb = 0;
      pix1 = ((rr << 16) | (gg << 8) | (bb));
      
      *p = pix0;
      p += (width - 1);
      *p = pix1;
    }
  }
}


void BImage::renderBevel2(Bool solid, Bool solidblack) {
  if (width > 4 && height > 4) {
    unsigned long pix0 = 0, pix1 = 0;
    unsigned char r, g, b, rr, gg, bb;

    unsigned int w = width - 2, h = height - 1, wh = width * (height - 3);
    unsigned long *p = data + width + 1;
    
    if (solid) {
      if (solidblack) {
	pix0 = 0xc0c0c0;
	pix1 = 0x606060;   
      } else {
	rr = bg_color.r * 3 / 2;
	if (rr < bg_color.r) rr = ~0;
	gg = bg_color.g * 3 / 2;
	if (gg < bg_color.g) gg = ~0;
	bb = bg_color.b * 3 / 2;
	if (bb < bg_color.b) bb = ~0;
	pix0 = ((rr << 16) | (gg << 8) | (bb));
	
	rr = bg_color.r * 3 / 4;
	if (rr > bg_color.r) rr = 0;
	gg = bg_color.g * 3 / 4;
	if (gg > bg_color.g) gg = 0;
	bb = bg_color.b * 3 / 4;
	if (bb > bg_color.b) bb = 0;
	pix1 = ((rr << 16) | (gg << 8) | (bb));
      }
      
      while (--w) {
	*p = pix0;
	*((p++) + wh) = pix1;
      }
      
      p = data + width;
      while (--h) {
	*(++p) = pix0;
	p += (width - 3);
	*(p++) = pix1;
	p++;
      }
    } else {
      while (--w) {
	r = (unsigned char) (((*p) & 0xff0000) >> 16);
	rr = r * 3 / 2;
	if (rr < r) rr = ~0;
	g = (unsigned char) (((*p) & 0x00ff00) >> 8);
	gg = g * 3 / 2;
	if (gg < g) gg = ~0;
	b = (unsigned char) ((*p) & 0x0000ff);
	bb = b * 3 / 2;
	if (bb < b) bb = ~0;
	pix0 = ((rr << 16) | (gg << 8) | (bb));
	
	r = (unsigned char) (((*(p + wh)) & 0xff0000) >> 16);
	rr = r * 3 / 4;
	if (rr > (unsigned char) r) rr = 0;
	g = (unsigned char) (((*(p + wh)) & 0x00ff00) >> 8);
	gg = g * 3 / 4;
	if (gg > (unsigned char) g) gg = 0;
	b = (unsigned char) ((*(p + wh)) & 0x0000ff);
	bb = b * 3 / 4;
	if (bb > (unsigned char) b) bb = 0;
	pix1 = ((rr << 16) | (gg << 8) | (bb));

	*p = pix0;
	*((p++) + wh) = pix1;
      }
      
      p = data + width;
      while (--h) {
	r = (unsigned char) (((*p) & 0xff0000) >> 16);
	rr = r * 3 / 2;
	if (rr < r) rr = ~0;
	g = (unsigned char) (((*p) & 0x00ff00) >> 8);
	gg = g * 3 / 2;
	if (gg < g) gg = ~0;
	b = (unsigned char) ((*p) & 0x0000ff);
	bb = b * 3 / 2;
	if (bb < b) bb = ~0;
	pix0 = ((rr << 16) | (gg << 8) | (bb));
	
	r = (unsigned char) (((*(p + (width - 3))) & 0xff0000) >> 16);
	rr = r * 3 / 4;
	if (rr > (unsigned char) r) rr = 0;
	g = (unsigned char) (((*(p + (width - 3))) & 0x00ff00) >> 8);
	gg = g * 3 / 4;
	if (gg > (unsigned char) g) gg = 0;
	b = (unsigned char) ((*(p + (width - 3))) & 0x0000ff);
	bb = b * 3 / 4;
	if (bb > (unsigned char) b) bb = 0;
	pix1 = ((rr << 16) | (gg << 8) | (bb));

	*(++p) = pix0;
	p += (width - 3);
	*(p++) = pix1;
	p++;
      }
    }
  }
}


void BImage::invertImage(void) {
  unsigned int wh = width * height;
  unsigned long *p, *pp, *new_data = new unsigned long[wh];
  pp = new_data;
  p = data + wh - 1;

  while (--wh) *(pp++) = *(p--);
  *pp = *p;
  
  delete [] data;
  data = new_data;
}


void BImage::renderDGradient(const BColor &from, const BColor &to) {
  float fr, fg, fb, tr, tg, tb, dr, dg, db,
    w = (float) width, h = (float) height,
    dx, dy, xr, xg, xb, yr, yg, yb;
  
  unsigned char r, g, b;
  unsigned int i, ii;
  unsigned long *p = data;
  
  fr = (float) from.r;
  fg = (float) from.g;
  fb = (float) from.b;
  
  tr = (float) to.r;
  tg = (float) to.g;
  tb = (float) to.b;
  
  dr = tr - fr;
  dg = tg - fg;
  db = tb - fb;

  for (i = 0; i < height; ++i) {
#ifdef GradientHack
    dy = sin((i / h) * M_PI_2);
#else
    dy = i / h;
#endif
    
    yr = (dr * dy) + fr;
    yg = (dg * dy) + fg;
    yb = (db * dy) + fb;

    for (ii = 0; ii < width; ++ii) {
#ifdef GradientHack
      dx = cos((ii / w) * M_PI_2);
#else
      dx = ii / w;
#endif
      
      xr = (dr * dx) + fr;
      xg = (dg * dx) + fg;
      xb = (db * dx) + fb;
      
      r = (unsigned char) ((xr + yr) / 2);
      g = (unsigned char) ((xg + yg) / 2);
      b = (unsigned char) ((xb + yb) / 2);
      (*(p++)) = (unsigned long) ((r << 16) | (g << 8) | (b));
    }
  }
}


void BImage::renderHGradient(const BColor &from, const BColor &to) {
  float fr, fg, fb, tr, tg, tb, dr, dg, db, dx, xr, xg, xb;
  
  fr = (float) from.r;
  fg = (float) from.g;
  fb = (float) from.b;

  tr = (float) to.r;
  tg = (float) to.g;
  tb = (float) to.b;

  dr = tr - fr;
  dg = tg - fg;
  db = tb - fb;

  unsigned long *p = data, *d = new unsigned long[width], *dd = d;
  unsigned char r = from.r, g = from.g, b = from.b;

  // this renders one line of the hgradient to a buffer...

  unsigned int ii;
  float w = (float) width;
  for (ii = 0; ii < width; ++ii) {
#ifdef GradientHack
    dx = cos((ii / w) * M_PI_2);
#else
    dx = ii / w;
#endif
    
    xr = (dr * dx) + fr;
    xg = (dg * dx) + fg;
    xb = (db * dx) + fb;
      
    r = (unsigned char) (xr);
    g = (unsigned char) (xg);
    b = (unsigned char) (xb);
    *(dd++) = (unsigned long) ((r << 16) | (g << 8) | (b));
  }

  // and this copies the buffer to the image...
  unsigned int wh = width * height;

  ii = 0;
  while (--wh) {
    *(p++) = *(d + (ii++));
    if (ii == width) ii = 0;
  }

  *p = *(d + ii);
  delete [] d;
}


void BImage::renderVGradient(const BColor &from, const BColor &to) {
  float fr, fg, fb, tr, tg, tb, dr, dg, db, dy, yr, yg, yb;
  
  fr = (float) from.r;
  fg = (float) from.g;
  fb = (float) from.b;

  tr = (float) to.r;
  tg = (float) to.g;
  tb = (float) to.b;

  dr = tr - fr;
  dg = tg - fg;
  db = tb - fb;

  unsigned long *p = data;
  unsigned char r = from.r, g = from.g, b = from.b;
  unsigned int ii;
  float h = (float) height;

  for (ii = 0; ii < height; ++ii) {
#ifdef GradientHack
    dy = sin((ii/ h) * M_PI_2);
#else
    dy = ii / h;
#endif
    
    yr = (dr * dy) + fr;
    yg = (dg * dy) + fg;
    yb = (db * dy) + fb;
      
    r = (unsigned char) (yr);
    g = (unsigned char) (yg);
    b = (unsigned char) (yb);

    unsigned int w = width;
    while (--w)
      *(p++) = (unsigned long) ((r << 16) | (g << 8) | (b));

    *(p++) = (unsigned long) ((r << 16) | (g << 8) | (b));
  }
}
