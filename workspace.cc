//
// workspace.cc for Blackbox - an X11 Window manager
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
#include "graphics.hh"

#include <stdio.h>


WorkspaceMenu::WorkspaceMenu(Workspace *w, BlackboxSession *s) :
  BlackboxMenu(s) { workspace = w; setMovable(False); }
void WorkspaceMenu::showMenu(void)
{ BlackboxMenu::showMenu(); }
void WorkspaceMenu::hideMenu(void)
{ BlackboxMenu::hideMenu(); }
void WorkspaceMenu::moveMenu(int x, int y)
{ BlackboxMenu::moveMenu(x, y); }
void WorkspaceMenu::updateMenu(void)
{ BlackboxMenu::updateMenu(); }
int WorkspaceMenu::insert(char **u)
{ return BlackboxMenu::insert(u); }
int WorkspaceMenu::remove(int i)
{ return BlackboxMenu::remove(i); }
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
      w->raiseWindow();
      w->setInputFocus();

      workspace->ws_manager->hideMenu();
    } 
  }
}


WorkspaceManagerMenu::WorkspaceManagerMenu(WorkspaceManager *m,
					   BlackboxSession *s) :
  BlackboxMenu(s)
{ ws_manager = m; hideTitle(); setMovable(False); }
void WorkspaceManagerMenu::showMenu(void)
{ BlackboxMenu::showMenu(); }
void WorkspaceManagerMenu::hideMenu(void)
{ BlackboxMenu::hideMenu(); }
void WorkspaceManagerMenu::moveMenu(int x, int y)
{ BlackboxMenu::moveMenu(x, y); }
void WorkspaceManagerMenu::updateMenu(void)
{ BlackboxMenu::updateMenu(); }
int WorkspaceManagerMenu::insert(char *l)
{ return BlackboxMenu::insert(l); }
int WorkspaceManagerMenu::insert(char *l, WorkspaceMenu *m)
{ return BlackboxMenu::insert(l, m); }


void WorkspaceManagerMenu::itemPressed(int, int) { }
void WorkspaceManagerMenu::itemReleased(int button, int item) {
  if (button == 1)
    if (item == 0) {
      Workspace *wkspc = new Workspace(ws_manager,
				       ws_manager->workspaces_list->count());
      ws_manager->workspaces_list->append(wkspc);
      insert(wkspc->name(), wkspc->menu());
      updateMenu();
    } else if (item == 1) {
      int i = ws_manager->workspaces_list->count();
      if (i > 1) {
	Workspace *wkspc = ws_manager->workspaces_list->at(i - 1);
	if (ws_manager->currentWorkspaceID() == (i - 1))
	  ws_manager->changeWorkspaceID(i - 2);
          XClearWindow(ws_manager->display, ws_manager->frame.workspace_button);
          XDrawString(ws_manager->display, ws_manager->frame.workspace_button,
                      ws_manager->buttonGC, 4, 2 +
                      ws_manager->session->titleFont()->ascent,
                      ws_manager->frame.title, strlen(ws_manager->frame.title));
	wkspc->removeAll();
	ws_manager->workspaces_list->remove(i - 1);
	BlackboxMenu::remove(i + 1);
	delete wkspc;
	updateMenu();
      }
    } else if ((item - 2) != ws_manager->currentWorkspaceID()) {
      ws_manager->changeWorkspaceID(item - 2);
      XSetWindowBackgroundPixmap(ws_manager->display,
				 ws_manager->frame.workspace_button,
				 ws_manager->frame.button);
      XClearWindow(ws_manager->display, ws_manager->frame.workspace_button);
      XDrawString(ws_manager->display, ws_manager->frame.workspace_button,
		  ws_manager->buttonGC, 4, 2 +
		  ws_manager->session->titleFont()->ascent,
		  ws_manager->frame.title, strlen(ws_manager->frame.title));
      hideMenu();
    }
}


