// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Screen.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2003
//         Bradley T Hughes <bhughes at trolltech.com>
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
#include <X11/Xatom.h>
#include <X11/keysym.h>

// for strcasestr()
#ifndef _GNU_SOURCE
#  define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif // HAVE_STDLIB_H

#ifdef HAVE_STRING_H
#  include <string.h>
#endif // HAVE_STRING_H

#ifdef    HAVE_CTYPE_H
#  include <ctype.h>
#endif // HAVE_CTYPE_H

#ifdef    HAVE_UNISTD_H
#  include <sys/types.h>
#  include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef    HAVE_DIRENT_H
#  include <dirent.h>
#endif // HAVE_DIRENT_H

#ifdef    HAVE_LOCALE_H
#  include <locale.h>
#endif // HAVE_LOCALE_H

#ifdef    HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#ifdef    HAVE_STDARG_H
#  include <stdarg.h>
#endif // HAVE_STDARG_H
}

#include <assert.h>

#include <algorithm>
#include <functional>
#include <string>

#include "i18n.hh"
#include "blackbox.hh"
#include "Clientmenu.hh"
#include "Font.hh"
#include "Iconmenu.hh"
#include "Image.hh"
#include "Menu.hh"
#include "Netwm.hh"
#include "Pen.hh"
#include "PixmapCache.hh"
#include "Resource.hh"
#include "Screen.hh"
#include "Slit.hh"
#include "Rootmenu.hh"
#include "Toolbar.hh"
#include "Util.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"


static bool running = True;

static int anotherWMRunning(Display *display, XErrorEvent *) {
  fprintf(stderr, bt::i18n(ScreenSet, ScreenAnotherWMRunning,
          "BScreen::BScreen: an error occured while querying the X server.\n"
          "  another window manager already running on display %s.\n"),
          DisplayString(display));

  running = False;

  return(-1);
}


BScreen::BScreen(Blackbox *bb, unsigned int scrn) :
  screen_info(bb->display().screenInfo(scrn)), blackbox(bb) {

  XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);
  XSelectInput(screen_info.display().XDisplay(),
               screen_info.rootWindow(),
               ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
               SubstructureRedirectMask | 
	       StructureNotifyMask | SubstructureNotifyMask |
	       ButtonPressMask | ButtonReleaseMask;

  XSync(screen_info.display().XDisplay(), False);
  XSetErrorHandler((XErrorHandler) old);

  managed = running;
  if (! managed) return;

  fprintf(stderr, bt::i18n(ScreenSet, ScreenManagingScreen,
                           "BScreen::BScreen: managing screen %d "
                           "using visual 0x%lx, depth %d\n"),
          screen_info.screenNumber(),
          XVisualIDFromVisual(screen_info.visual()),
          screen_info.depth());

  blackbox->insertEventHandler(screen_info.rootWindow(), this);

  rootmenu = 0;

  geom_pixmap = None;

  XDefineCursor(blackbox->XDisplay(), screen_info.rootWindow(),
                blackbox->getSessionCursor());

  // start off full screen, top left.
  usableArea.setSize(screen_info.width(), screen_info.height());

  blackbox->load_rc(this);

  LoadStyle();

  XGCValues gcv;
  unsigned long gc_value_mask = GCForeground;
  if (! bt::i18n.multibyte()) gc_value_mask |= GCFont;

  gcv.foreground = WhitePixel(blackbox->XDisplay(),
                              screen_info.screenNumber())
                   ^ BlackPixel(blackbox->XDisplay(),
                                screen_info.screenNumber());
  gcv.function = GXxor;
  gcv.subwindow_mode = IncludeInferiors;
  opGC = XCreateGC(blackbox->XDisplay(), screen_info.rootWindow(),
                   GCForeground | GCFunction | GCSubwindowMode, &gcv);

  const char *s =
    bt::i18n(ScreenSet, ScreenPositionLength, "0: 0000 x 0: 0000");
  bt::Rect geomr = bt::textRect(_resource.windowStyle()->font, s);
  geom_w = geomr.width() + (_resource.bevelWidth() * 2);
  geom_h = geomr.height() + (_resource.bevelWidth() * 2);

  XSetWindowAttributes setattrib;
  unsigned long mask = CWBorderPixel | CWColormap | CWSaveUnder;
  setattrib.border_pixel =
    _resource.borderColor()->pixel(screen_info.screenNumber());
  setattrib.colormap = screen_info.colormap();
  setattrib.save_under = True;

  geom_window = XCreateWindow(blackbox->XDisplay(),
                              screen_info.rootWindow(),
                              0, 0, geom_w, geom_h, _resource.borderWidth(),
                              screen_info.depth(), InputOutput,
                              screen_info.visual(), mask, &setattrib);
  geom_visible = False;

  bt::Texture texture = _resource.windowStyle()->l_focus;
  geom_pixmap = bt::PixmapCache::find(screen_info.screenNumber(),
                                      texture, geom_w, geom_h, geom_pixmap);
  if (geom_pixmap == ParentRelative) {
    texture = _resource.windowStyle()->t_focus;
    geom_pixmap = bt::PixmapCache::find(screen_info.screenNumber(),
                                        texture, geom_w, geom_h, geom_pixmap);
  }
  if (! geom_pixmap)
    XSetWindowBackground(blackbox->XDisplay(), geom_window,
                         texture.color().pixel(screen_info.screenNumber()));
  else
    XSetWindowBackgroundPixmap(blackbox->XDisplay(),
                               geom_window, geom_pixmap);

  workspacemenu =
    new Workspacemenu(*blackbox, screen_info.screenNumber(), this);
  iconmenu =
    new Iconmenu(*blackbox, screen_info.screenNumber(), this);
  configmenu =
    new Configmenu(*blackbox, screen_info.screenNumber(), this);

  Workspace *wkspc = (Workspace *) 0;
  if (_resource.numberOfWorkspaces() != 0) {
    for (unsigned int i = 0; i < _resource.numberOfWorkspaces(); ++i) {
      wkspc = new Workspace(this, i);
      workspacesList.push_back(wkspc);
      workspacemenu->insertItem(wkspc->getName(), wkspc->getMenu(),
                                wkspc->getID());
    }
  } else {
    wkspc = new Workspace(this, 0u);
    workspacesList.push_back(wkspc);
    workspacemenu->insertItem(wkspc->getName(), wkspc->getMenu(),
                              wkspc->getID());
  }

  workspacemenu->insertIconMenu(iconmenu);

  current_workspace = workspacesList.front();
  current_workspace_id = current_workspace->getID();
  workspacemenu->setItemChecked(current_workspace->getID(), true);

  toolbar = new Toolbar(this);

  slit = new Slit(this);

  InitMenu();

  raiseWindows((WindowStack*) 0);

  const bt::Netwm& netwm = blackbox->netwm();
  /*
    netwm requires the window manager to set a property on a window it creates
    which is the id of that window.  We must also set an equivalent property
    on the root window.  Then we must set _NET_WM_NAME on the child window
    to be the name of the wm.
  */
  netwm.setSupportingWMCheck(screen_info.rootWindow(), geom_window);
  netwm.setSupportingWMCheck(geom_window, geom_window);
  netwm.setWMName(geom_window, "Blackbox");

  netwm.setNumberOfDesktops(screen_info.rootWindow(),
                             workspacesList.size());
  netwm.setDesktopGeometry(screen_info.rootWindow(),
                            screen_info.width(), screen_info.height());
  netwm.setActiveWindow(screen_info.rootWindow(), None);
  updateWorkareaHint();
  updateDesktopNamesHint();

  Atom supported[46] = {
    netwm.clientList(),
    netwm.clientListStacking(),
    netwm.numberOfDesktops(),
    netwm.desktopGeometry(),
    netwm.currentDesktop(),
    netwm.desktopNames(),
    netwm.activeWindow(),
    netwm.workarea(),
    netwm.closeWindow(),
    netwm.moveresizeWindow(),
    netwm.wmName(),
    netwm.wmVisibleName(),
    netwm.wmIconName(),
    netwm.wmVisibleIconName(),
    netwm.wmDesktop(),
    netwm.wmWindowType(),
    netwm.wmWindowTypeDesktop(),
    netwm.wmWindowTypeDock(),
    netwm.wmWindowTypeToolbar(),
    netwm.wmWindowTypeMenu(),
    netwm.wmWindowTypeUtility(),
    netwm.wmWindowTypeSplash(),
    netwm.wmWindowTypeDialog(),
    netwm.wmWindowTypeNormal(),
    netwm.wmState(),
    netwm.wmStateModal(),
    /* sticky would go here, but we do not need it */
    netwm.wmStateMaximizedVert(),
    netwm.wmStateMaximizedHorz(),
    netwm.wmStateShaded(),
    netwm.wmStateSkipTaskbar(),
    netwm.wmStateSkipPager(),
    netwm.wmStateHidden(),
    netwm.wmStateFullscreen(),
    netwm.wmStateAbove(),
    netwm.wmStateBelow(),
    netwm.wmAllowedActions(),
    netwm.wmActionMove(),
    netwm.wmActionResize(),
    netwm.wmActionMinimize(),
    netwm.wmActionShade(),
    netwm.wmActionMaximizeHorz(),
    netwm.wmActionMaximizeVert(),
    netwm.wmActionFullscreen(),
    netwm.wmActionChangeDesktop(),
    netwm.wmActionClose(),
    netwm.wmStrut()
  };

  netwm.setSupported(screen_info.rootWindow(), supported, 46);

  unsigned int i, j, nchild;
  Window r, p, *children;
  XQueryTree(blackbox->XDisplay(), screen_info.rootWindow(), &r, &p,
             &children, &nchild);

  // preen the window list of all icon windows... for better dockapp support
  for (i = 0; i < nchild; i++) {
    if (children[i] == None) continue;

    XWMHints *wmhints = XGetWMHints(blackbox->XDisplay(),
                                    children[i]);

    if (wmhints) {
      if ((wmhints->flags & IconWindowHint) &&
          (wmhints->icon_window != children[i])) {
        for (j = 0; j < nchild; j++) {
          if (children[j] == wmhints->icon_window) {
            children[j] = None;
            break;
          }
        }
      }

      XFree(wmhints);
    }
  }

  // manage shown windows
  for (i = 0; i < nchild; ++i) {
    if (children[i] == None || ! blackbox->validateWindow(children[i]))
      continue;

    XWindowAttributes attrib;
    if (XGetWindowAttributes(blackbox->XDisplay(), children[i], &attrib)) {
      if (attrib.override_redirect) continue;

      if (attrib.map_state != IsUnmapped) {
        manageWindow(children[i]);
      }
    }
  }

  XFree(children);

  updateClientListHint();
  updateClientListStackingHint();
}


