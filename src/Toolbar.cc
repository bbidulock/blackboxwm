//
// Toolbar.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 1999 by Brad Hughes, bhughes@tcac.net
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "blackbox.hh"
#include "Clientmenu.hh"
#include "Icon.hh"
#include "Rootmenu.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"

#include <X11/Xutil.h>
#include <X11/keysym.h>

#if HAVE_STDIO_H
#  include <stdio.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif


Toolbar::Toolbar(Blackbox *bb, BScreen *scrn) {
  blackbox = bb;
  screen = scrn;

  image_ctrl = screen->getImageControl();

  editing = wait_button = False;
  on_top = screen->isToolbarOnTop();
  new_workspace_name = new_name_pos = 0;

  display = blackbox->getDisplay();
  XSetWindowAttributes attrib;
  unsigned long create_mask = CWBackPixmap|CWBackPixel|CWBorderPixel|
    CWOverrideRedirect |CWCursor|CWEventMask; 
  attrib.background_pixmap = None;
  attrib.background_pixel = attrib.border_pixel =
    screen->getBorderColor().pixel;
  attrib.override_redirect = True;
  attrib.cursor = blackbox->getSessionCursor();
  attrib.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask |
    FocusChangeMask | KeyPressMask;

  frame.window =
    XCreateWindow(display, screen->getRootWindow(), 0, 0, 1, 1, 0,
		  screen->getDepth(), InputOutput, screen->getVisual(),
		  create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.window, this);
  
  XGCValues gcv;
  gcv.font = screen->getTitleFont()->fid;
  gcv.foreground = screen->getTResource()->toolbar.textColor.pixel;
  buttonGC = XCreateGC(display, frame.window,
		       GCFont|GCForeground, &gcv);
  
  frame.menuButton =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
		  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.menuButton, this);

  frame.workspaceLabel =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
		  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.workspaceLabel, this);
  
  frame.workspacePrev =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
		  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.workspacePrev, this);
  
  frame.workspaceNext =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
		  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.workspaceNext, this);
  
  frame.windowLabel =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
		  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.windowLabel, this);
  
  frame.windowPrev =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
		  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.windowPrev, this);

  frame.windowNext =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
		  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.windowNext, this);

  frame.clock =
    XCreateWindow(display, frame.window, 0, 0, 1, 1, 0, screen->getDepth(),
		  InputOutput, screen->getVisual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.clock, this);
  
  frame.frame = frame.label = frame.button = frame.pbutton = frame.clk =
    frame.reading = None;
  
  reconfigure();
  
  XMapSubwindows(display, frame.window);
  XMapWindow(display, frame.window);
}


Toolbar::~Toolbar(void) {
  XUnmapWindow(display, frame.window);

  if (frame.frame) image_ctrl->removeImage(frame.frame);
  if (frame.label) image_ctrl->removeImage(frame.label);
  if (frame.button) image_ctrl->removeImage(frame.button);
  if (frame.pbutton) image_ctrl->removeImage(frame.pbutton);
  if (frame.clk) image_ctrl->removeImage(frame.clk);
  if (frame.reading) image_ctrl->removeImage(frame.reading);

  blackbox->removeToolbarSearch(frame.window);

  blackbox->removeToolbarSearch(frame.menuButton);
  blackbox->removeToolbarSearch(frame.workspaceLabel);
  blackbox->removeToolbarSearch(frame.workspacePrev);
  blackbox->removeToolbarSearch(frame.workspaceNext);
  blackbox->removeToolbarSearch(frame.windowLabel);
  blackbox->removeToolbarSearch(frame.windowPrev);
  blackbox->removeToolbarSearch(frame.windowNext);
  blackbox->removeToolbarSearch(frame.clock);
  
  XDestroyWindow(display, frame.menuButton);
  XDestroyWindow(display, frame.workspaceLabel);
  XDestroyWindow(display, frame.workspacePrev);
  XDestroyWindow(display, frame.workspaceNext);
  XDestroyWindow(display, frame.windowLabel);
  XDestroyWindow(display, frame.windowPrev);
  XDestroyWindow(display, frame.windowNext);
  XDestroyWindow(display, frame.clock);

  XDestroyWindow(display, frame.window);
}


