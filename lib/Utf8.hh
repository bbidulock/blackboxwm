// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
#ifndef UTF8_HH
#define UTF8_HH

#include <string>

namespace bt
{
  bool hasUtf8();

  std::string toUtf8(const std::string &string);
  std::string toLocale(const std::string &string);

} // namespace bt

#endif // UTF8_HH