BScreen::~BScreen(void) {
  if (! managed) return;

  blackbox->removeEventHandler(screen_info.rootWindow());

  bt::PixmapCache::release(geom_pixmap);

  if (geom_window != None)
    XDestroyWindow(blackbox->XDisplay(), geom_window);

  std::for_each(workspacesList.begin(), workspacesList.end(),
                bt::PointerAssassin());

  std::for_each(iconList.begin(), iconList.end(), bt::PointerAssassin());

  delete rootmenu;
  delete workspacemenu;
  delete iconmenu;
  delete configmenu;
  delete slit;
  delete toolbar;

  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                   blackbox->netwm().supportingWMCheck());
  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                    blackbox->netwm().supported());
  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                    blackbox->netwm().numberOfDesktops());
  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                    blackbox->netwm().desktopGeometry());
  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                    blackbox->netwm().currentDesktop());
  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                    blackbox->netwm().activeWindow());
  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                    blackbox->netwm().workarea());

  XFreeGC(blackbox->XDisplay(), opGC);
}


void BScreen::reconfigure(void) {
  LoadStyle();

  XGCValues gcv;
  unsigned long gc_value_mask = GCForeground;
  if (! bt::i18n.multibyte()) gc_value_mask |= GCFont;

  gcv.foreground = WhitePixel(blackbox->XDisplay(),
                              screen_info.screenNumber())
                   ^ BlackPixel(blackbox->XDisplay(),
                                screen_info.screenNumber());
  gcv.function = GXxor;
  gcv.subwindow_mode = IncludeInferiors;
  XChangeGC(blackbox->XDisplay(), opGC,
            GCForeground | GCFunction | GCSubwindowMode, &gcv);

  const char *s =
    bt::i18n(ScreenSet, ScreenPositionLength, "0: 0000 x 0: 0000");
  bt::Rect geomr = bt::textRect(_resource.windowStyle()->font, s);
  geom_w = geomr.width() + (_resource.bevelWidth() * 2);
  geom_h = geomr.height() + (_resource.bevelWidth() * 2);

  bt::Texture texture = _resource.windowStyle()->l_focus;
  geom_pixmap = bt::PixmapCache::find(screen_info.screenNumber(),
                                      texture, geom_w, geom_h, geom_pixmap);
  if (geom_pixmap == ParentRelative) {
    texture = _resource.windowStyle()->t_focus;
    geom_pixmap = bt::PixmapCache::find(screen_info.screenNumber(),
                                        texture, geom_w, geom_h, geom_pixmap);
  }
  if (! geom_pixmap)
    XSetWindowBackground(blackbox->XDisplay(), geom_window,
                         texture.color().pixel(screen_info.screenNumber()));
  else
    XSetWindowBackgroundPixmap(blackbox->XDisplay(),
                               geom_window, geom_pixmap);

  XSetWindowBorderWidth(blackbox->XDisplay(), geom_window,
                        _resource.borderWidth());
  XSetWindowBorder(blackbox->XDisplay(), geom_window,
                   _resource.borderColor()->pixel(screen_info.screenNumber()));

  workspacemenu->reconfigure();
  iconmenu->reconfigure();

  InitMenu();
  raiseWindows((WindowStack*) 0);
  rootmenu->reconfigure();

  configmenu->reconfigure();

  toolbar->reconfigure();

  slit->reconfigure();

  std::for_each(workspacesList.begin(), workspacesList.end(),
                std::mem_fun(&Workspace::reconfigure));

  BlackboxWindowList::iterator iit = iconList.begin();
  for (; iit != iconList.end(); ++iit) {
    BlackboxWindow *bw = *iit;
    if (bw->validateClient())
      bw->reconfigure();
  }
}


void BScreen::rereadMenu(void) {
  InitMenu();
  raiseWindows((WindowStack*) 0);

  rootmenu->reconfigure();
}


