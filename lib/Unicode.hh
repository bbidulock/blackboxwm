// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Unicode.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2004
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

#ifndef __Unicode_hh
#define __Unicode_hh

#include <string>

namespace bt
{
  /*
   * Returns true if the system supports Unicode conversions.
   */
  bool hasUnicode();

  /*
   * Unicode string type.
   */
  typedef std::basic_string<unsigned int> ustring;

  /*
   * Converts multibyte locale-encoded string to wide-char Unicode
   * string (aka UTF-32).
   */
  ustring toUnicode(const std::string &string);

  /*
   * Converts wide-char Unicode string (aka UTF-32) to multibyte
   * locale-encoded string.
   */
  std::string toLocale(const ustring &string);

  /*
   * Converts UTF-32 to UTF-8.
   */
  std::string toUtf8(const ustring &utf32);

  /*
   * Convert UTF-8 to UTF-32.
   */
  ustring toUtf32(const std::string &utf8);

} // namespace bt

#endif // __Unicode_hh
