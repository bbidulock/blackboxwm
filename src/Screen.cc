// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Screen.cc for Blackbox - an X11 Window manager
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

#include <X11/Xatom.h>
#include <X11/keysym.h>

#include "Screen.hh"

#include "i18n.hh"
#include "blackbox.hh"
#include "Clientmenu.hh"
#include "Configmenu.hh"
#include "GCCache.hh"
#include "Iconmenu.hh"
#include "Image.hh"
#include "Netizen.hh"

#ifdef    SLIT
#include "Slit.hh"
#endif // SLIT

#include "Rootmenu.hh"
#include "Toolbar.hh"
#include "Util.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#  include <sys/types.h>
#endif // STDC_HEADERS

#ifdef    HAVE_CTYPE_H
#  include <ctype.h>
#endif // HAVE_CTYPE_H

#ifdef    HAVE_DIRENT_H
#  include <dirent.h>
#endif // HAVE_DIRENT_H

#ifdef    HAVE_LOCALE_H
#  include <locale.h>
#endif // HAVE_LOCALE_H

#ifdef    HAVE_UNISTD_H
#  include <sys/types.h>
#  include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef    HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#ifndef    HAVE_SNPRINTF
#  include "bsd-snprintf.h"
#endif // !HAVE_SNPRINTF

#include <algorithm>
#include <iostream>


static Bool running = True;

static int anotherWMRunning(Display *display, XErrorEvent *) {
  fprintf(stderr, i18n(ScreenSet, ScreenAnotherWMRunning,
                       "BScreen::BScreen: an error occured while querying the X server.\n"
                       "  another window manager already running on display %s.\n"),
          DisplayString(display));

  running = False;

  return(-1);
}

struct dcmp {
  bool operator()(const char *one, const char *two) const {
    return (strcmp(one, two) < 0) ? True : False;
  }
};


BScreen::BScreen(Blackbox *bb, int scrn)
  : _style(scrn)
{
  screeninfo = BaseDisplay::instance()->screenInfo(scrn);

  blackbox = bb;

  event_mask = ColormapChangeMask | PropertyChangeMask |
               EnterWindowMask | LeaveWindowMask |
               SubstructureRedirectMask | KeyPressMask | KeyReleaseMask |
               ButtonPressMask | ButtonReleaseMask;

  XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);
  XSelectInput(*blackbox, screenInfo()->rootWindow(), event_mask);
  XSync(*blackbox, False);
  XSetErrorHandler((XErrorHandler) old);

  managed = running;
  if (! managed)
    return;
}

BScreen::~BScreen(void)
{
  if (! managed) return;

  /*
    if (geom_pixmap != None)
    image_control->removeImage(geom_pixmap);

    if (geom_window != None)
    XDestroyWindow(*blackbox, geom_window);
  */

  removeWorkspaceNames();

  while (workspacesList->count())
    delete workspacesList->remove(0);

  while (iconList->count())
    delete iconList->remove(0);

  while (netizenList->count())
    delete netizenList->remove(0);

#ifdef    HAVE_STRFTIME
  if (resource.strftime_format)
    delete [] resource.strftime_format;
#endif // HAVE_STRFTIME

  delete rootmenu;
  delete workspacemenu;
  delete iconmenu;
  delete configmenu;

#ifdef    SLIT
  delete slit;
#endif // SLIT

  delete toolbar;
  delete _toolbar2;
  delete image_control;

  delete workspacesList;
  delete workspaceNames;
  delete iconList;
  delete netizenList;
  delete strutList;
}

