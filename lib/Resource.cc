// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Resource.cc for Blackbox - an X11 Window manager
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

extern "C" {
#include <stdio.h>
}

#include "Resource.hh"
#include "Util.hh"

namespace bt {

  Resource::Resource(void) : db(NULL) { }


  Resource::Resource(const std::string &filename) : db(NULL) {
    load(filename);
  }


  Resource::~Resource(void) {
    XrmDestroyDatabase(db);
  }


  void Resource::load(const std::string &filename) {
    XrmDestroyDatabase(db);
    db = XrmGetFileDatabase(expandTilde(filename).c_str());
  }


  void Resource::save(const std::string &filename) {
    if (! valid()) return;
    XrmPutFileDatabase(db, expandTilde(filename).c_str());
  }


  void Resource::merge(const std::string &filename) {
    XrmCombineFileDatabase(expandTilde(filename).c_str(), &db, true);
  }


  std::string Resource::read(const char* name, const char* classname,
                             const char* default_value) const {
    XrmValue value;
    char *value_type;
    if (XrmGetResource(db, name, classname, &value_type, &value))
      return std::string(value.addr, value.size - 1);
    return std::string(default_value);
  }


  std::string Resource::read(const std::string& name,
                             const std::string& classname,
                             const std::string& default_value) const {
    XrmValue value;
    char *value_type;
    if (XrmGetResource(db, name.c_str(), classname.c_str(),
                       &value_type, &value))
      return std::string(value.addr, value.size - 1);
    return default_value;
  }


  int Resource::read(const char* name, const char* classname,
                     int default_value) const {
    XrmValue value;
    char *value_type;
    if (XrmGetResource(db, name, classname, &value_type, &value)) {
      int output;
      sscanf(value.addr, "%d", &output);
      return output;
    }
    return default_value;
  }


  unsigned int Resource::read(const char* name, const char* classname,
                              unsigned int default_value) const {
    XrmValue value;
    char *value_type;
    if (XrmGetResource(db, name, classname, &value_type, &value)) {
      unsigned int output;
      sscanf(value.addr, "%u", &output);
      return output;
    }
    return default_value;
  }


  long Resource::read(const char* name, const char* classname,
                      long default_value) const {
    XrmValue value;
    char *value_type;
    if (XrmGetResource(db, name, classname, &value_type, &value)) {
      long output;
      sscanf(value.addr, "%ld", &output);
      return output;
    }
    return default_value;
  }


  unsigned long Resource::read(const char* name, const char* classname,
                               unsigned long default_value) const {
    XrmValue value;
    char *value_type;
    if (XrmGetResource(db, name, classname, &value_type, &value)) {
      long output;
      sscanf(value.addr, "%lu", &output);
      return output;
    }
    return default_value;
  }


  bool Resource::read(const char* name, const char* classname,
                      bool default_value) const {
    XrmValue value;
    char *value_type;
    if (XrmGetResource(db, name, classname, &value_type, &value)) {
      if (strncasecmp(value.addr, "true", value.size) == 0)
        return true;
      return false;
    }
    return default_value;
  }


  double Resource::read(const char* name, const char* classname,
                        double default_value) const {
    XrmValue value;
    char *value_type;
    if (XrmGetResource(db, name, classname, &value_type, &value)) {
      double output;
      sscanf(value.addr, "%f", &output);
      return output;
    }
    return default_value;
  }


  void Resource::write(const char* resource, const char* value) {
    XrmPutStringResource(&db, resource, value);
  }


  void Resource::write(const char* resource, int value) {
    char tmp[16];
    sprintf(tmp, "%d", value);
    write(resource, tmp);
  }


  void Resource::write(const char* resource, unsigned int value) {
    char tmp[16];
    sprintf(tmp, "%u", value);
    write(resource, tmp);
  }


  void Resource::write(const char* resource, long value) {
    char tmp[64];
    sprintf(tmp, "%ld", value);
    write(resource, tmp);
  }


  void Resource::write(const char* resource, unsigned long value) {
    char tmp[64];
    sprintf(tmp, "%lu", value);
    write(resource, tmp);
  }


  void Resource::write(const char* resource, bool value) {
    write(resource, boolAsString(value));
  }


  void Resource::write(const char* resource, double value) {
    char tmp[80];
    sprintf(tmp, "%f", value);
    write(resource, tmp);
  }

}
