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

#include "Image.hh"
#include "Color.hh"
#include "Display.hh"
#include "Texture.hh"


bt::ImageControl::ImageControl(TimerQueueManager *app,
                               Display &_display,
                               const ScreenInfo *scrn,
                               unsigned long cache_timeout,
                               unsigned long cmax) : display(_display)
{
  cache_max = cmax;
  if (cache_timeout) {
    timer = new Timer(app, this);
    timer->setTimeout(cache_timeout);
    timer->start();
  } else {
    timer = (bt::Timer *) 0;
  }
  window = scrn->rootWindow();
  colormap = scrn->colormap();
  screen = scrn->screenNumber();
}


bt::ImageControl::~ImageControl(void) {
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
    const unsigned long pixel1 =
      c1.red() << 24 | c1.green() << 16 | c1.blue();
    const unsigned long pixel2 =
      c2.red() << 24 | c2.green() << 16 | c2.blue();
    if (tmp.width == width && tmp.height == height &&
        tmp.texture == texture && tmp.pixel1 == pixel1)
      if (texture & bt::Texture::Gradient) {
        if (tmp.pixel2 == pixel2) {
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

  bt::Image image(width, height);
  pixmap = image.render(display, screen, texture);

  if (!pixmap)
    return None;

  CachedImage tmp;

  tmp.pixmap = pixmap;
  tmp.width = width;
  tmp.height = height;
  tmp.count = 1;
  tmp.texture = texture.texture();

  const Color &c1 = texture.color();
  const Color &c2 = texture.colorTo();
  const unsigned long pixel1 =
    c1.red() << 24 | c1.green() << 16 | c1.blue();
  const unsigned long pixel2 =
    c2.red() << 24 | c2.green() << 16 | c2.blue();

  tmp.pixel1 = pixel1;

  if (texture.texture() & bt::Texture::Gradient)
    tmp.pixel2 = pixel2;
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
