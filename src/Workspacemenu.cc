// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Workspacemenu.cc for Blackbox - an X11 Window manager
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

#include "Workspacemenu.hh"

extern "C" {
#include <assert.h>
}

#include "Screen.hh"
#include "Workspace.hh"
#include "i18n.hh"


Workspacemenu::Workspacemenu(bt::Application &app, unsigned int screen,
                             BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen) {
  setAutoDelete(false);
  setTitle(bt::i18n(WorkspacemenuSet, WorkspacemenuWorkspacesTitle,
                    "Workspaces"));
  showTitle();

  insertItem(bt::i18n(WorkspacemenuSet, WorkspacemenuNewWorkspace,
                      "New Workspace"), 497);
  insertItem(bt::i18n(WorkspacemenuSet, WorkspacemenuRemoveLast,
                      "Remove Last"), 498);
  insertSeparator();
}


void Workspacemenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1)
    return;

  switch (id) {
  case 497:
    _bscreen->addWorkspace();
    break;

  case 498:
    _bscreen->removeLastWorkspace();
    break;

  case 499: // iconmenu
    break;

  default:
    assert(id < _bscreen->getWorkspaceCount());
    if (_bscreen->getCurrentWorkspaceID() != id) {
      _bscreen->changeWorkspaceID(id);
      hideAll();
    }
    break;
  } // switch
}
