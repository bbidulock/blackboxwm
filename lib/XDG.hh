// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// XDG.hh for Blackbox - an X11 Window manager
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

#ifndef __XDG_hh
#define __XDG_hh

#include <list>
#include <string>

namespace bt {

  namespace XDG {

    class BaseDir
    {
    public:
      /*
        This is an interface for basedir-spec 0.6 found at
        http://www.freedesktop.org/wiki/Standards_2fbasedir_2dspec
      */
      static std::string dataHome();
      static std::string configHome();
      static std::list<std::string> dataDirs();
      static std::list<std::string> configDirs();
      static std::string cacheHome();

      /*
        These functions return an absolute path to where a particular
        type of file can be written, creating directories as needed.
        All return an empty string if nothing can be written
        (i.e. destination directory does not exist and could not be
        created).
      */
      static std::string writeDataFile(const std::string &filename);
      static std::string writeConfigFile(const std::string &filename);
      static std::string writeCacheFile(const std::string &filename);
    };

  } // namespace XDG

} // namespace bt

#endif // __XDG_hh