Workspace::Workspace(WorkspaceManager *m, int id) {
  ws_manager = m;
  workspace_id = id;

  workspace_list = new llist<BlackboxWindow>;
  workspace_menu = new WorkspaceMenu(this, ws_manager->Session());

  workspace_name = new char[20];
  sprintf(workspace_name, "Workspace %d", workspace_id);
  workspace_menu->setMenuLabel(workspace_name);
  workspace_menu->updateMenu();
}


Workspace::~Workspace(void) {
  delete workspace_list;
  delete workspace_menu;
  delete [] workspace_name;
}


void Workspace::showMenu(void) { workspace_menu->showMenu(); }
void Workspace::hideMenu(void) { workspace_menu->hideMenu(); }
void Workspace::moveMenu(int x, int y) { workspace_menu->moveMenu(x, y); }
void Workspace::updateMenu(void) { workspace_menu->updateMenu(); }


const int Workspace::addWindow(BlackboxWindow *w) {
  w->setWorkspace(workspace_id);
  w->setWindowNumber(workspace_list->count());
  workspace_list->append(w);
  workspace_menu->insert(w->Title());
  workspace_menu->updateMenu();
  return w->windowNumber();
}


const int Workspace::removeWindow(BlackboxWindow *w) {
  workspace_menu->remove((const int) w->windowNumber());
  workspace_list->remove((const int) w->windowNumber());
  for (int i = 0; i < workspace_list->count(); ++i)
    workspace_list->at(i)->setWindowNumber(i);

  workspace_menu->updateMenu();
  return workspace_list->count();
}


int Workspace::showAll(void) {
  BlackboxWindow *win;

  int i;
  for (i = 0; i < workspace_list->count(); ++i) {
    win = workspace_list->at(i);
    if (! win->isIconic())
      win->deiconifyWindow();
  }
  
  return i;
}


int Workspace::hideAll(void) {
  BlackboxWindow *win;

  int i;
  for (i = 0; i < workspace_list->count(); ++i) {
    win = workspace_list->at(i);
    if (! win->isIconic())
      win->withdrawWindow();
  }

  return i;
} 


int Workspace::removeAll(void) {
  BlackboxWindow *win;

  int i;
  for (i = 0; i < workspace_list->count(); ++i) {
    win = workspace_list->at(i);
    ws_manager->currentWorkspace()->addWindow(win);
    if (! win->isIconic())
      win->iconifyWindow();
  }

  return i;
}


void Workspace::Dissociate(void) {
  BlackboxWindow *win;
  Display *display = ws_manager->Session()->control();
  int i;

  XGrabServer(display);
  for (i = 0; i < workspace_list->count(); ++i) {
    win = workspace_list->at(i);
    XUnmapWindow(display, win->frameWindow());
    XReparentWindow(display, win->clientWindow(),
		    ws_manager->Session()->Root(), win->XFrame(),
		    win->YFrame());
    XMoveResizeWindow(display, win->clientWindow(), win->XFrame(),
		      win->YFrame(), win->clientWidth(), win->clientHeight());
    XMapWindow(display, win->clientWindow());
  }

  XSync(display, 0);
  XUngrabServer(display);
}


int Workspace::menuVisible(void)
{ return workspace_menu->menuVisible(); }


