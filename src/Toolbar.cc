//
// Toolbar.cc for Blackbox - an X11 Window manager
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

#include "Toolbar.hh"
#include "Workspacemenu.hh"

#include "blackbox.hh"
#include "Rootmenu.hh"
#include "Workspace.hh"
#include "Icon.hh"

#include <stdio.h>
#include <sys/time.h>
 

// *************************************************************************
// Workspace manager class code
// *************************************************************************

Toolbar::Toolbar(Blackbox *bb, int c) {
  blackbox = bb;
  image_ctrl = blackbox->imageControl();
  workspacesList = new LinkedList<Workspace>;

  wait_button = wait_ibutton = False;
  raised = blackbox->toolbarRaised();

  display = blackbox->control();
  XSetWindowAttributes attrib;
  unsigned long create_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWOverrideRedirect |CWCursor|CWEventMask; 
  attrib.background_pixmap = None;
  attrib.background_pixel = attrib.border_pixel =
    blackbox->borderColor().pixel;
  attrib.override_redirect = True;
  attrib.cursor = blackbox->sessionCursor();
  attrib.event_mask = NoEventMask;

  frame.bevel_w = blackbox->bevelWidth();

  frame.width = blackbox->xRes() * 2 / 3;
  frame.height = blackbox->titleFont()->ascent +
    blackbox->titleFont()->descent + (frame.bevel_w * 4);

  frame.x = (blackbox->xRes() - frame.width) / 2;
  frame.y = blackbox->yRes() - frame.height;
  frame.clock_h = frame.ib_h = frame.label_h = frame.button_h =
    frame.height - (frame.bevel_w * 2);

  frame.button_w = (frame.button_h * 3) / 4;
  frame.ib_w = XTextWidth(blackbox->titleFont(), "Icons",
			  strlen("Icons")) + (frame.bevel_w * 2);
  frame.clock_w = XTextWidth(blackbox->titleFont(), "00:00000",
			     strlen("00:00000")) + (frame.bevel_w * 2);
  
  frame.label_w =
    (frame.width - (frame.clock_w + frame.ib_w + (frame.button_w * 6) +
		    (frame.bevel_w * 6) + 8)) / 2;

  frame.base =
    XCreateWindow(display, blackbox->Root(), frame.x, frame.y, frame.width,
		  frame.height, 0, blackbox->Depth(), InputOutput,
		  blackbox->visual(), create_mask, &attrib);

  attrib.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask;

  frame.window =
    XCreateWindow(display, frame.base, 0, 0, frame.width, frame.height, 0,
		  blackbox->Depth(), InputOutput, blackbox->visual(),
		  create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.window, this);
  
  frame.frame =
    image_ctrl->renderImage(frame.width, frame.height,
			  blackbox->tResource()->toolbar.texture,
			  blackbox->tResource()->toolbar.color,
			  blackbox->tResource()->toolbar.colorTo);
  XSetWindowBackgroundPixmap(display, frame.window, frame.frame);

  XGCValues gcv;
  gcv.font = blackbox->titleFont()->fid;
  gcv.foreground = blackbox->tResource()->toolbar.textColor.pixel;
  buttonGC = XCreateGC(display, frame.window,
		       GCFont|GCForeground, &gcv);
  
  frame.workspaceLabel =
    XCreateWindow(display, frame.window, frame.bevel_w, frame.bevel_w,
		  frame.label_w, frame.label_h, 0, blackbox->Depth(),
		  InputOutput, blackbox->visual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.workspaceLabel, this);
  
  frame.label =
    image_ctrl->renderImage(frame.label_w, frame.label_h,
			  blackbox->tResource()->label.texture,
			  blackbox->tResource()->label.color,
			  blackbox->tResource()->label.colorTo);
  XSetWindowBackgroundPixmap(display, frame.workspaceLabel, frame.label);

  frame.workspacePrev =
    XCreateWindow(display, frame.window, frame.bevel_w + frame.label_w + 1,
		  frame.bevel_w, frame.button_w, frame.button_h, 0,
                  blackbox->Depth(), InputOutput, blackbox->visual(),
                  create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.workspacePrev, this);
  
  frame.workspaceNext =
    XCreateWindow(display, frame.window, frame.button_w + frame.label_w +
		  frame.bevel_w + 2, frame.bevel_w,
		  frame.button_w, frame.button_h, 0, blackbox->Depth(),
		  InputOutput, blackbox->visual(), create_mask,
		  &attrib);
  blackbox->saveToolbarSearch(frame.workspaceNext, this);

  frame.button =
    image_ctrl->renderImage(frame.button_w, frame.button_h,
			  blackbox->tResource()->button.texture,
			  blackbox->tResource()->button.color,
			  blackbox->tResource()->button.colorTo);
  frame.pbutton =
    image_ctrl->renderImage(frame.button_w, frame.button_h,
			  blackbox->tResource()->button.ptexture,
			  blackbox->tResource()->button.pressed,
			  blackbox->tResource()->button.pressedTo);
  XSetWindowBackgroundPixmap(display, frame.workspaceNext, frame.button);
  XSetWindowBackgroundPixmap(display, frame.workspacePrev, frame.button);
  
  frame.windowLabel =
    XCreateWindow(display, frame.window, (frame.button_w * 2) + frame.label_w +
		  (frame.bevel_w * 2) + 3, frame.bevel_w, frame.label_w,
		  frame.label_h, 0, blackbox->Depth(), InputOutput,
		  blackbox->visual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.windowLabel, this);
  XSetWindowBackgroundPixmap(display, frame.windowLabel, frame.label);

  frame.windowPrev =
    XCreateWindow(display, frame.window, (frame.button_w * 2) +
		  (frame.label_w * 2) + (frame.bevel_w * 2) + 4, frame.bevel_w,
		  frame.button_w, frame.button_h, 0, blackbox->Depth(),
		  InputOutput, blackbox->visual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.windowPrev, this);
  XSetWindowBackgroundPixmap(display, frame.windowPrev, frame.button);

  frame.windowNext =
    XCreateWindow(display, frame.window, (frame.button_w * 3) +
		  (frame.label_w * 2) + (frame.bevel_w * 2) + 5, frame.bevel_w,
		  frame.button_w, frame.button_h, 0, blackbox->Depth(),
		  InputOutput, blackbox->visual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.windowNext, this);
  XSetWindowBackgroundPixmap(display, frame.windowNext, frame.button);

  frame.iconButton =
    XCreateWindow(display, frame.window, frame.width - frame.clock_w -
		  (frame.button_w * 2) - (frame.bevel_w * 3) - frame.ib_w -
		  3, frame.bevel_w, frame.ib_w, frame.ib_h, 0,
		  blackbox->Depth(), InputOutput, blackbox->visual(),
		  create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.iconButton, this);

  frame.ibutton =
    image_ctrl->renderImage(frame.ib_w, frame.ib_h,
			  blackbox->tResource()->button.texture,
			  blackbox->tResource()->button.color,
			  blackbox->tResource()->button.colorTo);
  frame.pibutton =
    image_ctrl->renderImage(frame.ib_w, frame.ib_h,
			  blackbox->tResource()->button.ptexture,
			  blackbox->tResource()->button.pressed,
			  blackbox->tResource()->button.pressedTo);
  XSetWindowBackgroundPixmap(display, frame.iconButton, frame.ibutton);
  
  frame.raiseButton =
    XCreateWindow(display, frame.window, frame.width - frame.clock_w -
		  (frame.bevel_w * 2) - frame.button_w - 2, frame.bevel_w,
		  frame.button_w, frame.button_h, 0, blackbox->Depth(),
		  InputOutput, blackbox->visual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.raiseButton, this);
  XSetWindowBackgroundPixmap(display, frame.raiseButton, frame.button);

  frame.clock =
    XCreateWindow(display, frame.window, frame.width - frame.clock_w -
		  frame.bevel_w, frame.bevel_w, frame.clock_w, frame.clock_h,
		  0, blackbox->Depth(), InputOutput, blackbox->visual(),
		  create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.clock, this);

  frame.clk =
    image_ctrl->renderImage(frame.clock_w, frame.clock_h,
			  blackbox->tResource()->clock.texture,
			  blackbox->tResource()->clock.color,
			  blackbox->tResource()->clock.colorTo);
  XSetWindowBackgroundPixmap(display, frame.clock, frame.clk);
  
  XClearWindow(display, frame.window);
  XClearWindow(display, frame.workspaceLabel);
  XClearWindow(display, frame.workspacePrev);
  XClearWindow(display, frame.workspaceNext);
  XClearWindow(display, frame.windowLabel);
  XClearWindow(display, frame.windowPrev);
  XClearWindow(display, frame.windowNext);
  XClearWindow(display, frame.iconButton);
  XClearWindow(display, frame.raiseButton);
  XClearWindow(display, frame.clock);

  XMapSubwindows(display, frame.window);
  XMapSubwindows(display, frame.base);
  XMapWindow(display, frame.base);

  wsMenu = new Workspacemenu(blackbox, this);
  iconMenu = new Iconmenu(blackbox);
  iconMenu->Move(frame.x + frame.width - frame.clock_w -
		 (frame.button_w * 2) - (frame.bevel_w * 4) - frame.ib_w,
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
  
  int l = strlen(current->Name()),
    x = (frame.label_w - XTextWidth(blackbox->titleFont(),
				    current->Name(), l))/ 2;
  XDrawString(display, frame.workspaceLabel, buttonGC, x,
	      blackbox->titleFont()->ascent +
	      blackbox->titleFont()->descent, current->Name(), l);

  checkClock(True);

  int hh = frame.button_h / 2, hw = frame.button_w / 2;
  
  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh;
  pts[1].x = 4; pts[1].y = 2;
  pts[2].x = 0; pts[2].y = -4;
  
  XFillPolygon(display, frame.workspacePrev, buttonGC, pts, 3, Convex,
	       CoordModePrevious);
  XFillPolygon(display, frame.windowPrev, buttonGC, pts, 3, Convex,
	       CoordModePrevious);
  
  pts[0].x = hw - 2; pts[0].y = hh - 2;
  pts[1].x = 4; pts[1].y =  2;
  pts[2].x = -4; pts[2].y = 2;
  
  XFillPolygon(display, frame.workspaceNext, buttonGC, pts, 3, Convex,
	       CoordModePrevious);
  XFillPolygon(display, frame.windowNext, buttonGC, pts, 3, Convex,
	       CoordModePrevious);

  XDrawString(display, frame.iconButton, buttonGC, frame.bevel_w,
	      (frame.ib_h + blackbox->titleFont()->ascent -
	       blackbox->titleFont()->descent) / 2,
	      "Icons", strlen("Icons"));

  XDrawRectangle(display, frame.raiseButton, buttonGC,
		 3, 3, frame.button_w - 7, frame.button_h - 7);
  XDrawLine(display, frame.raiseButton, buttonGC,
	    3, 4, frame.button_w - 5, 4);
}


Toolbar::~Toolbar(void) {
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

  if (frame.frame) image_ctrl->removeImage(frame.frame);
  if (frame.label) image_ctrl->removeImage(frame.label);
  if (frame.button) image_ctrl->removeImage(frame.button);
  if (frame.pbutton) image_ctrl->removeImage(frame.pbutton);
  if (frame.ibutton) image_ctrl->removeImage(frame.ibutton);
  if (frame.pibutton) image_ctrl->removeImage(frame.pibutton);
  if (frame.clk) image_ctrl->removeImage(frame.clk);

  blackbox->removeToolbarSearch(frame.clock);
  blackbox->removeToolbarSearch(frame.raiseButton);
  blackbox->removeToolbarSearch(frame.iconButton);
  blackbox->removeToolbarSearch(frame.windowNext);
  blackbox->removeToolbarSearch(frame.windowPrev);
  blackbox->removeToolbarSearch(frame.windowLabel);
  blackbox->removeToolbarSearch(frame.workspaceLabel);
  blackbox->removeToolbarSearch(frame.workspaceNext);
  blackbox->removeToolbarSearch(frame.workspacePrev);
  blackbox->removeToolbarSearch(frame.window);

  XDestroyWindow(display, frame.workspacePrev);
  XDestroyWindow(display, frame.workspaceNext);
  XDestroyWindow(display, frame.workspaceLabel);
  XDestroyWindow(display, frame.windowLabel);
  XDestroyWindow(display, frame.windowPrev);
  XDestroyWindow(display, frame.windowNext);
  XDestroyWindow(display, frame.iconButton);
  XDestroyWindow(display, frame.raiseButton);
  XDestroyWindow(display, frame.clock);
  XDestroyWindow(display, frame.window);
  XDestroyWindow(display, frame.base);
}


int Toolbar::addWorkspace(void) {
  Workspace *wkspc = new Workspace(this, workspacesList->count());
  workspacesList->insert(wkspc);

  wsMenu->insert(wkspc->Name());
  wsMenu->Update();
  wsMenu->Move(frame.x + 1, frame.y - wsMenu->Height() - 2);
  wkspc->Menu()->Move(frame.x + (frame.bevel_w * 2) + frame.label_w +
		      (frame.button_w * 2) + 1, frame.y -
		      wkspc->Menu()->Height() - 2);
  
  return workspacesList->count();
}


int Toolbar::removeLastWorkspace(void) {
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


void Toolbar::addIcon(BlackboxIcon *icon) {
  iconMenu->insert(icon);
  iconMenu->Move(frame.x + frame.width - frame.clock_w -
		 (frame.button_w * 2) - (frame.bevel_w * 4) - frame.ib_w,
		 frame.y - iconMenu->Height() - 2);
}


void Toolbar::removeIcon(BlackboxIcon *icon) {
  iconMenu->remove(icon);
  if ((! iconMenu->Count()) && iconMenu->Visible()) {
    iconMenu->Hide();
    
    XSetWindowBackgroundPixmap(display, frame.iconButton, frame.ibutton);
    XClearWindow(display, frame.iconButton);
    XDrawString(display, frame.iconButton, buttonGC, frame.bevel_w,
		(frame.ib_h + blackbox->titleFont()->ascent -
		 blackbox->titleFont()->descent) / 2,
		"Icons", strlen("Icons"));
  }
  
  iconMenu->Move(frame.x + frame.width - frame.clock_w -
		 (frame.button_w * 2) - (frame.bevel_w * 4) - frame.ib_w,
		 frame.y - iconMenu->Height() - 2);
}


void Toolbar::iconUpdate(void) {
  iconMenu->Update();
  iconMenu->Move(frame.x + frame.width - frame.clock_w -
		 (frame.button_w * 2) - (frame.bevel_w * 4) - frame.ib_w,
		 frame.y - iconMenu->Height() - 2);
}


Workspace *Toolbar::workspace(int w) {
  return workspacesList->find(w);
}


int Toolbar::currentWorkspaceID(void) {
  return current->workspaceID();
}


void Toolbar::changeWorkspaceID(int id) {
  if (id != current->workspaceID()) {
    current->hideAll();
    current = workspace(id);
   
    int l = strlen(current->Name()),
      x = (frame.label_w - XTextWidth(blackbox->titleFont(),
				      current->Name(), l))/ 2;

    XClearWindow(display, frame.workspaceLabel);
    XDrawString(display, frame.workspaceLabel, buttonGC, x,
		blackbox->titleFont()->ascent +
		blackbox->titleFont()->descent, current->Name(), l);
    
    current->showAll();
    XSync(display, False);
  }
}


void Toolbar::stackWindows(Window *workspace_stack, int num) {
  // create window stack... the number of windows for the current workspace,
  // 3 windows for the toolbar, root menu, and workspaces menu and then the
  // number of total workspaces (to stack the workspace menus)

  Window *session_stack =
    new Window[(num + zero->Count() + workspacesList->count() + 4)];
  
  int i = 0;
  *(session_stack + i++) = blackbox->Menu()->WindowID();

  if (raised) {
    *(session_stack + i++) = iconMenu->WindowID();
    *(session_stack + i++) = wsMenu->WindowID();
  
    LinkedListIterator<Workspace> it(workspacesList);
    for (; it.current(); it++) {
      if (it.current() == current)
	it.current()->Menu()->Move(frame.x + (frame.bevel_w * 2) +
				   frame.label_w + (frame.button_w * 2) + 1,
				   frame.y -
				   it.current()->Menu()->Height() - 2);
      *(session_stack + i++) = it.current()->Menu()->WindowID();
    }

    *(session_stack + i++) = frame.base;
  }

  int k = zero->Count();
  while (k--)
    *(session_stack + i++) = *(zero->windowStack() + k);

  k = num;
  while (k--)
    *(session_stack + i++) = *(workspace_stack + k);

  if (! raised) {
    *(session_stack + i++) = iconMenu->WindowID();
    *(session_stack + i++) = wsMenu->WindowID();
    
    LinkedListIterator<Workspace> it(workspacesList);
    for (; it.current(); it++) {
      if (it.current() == current)
	it.current()->Menu()->Move(frame.x + (frame.bevel_w * 2) +
				   frame.label_w + (frame.button_w * 2) + 1,
				   frame.y -
				   it.current()->Menu()->Height() - 2);
      *(session_stack + i++) = it.current()->Menu()->WindowID();
    }

    *(session_stack + i++) = frame.base;
  }
  
  XRestackWindows(display, session_stack, i);
  delete [] session_stack;
}


void Toolbar::Reconfigure(void) {
  wait_button = wait_ibutton = False;
  
  frame.bevel_w = blackbox->bevelWidth();

  frame.width = blackbox->xRes() * 2 / 3;
  frame.height = blackbox->titleFont()->ascent +
    blackbox->titleFont()->descent + (frame.bevel_w * 4);

  frame.x = (blackbox->xRes() - frame.width) / 2;
  frame.y = blackbox->yRes() - frame.height;
  frame.ib_h = frame.clock_h = frame.label_h = frame.button_h =
    frame.height - (frame.bevel_w * 2);

  frame.button_w = (frame.button_h * 3) / 4;
  frame.ib_w = XTextWidth(blackbox->titleFont(), "Icons",
			  strlen("Icons")) + (frame.bevel_w * 2);
  frame.clock_w = XTextWidth(blackbox->titleFont(), "00:00000",
			     strlen("00:00000")) + (frame.bevel_w * 2);

  frame.label_w =
    (frame.width - (frame.clock_w + frame.ib_w + (frame.button_w * 6) +
		    (frame.bevel_w * 6) + 8)) / 2;

  XGCValues gcv;
  gcv.font = blackbox->titleFont()->fid;
  gcv.foreground = blackbox->tResource()->toolbar.textColor.pixel;
  XChangeGC(display, buttonGC, GCFont|GCForeground, &gcv);

  XMoveResizeWindow(display, frame.base, frame.x, frame.y,
		    frame.width, frame.height);
  XResizeWindow(display, frame.window, frame.width, frame.height);
  XMoveResizeWindow(display, frame.workspaceLabel, frame.bevel_w,
		    frame.bevel_w, frame.label_w, frame.label_h);
  XMoveResizeWindow(display, frame.workspacePrev, frame.bevel_w +
		    frame.label_w + 1, frame.bevel_w, frame.button_w,
		    frame.button_h);
  XMoveResizeWindow(display, frame.workspaceNext, frame.bevel_w +
		    frame.label_w + frame.button_w + 2, frame.bevel_w,
		    frame.button_w, frame.button_h);
  XMoveResizeWindow(display, frame.windowLabel, frame.label_w +
		    (frame.button_w * 2) + (frame.bevel_w * 2) + 3,
		    frame.bevel_w, frame.label_w, frame.label_h);
  XMoveResizeWindow(display, frame.windowPrev, (frame.label_w * 2) +
		    (frame.button_w * 2) + (frame.bevel_w * 2) + 4,
		    frame.bevel_w, frame.button_w, frame.button_h);
  XMoveResizeWindow(display, frame.windowNext, (frame.label_w * 2) +
		    (frame.button_w * 3) + (frame.bevel_w * 2) + 5,
		    frame.bevel_w, frame.button_w, frame.button_h);
  XMoveResizeWindow(display, frame.iconButton, frame.width - frame.clock_w -
		    (frame.button_w * 2) - (frame.bevel_w * 3) - frame.ib_w -
		    3, frame.bevel_w, frame.ib_w, frame.ib_h);
  XMoveResizeWindow(display, frame.raiseButton, frame.width - frame.clock_w -
		    (frame.bevel_w * 2) - frame.button_w - 2, frame.bevel_w,
		    frame.button_w, frame.button_h);
  XMoveResizeWindow(display, frame.clock, frame.width - frame.clock_w -
		    frame.bevel_w, frame.bevel_w, frame.clock_w,
		    frame.clock_h);
  
  Pixmap tmp = frame.frame;
  
  frame.frame =
    image_ctrl->renderImage(frame.width, frame.height,
			  blackbox->tResource()->toolbar.texture,
			  blackbox->tResource()->toolbar.color,
			  blackbox->tResource()->toolbar.colorTo);
  if (tmp) image_ctrl->removeImage(tmp);
  XSetWindowBackgroundPixmap(display, frame.window, frame.frame);
  
  tmp = frame.label;
  frame.label =
    image_ctrl->renderImage(frame.label_w, frame.label_h,
			  blackbox->tResource()->label.texture,
			  blackbox->tResource()->label.color,
			  blackbox->tResource()->label.colorTo);
  if (tmp) image_ctrl->removeImage(tmp);
  XSetWindowBackgroundPixmap(display, frame.workspaceLabel, frame.label);
  XSetWindowBackgroundPixmap(display, frame.windowLabel, frame.label);

  tmp = frame.button;
  frame.button =
    image_ctrl->renderImage(frame.button_w, frame.button_h,
			  blackbox->tResource()->button.texture,
			  blackbox->tResource()->button.color,
			  blackbox->tResource()->button.colorTo);
  if (tmp) image_ctrl->removeImage(tmp);

  tmp = frame.pbutton;
  frame.pbutton =
    image_ctrl->renderImage(frame.button_w, frame.button_h,
			  blackbox->tResource()->button.ptexture,
			  blackbox->tResource()->button.pressed,
			  blackbox->tResource()->button.pressedTo);
  if (tmp) image_ctrl->removeImage(tmp);
  XSetWindowBackgroundPixmap(display, frame.workspaceNext, frame.button);
  XSetWindowBackgroundPixmap(display, frame.workspacePrev, frame.button);
  XSetWindowBackgroundPixmap(display, frame.windowPrev, frame.button);
  XSetWindowBackgroundPixmap(display, frame.windowNext, frame.button);
  XSetWindowBackgroundPixmap(display, frame.raiseButton, frame.button);
  
  tmp = frame.ibutton;
  frame.ibutton =
    image_ctrl->renderImage(frame.ib_w, frame.ib_h,
			  blackbox->tResource()->button.texture,
			  blackbox->tResource()->button.color,
			  blackbox->tResource()->button.colorTo);
  if (tmp) image_ctrl->removeImage(tmp);

  tmp = frame.pibutton;
  frame.pibutton =
    image_ctrl->renderImage(frame.ib_w, frame.ib_h,
			  blackbox->tResource()->button.ptexture,
			  blackbox->tResource()->button.pressed,
			  blackbox->tResource()->button.pressedTo);
  if (tmp) image_ctrl->removeImage(tmp);
  XSetWindowBackgroundPixmap(display, frame.iconButton,
			     ((iconMenu->Visible()) ? frame.pibutton :
			      frame.ibutton));

  tmp = frame.clk;
  frame.clk =
    image_ctrl->renderImage(frame.clock_w, frame.clock_h,
			  blackbox->tResource()->clock.texture,
			  blackbox->tResource()->clock.color,
			  blackbox->tResource()->clock.colorTo);
  if (tmp) image_ctrl->removeImage(tmp);
  XSetWindowBackgroundPixmap(display, frame.clock, frame.clk);
  
  XSetWindowBackground(display, frame.base, blackbox->borderColor().pixel);
  XSetWindowBorder(display, frame.base, blackbox->borderColor().pixel);
  
  XClearWindow(display, frame.window);
  XClearWindow(display, frame.workspaceLabel);
  XClearWindow(display, frame.workspaceNext);
  XClearWindow(display, frame.workspacePrev);
  XClearWindow(display, frame.windowLabel);
  XClearWindow(display, frame.windowPrev);
  XClearWindow(display, frame.windowNext);
  XClearWindow(display, frame.iconButton);
  XClearWindow(display, frame.raiseButton);
  XClearWindow(display, frame.clock);
 
  int l = strlen(current->Name()),
    x = (frame.label_w - XTextWidth(blackbox->titleFont(),
				    current->Name(), l))/ 2;
  XDrawString(display, frame.workspaceLabel, buttonGC, x,
	      blackbox->titleFont()->ascent +
	      blackbox->titleFont()->descent, current->Name(), l);
  
  redrawLabel();
  checkClock(True);

  int hh = frame.button_h / 2, hw = frame.button_w / 2;
  
  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh;
  pts[1].x = 4; pts[1].y = 2;
  pts[2].x = 0; pts[2].y = -4;
  
  XFillPolygon(display, frame.workspacePrev, buttonGC, pts, 3, Convex,
	       CoordModePrevious);
  XFillPolygon(display, frame.windowPrev, buttonGC, pts, 3, Convex,
	       CoordModePrevious);
  
  pts[0].x = hw - 2; pts[0].y = hh - 2;
  pts[1].x = 4; pts[1].y =  2;
  pts[2].x = -4; pts[2].y = 2;
  
  XFillPolygon(display, frame.workspaceNext, buttonGC, pts, 3, Convex,
	       CoordModePrevious);
  XFillPolygon(display, frame.windowNext, buttonGC, pts, 3, Convex,
	       CoordModePrevious);

  wsMenu->Reconfigure();
  wsMenu->Move(frame.x + 1, frame.y - wsMenu->Height() - 2);

  iconMenu->Reconfigure();
  iconMenu->Move(frame.x + frame.width - frame.clock_w -
		 (frame.button_w * 2) - (frame.bevel_w * 4) - frame.ib_w,
		 frame.y - iconMenu->Height() - 2);

  XDrawString(display, frame.iconButton, buttonGC, frame.bevel_w,
	      (frame.ib_h + blackbox->titleFont()->ascent -
	       blackbox->titleFont()->descent) / 2,
	      "Icons", strlen("Icons"));

  XDrawRectangle(display, frame.raiseButton, buttonGC,
		 3, 3, frame.button_w - 7, frame.button_h - 7);
  XDrawLine(display, frame.raiseButton, buttonGC,
	    3, 4, frame.button_w - 5, 4);

  LinkedListIterator<Workspace> it(workspacesList);
  for (; it.current(); it++) {
    it.current()->Reconfigure();
    it.current()->Menu()->Move(frame.x + (frame.bevel_w * 2) + frame.label_w +
			       (frame.button_w * 2) + 1, frame.y -
			       it.current()->Menu()->Height() - 2);
  }

  current->restackWindows();
}


void Toolbar::checkClock(Bool redraw) {
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
      sprintf(t, "  %02d:%02d ", hour, minute);
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


void Toolbar::redrawLabel(Bool redraw) {
  if (current->Label()) {

    if (redraw)
      XClearWindow(display, frame.windowLabel);
    
    int l = strlen(*current->Label()),
      tx = (XTextWidth(blackbox->titleFont(), *current->Label(), l) +
	    (frame.bevel_w * 2)),
      x = (frame.label_w - tx) / 2;

    if (tx > (signed) frame.label_w) x = frame.bevel_w;

    XDrawString(display, frame.windowLabel, buttonGC, x,
		blackbox->titleFont()->ascent +
		blackbox->titleFont()->descent, *current->Label(), l);
  } else
    XClearWindow(display, frame.windowLabel);
}


// *************************************************************************
// event handlers
// *************************************************************************

void Toolbar::buttonPressEvent(XButtonEvent *be) {
  if (be->button == 1) {
    if (be->window == frame.workspacePrev) {
      XSetWindowBackgroundPixmap(display, frame.workspacePrev, frame.pbutton);
      XClearWindow(display, frame.workspacePrev);
      int hh = frame.button_h / 2, hw = frame.button_w / 2;
      
      XPoint pts[3];
      pts[0].x = hw - 2; pts[0].y = hh;
      pts[1].x = 4; pts[1].y = 2;
      pts[2].x = 0; pts[2].y = -4;
    
      XFillPolygon(display, frame.workspacePrev, buttonGC, pts, 3, Convex,
		   CoordModePrevious);
    } else if (be->window == frame.workspaceNext) {
      XSetWindowBackgroundPixmap(display, frame.workspaceNext, frame.pbutton);
      XClearWindow(display, frame.workspaceNext);
      int hh = frame.button_h / 2, hw = frame.button_w / 2;
    
      XPoint pts[3];
      pts[0].x = hw - 2; pts[0].y = hh - 2;
      pts[1].x = 4; pts[1].y =  2;
      pts[2].x = -4; pts[2].y = 2;
    
      XFillPolygon(display, frame.workspaceNext, buttonGC, pts, 3, Convex,
		   CoordModePrevious);
    } else if (be->window == frame.workspaceLabel) {
      if (! wsMenu->Visible() && wsMenu->Count()) {
	wsMenu->Show();
	wait_button = True;
      }
    } else if (be->window == frame.windowLabel) {
      if (! current->Menu()->Visible() && current->Menu()->Count()) {
	current->Menu()->Show();
	wait_button = True;
      }
    } else if (be->window == frame.windowPrev) {
      XSetWindowBackgroundPixmap(display, frame.windowPrev, frame.pbutton);
      XClearWindow(display, frame.windowPrev);
      int hh = frame.button_h / 2, hw = frame.button_w / 2;
      
      XPoint pts[3];
      pts[0].x = hw - 2; pts[0].y = hh;
      pts[1].x = 4; pts[1].y = 2;
      pts[2].x = 0; pts[2].y = -4;
      
      XFillPolygon(display, frame.windowPrev, buttonGC, pts, 3, Convex,
		   CoordModePrevious);
    } else if (be->window == frame.windowNext) {
      XSetWindowBackgroundPixmap(display, frame.windowNext, frame.pbutton);
      XClearWindow(display, frame.windowNext);
      int hh = frame.button_h / 2, hw = frame.button_w / 2;
      
      XPoint pts[3];
      pts[0].x = hw - 2; pts[0].y = hh - 2;
      pts[1].x = 4; pts[1].y =  2;
      pts[2].x = -4; pts[2].y = 2;
      
      XFillPolygon(display, frame.windowNext, buttonGC, pts, 3, Convex,
		   CoordModePrevious);
    } else if (be->window == frame.iconButton) {
      if (! iconMenu->Visible() && iconMenu->Count()) {
	XSetWindowBackgroundPixmap(display, frame.iconButton, frame.pibutton);
	XClearWindow(display, frame.iconButton);
	XDrawString(display, frame.iconButton, buttonGC, frame.bevel_w,
		    (frame.ib_h + blackbox->titleFont()->ascent -
		     blackbox->titleFont()->descent) / 2,
		    "Icons", strlen("Icons"));

	iconMenu->Show();
	wait_ibutton = True;
      }
    } else if (be->window == frame.raiseButton) {
      XSetWindowBackgroundPixmap(display, frame.raiseButton, frame.pbutton);
      XClearWindow(display, frame.raiseButton);
      XDrawRectangle(display, frame.raiseButton, buttonGC,
		     3, 3, frame.button_w - 7, frame.button_h - 7);
      XDrawLine(display, frame.raiseButton, buttonGC,
		3, 4, frame.button_w - 5, 4);
    }
  }
}


void Toolbar::buttonReleaseEvent(XButtonEvent *re) {
  if (re->button == 1) {
    if (re->window == frame.workspacePrev) {
      XSetWindowBackgroundPixmap(display, frame.workspacePrev, frame.button);
      XClearWindow(display, frame.workspacePrev);
      int hh = frame.button_h / 2, hw = frame.button_w / 2;
      
      XPoint pts[3];
      pts[0].x = hw - 2; pts[0].y = hh;
      pts[1].x = 4; pts[1].y = 2;
      pts[2].x = 0; pts[2].y = -4;
    
      XFillPolygon(display, frame.workspacePrev, buttonGC, pts, 3, Convex,
		   CoordModePrevious);

      if (re->x >= 0 && re->x < (signed) frame.button_w &&
	  re->y >= 0 && re->y < (signed) frame.button_h)
	if (current->workspaceID() > 1)
	  changeWorkspaceID(current->workspaceID() - 1);
	else
	  changeWorkspaceID(workspacesList->count() - 1);
    } else if (re->window == frame.workspaceNext) {
      XSetWindowBackgroundPixmap(display, frame.workspaceNext, frame.button);
      XClearWindow(display, frame.workspaceNext);
      int hh = frame.button_h / 2, hw = frame.button_w / 2;
    
      XPoint pts[3];
      pts[0].x = hw - 2; pts[0].y = hh - 2;
      pts[1].x = 4; pts[1].y =  2;
      pts[2].x = -4; pts[2].y = 2;
    
      XFillPolygon(display, frame.workspaceNext, buttonGC, pts, 3, Convex,
		   CoordModePrevious);
      
      if (re->x >= 0 && re->x < (signed) frame.button_w &&
	  re->y >= 0 && re->y < (signed) frame.button_h)
	if (current->workspaceID() < (workspacesList->count() - 1))
	  changeWorkspaceID(current->workspaceID() + 1);
	else
	  changeWorkspaceID(1);
    } else if (re->window == frame.workspaceLabel) {
      if (! wait_button)
	wsMenu->Hide();
      else
	wait_button = False;
    } else if (re->window == frame.windowLabel) {
      if (! wait_button)
	current->Menu()->Hide();
      else
	wait_button = False;
    } else if (re->window == frame.windowPrev) {
      XSetWindowBackgroundPixmap(display, frame.windowPrev, frame.button);
      XClearWindow(display, frame.windowPrev);
      int hh = frame.button_h / 2, hw = frame.button_w / 2;
      
      XPoint pts[3];
      pts[0].x = hw - 2; pts[0].y = hh;
      pts[1].x = 4; pts[1].y = 2;
      pts[2].x = 0; pts[2].y = -4;
      
      XFillPolygon(display, frame.windowPrev, buttonGC, pts, 3, Convex,
		   CoordModePrevious);
      if (re->x >= 0 && re->x < (signed) frame.button_w &&
	  re->y >= 0 && re->y < (signed) frame.button_h)
	blackbox->prevFocus();
    } else if (re->window == frame.windowNext) {
      XSetWindowBackgroundPixmap(display, frame.windowNext, frame.button);
      XClearWindow(display, frame.windowNext);
      int hh = frame.button_h / 2, hw = frame.button_w / 2;

      XPoint pts[3];
      pts[0].x = hw - 2; pts[0].y = hh - 2;
      pts[1].x = 4; pts[1].y =  2;
      pts[2].x = -4; pts[2].y = 2;

      XFillPolygon(display, frame.windowNext, buttonGC, pts, 3, Convex,
		   CoordModePrevious);
      if (re->x >= 0 && re->x < (signed) frame.button_w &&
	  re->y >= 0 && re->y < (signed) frame.button_h)
	blackbox->nextFocus();
    } else if (re->window == frame.iconButton) {
      if (! wait_ibutton) {
	XSetWindowBackgroundPixmap(display, frame.iconButton, frame.ibutton);
	XClearWindow(display, frame.iconButton);
	XDrawString(display, frame.iconButton, buttonGC, frame.bevel_w,
		    (frame.ib_h + blackbox->titleFont()->ascent -
		     blackbox->titleFont()->descent) / 2,
		    "Icons", strlen("Icons"));

	iconMenu->Hide();
      } else
	wait_ibutton = False;
    } else if (re->window == frame.raiseButton) {
      XSetWindowBackgroundPixmap(display, frame.raiseButton, frame.button);
      XClearWindow(display, frame.raiseButton);
      XDrawRectangle(display, frame.raiseButton, buttonGC,
		     3, 3, frame.button_w - 7, frame.button_h - 7);
      XDrawLine(display, frame.raiseButton, buttonGC,
		3, 4, frame.button_w - 5, 4);
      
      if (re->x >= 0 && re->x < (signed) frame.button_w &&
	  re->y >= 0 && re->y < (signed) frame.button_h) {
	raised = ((raised) ? False : True);
	current->restackWindows();
      }
    }
  }
}


void Toolbar::exposeEvent(XExposeEvent *ee) {
  if (ee->window == frame.clock) {
    checkClock(True);
  } else if (ee->window == frame.workspaceLabel) {
    int l = strlen(current->Name()),
      x = (frame.label_w - XTextWidth(blackbox->titleFont(),
				      current->Name(), l))/ 2;
    XDrawString(display, frame.workspaceLabel, buttonGC, x,
		blackbox->titleFont()->ascent +
		blackbox->titleFont()->descent, current->Name(), l);
  } else if (ee->window == frame.windowLabel) {
    redrawLabel();
  } else if (ee->window == frame.iconButton) {
    XDrawString(display, frame.iconButton, buttonGC, frame.bevel_w,
		(frame.ib_h + blackbox->titleFont()->ascent -
		 blackbox->titleFont()->descent) / 2,
		"Icons", strlen("Icons"));
  } else if (ee->window == frame.workspacePrev) {
    int hh = frame.button_h / 2, hw = frame.button_w / 2;
    
    XPoint pts[3];
    pts[0].x = hw - 2; pts[0].y = hh;
    pts[1].x = 4; pts[1].y = 2;
    pts[2].x = 0; pts[2].y = -4;
    
    XFillPolygon(display, frame.workspacePrev, buttonGC, pts, 3, Convex,
		 CoordModePrevious);
  } else if (ee->window == frame.workspaceNext) {
    int hh = frame.button_h / 2, hw = frame.button_w / 2;
    
    XPoint pts[3];
    pts[0].x = hw - 2; pts[0].y = hh - 2;
    pts[1].x = 4; pts[1].y =  2;
    pts[2].x = -4; pts[2].y = 2;
    
    XFillPolygon(display, frame.workspaceNext, buttonGC, pts, 3, Convex,
		 CoordModePrevious);
  } else if (ee->window == frame.windowPrev) {
        int hh = frame.button_h / 2, hw = frame.button_w / 2;
    
    XPoint pts[3];
    pts[0].x = hw - 2; pts[0].y = hh;
    pts[1].x = 4; pts[1].y = 2;
    pts[2].x = 0; pts[2].y = -4;
    
    XFillPolygon(display, frame.windowPrev, buttonGC, pts, 3, Convex,
		 CoordModePrevious);
  } else if (ee->window == frame.windowNext) {
    int hh = frame.button_h / 2, hw = frame.button_w / 2;
    
    XPoint pts[3];
    pts[0].x = hw - 2; pts[0].y = hh - 2;
    pts[1].x = 4; pts[1].y =  2;
    pts[2].x = -4; pts[2].y = 2;
    
    XFillPolygon(display, frame.windowNext, buttonGC, pts, 3, Convex,
		 CoordModePrevious);
  } else if (ee->window == frame.raiseButton) {
    XDrawRectangle(display, frame.raiseButton, buttonGC,
		   3, 3, frame.button_w - 7, frame.button_h - 7);
    XDrawLine(display, frame.raiseButton, buttonGC,
	      3, 4, frame.button_w - 5, 4);
  }
}
