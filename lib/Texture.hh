// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Texture.hh for Blackbox - an X11 Window manager
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

#ifndef TEXTURE_HH
#define TEXTURE_HH

#include "Color.hh"

#include <string>

typedef unsigned long Pixmap;


namespace bt {

  class ImageControl;
  class Resource;

  class Texture {
  public:
    enum Type {
      // bevel options
      Flat                = (1l<<0),
      Sunken              = (1l<<1),
      Raised              = (1l<<2),
      // textures
      Solid               = (1l<<3),
      Gradient            = (1l<<4),
      // gradients
      Horizontal          = (1l<<5),
      Vertical            = (1l<<6),
      Diagonal            = (1l<<7),
      CrossDiagonal       = (1l<<8),
      Rectangle           = (1l<<9),
      Pyramid             = (1l<<10),
      PipeCross           = (1l<<11),
      Elliptic            = (1l<<12),
      // inverted image
      Invert              = (1l<<14),
      // parent relative image
      Parent_Relative     = (1l<<15),
      // fake interlaced image
      Interlaced          = (1l<<16),
      // border around image
      Border              = (1l<<17)
    };

    Texture(const bt::Display * const _display = 0,
            unsigned int _screen = ~(0u),
            bt::ImageControl* _ctrl = 0);
    Texture(const std::string &_description,
            const bt::Display * const _display = 0,
            unsigned int _screen = ~(0u),
            bt::ImageControl* _ctrl = 0);

    void setColor(const bt::Color &new_color);
    void setColorTo(const bt::Color &new_colorTo);
    void setBorderColor(const bt::Color &new_borderColor);

    inline const bt::Color &color(void) const { return c; }
    inline const bt::Color &colorTo(void) const { return ct; }
    inline const bt::Color &borderColor(void) const { return bc; }
    inline const bt::Color &lightColor(void) const { return lc; }
    inline const bt::Color &shadowColor(void) const { return sc; }

    inline unsigned long texture(void) const { return t; }
    inline void setTexture(const unsigned long _texture) { t  = _texture; }
    inline void addTexture(const unsigned long _texture) { t |= _texture; }

    inline unsigned int borderWidth(void) const { return bw; }
    inline void setBorderWidth(unsigned int new_bw) { bw = new_bw; }

    Texture &operator=(const Texture &tt);
    inline bool operator==(const Texture &tt)
    { return (c == tt.c && ct == tt.ct && bc == tt.bc &&
              lc == tt.lc && sc == tt.sc && t == tt.t && bw == tt.bw); }
    inline bool operator!=(const Texture &tt)
    { return (! operator==(tt)); }

    const bt::Display *display(void) const { return dpy; }
    unsigned int screen(void) const { return scrn; }
    void setDisplay(const bt::Display * const _display,
                    const unsigned int _screen);
    inline void setImageControl(bt::ImageControl* _ctrl) { ctrl = _ctrl; }
    inline const std::string &description(void) const { return descr; }
    void setDescription(const std::string &d);

    Pixmap render(const unsigned int width, const unsigned int height,
                  const Pixmap old = 0);

  private:
    bt::Color c, ct, bc, lc, sc;
    std::string descr;
    unsigned long t;
    unsigned int bw;
    const bt::Display *dpy;
    bt::ImageControl *ctrl;
    unsigned int scrn;
  };

  Texture
  textureResource(const Resource &resource,
                  const std::string &name,
                  const std::string &classname,
                  const std::string &default_color = std::string("black"),
                  const bt::Display * const _display = 0,
                  unsigned int _screen = ~(0u),
                  bt::ImageControl* _ctrl = 0);

} // namespace bt

#endif // TEXTURE_HH
