// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Unicode.hh for Blackbox - an X11 Window manager
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
   * Unicode character type.
   */
  typedef unsigned int Uchar;

  /*
   * Unicode string type.
   */
  typedef std::basic_string<Uchar> ustring;

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

/*
 * Workaround incomplete std::char_traits<> implementation in come
 * versions of the GNU C++ compiler.
 */
#if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#  if __GNUC__ == 3
namespace std {

  template<>
  struct char_traits<bt::Uchar>
  {
    typedef bt::Uchar    char_type;
    typedef unsigned int int_type;
    typedef streampos    pos_type;
    typedef streamoff    off_type;
    typedef mbstate_t    state_type;

    static void
    assign(bt::Uchar &c1, const bt::Uchar &c2)
    { c1 = c2; }

    static bool
    eq(const bt::Uchar &c1, const bt::Uchar &c2)
    { return c1 == c2; }

    static bool
    lt(const bt::Uchar &c1, const bt::Uchar &c2)
    { return c1 < c2; }

    static int
    compare(const bt::Uchar *s1, const bt::Uchar *s2, size_t n) {
      for (size_t x = 0; x < n; ++x) {
        if (lt(s1[x], s2[x]))
          return -1;
        if (lt(s2[x], s1[x]))
          return 1;
      }
      return 0;
    }

    static size_t
    length(const bt::Uchar *s)
    {
      size_t x = 0;
      while (!eq(s[x], bt::Uchar()))
        ++x;
      return x;
    }

    static const bt::Uchar *
    find(const bt::Uchar *s, std::size_t n, const bt::Uchar &c)
    {
      for (std::size_t x = 0; x < n; ++x)
        if (eq(s[x], c))
          return s + x;
      return 0;
    }

    static bt::Uchar *
    move(bt::Uchar *d, const bt::Uchar *s, std::size_t n) {
      return (static_cast<bt::Uchar *>
              (std::memmove(d, s, n  *sizeof(bt::Uchar ))));
    }

    static bt::Uchar *
    copy(bt::Uchar *d, const bt::Uchar *s, std::size_t n) {
      std::copy(s, s + n, d);
      return d;
    }

    static bt::Uchar *
    assign(bt::Uchar *s, std::size_t n, bt::Uchar c) {
      std::fill_n(s, n, c);
      return s;
    }

    static bt::Uchar
    to_char_type(const unsigned int &c)
    { return c; }

    static unsigned int
    to_int_type(const bt::Uchar &c)
    { return c; }

    static bool
    eq_int_type(const unsigned int &c1, const unsigned int &c2)
    { return c1 == c2; }

    static int_type
    eof()
    { return static_cast<unsigned int>(EOF); }

    static unsigned int
    not_eof(const unsigned int &c)
    { return (c == eof()) ? 0 : c; }

  };

} // namespace std
#  endif // __GNUC__ == 3
#endif // __GNUC__ && !__INTEL_COMPILER

#endif // __Unicode_hh
