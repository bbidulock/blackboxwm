/*
 * bsetroot.c for Blackbox 0.50.x beta - an X11 Window manager
 * Copyright (c) 1997 - 1999 by Brad Hughes, bhughes@tcac.net
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   (See the included file COPYING / GPL-2.0)
 *
 * Part of this code were derived from xsetroot. The Copyright for xsetroot is
 * included below.
 *
 */

/*
 * $XConsortium: xsetroot.c,v 1.23 94/04/17 20:24:34 hersh Exp $
 *
Copyright (c) 1987,  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rightsto use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software isfurnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
 */

/*
 * xsetroot.c   MIT Project Athena, X Window System root window
 *              parameter setting utility.  This program will set
 *              various parameters of the X root window.
 *
 *  Author:     Mark Lillibridge, MIT Project Athena
 *              11-Jun-87
 */
/* $XFree86: xc/programs/xsetroot/xsetroot.c,v 1.2 1996/12/28 08:22:33 dawes Exp
 $ */


/* Includes */

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#ifdef STDC_HEADERS
#  include <stdlib.h>
#endif

#ifdef HAVE_STDIO_H
#  include <stdio.h>
#endif

#ifdef HAVE_MALLOC_H
#  include <malloc.h>
#endif


/* Global data */

Bool unsave_past = False, save_colors = False;
Display *display;
Pixmap *save_pixmap = (Pixmap *) 0;

char *program_name, *fore_color = NULL, *back_color = NULL;
int number_of_screens;


/* Function definitions */

void usage(void) {
    fprintf(stderr,
	    "%s 1.0 : (c) 1998 Brad Hughes\n\n"
	    "  -display <string>\tuse display connection\n"
	    "  -fg <color>\t\tuse foreground color\n"
	    "  -bg <color>\t\tuse background color\n"
	    "  -help\t\t\tprint this help text and exit\n"
	    "  -solid <color>\tuse solid color\n"
	    "  -mod <x> <y>\t\tuse modula pattern\n", program_name);
    
    exit(1);
}


Pixmap getModulaBitmap(int screen, int x, int y) {
  char data[32];
  long pattern = 0;

  register int i;

  for (i = 0; i < 16; i++) {
    pattern <<= 1;
    if ((i % x) == 0)
      pattern |= 0x0001;
  }

  for (i = 0; i < 16; i++) {
    if ((i % y) == 0) {
      data[(i * 2)] = (char)0xff;
      data[(i * 2) + 1] = (char)0xff;
    } else {
      data[(i * 2)] = pattern & 0xff;
      data[(i * 2) + 1] = (pattern >> 8) & 0xff;
    }
  }
  
  return XCreateBitmapFromData(display, RootWindow(display, screen),
                               data, 16, 16);
}


unsigned long getColor(int screen, const char *colorname, unsigned long
		       default_pixel) {
  XColor color;
  XWindowAttributes attributes;

  XGetWindowAttributes(display, RootWindow(display, screen), &attributes);
  color.pixel = default_pixel;

  if (!XParseColor(display, attributes.colormap, colorname, &color)) {
    fprintf(stderr, "%s: getColor: color parse error: \"%s\"\n",
            program_name, colorname);
  } else if (!XAllocColor(display, attributes.colormap, &color)) {
    fprintf(stderr, "%s: getColor: color alloc error: \"%s\"\n",
            program_name, colorname);
  }

  if ((color.pixel != BlackPixel(display, screen)) &&
      (color.pixel != WhitePixel(display, screen)) &&
      (DefaultVisual(display, screen)->class & 1))
    save_colors = 1;
  
  return color.pixel;
}


void cleanup(void) {
  Atom prop, type;
  Bool unsave_past_for_screen;
  int format, i;
  unsigned long length, after;
  unsigned char *data;

  prop = XInternAtom(display, "_XSETROOT_ID", False);
  
  for (i = 0; i < number_of_screens; i++) {
    unsave_past_for_screen = unsave_past;

    if (! (DefaultVisual(display, i)->class & 1))
      unsave_past_for_screen = 0;
    
    if ((! unsave_past_for_screen) && (! save_colors))
      continue;
  
    if (unsave_past_for_screen) {
      XGetWindowProperty(display, RootWindow(display, i), prop, 0L, 1L, True,
			 AnyPropertyType, &type, &format, &length,
			 &after, &data);
      
      if ((type == XA_PIXMAP) && (format == 32) &&
	  (length == 1) && (after == 0))
	XKillClient(display, *((Pixmap *) data));
      else if (type != None)
	fprintf(stderr, "%s: warning: _XSETROOT_ID property is garbage\n",
		program_name);
    }

    if (save_colors) {
      if (! save_pixmap[i])
	save_pixmap[i] = XCreatePixmap(display, RootWindow(display, i),
				       1, 1, 1);
      
      XChangeProperty(display, RootWindow(display, i), prop, XA_PIXMAP, 32,
		      PropModeReplace, (unsigned char *) &save_pixmap, 1);
    }
  }

  XSetCloseDownMode(display, RetainPermanent);
}


