// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Rootmenu.cc for Blackbox - an X11 Window manager
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

// stupid macros needed to access some functions in version 2 of the GNU C
// library
#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "blackbox.hh"
#include "Rootmenu.hh"
#include "Util.hh"

#include <string>
using std::string;


Rootmenu::Rootmenu(int scr)
    : Basemenu(scr)
{
}

void Rootmenu::itemClicked(const Item &item, int button)
{
  if (button != 1)
    return;

  switch(item.function()) {
  case Execute:
    bexec(item.command(),
          BaseDisplay::instance()->screenInfo(screenNumber())->displayString());
    break;

  case Restart:
    Blackbox::instance()->restart();
    break;

  case RestartOther:
    Blackbox::instance()->restart(item.command().c_str());
    break;

  case Exit:
    Blackbox::instance()->shutdown();
    break;

  case SetStyle:
    Blackbox::instance()->saveStyleFilename(item.command().c_str());
    // fall through intended

  case Reconfigure:
    Blackbox::instance()->reconfigure();
    break;
  }
}