void ScreenResource::loadRCFile(unsigned int screen, const std::string& rc) {
  XrmDatabase database = (XrmDatabase) 0;

  database = XrmGetFileDatabase(rc.c_str());

  XrmValue value;
  char *value_type, name_lookup[1024], class_lookup[1024];
  int int_value;

  // window settings and behavior
  sprintf(name_lookup,  "session.screen%d.placementIgnoresShaded", screen);
  sprintf(class_lookup, "Session.Screen%d.placementIgnoresShaded", screen);
  wconfig.ignore_shaded = True;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type, &value)
      && ! strncasecmp(value.addr, "false", value.size)) {
    wconfig.ignore_shaded = False;
  }

  sprintf(name_lookup,  "session.screen%d.fullMaximization", screen);
  sprintf(class_lookup, "Session.Screen%d.FullMaximization", screen);
  wconfig.full_max = False;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type, &value)
      && ! strncasecmp(value.addr, "true", value.size)) {
    wconfig.full_max = True;
  }

  sprintf(name_lookup,  "session.screen%d.focusNewWindows", screen);
  sprintf(class_lookup, "Session.Screen%d.FocusNewWindows", screen);
  wconfig.focus_new = False;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type, &value)
      && ! strncasecmp(value.addr, "true", value.size)) {
    wconfig.focus_new = True;
  }

  sprintf(name_lookup,  "session.screen%d.focusLastWindow", screen);
  sprintf(class_lookup, "Session.Screen%d.focusLastWindow", screen);
  wconfig.focus_last = False;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type, &value)
      && ! strncasecmp(value.addr, "true", value.size)) {
    wconfig.focus_last = True;
  }

  sprintf(name_lookup,  "session.screen%d.rowPlacementDirection", screen);
  sprintf(class_lookup, "Session.Screen%d.RowPlacementDirection", screen);
  wconfig.row_direction = LeftRight;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type, &value)
      && ! strncasecmp(value.addr, "righttoleft", value.size)) {
    wconfig.row_direction = RightLeft;
  }

  sprintf(name_lookup,  "session.screen%d.colPlacementDirection", screen);
  sprintf(class_lookup, "Session.Screen%d.ColPlacementDirection", screen);
  wconfig.col_direction = TopBottom;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type, &value)
      && ! strncasecmp(value.addr, "bottomtotop", value.size)) {
    wconfig.col_direction = BottomTop;
  }

  sprintf(name_lookup,  "session.screen%d.windowPlacement", screen);
  sprintf(class_lookup, "Session.Screen%d.WindowPlacement", screen);
  wconfig.placement_policy = RowSmartPlacement;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    if (! strncasecmp(value.addr, "ColSmartPlacement", value.size))
      wconfig.placement_policy = ColSmartPlacement;
    else if (! strncasecmp(value.addr, "CascadePlacement", value.size))
      wconfig.placement_policy = CascadePlacement;
  }

  sprintf(name_lookup,  "session.screen%d.focusModel", screen);
  sprintf(class_lookup, "Session.Screen%d.FocusModel", screen);
  wconfig.sloppy_focus = True;
  wconfig.auto_raise = False;
  wconfig.click_raise = False;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    std::string fmodel = value.addr;

    if (fmodel.find("ClickToFocus") != std::string::npos) {
      wconfig.sloppy_focus = False;
    } else {
      // must be sloppy

      if (fmodel.find("AutoRaise") != std::string::npos)
        wconfig.auto_raise = True;
      if (fmodel.find("ClickRaise") != std::string::npos)
        wconfig.click_raise = True;
    }
  }

  sprintf(name_lookup,  "session.screen%d.edgeSnapThreshold", screen);
  sprintf(class_lookup, "Session.Screen%d.EdgeSnapThreshold", screen);
  wconfig.edge_snap_threshold = 0;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      sscanf(value.addr, "%d", &int_value) == 1) {
    wconfig.edge_snap_threshold = int_value;
  }

  wconfig.opaque_move = False;
  if (XrmGetResource(database, "session.opaqueMove", "Session.OpaqueMove",
                     &value_type, &value) &&
      ! strncasecmp("true", value.addr, value.size)) {
  wconfig.opaque_move = True;
  }

  // toolbar settings
  sprintf(name_lookup,  "session.screen%d.toolbar.widthPercent", screen);
  sprintf(class_lookup, "Session.Screen%d.Toolbar.WidthPercent", screen);
  tconfig.width_percent = 66;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      sscanf(value.addr, "%d", &int_value) == 1 &&
      int_value > 0 && int_value <= 100) {
    tconfig.width_percent = int_value;
  }

  sprintf(name_lookup, "session.screen%d.toolbar.placement", screen);
  sprintf(class_lookup, "Session.Screen%d.Toolbar.Placement", screen);
  tconfig.placement = Toolbar::BottomCenter;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    if (! strncasecmp(value.addr, "TopLeft", value.size))
      tconfig.placement = Toolbar::TopLeft;
    else if (! strncasecmp(value.addr, "BottomLeft", value.size))
      tconfig.placement = Toolbar::BottomLeft;
    else if (! strncasecmp(value.addr, "TopCenter", value.size))
      tconfig.placement = Toolbar::TopCenter;
    else if (! strncasecmp(value.addr, "TopRight", value.size))
      tconfig.placement = Toolbar::TopRight;
    else if (! strncasecmp(value.addr, "BottomRight", value.size))
      tconfig.placement = Toolbar::BottomRight;
  }

  sprintf(name_lookup,  "session.screen%d.toolbar.onTop", screen);
  sprintf(class_lookup, "Session.Screen%d.Toolbar.OnTop", screen);
  tconfig.on_top = False;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "true", value.size)) {
    tconfig.on_top = True;
  }

  sprintf(name_lookup,  "session.screen%d.toolbar.autoHide", screen);
  sprintf(class_lookup, "Session.Screen%d.Toolbar.autoHide", screen);
  tconfig.auto_hide = False;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "true", value.size)) {
    tconfig.auto_hide = True;
  }

  sprintf(name_lookup,  "session.screen%d.strftimeFormat", screen);
  sprintf(class_lookup, "Session.Screen%d.StrftimeFormat", screen);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    tconfig.strftime_format = value.addr;
  } else {
    tconfig.strftime_format = "%I:%M %p";
  }

  // slit settings
  sprintf(name_lookup, "session.screen%d.slit.placement", screen);
  sprintf(class_lookup, "Session.Screen%d.Slit.Placement", screen);
  sconfig.placement = Slit::CenterRight;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    if (! strncasecmp(value.addr, "TopLeft", value.size))
      sconfig.placement = Slit::TopLeft;
    else if (! strncasecmp(value.addr, "CenterLeft", value.size))
      sconfig.placement = Slit::CenterLeft;
    else if (! strncasecmp(value.addr, "BottomLeft", value.size))
      sconfig.placement = Slit::BottomLeft;
    else if (! strncasecmp(value.addr, "TopCenter", value.size))
      sconfig.placement = Slit::TopCenter;
    else if (! strncasecmp(value.addr, "BottomCenter", value.size))
      sconfig.placement = Slit::BottomCenter;
    else if (! strncasecmp(value.addr, "TopRight", value.size))
      sconfig.placement = Slit::TopRight;
    else if (! strncasecmp(value.addr, "BottomRight", value.size))
      sconfig.placement = Slit::BottomRight;
  }

  sprintf(name_lookup, "session.screen%d.slit.direction", screen);
  sprintf(class_lookup, "Session.Screen%d.Slit.Direction", screen);
  sconfig.direction = Slit::Vertical;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "Horizontal", value.size)) {
    sconfig.direction = Slit::Horizontal;
  }

  sprintf(name_lookup, "session.screen%d.slit.onTop", screen);
  sprintf(class_lookup, "Session.Screen%d.Slit.OnTop", screen);
  sconfig.on_top = False;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "True", value.size)) {
    sconfig.on_top = True;
  }

  sprintf(name_lookup, "session.screen%d.slit.autoHide", screen);
  sprintf(class_lookup, "Session.Screen%d.Slit.AutoHide", screen);
  sconfig.auto_hide = False;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "true", value.size)) {
    sconfig.auto_hide = True;
  }

  // general screen settings
  sprintf(name_lookup,  "session.screen%d.disableBindingsWithScrollLock",
          screen);
  sprintf(class_lookup, "Session.Screen%d.disableBindingsWithScrollLock",
          screen);
  allow_scroll_lock = False;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value) &&
      ! strncasecmp(value.addr, "true", value.size)) {
    allow_scroll_lock = True;
  }

  sprintf(name_lookup,  "session.screen%d.workspaces", screen);
  sprintf(class_lookup, "Session.Screen%d.Workspaces", screen);
  workspace_count = 1;
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type, &value)
      && sscanf(value.addr, "%d", &int_value) == 1
      && int_value > 0 && int_value < 128) {
    workspace_count = int_value;
  }

  sprintf(name_lookup,  "session.screen%d.workspaceNames", screen);
  sprintf(class_lookup, "Session.Screen%d.WorkspaceNames", screen);
  workspaces.clear();
  workspaces.reserve(workspace_count);
  if (XrmGetResource(database, name_lookup, class_lookup, &value_type,
                     &value)) {
    std::string search = value.addr;
    std::string::const_iterator it = search.begin(), end = search.end();
    while (1) {
      std::string::const_iterator tmp = it; // current string.begin()
      it = std::find(tmp, end, ',');   // look for comma between tmp and end
      workspaces.push_back(std::string(tmp, it)); // string = search[tmp:it]
      if (it == end) break;
      ++it;
    }
  }

  dither_mode = bt::OrderedDither;
  if (XrmGetResource(database, "session.imageDither", "Session.ImageDither",
                     &value_type, &value)) {
    if (! strncasecmp("ordereddither", value.addr, value.size))
      dither_mode = bt::OrderedDither;
    else if (! strncasecmp("floydsteinbergdither", value.addr, value.size))
      dither_mode = bt::FloydSteinbergDither;
    else if (! strncasecmp("nodither", value.addr, value.size))
      dither_mode = bt::NoDither;
  }

  XrmDestroyDatabase(database);
}


