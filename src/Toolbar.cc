// Toolbar.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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

// stupid macros needed to access some functions in version 2 of the GNU C
// library
#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "blackbox.hh"
#include "Clientmenu.hh"
#include "GCCache.hh"
#include "Iconmenu.hh"
#include "Image.hh"
#include "Rootmenu.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"
#ifdef SLIT
#include "Slit.hh"
#endif

#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef    STDC_HEADERS
#  include <string.h>
#endif // STDC_HEADERS

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else // !TIME_WITH_SYS_TIME
# ifdef    HAVE_SYS_TIME_H
#  include <sys/time.h>
# else // !HAVE_SYS_TIME_H
#  include <time.h>
# endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME


Toolbar::Toolbar(BScreen *scrn)
{
  screen = scrn;
  blackbox = Blackbox::instance();

  // get the clock updating every minute
  clock_timer = new BTimer(blackbox, this);
  timeval now;
  gettimeofday(&now, 0);
  clock_timer->setTimeout((60 - (now.tv_sec % 60)) * 1000);
  clock_timer->start();

  hide_handler.toolbar = this;
  hide_timer = new BTimer(blackbox, &hide_handler);
  hide_timer->setTimeout(blackbox->getAutoRaiseDelay());
  hide_timer->fireOnce(True);

  image_ctrl = screen->getImageControl();

  on_top = screen->isToolbarOnTop();
  hidden = do_auto_hide = screen->doToolbarAutoHide();

  editing = False;
  new_workspace_name = (char *) 0;
  new_name_pos = 0;
  frame.grab_x = frame.grab_y = 0;

  toolbarmenu = new Toolbarmenu(this);

  XSetWindowAttributes attrib;
  unsigned long create_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
                              CWColormap | CWOverrideRedirect | CWEventMask;
  attrib.background_pixmap = None;
  attrib.background_pixel = attrib.border_pixel = screen->style()->borderColor().pixel();
  attrib.colormap = screen->colormap();
  attrib.override_redirect = True;
  attrib.event_mask = ButtonPressMask | ButtonReleaseMask |
                      EnterWindowMask | LeaveWindowMask;

  frame.window =
    XCreateWindow(*blackbox, screen->rootWindow(), 0, 0, 1, 1, 0,
		  screen->depth(), InputOutput, screen->visual(),
		  create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.window, this);

  attrib.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask |
                      KeyPressMask | EnterWindowMask;

  frame.workspace_label =
    XCreateWindow(*blackbox, frame.window, 0, 0, 1, 1, 0, screen->depth(),
		  InputOutput, screen->visual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.workspace_label, this);

  frame.window_label =
    XCreateWindow(*blackbox, frame.window, 0, 0, 1, 1, 0, screen->depth(),
		  InputOutput, screen->visual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.window_label, this);

  frame.clock =
    XCreateWindow(*blackbox, frame.window, 0, 0, 1, 1, 0, screen->depth(),
		  InputOutput, screen->visual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.clock, this);

  frame.psbutton =
    XCreateWindow(*blackbox ,frame.window, 0, 0, 1, 1, 0, screen->depth(),
                  InputOutput, screen->visual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.psbutton, this);

  frame.nsbutton =
    XCreateWindow(*blackbox ,frame.window, 0, 0, 1, 1, 0, screen->depth(),
                  InputOutput, screen->visual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.nsbutton, this);

  frame.pwbutton =
    XCreateWindow(*blackbox ,frame.window, 0, 0, 1, 1, 0, screen->depth(),
                  InputOutput, screen->visual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.pwbutton, this);

  frame.nwbutton =
    XCreateWindow(*blackbox ,frame.window, 0, 0, 1, 1, 0, screen->depth(),
                  InputOutput, screen->visual(), create_mask, &attrib);
  blackbox->saveToolbarSearch(frame.nwbutton, this);

  frame.base = frame.label = frame.wlabel = frame.clk = frame.button = frame.pbutton = 0;

  screen->addStrut(&strut);

  reconfigure();

  XMapSubwindows(*blackbox, frame.window);
  XMapWindow(*blackbox, frame.window);
}


Toolbar::~Toolbar(void) {
  XUnmapWindow(*blackbox, frame.window);

  if (frame.base) image_ctrl->removeImage(frame.base);
  if (frame.label) image_ctrl->removeImage(frame.label);
  if (frame.wlabel) image_ctrl->removeImage(frame.wlabel);
  if (frame.clk) image_ctrl->removeImage(frame.clk);
  if (frame.button) image_ctrl->removeImage(frame.button);
  if (frame.pbutton) image_ctrl->removeImage(frame.pbutton);

  blackbox->removeToolbarSearch(frame.window);
  blackbox->removeToolbarSearch(frame.workspace_label);
  blackbox->removeToolbarSearch(frame.window_label);
  blackbox->removeToolbarSearch(frame.clock);
  blackbox->removeToolbarSearch(frame.psbutton);
  blackbox->removeToolbarSearch(frame.nsbutton);
  blackbox->removeToolbarSearch(frame.pwbutton);
  blackbox->removeToolbarSearch(frame.nwbutton);

  XDestroyWindow(*blackbox, frame.workspace_label);
  XDestroyWindow(*blackbox, frame.window_label);
  XDestroyWindow(*blackbox, frame.clock);

  XDestroyWindow(*blackbox, frame.window);

  delete hide_timer;
  delete clock_timer;
  delete toolbarmenu;
}