void BScreen::initialize()
{
#ifdef    HAVE_GETPID
  pid_t bpid = getpid();
  XChangeProperty(*blackbox, screenInfo()->rootWindow(),
                  blackbox->getBlackboxPidAtom(), XA_CARDINAL,
                  sizeof(pid_t) * 8, PropModeReplace,
                  (unsigned char *) &bpid, 1);
#endif // HAVE_GETPID

  XDefineCursor(*blackbox, screenInfo()->rootWindow(),
                blackbox->getSessionCursor());

  rootmenu = 0;

#ifdef    HAVE_STRFTIME
  resource.strftime_format = 0;
#endif // HAVE_STRFTIME

  workspaceNames = new LinkedList<char>;
  workspacesList = new LinkedList<Workspace>;
  netizenList = new LinkedList<Netizen>;
  iconList = new LinkedList<BlackboxWindow>;
  strutList = new LinkedList<NETStrut>;

  // start off full screen, top left.
  usableArea.setRect(0, 0, screeninfo->width(), screeninfo->height());

  image_control =
    new BImageControl(blackbox, screenInfo(), True, blackbox->getColorsPerChannel(),
                      blackbox->getCacheLife(), blackbox->getCacheMax());
  image_control->installRootColormap();
  root_colormap_installed = True;

  blackbox->load_rc(this);

  image_control->setDither(resource.image_dither);

  style()->setCurrentStyle(blackbox->getStyleFilename());

  /*
    ### TODO - make the geom window a separate widget

    const char *s =  i18n(ScreenSet, ScreenPositionLength,
    "0: 0000 x 0: 0000");
    int l = strlen(s);

    if (i18n->multibyte()) {
    XRectangle ink, logical;
    XmbTextExtents(resource.wstyle.fontset, s, l, &ink, &logical);
    geom_w = logical.width;

    geom_h = resource.wstyle.fontset_extents->max_ink_extent.height;
    } else {
    geom_h = resource.wstyle.font->ascent +
    resource.wstyle.font->descent;

    geom_w = XTextWidth(resource.wstyle.font, s, l);
    }

    geom_w += (resource.bevel_width * 2);
    geom_h += (resource.bevel_width * 2);

    XSetWindowAttributes attrib;
    unsigned long mask = CWBorderPixel | CWColormap | CWSaveUnder;
    attrib.border_pixel = getBorderColor()->pixel();
    attrib.colormap = colormap();
    attrib.save_under = True;

    geom_window =
    XCreateWindow(*blackbox, screenInfo()->rootWindow(),
    0, 0, geom_w, geom_h, resource.border_width, depth(),
    InputOutput, visual(), mask, &attrib);
    geom_visible = False;

    if (resource.wstyle.l_focus.texture() & BImage_ParentRelative) {
    if (resource.wstyle.t_focus.texture() ==
    (BImage_Flat | BImage_Solid)) {
    geom_pixmap = None;
    XSetWindowBackground(*blackbox, geom_window,
    resource.wstyle.t_focus.color().pixel());
    } else {
    geom_pixmap = image_control->renderImage(geom_w, geom_h,
    resource.wstyle.t_focus);
    XSetWindowBackgroundPixmap(*blackbox,
    geom_window, geom_pixmap);
    }
    } else {
    if (resource.wstyle.l_focus.texture() ==
    (BImage_Flat | BImage_Solid)) {
    geom_pixmap = None;
    XSetWindowBackground(*blackbox, geom_window,
    resource.wstyle.l_focus.color().pixel());
    } else {
    geom_pixmap = image_control->renderImage(geom_w, geom_h,
    resource.wstyle.l_focus);
    XSetWindowBackgroundPixmap(*blackbox,
    geom_window, geom_pixmap);
    }
    }
  */

  workspacemenu = new Workspacemenu(this);
  iconmenu = new Iconmenu(screenNumber());
  configmenu = new Configmenu(screenNumber());

  Workspace *wkspc = (Workspace *) 0;
  if (resource.workspaces != 0) {
    for (int i = 0; i < resource.workspaces; ++i) {
      wkspc = new Workspace(this, workspacesList->count());
      workspacesList->insert(wkspc);
      workspacemenu->insert(wkspc->getName(), wkspc->getMenu());
    }
  } else {
    wkspc = new Workspace(this, workspacesList->count());
    workspacesList->insert(wkspc);
    workspacemenu->insert(wkspc->getName(), wkspc->getMenu());
  }

  workspacemenu->insertSeparator();
  workspacemenu->insert(i18n(IconSet, IconIcons, "Icons"),
                        iconmenu);

  current_workspace = workspacesList->first();
  workspacemenu->setItemChecked(3, true);

  toolbar = new Toolbar(this);
  _toolbar2 = new Toolbar2(this);

#ifdef    SLIT
  slit = new Slit(this);
#endif // SLIT

  InitMenu();

  raiseWindows(0, 0);

  updateAvailableArea();

  changeWorkspaceID(0);

  int i;
  unsigned int nchild;
  Window r, p, *children;
  XQueryTree(*blackbox, screenInfo()->rootWindow(), &r, &p,
             &children, &nchild);

  // preen the window list of all icon windows... for better dockapp support
  for (i = 0; i < (int) nchild; i++) {
    if (children[i] == None) continue;

    XWMHints *wmhints = XGetWMHints(*blackbox,
                                    children[i]);

    if (wmhints) {
      if ((wmhints->flags & IconWindowHint) &&
          (wmhints->icon_window != children[i]))
        for (int j = 0; j < (int) nchild; j++)
          if (children[j] == wmhints->icon_window) {
            children[j] = None;

            break;
          }

      XFree(wmhints);
    }
  }

  // manage shown windows
  for (i = 0; i < (int) nchild; ++i) {
    if (children[i] == None || (! blackbox->validateWindow(children[i])))
      continue;

    XWindowAttributes attrib;
    if (XGetWindowAttributes(*blackbox, children[i],
                             &attrib)) {
      if (attrib.override_redirect) continue;

      if (attrib.map_state != IsUnmapped) {
        new BlackboxWindow(blackbox, children[i], this);

        BlackboxWindow *win = blackbox->searchWindow(children[i]);
        if (win) {
          XMapRequestEvent mre;
          mre.window = children[i];
          win->restoreAttributes();
          win->mapRequestEvent(&mre);
        }
      }
    }
  }

  XFree(children);

  // call this again just in case a window we found updates the Strut list
  updateAvailableArea();

  if (! resource.sloppy_focus)
    XSetInputFocus(*blackbox, toolbar->getWindowID(),
                   RevertToParent, CurrentTime);

  XFlush(*blackbox);
}

