// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Clientmenu.cc for Blackbox - an X11 Window manager
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

#include "Clientmenu.hh"

extern "C" {
#include <assert.h>
}

#include "Window.hh"
#include "Workspace.hh"


Clientmenu::Clientmenu(bt::Application &app, unsigned int screen,
                       Workspace *workspace)
  : bt::Menu(app, screen), _workspace(workspace) {
  setAutoDelete(false);
  showTitle();
}


void Clientmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button > 2) return;

  BlackboxWindow *window = _workspace->getWindow(id);
  assert(window != 0);

  if (! _workspace->isCurrent()) {
    if (button == 1) _workspace->setCurrent();
    else if (button == 2) window->deiconify(true, false);
  }

  _workspace->raiseWindow(window);
  window->setInputFocus();
}
