// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Window.cc for Blackbox - an X11 Window manager
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

#include <cctype>

#include "bsetroot.hh"
#include "Pen.hh"
#include "Texture.hh"
#include "i18n.hh"


bt::I18n bt::i18n;

bsetroot::bsetroot(int argc, char **argv, char *dpy_name): display(dpy_name) {
  bool mod = False, sol = False, grd = False;
  int mod_x = 0, mod_y = 0;

  for (int i = 1; i < argc; i++) {
    if (! strcmp("-help", argv[i])) {
      usage();
    } else if ((! strcmp("-fg", argv[i])) ||
               (! strcmp("-foreground", argv[i])) ||
               (! strcmp("-from", argv[i]))) {
      if ((++i) >= argc) usage(1);

      fore = argv[i];
    } else if ((! strcmp("-bg", argv[i])) ||
               (! strcmp("-background", argv[i])) ||
               (! strcmp("-to", argv[i]))) {
      if ((++i) >= argc) usage(1);

      back = argv[i];
    } else if (! strcmp("-solid", argv[i])) {
      if ((++i) >= argc) usage(1);

      fore = argv[i];
      sol = True;
    } else if (! strcmp("-mod", argv[i])) {
      if ((++i) >= argc) usage();

      mod_x = atoi(argv[i]);

      if ((++i) >= argc) usage();

      mod_y = atoi(argv[i]);

      if (mod_x < 1) mod_x = 1;
      if (mod_y < 1) mod_y = 1;

      mod = True;
    } else if (! strcmp("-gradient", argv[i])) {
      if ((++i) >= argc) usage();

      grad = argv[i];
      grd = True;
    } else if (! strcmp("-display", argv[i])) {
      // -display passed through tests ealier... we just skip it now
      i++;
    } else {
      usage();
    }
  }

  if ((mod + sol + grd) != True) {
    fprintf(stderr, bt::i18n(bsetrootSet, bsetrootMustSpecify,
           "bsetroot: error: must specify one of: -solid, -mod, -gradient\n"));

    usage(2);
  }

  if (sol && ! fore.empty())
    solid();
  else if (mod && mod_x && mod_y && ! (fore.empty() || back.empty()))
    modula(mod_x, mod_y);
  else if (grd && ! (grad.empty() || fore.empty() || back.empty()))
    gradient();
  else usage();
}


bsetroot::~bsetroot(void) {
  XSetCloseDownMode(display.XDisplay(), RetainPermanent);
  XKillClient(display.XDisplay(), AllTemporary);
}


// adapted from wmsetbg
void bsetroot::setPixmapProperty(int screen, Pixmap pixmap) {
  static Atom rootpmap_id = None, esetroot_id = None;
  Atom type;
  int format;
  unsigned long length, after;
  unsigned char *data;
  const bt::ScreenInfo * const screen_info = display.screenNumber(screen);

  if (rootpmap_id == None) {
    rootpmap_id = XInternAtom(display.XDisplay(), "_XROOTPMAP_ID", False);
    esetroot_id = XInternAtom(display.XDisplay(), "ESETROOT_PMAP_ID", False);
  }

  XGrabServer(display.XDisplay());

  /* Clear out the old pixmap */
  XGetWindowProperty(display.XDisplay(), screen_info->getRootWindow(),
		     rootpmap_id, 0L, 1L, False, AnyPropertyType,
		     &type, &format, &length, &after, &data);

  if ((type == XA_PIXMAP) && (format == 32) && (length == 1)) {
    unsigned char* data_esetroot = 0;
    XGetWindowProperty(display.XDisplay(), screen_info->getRootWindow(),
                       esetroot_id, 0L, 1L, False, AnyPropertyType,
                       &type, &format, &length, &after, &data_esetroot);
    if (data && data_esetroot && *((Pixmap *) data)) {
      XKillClient(display.XDisplay(), *((Pixmap *) data));
      XSync(display.XDisplay(), False);
      XFree(data_esetroot);
    }
    XFree(data);
  }

  if (pixmap) {
    XChangeProperty(display.XDisplay(), screen_info->getRootWindow(),
		    rootpmap_id, XA_PIXMAP, 32, PropModeReplace,
		    (unsigned char *) &pixmap, 1);
    XChangeProperty(display.XDisplay(), screen_info->getRootWindow(),
		    esetroot_id, XA_PIXMAP, 32, PropModeReplace,
		    (unsigned char *) &pixmap, 1);
  } else {
    XDeleteProperty(display.XDisplay(), screen_info->getRootWindow(),
		    rootpmap_id);
    XDeleteProperty(display.XDisplay(), screen_info->getRootWindow(),
		    esetroot_id);
  }

  XUngrabServer(display.XDisplay());
  XFlush(display.XDisplay());
}


