// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Display.cc for Blackbox - an X11 Window manager
// Copyright (c) 2002 Sean 'Shaleh' Perry <shaleh at debian.org>
// Copyright (c) 2002 Bradley T Hughes <bhughes at trolltech.com>
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

extern "C" {
#include <assert.h>
#include <fcntl.h>
}

#include <algorithm>

#include "Display.hh"


namespace bt {

  void createColorCache(const Display &display);
  void destroyColorCache(void);


  void createFontCache(const Display &display);
  void destroyFontCache(void);


  void createPenCache(const Display &display);
  void destroyPenCache(void);

  void createPixmapCache(const Display &display);
  void destroyPixmapCache(void);

} // namespace bt


bt::Display::Display(const char *dpy_name) {
  if (! (xdisplay = XOpenDisplay(dpy_name)))
      ::exit(2);

  if (fcntl(ConnectionNumber(xdisplay), F_SETFD, 1) == -1)
    ::exit(2);

  screenInfoList.reserve(ScreenCount(xdisplay));
  for (int i = 0; i < ScreenCount(xdisplay); ++i)
    screenInfoList.push_back(new bt::ScreenInfo(*this, i));

  createColorCache(*this);
  createFontCache(*this);
  createPenCache(*this);
  createPixmapCache(*this);
}


bt::Display::~Display() {
  destroyPixmapCache();
  destroyPenCache();
  destroyFontCache();
  destroyColorCache();

  std::for_each(screenInfoList.begin(), screenInfoList.end(),
           bt::PointerAssassin());

  XCloseDisplay(xdisplay);
}


const bt::ScreenInfo &bt::Display::screenInfo(unsigned int i) const {
  assert(i < screenInfoList.size());
  return *screenInfoList[i];
}


bt::ScreenInfo::ScreenInfo(bt::Display& d, unsigned int num)
  : _display(d), _screennumber(num)
{
  _rootwindow = RootWindow(_display.XDisplay(), _screennumber);

  _rect.setSize(WidthOfScreen(ScreenOfDisplay(_display.XDisplay(),
                                              _screennumber)),
                HeightOfScreen(ScreenOfDisplay(_display.XDisplay(),
                                               _screennumber)));

  /*
    If the default depth is at least 8 we will use that,
    otherwise we try to find the largest TrueColor visual.
    Preference is given to 24 bit over larger depths if 24 bit is an option.
  */

  _depth = DefaultDepth(_display.XDisplay(), _screennumber);
  _visual = DefaultVisual(_display.XDisplay(), _screennumber);
  _colormap = DefaultColormap(_display.XDisplay(), _screennumber);

  if (_depth < 8) {
    // search for a TrueColor Visual... if we can't find one...
    // we will use the default visual for the screen
    XVisualInfo vinfo_template, *vinfo_return;
    int vinfo_nitems;
    int best = -1;

    vinfo_template.screen = _screennumber;
    vinfo_template.c_class = TrueColor;

    vinfo_return = XGetVisualInfo(_display.XDisplay(),
                                  VisualScreenMask | VisualClassMask,
                                  &vinfo_template, &vinfo_nitems);
    if (vinfo_return) {
      int max_depth = 1;
      for (int i = 0; i < vinfo_nitems; ++i) {
        if (vinfo_return[i].depth < max_depth ||
            // prefer 24 bit over 32
            (max_depth == 24 && vinfo_return[i].depth > 24))
          continue;
        max_depth = vinfo_return[i].depth;
        best = i;
      }
      if (max_depth < _depth) best = -1;
    }

    if (best != -1) {
      _depth = vinfo_return[best].depth;
      _visual = vinfo_return[best].visual;
      _colormap = XCreateColormap(_display.XDisplay(), _rootwindow,
                                  _visual, AllocNone);
    }

    XFree(vinfo_return);
  }

  // get the default display string and strip the screen number
  std::string default_string = DisplayString(_display.XDisplay());
  const std::string::size_type pos = default_string.rfind(".");
  if (pos != std::string::npos)
    default_string.resize(pos);

  _displaystring = std::string("DISPLAY=") + default_string + '.' +
                   bt::itostring(_screennumber);
}
