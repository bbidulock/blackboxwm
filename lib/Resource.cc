// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
//
// Blackbox - an X11 Window manager
//
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh at debian.org>
// Copyright (c) 1997 - 2000, 2002 Bradley T Hughes <bhughes at trolltech.com>
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


bt::Resource::Resource(void) : db(NULL) {

}

bt::Resource::Resource(const std::string &filename) : db(NULL) {
  load(filename);
}

bt::Resource::~Resource(void) {
  XrmDestroyDatabase(db);
}

void bt::Resource::load(const std::string &filename) {
  XrmDestroyDatabase(db);
  db = XrmGetFileDatabase(expandTilde(filename).c_str());
}

void bt::Resource::save(const std::string &filename) {
  if (! valid()) return;
  XrmPutFileDatabase(db, expandTilde(filename).c_str());
}

void bt::Resource::merge(const std::string &filename) {
  XrmCombineFileDatabase(expandTilde(filename).c_str(), &db, true);
}

std::string bt::Resource::read(const std::string &name,
                               const std::string &classname,
                               const std::string &default_value) const {
  XrmValue value;
  char *value_type;
  if (XrmGetResource(db, name.c_str(), classname.c_str(), &value_type, &value))
    return std::string(value.addr, value.size);
  return default_value;
}

void bt::Resource::write(const std::string &resource,
                         const std::string &value)
{
  XrmPutStringResource(&db, resource.c_str(), value.c_str());
}
