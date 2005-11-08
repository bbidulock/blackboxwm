// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Pen.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh@debian.org>
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

#include "Pen.hh"
#include "Display.hh"
#include "Color.hh"
#include "Util.hh"

#include <algorithm>

#include <X11/Xlib.h>
#ifdef XFT
#  include <X11/Xft/Xft.h>
#endif

#include <assert.h>
#include <stdio.h>

namespace bt {

  class PenLoader
  {
    const Display &_display;
  public:
    PenLoader(const Display &display_)
      : _display(display_)
    { }
    const Display &display(void) const
    { return _display; }
    ::Display *XDisplay(void) const
    { return _display.XDisplay(); }
  };

  static PenLoader *penloader = 0;

  void createPenLoader(const Display &display)
  {
    assert(penloader == 0);
    penloader = new PenLoader(display);
  }
  void destroyPenLoader(void)
  {
    delete penloader;
    penloader = 0;
  }

} // namespace bt

bt::Pen::Pen(unsigned int screen_)
  : _screen(screen_), _function(GXcopy),  _linewidth(0),
    _subwindow(ClipByChildren), _dirty(false), _gc(0), _xftdraw(0)
{ }

bt::Pen::Pen(unsigned int screen_, const Color &color_)
  : _screen(screen_), _color(color_), _function(GXcopy), _linewidth(0),
    _subwindow(ClipByChildren), _dirty(false), _gc(0), _xftdraw(0)
{ }

bt::Pen::~Pen(void)
{
  if (_gc)
    XFreeGC(penloader->XDisplay(), _gc);
  _gc = 0;

#ifdef XFT
  if (_xftdraw)
    XftDrawDestroy(_xftdraw);
  _xftdraw = 0;
#endif
}

void bt::Pen::setColor(const Color &color_)
{
  _color = color_;
  _dirty = true;
}

void bt::Pen::setGCFunction(int function)
{
  _function = function;
  _dirty = true;
}

void bt::Pen::setLineWidth(int linewidth)
{
  _linewidth = linewidth;
  _dirty = true;
}

void bt::Pen::setSubWindowMode(int subwindow)
{
  _subwindow = subwindow;
  _dirty = true;
}

::Display *bt::Pen::XDisplay(void) const
{ return penloader->XDisplay(); }

const bt::Display &bt::Pen::display(void) const
{ return penloader->display(); }

const GC &bt::Pen::gc(void) const
{
  if (!_gc || _dirty) {
    XGCValues gcv;
    gcv.foreground = _color.pixel(_screen);
    gcv.function = _function;
    gcv.line_width = _linewidth;
    gcv.subwindow_mode = _subwindow;
    if (!_gc) {
      _gc = XCreateGC(penloader->XDisplay(),
                      penloader->display().screenInfo(_screen).rootWindow(),
                      (GCForeground
                       | GCFunction
                       | GCLineWidth
                       | GCSubwindowMode),
                      &gcv);

    } else {
      XChangeGC(penloader->XDisplay(),
                _gc,
                (GCForeground
                 | GCFunction
                 | GCLineWidth
                 | GCSubwindowMode),
                &gcv);
    }
    _dirty = false;
  }
  assert(_gc != 0);
  return _gc;
}

XftDraw *bt::Pen::xftDraw(Drawable drawable) const
{
#ifdef XFT
  if (!_xftdraw) {
    const ScreenInfo &screeninfo = penloader->display().screenInfo(_screen);
    _xftdraw = XftDrawCreate(penloader->XDisplay(),
                             drawable,
                             screeninfo.visual(),
                             screeninfo.colormap());
  } else if (XftDrawDrawable(_xftdraw) != drawable) {
    XftDrawChange(_xftdraw, drawable);
  }
  assert(_xftdraw != 0);
  return _xftdraw;
#else
  return 0;
#endif
}