void Toolbar::reconfigure(void) {
  wait_button = False;
  
  frame.bevel_w = screen->getBevelWidth();
  frame.width = screen->getXRes() * screen->getToolbarWidthPercent() / 100;
  frame.height = screen->getTitleFont()->ascent +
    screen->getTitleFont()->descent + (frame.bevel_w * 4);
  frame.x = (screen->getXRes() - frame.width) / 2;
  frame.y = screen->getYRes() - frame.height;
  frame.clock_h = frame.label_h = frame.wlabel_h = frame.button_h =
    frame.height - (frame.bevel_w * 2);
  frame.button_w = (frame.button_h * 3) / 4;;

#ifdef HAVE_STRFTIME
  time_t ttmp = time(NULL);
  struct tm *tt = 0;
  
  if (ttmp != -1) {
    tt= localtime(&ttmp);
    if (tt) {
      char t[1024], *time_string = (char *) 0;
      int i, len = strftime(t, 1024, screen->getStrftimeFormat(), tt);
      
      time_string = new char[len + 1];
      for (i = 0; i < len; i++)
	*(time_string + i) = '0';
      *(time_string + len + 1) = '\0';
      
      frame.clock_w = XTextWidth(screen->getTitleFont(), time_string, len) +
	(frame.bevel_w * 2);

      delete [] time_string;
    } else
      frame.clock_w = 0;
  } else
    frame.clock_w = 0;
#else
  frame.clock_w = XTextWidth(screen->getTitleFont(), "00:00000",
			     strlen("00:00000")) + (frame.bevel_w * 2);
#endif

  int i;
  unsigned int w = 0;
  frame.wlabel_w = 0;

  for (i = 0; i < screen->getCount(); i++) {
    w = XTextWidth(screen->getTitleFont(),
		   screen->getWorkspace(i)->getName(),
		   strlen(screen->getWorkspace(i)->getName())) +
      (frame.bevel_w * 2);
    
    if (w > frame.wlabel_w) frame.wlabel_w = w;
  }

  frame.wlabel_w += (frame.wlabel_h * 2);
  if (frame.wlabel_w < frame.clock_w) frame.wlabel_w = frame.clock_w;

  frame.label_w = (frame.width -
		   (frame.clock_w + (frame.button_w * 5) + frame.wlabel_w +
		    (frame.bevel_w * 5) + 8));
  
  Pixmap tmp = frame.frame;
  frame.frame =
    image_ctrl->renderImage(frame.width, frame.height,
			    &(screen->getTResource()->toolbar.texture));
  if (tmp) image_ctrl->removeImage(tmp);
  
  tmp = frame.label;
  frame.label =
    image_ctrl->renderImage(frame.label_w, frame.label_h,
			    &(screen->getTResource()->label.texture));
  if (tmp) image_ctrl->removeImage(tmp);

  tmp = frame.wlabel;
  frame.wlabel =
    image_ctrl->renderImage(frame.wlabel_w, frame.wlabel_h,
			    &(screen->getTResource()->label.texture));
  if (tmp) image_ctrl->removeImage(tmp);
  
  tmp = frame.button;
  frame.button =
    image_ctrl->renderImage(frame.button_w, frame.button_h,
			    &(screen->getTResource()->button.texture));
  if (tmp) image_ctrl->removeImage(tmp);
  
  tmp = frame.pbutton;
  frame.pbutton =
    image_ctrl->renderImage(frame.button_w, frame.button_h,
			    &(screen->getTResource()->button.pressedTexture));
  if (tmp) image_ctrl->removeImage(tmp);
  
  tmp = frame.clk;
  frame.clk =
    image_ctrl->renderImage(frame.clock_w, frame.clock_h,
			    &(screen->getTResource()->clock.texture));
  if (tmp) image_ctrl->removeImage(tmp);

  tmp = frame.reading;
  frame.reading =
    image_ctrl->renderImage(frame.wlabel_w, frame.wlabel_h,
			    &(screen->getWResource()->
			      decoration.focusTexture));
  if (tmp) image_ctrl->removeImage(tmp);
  
  XGCValues gcv;
  gcv.font = screen->getTitleFont()->fid;
  gcv.foreground = screen->getTResource()->toolbar.textColor.pixel;
  XChangeGC(display, buttonGC, GCFont|GCForeground, &gcv);
  
  XMoveResizeWindow(display, frame.window, frame.x, frame.y,
		    frame.width, frame.height);
  XMoveResizeWindow(display, frame.menuButton, frame.bevel_w, frame.bevel_w,
		    frame.button_w, frame.button_h);
  XMoveResizeWindow(display, frame.workspaceLabel, (frame.bevel_w * 2) +
		    frame.button_w + 1, frame.bevel_w, frame.wlabel_w,
		    frame.wlabel_h);
  XMoveResizeWindow(display, frame.workspacePrev, (frame.bevel_w * 2) +
		    frame.wlabel_w + frame.button_w + 2, frame.bevel_w,
		    frame.button_w, frame.button_h);
  XMoveResizeWindow(display, frame.workspaceNext, (frame.bevel_w * 2) +
		    frame.wlabel_w + (frame.button_w * 2) + 3, frame.bevel_w,
		    frame.button_w, frame.button_h);
  XMoveResizeWindow(display, frame.windowLabel, (frame.bevel_w * 3) +
		    (frame.button_w * 3) + frame.wlabel_w + 4,
		    frame.bevel_w, frame.label_w, frame.label_h);
  XMoveResizeWindow(display, frame.windowPrev, (frame.bevel_w * 3) +
		    (frame.button_w * 3) + frame.wlabel_w + frame.label_w + 5,
		    frame.bevel_w, frame.button_w, frame.button_h);
  XMoveResizeWindow(display, frame.windowNext, (frame.bevel_w * 3) +
		    (frame.button_w * 4) + frame.wlabel_w + frame.label_w + 6,
		    frame.bevel_w, frame.button_w, frame.button_h);
  XMoveResizeWindow(display, frame.clock, frame.width - frame.clock_w -
		    frame.bevel_w, frame.bevel_w, frame.clock_w,
		    frame.clock_h);
  
  XSetWindowBackgroundPixmap(display, frame.window, frame.frame);
  XSetWindowBackgroundPixmap(display, frame.menuButton,
                             ((screen->getWorkspacemenu()->isVisible()) ?
			      frame.pbutton : frame.button));
  XSetWindowBackgroundPixmap(display, frame.workspaceLabel, frame.wlabel);
  XSetWindowBackgroundPixmap(display, frame.windowLabel, frame.label);
  XSetWindowBackgroundPixmap(display, frame.workspaceNext, frame.button);
  XSetWindowBackgroundPixmap(display, frame.workspacePrev, frame.button);
  XSetWindowBackgroundPixmap(display, frame.windowPrev, frame.button);
  XSetWindowBackgroundPixmap(display, frame.windowNext, frame.button);
  XSetWindowBackgroundPixmap(display, frame.clock, frame.clk);
  
  XSetWindowBorder(display, frame.window, screen->getBorderColor().pixel);
  
  XClearWindow(display, frame.window);
  XClearWindow(display, frame.menuButton);
  XClearWindow(display, frame.workspaceLabel);
  XClearWindow(display, frame.workspaceNext);
  XClearWindow(display, frame.workspacePrev);
  XClearWindow(display, frame.windowLabel);
  XClearWindow(display, frame.windowPrev);
  XClearWindow(display, frame.windowNext);
  XClearWindow(display, frame.clock);
  
  redrawWindowLabel();
  redrawWorkspaceLabel();
  checkClock(True);

  redrawMenuButton();
  redrawPrevWorkspaceButton();
  redrawNextWorkspaceButton();
  redrawPrevWindowButton();
  redrawNextWindowButton();

  if (screen->getWorkspacemenu()->isVisible())
    screen->getWorkspacemenu()->
      move(frame.x, frame.y -
	   screen->getWorkspacemenu()->getHeight() - 1);
}


