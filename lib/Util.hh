// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Util.cc for Blackbox - an X11 Window manager
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

#ifndef __Util_hh
#define __Util_hh

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
}

#include <string>

#ifndef   HAVE_BASENAME
std::string basename(const std::string& path);
#endif

struct timeval; // forward declare to avoid the header


namespace bt {

  class NoCopy {
  protected:
    NoCopy(void) {}
  private:
    NoCopy(const NoCopy&);
    NoCopy& operator=(const NoCopy&);
  };

  inline bool within(int x, int y, int width, int height) {
    return ((x >= 0 && x <= width) && (y >= 0 && y <= height));
  }

  /* XXX: this needs autoconf help */
  const unsigned int BSENTINEL = 65535;

  std::string expandTilde(const std::string& s);

  void bexec(const std::string& command, const std::string& displaystring);

  std::string textPropertyToString(Display *display, XTextProperty& text_prop);

  ::timeval normalizeTimeval(const ::timeval &tm);

  struct PointerAssassin {
    template<typename T>
    void operator()(const T ptr) const {
      delete ptr;
    }
  };

  std::string itostring(unsigned long i);
  std::string itostring(long i);
  std::string itostring(unsigned int i);
  std::string itostring(int i);
  std::string itostring(unsigned short i);
  std::string itostring(short i);

} // namespace bt

#endif // __Util_hh