// adapted from wmsetbg
Pixmap bsetroot::duplicatePixmap(int screen, Pixmap pixmap,
				 int width, int height) {
  XSync(display.XDisplay(), False);

  Pixmap copyP = XCreatePixmap(display.XDisplay(),
			       display.screenNumber(screen)->getRootWindow(),
			       width, height,
			       DefaultDepth(display.XDisplay(), screen));
  XCopyArea(display.XDisplay(), pixmap, copyP,
            DefaultGC(display.XDisplay(), screen),
	    0, 0, width, height, 0, 0);
  XSync(display.XDisplay(), False);

  return copyP;
}


unsigned long bsetroot::duplicateColor(unsigned int screen,
                                       const bt::Color &color) {
  /*
    When using a colormap that is not read-only, we need to
    reallocate the color we are using.  The application's color cache
    will be freed on exit, and another client can allocate a new
    color at the same pixel value, which will immediately change the
    color of the root window.

    this is not what we want, so we need to make sure that the pixel
    we are using is doubly allocated, so that it stays around with the
    pixmap.  It will be released when we use
    XKillClient(..., AllTemporary);
  */
  const bt::ScreenInfo * const screen_info = display.screenNumber(screen);
  unsigned long pixel = color.pixel(screen);
  XColor xcolor;
  xcolor.pixel = pixel;
  XQueryColor(display.XDisplay(), screen_info->getColormap(), &xcolor);
  if (! XAllocColor(display.XDisplay(), screen_info->getColormap(),
                    &xcolor)) {
    fprintf(stderr, "warning: couldn't duplicate color %02x/%02x/%02x\n",
            color.red(), color.green(), color.blue());
  }
  return pixel;
}


void bsetroot::solid(void) {
  bt::Color c = bt::Color::namedColor(display, 0, fore);

  for (unsigned int screen = 0; screen < display.screenCount(); screen++) {
    const bt::ScreenInfo * const screen_info = display.screenNumber(screen);
    unsigned long pixel = duplicateColor(screen, c);

    XSetWindowBackground(display.XDisplay(), screen_info->getRootWindow(),
                         pixel);
    XClearWindow(display.XDisplay(), screen_info->getRootWindow());

    Pixmap pixmap =
      XCreatePixmap(display.XDisplay(), screen_info->getRootWindow(),
                    8, 8, DefaultDepth(display.XDisplay(), screen));

    bt::Pen pen(screen, c);
    XFillRectangle(display.XDisplay(), pixmap, pen.gc(), 0, 0, 8, 8);

    setPixmapProperty(screen, duplicatePixmap(screen, pixmap, 8, 8));

    XFreePixmap(display.XDisplay(), pixmap);
  }
}


void bsetroot::modula(int x, int y) {
  char data[32];
  long pattern;

  unsigned int screen, i;

  bt::Color f = bt::Color::namedColor(display, 0, fore),
            b = bt::Color::namedColor(display, 0, back);

  for (pattern = 0, screen = 0; screen < display.screenCount(); screen++) {
    for (i = 0; i < 16; i++) {
      pattern <<= 1;
      if ((i % x) == 0)
        pattern |= 0x0001;
    }

    for (i = 0; i < 16; i++) {
      if ((i %  y) == 0) {
        data[(i * 2)] = static_cast<char>(0xff);
        data[(i * 2) + 1] = static_cast<char>(0xff);
      } else {
        data[(i * 2)] = pattern & 0xff;
        data[(i * 2) + 1] = (pattern >> 8) & 0xff;
      }
    }

    GC gc;
    Pixmap bitmap;
    const bt::ScreenInfo * const screen_info = display.screenNumber(screen);

    bitmap =
      XCreateBitmapFromData(display.XDisplay(), screen_info->getRootWindow(),
                            data, 16, 16);

    XGCValues gcv;
    gcv.foreground = duplicateColor(screen, f);
    gcv.background = duplicateColor(screen, b);

    gc = XCreateGC(display.XDisplay(), screen_info->getRootWindow(),
                   GCForeground | GCBackground, &gcv);

    Pixmap pixmap = XCreatePixmap(display.XDisplay(),
				  screen_info->getRootWindow(),
				  16, 16, screen_info->getDepth());

    XCopyPlane(display.XDisplay(), bitmap, pixmap, gc,
               0, 0, 16, 16, 0, 0, 1l);
    XSetWindowBackgroundPixmap(display.XDisplay(),
                               screen_info->getRootWindow(),
                               pixmap);
    XClearWindow(display.XDisplay(), screen_info->getRootWindow());

    setPixmapProperty(screen,
		      duplicatePixmap(screen, pixmap, 16, 16));

    XFreeGC(display.XDisplay(), gc);
    XFreePixmap(display.XDisplay(), bitmap);

    if (! (screen_info->getVisual()->c_class & 1))
      XFreePixmap(display.XDisplay(), pixmap);
  }
}


