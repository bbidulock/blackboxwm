// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Resource.hh for Blackbox - an X11 Window manager
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

#ifndef __Resource_hh
#define __Resource_hh

#include <string>

// forward declaration
typedef struct _XrmHashBucketRec *XrmDatabase;


namespace bt {

  inline const char* boolAsString(bool b)
  { return (b) ? "True" : "False"; }

  class Resource {
  public:
    Resource(void);
    explicit Resource(const std::string &filename);
    ~Resource(void);

    inline bool valid(void) const
    { return db != NULL; }

    void load(const std::string &filename);
    void save(const std::string &filename);
    void merge(const std::string &filename);

    std::string read(const char* name,
                     const char* classname,
                     const char* default_value = "") const;
    std::string read(const std::string &name,
                     const std::string &classname,
                     const std::string &default_value = std::string()) const;

    int read(const char* name, const char* classname,
             int default_value) const;
    unsigned int read(const char* name, const char* classname,
                      unsigned int default_value) const;

    long read(const char* name, const char* classname,
              long default_value) const;
    unsigned long read(const char* name, const char* classname,
                       unsigned long default_value) const;

    bool read(const char* name, const char* classname,
              bool default_value) const;

    double read(const char* name, const char* classname,
                double default_value) const;

    void write(const char *resource, const std::string &value);
    void write(const char* resource, const char* value);
    void write(const char* resource, int value);
    void write(const char* resource, unsigned int value);
    void write(const char* resource, long value);
    void write(const char* resource, unsigned long value);
    void write(const char* resource, bool value);
    void write(const char* resource, double value);

  private:
    XrmDatabase db;
  };

} // namespace bt

#endif // __Resource_hh