#ifdef    HAVE_STRFTIME
void Toolbar::checkClock(Bool redraw) {
#else //  HAVE_STRFTIME
void Toolbar::checkClock(Bool redraw, Bool date) {
#endif // HAVE_STRFTIME
  time_t tmp = 0;
  struct tm *tt = 0;
  
  if ((tmp = time(NULL)) != -1) {
    if (! (tt = localtime(&tmp))) return;
    if (tt->tm_min != frame.minute || tt->tm_hour != frame.hour) {
      frame.hour = tt->tm_hour;
      frame.minute = tt->tm_min;
      XClearWindow(display, frame.clock);
      redraw = True;
    }
  }

  if (redraw) {
#ifdef    HAVE_STRFTIME
    char t[1024];
    if (! strftime(t, 1024, screen->getStrftimeFormat(), tt))
      return;
#else //  HAVE_STRFTIME
    char t[9];
    if (date) {
      // format the date... with special consideration for y2k ;)
      if (screen->getDateFormat() == Blackbox::B_EuropeanDate)
        sprintf(t, "%02d.%02d.%02d", tt->tm_mday, tt->tm_mon + 1,
                (tt->tm_year >= 100) ? tt->tm_year - 100 : tt->tm_year);
      else
        sprintf(t, "%02d/%02d/%02d", tt->tm_mon + 1, tt->tm_mday,
                (tt->tm_year >= 100) ? tt->tm_year - 100 : tt->tm_year);
    } else {
      if (blackbox->isClock24Hour())
	sprintf(t, "  %02d:%02d ", frame.hour, frame.minute);
      else
	sprintf(t, "%02d:%02d %cm",
		((frame.hour > 12) ? frame.hour - 12 :
		 ((frame.hour == 0) ? 12 : frame.hour)), frame.minute,
		((frame.hour >= 12) ? 'p' : 'a'));
    }
#endif // HAVE_STRFTIME

    int len = strlen(t);
    unsigned int slen = XTextWidth(screen->getTitleFont(), t, len);
    XDrawString(display, frame.clock, buttonGC, (frame.clock_w - slen) / 2,
		(frame.clock_h + screen->getTitleFont()->ascent -
		 screen->getTitleFont()->descent) / 2,
		t, len);
  }
}


void Toolbar::redrawWindowLabel(Bool redraw) {
  if (screen->getCurrentWorkspace()->getLabel()) {
    if (redraw)
      XClearWindow(display, frame.windowLabel);
    
    int l = strlen(*(screen->getCurrentWorkspace()->getLabel())),
      tx = (XTextWidth(screen->getTitleFont(),
		       *(screen->getCurrentWorkspace()->getLabel()), l) +
	    (frame.bevel_w * 2)),
      x = (frame.label_w - tx) / 2;
    
    if (tx > (signed) frame.label_w) x = frame.bevel_w;
    
    XDrawString(display, frame.windowLabel, buttonGC, x,
		(frame.label_h + screen->getTitleFont()->ascent -
                 screen->getTitleFont()->descent) / 2,
                *(screen->getCurrentWorkspace()->getLabel()), l);
  } else
    XClearWindow(display, frame.windowLabel);
}
 
 
void Toolbar::redrawWorkspaceLabel(Bool redraw) {
  if (screen->getCurrentWorkspace()->getName()) {
    if (redraw)
      XClearWindow(display, frame.workspaceLabel);
    
    int l = strlen(screen->getCurrentWorkspace()->getName()),
      tx = (XTextWidth(screen->getTitleFont(),
		       screen->getCurrentWorkspace()->getName(), l) +
	    (frame.bevel_w * 2)),
      x = (frame.wlabel_w - tx) / 2;
    
    if (tx > (signed) frame.wlabel_w) x = frame.bevel_w;
    
    XDrawString(display, frame.workspaceLabel, buttonGC, x,
		(frame.wlabel_h + screen->getTitleFont()->ascent -
                 screen->getTitleFont()->descent) / 2,
                screen->getCurrentWorkspace()->getName(), l);
  }
}


void Toolbar::redrawMenuButton(Bool pressed, Bool redraw) {
  if (redraw) {
    XSetWindowBackgroundPixmap(display, frame.menuButton,
			       ((pressed) ? frame.pbutton : frame.button));
    XClearWindow(display, frame.menuButton);
  }
  
  int hh = frame.button_h / 2, hw = frame.button_w / 2;
  
  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh + 2;
  pts[1].x = 4; pts[1].y = 0;
  pts[2].x = -2; pts[2].y = -4;
  
  XFillPolygon(display, frame.menuButton, buttonGC, pts, 3, Convex,
	       CoordModePrevious);
}


void Toolbar::redrawPrevWorkspaceButton(Bool pressed, Bool redraw) {
  if (redraw) {
    XSetWindowBackgroundPixmap(display, frame.workspacePrev,
			       ((pressed) ? frame.pbutton : frame.button));
    XClearWindow(display, frame.workspacePrev);
  }

  int hh = frame.button_h / 2, hw = frame.button_w / 2;
  
  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh;
  pts[1].x = 4; pts[1].y = 2;
  pts[2].x = 0; pts[2].y = -4;
  
  XFillPolygon(display, frame.workspacePrev, buttonGC, pts, 3, Convex,
	       CoordModePrevious);
}


void Toolbar::redrawNextWorkspaceButton(Bool pressed, Bool redraw) {
  if (redraw) {
    XSetWindowBackgroundPixmap(display, frame.workspaceNext,
			       ((pressed) ? frame.pbutton : frame.button));
    XClearWindow(display, frame.workspaceNext);
  }

  int hh = frame.button_h / 2, hw = frame.button_w / 2;
  
  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh - 2;
  pts[1].x = 4; pts[1].y =  2;
  pts[2].x = -4; pts[2].y = 2;
  
  XFillPolygon(display, frame.workspaceNext, buttonGC, pts, 3, Convex,
	       CoordModePrevious);
}


void Toolbar::redrawPrevWindowButton(Bool pressed, Bool redraw) {
  if (redraw) {
    XSetWindowBackgroundPixmap(display, frame.windowPrev,
			       ((pressed) ? frame.pbutton : frame.button));
    XClearWindow(display, frame.windowPrev);
  }

  int hh = frame.button_h / 2, hw = frame.button_w / 2;
  
  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh;
  pts[1].x = 4; pts[1].y = 2;
  pts[2].x = 0; pts[2].y = -4;
  
  XFillPolygon(display, frame.windowPrev, buttonGC, pts, 3, Convex,
	       CoordModePrevious);
}


void Toolbar::redrawNextWindowButton(Bool pressed, Bool redraw) {
  if (redraw) {
    XSetWindowBackgroundPixmap(display, frame.windowNext,
			       ((pressed) ? frame.pbutton : frame.button));
    XClearWindow(display, frame.windowNext);
  }
  
  int hh = frame.button_h / 2, hw = frame.button_w / 2;
  
  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh - 2;
  pts[1].x = 4; pts[1].y =  2;
  pts[2].x = -4; pts[2].y = 2;
  
  XFillPolygon(display, frame.windowNext, buttonGC, pts, 3, Convex,
	       CoordModePrevious);
}


void Toolbar::buttonPressEvent(XButtonEvent *be) {
  if (be->button == 1) {
    if (be->window == frame.menuButton) {
      if (! screen->getWorkspacemenu()->isVisible()) {
	redrawMenuButton(True, True);
	
	screen->getWorkspacemenu()->
	  move(frame.x, frame.y - screen->getWorkspacemenu()->getHeight() - 1);
	screen->getWorkspacemenu()->show();
	
	wait_button = True;
      }
    } else if (be->window == frame.workspacePrev) {
      redrawPrevWorkspaceButton(True, True);
    } else if (be->window == frame.workspaceNext) {
      redrawNextWorkspaceButton(True, True);
    } else if (be->window == frame.windowPrev) {
      redrawPrevWindowButton(True, True);
    } else if (be->window == frame.windowNext) {
      redrawNextWindowButton(True, True);
#ifndef   HAVE_STRFTIME
    } else if (be->window == frame.clock) {
      XClearWindow(display, frame.clock);

      checkClock(True, True);
#endif // HAVE_STRFTIME
    } else {
      on_top = True;
      screen->raiseWindows((Window *) 0, 0);
    }
  } else if (be->button == 2) {
    on_top = False;
    screen->raiseWindows((Window *) 0, 0);
  } else if (be->button == 3) {
    if (be->window == frame.workspaceLabel) {
      Window window;
      int foo;

      editing = True;
      if (XGetInputFocus(display, &window, &foo) && 
	  window == frame.workspaceLabel)
	return;
      
      XSetInputFocus(display, frame.workspaceLabel, RevertToParent,
		     CurrentTime);
      
      XSetWindowBackgroundPixmap(display, frame.workspaceLabel, frame.reading);
      XClearWindow(display, frame.workspaceLabel);
      XSync(display, False);
    }
  }
}


void Toolbar::buttonReleaseEvent(XButtonEvent *re) {
  if (re->button == 1) {
    if (re->window == frame.menuButton) {
      if (! wait_button) {
	redrawMenuButton(False, True);
	
	if (re->x >= 0 && re->x < (signed) frame.button_w &&
	    re->y >= 0 && re->y < (signed) frame.button_h)
	  screen->getWorkspacemenu()->hide();
      } else
	wait_button = False;
    } else if (re->window == frame.workspacePrev) {
      redrawPrevWorkspaceButton(False, True);
 
      if (re->x >= 0 && re->x < (signed) frame.button_w &&
	  re->y >= 0 && re->y < (signed) frame.button_h)
	if (screen->getCurrentWorkspace()->getWorkspaceID() > 0)
	  screen->changeWorkspaceID(screen->getCurrentWorkspace()->
				    getWorkspaceID() - 1);
	else
	  screen->changeWorkspaceID(screen->getCount() - 1);
    } else if (re->window == frame.workspaceNext) {
      redrawNextWorkspaceButton(False, True);
      
      if (re->x >= 0 && re->x < (signed) frame.button_w &&
	  re->y >= 0 && re->y < (signed) frame.button_h)
	if (screen->getCurrentWorkspace()->getWorkspaceID() <
	    screen->getCount() - 1)
	  screen->changeWorkspaceID(screen->getCurrentWorkspace()->
				    getWorkspaceID() + 1);
	else
	  screen->changeWorkspaceID(0);
    } else if (re->window == frame.windowLabel) {
      screen->raiseFocus();
    } else if (re->window == frame.windowPrev) {
      redrawPrevWindowButton(False, True);
      
      if (re->x >= 0 && re->x < (signed) frame.button_w &&
	  re->y >= 0 && re->y < (signed) frame.button_h)
	screen->prevFocus();
    } else if (re->window == frame.windowNext) {
      redrawNextWindowButton(False, True);
      
      if (re->x >= 0 && re->x < (signed) frame.button_w &&
	  re->y >= 0 && re->y < (signed) frame.button_h)
	screen->nextFocus();
    } else if (re->window == frame.clock) {
      XClearWindow(display, frame.clock);
      
      checkClock(True);
    }
  }
}
 

void Toolbar::exposeEvent(XExposeEvent *ee) {
  if (ee->window == frame.clock) checkClock(True);
  else if (ee->window == frame.workspaceLabel) redrawWorkspaceLabel();
  else if (ee->window == frame.windowLabel) redrawWindowLabel();
  else if (ee->window == frame.workspacePrev) redrawPrevWorkspaceButton();
  else if (ee->window == frame.workspaceNext) redrawNextWorkspaceButton();
  else if (ee->window == frame.windowPrev) redrawPrevWindowButton();
  else if (ee->window == frame.windowNext) redrawNextWindowButton();
  else if (ee->window == frame.menuButton) redrawMenuButton();
}


void Toolbar::keyPressEvent(XKeyEvent *ke) {
  if (ke->window == frame.workspaceLabel && editing) {
    blackbox->grab();

    if (! new_workspace_name) {
      new_workspace_name = new char[1024];
      new_name_pos = new_workspace_name;

      if (! new_workspace_name) return;
    }
    
    KeySym ks;
    char keychar[1];
    XLookupString(ke, keychar, 1, &ks, 0);

    if (ks == XK_Return) {
      *(new_name_pos) = 0;

      editing = False;
      XSetInputFocus(display, PointerRoot, RevertToParent, CurrentTime);
      
      // check to make sure that new_name[0] != 0... otherwise we have a null
      // workspace name which causes serious problems, especially for the
      // Blackbox::LoadRC() method.
      if (*new_workspace_name) {
 	screen->getCurrentWorkspace()->setName(new_workspace_name);
	screen->getCurrentWorkspace()->getMenu()->hide();
	screen->getWorkspacemenu()->
	  remove(screen->getCurrentWorkspace()->getWorkspaceID() + 2);
	screen->getWorkspacemenu()->
	  insert(screen->getCurrentWorkspace()->getName(),
		 screen->getCurrentWorkspace()->getMenu(),
		 screen->getCurrentWorkspace()->getWorkspaceID() + 1);
	screen->getWorkspacemenu()->update();
      }
      
      delete [] new_workspace_name;
      new_workspace_name = new_name_pos = 0;

      reconfigure();
    } else if (! (ks == XK_Shift_L || ks == XK_Shift_R ||
		  ks == XK_Control_L || ks == XK_Control_R ||
		  ks == XK_Caps_Lock || ks == XK_Shift_Lock ||
		  ks == XK_Meta_L || ks == XK_Meta_R ||
		  ks == XK_Alt_L || ks == XK_Alt_R ||
		  ks == XK_Super_L || ks == XK_Super_R ||
		  ks == XK_Hyper_L || ks == XK_Hyper_R)) {
      if (ks == XK_BackSpace) {
	if (new_name_pos != new_workspace_name) {
	  *(--new_name_pos) = 0;
	} else {
	  *new_workspace_name = 0;
	}
      } else {
	*(new_name_pos++) = *keychar;
	*(new_name_pos) = 0;
      }
      
      XClearWindow(display, frame.workspaceLabel);
      int l = strlen(new_workspace_name),
	x = (frame.wlabel_w - XTextWidth(screen->getTitleFont(),
					 new_workspace_name, l)) / 2;
      if (x < (signed) frame.bevel_w) x = frame.bevel_w;
      XDrawString(display, frame.workspaceLabel, buttonGC, x,
		  (frame.wlabel_h +screen->getTitleFont()->ascent -
		   screen->getTitleFont()->descent) / 2,
		  new_workspace_name, l);
    }
    
    blackbox->ungrab();
  }
}
 