WorkspaceManager::WorkspaceManager(BlackboxSession *s, int c) {
  Workspace *wkspc = 0;
  session = s;
  
  workspaces_list = new llist<Workspace>;
  ilist = new llist<BlackboxIcon>;
  workspaces_menu = new WorkspaceManagerMenu(this, session);

  frame.title = new char[20];
  sprintf(frame.title, "Workspace 0");
  workspaces_menu->insert("New Workspace");
  workspaces_menu->insert("Delete last");

  if (c != 0) {
    for (int i = 0; i < c; ++i) {
      wkspc = new Workspace(this, i);
      workspaces_list->append(wkspc);
      workspaces_menu->insert(wkspc->name(), wkspc->menu());
    }
  } else {
    wkspc = new Workspace(this, 0);
    workspaces_list->append(wkspc);
    workspaces_menu->insert(wkspc->name(),wkspc->menu());
  }

  workspaces_menu->updateMenu();
  current = workspaces_list->at(0);

  /*
    lets do something new with the Workspace Manager and create a kind of tool
     bar that lives on the left side of the root window... and have it house
     the workspace manager menu (which in turn houses the workspace menus)
  */

  sub = False;
  display = session->control();
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWOverrideRedirect |CWCursor|CWEventMask; 
  
  attrib_create.background_pixmap = None;
  attrib_create.background_pixel = session->frameColor().pixel;
  attrib_create.border_pixel = session->frameColor().pixel;
  attrib_create.override_redirect = True;
  attrib_create.cursor = session->sessionCursor();
  attrib_create.event_mask = NoEventMask;
  
  frame.frame_w = 100;
  frame.frame_h = session->YResolution();
  frame.base =
    XCreateWindow(display, session->Root(), 0, 0, frame.frame_w,
		  frame.frame_h, 0, session->Depth(), InputOutput,
		  session->visual(), create_mask, &attrib_create);

  attrib_create.event_mask = StructureNotifyMask|SubstructureNotifyMask|
    SubstructureRedirectMask|ButtonPressMask|ButtonReleaseMask|
    ButtonMotionMask|ExposureMask;
  frame.window =
    XCreateWindow(display, frame.base, 0, 0, frame.frame_w, frame.frame_h,
		  0, session->Depth(), InputOutput, session->visual(),
		  create_mask, &attrib_create);
  XSaveContext(display, frame.window, session->wsContext(), (XPointer) this);

  BImage image(session, frame.frame_w, frame.frame_h, session->Depth(),
	       session->frameColor());
  Pixmap p = image.renderImage(session->frameTexture(), 1,
			       session->frameColor(),
			       session->frameToColor());

  XSetWindowBackgroundPixmap(display, frame.window, p);
  if (p) XFreePixmap(display, p);

  frame.button_w = frame.frame_w - 6;
  frame.button_h = session->titleFont()->ascent +
    session->titleFont()->descent + 8;
  frame.workspace_button =
    XCreateWindow(display, frame.window, 3, 3, frame.button_w, frame.button_h,
		  0, session->Depth(), InputOutput, session->visual(),
		  create_mask, &attrib_create);
  XSaveContext(display, frame.workspace_button, session->wsContext(),
	       (XPointer) this);
  XGCValues gcv;
  gcv.font = session->titleFont()->fid;
  gcv.foreground = session->getColor("white");
  buttonGC = XCreateGC(display, frame.workspace_button,
		       GCFont|GCForeground, &gcv);

  BImage bimage(session, frame.button_w, frame.button_h, session->Depth(),
		session->buttonColor());

  frame.button = bimage.renderImage(session->frameTexture(), 0,
				    session->focusColor(),
				    session->focusToColor());

  frame.pbutton = bimage.renderInvertedImage(session->frameTexture(), 0,
					     session->focusColor(),
					     session->focusToColor());

  XSetWindowBackgroundPixmap(display, frame.workspace_button, frame.button);
  
  frame.icon =
    XCreateWindow(display, frame.window, 3, frame.frame_h / 2, frame.button_w,
		  (frame.frame_h / 2) - 4, 0, session->Depth(), InputOutput,
		  session->visual(), create_mask, &attrib_create);
  XSaveContext(display, frame.icon, session->wsContext(), (XPointer) this);
  
  BImage iimage(session, frame.button_w, (frame.frame_h / 2) - 4,
		session->Depth(), session->iconColor());

  p = iimage.renderSolidImage(BlackboxSession::B_TextureSSolid, 0,
			      session->iconColor());
  
  XSetWindowBackgroundPixmap(display, frame.icon, p);
  if (p) XFreePixmap(display, p);
  
  XClearWindow(display, frame.window);
  XClearWindow(display, frame.workspace_button);
  XClearWindow(display, frame.icon);

  XMapSubwindows(display, frame.window);
  XMapSubwindows(display, frame.base);
  XMapRaised(display, frame.base);

  XDrawString(display, frame.workspace_button, buttonGC, 4, 2 +
	      session->titleFont()->ascent, frame.title, strlen(frame.title));
}


