// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
#include "Utf8.hh"

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include <iconv.h>
#ifdef HAVE_NL_LANGINFO
#  include <langinfo.h>
#endif

namespace bt {

  static const iconv_t invalid = reinterpret_cast<iconv_t>(-1);

  static bool locale_is_utf8(void) {
    static bool is_utf8 = false;
    static bool done = false;

    if (done)
      return is_utf8;

    setlocale(LC_ALL, "");

    std::string codeset;
#ifdef HAVE_NL_LANGINFO
    codeset = nl_langinfo(CODESET);
#else
    std::string locale = setlocale(LC_CTYPE, 0);
    std::string::const_iterator it = locale.begin();
    const std::string::const_iterator end = locale.end();
    for (; it != end; ++it) {
      if (*it == '.') {
        // found codeset separator
        ++it;
        codeset = std::string(it, end);
      }
    }
#endif // HAVE_NL_LANGINFO

    is_utf8 = (codeset == "UTF-8");
    done = true;
    return is_utf8;
  }

  static std::string convert(iconv_t cd, const std::string &string) {
    const char *in = string.c_str();
    size_t in_sz = string.size();

    std::string ret;
    ret.resize(in_sz);

    char *out = const_cast<char *>(ret.c_str());
    size_t out_sz = ret.size();

    do {
      size_t l = iconv(cd, &in, &in_sz, &out, &out_sz);

      if (l == (size_t) -1) {
        switch (errno) {
        case EILSEQ:
        case EINVAL:
          {
            const size_t off = string.size() - in_sz + 1;
            in = string.c_str() + off;
            in_sz = string.size() - off;
            break;
          }
        case E2BIG:
          {
            const size_t off = ret.size() - out_sz;
            ret.resize(ret.size() * 2);
            out = const_cast<char *>(ret.c_str()) + off;
            out_sz = ret.size() - off;
            break;
          }
        default:
          perror("iconv");
          return std::string();
        }
      }
    } while (in_sz != 0);
    ret.resize(ret.size() - out_sz);

    return ret;
  }

} // namespace bt

bool bt::hasUtf8() {
  static bool has_utf8 = false;
  static bool done = false;

  if (done)
    return has_utf8;

  has_utf8 = locale_is_utf8();

  if (!has_utf8) {
    iconv_t utf8 = iconv_open("UTF-8", "");
    iconv_t locale = iconv_open("", "UTF-8");

    has_utf8 = (utf8 != invalid && locale != invalid);

    if (utf8 != invalid)
      iconv_close(utf8);
    if (locale != invalid)
      iconv_close(locale);
  }

  done = true;
  return has_utf8;
}

std::string bt::toUtf8(const std::string &string) {
  if (locale_is_utf8())
    return string;
  iconv_t utf8 = iconv_open("UTF-8", "");
  if (utf8 == invalid)
    return std::string();
  std::string str = convert(utf8, string);
  iconv_close(utf8);
  return str;
}

std::string bt::toLocale(const std::string &string) {
  if (locale_is_utf8())
    return string;
  iconv_t locale = iconv_open("", "UTF-8");
  if (locale == invalid)
    return std::string();
  std::string str = convert(locale, string);
  iconv_close(locale);
  return str;
}
