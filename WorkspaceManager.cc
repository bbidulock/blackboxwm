//
// WorkspaceManager.cc for Blackbox - an X11 Window manager
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
#define __GNU_SOURCE
#endif

#include "WorkspaceManager.hh"
#include "Workspacemenu.hh"

#include "blackbox.hh"
#include "Rootmenu.hh"
#include "Workspace.hh"
#include "icon.hh"

#include <stdio.h>
#include <sys/time.h>


// *************************************************************************
// Workspace manager class code
// *************************************************************************

WorkspaceManager::WorkspaceManager(Blackbox *bb, int c) {
  blackbox = bb;
  workspacesList = new LinkedList<Workspace>;
  waitb1 = waitb3 = waiti = False;

  display = blackbox->control();
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWOverrideRedirect |CWCursor|CWEventMask; 
  
  attrib_create.background_pixmap = None;
  attrib_create.background_pixel = attrib_create.border_pixel =
    blackbox->borderColor().pixel;
  attrib_create.override_redirect = True;
  attrib_create.cursor = blackbox->sessionCursor();
  attrib_create.event_mask = NoEventMask;

  frame.bevel_w = blackbox->bevelWidth();
  frame.width = blackbox->XResolution() * 2 / 3;
  frame.height =
    ((blackbox->titleFont()->ascent + blackbox->titleFont()->descent +
      (frame.bevel_w * 2)) * 2) + (frame.bevel_w * 2);
  frame.x = (blackbox->XResolution() - frame.width) / 2;
  frame.y = blackbox->YResolution() - frame.height;
  frame.clock_h = frame.ib_h = frame.wsd_h = frame.button_h =
    frame.height - (frame.bevel_w * 2);
  frame.button_w = frame.button_h / 3;
  frame.wsd_w = frame.width / 4;
  frame.ib_w = XTextWidth(blackbox->titleFont(), "Icons",
			  strlen("Icons")) + (frame.bevel_w * 2);
  frame.clock_w = XTextWidth(blackbox->titleFont(), "00:00000",
			     strlen("00:00000")) + (frame.bevel_w * 2);

  frame.base =
    XCreateWindow(display, blackbox->Root(), frame.x, frame.y, frame.width,
		  frame.height, 0, blackbox->Depth(), InputOutput,
		  blackbox->visual(), create_mask, &attrib_create);

  attrib_create.event_mask = StructureNotifyMask|SubstructureNotifyMask|
    SubstructureRedirectMask|ButtonPressMask|ButtonReleaseMask|
    ButtonMotionMask|ExposureMask;

  frame.window =
    XCreateWindow(display, frame.base, 0, 0, frame.width, frame.height, 0,
		  blackbox->Depth(), InputOutput, blackbox->visual(),
		  create_mask, &attrib_create);
  blackbox->saveWSManagerSearch(frame.window, this);

  BImage *t_image = new BImage(blackbox, frame.width, frame.height,
			       blackbox->Depth());
  Pixmap p = t_image->renderImage(blackbox->sWindowTexture(),
				  blackbox->sWColor(), blackbox->sWColorTo());
  delete t_image;
  XSetWindowBackgroundPixmap(display, frame.window, p);
  if (p) XFreePixmap(display, p);

  XGCValues gcv;
  gcv.font = blackbox->titleFont()->fid;
  gcv.foreground = blackbox->sTextColor().pixel;
  buttonGC = XCreateGC(display, frame.window,
		       GCFont|GCForeground, &gcv);

  frame.workspaceDock =
    XCreateWindow(display, frame.window, frame.bevel_w, frame.bevel_w,
		  frame.wsd_w, frame.wsd_h, 0, blackbox->Depth(), InputOutput,
		  blackbox->visual(), create_mask, &attrib_create);
  blackbox->saveWSManagerSearch(frame.workspaceDock, this);

  BImage *w_image = new BImage(blackbox, frame.wsd_w, frame.wsd_h,
			       blackbox->Depth());
  p = w_image->renderImage(blackbox->sLabelTexture(),
			   blackbox->sLColor(), blackbox->sLColorTo());
  delete w_image;
  XSetWindowBackgroundPixmap(display, frame.workspaceDock, p);
  if (p) XFreePixmap(display, p);

  frame.bButton =
    XCreateWindow(display, frame.window, frame.bevel_w + frame.wsd_w,
		  frame.bevel_w,
		  frame.button_w, frame.button_h, 0, blackbox->Depth(),
		  InputOutput, blackbox->visual(), create_mask,
		  &attrib_create);
  blackbox->saveWSManagerSearch(frame.bButton, this);
  
  frame.fButton =
    XCreateWindow(display, frame.window, frame.button_w + frame.wsd_w +
		  frame.bevel_w, frame.bevel_w,
		  frame.button_w, frame.button_h, 0, blackbox->Depth(),
		  InputOutput, blackbox->visual(), create_mask,
		  &attrib_create);
  blackbox->saveWSManagerSearch(frame.fButton, this);

  BImage *b_image = new BImage(blackbox, frame.button_w, frame.button_h,
			       blackbox->Depth());
  frame.button =
    b_image->renderImage(blackbox->sButtonTexture(),
			 blackbox->sBColor(), blackbox->sBColorTo());
  frame.pbutton =
    b_image->renderInvertedImage(blackbox->sButtonTexture(),
				 blackbox->sBColor(), blackbox->sBColorTo());
  delete b_image;
  XSetWindowBackgroundPixmap(display, frame.fButton, frame.button);
  XSetWindowBackgroundPixmap(display, frame.bButton, frame.button);

  frame.iconButton =
    XCreateWindow(display, frame.window, (frame.button_w * 2) +
		  frame.wsd_w + frame.bevel_w, frame.bevel_w, frame.ib_w,
		  frame.ib_h, 0, blackbox->Depth(), InputOutput,
		  blackbox->visual(), create_mask, &attrib_create);
  blackbox->saveWSManagerSearch(frame.iconButton, this);

  BImage *i_image = new BImage(blackbox, frame.ib_w, frame.ib_h,
			       blackbox->Depth());
  frame.ibutton =
    i_image->renderImage(blackbox->sButtonTexture(),
			 blackbox->sBColor(), blackbox->sBColorTo());
  frame.pibutton =
    i_image->renderInvertedImage(blackbox->sButtonTexture(),
				 blackbox->sBColor(), blackbox->sBColorTo());
  delete i_image;
  XSetWindowBackgroundPixmap(display, frame.iconButton, frame.ibutton);

  frame.clock =
    XCreateWindow(display, frame.window, frame.width - frame.clock_w -
		  frame.bevel_w, frame.bevel_w, frame.clock_w, frame.clock_h,
		  0, blackbox->Depth(), InputOutput, blackbox->visual(),
		  create_mask, &attrib_create);
  blackbox->saveWSManagerSearch(frame.clock, this);

  BImage *c_image = new BImage(blackbox, frame.clock_w, frame.clock_h,
			       blackbox->Depth());
  p = c_image->renderImage(blackbox->sClockTexture(),
			   blackbox->sCColor(), blackbox->sCColorTo());
  delete c_image;
  XSetWindowBackgroundPixmap(display, frame.clock, p);
  if (p) XFreePixmap(display, p);
  
  XClearWindow(display, frame.window);
  XClearWindow(display, frame.workspaceDock);
  XClearWindow(display, frame.fButton);
  XClearWindow(display, frame.bButton);
  XClearWindow(display, frame.clock);

  XMapSubwindows(display, frame.window);
  XMapSubwindows(display, frame.base);
  XMapWindow(display, frame.base);

  wsMenu = new Workspacemenu(blackbox, this);
  iconMenu = new Iconmenu(blackbox);
  iconMenu->Move(frame.x + frame.wsd_w + (frame.button_w * 2) + frame.bevel_w,
		 frame.y - iconMenu->Height() - 2);

  addWorkspace();
  wsMenu->remove(2);
  if (c != 0)
    for (int i = 0; i < c; ++i)
      addWorkspace();
  else
    addWorkspace();
  
  zero = workspacesList->first();
  current = workspacesList->find(1);
  
  redrawWSD(True);
  checkClock(True);

  int hh = frame.button_h / 2, hw = frame.button_w / 2;
  
  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh;
  pts[1].x = 4; pts[1].y = 2;
  pts[2].x = 0; pts[2].y = -4;
  
  XFillPolygon(display, frame.bButton, buttonGC, pts, 3, Convex,
	       CoordModePrevious);
  
  pts[0].x = hw - 2; pts[0].y = hh - 2;
  pts[1].x = 4; pts[1].y =  2;
  pts[2].x = -4; pts[2].y = 2;
  
  XFillPolygon(display, frame.fButton, buttonGC, pts, 3, Convex,
	       CoordModePrevious);

  XDrawString(display, frame.iconButton, buttonGC, frame.bevel_w,
	      (frame.ib_h + blackbox->titleFont()->ascent -
	       blackbox->titleFont()->descent) / 2,
	      "Icons", strlen("Icons"));
}