WorkspaceManager::~WorkspaceManager(void) {
  DissociateAll();
  delete ilist;
  delete workspaces_menu;

  for (int i = 0; i < workspaces_list->count(); ++i) {
    delete workspaces_list->at(0);
    workspaces_list->remove(0);
  }
  delete workspaces_list;

  if (frame.button) XFreePixmap(display, frame.button);
  if (frame.pbutton) XFreePixmap(display, frame.pbutton);

  XDeleteContext(display, frame.workspace_button, session->wsContext());
  XDestroyWindow(display, frame.workspace_button);
  XDeleteContext(display, frame.workspace_button, session->wsContext());
  XDestroyWindow(display, frame.window);
  XDestroyWindow(display, frame.base);
}


void WorkspaceManager::DissociateAll(void) {
  for (int i = 0; i < workspaces_list->count(); ++i)
    workspaces_list->at(i)->Dissociate();
}


Workspace *WorkspaceManager::workspace(int w) {
  return workspaces_list->at(w);
}


void WorkspaceManager::changeWorkspaceID(int id) {
  if (id != current->workspaceID()) {
    current->hideMenu();
    current->hideAll();
    current = workspace(id);
    sprintf(frame.title, "Workspace %d", id);
    XClearWindow(display, frame.workspace_button);
    XDrawString(display, frame.workspace_button, buttonGC, 4, 2 +
                session->titleFont()->ascent, frame.title,
		strlen(frame.title));
    current->showAll();
  }
}


void WorkspaceManager::arrangeIcons(void) {
  int i;
  BlackboxIcon *icon = NULL;
  
  XGrabServer(display);
  for (i = 0; i < ilist->count(); ++i) {
    icon = ilist->at(i);
    icon->move(4, (icon->Height() - 1) * (ilist->count() - (i + 1)) +
	       (frame.frame_h / 2) + 1);
    XMoveWindow(display, icon->iconWindow(), 1,
		(icon->Height() - 1) * (ilist->count() - (i + 1)) + 1);
    icon->exposeEvent(NULL);
  }
  
  XUngrabServer(display);
}


void WorkspaceManager::buttonPressEvent(XButtonEvent *be) {
  if (be->window == frame.window) {
    if (be->button == 1)
      XRaiseWindow(display, frame.window);
    else if (be->button == 2)
      XLowerWindow(display, frame.window);
  } else if (be->window == frame.workspace_button)
    if (be->button == 1)
      if (! workspaces_menu->menuVisible())
	showMenu();
}


void WorkspaceManager::buttonReleaseEvent(XButtonEvent *re) {
  if (re->window == frame.workspace_button) {
    if (re->button == 1) {
      if (sub)
	sub = False;
      else
	hideMenu();
    }
  }
}


void WorkspaceManager::exposeEvent(XExposeEvent *ee) {
  if (ee->window == frame.workspace_button)
    XDrawString(display, frame.workspace_button, buttonGC, 4, 2 +
		session->titleFont()->ascent, frame.title,
		strlen(frame.title));
}


void WorkspaceManager::showMenu(void) {
  XSetWindowBackgroundPixmap(display, frame.workspace_button,
			     frame.pbutton);
  XClearWindow(display, frame.workspace_button);
  XDrawString(display, frame.workspace_button, buttonGC, 4, 2 +
	      session->titleFont()->ascent, frame.title,
	      strlen(frame.title));
  workspaces_menu->moveMenu(3, 3 + frame.button_h);
  workspaces_menu->showMenu();
  sub = True;
}


void WorkspaceManager::hideMenu(void) {
  XSetWindowBackgroundPixmap(display, frame.workspace_button,
			     frame.button);
  XClearWindow(display, frame.workspace_button);
  XDrawString(display, frame.workspace_button, buttonGC, 4, 2 +
	      session->titleFont()->ascent, frame.title,
	      strlen(frame.title));
  workspaces_menu->hideMenu();
}
