// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Texture.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2003
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

#include "Texture.hh"
#include "Display.hh"
#include "Pen.hh"
#include "Resource.hh"

#include <X11/Xlib.h>
#include <ctype.h>

#include <algorithm>


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


void bt::drawTexture(unsigned int screen,
                     const Texture &texture,
                     Drawable drawable,
                     const Rect &trect,
                     const Rect &urect,
                     Pixmap pixmap) {
  Pen pen(screen, texture.color());

  if ((texture.texture() & Texture::Gradient) && pixmap) {
    XCopyArea(pen.XDisplay(), pixmap, drawable, pen.gc(),
              urect.x() - trect.x(), urect.y() - trect.y(),
              urect.width(), urect.height(), urect.x(), urect.y());
    return;
  } else if (! (texture.texture() & Texture::Solid)) {
    XClearArea(pen.XDisplay(), drawable,
               urect.x(), urect.y(), urect.width(), urect.height(), False);
    return; // might be Parent_Relative or empty
  }

  XFillRectangle(pen.XDisplay(), drawable, pen.gc(),
                 urect.x(), urect.y(), urect.width(), urect.height());

  int bw = static_cast<int>(texture.borderWidth());
  if (texture.texture() & bt::Texture::Border &&
      (trect.left() == urect.left() || trect.right() == urect.right() ||
       trect.top() == urect.top() || trect.bottom() == urect.bottom())) {
    Pen penborder(screen, texture.borderColor());
    penborder.setLineWidth(bw);
    XDrawRectangle(pen.XDisplay(), drawable, penborder.gc(),
                   trect.x() + bw / 2,  trect.y() + bw / 2,
                   trect.width() - bw, trect.height() - bw);
  }

  if (texture.texture() & bt::Texture::Interlaced) {
    Pen peninterlace(screen, texture.colorTo());
    int begin = trect.top()    + bw;
    while (begin < urect.top()) begin += 2;
    int end   = std::min(trect.bottom() - bw, urect.bottom());

    for (int i = begin; i <= end; i += 2)
      XDrawLine(pen.XDisplay(), drawable, peninterlace.gc(),
                std::max(trect.left()  + bw, urect.left()),  i,
                std::min(trect.right() - bw, urect.right()), i);
  }

  if ((trect.left()   + bw >= urect.left()  ||
       trect.right()  - bw <= urect.right() ||
       trect.top()    + bw >= urect.top()   ||
       trect.bottom() - bw <= urect.bottom())) {
    // draw bevel
    Pen penlight(screen, texture.lightColor());
    Pen penshadow(screen, texture.shadowColor());

    if (texture.texture() & bt::Texture::Raised) {
      XDrawLine(pen.XDisplay(), drawable, penshadow.gc(),
                trect.left() + bw,  trect.bottom() - bw,
                trect.right() - bw, trect.bottom() - bw);
      XDrawLine(pen.XDisplay(), drawable, penshadow.gc(),
                trect.right() - bw, trect.bottom() - bw,
                trect.right() - bw, trect.top() + bw);

      XDrawLine(pen.XDisplay(), drawable, penlight.gc(),
                trect.left() + bw, trect.top() + bw,
                trect.right() - bw, trect.top() + bw);
      XDrawLine(pen.XDisplay(), drawable, penlight.gc(),
                trect.left() + bw, trect.bottom() - bw,
                trect.left() + bw, trect.top() + bw);
    } else if (texture.texture() & bt::Texture::Sunken) {
      XDrawLine(pen.XDisplay(), drawable, penlight.gc(),
                trect.left() + bw,  trect.bottom() - bw,
                trect.right() - bw, trect.bottom() - bw);
      XDrawLine(pen.XDisplay(), drawable, penlight.gc(),
                trect.right() - bw, trect.bottom() - bw,
                trect.right() - bw, trect.top() + bw);

      XDrawLine(pen.XDisplay(), drawable, penshadow.gc(),
                trect.left() + bw,  trect.top() + bw,
                trect.right() - bw, trect.top() + bw);
      XDrawLine(pen.XDisplay(), drawable, penshadow.gc(),
                trect.left() + bw, trect.bottom() - bw,
                trect.left() + bw, trect.top() + bw);
    }
  }
}
