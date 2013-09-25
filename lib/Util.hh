// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Util.hh for Blackbox - an X11 Window manager
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

#ifndef __Util_hh
#define __Util_hh

#include <X11/Xutil.h>

#include <limits.h>
#include <string>

// forward declarations of X11 types
typedef struct _XDisplay Display;
typedef union _XEvent XEvent;
typedef struct _XGC *GC;
typedef struct _XOC *XFontSet;
typedef struct _XftFont XftFont;
typedef unsigned long Time;
typedef unsigned long XID;
typedef XID Atom;
typedef XID Colormap;
typedef XID Cursor;
typedef XID Drawable;
typedef XID Pixmap;
typedef XID Window;

namespace bt {

  // XXX perhaps we could just call this SENTINEL?
  const unsigned int BSENTINEL = UINT_MAX;

  class NoCopy {
  protected:
    inline NoCopy(void)
    { }
  private:
    NoCopy(const NoCopy&);
    NoCopy& operator=(const NoCopy&);
  };

  class PointerAssassin {
  public:
    template<typename T>
    inline void operator()(const T ptr) const
    { delete ptr; }
  };

  inline bool within(int x, int y, int width, int height)
  { return ((x >= 0 && x <= width) && (y >= 0 && y <= height)); }

  void bexec(const std::string& command, const std::string& displaystring);

  std::string basename(const std::string& path);
  std::string dirname(const std::string& path);

  // equivalent to the shell command 'mkdir -m mode -p path'
  bool mkdirhier(const std::string &path, int mode = 0777);

  std::string expandTilde(const std::string& s);

  std::string itostring(unsigned long i);
  std::string itostring(long i);

  inline std::string itostring(unsigned int i)
  { return itostring(static_cast<unsigned long>(i)); }

  inline std::string itostring(int i)
  { return itostring(static_cast<long>(i)); }

  inline std::string itostring(unsigned short i)
  { return itostring(static_cast<unsigned long>(i)); }

  inline std::string itostring(short i)
  { return itostring(static_cast<long>(i)); }

  std::string tolower(const std::string &string);

  std::string textPropertyToString(::Display *display,
                                   ::XTextProperty& text_prop);

} // namespace bt

#endif // __Util_hh