WorkspaceManager::~WorkspaceManager(void) {
  XUnmapWindow(display, frame.base);

  delete wsMenu;
  delete iconMenu;

  int n = workspacesList->count(), i;
  for (i = 0; i < n; i++) {
    Workspace *tmp = workspacesList->find(0);
    workspacesList->remove(0);
    delete tmp;
  }
  delete workspacesList;

  if (frame.button) XFreePixmap(display, frame.button);
  if (frame.pbutton) XFreePixmap(display, frame.pbutton);

  /*  blackbox->removeWSManagerSearch(frame.clock);
  blackbox->removeWSManagerSearch(frame.workspaceDock);
  blackbox->removeWSManagerSearch(frame.fButton);
  blackbox->removeWSManagerSearch(frame.bButton);
  blackbox->removeWSManagerSearch(frame.window);
  */

  XDestroyWindow(display, frame.bButton);
  XDestroyWindow(display, frame.fButton);
  XDestroyWindow(display, frame.workspaceDock);
  XDestroyWindow(display, frame.clock);
  XDestroyWindow(display, frame.base);
}


int WorkspaceManager::addWorkspace(void) {
  Workspace *wkspc = new Workspace(this, workspacesList->count());
  workspacesList->insert(wkspc);

  wsMenu->insert(wkspc->Name());
  wsMenu->Update();
  wsMenu->Move(frame.x + 1, frame.y - wsMenu->Height() - 2);

  return workspacesList->count();
}