void BScreen::reconfigure(void)
{
  style()->setCurrentStyle(blackbox->getStyleFilename());

  /*
    ### TODO make the geom display a separate widget
    const char *s = i18n(ScreenSet, ScreenPositionLength,
    "0: 0000 x 0: 0000");
    int l = strlen(s);

    if (i18n->multibyte()) {
    XRectangle ink, logical;
    XmbTextExtents(resource.wstyle.fontset, s, l, &ink, &logical);
    geom_w = logical.width;

    geom_h = resource.wstyle.fontset_extents->max_ink_extent.height;
    } else {
    geom_w = XTextWidth(resource.wstyle.font, s, l);

    geom_h = resource.wstyle.font->ascent +
    resource.wstyle.font->descent;
    }

    geom_w += (resource.bevel_width * 2);
    geom_h += (resource.bevel_width * 2);

    Pixmap tmp = geom_pixmap;
    if (resource.wstyle.l_focus.texture() & BImage_ParentRelative) {
    if (resource.wstyle.t_focus.texture() == (BImage_Flat | BImage_Solid)) {
    geom_pixmap = None;
    XSetWindowBackground(*blackbox, geom_window,
    resource.wstyle.t_focus.color().pixel());
    } else {
    geom_pixmap = image_control->renderImage(geom_w, geom_h,
    resource.wstyle.t_focus);
    XSetWindowBackgroundPixmap(*blackbox,
    geom_window, geom_pixmap);
    }
    } else {
    if (resource.wstyle.l_focus.texture() == (BImage_Flat | BImage_Solid)) {
    geom_pixmap = None;
    XSetWindowBackground(*blackbox, geom_window,
    resource.wstyle.l_focus.color().pixel());
    } else {
    geom_pixmap = image_control->renderImage(geom_w, geom_h,
    resource.wstyle.l_focus);
    XSetWindowBackgroundPixmap(*blackbox,
    geom_window, geom_pixmap);
    }
    }
    if (tmp) image_control->removeImage(tmp);

    XSetWindowBorderWidth(*blackbox, geom_window,
    resource.border_width);
    XSetWindowBorder(*blackbox, geom_window,
    resource.border_color.pixel());
  */

  workspacemenu->reconfigure();
  iconmenu->reconfigure();

  InitMenu();
  rootmenu->reconfigure();

  configmenu->reconfigure();

  _toolbar2->reconfigure();
  toolbar->reconfigure();

#ifdef    SLIT
  slit->reconfigure();
#endif // SLIT

  LinkedListIterator<Workspace> wit(workspacesList);
  for (Workspace *w = wit.current(); w; wit++, w = wit.current())
    w->reconfigure();

  LinkedListIterator<BlackboxWindow> iit(iconList);
  for (BlackboxWindow *bw = iit.current(); bw; iit++, bw = iit.current())
    if (bw->validateClient())
      bw->reconfigure();

  image_control->timeout();
}

void BScreen::rereadMenu(void)
{
  InitMenu();
  raiseWindows(0, 0);
  rootmenu->reconfigure();
}

void BScreen::removeWorkspaceNames(void)
{
  while (workspaceNames->count())
    delete [] workspaceNames->remove(0);
}

void BScreen::addIcon(BlackboxWindow *w) {
  if (! w) return;

  w->setWorkspace(-1);
  w->setWindowNumber(iconList->count());

  iconList->insert(w);
  iconmenu->insert(*w->getIconTitle());
}

void BScreen::removeIcon(BlackboxWindow *w) {
  if (! w)
    return;

  iconList->remove(w->getWindowNumber());
  iconmenu->remove(w->getWindowNumber());

  LinkedListIterator<BlackboxWindow> it(iconList);
  BlackboxWindow *bw = it.current();
  for (int i = 0; bw; it++, bw = it.current())
    bw->setWindowNumber(i++);
}

void BScreen::changeIconName(BlackboxWindow *w)
{
  iconmenu->change(w->getWindowNumber(), *w->getIconTitle());
}

BlackboxWindow *BScreen::icon(int index) {
  if (index >= 0 && index < iconList->count())
    return iconList->find(index);
  return (BlackboxWindow *) 0;
}

