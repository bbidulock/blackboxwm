// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Workspace.cc for Blackbox - an X11 Window manager
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

#include "Workspace.hh"
#include "Clientmenu.hh"
#include "Screen.hh"
#include "Window.hh"

#include <Util.hh>

#include <assert.h>


Workspace::Workspace(BScreen *scrn, unsigned int i) {
  _screen = scrn;

  _id = i;

  clientmenu = new Clientmenu(*_screen->getBlackbox(), *_screen, _id);

  lastfocus = (BlackboxWindow *) 0;

  setName(_screen->resource().nameOfWorkspace(i));
}


const std::string& Workspace::name(void) const
{ return _screen->resource().nameOfWorkspace(_id); }


void Workspace::setName(const std::string& new_name) {
  std::string the_name;

  if (! new_name.empty()) {
    the_name = new_name;
  } else {
    char default_name[80];
    sprintf(default_name, "Workspace %u", _id + 1);
    the_name = default_name;
  }

  _screen->resource().saveWorkspaceName(_id, the_name);
  clientmenu->setTitle(the_name);
}


void Workspace::addWindow(BlackboxWindow *win) {
  assert(win != 0);
  assert(win->workspace() == _id || win->workspace() == bt::BSENTINEL);

  win->setWorkspace(_id);
  const std::string s = bt::ellideText(win->getTitle(), 60, "...");
  int wid = clientmenu->insertItem(s);
  win->setWindowNumber(wid);
}


void Workspace::removeWindow(BlackboxWindow *win) {
  assert(win != 0 && win->workspace() == _id);

  clientmenu->removeItem(win->windowNumber());
  win->setWindowNumber(bt::BSENTINEL);
  win->setWorkspace(bt::BSENTINEL);
}