int WorkspaceManager::removeLastWorkspace(void) {
  if (workspacesList->count() > 2) {
    Workspace *wkspc = workspacesList->last();
    
    if (current->workspaceID() == wkspc->workspaceID())
      changeWorkspaceID(current->workspaceID() - 1);

    wkspc->removeAll();
    
    wsMenu->remove(wkspc->workspaceID() + 1);
    wsMenu->Update();
    wsMenu->Move(frame.x + 1, frame.y - wsMenu->Height() - 2);

    workspacesList->remove(wkspc);
    delete wkspc;

    return workspacesList->count();
  }

  return 0;
}


void WorkspaceManager::addIcon(BlackboxIcon *icon) {
  iconMenu->insert(icon);
  iconMenu->Move(frame.x + frame.wsd_w + (frame.button_w * 2) + frame.bevel_w,
		 frame.y - iconMenu->Height() - 2);
}


void WorkspaceManager::removeIcon(BlackboxIcon *icon) {
  iconMenu->remove(icon);
  iconMenu->Move(frame.x + frame.wsd_w + (frame.button_w * 2) + frame.bevel_w,
		 frame.y - iconMenu->Height() - 2);
}


void WorkspaceManager::iconUpdate(void) {
  iconMenu->Update();
  iconMenu->Move(frame.x + frame.wsd_w + (frame.button_w * 2) + frame.bevel_w,
		 frame.y - iconMenu->Height() - 2);
}


void WorkspaceManager::DissociateAll(void) {
  LinkedListIterator<Workspace> it(workspacesList);
  for (; it.current(); it++)
    it.current()->Dissociate();
}


Workspace *WorkspaceManager::workspace(int w) {
  return workspacesList->find(w);
}


int WorkspaceManager::currentWorkspaceID(void) {
  return current->workspaceID();
}