int BScreen::addWorkspace(void) {
  Workspace *wkspc = new Workspace(this, workspacesList->count());
  workspacesList->insert(wkspc);

  workspacemenu->insert(wkspc->getName(), wkspc->getMenu(),
			wkspc->getWorkspaceID() + 3);

  toolbar->reconfigure();

  updateNetizenWorkspaceCount();

  return workspacesList->count();
}


int BScreen::removeLastWorkspace(void) {
  if (workspacesList->count() == 1)
    return 0;

  Workspace *wkspc = workspacesList->last();

  if (current_workspace->getWorkspaceID() == wkspc->getWorkspaceID())
    changeWorkspaceID(current_workspace->getWorkspaceID() - 1);

  wkspc->removeAll();

  workspacemenu->remove(wkspc->getWorkspaceID() + 3);

  workspacesList->remove(wkspc);
  delete wkspc;

  toolbar->reconfigure();

  updateNetizenWorkspaceCount();

  return workspacesList->count();
}


void BScreen::changeWorkspaceID(int id) {
  if (! current_workspace) return;

  if (id != current_workspace->getWorkspaceID()) {
    current_workspace->hideAll();

    workspacemenu->setItemChecked(current_workspace->getWorkspaceID() + 3,
                                   false);

    if (blackbox->getFocusedWindow() &&
	blackbox->getFocusedWindow()->getScreen() == this &&
        (! blackbox->getFocusedWindow()->isStuck())) {
      current_workspace->setLastFocusedWindow(blackbox->getFocusedWindow());
      blackbox->setFocusedWindow((BlackboxWindow *) 0);
    }

    current_workspace = getWorkspace(id);

    workspacemenu->setItemChecked(current_workspace->getWorkspaceID() + 3,
				   true);
    toolbar->redrawWorkspaceLabel(True);

    current_workspace->showAll();

    if (resource.focus_last && current_workspace->getLastFocusedWindow()) {
      XSync(*blackbox, False);
      current_workspace->getLastFocusedWindow()->setInputFocus();
    }
  }

  updateNetizenCurrentWorkspace();
}


void BScreen::addNetizen(Netizen *n) {
  netizenList->insert(n);

  n->sendWorkspaceCount();
  n->sendCurrentWorkspace();

  LinkedListIterator<Workspace> it(workspacesList);
  for (Workspace *w = it.current(); w; it++, w = it.current()) {
    for (int i = 0; i < w->getCount(); i++)
      n->sendWindowAdd(w->getWindow(i)->getClientWindow(),
		       w->getWorkspaceID());
  }

  Window f = ((blackbox->getFocusedWindow()) ?
              blackbox->getFocusedWindow()->getClientWindow() : None);
  n->sendWindowFocus(f);
}


void BScreen::removeNetizen(Window w) {
  LinkedListIterator<Netizen> it(netizenList);
  int i = 0;

  for (Netizen *n = it.current(); n; it++, i++, n = it.current())
    if (n->windowID() == w) {
      Netizen *tmp = netizenList->remove(i);
      delete tmp;

      break;
    }
}


void BScreen::updateNetizenCurrentWorkspace(void) {
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendCurrentWorkspace();
}


void BScreen::updateNetizenWorkspaceCount(void) {
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendWorkspaceCount();
}


void BScreen::updateNetizenWindowFocus(void) {
  Window f = ((blackbox->getFocusedWindow()) ?
              blackbox->getFocusedWindow()->getClientWindow() : None);
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendWindowFocus(f);
}


void BScreen::updateNetizenWindowAdd(Window w, unsigned long p) {
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendWindowAdd(w, p);
}


void BScreen::updateNetizenWindowDel(Window w) {
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendWindowDel(w);
}


void BScreen::updateNetizenWindowRaise(Window w) {
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendWindowRaise(w);
}


void BScreen::updateNetizenWindowLower(Window w) {
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendWindowLower(w);
}


void BScreen::updateNetizenConfigNotify(XEvent *e) {
  LinkedListIterator<Netizen> it(netizenList);
  for (Netizen *n = it.current(); n; it++, n = it.current())
    n->sendConfigNotify(e);
}


void BScreen::raiseWindows(Window *workspace_stack, int num)
{
  Window *session_stack = new Window[num + 2];
  int i = 0, k = num;

  if (toolbar->isOnTop())
    *(session_stack + i++) = toolbar->getWindowID();

#ifdef    SLIT
  if (slit->isOnTop())
    *(session_stack + i++) = slit->getWindowID();
#endif // SLIT

  while (k--)
    *(session_stack + i++) = *(workspace_stack + k);

  if (i > 0) {
    XRaiseWindow(*blackbox, *session_stack);
    XRestackWindows(*blackbox, session_stack, i);
  }

  delete [] session_stack;
}


#ifdef    HAVE_STRFTIME
void BScreen::saveStrftimeFormat(char *format) {
  if (resource.strftime_format)
    delete [] resource.strftime_format;

  resource.strftime_format = bstrdup(format);
}
#endif // HAVE_STRFTIME