int main(int argc, char **argv) {
  Bool modula = False, solid = False;
  char *display_name = NULL;
  char *solid_color = NULL;
  int mod_x = 0;
  int mod_y = 0;

  register int i;

  program_name=argv[0];
  
  for (i = 1; i < argc; i++) {
    if (! strcmp ("-display", argv[i])) {
      if ((++i) >= argc)
	usage();

      display_name = argv[i];
    } else if (!strcmp("-help", argv[i])) {
      usage();
    } else if ((! strcmp("-fg",argv[i])) ||
	       (! strcmp("-foreground",argv[i]))) {
      if ((++i) >= argc)
	usage();

      fore_color = argv[i];
    } else if ((! strcmp("-bg",argv[i])) ||
	       (! strcmp("-background",argv[i]))) {
      if ((++i) >= argc)
	usage();

      back_color = argv[i];
    } else if (!strcmp("-solid", argv[i])) {
      if ((++i) >= argc)
	usage();

      solid_color = argv[i];
      solid = True;
    } else if (!strcmp("-mod", argv[i])) {
      if ((++i) >= argc)
	usage();

      mod_x = atoi(argv[i]);
      if (mod_x <= 0) mod_x = 1;

      if (++i>=argc)
	usage();

      mod_y = atoi(argv[i]);
      if (mod_y <= 0) mod_y = 1;

      modula = True;
    } else
      usage();
  } 
  
  if (solid && modula) {
    fprintf(stderr, "%s: cannot perform both -solid and -mod options\n",
	    program_name);
    usage();
  }
  
  display = XOpenDisplay(display_name);
  if (! display) {
    fprintf(stderr, "%s:  unable to open display '%s'\n",
	    program_name, XDisplayName (display_name));

    exit(2);
  }
  
  number_of_screens = ScreenCount(display);
  save_pixmap = (Pixmap *) malloc(number_of_screens * sizeof(Pixmap));
  if (! save_pixmap) {
    fprintf(stderr, "%s: internal error, not enough free memory\n",
	    program_name);

    exit(1);
  }
  
  if (solid && solid_color) {
    for (i = 0; i < number_of_screens; i++) {
      XSetWindowBackground(display, RootWindow(display, i),
			   getColor(i, solid_color, BlackPixel(display, i)));
      XClearWindow(display, RootWindow(display, i));
    }
    
    unsave_past = True;
  } else if (modula && mod_x && mod_y) {
    Pixmap bitmap, pixmap;
    GC gc;
    XGCValues gcv;
    int i;

    for (i = 0; i < number_of_screens; i++) {
      bitmap = getModulaBitmap(i, mod_x, mod_y);

      gcv.foreground = getColor(i, fore_color, WhitePixel(display, i));
      gcv.background = getColor(i, back_color, BlackPixel(display, i));

      gc = XCreateGC(display, RootWindow(display, i),
		     GCForeground | GCBackground, &gcv);

      pixmap = XCreatePixmap(display, RootWindow(display, i), 16, 16,
			     (unsigned int) DefaultDepth(display, i));

      XCopyPlane(display, bitmap, pixmap, gc, 0, 0, 16, 16, 0, 0,
		 (unsigned long) 1);
      XSetWindowBackgroundPixmap(display, RootWindow(display, i), pixmap);

      XFreeGC(display, gc);
      XFreePixmap(display, bitmap);
      
      if (save_colors) {
        save_pixmap[i] = pixmap;
      } else {
	save_pixmap[i] = (Pixmap) None;
        XFreePixmap(display, pixmap);
      }
      
      XClearWindow(display, RootWindow(display, i));
      unsave_past = True;
    }
  } else
    fprintf(stderr, "%s: no options given, cleaning up and exiting.\n",
	    program_name);
  
  cleanup();

  free(save_pixmap);

  XCloseDisplay(display);

  return 0;
}