void WorkspaceManager::changeWorkspaceID(int id) {
  if (id != current->workspaceID()) {
    current->hideAll();
    current = workspace(id);
    redrawWSD(True);
    current->showAll();
    XSync(display, False);
  }
}


void WorkspaceManager::stackWindows(Window *workspace_stack, int num) {
  // create window stack... the number of windows for the current workspace,
  // 3 windows for the toolbar, root menu, and workspaces menu and then the
  // number of total workspaces (to stack the workspace menus)

  Window *session_stack =
    new Window[(num + zero->Count() + workspacesList->count() + 4)];
  
  int i = 0;
  *(session_stack + i++) = blackbox->Menu()->WindowID();
  *(session_stack + i++) = iconMenu->WindowID();
  *(session_stack + i++) = wsMenu->WindowID();
  
  int fx = frame.x + ((wsMenu->Visible()) ? wsMenu->Width() + 1 : 0) + 1;
  LinkedListIterator<Workspace> it(workspacesList);
  for (; it.current(); it++) {
    if (it.current() == current)
      it.current()->Menu()->Move(fx, frame.y -
				 it.current()->Menu()->Height() - 2);
    *(session_stack + i++) = it.current()->Menu()->WindowID();
  }

  int k = zero->Count();
  while (k--)
    *(session_stack + i++) = *(zero->windowStack() + k);

  k = num;
  while (k--)
    *(session_stack + i++) = *(workspace_stack + k);
  
  *(session_stack + i++) = frame.base;

  XRestackWindows(display, session_stack, i);
  delete [] session_stack;
}