void BScreen::addWorkspaceName(char *name) {
  workspaceNames->insert(bstrdup(name));
}


char* BScreen::getNameOfWorkspace(int id) {
  char *name = (char *) 0;

  if (id >= 0 && id < workspaceNames->count()) {
    char *wkspc_name = workspaceNames->find(id);

    if (wkspc_name)
      name = wkspc_name;
  }
  return name;
}


void BScreen::reassociateWindow(BlackboxWindow *w, int wkspc_id, Bool ignore_sticky) {
  if (! w) return;

  if (wkspc_id == -1)
    wkspc_id = current_workspace->getWorkspaceID();

  if (w->getWorkspaceNumber() == wkspc_id)
    return;

  if (w->isIconic()) {
    removeIcon(w);
    getWorkspace(wkspc_id)->addWindow(w);
  } else if (ignore_sticky || ! w->isStuck()) {
    getWorkspace(w->getWorkspaceNumber())->removeWindow(w);
    getWorkspace(wkspc_id)->addWindow(w);
  }
}


void BScreen::nextFocus(void) {
  Bool have_focused = False;
  int focused_window_number = -1;
  BlackboxWindow *next;

  if (blackbox->getFocusedWindow()) {
    if (blackbox->getFocusedWindow()->getScreen()->screenNumber() ==
	screenNumber()) {
      have_focused = True;
      focused_window_number = blackbox->getFocusedWindow()->getWindowNumber();
    }
  }

  if ((getCurrentWorkspace()->getCount() > 1) && have_focused) {
    int next_window_number = focused_window_number;
    do {
      if ((++next_window_number) >= getCurrentWorkspace()->getCount())
	next_window_number = 0;

      next = getCurrentWorkspace()->getWindow(next_window_number);
    } while ((! next->setInputFocus()) && (next_window_number !=
					   focused_window_number));

    if (next_window_number != focused_window_number)
      getCurrentWorkspace()->raiseWindow(next);
  } else if (getCurrentWorkspace()->getCount() >= 1) {
    next = current_workspace->getWindow(0);

    current_workspace->raiseWindow(next);
    next->setInputFocus();
  }
}


void BScreen::prevFocus(void) {
  Bool have_focused = False;
  int focused_window_number = -1;
  BlackboxWindow *prev;

  if (blackbox->getFocusedWindow()) {
    if (blackbox->getFocusedWindow()->getScreen()->screenNumber() ==
	screenNumber()) {
      have_focused = True;
      focused_window_number = blackbox->getFocusedWindow()->getWindowNumber();
    }
  }

  if ((getCurrentWorkspace()->getCount() > 1) && have_focused) {
    int prev_window_number = focused_window_number;
    do {
      if ((--prev_window_number) < 0)
	prev_window_number = getCurrentWorkspace()->getCount() - 1;

      prev = getCurrentWorkspace()->getWindow(prev_window_number);
    } while ((! prev->setInputFocus()) && (prev_window_number !=
					   focused_window_number));

    if (prev_window_number != focused_window_number)
      getCurrentWorkspace()->raiseWindow(prev);
  } else if (getCurrentWorkspace()->getCount() >= 1) {
    prev = current_workspace->getWindow(0);

    current_workspace->raiseWindow(prev);
    prev->setInputFocus();
  }
}


void BScreen::raiseFocus(void) {
  Bool have_focused = False;
  int focused_window_number = -1;

  if (blackbox->getFocusedWindow()) {
    if (blackbox->getFocusedWindow()->getScreen()->screenNumber() ==
	screenNumber()) {
      have_focused = True;
      focused_window_number = blackbox->getFocusedWindow()->getWindowNumber();
    }
  }

  if ((getCurrentWorkspace()->getCount() > 1) && have_focused)
    getWorkspace(blackbox->getFocusedWindow()->getWorkspaceNumber())->
      raiseWindow(blackbox->getFocusedWindow());
}


