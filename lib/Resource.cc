// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Resource.cc for Blackbox - an X11 Window manager
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

#include "Resource.hh"
#include "Util.hh"

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include <stdio.h>


bt::Resource::Resource(void)
  : db(NULL)
{ }


bt::Resource::Resource(const std::string &filename)
  : db(NULL)
{ load(filename); }


bt::Resource::~Resource(void)
{ XrmDestroyDatabase(db); }


void bt::Resource::load(const std::string &filename) {
  XrmDestroyDatabase(db);
  db = XrmGetFileDatabase(expandTilde(filename).c_str());
}


void bt::Resource::save(const std::string &filename) {
  if (!valid())
    return;
  XrmPutFileDatabase(db, expandTilde(filename).c_str());
}


void bt::Resource::merge(const std::string &filename)
{ XrmCombineFileDatabase(expandTilde(filename).c_str(), &db, true); }


std::string bt::Resource::read(const char* name, const char* classname,
                               const char* default_value) const {
  XrmValue value;
  char *value_type;
  if (XrmGetResource(db, name, classname, &value_type, &value))
    return std::string(value.addr, value.size - 1);
  return std::string(default_value);
}


std::string bt::Resource::read(const std::string& name,
                               const std::string& classname,
                               const std::string& default_value) const {
  XrmValue value;
  char *value_type;
  if (XrmGetResource(db, name.c_str(), classname.c_str(),
                     &value_type, &value))
    return std::string(value.addr, value.size - 1);
  return default_value;
}


int bt::Resource::read(const char* name, const char* classname,
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


unsigned int bt::Resource::read(const char* name, const char* classname,
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


long bt::Resource::read(const char* name, const char* classname,
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


unsigned long bt::Resource::read(const char* name, const char* classname,
                                 unsigned long default_value) const {
  XrmValue value;
  char *value_type;
  if (XrmGetResource(db, name, classname, &value_type, &value)) {
    unsigned long output;
    sscanf(value.addr, "%lu", &output);
    return output;
  }
  return default_value;
}


bool bt::Resource::read(const char* name, const char* classname,
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


double bt::Resource::read(const char* name, const char* classname,
                          double default_value) const {
  XrmValue value;
  char *value_type;
  if (XrmGetResource(db, name, classname, &value_type, &value)) {
    double output;
    sscanf(value.addr, "%lf", &output);
    return output;
  }
  return default_value;
}


void bt::Resource::write(const char *resource, const std::string &value)
{ write(resource, value.c_str()); }


void bt::Resource::write(const char* resource, const char* value)
{ XrmPutStringResource(&db, resource, value); }


void bt::Resource::write(const char* resource, int value) {
  char tmp[16];
  sprintf(tmp, "%d", value);
  write(resource, tmp);
}


void bt::Resource::write(const char* resource, unsigned int value) {
  char tmp[16];
  sprintf(tmp, "%u", value);
  write(resource, tmp);
}


void bt::Resource::write(const char* resource, long value) {
  char tmp[64];
  sprintf(tmp, "%ld", value);
  write(resource, tmp);
}


void bt::Resource::write(const char* resource, unsigned long value) {
  char tmp[64];
  sprintf(tmp, "%lu", value);
  write(resource, tmp);
}


void bt::Resource::write(const char* resource, bool value)
{ write(resource, boolAsString(value)); }


void bt::Resource::write(const char* resource, double value) {
  char tmp[80];
  sprintf(tmp, "%f", value);
  write(resource, tmp);
}