void WorkspaceManager::Reconfigure(void) {
  frame.bevel_w = blackbox->bevelWidth();
  frame.width = blackbox->XResolution() * 2 / 3;
  frame.height =
    ((blackbox->titleFont()->ascent + blackbox->titleFont()->descent +
      (frame.bevel_w * 2)) * 2) + (frame.bevel_w * 2);
  frame.x = (blackbox->XResolution() - frame.width) / 2;
  frame.y = blackbox->YResolution() - frame.height;
  frame.ib_h = frame.clock_h = frame.wsd_h = frame.button_h =
    frame.height - (frame.bevel_w * 2);
  frame.button_w = frame.button_h / 3;
  frame.wsd_w = frame.width / 4;
  frame.ib_w = XTextWidth(blackbox->titleFont(), "Icons",
			  strlen("Icons")) + (frame.bevel_w * 2);
  frame.clock_w = XTextWidth(blackbox->titleFont(), "00:00000",
			     strlen("00:00000")) + (frame.bevel_w * 2);

  XGCValues gcv;
  gcv.font = blackbox->titleFont()->fid;
  gcv.foreground = blackbox->sTextColor().pixel;
  XChangeGC(display, buttonGC, GCFont|GCForeground, &gcv);

  XMoveResizeWindow(display, frame.base, frame.x, frame.y,
		    frame.width, frame.height);
  XResizeWindow(display, frame.window, frame.width, frame.height);
  XMoveResizeWindow(display, frame.workspaceDock, frame.bevel_w, frame.bevel_w,
		    frame.wsd_w, frame.wsd_h);
  XMoveResizeWindow(display, frame.bButton, frame.bevel_w + frame.wsd_w,
		    frame.bevel_w, frame.button_w, frame.button_h);
  XMoveResizeWindow(display, frame.fButton, frame.bevel_w + frame.wsd_w +
		    frame.button_w, frame.bevel_w, frame.button_w,
		    frame.button_h);
  XMoveResizeWindow(display, frame.iconButton, frame.bevel_w + frame.wsd_w +
		    (frame.button_w * 2), frame.bevel_w, frame.ib_w,
		    frame.ib_h);
  XMoveResizeWindow(display, frame.clock, frame.width - frame.clock_w -
		    frame.bevel_w, frame.bevel_w, frame.clock_w,
		    frame.clock_h);
  
  BImage *t_image = new BImage(blackbox, frame.width, frame.height,
			       blackbox->Depth());
  Pixmap p = t_image->renderImage(blackbox->sWindowTexture(),
				  blackbox->sWColor(), blackbox->sWColorTo());
  delete t_image;
  XSetWindowBackgroundPixmap(display, frame.window, p);
  if (p) XFreePixmap(display, p);

  BImage *w_image = new BImage(blackbox, frame.wsd_w, frame.wsd_h,
			       blackbox->Depth());
  p = w_image->renderImage(blackbox->sLabelTexture(),
			   blackbox->sLColor(), blackbox->sLColorTo());
  delete w_image;
  XSetWindowBackgroundPixmap(display, frame.workspaceDock, p);
  if (p) XFreePixmap(display, p);

  if (frame.button) XFreePixmap(display, frame.button);
  if (frame.pbutton) XFreePixmap(display, frame.pbutton);

  BImage *b_image = new BImage(blackbox, frame.button_w, frame.button_h,
			       blackbox->Depth());
  frame.button =
    b_image->renderImage(blackbox->sButtonTexture(),
			 blackbox->sBColor(), blackbox->sBColorTo());
  frame.pbutton =
    b_image->renderInvertedImage(blackbox->sButtonTexture(),
				 blackbox->sBColor(), blackbox->sBColorTo());
  delete b_image;
  XSetWindowBackgroundPixmap(display, frame.fButton, frame.button);
  XSetWindowBackgroundPixmap(display, frame.bButton, frame.button);

  BImage *i_image = new BImage(blackbox, frame.ib_w, frame.ib_h,
			       blackbox->Depth());
  frame.ibutton =
    i_image->renderImage(blackbox->sButtonTexture(),
			 blackbox->sBColor(), blackbox->sBColorTo());
  frame.pibutton =
    i_image->renderInvertedImage(blackbox->sButtonTexture(),
				 blackbox->sBColor(), blackbox->sBColorTo());
  delete i_image;
  XSetWindowBackgroundPixmap(display, frame.iconButton,
			     ((iconMenu->Visible()) ? frame.pibutton :
			      frame.ibutton));
  
  BImage *c_image = new BImage(blackbox, frame.clock_w, frame.clock_h,
			       blackbox->Depth());
  p = c_image->renderImage(blackbox->sClockTexture(),
			   blackbox->sCColor(), blackbox->sCColorTo());
  delete c_image;
  XSetWindowBackgroundPixmap(display, frame.clock, p);
  if (p) XFreePixmap(display, p);

  XSetWindowBackground(display, frame.base, blackbox->borderColor().pixel);
  XSetWindowBorder(display, frame.base, blackbox->borderColor().pixel);

  XClearWindow(display, frame.window);
  XClearWindow(display, frame.workspaceDock);
  XClearWindow(display, frame.fButton);
  XClearWindow(display, frame.bButton);
  XClearWindow(display, frame.iconButton);
  XClearWindow(display, frame.clock);
 
  redrawWSD(True);
  checkClock(True);

  int hh = frame.button_h / 2, hw = frame.button_w / 2;
  
  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh;
  pts[1].x = 4; pts[1].y = 2;
  pts[2].x = 0; pts[2].y = -4;
  
  XFillPolygon(display, frame.bButton, buttonGC, pts, 3, Convex,
	       CoordModePrevious);
  
  pts[0].x = hw - 2; pts[0].y = hh - 2;
  pts[1].x = 4; pts[1].y =  2;
  pts[2].x = -4; pts[2].y = 2;
  
  XFillPolygon(display, frame.fButton, buttonGC, pts, 3, Convex,
	       CoordModePrevious);

  wsMenu->Reconfigure();
  wsMenu->Move(frame.x + 1, frame.y - wsMenu->Height() - 2);

  iconMenu->Reconfigure();
  iconMenu->Move(frame.x + frame.wsd_w + (frame.button_w * 2) + frame.bevel_w,
		 frame.y - iconMenu->Height() - 2);

  XDrawString(display, frame.iconButton, buttonGC, frame.bevel_w,
	      (frame.ib_h + blackbox->titleFont()->ascent -
	       blackbox->titleFont()->descent) / 2,
	      "Icons", strlen("Icons"));

  LinkedListIterator<Workspace> it(workspacesList);
  for (; it.current(); it++)
    it.current()->Reconfigure();

  current->Menu()->Move(frame.x + ((wsMenu->Visible()) ?
                                   wsMenu->Width() + 2 : 2),
                        frame.y - current->Menu()->Height() - 2);
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
    if (blackbox->clock24Hour())
      sprintf(t, "[ %02d:%02d]", hour, minute);
    else
      sprintf(t, "%02d:%02d %cm",
	      ((hour > 12) ? hour - 12 : ((hour == 0) ? 12 : hour)), minute,
	      ((hour >= 12) ? 'p' : 'a'));
    
    int len = strlen(t);
    XDrawString(display, frame.clock, buttonGC, frame.bevel_w,
		(frame.clock_h + blackbox->titleFont()->ascent -
		 blackbox->titleFont()->descent) / 2,
		t, len);
  }
}


