// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// bsetroot - a background setting utility
// Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh at debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2005
//         Bradley T Hughes <bhughes at trolltech.com>
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

#include "bsetroot.hh"

#include <Pen.hh>
#include <Texture.hh>

#include <cctype>

#include <X11/Xatom.h>
#include <stdio.h>


// ignore all X errors
static int x11_error(::Display *, XErrorEvent *)
{ return 0; }


bsetroot::bsetroot(int argc, char **argv, char *dpy_name,
                   bool multi_head): display(dpy_name, multi_head) {
  XSetErrorHandler(x11_error); // silently handle all errors

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
      // -display passed through tests ealier... we
      i++;
    } else {
      usage();
    }
  }

  if ((mod + sol + grd) != True) {
    fprintf(stderr,
            "bsetroot: error: must specify one of: -solid, -mod, -gradient\n");

    usage(2);
  }

  // keep the server grabbed while rendering all screens
  XGrabServer(display.XDisplay());

  if (sol && ! fore.empty())
    solid();
  else if (mod && mod_x && mod_y && ! (fore.empty() || back.empty()))
    modula(mod_x, mod_y);
  else if (grd && ! (grad.empty() || fore.empty() || back.empty()))
    gradient();
  else
    usage();

  // ungrab the server and discard any events
  XUngrabServer(display.XDisplay());
  XSync(display.XDisplay(), True);
}


bsetroot::~bsetroot(void) {
  XSetCloseDownMode(display.XDisplay(), RetainPermanent);
  XKillClient(display.XDisplay(), AllTemporary);
}


// adapted from wmsetbg
void bsetroot::setPixmapProperty(int screen, Pixmap pixmap) {
  Atom rootpmap_id = XInternAtom(display.XDisplay(), "_XROOTPMAP_ID", False),
       esetroot_id = XInternAtom(display.XDisplay(), "ESETROOT_PMAP_ID", False);

  const bt::ScreenInfo &screen_info = display.screenInfo(screen);

  Atom type;
  int format;
  unsigned long length, after;
  unsigned char *data = 0;
  Pixmap xrootpmap = None;
  Pixmap esetrootpmap = None;

  // Clear out the old _XROOTPMAP_ID property
  XGetWindowProperty(display.XDisplay(), screen_info.rootWindow(),
		     rootpmap_id, 0L, 1L, True, AnyPropertyType,
                     &type, &format, &length, &after, &data);

  if (data && type == XA_PIXMAP && format == 32 && length == 1)
    xrootpmap = *(reinterpret_cast<Pixmap *>(data));

  if (data) XFree(data);

  // Clear out the old ESETROOT_PMAP_ID property
  XGetWindowProperty(display.XDisplay(), screen_info.rootWindow(),
                     esetroot_id, 0L, 1L, True, AnyPropertyType,
                     &type, &format, &length, &after, &data);

  if (data && type == XA_PIXMAP && format == 32 && length == 1)
    esetrootpmap = *(reinterpret_cast<Pixmap *>(data));

  if (data) XFree(data);

  // Destroy the old pixmaps
  if (xrootpmap)
    XKillClient(display.XDisplay(), xrootpmap);
  if (esetrootpmap && esetrootpmap != xrootpmap)
    XKillClient(display.XDisplay(), esetrootpmap);

  if (pixmap) {
    XChangeProperty(display.XDisplay(), screen_info.rootWindow(),
		    rootpmap_id, XA_PIXMAP, 32, PropModeReplace,
		    reinterpret_cast<unsigned char *>(&pixmap), 1);
    XChangeProperty(display.XDisplay(), screen_info.rootWindow(),
		    esetroot_id, XA_PIXMAP, 32, PropModeReplace,
		    reinterpret_cast<unsigned char *>(&pixmap), 1);
  }
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
  const bt::ScreenInfo &screen_info = display.screenInfo(screen);
  unsigned long pixel = color.pixel(screen);
  XColor xcolor;
  xcolor.pixel = pixel;
  XQueryColor(display.XDisplay(), screen_info.colormap(), &xcolor);
  if (! XAllocColor(display.XDisplay(), screen_info.colormap(),
                    &xcolor)) {
    fprintf(stderr, "warning: couldn't duplicate color %02x/%02x/%02x\n",
            color.red(), color.green(), color.blue());
  }
  return pixel;
}