void BScreen::InitMenu(void)
{
  if (rootmenu)
    rootmenu->clear();
  else
    rootmenu = new Rootmenu(screenNumber());

  Bool defaultMenu = True;

  if (blackbox->getMenuFilename()) {
    FILE *menu_file = fopen(blackbox->getMenuFilename(), "r");

    if (!menu_file) {
      perror(blackbox->getMenuFilename());
    } else {
      if (feof(menu_file)) {
        fprintf(stderr, i18n(ScreenSet, ScreenEmptyMenuFile,
                             "%s: Empty menu file"),
                blackbox->getMenuFilename());
      } else {
        char line[1024], label[1024];
        memset(line, 0, 1024);
        memset(label, 0, 1024);

        while (fgets(line, 1024, menu_file) && ! feof(menu_file)) {
          if (line[0] != '#') {
            int i, key = 0, index = -1, len = strlen(line);

            key = 0;
            for (i = 0; i < len; i++) {
              if (line[i] == '[') index = 0;
              else if (line[i] == ']') break;
              else if (line[i] != ' ')
                if (index++ >= 0)
                  key += tolower(line[i]);
            }

            if (key == 517) {
              index = -1;
              for (i = index; i < len; i++) {
                if (line[i] == '(') index = 0;
                else if (line[i] == ')') break;
                else if (index++ >= 0) {
                  if (line[i] == '\\' && i < len - 1) i++;
                  label[index - 1] = line[i];
                }
              }

              if (index == -1) index = 0;
              label[index] = '\0';

              rootmenu->setTitle(label);
              rootmenu->showTitle();
              defaultMenu = parseMenuFile(menu_file, rootmenu);
              break;
            }
          }
        }
      }
      fclose(menu_file);
    }
  }

  if (defaultMenu) {
    // rootmenu->setInternalMenu();
    /*
      rootmenu->insert(i18n(ScreenSet, Screenxterm, "xterm"),
      BScreen::Execute,
      i18n(ScreenSet, Screenxterm, "xterm"));
      rootmenu->insert(i18n(ScreenSet, ScreenRestart, "Restart"),
      BScreen::Restart);
      rootmenu->insert(i18n(ScreenSet, ScreenExit, "Exit"),
      BScreen::Exit);
    */
  } else {
    blackbox->saveMenuFilename(blackbox->getMenuFilename());
  }
}