void Toolbar::reconfigure(void)
{
    BStyle *style = screen->style();
    frame.bevel_w = screen->style()->bevelWidth();
    frame.width = screen->width() * screen->getToolbarWidthPercent() / 100;
    if (i18n->multibyte())
	frame.height = style->toolbarFontSetExtents()->max_ink_extent.height;
    else
	frame.height = style->toolbarFont()->ascent + style->toolbarFont()->descent;
    frame.button_w = frame.height;
    frame.height += 2;
    frame.label_h = frame.height;
    frame.height += (frame.bevel_w * 2);

    // left and right are always 0
    strut.top = strut.bottom = 0;

    switch (screen->getToolbarPlacement()) {
    case TopLeft:
	frame.x = 0;
	frame.y = 0;
	frame.x_hidden = 0;
	frame.y_hidden = screen->style()->bevelWidth() - screen->style()->borderWidth()
			 - frame.height;
	break;

    case BottomLeft:
	frame.x = 0;
	frame.y = screen->height() - frame.height
		  - (screen->style()->borderWidth() * 2);
	frame.x_hidden = 0;
	frame.y_hidden = screen->height() - screen->style()->bevelWidth()
			 - screen->style()->borderWidth();
	break;

    case TopCenter:
	frame.x = (screen->width() - frame.width) / 2;
	frame.y = 0;
	frame.x_hidden = frame.x;
	frame.y_hidden = screen->style()->bevelWidth() - screen->style()->borderWidth()
			 - frame.height;
	break;

    case BottomCenter:
    default:
	frame.x = (screen->width() - frame.width) / 2;
	frame.y = screen->height() - frame.height
		  - (screen->style()->borderWidth() * 2);
	frame.x_hidden = frame.x;
	frame.y_hidden = screen->height() - screen->style()->bevelWidth()
			 - screen->style()->borderWidth();
	break;

    case TopRight:
	frame.x = screen->width() - frame.width
		  - (screen->style()->borderWidth() * 2);
	frame.y = 0;
	frame.x_hidden = frame.x;
	frame.y_hidden = screen->style()->bevelWidth() - screen->style()->borderWidth()
			 - frame.height;
	break;

    case BottomRight:
	frame.x = screen->width() - frame.width
		  - (screen->style()->borderWidth() * 2);
	frame.y = screen->height() - frame.height
		  - (screen->style()->borderWidth() * 2);
	frame.x_hidden = frame.x;
	frame.y_hidden = screen->height() - screen->style()->bevelWidth()
			 - screen->style()->borderWidth();
	break;
    }

    switch(screen->getToolbarPlacement()) {
    case TopLeft:
    case TopCenter:
    case TopRight:
	strut.top = getY() + 1;
	break;
    default:
	strut.bottom = screen->height() - getY() - 1;
    }

    screen->updateAvailableArea();

#ifdef    HAVE_STRFTIME
    time_t ttmp = time(NULL);
    struct tm *tt = 0;

    if (ttmp != -1) {
	tt = localtime(&ttmp);
	if (tt) {
	    char t[1024], *time_string = (char *) 0;
	    int len = strftime(t, 1024, screen->getStrftimeFormat(), tt);

	    time_string = new char[len + 1];

	    memset(time_string, '0', len);
	    *(time_string + len) = '\0';

	    if (i18n->multibyte()) {
		XRectangle ink, logical;
		XmbTextExtents(style->toolbarFontSet(), time_string, len,
			       &ink, &logical);
		frame.clock_w = logical.width;
	    } else {
		frame.clock_w = XTextWidth(style->toolbarFont(), time_string, len);
	    }
	    frame.clock_w += (frame.bevel_w * 4);

	    delete [] time_string;
	} else {
	    frame.clock_w = 0;
	}
    } else {
	frame.clock_w = 0;
    }
#else // !HAVE_STRFTIME
    frame.clock_w =
	XTextWidth(style->toolbarFont(),
		   i18n->getMessage(ToolbarSet, ToolbarNoStrftimeLength,
				    "00:00000"),
		   strlen(i18n->getMessage(ToolbarSet, ToolbarNoStrftimeLength,
					   "00:00000"))) + (frame.bevel_w * 4);
#endif // HAVE_STRFTIME

    int i;
    unsigned int w = 0;
    frame.workspace_label_w = 0;

    for (i = 0; i < screen->getCount(); i++) {
	if (i18n->multibyte()) {
	    XRectangle ink, logical;
	    XmbTextExtents(style->toolbarFontSet(),
			   screen->getWorkspace(i)->getName(),
			   strlen(screen->getWorkspace(i)->getName()),
			   &ink, &logical);
	    w = logical.width;
	} else {
	    w = XTextWidth(style->toolbarFont(),
			   screen->getWorkspace(i)->getName(),
			   strlen(screen->getWorkspace(i)->getName()));
	}
	w += (frame.bevel_w * 4);

	if (w > frame.workspace_label_w) frame.workspace_label_w = w;
    }

    if (frame.workspace_label_w < frame.clock_w)
	frame.workspace_label_w = frame.clock_w;
    else if (frame.workspace_label_w > frame.clock_w)
	frame.clock_w = frame.workspace_label_w;

    frame.window_label_w =
	(frame.width - (frame.clock_w + (frame.button_w * 4) +
			frame.workspace_label_w + (frame.bevel_w * 8) + 6));

    if (hidden) {
	XMoveResizeWindow(*blackbox, frame.window, frame.x_hidden, frame.y_hidden,
			  frame.width, frame.height);
    } else {
	XMoveResizeWindow(*blackbox, frame.window, frame.x, frame.y,
			  frame.width, frame.height);
    }

    XMoveResizeWindow(*blackbox, frame.workspace_label, frame.bevel_w,
		      frame.bevel_w, frame.workspace_label_w,
		      frame.label_h);
    XMoveResizeWindow(*blackbox, frame.psbutton, (frame.bevel_w * 2) +
		      frame.workspace_label_w + 1, frame.bevel_w + 1,
		      frame.button_w, frame.button_w);
    XMoveResizeWindow(*blackbox ,frame.nsbutton, (frame.bevel_w * 3) +
		      frame.workspace_label_w + frame.button_w + 2,
		      frame.bevel_w + 1, frame.button_w, frame.button_w);
    XMoveResizeWindow(*blackbox, frame.window_label, (frame.bevel_w * 4) +
		      (frame.button_w * 2) + frame.workspace_label_w + 3,
		      frame.bevel_w, frame.window_label_w, frame.label_h);
    XMoveResizeWindow(*blackbox, frame.pwbutton, (frame.bevel_w * 5) +
		      (frame.button_w * 2) + frame.workspace_label_w +
		      frame.window_label_w + 4, frame.bevel_w + 1,
		      frame.button_w, frame.button_w);
    XMoveResizeWindow(*blackbox, frame.nwbutton, (frame.bevel_w * 6) +
		      (frame.button_w * 3) + frame.workspace_label_w +
		      frame.window_label_w + 5, frame.bevel_w + 1,
		      frame.button_w, frame.button_w);
    XMoveResizeWindow(*blackbox, frame.clock, frame.width - frame.clock_w -
		      frame.bevel_w, frame.bevel_w, frame.clock_w,
		      frame.label_h);

    BTexture texture = style->toolbar();
    frame.base = texture.render( Size( frame.width, frame.height ), frame.base );
    if ( ! frame.base )
	XSetWindowBackground(*blackbox, frame.window, texture.color().pixel());
    else
	XSetWindowBackgroundPixmap( *blackbox, frame.window, frame.base );

    texture = style->toolbarWindowLabel();
    frame.label = texture.render( Size( frame.window_label_w, frame.label_h ),
				  frame.label );
    if ( ! frame.label )
	XSetWindowBackground(*blackbox, frame.window_label, texture.color().pixel());
    else
	XSetWindowBackgroundPixmap( *blackbox, frame.window_label, frame.label );

    texture = style->toolbarWorkspaceLabel();
    frame.wlabel = texture.render( Size( frame.workspace_label_w, frame.label_h ),
				   frame.wlabel );
    if ( ! frame.wlabel )
	XSetWindowBackground(*blackbox, frame.workspace_label, texture.color().pixel());
    else
	XSetWindowBackgroundPixmap( *blackbox, frame.workspace_label, frame.wlabel );

    texture = style->toolbarClockLabel();
    frame.clk = texture.render( Size( frame.clock_w, frame.label_h ), frame.clk );
    if ( ! frame.clk )
	XSetWindowBackground(*blackbox, frame.clock, texture.color().pixel());
    else
	XSetWindowBackgroundPixmap( *blackbox, frame.clock, frame.clk );

    texture = style->toolbarButton();
    frame.button = texture.render( Size( frame.button_w, frame.button_w ),
				   frame.button );
    if ( ! frame.button ) {
	frame.button_pixel = texture.color().pixel();
	XSetWindowBackground(*blackbox, frame.psbutton, frame.button_pixel);
	XSetWindowBackground(*blackbox, frame.nsbutton, frame.button_pixel);
	XSetWindowBackground(*blackbox, frame.pwbutton, frame.button_pixel);
	XSetWindowBackground(*blackbox, frame.nwbutton, frame.button_pixel);
    } else {
	XSetWindowBackgroundPixmap( *blackbox, frame.psbutton, frame.button );
	XSetWindowBackgroundPixmap( *blackbox, frame.nsbutton, frame.button );
	XSetWindowBackgroundPixmap( *blackbox, frame.pwbutton, frame.button );
	XSetWindowBackgroundPixmap( *blackbox, frame.nwbutton, frame.button );
    }

    texture = style->toolbarButtonPressed();
    frame.pbutton = texture.render( Size( frame.button_w, frame.button_w ),
				    frame.pbutton );
    if ( ! frame.pbutton )
	frame.pbutton_pixel = texture.color().pixel();

    XSetWindowBorder(*blackbox, frame.window, screen->style()->borderColor().pixel());
    XSetWindowBorderWidth(*blackbox, frame.window, screen->style()->borderWidth());

    XClearWindow(*blackbox, frame.window);
    XClearWindow(*blackbox, frame.workspace_label);
    XClearWindow(*blackbox, frame.window_label);
    XClearWindow(*blackbox, frame.clock);
    XClearWindow(*blackbox, frame.psbutton);
    XClearWindow(*blackbox, frame.nsbutton);
    XClearWindow(*blackbox, frame.pwbutton);
    XClearWindow(*blackbox, frame.nwbutton);

    redrawWindowLabel();
    redrawWorkspaceLabel();
    redrawPrevWorkspaceButton();
    redrawNextWorkspaceButton();
    redrawPrevWindowButton();
    redrawNextWindowButton();
    checkClock(True);

    toolbarmenu->reconfigure();
}