void bsetroot::solid(void) {
  bt::Color c = bt::Color::namedColor(display, 0, fore);

  for (unsigned int screen = 0; screen < display.screenCount(); screen++) {
    const bt::ScreenInfo &screen_info = display.screenInfo(screen);
    unsigned long pixel = duplicateColor(screen, c);

    XSetWindowBackground(display.XDisplay(), screen_info.rootWindow(), pixel);
    XClearWindow(display.XDisplay(), screen_info.rootWindow());

    Pixmap pixmap =
      XCreatePixmap(display.XDisplay(), screen_info.rootWindow(),
                    8, 8, DefaultDepth(display.XDisplay(), screen));

    bt::Pen pen(screen, c);
    XFillRectangle(display.XDisplay(), pixmap, pen.gc(), 0, 0, 8, 8);

    setPixmapProperty(screen, pixmap);
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

    const bt::ScreenInfo &screen_info = display.screenInfo(screen);

    XGCValues gcv;
    gcv.foreground = duplicateColor(screen, f);
    gcv.background = duplicateColor(screen, b);

    GC gc = XCreateGC(display.XDisplay(), screen_info.rootWindow(),
                      GCForeground | GCBackground, &gcv);

    Pixmap pixmap = XCreatePixmap(display.XDisplay(),
				  screen_info.rootWindow(),
				  16, 16, screen_info.depth());

    Pixmap bitmap =
      XCreateBitmapFromData(display.XDisplay(), screen_info.rootWindow(),
                            data, 16, 16);
    XCopyPlane(display.XDisplay(), bitmap, pixmap, gc,
               0, 0, 16, 16, 0, 0, 1l);
    XFreeGC(display.XDisplay(), gc);
    XFreePixmap(display.XDisplay(), bitmap);

    XSetWindowBackgroundPixmap(display.XDisplay(),
                               screen_info.rootWindow(),
                               pixmap);
    XClearWindow(display.XDisplay(), screen_info.rootWindow());

    setPixmapProperty(screen, pixmap);
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
  texture.setColor1(f);
  texture.setColor2(b);

  for (unsigned int screen = 0; screen < display.screenCount(); screen++) {
    const bt::ScreenInfo &screen_info = display.screenInfo(screen);

    bt::Image image(screen_info.width(), screen_info.height());
    Pixmap pixmap = image.render(display, screen, texture);

    XSetWindowBackgroundPixmap(display.XDisplay(),
                               screen_info.rootWindow(),
                               pixmap);
    XClearWindow(display.XDisplay(), screen_info.rootWindow());

    setPixmapProperty(screen, pixmap);
  }
}


void bsetroot::usage(int exit_code) {
  fprintf(stderr,
          "bsetroot 3.1\n\n"
          "Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry\n"
          "Copyright (c) 1997 - 2000, 2002 - 2005 Bradley T Hughes\n");
  fprintf(stderr,
          "  -display <string>        use display connection\n"
          "  -mod <x> <y>             modula pattern\n"
          "  -foreground, -fg <color> modula foreground color\n"
          "  -background, -bg <color> modula background color\n\n"
          "  -gradient <texture>      gradient texture\n"
          "  -from <color>            gradient start color\n"
          "  -to <color>              gradient end color\n\n"
          "  -solid <color>           solid color\n\n"
          "  -help                    print this help text and exit\n");
  exit(exit_code);
}

int main(int argc, char **argv) {
  char *display_name = 0;
  bool multi_head = False;

  for (int i = 1; i < argc; i++) {
    if (! strcmp(argv[i], "-display")) {
      // check for -display option

      if ((++i) >= argc) {
        fprintf(stderr, "error: '-display' requires an argument\n");

        ::exit(1);
      }

      display_name = argv[i];
    } else if (! strcmp(argv[i], "-multi")) {
      multi_head = True;
    }
  }

  bsetroot app(argc, argv, display_name, multi_head);
  return 0;
}