void ScreenResource::loadStyle(BScreen* screen, const std::string& style) {
  const bt::Display &display = screen->getBlackbox()->display();
  unsigned int screen_num = screen->screenNumber();

  // use the user selected style
  bt::Resource res(style);
  if (! res.valid())
    res.load(DEFAULTSTYLE);

  // load bevel and border widths
  std::string wstr = res.read("borderWidth", "BorderWidth", "1");
  border_width = strtoul(wstr.c_str(), 0, 0);
  wstr = res.read("bevelWidth", "BevelWidth", "3");
  bevel_width = strtoul(wstr.c_str(), 0, 0);

  // load menu style
  bt::MenuStyle::get(*screen->getBlackbox(), screen_num)->load(res);

  // load fonts
  wstyle.font.setFontName(res.read("window.font", "Window.Font", "fixed"));
  tstyle.font.setFontName(res.read("toolbar.font", "Toolbar.Font", "fixed"));

  // load window config
  wstyle.t_focus =
    bt::textureResource(display, screen_num, res,
                        "window.title.focus", "Window.Title.Focus",
                        "white");
  wstyle.t_unfocus =
    bt::textureResource(display, screen_num, res,
                        "window.title.unfocus", "Window.Title.Unfocus",
                        "black");
  wstyle.l_focus =
    bt::textureResource(display, screen_num, res,
                        "window.label.focus", "Window.Label.Focus",
                        "white");
  wstyle.l_unfocus =
    bt::textureResource(display, screen_num, res,
                        "window.label.unfocus", "Window.Label.Unfocus",
                        "black");
  wstyle.h_focus =
    bt::textureResource(display, screen_num, res,
                        "window.handle.focus", "Window.Handle.Focus",
                        "white");
  wstyle.h_unfocus =
    bt::textureResource(display, screen_num, res,
                        "window.handle.unfocus", "Window.Handle.Unfocus",
                        "black");

  wstr = res.read("window.handleHeight", "Window.HandleHeight", "6");

  wstyle.handle_height =
    static_cast<unsigned int>(strtoul(wstr.c_str(), 0, 0));

  wstyle.bevel_width = bevel_width;

  // the height of the titlebar is based upon the height of the font being
  // used to display the window's title
  wstyle.label_height = bt::textHeight(wstyle.font) + 2;
  wstyle.title_height = wstyle.label_height + wstyle.bevel_width * 2;
  wstyle.button_width = wstyle.label_height - 2;
  wstyle.grip_width = wstyle.button_width * 2;

  wstyle.g_focus = bt::textureResource(display, screen_num, res,
                                       "window.grip.focus",
                                       "Window.Grip.Focus",
                                       "white");
  wstyle.g_unfocus = bt::textureResource(display, screen_num, res,
                                         "window.grip.unfocus",
                                         "Window.Grip.Unfocus",
                                         "black");
  wstyle.b_focus =
    bt::textureResource(display, screen_num, res,
                        "window.button.focus", "Window.Button.Focus",
                        "white");
  wstyle.b_unfocus =
    bt::textureResource(display, screen_num, res,
                        "window.button.unfocus", "Window.Button.Unfocus",
                        "black");
  wstyle.b_pressed =
    bt::textureResource(display, screen_num, res,
                        "window.button.pressed", "Window.Button.Pressed",
                        "black");

  // we create the window.frame texture by hand because it exists only to
  // make the code cleaner and is not actually used for display
  bt::Color color;
  color = bt::Color::namedColor(display, screen_num,
                                res.read("window.frame.focusColor",
                                         "Window.Frame.FocusColor",
                                         "white"));
  wstyle.f_focus.setDescription("solid flat");
  wstyle.f_focus.setColor(color);

  color = bt::Color::namedColor(display, screen_num,
                                res.read("window.frame.unfocusColor",
                                         "Window.Frame.UnfocusColor",
                                         "white"));
  wstyle.f_unfocus.setDescription("solid flat");
  wstyle.f_unfocus.setColor(color);

  wstr = res.read("window.frameWidth", "Window.FrameWidth",
                  bt::itostring(bevel_width));
  wstyle.frame_width = static_cast<unsigned int>(strtoul(wstr.c_str(), 0, 0));

  wstyle.l_text_focus =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.label.focus.textColor",
                                   "Window.Label.Focus.TextColor",
                                   "black"));
  wstyle.l_text_unfocus =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.label.unfocus.textColor",
                                   "Window.Label.Unfocus.TextColor",
                                   "white"));
  wstyle.b_pic_focus =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.button.focus.picColor",
                                   "Window.Button.Focus.PicColor",
                                   "black"));
  wstyle.b_pic_unfocus =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.button.unfocus.picColor",
                                   "Window.Button.Unfocus.PicColor",
                                   "white"));

  wstyle.alignment =
    bt::alignResource(res, "window.alignment", "Window.Alignment");

  // load toolbar config
  tstyle.toolbar = bt::textureResource(display, screen_num, res,
                                       "toolbar", "Toolbar",
                                       "black");
  tstyle.label = bt::textureResource(display, screen_num, res,
                                     "toolbar.label", "Toolbar.Label",
                                     "black");
  tstyle.window =
    bt::textureResource(display, screen_num, res,
                        "toolbar.windowLabel", "Toolbar.WindowLabel",
                        "black");
  tstyle.button =
    bt::textureResource(display, screen_num, res,
                        "toolbar.button", "Toolbar.Button",
                        "white");
  tstyle.pressed =
    bt::textureResource(display, screen_num, res,
                        "toolbar.button.pressed", "Toolbar.Button.Pressed",
                        "black");
  tstyle.clock =
    bt::textureResource(display, screen_num, res,
                        "toolbar.clock", "Toolbar.Clock", "black");

  tstyle.l_text =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.label.textColor",
                                   "Toolbar.Label.TextColor",
                                   "white"));
  tstyle.w_text =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.windowLabel.textColor",
                                   "Toolbar.WindowLabel.TextColor",
                                   "white"));
  tstyle.c_text =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.clock.textColor",
                                   "Toolbar.Clock.TextColor",
                                   "white"));
  tstyle.b_pic =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.button.picColor",
                                   "Toolbar.Button.PicColor",
                                   "black"));
  tstyle.alignment =
    bt::alignResource(res, "toolbar.alignment", "Toolbar.Alignment");

  border_color = bt::Color::namedColor(display, screen_num,
                                       res.read("borderColor",
                                                "BorderColor",
                                                "black"));

  root_command = res.read("rootCommand", "RootCommand");

  // sanity checks
  if (wstyle.t_focus.texture() == bt::Texture::Parent_Relative)
    wstyle.t_focus = wstyle.f_focus;
  if (wstyle.t_unfocus.texture() == bt::Texture::Parent_Relative)
    wstyle.t_unfocus = wstyle.f_unfocus;
  if (wstyle.h_focus.texture() == bt::Texture::Parent_Relative)
    wstyle.h_focus = wstyle.f_focus;
  if (wstyle.h_unfocus.texture() == bt::Texture::Parent_Relative)
    wstyle.h_unfocus = wstyle.f_unfocus;

  if (tstyle.toolbar.texture() == bt::Texture::Parent_Relative) {
    tstyle.toolbar.setTexture(bt::Texture::Flat | bt::Texture::Solid);
    tstyle.toolbar.setColor(bt::Color(0, 0, 0));
  }
}


