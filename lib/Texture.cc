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
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
}

#include "Texture.hh"
#include "BaseDisplay.hh"
#include "Image.hh"
#include "Resource.hh"


bt::Texture::Texture(void) : t(0ul), bw(0u) { }

bt::Texture::Texture(const bt::Texture& tt) {
  *this = tt;
}


void bt::Texture::setColor(const bt::Color &new_color) {
  c = new_color;

  unsigned char r, g, b, rr, gg, bb;
  r = c.red();
  g = c.green();
  b = c.blue();

  // calculate the light color
  rr = r + (r >> 1);
  gg = g + (g >> 1);
  bb = b + (b >> 1);
  if (rr < r) rr = ~0;
  if (gg < g) gg = ~0;
  if (bb < b) bb = ~0;
  lc.setRGB(rr, gg, bb);

  // calculate the shadow color
  rr = (r >> 2) + (r >> 1);
  gg = (g >> 2) + (g >> 1);
  bb = (b >> 2) + (b >> 1);
  if (rr > r) rr = 0;
  if (gg > g) gg = 0;
  if (bb > b) bb = 0;
  sc.setRGB(rr, gg, bb);
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

    if (descr.find("interlaced") != std::string::npos)
      addTexture(bt::Texture::Interlaced);

    if (descr.find("border") != std::string::npos)
      addTexture(bt::Texture::Border);
  }
}


bt::Texture& bt::Texture::operator=(const bt::Texture &tt) {
  descr = tt.descr;

  c  = tt.c;
  ct = tt.ct;
  bc = tt.bc;
  lc = tt.lc;
  sc = tt.sc;
  t  = tt.t;
  bw = tt.bw;
  return *this;
}


Pixmap bt::Texture::render(const Display &display, unsigned int screen,
                           ImageControl &image_control, // go away!
                           unsigned int width, unsigned int height,
                           Pixmap old) {
  Pixmap ret;
  if (texture() == (bt::Texture::Flat | bt::Texture::Solid)) {
    ret = None;
  } else if (texture() == bt::Texture::Parent_Relative) {
    ret = ParentRelative;
  } else {
    ret = image_control.renderImage(width, height, *this);
    // ret = bt::Image(width, height).render(display, screen, *this);
  }

  if (old) image_control.removeImage(old);

  return ret;
}

bt::Texture bt::textureResource(const Display &display,
                                unsigned int screen,
                                const bt::Resource &resource,
                                const std::string &name,
                                const std::string &class_name,
                                const std::string &default_color) {
  Texture texture;
  texture.setDescription(resource.read(name, class_name, "flat solid"));

  Color c1, c2, c3;
  c1 = Color::namedColor(display, screen,
                         resource.read(name + ".color",
                                       class_name + ".Color",
                                       default_color));
  c2 = Color::namedColor(display, screen,
                         resource.read(name + ".colorTo",
                                       class_name + ".ColorTo",
                                       default_color));

  texture.setColor(c1);
  texture.setColorTo(c2);

  if (texture.texture() & bt::Texture::Border) {
    c3 = Color::namedColor(display, screen,
                           resource.read(name + ".borderColor",
                                         class_name + ".BorderColor",
                                         default_color));
    texture.setBorderColor(c3);

    const std::string bstr =
      resource.read(name + ".borderWidth", class_name + ".BorderWidth", "1");
    int bw = static_cast<unsigned int>(strtoul(bstr.c_str(), 0, 0));
    texture.setBorderWidth(bw);
  }

  return texture;
}
