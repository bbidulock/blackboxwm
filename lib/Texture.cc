// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Texture.cc for Blackbox - an X11 Window manager
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#include <stdio.h>
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
}

#include <assert.h>

#include "Texture.hh"

#include "BaseDisplay.hh"
#include "Image.hh"


bt::Texture::Texture(const BaseDisplay * const _display,
                     unsigned int _screen, BImageControl* _ctrl)
  : c(_display, _screen), ct(_display, _screen),
    lc(_display, _screen), sc(_display, _screen), t(0),
    dpy(_display), ctrl(_ctrl), scrn(_screen) { }


bt::Texture::Texture(const std::string &d, const BaseDisplay * const _display,
                     unsigned int _screen, BImageControl* _ctrl)
  : c(_display, _screen), ct(_display, _screen),
    lc(_display, _screen), sc(_display, _screen), t(0),
    dpy(_display), ctrl(_ctrl), scrn(_screen) {
  setDescription(d);
}


void bt::Texture::setColor(const bt::Color &cc) {
  c = cc;
  c.setDisplay(display(), screen());

  unsigned char r, g, b, rr, gg, bb;

  // calculate the light color
  r = c.red();
  g = c.green();
  b = c.blue();
  rr = r + (r >> 1);
  gg = g + (g >> 1);
  bb = b + (b >> 1);
  if (rr < r) rr = ~0;
  if (gg < g) gg = ~0;
  if (bb < b) bb = ~0;
  lc = bt::Color(rr, gg, bb, display(), screen());

  // calculate the shadow color
  r = c.red();
  g = c.green();
  b = c.blue();
  rr = (r >> 2) + (r >> 1);
  gg = (g >> 2) + (g >> 1);
  bb = (b >> 2) + (b >> 1);
  if (rr > r) rr = 0;
  if (gg > g) gg = 0;
  if (bb > b) bb = 0;
  sc = bt::Color(rr, gg, bb, display(), screen());
}


void bt::Texture::setDescription(const std::string &d) {
  descr.erase();
  descr.reserve(d.length());

  std::string::const_iterator it = d.begin(), end = d.end();
  for (; it != end; ++it)
    descr += tolower(*it);

  if (descr.find("parentrelative") != std::string::npos) {
    setTexture(bt::Texture::Parent_Relative);
  } else {
    setTexture(0);

    if (descr.find("gradient") != std::string::npos) {
      addTexture(bt::Texture::Gradient);
      if (descr.find("crossdiagonal") != std::string::npos)
        addTexture(bt::Texture::CrossDiagonal);
      else if (descr.find("rectangle") != std::string::npos)
        addTexture(bt::Texture::Rectangle);
      else if (descr.find("pyramid") != std::string::npos)
        addTexture(bt::Texture::Pyramid);
      else if (descr.find("pipecross") != std::string::npos)
        addTexture(bt::Texture::PipeCross);
      else if (descr.find("elliptic") != std::string::npos)
        addTexture(bt::Texture::Elliptic);
      else if (descr.find("horizontal") != std::string::npos)
        addTexture(bt::Texture::Horizontal);
      else if (descr.find("vertical") != std::string::npos)
        addTexture(bt::Texture::Vertical);
      else
        addTexture(bt::Texture::Diagonal);
    } else {
      addTexture(bt::Texture::Solid);
    }

    if (descr.find("sunken") != std::string::npos)
      addTexture(bt::Texture::Sunken);
    else if (descr.find("flat") != std::string::npos)
      addTexture(bt::Texture::Flat);
    else
      addTexture(bt::Texture::Raised);

    if (! (texture() & bt::Texture::Flat)) {
      if (descr.find("bevel2") != std::string::npos)
        addTexture(bt::Texture::Bevel2);
      else
        addTexture(bt::Texture::Bevel1);
    }

    if (descr.find("interlaced") != std::string::npos)
      addTexture(bt::Texture::Interlaced);
  }
}

void bt::Texture::setDisplay(const BaseDisplay * const _display,
                          const unsigned int _screen) {
  if (_display == display() && _screen == screen()) {
    // nothing to do
    return;
  }

  dpy = _display;
  scrn = _screen;
  c.setDisplay(_display, _screen);
  ct.setDisplay(_display, _screen);
  lc.setDisplay(_display, _screen);
  sc.setDisplay(_display, _screen);
}


bt::Texture& bt::Texture::operator=(const bt::Texture &tt) {
  c  = tt.c;
  ct = tt.ct;
  lc = tt.lc;
  sc = tt.sc;
  descr = tt.descr;
  t  = tt.t;
  dpy = tt.dpy;
  scrn = tt.scrn;
  ctrl = tt.ctrl;

  return *this;
}


Pixmap bt::Texture::render(const unsigned int width, const unsigned int height,
                        const Pixmap old) {
  assert(display() != 0);

  if (texture() == (bt::Texture::Flat | bt::Texture::Solid))
    return None;
  if (texture() == bt::Texture::Parent_Relative)
    return ParentRelative;

  if (screen() == ~(0u))
    scrn = DefaultScreen(display()->getXDisplay());

  assert(ctrl != 0);
  Pixmap ret = ctrl->renderImage(width, height, *this);

  if (old)
    ctrl->removeImage(old);

  return ret;
}
