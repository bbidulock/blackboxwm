// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Display.hh for Blackbox - an X11 Window manager
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

#ifndef   __Display_hh
#define   __Display_hh

#include "Rect.hh"
#include "Util.hh"

#include <X11/Xlib.h>

namespace bt {

  // forward declarations
  class ScreenInfo;

  class Display : public NoCopy {
  private:
    ::Display *xdisplay;
    ScreenInfo** screen_info_list;
    size_t screen_info_count;

  public:
    Display(const char *dpy_name, bool multi_head);
    ~Display(void);

    inline ::Display* XDisplay(void) const
    { return xdisplay; }

    inline unsigned int screenCount(void) const
    { return screen_info_count; }
    const ScreenInfo &screenInfo(unsigned int i) const;
  };

  class ScreenInfo: public NoCopy {
  private:
    Display& _display;
    Visual *_visual;
    Window _rootwindow;
    Colormap _colormap;
    int _depth;
    unsigned int _screennumber;
    std::string _displaystring;
    Rect _rect;

  public:
    ScreenInfo(Display& d, unsigned int num);

    inline Display& display(void) const
    { return _display; }

    inline Visual *visual(void) const
    { return _visual; }
    inline Window rootWindow(void) const
    { return _rootwindow; }
    inline Colormap colormap(void) const
    { return _colormap; }

    inline int depth(void) const
    { return _depth; }

    inline unsigned int screenNumber(void) const
    { return _screennumber; }

    inline const Rect& rect(void) const
    { return _rect; }
    inline unsigned int width(void) const
    { return _rect.width(); }
    inline unsigned int height(void) const
    { return _rect.height(); }

    inline const std::string& displayString(void) const
    { return _displaystring; }
  };

} // namespace bt

#endif // __Display_hh