void BScreen::LoadStyle(void) {
  _resource.loadStyle(this, blackbox->getStyleFilename());

  if (! _resource.rootCommand().empty())
    bt::bexec(_resource.rootCommand(), screen_info.displayString());
}


void BScreen::iconifyWindow(BlackboxWindow *w) {
  assert(w != 0);

  if (w->getWorkspaceNumber() != bt::BSENTINEL) {
    Workspace* wkspc = getWorkspace(w->getWorkspaceNumber());
    wkspc->removeWindow(w);
    w->setWorkspace(bt::BSENTINEL);
  }

  int id = iconmenu->insertItem(bt::ellideText(w->getIconTitle(), 60, "..."));
  w->setWindowNumber(id);
  iconList.push_back(w);
}


void BScreen::removeIcon(BlackboxWindow *w) {
  assert(w != 0);
  iconList.remove(w);
  iconmenu->removeItem(w->getWindowNumber());
}


BlackboxWindow *BScreen::getIcon(unsigned int index) {
  if (index < iconList.size()) {
    BlackboxWindowList::iterator it = iconList.begin();
    std::advance<BlackboxWindowList::iterator,signed>(it, index);
    return *it;
  }

  return (BlackboxWindow *) 0;
}


unsigned int BScreen::addWorkspace(void) {
  Workspace *wkspc = new Workspace(this, workspacesList.size());
  workspacesList.push_back(wkspc);

  workspacemenu->insertItem(wkspc->getName(), wkspc->getMenu(),
                            wkspc->getID(), workspacemenu->count() - 2);

  toolbar->reconfigure();

  blackbox->netwm().setNumberOfDesktops(screen_info.rootWindow(),
                                         workspacesList.size());
  updateDesktopNamesHint();

  return workspacesList.size();
}


unsigned int BScreen::removeLastWorkspace(void) {
  if (workspacesList.size() == 1)
    return 1;

  Workspace *wkspc = workspacesList.back();
  workspacesList.pop_back();

  if (current_workspace->getID() == wkspc->getID())
    changeWorkspaceID(current_workspace->getID() - 1);

  wkspc->transferWindows(*(workspacesList.back()));

  workspacemenu->removeItem(wkspc->getID());

  delete wkspc;

  toolbar->reconfigure();

  blackbox->netwm().setNumberOfDesktops(screen_info.rootWindow(),
                                         workspacesList.size());
  updateDesktopNamesHint();

  return workspacesList.size();
}


void BScreen::changeWorkspaceID(unsigned int id) {
  if (! current_workspace || id == current_workspace->getID()) return;

  current_workspace->hide();

  workspacemenu->setItemChecked(current_workspace->getID(), false);

  current_workspace = getWorkspace(id);
  current_workspace_id = current_workspace->getID();

  current_workspace->show();

  workspacemenu->setItemChecked(current_workspace->getID(), true);
  toolbar->redrawWorkspaceLabel();

  blackbox->netwm().setCurrentDesktop(screen_info.rootWindow(),
                                       current_workspace->getID());
}


void BScreen::addWindow(Window w) {
  manageWindow(w);
  BlackboxWindow *win = blackbox->findWindow(w);
  if (win)
    updateClientListHint();
}


void BScreen::manageWindow(Window w) {
  XWMHints *wmhint = XGetWMHints(blackbox->XDisplay(), w);
  if (wmhint && (wmhint->flags & StateHint) &&
      wmhint->initial_state == WithdrawnState) {
    slit->addClient(w);
    return;
  }

  new BlackboxWindow(blackbox, w, this);

  BlackboxWindow *win = blackbox->findWindow(w);
  if (! win)
    return;

  Workspace* wkspc = (win->getWorkspaceNumber() > getWorkspaceCount()) ?
    current_workspace : getWorkspace(win->getWorkspaceNumber());

  bool place_window = True;
  if (blackbox->startingUp() ||
      ((win->isTransient() || win->normalHintFlags() & (PPosition|USPosition))
       && win->clientRect().intersects(screen_info.rect())))
    place_window = False;

  wkspc->addWindow(win, place_window);
  windowList.push_back(win);

  XMapRequestEvent mre;
  mre.window = w;
  win->mapRequestEvent(&mre);
}


void BScreen::releaseWindow(BlackboxWindow *w, bool remap) {
  unmanageWindow(w, remap);
  updateClientListHint();
}


void BScreen::unmanageWindow(BlackboxWindow *w, bool remap) {
  w->restore(remap);

  if (w->isModal()) w->setModal(False);

  if (w->getWorkspaceNumber() != bt::BSENTINEL &&
      w->getWindowNumber() != bt::BSENTINEL)
    getWorkspace(w->getWorkspaceNumber())->removeWindow(w);
  else if (w->isIconic())
    removeIcon(w);

  windowList.remove(w);

  if (blackbox->getFocusedWindow() == w)
    blackbox->setFocusedWindow((BlackboxWindow *) 0);

  /*
    some managed windows can also be window group controllers.  when
    unmanaging such windows, we should also delete the window group.
  */
  BWindowGroup *group = blackbox->findWindowGroup(w->getClientWindow());
  delete group;

  delete w;
}


void BScreen::raiseWindow(BlackboxWindow *w) {
  Workspace *wkspc = getWorkspace(w->getWorkspaceNumber());
  wkspc->raiseWindow(w);
}


void BScreen::lowerWindow(BlackboxWindow *w) {
  Workspace *wkspc = getWorkspace(w->getWorkspaceNumber());
  wkspc->lowerWindow(w);
}


void
BScreen::raiseWindows(const bt::Netwm::WindowList* const workspace_stack) {
  // the 8 represents the number of blackbox windows such as menus
  const unsigned int workspace_stack_size =
    (workspace_stack) ? workspace_stack->size() : 0;
  std::vector<Window> session_stack(workspace_stack_size + 2);
  std::back_insert_iterator<std::vector<Window> > it(session_stack);

  if (toolbar->isOnTop())
    *(it++) = toolbar->getWindowID();

  if (slit->isOnTop())
    *(it++) = slit->getWindowID();

  if (workspace_stack_size)
    std::copy(workspace_stack->rbegin(), workspace_stack->rend(), it);

  if (! session_stack.empty()) {
    XRaiseWindow(blackbox->XDisplay(), session_stack[0]);
    XRestackWindows(blackbox->XDisplay(), &session_stack[0],
                    session_stack.size());
  }

  updateClientListStackingHint();
}


/*
 * I would love to kill this function and the accompanying workspaceNames
 * list.  However, we have a chicken and egg situation.  The names are read
 * in during load_rc() which happens before the workspaces are created.
 * The current solution is to read the names into a list, then use the list
 * later for constructing the workspaces.  It is only used during initial
 * BScreen creation.
 */
const std::string BScreen::getNameOfWorkspace(unsigned int id) {
  if (id < _resource.numberOfWorkspaces())
    return _resource.workspaceName(id);
  return std::string("");
}


