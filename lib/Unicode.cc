// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
#include "Unicode.hh"

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <errno.h>
#include <iconv.h>
#include <locale.h>
#include <stdio.h>

namespace bt {

  static const iconv_t invalid = reinterpret_cast<iconv_t>(-1);

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

  template <typename _Source, typename _Target>
  static void convert(const char *target, const char *source,
                      const _Source &in, _Target &out) {
    iconv_t cd = iconv_open(target, source);
    if (cd == invalid)
      return;

#ifdef HAVE_GNU_LIBICONV
    // GNU libiconv
    const char *inp = reinterpret_cast<const char *>(in.c_str());
#else
    // POSIX compliant iconv(3)
    char *inp =
      reinterpret_cast<char *>
      (const_cast<typename _Source::value_type *>(in.c_str()));
#endif
    const typename _Source::size_type in_size =
      in.size() * sizeof(typename _Source::value_type);
    typename _Source::size_type in_bytes = in_size;

    out.resize(in_size);

    char *outp =
      reinterpret_cast<char *>
      (const_cast<typename _Target::value_type *>(out.c_str()));
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
            inp = reinterpret_cast<const char *>(in.c_str()) + off;
#else
            // POSIX compliant iconv(3)
            inp =
              reinterpret_cast<char *>
              (const_cast<typename _Source::value_type *>(in.c_str()));
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
              (const_cast<typename _Target::value_type *>(out.c_str())) + off;
            out_bytes = out_size - off;
            break;
          }
        default:
          perror("iconv");
          out.clear();
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

  struct {
    const char *to;
    const char *from;
  } conversions[] = {
    { "UTF-32", "" },
    { "UTF-32", "UTF-8" },
    { "UTF-8", "UTF-32" },
    { "", "UTF-32" },
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
  convert("UTF-32", "", string, ret);
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
  convert("", "UTF-32", string, ret);
  return ret;
}

std::string bt::toUtf8(const bt::ustring &utf32) {
  std::string ret;
  if (!hasUnicode())
    return ret;
  ret.reserve(utf32.size());
  convert("UTF-8", "UTF-32", utf32, ret);
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