void WorkspaceManager::redrawWSD(Bool clear) {
  if (clear)
    XClearWindow(display, frame.workspaceDock);
  
  int l = strlen(current->Name()),
    x = (frame.wsd_w - XTextWidth(blackbox->titleFont(),
				  current->Name(), l))/ 2;
  XDrawString(display, frame.workspaceDock, buttonGC, x, 2 +
	      blackbox->titleFont()->ascent, current->Name(), l);

  l = strlen(current->Label());
  x = (frame.wsd_w - XTextWidth(blackbox->titleFont(),
				current->Label(), l)) / 2;
  XDrawString(display, frame.workspaceDock, buttonGC, x,
	      (frame.wsd_h / 2) + 2 + blackbox->titleFont()->ascent,
	      current->Label(), l);
}


// *************************************************************************
// event handlers
// *************************************************************************

void WorkspaceManager::buttonPressEvent(XButtonEvent *be) {
  if (be->button == 1) {
    if (be->window == frame.bButton) {
      XSetWindowBackgroundPixmap(display, frame.bButton, frame.pbutton);
      XClearWindow(display, frame.bButton);
      int hh = frame.button_h / 2, hw = frame.button_w / 2;
      
      XPoint pts[3];
      pts[0].x = hw - 2; pts[0].y = hh;
      pts[1].x = 4; pts[1].y = 2;
      pts[2].x = 0; pts[2].y = -4;
    
      XFillPolygon(display, frame.bButton, buttonGC, pts, 3, Convex,
		   CoordModePrevious);      
    } else if (be->window == frame.fButton) {
      XSetWindowBackgroundPixmap(display, frame.fButton, frame.pbutton);
      XClearWindow(display, frame.fButton);
      int hh = frame.button_h / 2, hw = frame.button_w / 2;
    
      XPoint pts[3];
      pts[0].x = hw - 2; pts[0].y = hh - 2;
      pts[1].x = 4; pts[1].y =  2;
      pts[2].x = -4; pts[2].y = 2;
    
      XFillPolygon(display, frame.fButton, buttonGC, pts, 3, Convex,
		   CoordModePrevious);
    } else if (be->window == frame.workspaceDock) {
      if (! wsMenu->Visible()) {
	if (current->Menu()->Visible())
	  current->Menu()->Move(frame.x + wsMenu->Width() + 2, frame.y -
				current->Menu()->Height() - 2);
	wsMenu->Show();
	
	waitb1 = True;
      }
    } else if (be->window == frame.iconButton) {
      if (! iconMenu->Visible()) {
	XSetWindowBackgroundPixmap(display, frame.iconButton, frame.pibutton);
	XClearWindow(display, frame.iconButton);
	XDrawString(display, frame.iconButton, buttonGC, frame.bevel_w,
		    (frame.ib_h + blackbox->titleFont()->ascent -
		     blackbox->titleFont()->descent) / 2,
		    "Icons", strlen("Icons"));

	iconMenu->Show();

	waiti = True;
      }
    }
  } else if (be->button == 3) {
    if (be->window == frame.workspaceDock) {
      if (! current->Menu()->Visible()) {
	int fx = frame.x + ((wsMenu->Visible()) ? wsMenu->Width() + 2 : 1);
	current->Menu()->Move(fx, frame.y - current->Menu()->Height() - 2);
	current->Menu()->Show();
	
	waitb3 = True;
      }
    }
  }
}