void BScreen::reassociateWindow(BlackboxWindow *w, unsigned int wkspc_id) {
  if (! w) return;

  if (wkspc_id == bt::BSENTINEL)
    wkspc_id = current_workspace->getID();

  if (w->getWorkspaceNumber() == wkspc_id)
    return;

  if (w->isIconic()) {
    removeIcon(w);
    getWorkspace(wkspc_id)->addWindow(w);
  } else {
    getWorkspace(w->getWorkspaceNumber())->removeWindow(w);
    getWorkspace(wkspc_id)->addWindow(w);
  }
}


void BScreen::propagateWindowName(const BlackboxWindow *w) {
  if (! w->isIconic()) {
    Clientmenu *clientmenu = getWorkspace(w->getWorkspaceNumber())->getMenu();
    clientmenu->changeItem(w->getWindowNumber(),
                           bt::ellideText(w->getTitle(), 60, "..."));

    if (blackbox->getFocusedWindow() == w)
      toolbar->redrawWindowLabel();
  } else {
    iconmenu->changeItem(w->getWindowNumber(), w->getIconTitle());
  }
}


void BScreen::nextFocus(void) const {
  BlackboxWindow *focused = blackbox->getFocusedWindow(),
    *next = focused;

  if (focused &&
      focused->getScreen()->screen_info.screenNumber() ==
      screen_info.screenNumber() &&
      current_workspace->getCount() > 1) {
    do {
      next = current_workspace->getNextWindowInList(next);
    } while(next != focused && ! next->setInputFocus());

    if (next != focused)
      current_workspace->raiseWindow(next);
  } else if (current_workspace->getCount() > 0) {
    next = current_workspace->getTopWindowOnStack();

    next->setInputFocus();
    current_workspace->raiseWindow(next);
  }
}


void BScreen::prevFocus(void) const {
  BlackboxWindow *focused = blackbox->getFocusedWindow(),
    *next = focused;

  if (focused &&
      focused->getScreen()->screen_info.screenNumber() ==
      screen_info.screenNumber() &&
      current_workspace->getCount() > 1) {
    do {
      next = current_workspace->getPrevWindowInList(next);
    } while(next != focused && ! next->setInputFocus());

    if (next != focused)
      current_workspace->raiseWindow(next);
  } else if (current_workspace->getCount() > 0) {
    next = current_workspace->getTopWindowOnStack();

    next->setInputFocus();
    current_workspace->raiseWindow(next);
  }
}


void BScreen::raiseFocus(void) const {
  BlackboxWindow *focused = blackbox->getFocusedWindow();
  if (! focused)
    return;

  // if on this Screen, raise it
  if (focused->getScreen()->screen_info.screenNumber() ==
      screen_info.screenNumber()) {
    Workspace *workspace = getWorkspace(focused->getWorkspaceNumber());
    workspace->raiseWindow(focused);
  }
}


void BScreen::InitMenu(void) {
  if (rootmenu) {
    rootmenu->clear();
  } else {
    rootmenu = new Rootmenu(*blackbox, screen_info.screenNumber(), this);
    rootmenu->showTitle();
  }
  bool defaultMenu = True;

  if (blackbox->getMenuFilename()) {
    FILE *menu_file = fopen(blackbox->getMenuFilename(), "r");

    if (!menu_file) {
      perror(blackbox->getMenuFilename());
    } else {
      if (feof(menu_file)) {
        fprintf(stderr, bt::i18n(ScreenSet, ScreenEmptyMenuFile,
                                 "%s: Empty menu file"),
                blackbox->getMenuFilename());
      } else {
        char line[1024], label[1024];
        memset(line, 0, 1024);
        memset(label, 0, 1024);

        while (fgets(line, 1024, menu_file) && ! feof(menu_file)) {
          if (line[0] == '#')
            continue;

          int i, key = 0, index = -1, len = strlen(line);

          for (i = 0; i < len; i++) {
            if (line[i] == '[') index = 0;
            else if (line[i] == ']') break;
            else if (line[i] != ' ')
              if (index++ >= 0)
                key += tolower(line[i]);
          }

          if (key == 517) { // [begin]
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
            defaultMenu = parseMenuFile(menu_file, rootmenu);
            break;
          }
        }
      }
      fclose(menu_file);
    }
  }

  if (defaultMenu) {
    rootmenu->setTitle(bt::i18n(BasemenuSet, BasemenuBlackboxMenu,
                                "Blackbox Menu"));

    rootmenu->insertFunction(bt::i18n(ScreenSet, Screenxterm, "xterm"),
                             BScreen::Execute,
                             bt::i18n(ScreenSet, Screenxterm, "xterm"));
    rootmenu->insertFunction(bt::i18n(ScreenSet, ScreenRestart, "Restart"),
                             BScreen::Restart);
    rootmenu->insertFunction(bt::i18n(ScreenSet, ScreenExit, "Exit"),
                             BScreen::Exit);
  } else {
    blackbox->saveMenuFilename(blackbox->getMenuFilename());
  }
}


static
size_t string_within(char begin, char end,
                     const char *input, size_t start_at, size_t length,
                     char *output) {
  bool parse = False;
  size_t index = 0;
  size_t i = start_at;
  for (; i < length; ++i) {
    if (input[i] == begin) {
      parse = True;
    } else if (input[i] == end) {
      break;
    } else if (parse) {
      if (input[i] == '\\' && i < length - 1) i++;
      output[index++] = input[i];
    }
  }

  if (parse)
    output[index] = '\0';
  else
    output[0] = '\0';

  return i;
}


