//
// Workspacemenu.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@arn.net
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// (See the included file COPYING / GPL-2.0)
//

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "WorkspaceManager.hh"
#include "Workspacemenu.hh"
#include "Workspace.hh"


Workspacemenu::Workspacemenu(Blackbox *bb, WorkspaceManager *wsm) :
  Basemenu(bb)
{
  wsManager = wsm;
  setTitleVisibility(False);
  setMovable(False);
  defaultMenu();

  insert("New Workspace");
  insert("Remove Last");
}


Workspacemenu::~Workspacemenu(void) {
  
}


void Workspacemenu::itemSelected(int button, int index) {
  if (button == 1) {
    if (index == 0) {
      wsManager->addWorkspace();
    } else if (index == 1) {
      wsManager->removeLastWorkspace();
    } else if (wsManager->currentWorkspace()->workspaceID() != (index - 1)) {
      wsManager->changeWorkspaceID(index - 1);
      Hide();
    }
  }
}