Bool BScreen::parseMenuFile(FILE *file, Rootmenu *menu) {
  char line[1024], label[1024], command[1024];

  while (! feof(file)) {
    memset(line, 0, 1024);
    memset(label, 0, 1024);
    memset(command, 0, 1024);

    if (fgets(line, 1024, file)) {
      if (line[0] != '#') {
        int i, key = 0, parse = 0, index = -1,
       line_length = strlen(line);

        // determine the keyword
        key = 0;
        for (i = 0; i < line_length; i++) {
          if (line[i] == '[') parse = 1;
          else if (line[i] == ']') break;
          else if (line[i] != ' ')
            if (parse)
              key += tolower(line[i]);
        }

        // get the label enclosed in ()'s
        parse = 0;

        for (i = 0; i < line_length; i++) {
          if (line[i] == '(') {
            index = 0;
            parse = 1;
          } else if (line[i] == ')') break;
          else if (index++ >= 0) {
            if (line[i] == '\\' && i < line_length - 1) i++;
            label[index - 1] = line[i];
          }
        }

        if (parse) {
          label[index] = '\0';
        } else {
          label[0] = '\0';
        }

        // get the command enclosed in {}'s
        parse = 0;
        index = -1;
        for (i = 0; i < line_length; i++) {
          if (line[i] == '{') {
            index = 0;
            parse = 1;
          } else if (line[i] == '}') break;
          else if (index++ >= 0) {
            if (line[i] == '\\' && i < line_length - 1) i++;
            command[index - 1] = line[i];
          }
        }

        if (parse) {
          command[index] = '\0';
        } else {
          command[0] = '\0';
        }

        switch (key) {
        case 311: //end
          return ((menu->count() == 0) ? True : False);

          break;

        case 328: // sep
          menu->insertSeparator();
          break;

        case 333: // nop
          menu->insert(label);
          break;

        case 421: // exec
          if ((! *label) && (! *command)) {
            fprintf(stderr, i18n(ScreenSet, ScreenEXECError,
                                 "BScreen::parseMenuFile: [exec] error, "
                                 "no menu label and/or command defined\n"));
            continue;
          }

          menu->insert(label, Rootmenu::Item(Rootmenu::Execute, command));
          break;

        case 442: // exit
          if (! *label) {
            fprintf(stderr, i18n(ScreenSet, ScreenEXITError,
                                 "BScreen::parseMenuFile: [exit] error, "
                                 "no menu label defined\n"));
            continue;
          }

          menu->insert(label, Rootmenu::Exit);

          break;

        case 561: // style
          {
            if ((! *label) || (! *command)) {
              fprintf(stderr, i18n(ScreenSet, ScreenSTYLEError,
                                   "BScreen::parseMenuFile: [style] error, "
                                   "no menu label and/or filename defined\n"));
              continue;
            }

            char *style = expandTilde(command);
            menu->insert(label, Rootmenu::Item(Rootmenu::SetStyle, style));
	    delete [] style;
          }

          break;

        case 630: // config
          if (! *label) {
            fprintf(stderr, i18n(ScreenSet, ScreenCONFIGError,
                                 "BScreen::parseMenufile: [config] error, "
                                 "no label defined"));
            continue;
          }

          menu->insert(label, configmenu);
          break;

        case 740: // include
          {
            if (! *label) {
              fprintf(stderr, i18n(ScreenSet, ScreenINCLUDEError,
                                   "BScreen::parseMenuFile: [include] error, "
                                   "no filename defined\n"));
              continue;
            }

            char *newfile = expandTilde(label);
	    FILE *submenufile = fopen(newfile, "r");

	    if (submenufile) {
	      struct stat buf;
	      if (fstat(fileno(submenufile), &buf) ||
		  (! S_ISREG(buf.st_mode))) {
		fprintf(stderr,
			i18n(ScreenSet, ScreenINCLUDEErrorReg,
                             "BScreen::parseMenuFile: [include] error: "
                             "'%s' is not a regular file\n"), newfile);
		delete [] newfile;
		break;
	      }

	      if (! feof(submenufile)) {
		if (! parseMenuFile(submenufile, menu))
		  blackbox->saveMenuFilename(newfile);

		fclose(submenufile);
	      }
	    } else {
	      perror(newfile);
	    }
	    delete [] newfile;
          }

          break;

        case 767: // submenu
          {
            if (! *label) {
              fprintf(stderr, i18n(ScreenSet, ScreenSUBMENUError,
                                   "BScreen::parseMenuFile: [submenu] error, "
                                   "no menu label defined\n"));
              continue;
            }

            Rootmenu *submenu = new Rootmenu(screenNumber());

            if (*command)
              submenu->setTitle(command);
            else
              submenu->setTitle(label);
            submenu->showTitle();

            parseMenuFile(file, submenu);
            menu->insert(label, submenu);
          }

          break;

        case 773: // restart
          {
            if (! *label) {
              fprintf(stderr, i18n(ScreenSet, ScreenRESTARTError,
                                   "BScreen::parseMenuFile: [restart] error, "
                                   "no menu label defined\n"));
              continue;
            }

            if (*command)
              menu->insert(label, Rootmenu::Item(Rootmenu::RestartOther, command));
            else
              menu->insert(label, Rootmenu::Restart);
          }

          break;

        case 845: // reconfig
          {
            if (! *label) {
              fprintf(stderr, i18n(ScreenSet, ScreenRECONFIGError,
                                   "BScreen::parseMenuFile: [reconfig] error, "
                                   "no menu label defined\n"));
              continue;
            }

            menu->insert(label, Rootmenu::Reconfigure);
          }

          break;

        case 995: // stylesdir
        case 1113: // stylesmenu
          {
            Bool newmenu = ((key == 1113) ? True : False);

            if ((! *label) || ((! *command) && newmenu)) {
              fprintf(stderr,
                      i18n(ScreenSet, ScreenSTYLESDIRError,
                           "BScreen::parseMenuFile: [stylesdir/stylesmenu]"
                           " error, no directory defined\n"));
              continue;
            }

            char *directory = ((newmenu) ? command : label);

            char *stylesdir = expandTilde(directory);

            struct stat statbuf;

            if (! stat(stylesdir, &statbuf)) {
              if (S_ISDIR(statbuf.st_mode)) {
                Rootmenu *stylesmenu;

                if (newmenu)
                  stylesmenu = new Rootmenu(screenNumber());
                else
                  stylesmenu = menu;

                DIR *d = opendir(stylesdir);
                int entries = 0;
                struct dirent *p;

                // get the total number of directory entries
                while ((p = readdir(d))) entries++;
                rewinddir(d);

                char **ls = new char* [entries];
                int index = 0;
                while ((p = readdir(d)))
                  ls[index++] = bstrdup(p->d_name);

                closedir(d);

                std::sort(ls, ls + entries, dcmp());

		// walk list of filenames, skipping tilde files and dotfiles,
		// inserting them into the stylesmenu after adding the
		// directory name
                for (int n = 0; n < entries; n++) {
                  if (ls[n][strlen(ls[n])-1] != '~' ||
		      ls[n][0] == '.') {
		    string style = stylesdir;
		    style = style + '/' + ls[n];

                    if ((! stat(style.c_str(), &statbuf)) &&
			S_ISREG(statbuf.st_mode)) {
                      stylesmenu->insert(ls[n],
					 Rootmenu::Item(Rootmenu::SetStyle,
							style));
		    }
                  }

                  delete [] ls[n];
                }

                delete [] ls;

                if (newmenu) {
                  stylesmenu->setTitle(label);
                  stylesmenu->showTitle();
                  menu->insert(label, stylesmenu);
                }

                blackbox->saveMenuFilename(stylesdir);
              } else {
                fprintf(stderr, i18n(ScreenSet,
                                     ScreenSTYLESDIRErrorNotDir,
                                     "BScreen::parseMenuFile:"
                                     " [stylesdir/stylesmenu] error, %s is not a"
                                     " directory\n"), stylesdir);
              }
            } else {
              fprintf(stderr,
                      i18n(ScreenSet, ScreenSTYLESDIRErrorNoExist,
                           "BScreen::parseMenuFile: [stylesdir/stylesmenu]"
                           " error, %s does not exist\n"), stylesdir);
            }
	    delete [] stylesdir;
            break;
          }

        case 1090: // workspaces
          {
            if (! *label) {
              fprintf(stderr,
                      i18n(ScreenSet, ScreenWORKSPACESError,
                           "BScreen:parseMenuFile: [workspaces] error, "
                           "no menu label defined\n"));
              continue;
            }

            menu->insert(label, workspacemenu);

            break;
          }
        }
      }
    }
  }

  return ((menu->count() == 0) ? True : False);
}


