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
#include <sys/time.h>


// *************************************************************************
// Workspace class code
// *************************************************************************

Workspace::Workspace(WorkspaceManager *m, int id) {
  ws_manager = m;
  workspace_id = id;
  window_stack = 0;

  workspace_list = new llist<BlackboxWindow>;
  workspace_menu = new WorkspaceMenu(this, ws_manager->Session());

  workspace_name = new char[13];
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

  Window *tmp_stack = new Window[workspace_list->count()];
  int i;
  for (i = 0; i < (workspace_list->count() - 1); ++i)
    *(tmp_stack + i) = *(window_stack + i);

  *(tmp_stack + i) = w->frameWindow();

  delete [] window_stack;
  window_stack = tmp_stack;
  if (ws_manager->currentWorkspaceID() == workspace_id)
    ws_manager->stackWindows(window_stack, workspace_list->count());

  return w->windowNumber();
}


const int Workspace::removeWindow(BlackboxWindow *w) {
  Window *tmp_stack = new Window[workspace_list->count() - 1];
  int ii = 0;
  for (int i = 0; i < workspace_list->count(); ++i)
    if (*(window_stack + i) != w->frameWindow())
      *(tmp_stack + (ii++)) = *(window_stack + i);

  delete [] window_stack;
  window_stack = tmp_stack;
  if (ws_manager->currentWorkspaceID() == workspace_id)
    ws_manager->stackWindows(window_stack, workspace_list->count() - 1);

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
  ws_manager->stackWindows(window_stack, workspace_list->count());
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


void Workspace::raiseWindow(BlackboxWindow *w) {
  // this just arranges the windows... and then passes it to the workspace
  // manager... which adds this stack to it's own stack... and the workspace
  // manager then tells the X server to restack the windows

  Window *tmp_stack = new Window[workspace_list->count()];
  int i = 0, ii = 0;

  if (w->isTransient()) raiseWindow(w->TransientFor());

  for (i = 0; i < workspace_list->count(); ++i)
    if (*(window_stack + i) != w->frameWindow())
      *(tmp_stack + (ii++)) = *(window_stack + i);
  
  *(tmp_stack + workspace_list->count() - 1) = w->frameWindow();
  
  for (i = 0; i < workspace_list->count(); ++i)
    *(window_stack + i) = *(tmp_stack + i);

  delete [] tmp_stack;
  if (w->hasTransient()) raiseWindow(w->Transient());
  ws_manager->stackWindows(window_stack, workspace_list->count());
}


void Workspace::lowerWindow(BlackboxWindow *w) {
  Window *tmp_stack = new Window[workspace_list->count()];
  int i, ii = 1;
  
  if (w->hasTransient()) lowerWindow(w->Transient());
  
  for (i = 0; i < workspace_list->count(); ++i)
    if (*(window_stack + i) != w->frameWindow())
      *(tmp_stack + (ii++)) = *(window_stack + i);
  
  *(tmp_stack) = w->frameWindow();
  
  for (i = 0; i < workspace_list->count(); ++i)
    *(window_stack + i) = *(tmp_stack + i);

  delete [] tmp_stack;
  if (w->isTransient()) lowerWindow(w->TransientFor());
  ws_manager->stackWindows(window_stack, workspace_list->count());
}


void Workspace::Reconfigure(void) {
  workspace_menu->Reconfigure();
  workspace_menu->updateMenu();

  for (int i = 0; i < workspace_list->count(); i++)
    workspace_list->at(i)->Reconfigure();
}


BlackboxWindow *Workspace::window(int index) {
  if ((index >= 0) && (index < workspace_list->count()))
    return workspace_list->at(index);
  else
    return 0;
}


const int Workspace::count(void)
{ return workspace_list->count(); }


// *************************************************************************
// Workspace manager class code
// *************************************************************************

WorkspaceManager::WorkspaceManager(BlackboxSession *s, int c) {
  debug = new Debugger('^');
#ifdef DEBUG
  debug->enable();
#endif
  debug->msg("%s: WorkspaceManager::WorkspaceManager\n", __FILE__);

  Workspace *wkspc = 0;
  session = s;
  
  workspaces_list = new llist<Workspace>;
  ilist = new llist<BlackboxIcon>;
  workspaces_menu = new WorkspaceManagerMenu(this, session);

  frame.title = new char[13];
  sprintf(frame.title, "Workspace 0");
  workspaces_menu->insert("New Workspace");
  workspaces_menu->insert("Delete last");

  if (c != 0) {
    if (c > 25) c = 25;
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
     box that lives on the left side of the root window... and have it house
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

  frame.button_w = XTextWidth(session->titleFont(), "Workspace 00", 13) + 8;
  frame.button_h = session->titleFont()->ascent +
    session->titleFont()->descent + 8; 

  frame.frame_w = frame.button_w + 6;
  frame.frame_h = session->YResolution() - 2;
  frame.base =
    XCreateWindow(display, session->Root(), 0, 0, frame.frame_w,
		  frame.frame_h, 1, session->Depth(), InputOutput,
		  session->visual(), create_mask, &attrib_create);

  attrib_create.event_mask = StructureNotifyMask|SubstructureNotifyMask|
    SubstructureRedirectMask|ButtonPressMask|ButtonReleaseMask|
    ButtonMotionMask|ExposureMask;

  frame.window =
    XCreateWindow(display, frame.base, 0, 0, frame.frame_w,
		  frame.frame_h, 0, session->Depth(), InputOutput,
		  session->visual(), create_mask, &attrib_create);
  XSaveContext(display, frame.window, session->wsContext(), (XPointer) this);

  BImage image(session, frame.frame_w, frame.frame_h,
	       session->Depth(), session->frameColor());
  Pixmap p = image.renderImage(session->toolboxTexture(), 1,
			       session->toolboxColor(),
			       session->toolboxToColor());

  XSetWindowBackgroundPixmap(display, frame.window, p);
  if (p) XFreePixmap(display, p);

  frame.workspace_button =
    XCreateWindow(display, frame.window, 3, 3, frame.button_w, frame.button_h,
		  0, session->Depth(), InputOutput, session->visual(),
		  create_mask, &attrib_create);
  XSaveContext(display, frame.workspace_button, session->wsContext(),
	       (XPointer) this);

 XGCValues gcv;
  gcv.font = session->titleFont()->fid;
  gcv.foreground = session->toolboxTextColor().pixel;
  buttonGC = XCreateGC(display, frame.workspace_button,
		       GCFont|GCForeground, &gcv);

  BImage bimage(session, frame.button_w, frame.button_h,
		session->Depth(), session->buttonColor());
  frame.button = bimage.renderImage(session->buttonTexture(), 0,
				    session->buttonColor(),
				    session->buttonToColor());
  frame.pbutton = bimage.renderInvertedImage(session->buttonTexture(), 0,
					     session->buttonColor(),
					     session->buttonToColor());

  XSetWindowBackgroundPixmap(display, frame.workspace_button, frame.button);
  
  frame.icon =
    XCreateWindow(display, frame.window, 3, frame.frame_h / 2,
		  frame.button_w, (frame.frame_h / 2) - 2, 0,
		  session->Depth(), InputOutput, session->visual(),
		  create_mask, &attrib_create);
  XSaveContext(display, frame.icon, session->wsContext(), (XPointer) this);
  
  BImage iimage(session, frame.button_w, (frame.frame_h / 2) - 2,
		session->Depth(), session->toolboxColor());
  p = iimage.renderInvertedImage(session->toolboxTexture(), 0,
				 session->toolboxColor(),
				 session->toolboxToColor());
  
  XSetWindowBackgroundPixmap(display, frame.icon, p);
  if (p) XFreePixmap(display, p);

  frame.clock =
    XCreateWindow(display, frame.window, 3,
		  frame.frame_h / 2 - frame.button_h - 2, frame.button_w,
		  frame.button_h, 0, session->Depth(), InputOutput,
		  session->visual(), create_mask, &attrib_create);
  XSaveContext(display, frame.clock, session->wsContext(), (XPointer) this);

  BImage cimage(session, frame.button_w, frame.button_h,
		session->Depth(), session->toolboxColor());
  p = cimage.renderInvertedImage(session->toolboxTexture(), 0,
				 session->toolboxColor(),
				 session->toolboxToColor());
  
  XSetWindowBackgroundPixmap(display, frame.clock, p);
  if (p) XFreePixmap(display, p);
  
  XClearWindow(display, frame.window);
  XClearWindow(display, frame.workspace_button);
  XClearWindow(display, frame.icon);
  XClearWindow(display, frame.clock);

  XMapSubwindows(display, frame.window);
  XMapSubwindows(display, frame.base);
  XMapWindow(display, frame.base);

  XDrawString(display, frame.workspace_button, buttonGC, 4, 3 +
	      session->titleFont()->ascent, frame.title, strlen(frame.title));
}


WorkspaceManager::~WorkspaceManager(void) {
  debug->msg("%s: WorkspaceManager::~WorkspaceManager\n", __FILE__);

  DissociateAll();
  delete ilist;
  delete workspaces_menu;
  if (frame.title) delete frame.title;

  for (int i = 0; i < workspaces_list->count(); ++i) {
    delete workspaces_list->at(0);
    workspaces_list->remove(0);
  }
  delete workspaces_list;

  if (frame.button) XFreePixmap(display, frame.button);
  if (frame.pbutton) XFreePixmap(display, frame.pbutton);

  XDeleteContext(display, frame.clock, session->wsContext());
  XDestroyWindow(display, frame.clock);
  XDeleteContext(display, frame.icon, session->wsContext());
  XDestroyWindow(display, frame.icon);
  XDeleteContext(display, frame.workspace_button, session->wsContext());
  XDestroyWindow(display, frame.workspace_button);
  XDeleteContext(display, frame.workspace_button, session->wsContext());
  XDestroyWindow(display, frame.window);
  XDestroyWindow(display, frame.base);

  delete debug;
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
    XGrabServer(display);
    hideMenu();
    current->hideAll();
    current = workspace(id);
    sprintf(frame.title, "Workspace %d", id);
    XClearWindow(display, frame.workspace_button);
    XDrawString(display, frame.workspace_button, buttonGC, 4, 3 +
                session->titleFont()->ascent, frame.title,
		strlen(frame.title));
    current->showAll();
    XUngrabServer(display);
  }
}


void WorkspaceManager::arrangeIcons(void) {
  int i;
  BlackboxIcon *icon = NULL;
  
  XGrabServer(display);
  for (i = 0; i < ilist->count(); ++i) {
    icon = ilist->at(i);
    icon->move(4, (icon->Height()) * (ilist->count() - (i + 1)) +
	       (frame.frame_h / 2) + 1);
    XMoveWindow(display, icon->iconWindow(), 1,
		(icon->Height()) * (ilist->count() - (i + 1)) + 1);
    icon->exposeEvent(NULL);
  }
  
  XUngrabServer(display);
}


void WorkspaceManager::buttonPressEvent(XButtonEvent *be) {
  if (be->window == frame.workspace_button)
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
    XDrawString(display, frame.workspace_button, buttonGC, 4, 3 +
		session->titleFont()->ascent, frame.title,
		strlen(frame.title));
  else if (ee->window == frame.clock)
    checkClock(True);
}


void WorkspaceManager::showMenu(void) {
  XSetWindowBackgroundPixmap(display, frame.workspace_button,
			     frame.pbutton);
  XClearWindow(display, frame.workspace_button);
  XDrawString(display, frame.workspace_button, buttonGC, 4, 3 +
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
  XDrawString(display, frame.workspace_button, buttonGC, 4, 3 +
	      session->titleFont()->ascent, frame.title,
	      strlen(frame.title));
  workspaces_menu->hideMenu();
}


void WorkspaceManager::stackWindows(Window *workspace_stack, int num) {
  // create window stack... the number of windows for the current workspace,
  // 3 windows for the toolbar, root menu, and workspaces menu and then the
  // number of total workspaces (to stack the workspace menus)

  Window *session_stack = new Window[num + 3 + workspaces_list->count()];

  int i = 0;
  *(session_stack + i++) = session->menu()->windowID();
  *(session_stack + i++) = workspaces_menu->windowID();

  for (int j = 0; j < workspaces_list->count(); j++)
    *(session_stack + i++) = workspaces_list->at(j)->menu()->windowID();
    
  int k = num;
  while (k--)
    *(session_stack + i++) = *(workspace_stack + k);

  *(session_stack + i++) = frame.base;

  XRestackWindows(display, session_stack, i);
  delete [] session_stack;
}


void WorkspaceManager::Reconfigure(void) {
  debug->msg("%s: WorkspaceManager::Reconfigure\n", __FILE__);

  frame.button_w = XTextWidth(session->titleFont(), "Workspace 00", 13) + 8;
  frame.button_h = session->titleFont()->ascent +
    session->titleFont()->descent + 8; 

  frame.frame_w = frame.button_w + 6;
  frame.frame_h = session->YResolution() - 2;

  XGCValues gcv;
  gcv.font = session->titleFont()->fid;
  gcv.foreground = session->toolboxTextColor().pixel;
  XChangeGC(display, buttonGC, GCFont|GCForeground, &gcv);

  BImage image(session, frame.frame_w, frame.frame_h,
	       session->Depth(), session->frameColor());
  Pixmap p = image.renderImage(session->toolboxTexture(), 1,
			       session->toolboxColor(),
			       session->toolboxToColor());
  
  XResizeWindow(display, frame.base, frame.frame_w, frame.frame_h);
  XResizeWindow(display, frame.window, frame.frame_w, frame.frame_h);
  XSetWindowBackgroundPixmap(display, frame.window, p);
  if (p) XFreePixmap(display, p);

  if (frame.button) XFreePixmap(display, frame.button);
  if (frame.pbutton) XFreePixmap(display, frame.pbutton);

  BImage bimage(session, frame.button_w, frame.button_h,
		session->Depth(), session->buttonColor());
  frame.button = bimage.renderImage(session->buttonTexture(), 0,
				    session->buttonColor(),
				    session->buttonToColor());
  frame.pbutton = bimage.renderInvertedImage(session->buttonTexture(), 0,
					     session->buttonColor(),
					     session->buttonToColor());

  XResizeWindow(display, frame.workspace_button, frame.button_w,
		frame.button_h);
  if (workspaces_menu->menuVisible())
    XSetWindowBackgroundPixmap(display, frame.workspace_button, frame.pbutton);
  else
    XSetWindowBackgroundPixmap(display, frame.workspace_button, frame.button);
  
  BImage iimage(session, frame.button_w, (frame.frame_h / 2) - 2,
		session->Depth(), session->toolboxColor());
  p = iimage.renderInvertedImage(session->toolboxTexture(), 0,
				 session->toolboxColor(),
				 session->toolboxToColor());


  XResizeWindow(display, frame.icon, frame.button_w, (frame.frame_h / 2) - 2);
  XSetWindowBackgroundPixmap(display, frame.icon, p);
  if (p) XFreePixmap(display, p);

  BImage cimage(session, frame.button_w, frame.button_h,
	        session->Depth(), session->toolboxColor());
  p = cimage.renderInvertedImage(session->toolboxTexture(), 0,
				 session->toolboxColor(),
				 session->toolboxToColor());
  
  XResizeWindow(display, frame.clock, frame.button_w, frame.button_h);
  XSetWindowBackgroundPixmap(display, frame.clock, p);
  if (p) XFreePixmap(display, p);

  XSetWindowBackground(display, frame.base, session->frameColor().pixel);
  XSetWindowBorder(display, frame.base, session->frameColor().pixel);
  XClearWindow(display, frame.window);
  XClearWindow(display, frame.workspace_button);
  XClearWindow(display, frame.icon);
  XClearWindow(display, frame.clock);
  
  XDrawString(display, frame.workspace_button, buttonGC, 4, 3 +
	      session->titleFont()->ascent, frame.title,
	      strlen(frame.title));
  checkClock(True);

  debug->msg("\t[ workspace menu update ]\n");
  workspaces_menu->Reconfigure();
  workspaces_menu->updateMenu();

  debug->msg("\t[ workspace update ]\n");
  for (int i = 0; i < workspaces_list->count(); i++)
    workspace(i)->Reconfigure();

  debug->msg("%s: leaving WorkspaceManager::Reconfigure\n", __FILE__);
}


void WorkspaceManager::checkClock(Bool redraw) {
  static int hour, minute;
  time_t tmp;
  struct tm *tt;
  
  if ((tmp = time(NULL)) != -1) {
    tt = localtime(&tmp);
    if (tt->tm_min != minute || tt->tm_hour != hour) {
      hour = tt->tm_hour;
      minute = tt->tm_min;
      XClearWindow(display, frame.clock);
      redraw = True;
    }
  }

  if (redraw) {
    char t[9];
    sprintf(t, "%02d:%02d %cm", ((hour > 12) ? hour - 12 : hour), minute,
	    ((hour > 12) ? 'p' : 'a'));
	  
    int len = strlen(t);
    XDrawString(display, frame.clock, buttonGC,
		(frame.button_w -
		 XTextWidth(session->titleFont(), t, len)) / 2,
		3 + session->titleFont()->ascent, t, len);
  }
}
