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

extern "C" {
#include <assert.h>
}

#include "Workspacemenu.hh"
#include "Iconmenu.hh"
#include "Screen.hh"
#include "i18n.hh"

static const unsigned int NewWorkspaceId = 497u;
static const unsigned int RemoveLastId   = 498u;
static const unsigned int IconmenuId     = 499u;


Workspacemenu::Workspacemenu(bt::Application &app, unsigned int screen,
                             BScreen *bscreen)
  : bt::Menu(app, screen), _bscreen(bscreen) {
  setAutoDelete(false);
  setTitle(bt::i18n(WorkspacemenuSet, WorkspacemenuWorkspacesTitle,
                    "Workspaces"));
  showTitle();

  insertItem(bt::i18n(WorkspacemenuSet, WorkspacemenuNewWorkspace,
                      "New Workspace"), NewWorkspaceId);
  insertItem(bt::i18n(WorkspacemenuSet, WorkspacemenuRemoveLast,
                      "Remove Last"), RemoveLastId);
  insertSeparator();
}


void Workspacemenu::insertIconMenu(Iconmenu *iconmenu) {
  insertSeparator();
  insertItem(bt::i18n(IconSet, IconIcons, "Icons"), iconmenu, IconmenuId);
}


void Workspacemenu::itemClicked(unsigned int id, unsigned int) {
  switch (id) {
  case NewWorkspaceId:
    _bscreen->addWorkspace();
    break;

  case RemoveLastId:
    _bscreen->removeLastWorkspace();
    break;

  case IconmenuId:
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