bool BScreen::parseMenuFile(FILE *file, Rootmenu *menu) {
  char line[1024], keyword[1024], label[1024], command[1024];
  bool done = False;

  while (! (done || feof(file))) {
    memset(line, 0, 1024);
    memset(label, 0, 1024);
    memset(command, 0, 1024);

    if (! fgets(line, 1024, file))
      continue;

    if (line[0] == '#') // comment, skip it
      continue;

    size_t line_length = strlen(line);
    unsigned int key = 0;

    // get the keyword enclosed in []'s
    size_t pos = string_within('[', ']', line, 0, line_length, keyword);

    if (keyword[0] == '\0') {  // no keyword, no menu entry
      continue;
    } else {
      size_t len = strlen(keyword);
      for (size_t i = 0; i < len; ++i) {
        if (keyword[i] != ' ')
          key += tolower(keyword[i]);
      }
    }

    // get the label enclosed in ()'s
    pos = string_within('(', ')', line, pos, line_length, label);

    // get the command enclosed in {}'s
    pos = string_within('{', '}', line, pos, line_length, command);

    switch (key) {
    case 311: // end
      done = True;

      break;

    case 333: // nop
      if (! *label)
        label[0] = '\0';
      menu->insertItem(label);

      break;

    case 421: // exec
      if (! (*label && *command)) {
        fprintf(stderr, bt::i18n(ScreenSet, ScreenEXECError,
                                 "BScreen::parseMenuFile: [exec] error, "
                                 "no menu label and/or command defined\n"));
        continue;
      }

      menu->insertFunction(label, BScreen::Execute, command);
      break;

    case 442: // exit
      if (! *label) {
        fprintf(stderr, bt::i18n(ScreenSet, ScreenEXITError,
                                 "BScreen::parseMenuFile: [exit] error, "
                                 "no menu label defined\n"));
        continue;
      }

      menu->insertFunction(label, BScreen::Exit);
      break;

    case 561: { // style
      if (! (*label && *command)) {
        fprintf(stderr,
                bt::i18n(ScreenSet, ScreenSTYLEError,
                         "BScreen::parseMenuFile: [style] error, "
                         "no menu label and/or filename defined\n"));
        continue;
      }

      std::string style = bt::expandTilde(command);
      menu->insertFunction(label, BScreen::SetStyle, style.c_str());
      break;
    }

    case 630: // config
      if (! *label) {
        fprintf(stderr, bt::i18n(ScreenSet, ScreenCONFIGError,
                                 "BScreen::parseMenufile: [config] error, "
                                 "no label defined"));
        continue;
      }

      menu->insertItem(label, configmenu);
      break;

    case 740: { // include
      if (! *label) {
        fprintf(stderr, bt::i18n(ScreenSet, ScreenINCLUDEError,
                                 "BScreen::parseMenuFile: [include] error, "
                                 "no filename defined\n"));
        continue;
      }

      std::string newfile = bt::expandTilde(label);
      FILE *submenufile = fopen(newfile.c_str(), "r");

      if (! submenufile) {
        perror(newfile.c_str());
        continue;
      }

      struct stat buf;
      if (fstat(fileno(submenufile), &buf) ||
          ! S_ISREG(buf.st_mode)) {
        fprintf(stderr,
                bt::i18n(ScreenSet, ScreenINCLUDEErrorReg,
                         "BScreen::parseMenuFile: [include] error: "
                         "'%s' is not a regular file\n"), newfile.c_str());
        break;
      }

      if (! feof(submenufile)) {
        if (! parseMenuFile(submenufile, menu))
          blackbox->saveMenuFilename(newfile);

        fclose(submenufile);
      }
    }

      break;

    case 767: { // submenu
      if (! *label) {
        fprintf(stderr, bt::i18n(ScreenSet, ScreenSUBMENUError,
                                 "BScreen::parseMenuFile: [submenu] error, "
                                 "no menu label defined\n"));
        continue;
      }

      Rootmenu *submenu =
        new Rootmenu(*blackbox, screen_info.screenNumber(), this);
      submenu->showTitle();

      if (*command)
        submenu->setTitle(command);
      else
        submenu->setTitle(label);

      parseMenuFile(file, submenu);
      menu->insertItem(label, submenu);
    }

      break;

    case 773: { // restart
      if (! *label) {
        fprintf(stderr, bt::i18n(ScreenSet, ScreenRESTARTError,
                                 "BScreen::parseMenuFile: [restart] error, "
                                 "no menu label defined\n"));
        continue;
      }

      if (*command)
        menu->insertFunction(label, BScreen::RestartOther, command);
      else
        menu->insertFunction(label, BScreen::Restart);
      break;
    }

    case 845: { // reconfig
      if (! *label) {
        fprintf(stderr,
                bt::i18n(ScreenSet, ScreenRECONFIGError,
                         "BScreen::parseMenuFile: [reconfig] error, "
                         "no menu label defined\n"));
        continue;
      }

      menu->insertFunction(label, BScreen::Reconfigure);
      break;
    }

    case 995:    // stylesdir
    case 1113: { // stylesmenu
      bool newmenu = ((key == 1113) ? True : False);

      if (! *label || (! *command && newmenu)) {
        fprintf(stderr,
                bt::i18n(ScreenSet, ScreenSTYLESDIRError,
                         "BScreen::parseMenuFile: [stylesdir/stylesmenu]"
                         " error, no directory defined\n"));
        continue;
      }

      char *directory = ((newmenu) ? command : label);

      std::string stylesdir = bt::expandTilde(directory);

      struct stat statbuf;

      if (stat(stylesdir.c_str(), &statbuf) == -1) {
        fprintf(stderr,
                bt::i18n(ScreenSet, ScreenSTYLESDIRErrorNoExist,
                         "BScreen::parseMenuFile: [stylesdir/stylesmenu]"
                         " error, %s does not exist\n"), stylesdir.c_str());
        continue;
      }
      if (! S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr,
                bt::i18n(ScreenSet, ScreenSTYLESDIRErrorNotDir,
                         "BScreen::parseMenuFile:"
                         " [stylesdir/stylesmenu] error, %s is not a"
                         " directory\n"), stylesdir.c_str());
        continue;
      }

      Rootmenu *stylesmenu;

      if (newmenu) {
        stylesmenu =
          new Rootmenu(*blackbox, screen_info.screenNumber(),this);
        stylesmenu->showTitle();
      } else {
        stylesmenu = menu;
      }

      DIR *d = opendir(stylesdir.c_str());
      struct dirent *p;
      std::vector<std::string> ls;

      while((p = readdir(d)))
        ls.push_back(p->d_name);

      closedir(d);

      std::sort(ls.begin(), ls.end());

      std::vector<std::string>::iterator it = ls.begin(),
                                        end = ls.end();
      for (; it != end; ++it) {
        std::string& fname = *it;

        if (fname[0] == '.' || fname[fname.size()-1] == '~')
          continue;

        std::string style = stylesdir;
        style += '/';
        style += fname;

        if (! stat(style.c_str(), &statbuf) && S_ISREG(statbuf.st_mode)) {
          // convert 'This_Long_Name' to 'This Long Name'
          std::replace(fname.begin(), fname.end(), '_', ' ');

          stylesmenu->insertFunction(fname, BScreen::SetStyle, style);
        }
      }

      if (newmenu) {
        stylesmenu->setTitle(label);
        menu->insertItem(label, stylesmenu);
      }

      blackbox->saveMenuFilename(stylesdir);
    }
      break;

    case 1090: { // workspaces
      if (! *label) {
        fprintf(stderr,
                bt::i18n(ScreenSet, ScreenWORKSPACESError,
                         "BScreen:parseMenuFile: [workspaces] error, "
                         "no menu label defined\n"));
        continue;
      }

      menu->insertItem(label, workspacemenu);
      break;
    }
    } // switch
  }

  return (menu->count() == 0);
}


void BScreen::shutdown(void) {
  XSelectInput(blackbox->XDisplay(), screen_info.rootWindow(),
               NoEventMask);
  XSync(blackbox->XDisplay(), False);

  while(! windowList.empty())
    unmanageWindow(windowList.back(), True);

  slit->shutdown();
}


