// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Unicode.cc for Blackbox - an X11 Window manager
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

#include "Unicode.hh"

#include <algorithm>

#include <errno.h>
#include <iconv.h>
#include <locale.h>
#include <stdio.h>

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#ifdef HAVE_NL_LANGINFO
#  include <langinfo.h>
#endif


namespace bt {

  static const iconv_t invalid = reinterpret_cast<iconv_t>(-1);
  static std::string codeset;

  static unsigned int byte_swap(unsigned int c) {
    wchar_t ret;
    int x = sizeof(wchar_t);
    char *s = reinterpret_cast<char *>(&c);
    char *d = reinterpret_cast<char *>(&ret) + x - 1;
    while (x-- > 0)
      *d-- = *s++;
    return ret;
  }

  static ustring native_endian(const ustring &string) {
    if (string.empty())
      return string;
    if (*string.begin() == 0x0000feff) {
      // begins with BOM in native endian
      return ustring(string.begin() + 1, string.end());
    } else if (*string.begin() == 0xfffe0000) {
      // BOM is byte swapped, convert to native endian
      ustring ret = ustring(string.begin() + 1, string.end());
      ustring::iterator it = ret.begin();
      const ustring::iterator end = ret.end();
      for (; it != end; ++it)
        *it = byte_swap(*it);
      return ret;
    } else {
      return string;
    }
  }

  static ustring add_bom(const ustring &string) {
    ustring ret;
    ret.push_back(0x0000feff);
    return ret + string;
  }

  template <typename _Source, typename _Target>
  static void convert(const char *target, const char *source,
                      const _Source &in, _Target &out) {
    iconv_t cd = iconv_open(target, source);
    if (cd == invalid)
      return;

#ifdef HAVE_GNU_LIBICONV
    // GNU libiconv
    const char *inp = reinterpret_cast<const char *>(in.data());
#else
    // POSIX compliant iconv(3)
    char *inp =
      reinterpret_cast<char *>
      (const_cast<typename _Source::value_type *>(in.data()));
#endif
    const typename _Source::size_type in_size =
      in.size() * sizeof(typename _Source::value_type);
    typename _Source::size_type in_bytes = in_size;

    out.resize(in_size);

    char *outp =
      reinterpret_cast<char *>
      (const_cast<typename _Target::value_type *>(out.data()));
    typename _Target::size_type out_size =
      out.size() * sizeof(typename _Target::value_type);
    typename _Target::size_type out_bytes = out_size;

    do {
      size_t l = iconv(cd, &inp, &in_bytes, &outp, &out_bytes);

      if (l == (size_t) -1) {
        switch (errno) {
        case EILSEQ:
        case EINVAL:
          {
            const typename _Source::size_type off = in_size - in_bytes + 1;
#ifdef HAVE_GNU_LIBICONV
            // GNU libiconv
            inp = reinterpret_cast<const char *>(in.data()) + off;
#else
            // POSIX compliant iconv(3)
            inp =
              reinterpret_cast<char *>
              (const_cast<typename _Source::value_type *>(in.data()));
#endif
            in_bytes = in_size - off;
            break;
          }
        case E2BIG:
          {
            const typename _Target::size_type off = out_size - out_bytes;
            out.resize(out.size() * 2);
            out_size = out.size() * sizeof(typename _Target::value_type);

            outp =
              reinterpret_cast<char *>
              (const_cast<typename _Target::value_type *>(out.data())) + off;
            out_bytes = out_size - off;
            break;
          }
        default:
          perror("iconv");
          out = _Target();
          iconv_close(cd);
          return;
        }
      }
    } while (in_bytes != 0);

    out.resize((out_size - out_bytes) / sizeof(typename _Target::value_type));
    iconv_close(cd);
  }

} // namespace bt

bool bt::hasUnicode() {
  static bool has_unicode = true;
  static bool done = false;

  if (done)
    return has_unicode;

  setlocale(LC_ALL, "");

#ifdef HAVE_NL_LANGINFO
  codeset = nl_langinfo(CODESET);
#else
  std::string locale = setlocale(LC_CTYPE, 0);
  std::string::const_iterator it = locale.begin();
  const std::string::const_iterator end = locale.end();
  codeset = ""; // empty by default, not null
  for (; it != end; ++it) {
    if (*it == '.') {
      // found codeset separator
      ++it;
      codeset = std::string(it, end);
    }
  }
#endif // HAVE_NL_LANGINFO

  struct {
    const char *to;
    const char *from;
  } conversions[] = {
    { "UTF-32", codeset.c_str() },
    { "UTF-32", "UTF-8" },
    { "UTF-8", "UTF-32" },
    { codeset.c_str(), "UTF-32" },
  };
  static const int conversions_count = 4;

  for (int x = 0; x < conversions_count; ++x) {
    iconv_t cd = iconv_open(conversions[x].to, conversions[x].from);

    if (cd == invalid) {
      has_unicode = false;
      break;
    }

    iconv_close(cd);
  }

  done = true;
  return has_unicode;
}

bt::ustring bt::toUnicode(const std::string &string) {
  bt::ustring ret;
  if (!hasUnicode()) {
    // cannot convert to Unicode, return something instead of nothing
    ret.resize(string.size());
    std::copy(string.begin(), string.end(), ret.begin());
    return ret;
  }
  ret.reserve(string.size());
  convert("UTF-32", codeset.c_str(), string, ret);
  return native_endian(ret);
}

std::string bt::toLocale(const bt::ustring &string) {
  std::string ret;
  if (!hasUnicode()) {
    // cannot convert from Unicode, return something instead of nothing
    ret.resize(string.size());
    std::copy(string.begin(), string.end(), ret.begin());
    return ret;
  }
  ret.reserve(string.size());
  convert(codeset.c_str(), "UTF-32", add_bom(string), ret);
  return ret;
}

std::string bt::toUtf8(const bt::ustring &utf32) {
  std::string ret;
  if (!hasUnicode())
    return ret;
  ret.reserve(utf32.size());
  convert("UTF-8", "UTF-32", add_bom(utf32), ret);
  return ret;
}

bt::ustring bt::toUtf32(const std::string &utf8) {
  ustring ret;
  if (!hasUnicode())
    return ret;
  ret.reserve(utf8.size());
  convert("UTF-32", "UTF-8", utf8, ret);
  return native_endian(ret);
}
