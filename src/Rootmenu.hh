// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Rootmenu.hh for Blackbox - an X11 Window manager
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

#ifndef   __Rootmenu_hh
#define   __Rootmenu_hh

#include "Menu.hh"

#include <map>

// forward declaration
class BScreen;


class Rootmenu : public bt::Menu {
public:
  Rootmenu(bt::Application &app, unsigned int screen, BScreen *bscreen);

  void insertFunction(const std::string &label,
                      unsigned int function,
                      const std::string &exec = std::string(),
                      unsigned int id = ~0u,
                      unsigned int index = ~0u);

protected:
  void itemClicked(unsigned int id, unsigned int button);

private:
  BScreen *_bscreen;

  struct _function {
    _function(unsigned int f, const std::string &s) : func(f), string(s) { }
    unsigned int func;
    std::string string;
  };
  typedef std::map<unsigned int,const _function> FunctionMap;
  FunctionMap _funcmap;
};

#endif // __Rootmenu_hh

