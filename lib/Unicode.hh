// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
#ifndef UNICODE_HH
#define UNICODE_HH

#include <string>

namespace bt
{
  // returns true if the system supports Unicode conversions
  bool hasUnicode();

  // unicode string type
  typedef std::basic_string<unsigned int> ustring;

  // converts multibyte locale-encoded string to wide-char Unicode
  // string (aka UTF-32)
  ustring toUnicode(const std::string &string);
  // convert wide-char Unicode string (aka UTF-32) to multibyte
  // locale-encoded string
  std::string toLocale(const ustring &string);

  // convert UTF-32 to UTF-8
  std::string toUtf8(const ustring &utf32);
  // convert UTF-8 to UTF-32
  ustring toUtf32(const std::string &utf8);

} // namespace bt

#endif // UNICODE_HH
