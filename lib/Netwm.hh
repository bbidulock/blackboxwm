// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// netwm.hh for Blackbox - an X11 Window manager
// Copyright (c) 2002 Sean 'Shaleh' Perry <shaleh@debian.org>
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

#ifndef _blackbox_netwm_hh
#define _blackbox_netwm_hh

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xatom.h>
}

#include <string>

class Netwm {
public:
  explicit Netwm(Display* display);
  ~Netwm(void) {};

  Atom utf8String(void) const { return utf8_string; }
  Atom supported(void) const { return net_supported; }
  Atom numberOfDesktops(void) const { return net_number_of_desktops; }
  Atom desktopGeometry(void) const { return net_desktop_geometry; }
  Atom desktopViewport(void) const { return net_desktop_viewport; }
  Atom currentDesktop(void) const { return net_current_desktop; }
  Atom activeWindow(void) const { return net_active_window; }
  Atom supportingWMCheck(void) const { return net_supporting_wm_check; }
  Atom wmName(void) const { return net_wm_name; }

  void setSupported(Atom supported[], unsigned int count,
		    Display *display, Window target) const ;
  void setNumberOfDesktops(unsigned int number, Display* display,
                           Window target) const;
  void setDesktopGeometry(unsigned int width, unsigned int height,
                          Display* display, Window target) const;
  void setDesktopViewport(unsigned int x, unsigned int y,
                          Display* display, Window target) const;
  void setCurrentDesktop(unsigned int number, Display* display,
                         Window target) const;
  void setActiveWindow(Window target, Window data, Display* display) const;
  void setSupportingWMCheck(Window target, Window data,
                            Display* display) const;
  void setWMName(const std::string& name, Window w, Display *display) const;

  void removeProperty(Atom atom, Display* display, Window target) const;

private:
  Netwm(const Netwm&);
  Netwm& operator=(const Netwm&);

  Atom utf8_string,
    net_supported, net_number_of_desktops, net_desktop_geometry,
    net_desktop_viewport, net_current_desktop, net_active_window,
    net_supporting_wm_check, net_wm_name;
};

#endif // _blackbox_netwm_hh
