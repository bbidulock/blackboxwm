//
// workspacemenu.cc for Blackbox - an X11 Window manager
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

#define _GNU_SOURCE
#include "workspace.hh"
#include "window.hh"
#include "session.hh"


// *************************************************************************
// Workspace Menu class code
// *************************************************************************
//
// allocations:
// none - items are redraw from char**'s that point to the workspace names
//
// *************************************************************************

WorkspaceMenu::WorkspaceMenu(Workspace *w, BlackboxSession *s) :
  BlackboxMenu(s)
{
  session = s;
  workspace = w;
  setMovable(False);
}


void WorkspaceMenu::showMenu(void) {
  BlackboxMenu::showMenu();
}


void WorkspaceMenu::hideMenu(void) {
  BlackboxMenu::hideMenu();
}


void WorkspaceMenu::moveMenu(int x, int y) {
  BlackboxMenu::moveMenu(x, y);
}


void WorkspaceMenu::updateMenu(void) {
  BlackboxMenu::updateMenu();
}


int WorkspaceMenu::insert(char **u) {
  return BlackboxMenu::insert(u);
}


int WorkspaceMenu::remove(int i) {
  return BlackboxMenu::remove(i);
}


void WorkspaceMenu::titlePressed(int) { }


void WorkspaceMenu::titleReleased(int) { }


void WorkspaceMenu::itemPressed(int, int) { }


void WorkspaceMenu::itemReleased(int button, int item) {
  if (button == 1) {
    if (workspace->ws_manager->currentWorkspaceID() !=
	workspace->workspace_id)
      workspace->ws_manager->changeWorkspaceID(workspace->workspace_id);

    if (item >= 0 && item < workspace->workspace_list->count()) {  
      hideMenu();
      BlackboxWindow *w = workspace->workspace_list->at(item);
      if (w->isIconic()) w->deiconifyWindow();
      session->raiseWindow(w);
      w->setInputFocus();

      workspace->ws_manager->hideMenu();
    } 
  }
}


// *************************************************************************
// Workspace Manager Menu class code
// *************************************************************************
//
// allocations:
// none - workspaces created are placed under the control of the workspace
//    manager
//
// *************************************************************************

WorkspaceManagerMenu::WorkspaceManagerMenu(WorkspaceManager *m,
					   BlackboxSession *s) :
  BlackboxMenu(s)
{
  ws_manager = m;
  hideTitle();
  setMovable(False);
}


void WorkspaceManagerMenu::showMenu(void) {
  BlackboxMenu::showMenu();
}


void WorkspaceManagerMenu::hideMenu(void) {
  BlackboxMenu::hideMenu();
}


void WorkspaceManagerMenu::moveMenu(int x, int y) {
  BlackboxMenu::moveMenu(x, y);
}


void WorkspaceManagerMenu::updateMenu(void) {
  BlackboxMenu::updateMenu();
}


int WorkspaceManagerMenu::insert(char *l) {
  return BlackboxMenu::insert(l);
}


int WorkspaceManagerMenu::insert(char *l, WorkspaceMenu *m) {
  return BlackboxMenu::insert(l, m);
}


void WorkspaceManagerMenu::itemPressed(int, int) { }


void WorkspaceManagerMenu::itemReleased(int button, int item) {
  if (button == 1)
    if (item == 0) {
      if (ws_manager->workspaces_list->count() < 25) {
	Workspace *wkspc = new Workspace(ws_manager,
					 ws_manager->workspaces_list->count());
	ws_manager->workspaces_list->append(wkspc);
	insert(wkspc->name(), wkspc->menu());
	XSync(ws_manager->display, False);
	updateMenu();
      }
    } else if (item == 1) {
      int i = ws_manager->workspaces_list->count();
      if (i > 1) {
	Workspace *wkspc = ws_manager->workspaces_list->at(i - 1);
	if (ws_manager->currentWorkspaceID() == (i - 1))
	  ws_manager->changeWorkspaceID(i - 2);
          XClearWindow(ws_manager->display,
		       ws_manager->frame.workspace_button);
          XDrawString(ws_manager->display, ws_manager->frame.workspace_button,
                      ws_manager->buttonGC, 4, 3 +
                      ws_manager->session->titleFont()->ascent,
                      ws_manager->frame.title,
		      strlen(ws_manager->frame.title));
	wkspc->removeAll();
	ws_manager->workspaces_list->remove(i - 1);
	BlackboxMenu::remove(i + 1);
	delete wkspc;
	XSync(ws_manager->display, False);
	updateMenu();
      }
    } else if ((item - 2) != ws_manager->currentWorkspaceID()) {
      ws_manager->changeWorkspaceID(item - 2);
      XSetWindowBackgroundPixmap(ws_manager->display,
				 ws_manager->frame.workspace_button,
				 ws_manager->frame.button);
      XClearWindow(ws_manager->display, ws_manager->frame.workspace_button);
      XDrawString(ws_manager->display, ws_manager->frame.workspace_button,
		  ws_manager->buttonGC, 4, 3 +
		  ws_manager->session->titleFont()->ascent,
		  ws_manager->frame.title, strlen(ws_manager->frame.title));
      hideMenu();
    }
}