#ifdef    HAVE_STRFTIME
void Toolbar::checkClock(Bool redraw) {
#else // !HAVE_STRFTIME
void Toolbar::checkClock(Bool redraw, Bool date) {
#endif // HAVE_STRFTIME
  time_t tmp = 0;
  struct tm *tt = 0;

  if ((tmp = time(NULL)) != -1) {
    if (! (tt = localtime(&tmp))) return;
    if (tt->tm_min != frame.minute || tt->tm_hour != frame.hour) {
      frame.hour = tt->tm_hour;
      frame.minute = tt->tm_min;
      XClearWindow(*blackbox, frame.clock);
      redraw = True;
    }
  }

  if (redraw) {
#ifdef    HAVE_STRFTIME
    char t[1024];
    if (! strftime(t, 1024, screen->getStrftimeFormat(), tt))
      return;
#else // !HAVE_STRFTIME
    char t[9];
    if (date) {
      // format the date... with special consideration for y2k ;)
      if (screen->getDateFormat() == Blackbox::B_EuropeanDate)
        sprintf(t, 18n->getMessage(ToolbarSet, ToolbarNoStrftimeDateFormatEu,
				   "%02d.%02d.%02d"),
		tt->tm_mday, tt->tm_mon + 1,
                (tt->tm_year >= 100) ? tt->tm_year - 100 : tt->tm_year);
      else
        sprintf(t, i18n->getMessage(ToolbarSet, ToolbarNoStrftimeDateFormat,
				    "%02d/%02d/%02d"),
		tt->tm_mon + 1, tt->tm_mday,
                (tt->tm_year >= 100) ? tt->tm_year - 100 : tt->tm_year);
    } else {
      if (screen->isClock24Hour())
	sprintf(t, i18n->getMessage(ToolbarSet, ToolbarNoStrftimeTimeFormat24,
				    "  %02d:%02d "),
		frame.hour, frame.minute);
      else
	sprintf(t, i18n->getMessage(ToolbarSet, ToolbarNoStrftimeTimeFormat12,
				    "%02d:%02d %sm"),
		((frame.hour > 12) ? frame.hour - 12 :
		 ((frame.hour == 0) ? 12 : frame.hour)), frame.minute,
		((frame.hour >= 12) ?
		 i18n->getMessage(ToolbarSet,
				  ToolbarNoStrftimeTimeFormatP, "p") :
		 i18n->getMessage(ToolbarSet,
				  ToolbarNoStrftimeTimeFormatA, "a")));
    }
#endif // HAVE_STRFTIME

    int dx = (frame.bevel_w * 2), dlen = strlen(t);
    unsigned int l;

    BStyle *style = screen->style();
    if (i18n->multibyte()) {
      XRectangle ink, logical;
      XmbTextExtents(style->toolbarFontSet(), t, dlen, &ink, &logical);
      l = logical.width;
    } else
      l = XTextWidth(style->toolbarFont(), t, dlen);

    l += (frame.bevel_w * 4);

    if (l > frame.clock_w) {
      for (; dlen >= 0; dlen--) {
	if (i18n->multibyte()) {
	  XRectangle ink, logical;
	  XmbTextExtents(style->toolbarFontSet(), t, dlen, &ink, &logical);
	  l = logical.width;
	} else
	  l = XTextWidth(style->toolbarFont(), t, dlen);
	l+= (frame.bevel_w * 4);

        if (l < frame.clock_w)
          break;
      }
    }
    switch (style->toolbarJustify()) {
    case BStyle::Left:
	break;

    case BStyle::Right:
      dx += frame.clock_w - l;
      break;

    case BStyle::Center:
      dx += (frame.clock_w - l) / 2;
      break;
    }

    BGCCache::Item &gc = BGCCache::instance()->find( style->toolbarClockTextColor(),
						     style->toolbarFont() );
    if (i18n->multibyte())
      XmbDrawString(*blackbox, frame.clock, style->toolbarFontSet(), gc.gc(),
		    dx, (1 - style->toolbarFontSetExtents()->max_ink_extent.y), t, dlen);
    else
      XDrawString(*blackbox, frame.clock, gc.gc(), dx,
		  (style->toolbarFont()->ascent + 1), t, dlen);
    BGCCache::instance()->release( gc );
  }
}


void Toolbar::redrawWindowLabel(Bool redraw)
{
    if (Blackbox::instance()->getFocusedWindow()) {
	if (redraw)
	    XClearWindow(*blackbox, frame.window_label);

	BlackboxWindow *foc = Blackbox::instance()->getFocusedWindow();
	if (foc->getScreen() != screen) return;

	int dx = (frame.bevel_w * 2), dlen = strlen(*foc->getTitle());
	unsigned int l;

	BStyle *style = screen->style();
	if (i18n->multibyte()) {
	    XRectangle ink, logical;
	    XmbTextExtents(style->toolbarFontSet(), *foc->getTitle(),
			   dlen, &ink, &logical);
	    l = logical.width;
	} else {
	    l = XTextWidth(style->toolbarFont(), *foc->getTitle(), dlen);
	}
	l += (frame.bevel_w * 4);

	if (l > frame.window_label_w) {
	    for (; dlen >= 0; dlen--) {
		if (i18n->multibyte()) {
		    XRectangle ink, logical;
		    XmbTextExtents(style->toolbarFontSet(), *foc->getTitle(), dlen,
				   &ink, &logical);
		    l = logical.width;
		} else {
		    l = XTextWidth(style->toolbarFont(), *foc->getTitle(), dlen);
		}
		l += (frame.bevel_w * 4);

		if (l < frame.window_label_w)
		    break;
	    }
	}
	switch (style->toolbarJustify()) {
	case BStyle::Left:
	    break;

	case BStyle::Right:
	    dx += frame.window_label_w - l;
	    break;

	case BStyle::Center:
	    dx += (frame.window_label_w - l) / 2;
	    break;
	}

	BGCCache::Item &gc =
	    BGCCache::instance()->find( style->toolbarWindowLabelTextColor(),
					style->toolbarFont() );
	if (i18n->multibyte())
	    XmbDrawString(*blackbox, frame.window_label, style->toolbarFontSet(),
			  gc.gc(),
			  dx, (1 - style->toolbarFontSetExtents()->max_ink_extent.y),
			  *foc->getTitle(), dlen);
	else
	    XDrawString(*blackbox, frame.window_label, gc.gc(),
			dx, (style->toolbarFont()->ascent + 1), *foc->getTitle(), dlen);
	BGCCache::instance()->release( gc );
    } else {
	XClearWindow(*blackbox, frame.window_label);
    }
}


void Toolbar::redrawWorkspaceLabel(Bool redraw) {
  if (screen->getCurrentWorkspace()->getName()) {
    if (redraw)
      XClearWindow(*blackbox, frame.workspace_label);

    int dx = (frame.bevel_w * 2), dlen =
	     strlen(screen->getCurrentWorkspace()->getName());
    unsigned int l;

    BStyle *style = screen->style();
    if (i18n->multibyte()) {
      XRectangle ink, logical;
      XmbTextExtents(style->toolbarFontSet(),
		     screen->getCurrentWorkspace()->getName(), dlen,
		     &ink, &logical);
      l = logical.width;
    } else {
      l = XTextWidth(style->toolbarFont(),
		     screen->getCurrentWorkspace()->getName(), dlen);
    }
    l += (frame.bevel_w * 4);

    if (l > frame.workspace_label_w) {
      for (; dlen >= 0; dlen--) {
	if (i18n->multibyte()) {
	  XRectangle ink, logical;
	  XmbTextExtents(style->toolbarFontSet(),
			 screen->getCurrentWorkspace()->getName(), dlen,
			 &ink, &logical);
	  l = logical.width;
	} else {
	  l = XTextWidth(style->toolbarFont(),
			 screen->getCurrentWorkspace()->getName(), dlen);
	}
	l += (frame.bevel_w * 4);

        if (l < frame.workspace_label_w)
          break;
      }
    }
    switch (style->toolbarJustify()) {
    case BStyle::Left:
	break;

    case BStyle::Right:
      dx += frame.workspace_label_w - l;
      break;

    case BStyle::Center:
      dx += (frame.workspace_label_w - l) / 2;
      break;
    }

    BGCCache::Item &gc =
	BGCCache::instance()->find( style->toolbarWorkspaceLabelTextColor(),
				    style->toolbarFont() );
    if (i18n->multibyte())
      XmbDrawString(*blackbox, frame.workspace_label, style->toolbarFontSet(), gc.gc(),
		    dx, (1 - style->toolbarFontSetExtents()->max_ink_extent.y),
		    (char *) screen->getCurrentWorkspace()->getName(), dlen);
    else
      XDrawString(*blackbox, frame.workspace_label, gc.gc(),
		  dx, (style->toolbarFont()->ascent + 1),
		  (char *) screen->getCurrentWorkspace()->getName(), dlen);
    BGCCache::instance()->release( gc );
  }
}


void Toolbar::redrawPrevWorkspaceButton(Bool pressed, Bool redraw) {
  if (redraw) {
    if (pressed) {
      if (frame.pbutton)
	XSetWindowBackgroundPixmap(*blackbox, frame.psbutton, frame.pbutton);
      else
	XSetWindowBackground(*blackbox, frame.psbutton, frame.pbutton_pixel);
    } else {
      if (frame.button)
        XSetWindowBackgroundPixmap(*blackbox, frame.psbutton, frame.button);
      else
	XSetWindowBackground(*blackbox, frame.psbutton, frame.button_pixel);
    }
    XClearWindow(*blackbox, frame.psbutton);
  }

  int hh = frame.button_w / 2, hw = frame.button_w / 2;

  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh;
  pts[1].x = 4; pts[1].y = 2;
  pts[2].x = 0; pts[2].y = -4;

  BGCCache::Item &gc =
      BGCCache::instance()->find( screen->style()->toolbarButtonPicColor() );
  XFillPolygon(*blackbox, frame.psbutton, gc.gc(), pts, 3, Convex, CoordModePrevious);
  BGCCache::instance()->release( gc );
}


void Toolbar::redrawNextWorkspaceButton(Bool pressed, Bool redraw) {
  if (redraw) {
    if (pressed) {
      if (frame.pbutton)
	XSetWindowBackgroundPixmap(*blackbox, frame.nsbutton, frame.pbutton);
      else
	XSetWindowBackground(*blackbox, frame.nsbutton, frame.pbutton_pixel);
    } else {
      if (frame.button)
        XSetWindowBackgroundPixmap(*blackbox, frame.nsbutton, frame.button);
      else
	XSetWindowBackground(*blackbox, frame.nsbutton, frame.button_pixel);
    }
    XClearWindow(*blackbox, frame.nsbutton);
  }

  int hh = frame.button_w / 2, hw = frame.button_w / 2;

  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh - 2;
  pts[1].x = 4; pts[1].y =  2;
  pts[2].x = -4; pts[2].y = 2;

  BGCCache::Item &gc =
      BGCCache::instance()->find( screen->style()->toolbarButtonPicColor() );
  XFillPolygon(*blackbox, frame.nsbutton, gc.gc(), pts, 3, Convex, CoordModePrevious);
  BGCCache::instance()->release( gc );
}


void Toolbar::redrawPrevWindowButton(Bool pressed, Bool redraw) {
  if (redraw) {
    if (pressed) {
      if (frame.pbutton)
	XSetWindowBackgroundPixmap(*blackbox, frame.pwbutton, frame.pbutton);
      else
	XSetWindowBackground(*blackbox, frame.pwbutton, frame.pbutton_pixel);
    } else {
      if (frame.button)
        XSetWindowBackgroundPixmap(*blackbox, frame.pwbutton, frame.button);
      else
	XSetWindowBackground(*blackbox, frame.pwbutton, frame.button_pixel);
    }
    XClearWindow(*blackbox, frame.pwbutton);
  }

  int hh = frame.button_w / 2, hw = frame.button_w / 2;

  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh;
  pts[1].x = 4; pts[1].y = 2;
  pts[2].x = 0; pts[2].y = -4;

  BGCCache::Item &gc =
      BGCCache::instance()->find( screen->style()->toolbarButtonPicColor() );
  XFillPolygon(*blackbox, frame.pwbutton, gc.gc(), pts, 3, Convex, CoordModePrevious);
  BGCCache::instance()->release( gc );
}


void Toolbar::redrawNextWindowButton(Bool pressed, Bool redraw) {
  if (redraw) {
    if (pressed) {
      if (frame.pbutton)
	XSetWindowBackgroundPixmap(*blackbox, frame.nwbutton, frame.pbutton);
      else
	XSetWindowBackground(*blackbox, frame.nwbutton, frame.pbutton_pixel);
    } else {
      if (frame.button)
        XSetWindowBackgroundPixmap(*blackbox, frame.nwbutton, frame.button);
      else
	XSetWindowBackground(*blackbox, frame.nwbutton, frame.button_pixel);
    }
    XClearWindow(*blackbox, frame.nwbutton);
  }

  int hh = frame.button_w / 2, hw = frame.button_w / 2;

  XPoint pts[3];
  pts[0].x = hw - 2; pts[0].y = hh - 2;
  pts[1].x = 4; pts[1].y =  2;
  pts[2].x = -4; pts[2].y = 2;

  BGCCache::Item &gc =
      BGCCache::instance()->find( screen->style()->toolbarButtonPicColor() );
  XFillPolygon(*blackbox, frame.nwbutton, gc.gc(), pts, 3, Convex, CoordModePrevious);
  BGCCache::instance()->release( gc );
}


void Toolbar::edit(void)
{
    Window window;
    int foo;

  editing = True;
  if (XGetInputFocus(*blackbox, &window, &foo) &&
      window == frame.workspace_label)
    return;

  XSetInputFocus(*blackbox, frame.workspace_label,
                 ((screen->isSloppyFocus()) ? RevertToPointerRoot :
                  RevertToParent),
                 CurrentTime);
  XClearWindow(*blackbox, frame.workspace_label);

  blackbox->setNoFocus(True);
  if (blackbox->getFocusedWindow())
    blackbox->getFocusedWindow()->setFocusFlag(False);

  BStyle *style = screen->style();
  BGCCache::Item &gc =
      BGCCache::instance()->find( style->toolbarWorkspaceLabelTextColor(),
				  style->toolbarFont() );
  XDrawRectangle(*blackbox, frame.workspace_label, gc.gc(),
                 frame.workspace_label_w / 2, 0, 1, frame.label_h - 1);
  BGCCache::instance()->release( gc );
}


void Toolbar::buttonPressEvent(XButtonEvent *be) {
  if (be->button == 1) {
    if (be->window == frame.psbutton)
      redrawPrevWorkspaceButton(True, True);
    else if (be->window == frame.nsbutton)
      redrawNextWorkspaceButton(True, True);
    else if (be->window == frame.pwbutton)
      redrawPrevWindowButton(True, True);
    else if (be->window == frame.nwbutton)
      redrawNextWindowButton(True, True);
#ifndef   HAVE_STRFTIME
    else if (be->window == frame.clock) {
      XClearWindow(*blackbox, frame.clock);
      checkClock(True, True);
    }
#endif // HAVE_STRFTIME
    else if (! on_top) {
      Window w[1] = { frame.window };
      screen->raiseWindows(w, 1);
    }
  } else if (be->button == 2 && (! on_top)) {
    XLowerWindow(*blackbox, frame.window);
  }
}



void Toolbar::buttonReleaseEvent(XButtonEvent *re) {
  if (re->button == 1) {
    if (re->window == frame.psbutton) {
      redrawPrevWorkspaceButton(False, True);

      if (re->x >= 0 && re->x < (signed) frame.button_w &&
          re->y >= 0 && re->y < (signed) frame.button_w)
        if (screen->getCurrentWorkspace()->getWorkspaceID() > 0)
          screen->changeWorkspaceID(screen->getCurrentWorkspace()->
                                    getWorkspaceID() - 1);
        else
          screen->changeWorkspaceID(screen->getCount() - 1);
    } else if (re->window == frame.nsbutton) {
      redrawNextWorkspaceButton(False, True);

      if (re->x >= 0 && re->x < (signed) frame.button_w &&
          re->y >= 0 && re->y < (signed) frame.button_w)
        if (screen->getCurrentWorkspace()->getWorkspaceID() <
            screen->getCount() - 1)
          screen->changeWorkspaceID(screen->getCurrentWorkspace()->
                                    getWorkspaceID() + 1);
        else
          screen->changeWorkspaceID(0);
    } else if (re->window == frame.pwbutton) {
      redrawPrevWindowButton(False, True);

      if (re->x >= 0 && re->x < (signed) frame.button_w &&
          re->y >= 0 && re->y < (signed) frame.button_w)
        screen->prevFocus();
    } else if (re->window == frame.nwbutton) {
      redrawNextWindowButton(False, True);

      if (re->x >= 0 && re->x < (signed) frame.button_w &&
          re->y >= 0 && re->y < (signed) frame.button_w)
        screen->nextFocus();
    } else if (re->window == frame.window_label)
      screen->raiseFocus();
#ifndef   HAVE_STRFTIME
    else if (re->window == frame.clock) {
      XClearWindow(*blackbox, frame.clock);
      checkClock(True);
    }
#endif // HAVE_STRFTIME
  } else if (re->button == 3) {
    toolbarmenu->popup( re->x_root, re->y_root );
  }
}


void Toolbar::enterNotifyEvent(XCrossingEvent *) {
  if (! do_auto_hide)
    return;

  if (hidden) {
    if (! hide_timer->isTiming()) hide_timer->start();
  } else {
    if (hide_timer->isTiming()) hide_timer->stop();
  }
}

void Toolbar::leaveNotifyEvent(XCrossingEvent *) {
  if (! do_auto_hide)
    return;

  if (hidden) {
    if (hide_timer->isTiming()) hide_timer->stop();
  } else if (! toolbarmenu->isVisible()) {
    if (! hide_timer->isTiming()) hide_timer->start();
  }
}


void Toolbar::exposeEvent(XExposeEvent *ee) {
  if (ee->window == frame.clock) checkClock(True);
  else if (ee->window == frame.workspace_label && (! editing))
    redrawWorkspaceLabel();
  else if (ee->window == frame.window_label) redrawWindowLabel();
  else if (ee->window == frame.psbutton) redrawPrevWorkspaceButton();
  else if (ee->window == frame.nsbutton) redrawNextWorkspaceButton();
  else if (ee->window == frame.pwbutton) redrawPrevWindowButton();
  else if (ee->window == frame.nwbutton) redrawNextWindowButton();
}


void Toolbar::keyPressEvent(XKeyEvent *ke) {
  if (ke->window == frame.workspace_label && editing) {
    if (! new_workspace_name) {
      new_workspace_name = new char[128];
      new_name_pos = 0;

      if (! new_workspace_name) return;
    }

    KeySym ks;
    char keychar[1];
    XLookupString(ke, keychar, 1, &ks, 0);

    // either we are told to end with a return or we hit the end of the buffer
    if (ks == XK_Return || new_name_pos == 127) {
      *(new_workspace_name + new_name_pos + 1) = 0;

      editing = False;

      blackbox->setNoFocus(False);
      if (blackbox->getFocusedWindow()) {
        blackbox->getFocusedWindow()->setInputFocus();
        blackbox->getFocusedWindow()->setFocusFlag(True);
      } else {
        XSetInputFocus(*blackbox, PointerRoot, None, CurrentTime);
      }
      // check to make sure that new_name[0] != 0... otherwise we have a null
      // workspace name which causes serious problems, especially for the
      // Blackbox::LoadRC() method.
      if (*new_workspace_name) {
 	screen->getCurrentWorkspace()->setName(new_workspace_name);
	screen->getCurrentWorkspace()->getMenu()->hide();
	screen->getWorkspacemenu()->
          change(screen->getCurrentWorkspace()->getWorkspaceID() + 3,
                 screen->getCurrentWorkspace()->getName());
      }

      delete [] new_workspace_name;
      new_workspace_name = (char *) 0;
      new_name_pos = 0;

      reconfigure();
    } else if (! (ks == XK_Shift_L || ks == XK_Shift_R ||
		  ks == XK_Control_L || ks == XK_Control_R ||
		  ks == XK_Caps_Lock || ks == XK_Shift_Lock ||
		  ks == XK_Meta_L || ks == XK_Meta_R ||
		  ks == XK_Alt_L || ks == XK_Alt_R ||
		  ks == XK_Super_L || ks == XK_Super_R ||
		  ks == XK_Hyper_L || ks == XK_Hyper_R)) {
      if (ks == XK_BackSpace) {
	if (new_name_pos > 0) {
	  --new_name_pos;
	  *(new_workspace_name + new_name_pos) = '\0';
	} else {
	  *new_workspace_name = '\0';
	}
      } else {
	*(new_workspace_name + new_name_pos) = *keychar;
	++new_name_pos;
	*(new_workspace_name + new_name_pos) = '\0';
      }

      XClearWindow(*blackbox, frame.workspace_label);
      int l = strlen(new_workspace_name), tw, x;

      BStyle *style = screen->style();
      if (i18n->multibyte()) {
	XRectangle ink, logical;
	XmbTextExtents(style->toolbarFontSet(),
		       new_workspace_name, l, &ink, &logical);
	tw = logical.width;
      } else {
	tw = XTextWidth(style->toolbarFont(),
			new_workspace_name, l);
      }
      x = (frame.workspace_label_w - tw) / 2;

      if (x < (signed) frame.bevel_w) x = frame.bevel_w;

      BGCCache::Item &gc =
	  BGCCache::instance()->find( style->toolbarWorkspaceLabelTextColor(),
				      style->toolbarFont() );
      if (i18n->multibyte())
	XmbDrawString(*blackbox, frame.workspace_label, style->toolbarFontSet(), gc.gc(),
		      x, (1 - style->toolbarFontSetExtents()->max_ink_extent.y),
		      new_workspace_name, l);
      else
	XDrawString(*blackbox, frame.workspace_label, gc.gc(),
		    x, (style->toolbarFont()->ascent + 1), new_workspace_name, l);

      XDrawRectangle(*blackbox, frame.workspace_label, gc.gc(),
		     x + tw, 0, 1, frame.label_h - 1);
    }
  }
}

void Toolbar::timeout(void)
{
  checkClock(True);

  timeval now;
  gettimeofday(&now, 0);
  clock_timer->setTimeout((60 - (now.tv_sec % 60)) * 1000);
}

void Toolbar::HideHandler::timeout(void)
{
  toolbar->hidden = ! toolbar->hidden;
  if (toolbar->hidden)
    XMoveWindow(*toolbar->blackbox, toolbar->frame.window,
		toolbar->frame.x_hidden, toolbar->frame.y_hidden);
  else
    XMoveWindow(*toolbar->blackbox, toolbar->frame.window,
		toolbar->frame.x, toolbar->frame.y);
}

Toolbarmenu::Toolbarmenu(Toolbar *tb)
  : Basemenu( tb->screen->screen() )
{
  toolbar = tb;

  placementmenu = new Placementmenu(this);

  insert(i18n->getMessage(CommonSet, CommonPlacementTitle, "Placement"),
	 placementmenu);
  insert(i18n->getMessage(CommonSet, CommonAlwaysOnTop, "Always on top"), 1);
  insert(i18n->getMessage(CommonSet, CommonAutoHide, "Auto hide"), 2);
  insert(i18n->getMessage(ToolbarSet, ToolbarEditWkspcName,
			  "Edit current workspace name"), 3);

  if (toolbar->isOnTop()) setItemChecked(1, True);
  if (toolbar->doAutoHide()) setItemChecked(2, True);
}

Toolbarmenu::~Toolbarmenu(void)
{
  delete placementmenu;
}

void Toolbarmenu::itemClicked( const Point &, const Item &item, int button )
{
  if (button != 1)
    return;


  switch (item.function()) {
  case 1:  // always on top
    toolbar->on_top = ((toolbar->isOnTop()) ? False : True);
    setItemChecked(1, toolbar->on_top);
    if (toolbar->isOnTop())
      toolbar->screen->raiseWindows((Window *) 0, 0);
    break;

  case 2: // auto hide
    toolbar->do_auto_hide = ((toolbar->doAutoHide()) ?  False : True);
    setItemChecked(2, toolbar->do_auto_hide);
#ifdef    SLIT
    toolbar->screen->getSlit()->reposition();
#endif // SLIT
    break;

  case 3: // edit current workspace name
    toolbar->edit();
    break;
  } // switch
}

void Toolbarmenu::hide(void)
{
  Basemenu::hide();
  if (toolbar->doAutoHide() && ! toolbar->isEditing())
    toolbar->hide_handler.timeout();
}

void Toolbarmenu::reconfigure(void)
{
  placementmenu->reconfigure();
  Basemenu::reconfigure();
}

Toolbarmenu::Placementmenu::Placementmenu(Toolbarmenu *tm)
    : Basemenu( tm->toolbar->screen->screen() )
{
    toolbarmenu = tm;

    // setMinimumSublevels(3);

    insert(i18n->getMessage(CommonSet, CommonPlacementTopLeft,
			    "Top Left"), Toolbar::TopLeft);
    insert(i18n->getMessage(CommonSet, CommonPlacementBottomLeft,
			    "Bottom Left"), Toolbar::BottomLeft);
    insert(i18n->getMessage(CommonSet, CommonPlacementTopCenter,
			    "Top Center"), Toolbar::TopCenter);
    insert(i18n->getMessage(CommonSet, CommonPlacementBottomCenter,
			    "Bottom Center"), Toolbar::BottomCenter);
    insert(i18n->getMessage(CommonSet, CommonPlacementTopRight,
			    "Top Right"), Toolbar::TopRight);
    insert(i18n->getMessage(CommonSet, CommonPlacementBottomRight,
			    "Bottom Right"), Toolbar::BottomRight);
}

void Toolbarmenu::Placementmenu::itemClicked(const Point &, const Item &item, int button )
{
  if (button != 1)
    return;

  toolbarmenu->toolbar->screen->saveToolbarPlacement(item.function());
  toolbarmenu->toolbar->reconfigure();

#ifdef    SLIT
  // reposition the slit as well to make sure it doesn't intersect the
  // toolbar
  toolbarmenu->toolbar->screen->getSlit()->reposition();
#endif // SLIT
}