void BScreen::showPosition(int x, int y) {
  if (! geom_visible) {
    XMoveResizeWindow(blackbox->XDisplay(), geom_window,
                      (screen_info.width() - geom_w) / 2,
                      (screen_info.height() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(blackbox->XDisplay(), geom_window);
    XRaiseWindow(blackbox->XDisplay(), geom_window);

    geom_visible = True;
  }

  char label[1024];
  sprintf(label, bt::i18n(ScreenSet, ScreenPositionFormat,
                      "X: %4d x Y: %4d"), x, y);

  XClearWindow(blackbox->XDisplay(), geom_window);

  bt::Pen pen(screen_info.screenNumber(), _resource.windowStyle()->l_text_focus);
  bt::Rect rect(_resource.bevelWidth(), _resource.bevelWidth(),
                geom_w - (_resource.bevelWidth() * 2),
                geom_h - (_resource.bevelWidth() * 2));
  bt::drawText(_resource.windowStyle()->font, pen, geom_window, rect,
               _resource.windowStyle()->alignment, label);
}


void BScreen::showGeometry(unsigned int gx, unsigned int gy) {
  if (! geom_visible) {
    XMoveResizeWindow(blackbox->XDisplay(), geom_window,
                      (screen_info.width() - geom_w) / 2,
                      (screen_info.height() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(blackbox->XDisplay(), geom_window);
    XRaiseWindow(blackbox->XDisplay(), geom_window);

    geom_visible = True;
  }

  char label[1024];

  sprintf(label, bt::i18n(ScreenSet, ScreenGeometryFormat,
                          "W: %4d x H: %4d"), gx, gy);

  XClearWindow(blackbox->XDisplay(), geom_window);

  bt::Pen pen(screen_info.screenNumber(),
              _resource.windowStyle()->l_text_focus);
  bt::Rect rect(_resource.bevelWidth(), _resource.bevelWidth(),
                geom_w - (_resource.bevelWidth() * 2),
                geom_h - (_resource.bevelWidth() * 2));
  bt::drawText(_resource.windowStyle()->font, pen, geom_window, rect,
               _resource.windowStyle()->alignment, label);
}


void BScreen::hideGeometry(void) {
  if (geom_visible) {
    XUnmapWindow(blackbox->XDisplay(), geom_window);
    geom_visible = False;
  }
}


void BScreen::addStrut(bt::Netwm::Strut *strut) {
  strutList.push_back(strut);
  updateAvailableArea();
}


void BScreen::removeStrut(bt::Netwm::Strut *strut) {
  strutList.remove(strut);
  updateAvailableArea();
}


void BScreen::updateStrut(void) {
  updateAvailableArea();
}


const bt::Rect& BScreen::availableArea(void) {
  if (_resource.doFullMax())
    return screen_info.rect(); // return the full screen
  return usableArea;
}


void BScreen::updateAvailableArea(void) {
  bt::Rect new_area;

  /* these values represent offsets from the screen edge
   * we look for the biggest offset on each edge and then apply them
   * all at once
   * do not be confused by the similarity to the names of Rect's members
   */
  bt::Netwm::Strut current;

  StrutList::const_iterator sit = strutList.begin(), send = strutList.end();

  for(; sit != send; ++sit) {
    const bt::Netwm::Strut* const strut = *sit;
    if (strut->left > current.left)
      current.left = strut->left;
    if (strut->top > current.top)
      current.top = strut->top;
    if (strut->right > current.right)
      current.right = strut->right;
    if (strut->bottom > current.bottom)
      current.bottom = strut->bottom;
  }

  new_area.setPos(current.left, current.top);
  new_area.setSize(screen_info.width() - (current.left + current.right),
                   screen_info.height() - (current.top + current.bottom));

  if (new_area != usableArea) {
    usableArea = new_area;
    BlackboxWindowList::iterator wit = windowList.begin(),
                                wend = windowList.end();
    for (; wit != wend; ++wit)
      if ((*wit)->isMaximized()) (*wit)->remaximize();

    updateWorkareaHint();
  }
}


Workspace* BScreen::getWorkspace(unsigned int index) const {
  assert(index < workspacesList.size());
  return workspacesList[index];
}


const std::string& BScreen::getWorkspaceName(unsigned int index) const {
  return getWorkspace(index)->getName();
}


void BScreen::clientMessageEvent(const XClientMessageEvent * const event) {
  if (event->format != 32) return;

  if (event->message_type == blackbox->netwm().numberOfDesktops()) {
    unsigned int number = event->data.l[0];
    unsigned int wkspc_count = getWorkspaceCount();
    if (number > wkspc_count) {
      for (; number != wkspc_count; --number)
        addWorkspace();
    } else if (number < wkspc_count) {
      for (; number != wkspc_count; ++number)
        removeLastWorkspace();
    }
  } else if (event->message_type == blackbox->netwm().desktopNames()) {
    getDesktopNames();
  } else if (event->message_type == blackbox->netwm().currentDesktop()) {
    unsigned int workspace = event->data.l[0];
    if (workspace < getWorkspaceCount() &&
        workspace != getCurrentWorkspaceID())
      changeWorkspaceID(workspace);
  }
}


void BScreen::buttonPressEvent(const XButtonEvent * const event) {
  if (event->button == 1) {
#warning "TODO: install root colormap"

    /*
      set this screen active.  keygrabs and input focus will stay on
      this screen until the user focuses a window on another screen or
      makes another screen active.
    */
    blackbox->setActiveScreen(this);
  } else if (event->button == 2) {
    workspacemenu->popup(event->x_root, event->y_root);
  } else if (event->button == 3) {
    blackbox->checkMenu();
    rootmenu->popup(event->x_root, event->y_root);
  }
}


void BScreen::configureRequestEvent(const XConfigureRequestEvent* const event)
{
  /*
    handle configure requests for windows that have no EventHandlers
    by simply configuring them as requested.

    note: the event->window parameter points to the window being
    configured, and event->parent points to the window that received
    the event (in this case, the root window, since
    SubstructureRedirect has been selected).
  */
  XWindowChanges xwc;
  xwc.x = event->x;
  xwc.y = event->y;
  xwc.width = event->width;
  xwc.height = event->height;
  xwc.border_width = event->border_width;
  xwc.sibling = event->above;
  xwc.stack_mode = event->detail;

  XConfigureWindow(blackbox->XDisplay(), event->window,
                   event->value_mask, &xwc);
}


void BScreen::toggleFocusModel(FocusModel model) {
  std::for_each(windowList.begin(), windowList.end(),
                std::mem_fun(&BlackboxWindow::ungrabButtons));

  if (model == SloppyFocus) {
    _resource.saveSloppyFocus(True);
  } else {
    _resource.saveSloppyFocus(False);
    _resource.saveAutoRaise(False);
    _resource.saveClickRaise(False);
  }

  std::for_each(windowList.begin(), windowList.end(),
                std::mem_fun(&BlackboxWindow::grabButtons));
}


void BScreen::updateWorkareaHint(void) const {
  unsigned int wkspc_count = workspacesList.size();
  unsigned long *workarea, *tmp;

  tmp = workarea = new unsigned long[wkspc_count * 4];

  for (unsigned int i = 0; i < wkspc_count; ++i) {
    tmp[0] = usableArea.x();
    tmp[1] = usableArea.y();
    tmp[2] = usableArea.width();
    tmp[3] = usableArea.height();
    tmp += 4;
  }

  blackbox->netwm().setWorkarea(screen_info.rootWindow(),
                                 workarea, wkspc_count);

  delete [] workarea;
}


void BScreen::updateDesktopNamesHint(void) const {
  std::string names;

  WorkspaceList::const_iterator it = workspacesList.begin(),
    end = workspacesList.end();
  for (; it != end; ++it)
    names += (*it)->getName() + '\0';

  blackbox->netwm().setDesktopNames(screen_info.rootWindow(), names);
}


void BScreen::updateClientListHint(void) const {
  if (windowList.empty()) {
    blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                      blackbox->netwm().clientList());
    return;
  }

  bt::Netwm::WindowList clientList(windowList.size());

  std::transform(windowList.begin(), windowList.end(), clientList.begin(),
                 std::mem_fun(&BlackboxWindow::getClientWindow));

  blackbox->netwm().setClientList(screen_info.rootWindow(), clientList);
}


void BScreen::updateClientListStackingHint(void) const {
  bt::Netwm::WindowList stack;

  WorkspaceList::const_iterator it = workspacesList.begin(),
    end = workspacesList.end();
  for (; it != end; ++it)
    (*it)->updateClientListStacking(stack);

  if (stack.empty()) {
    blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                      blackbox->netwm().clientListStacking());
    return;
  }

  blackbox->netwm().setClientListStacking(screen_info.rootWindow(), stack);
}


void BScreen::getDesktopNames(void) {
  bt::Netwm::UTF8StringList names;
  if(! blackbox->netwm().readDesktopNames(screen_info.rootWindow(), names))
    return;

  bt::Netwm::UTF8StringList::const_iterator it = names.begin(),
    end = names.end();
  WorkspaceList::iterator wit = workspacesList.begin(),
    wend = workspacesList.end();

  for (; wit != wend && it != end; ++wit, ++it) {
    if ((*wit)->getName() != *it)
      (*wit)->setName(*it);
  }

  if (names.size() < workspacesList.size())
    updateDesktopNamesHint();
}


BlackboxWindow* BScreen::getWindow(unsigned int workspace, unsigned int id) {
  return getWorkspace(workspace)->getWindow(id);
}


void BScreen::setWorkspaceName(unsigned int workspace,
                               const std::string& name) {
  getWorkspace(workspace)->setName(name);
}