void BScreen::shutdown(void)
{
  XGrabServer(*blackbox);

  XSelectInput(*blackbox, screenInfo()->rootWindow(), NoEventMask);
  XSync(*blackbox, False);

  LinkedListIterator<Workspace> it(workspacesList);
  for (Workspace *w = it.current(); w; it++, w = it.current())
    w->shutdown();

  while (iconList->count()) {
    iconList->first()->restore();
    delete iconList->first();
  }

#ifdef    SLIT
  slit->shutdown();
#endif // SLIT

  XUngrabServer(*blackbox);
}


void BScreen::showPosition(int x, int y)
{
  /*
    if (! geom_visible) {
    XMoveResizeWindow(*blackbox, geom_window,
    (width() - geom_w) / 2,
    (height() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(*blackbox, geom_window);
    XRaiseWindow(*blackbox, geom_window);

    geom_visible = True;
    }

    char label[1024];

    sprintf(label, i18n(ScreenSet, ScreenPositionFormat,
    "X: %4d x Y: %4d"), x, y);

    XClearWindow(*blackbox, geom_window);

    BGCCache::Item &gc = BGCCache::instance()->find(resource.wstyle.l_text_focus,
    resource.wstyle.font);
    if (i18n->multibyte()) {
    XmbDrawString(*blackbox, geom_window,
    resource.wstyle.fontset, gc.gc(),
    resource.bevel_width, resource.bevel_width -
    resource.wstyle.fontset_extents->max_ink_extent.y,
    label, strlen(label));
    } else {
    XDrawString(*blackbox, geom_window, gc.gc(),
    resource.bevel_width,
    resource.wstyle.font->ascent + resource.bevel_width,
    label, strlen(label));
    }
    BGCCache::instance()->release(gc);
  */
}


void BScreen::showGeometry(unsigned int gx, unsigned int gy)
{
  /*
    if (! geom_visible) {
    XMoveResizeWindow(*blackbox, geom_window,
    (width() - geom_w) / 2,
    (height() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(*blackbox, geom_window);
    XRaiseWindow(*blackbox, geom_window);

    geom_visible = True;
    }

    char label[1024];

    sprintf(label, i18n(ScreenSet, ScreenGeometryFormat,
    "W: %4d x H: %4d"), gx, gy);

    XClearWindow(*blackbox, geom_window);

    BGCCache::Item &gc = BGCCache::instance()->find(resource.wstyle.l_text_focus,
    resource.wstyle.font);
    if (i18n->multibyte()) {
    XmbDrawString(*blackbox, geom_window,
    resource.wstyle.fontset, gc.gc(),
    resource.bevel_width, resource.bevel_width -
    resource.wstyle.fontset_extents->max_ink_extent.y,
    label, strlen(label));
    } else {
    XDrawString(*blackbox, geom_window, gc.gc(),
    resource.bevel_width, resource.wstyle.font->ascent +
    resource.bevel_width, label, strlen(label));
    }
  */
}


void BScreen::hideGeometry(void)
{
  /*
    if (geom_visible) {
    XUnmapWindow(*blackbox, geom_window);
    geom_visible = False;
    }
  */
}

void BScreen::addStrut(NETStrut *strut)
{
  strutList->insert(strut);
}

void BScreen::removeStrut(NETStrut *strut)
{
  strutList->remove(strut);
}

void BScreen::updateAvailableArea(void)
{
  Rect newarea = screeninfo->rect();

  LinkedListIterator<NETStrut> it(strutList);
  unsigned int l = 0, r = 0, t = 0, b = 0;
  for(NETStrut *strut = it.current(); strut; ++it, strut = it.current()) {
    l += strut->left;
    r += strut->right;
    t += strut->top;
    b += strut->bottom;
  }

  newarea.setCoords(newarea.left() + l,  newarea.top() + t,
                    newarea.right() - r, newarea.bottom() - b);

  if (newarea != usableArea) {
    // TODO: update maximized windows to reflect new screen area
  }
  usableArea = newarea;
}

int BScreen::getCurrentWorkspaceID() const
{
  return current_workspace->getWorkspaceID();
}
