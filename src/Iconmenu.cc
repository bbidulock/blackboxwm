// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Icon.cc for Blackbox - an X11 Window manager
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "Iconmenu.hh"

extern "C" {
#include <assert.h>
}

#include "Screen.hh"
#include "Window.hh"
#include "i18n.hh"


Iconmenu::Iconmenu(bt::Application &app, unsigned int screen,
                   BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen) {
  setAutoDelete(false);
  setTitle(bt::i18n(IconSet, IconIcons, "Icons"));
  showTitle();
}


void Iconmenu::itemClicked(unsigned int id, unsigned int) {
  assert(id < _bscreen->getIconCount());

  BlackboxWindow *window = _bscreen->getIcon(id);
  assert(window != 0);

  window->deiconify();
  window->setInputFocus();
}