void WorkspaceManager::buttonReleaseEvent(XButtonEvent *re) {
  if (re->button == 1) {
    if (re->window == frame.bButton) {
      XSetWindowBackgroundPixmap(display, frame.bButton, frame.button);
      XClearWindow(display, frame.bButton);
      int hh = frame.button_h / 2, hw = frame.button_w / 2;
      
      XPoint pts[3];
      pts[0].x = hw - 2; pts[0].y = hh;
      pts[1].x = 4; pts[1].y = 2;
      pts[2].x = 0; pts[2].y = -4;
    
      XFillPolygon(display, frame.bButton, buttonGC, pts, 3, Convex,
		   CoordModePrevious);

      if (re->x >= 0 && re->x < (signed) frame.button_w &&
	  re->y >= 0 && re->y < (signed) frame.button_h)
	if (current->workspaceID() > 1)
	  changeWorkspaceID(current->workspaceID() - 1);
	else
	  changeWorkspaceID(workspacesList->count() - 1);
    } else if (re->window == frame.fButton) {
      XSetWindowBackgroundPixmap(display, frame.fButton, frame.button);
      XClearWindow(display, frame.fButton);
      int hh = frame.button_h / 2, hw = frame.button_w / 2;
    
      XPoint pts[3];
      pts[0].x = hw - 2; pts[0].y = hh - 2;
      pts[1].x = 4; pts[1].y =  2;
      pts[2].x = -4; pts[2].y = 2;
    
      XFillPolygon(display, frame.fButton, buttonGC, pts, 3, Convex,
		   CoordModePrevious);
      
      if (re->x >= 0 && re->x < (signed) frame.button_w &&
	  re->y >= 0 && re->y < (signed) frame.button_h)
	if (current->workspaceID() < (workspacesList->count() - 1))
	  changeWorkspaceID(current->workspaceID() + 1);
	else
	  changeWorkspaceID(1);
    } else if (re->window == frame.workspaceDock) {
      if (! waitb1) {
	if (current->Menu()->Visible())
	  current->Menu()->Move(frame.x + 1, frame.y -
				current->Menu()->Height() - 2);
	wsMenu->Hide();
      } else
	waitb1 = False;
    } else if (re->window == frame.iconButton) {
      if (! waiti) {
	XSetWindowBackgroundPixmap(display, frame.iconButton, frame.ibutton);
	XClearWindow(display, frame.iconButton);
	XDrawString(display, frame.iconButton, buttonGC, frame.bevel_w,
		    (frame.ib_h + blackbox->titleFont()->ascent -
		     blackbox->titleFont()->descent) / 2,
		    "Icons", strlen("Icons"));

	iconMenu->Hide();
      } else
	waiti = False;
    }
  } else if (re->button == 3) {
    if (re->window == frame.workspaceDock) {
      if (! waitb3)
	current->Menu()->Hide();
      else
	waitb3 = False;
    }
  }
}


void WorkspaceManager::exposeEvent(XExposeEvent *ee) {
  if (ee->window == frame.clock)
    checkClock(True);
  else if (ee->window == frame.workspaceDock)
    redrawWSD();
  else if (ee->window == frame.iconButton)
    XDrawString(display, frame.iconButton, buttonGC, frame.bevel_w,
		(frame.ib_h + blackbox->titleFont()->ascent -
		 blackbox->titleFont()->descent) / 2,
		"Icons", strlen("Icons"));
  else if (ee->window == frame.bButton) {
    int hh = frame.button_h / 2, hw = frame.button_w / 2;
    
    XPoint pts[3];
    pts[0].x = hw - 2; pts[0].y = hh;
    pts[1].x = 4; pts[1].y = 2;
    pts[2].x = 0; pts[2].y = -4;
    
    XFillPolygon(display, frame.bButton, buttonGC, pts, 3, Convex,
		 CoordModePrevious);
  } else if (ee->window == frame.fButton) {
    int hh = frame.button_h / 2, hw = frame.button_w / 2;
    
    XPoint pts[3];
    pts[0].x = hw - 2; pts[0].y = hh - 2;
    pts[1].x = 4; pts[1].y =  2;
    pts[2].x = -4; pts[2].y = 2;
    
    XFillPolygon(display, frame.fButton, buttonGC, pts, 3, Convex,
		 CoordModePrevious);
  }
}
