// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Clientmenu.cc for Blackbox - an X11 Window manager
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

#include "Clientmenu.hh"
#include "Window.hh"
#include "Screen.hh"

#include <assert.h>


Clientmenu::Clientmenu(bt::Application &app, BScreen& screen,
                       unsigned int workspace)
  : bt::Menu(app, screen.screenNumber()),
    _workspace(workspace), _screen(screen) {
  setAutoDelete(false);
  showTitle();
}


void Clientmenu::itemClicked(unsigned int id, unsigned int button) {
  BlackboxWindow *window = _screen.getWindow(_workspace, id);
  assert(window != 0);

  if (_workspace != _screen.getCurrentWorkspaceID()) {
    if (button == 2) window->deiconify(true, false);
    else  _screen.changeWorkspaceID(_workspace);
  }

  _screen.raiseWindow(window);
  window->setInputFocus();
}