void bsetroot::gradient(void) {
  /*
    we have to be sure that neither raised nor sunken is specified otherwise
    odd looking borders appear.  So we convert to lowercase then look for
    'raised' or 'sunken' in the description and erase them.  To be paranoid
    the search is done in a loop.
  */
  std::string descr;
  descr.reserve(grad.size());

  std::string::const_iterator it = grad.begin(), end = grad.end();
  for (; it != end; ++it)
    descr += tolower(*it);

  std::string::size_type pos;
  while ((pos = descr.find("raised")) != std::string::npos)
    descr.erase(pos, 6); // 6 is strlen raised

  while ((pos = descr.find("sunken")) != std::string::npos)
    descr.erase(pos, 6);

  // now add on 'flat' to prevent the bevels from being added
  descr += "flat";

  bt::Color f = bt::Color::namedColor(display, 0, fore);
  bt::Color b = bt::Color::namedColor(display, 0, back);

  bt::Texture texture;
  texture.setDescription(descr);
  texture.setColor(f);
  texture.setColorTo(b);

  for (unsigned int screen = 0; screen < display.screenCount(); screen++) {
    const bt::ScreenInfo * const screen_info = display.screenNumber(screen);

    bt::Image image(screen_info->getWidth(), screen_info->getHeight());
    Pixmap pixmap = image.render(display, screen, texture);

    XSetWindowBackgroundPixmap(display.XDisplay(),
                               screen_info->getRootWindow(),
                               pixmap);
    XClearWindow(display.XDisplay(), screen_info->getRootWindow());

    setPixmapProperty(screen,
		      duplicatePixmap(screen, pixmap,
				      screen_info->getWidth(),
				      screen_info->getHeight()));

    if (! (screen_info->getVisual()->c_class & 1))
      XFreePixmap(display.XDisplay(), pixmap);
  }
}


void bsetroot::usage(int exit_code) {
  fprintf(stderr,
          "bsetroot 3.0\n\n"
          "Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry\n"
          "Copyright (c) 1997 - 2000, 2002 Bradley T Hughes\n");
  fprintf(stderr,
          bt::i18n(bsetrootSet, bsetrootUsage,
               "  -display <string>        use display connection\n"
               "  -mod <x> <y>             modula pattern\n"
               "  -foreground, -fg <color> modula foreground color\n"
               "  -background, -bg <color> modula background color\n\n"
               "  -gradient <texture>      gradient texture\n"
               "  -from <color>            gradient start color\n"
               "  -to <color>              gradient end color\n\n"
               "  -solid <color>           solid color\n\n"
               "  -help                    print this help text and exit\n"));
  exit(exit_code);
}

int main(int argc, char **argv) {
  char *display_name = (char *) 0;

  bt::i18n.openCatalog("blackbox.cat");

  for (int i = 1; i < argc; i++) {
    if (! strcmp(argv[i], "-display")) {
      // check for -display option

      if ((++i) >= argc) {
        fprintf(stderr, bt::i18n(mainSet, mainDISPLAYRequiresArg,
		             "error: '-display' requires an argument\n"));

        ::exit(1);
      }

      display_name = argv[i];
    }
  }

  bsetroot app(argc, argv, display_name);

  return 0;
}
