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
#include "GCCache.hh"
#include "Iconmenu.hh"
#include "Image.hh"
#include "Netwm.hh"
#include "Screen.hh"
#include "Slit.hh"
#include "Rootmenu.hh"
#include "Toolbar.hh"
#include "Util.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"

#ifndef   FONT_ELEMENT_SIZE
#define   FONT_ELEMENT_SIZE 50
#endif // FONT_ELEMENT_SIZE


static bool running = True;

static int anotherWMRunning(Display *display, XErrorEvent *) {
  fprintf(stderr, bt::i18n(ScreenSet, ScreenAnotherWMRunning,
          "BScreen::BScreen: an error occured while querying the X server.\n"
          "  another window manager already running on display %s.\n"),
          DisplayString(display));

  running = False;

  return(-1);
}


BScreen::BScreen(Blackbox *bb, unsigned int scrn) : ScreenInfo(bb, scrn) {
  blackbox = bb;

  event_mask = ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
               SubstructureRedirectMask | ButtonPressMask | ButtonReleaseMask;

  XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);
  XSelectInput(getDisplay()->getXDisplay(), getRootWindow(), event_mask);
  XSync(getDisplay()->getXDisplay(), False);
  XSetErrorHandler((XErrorHandler) old);

  managed = running;
  if (! managed) return;

  fprintf(stderr, bt::i18n(ScreenSet, ScreenManagingScreen,
                           "BScreen::BScreen: managing screen %d "
                           "using visual 0x%lx, depth %d\n"),
          getScreenNumber(), XVisualIDFromVisual(getVisual()),
          getDepth());

  blackbox->insertEventHandler(getRootWindow(), this);

  rootmenu = 0;
  resource.stylerc = 0;

  resource.mstyle.t_fontset = resource.mstyle.f_fontset =
    resource.tstyle.fontset = resource.wstyle.fontset = (XFontSet) 0;
  resource.mstyle.t_font = resource.mstyle.f_font = resource.tstyle.font =
    resource.wstyle.font = (XFontStruct *) 0;

  geom_pixmap = None;
  
  timer = new bt::Timer(blackbox, this);
  timer->setTimeout(750l); // once every 1.5 seconds
  timer->start();

  XDefineCursor(blackbox->getXDisplay(), getRootWindow(),
                blackbox->getSessionCursor());

  // start off full screen, top left.
  usableArea.setSize(getWidth(), getHeight());
  area_is_dirty = False;

  image_control =
    new bt::ImageControl(blackbox, this, True, blackbox->getColorsPerChannel(),
                         blackbox->getCacheLife(), blackbox->getCacheMax());
  image_control->installRootColormap();
  root_colormap_installed = True;

  blackbox->load_rc(this);

  image_control->setDither(resource.image_dither);

  LoadStyle();

  XGCValues gcv;
  unsigned long gc_value_mask = GCForeground;
  if (! bt::i18n.multibyte()) gc_value_mask |= GCFont;

  gcv.foreground = WhitePixel(blackbox->getXDisplay(), getScreenNumber())
                   ^ BlackPixel(blackbox->getXDisplay(), getScreenNumber());
  gcv.function = GXxor;
  gcv.subwindow_mode = IncludeInferiors;
  opGC = XCreateGC(blackbox->getXDisplay(), getRootWindow(),
                   GCForeground | GCFunction | GCSubwindowMode, &gcv);

  const char *s =  bt::i18n(ScreenSet, ScreenPositionLength,
                            "0: 0000 x 0: 0000");
  int l = strlen(s);

  if (bt::i18n.multibyte()) {
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

  XSetWindowAttributes setattrib;
  unsigned long mask = CWBorderPixel | CWColormap | CWSaveUnder;
  setattrib.border_pixel = getBorderColor()->pixel();
  setattrib.colormap = getColormap();
  setattrib.save_under = True;

  geom_window = XCreateWindow(blackbox->getXDisplay(), getRootWindow(),
                              0, 0, geom_w, geom_h, resource.border_width,
                              getDepth(), InputOutput, getVisual(),
                              mask, &setattrib);
  geom_visible = False;

  bt::Texture* texture = &(resource.wstyle.l_focus);
  geom_pixmap = texture->render(geom_w, geom_h, geom_pixmap);
  if (geom_pixmap == ParentRelative) {
    texture = &(resource.wstyle.t_focus);
    geom_pixmap = texture->render(geom_w, geom_h, geom_pixmap);
  }
  if (! geom_pixmap)
    XSetWindowBackground(blackbox->getXDisplay(), geom_window,
                         texture->color().pixel());
  else
    XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                               geom_window, geom_pixmap);

  workspacemenu = new Workspacemenu(this);
  iconmenu = new Iconmenu(this);
  configmenu = new Configmenu(this);

  Workspace *wkspc = (Workspace *) 0;
  if (resource.workspaces != 0) {
    for (unsigned int i = 0; i < resource.workspaces; ++i) {
      wkspc = new Workspace(this, workspacesList.size());
      workspacesList.push_back(wkspc);
      workspacemenu->insert(wkspc->getName(), wkspc->getMenu());
    }
  } else {
    wkspc = new Workspace(this, workspacesList.size());
    workspacesList.push_back(wkspc);
    workspacemenu->insert(wkspc->getName(), wkspc->getMenu());
  }

  workspacemenu->insert(bt::i18n(IconSet, IconIcons, "Icons"), iconmenu);
  workspacemenu->update();

  current_workspace = workspacesList.front();
  workspacemenu->setItemSelected(2, True);

  removeWorkspaceNames(); // do not need them any longer

  toolbar = new Toolbar(this);

  slit = new Slit(this);

  InitMenu();

  raiseWindows((WindowStack*) 0);
  rootmenu->update();

  changeWorkspaceID(0);

  const bt::Netwm* const netwm = blackbox->netwm();
  /*
    netwm requires the window manager to set a property on a window it creates
    which is the id of that window.  We must also set an equivalent property
    on the root window.  Then we must set _NET_WM_NAME on the child window
    to be the name of the wm.
  */
  netwm->setSupportingWMCheck(getRootWindow(), geom_window);
  netwm->setSupportingWMCheck(geom_window, geom_window);
  netwm->setWMName(geom_window, "Blackbox");

  netwm->setNumberOfDesktops(getRootWindow(), workspacesList.size());
  netwm->setDesktopGeometry(getRootWindow(), getWidth(), getHeight());
  netwm->setActiveWindow(getRootWindow(), None);
  updateWorkareaHint();
  updateDesktopNamesHint();

  Atom supported[46] = {
    netwm->clientList(),
    netwm->clientListStacking(),
    netwm->numberOfDesktops(),
    netwm->desktopGeometry(),
    netwm->currentDesktop(),
    netwm->desktopNames(),
    netwm->activeWindow(),
    netwm->workarea(),
    netwm->closeWindow(),
    netwm->moveresizeWindow(),
    netwm->wmName(),
    netwm->wmVisibleName(),
    netwm->wmIconName(),
    netwm->wmVisibleIconName(),
    netwm->wmDesktop(),
    netwm->wmWindowType(),
    netwm->wmWindowTypeDesktop(),
    netwm->wmWindowTypeDock(),
    netwm->wmWindowTypeToolbar(),
    netwm->wmWindowTypeMenu(),
    netwm->wmWindowTypeUtility(),
    netwm->wmWindowTypeSplash(),
    netwm->wmWindowTypeDialog(),
    netwm->wmWindowTypeNormal(),
    netwm->wmState(),
    netwm->wmStateModal(),
    /* sticky would go here, but we do not need it */
    netwm->wmStateMaximizedVert(),
    netwm->wmStateMaximizedHorz(),
    netwm->wmStateShaded(),
    netwm->wmStateSkipTaskbar(),
    netwm->wmStateSkipPager(),
    netwm->wmStateHidden(),
    netwm->wmStateFullscreen(),
    netwm->wmStateAbove(),
    netwm->wmStateBelow(),
    netwm->wmAllowedActions(),
    netwm->wmActionMove(),
    netwm->wmActionResize(),
    netwm->wmActionMinimize(),
    netwm->wmActionShade(),
    netwm->wmActionMaximizeHorz(),
    netwm->wmActionMaximizeVert(),
    netwm->wmActionFullscreen(),
    netwm->wmActionChangeDesktop(),
    netwm->wmActionClose(),
    netwm->wmStrut()
  };

  netwm->setSupported(getRootWindow(), supported, 46);

  unsigned int i, j, nchild;
  Window r, p, *children;
  XQueryTree(blackbox->getXDisplay(), getRootWindow(), &r, &p,
             &children, &nchild);

  // preen the window list of all icon windows... for better dockapp support
  for (i = 0; i < nchild; i++) {
    if (children[i] == None) continue;

    XWMHints *wmhints = XGetWMHints(blackbox->getXDisplay(),
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
    if (XGetWindowAttributes(blackbox->getXDisplay(), children[i], &attrib)) {
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

  blackbox->removeEventHandler(getRootWindow());

  if (geom_pixmap != None)
    image_control->removeImage(geom_pixmap);

  if (geom_window != None)
    XDestroyWindow(blackbox->getXDisplay(), geom_window);

  std::for_each(workspacesList.begin(), workspacesList.end(),
                bt::PointerAssassin());

  std::for_each(iconList.begin(), iconList.end(), bt::PointerAssassin());

  delete rootmenu;
  delete workspacemenu;
  delete iconmenu;
  delete configmenu;
  delete slit;
  delete toolbar;
  delete image_control;
  delete timer;

  blackbox->netwm()->removeProperty(getRootWindow(),
                                    blackbox->netwm()->supportingWMCheck());
  blackbox->netwm()->removeProperty(getRootWindow(),
                                    blackbox->netwm()->supported());
  blackbox->netwm()->removeProperty(getRootWindow(),
                                    blackbox->netwm()->numberOfDesktops());
  blackbox->netwm()->removeProperty(getRootWindow(),
                                    blackbox->netwm()->desktopGeometry());
  blackbox->netwm()->removeProperty(getRootWindow(),
                                    blackbox->netwm()->currentDesktop());
  blackbox->netwm()->removeProperty(getRootWindow(),
                                    blackbox->netwm()->activeWindow());
  blackbox->netwm()->removeProperty(getRootWindow(),
                                    blackbox->netwm()->workarea());

  if (resource.wstyle.fontset)
    XFreeFontSet(blackbox->getXDisplay(), resource.wstyle.fontset);
  if (resource.mstyle.t_fontset)
    XFreeFontSet(blackbox->getXDisplay(), resource.mstyle.t_fontset);
  if (resource.mstyle.f_fontset)
    XFreeFontSet(blackbox->getXDisplay(), resource.mstyle.f_fontset);
  if (resource.tstyle.fontset)
    XFreeFontSet(blackbox->getXDisplay(), resource.tstyle.fontset);

  if (resource.wstyle.font)
    XFreeFont(blackbox->getXDisplay(), resource.wstyle.font);
  if (resource.mstyle.t_font)
    XFreeFont(blackbox->getXDisplay(), resource.mstyle.t_font);
  if (resource.mstyle.f_font)
    XFreeFont(blackbox->getXDisplay(), resource.mstyle.f_font);
  if (resource.tstyle.font)
    XFreeFont(blackbox->getXDisplay(), resource.tstyle.font);

  XFreeGC(blackbox->getXDisplay(), opGC);
}


void BScreen::removeWorkspaceNames(void) {
  workspaceNames.clear();
}


void BScreen::reconfigure(void) {
  LoadStyle();

  XGCValues gcv;
  unsigned long gc_value_mask = GCForeground;
  if (! bt::i18n.multibyte()) gc_value_mask |= GCFont;

  gcv.foreground = WhitePixel(blackbox->getXDisplay(),
                              getScreenNumber());
  gcv.function = GXinvert;
  gcv.subwindow_mode = IncludeInferiors;
  XChangeGC(blackbox->getXDisplay(), opGC,
            GCForeground | GCFunction | GCSubwindowMode, &gcv);

  const char *s = bt::i18n(ScreenSet, ScreenPositionLength,
                       "0: 0000 x 0: 0000");
  int l = strlen(s);

  if (bt::i18n.multibyte()) {
    XRectangle ink, logical;
    XmbTextExtents(resource.wstyle.fontset, s, l, &ink, &logical);
    geom_w = logical.width;

    geom_h = resource.wstyle.fontset_extents->max_ink_extent.height;
  } else {
    geom_w = XTextWidth(resource.wstyle.font, s, l);

    geom_h = resource.wstyle.font->ascent + resource.wstyle.font->descent;
  }

  geom_w += (resource.bevel_width * 2);
  geom_h += (resource.bevel_width * 2);

  bt::Texture* texture = &(resource.wstyle.l_focus);
  geom_pixmap = texture->render(geom_w, geom_h, geom_pixmap);
  if (geom_pixmap == ParentRelative) {
    texture = &(resource.wstyle.t_focus);
    geom_pixmap = texture->render(geom_w, geom_h, geom_pixmap);
  }
  if (! geom_pixmap)
    XSetWindowBackground(blackbox->getXDisplay(), geom_window,
                         texture->color().pixel());
  else
    XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                               geom_window, geom_pixmap);

  XSetWindowBorderWidth(blackbox->getXDisplay(), geom_window,
                        resource.border_width);
  XSetWindowBorder(blackbox->getXDisplay(), geom_window,
                   resource.border_color.pixel());

  workspacemenu->reconfigure();
  iconmenu->reconfigure();

  int remember_sub = rootmenu->getCurrentSubmenu();

  InitMenu();
  raiseWindows((WindowStack*) 0);
  rootmenu->reconfigure();
  rootmenu->drawSubmenu(remember_sub);

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

  image_control->timeout();
}


void BScreen::rereadMenu(void) {
  InitMenu();
  raiseWindows((WindowStack*) 0);

  rootmenu->reconfigure();
}


void BScreen::LoadStyle(void) {
  resource.stylerc = XrmGetFileDatabase(blackbox->getStyleFilename());
  if (! resource.stylerc)
    resource.stylerc = XrmGetFileDatabase(DEFAULTSTYLE);

  XrmValue value;
  char *value_type;

  // load fonts/fontsets
  if (resource.wstyle.fontset)
    XFreeFontSet(blackbox->getXDisplay(), resource.wstyle.fontset);
  if (resource.tstyle.fontset)
    XFreeFontSet(blackbox->getXDisplay(), resource.tstyle.fontset);
  if (resource.mstyle.f_fontset)
    XFreeFontSet(blackbox->getXDisplay(), resource.mstyle.f_fontset);
  if (resource.mstyle.t_fontset)
    XFreeFontSet(blackbox->getXDisplay(), resource.mstyle.t_fontset);
  resource.wstyle.fontset = 0;
  resource.tstyle.fontset = 0;
  resource.mstyle.f_fontset = 0;
  resource.mstyle.t_fontset = 0;
  if (resource.wstyle.font)
    XFreeFont(blackbox->getXDisplay(), resource.wstyle.font);
  if (resource.tstyle.font)
    XFreeFont(blackbox->getXDisplay(), resource.tstyle.font);
  if (resource.mstyle.f_font)
    XFreeFont(blackbox->getXDisplay(), resource.mstyle.f_font);
  if (resource.mstyle.t_font)
    XFreeFont(blackbox->getXDisplay(), resource.mstyle.t_font);
  resource.wstyle.font = 0;
  resource.tstyle.font = 0;
  resource.mstyle.f_font = 0;
  resource.mstyle.t_font = 0;

  if (bt::i18n.multibyte()) {
    resource.wstyle.fontset =
      readDatabaseFontSet("window.font", "Window.Font");
    resource.tstyle.fontset =
      readDatabaseFontSet("toolbar.font", "Toolbar.Font");
    resource.mstyle.t_fontset =
      readDatabaseFontSet("menu.title.font", "Menu.Title.Font");
    resource.mstyle.f_fontset =
      readDatabaseFontSet("menu.frame.font", "Menu.Frame.Font");

    resource.mstyle.t_fontset_extents =
      XExtentsOfFontSet(resource.mstyle.t_fontset);
    resource.mstyle.f_fontset_extents =
      XExtentsOfFontSet(resource.mstyle.f_fontset);
    resource.tstyle.fontset_extents =
      XExtentsOfFontSet(resource.tstyle.fontset);
    resource.wstyle.fontset_extents =
      XExtentsOfFontSet(resource.wstyle.fontset);
  } else {
    resource.wstyle.font =
      readDatabaseFont("window.font", "Window.Font");
    resource.tstyle.font =
      readDatabaseFont("toolbar.font", "Toolbar.Font");
    resource.mstyle.t_font =
      readDatabaseFont("menu.title.font", "Menu.Title.Font");
    resource.mstyle.f_font =
      readDatabaseFont("menu.frame.font", "Menu.Frame.Font");
  }

  // load window config
  resource.wstyle.t_focus =
    readDatabaseTexture("window.title.focus", "Window.Title.Focus", "white");
  resource.wstyle.t_unfocus =
    readDatabaseTexture("window.title.unfocus",
                        "Window.Title.Unfocus", "black");
  resource.wstyle.l_focus =
    readDatabaseTexture("window.label.focus", "Window.Label.Focus", "white" );
  resource.wstyle.l_unfocus =
    readDatabaseTexture("window.label.unfocus", "Window.Label.Unfocus",
                        "black");
  resource.wstyle.h_focus =
    readDatabaseTexture("window.handle.focus", "Window.Handle.Focus", "white");
  resource.wstyle.h_unfocus =
    readDatabaseTexture("window.handle.unfocus",
                        "Window.Handle.Unfocus", "black");
  resource.wstyle.g_focus =
    readDatabaseTexture("window.grip.focus", "Window.Grip.Focus", "white");
  resource.wstyle.g_unfocus =
    readDatabaseTexture("window.grip.unfocus", "Window.Grip.Unfocus", "black");
  resource.wstyle.b_focus =
    readDatabaseTexture("window.button.focus", "Window.Button.Focus", "white");
  resource.wstyle.b_unfocus =
    readDatabaseTexture("window.button.unfocus",
                        "Window.Button.Unfocus", "black");
  resource.wstyle.b_pressed =
    readDatabaseTexture("window.button.pressed",
                        "Window.Button.Pressed", "black");

  // we create the window.frame texture by hand because it exists only to
  // make the code cleaner and is not actually used for display
  bt::Color color = readDatabaseColor("window.frame.focusColor",
                                   "Window.Frame.FocusColor", "white");
  resource.wstyle.f_focus = bt::Texture("solid flat", getDisplay(),
                                     getScreenNumber(), image_control);
  resource.wstyle.f_focus.setColor(color);

  color = readDatabaseColor("window.frame.unfocusColor",
                            "Window.Frame.UnfocusColor", "white");
  resource.wstyle.f_unfocus = bt::Texture("solid flat", getDisplay(),
                                       getScreenNumber(), image_control);
  resource.wstyle.f_unfocus.setColor(color);

  resource.wstyle.l_text_focus =
    readDatabaseColor("window.label.focus.textColor",
                      "Window.Label.Focus.TextColor", "black");
  resource.wstyle.l_text_unfocus =
    readDatabaseColor("window.label.unfocus.textColor",
                      "Window.Label.Unfocus.TextColor", "white");
  resource.wstyle.b_pic_focus =
    readDatabaseColor("window.button.focus.picColor",
                      "Window.Button.Focus.PicColor", "black");
  resource.wstyle.b_pic_unfocus =
    readDatabaseColor("window.button.unfocus.picColor",
                      "Window.Button.Unfocus.PicColor", "white");

  resource.wstyle.justify = LeftJustify;
  if (XrmGetResource(resource.stylerc, "window.justify", "Window.Justify",
                     &value_type, &value)) {
    if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
      resource.wstyle.justify = RightJustify;
    else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
      resource.wstyle.justify = CenterJustify;
  }

  // sanity checks
  if (resource.wstyle.t_focus.texture() == bt::Texture::Parent_Relative)
    resource.wstyle.t_focus = resource.wstyle.f_focus;
  if (resource.wstyle.t_unfocus.texture() == bt::Texture::Parent_Relative)
    resource.wstyle.t_unfocus = resource.wstyle.f_unfocus;
  if (resource.wstyle.h_focus.texture() == bt::Texture::Parent_Relative)
    resource.wstyle.h_focus = resource.wstyle.f_focus;
  if (resource.wstyle.h_unfocus.texture() == bt::Texture::Parent_Relative)
    resource.wstyle.h_unfocus = resource.wstyle.f_unfocus;

  // load toolbar config
  resource.tstyle.toolbar =
    readDatabaseTexture("toolbar", "Toolbar", "black");
  resource.tstyle.label =
    readDatabaseTexture("toolbar.label", "Toolbar.Label", "black");
  resource.tstyle.window =
    readDatabaseTexture("toolbar.windowLabel", "Toolbar.WindowLabel", "black");
  resource.tstyle.button =
    readDatabaseTexture("toolbar.button", "Toolbar.Button", "white");
  resource.tstyle.pressed =
    readDatabaseTexture("toolbar.button.pressed",
                        "Toolbar.Button.Pressed", "black");
  resource.tstyle.clock =
    readDatabaseTexture("toolbar.clock", "Toolbar.Clock", "black");
  resource.tstyle.l_text =
    readDatabaseColor("toolbar.label.textColor",
                      "Toolbar.Label.TextColor", "white");
  resource.tstyle.w_text =
    readDatabaseColor("toolbar.windowLabel.textColor",
                      "Toolbar.WindowLabel.TextColor", "white");
  resource.tstyle.c_text =
    readDatabaseColor("toolbar.clock.textColor",
                      "Toolbar.Clock.TextColor", "white");
  resource.tstyle.b_pic =
    readDatabaseColor("toolbar.button.picColor",
                      "Toolbar.Button.PicColor", "black");

  resource.tstyle.justify = LeftJustify;
  if (XrmGetResource(resource.stylerc, "toolbar.justify",
                     "Toolbar.Justify", &value_type, &value)) {
    if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
      resource.tstyle.justify = RightJustify;
    else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
      resource.tstyle.justify = CenterJustify;
  }

  // sanity checks
  if (resource.tstyle.toolbar.texture() == bt::Texture::Parent_Relative) {
    resource.tstyle.toolbar = bt::Texture("solid flat", getDisplay(),
                                       getScreenNumber(), image_control);
    resource.tstyle.toolbar.setColor(bt::Color("black", getDisplay(),
                                            getScreenNumber()));
  }

  // load menu config
  resource.mstyle.title =
    readDatabaseTexture("menu.title", "Menu.Title", "white");
  resource.mstyle.frame =
    readDatabaseTexture("menu.frame", "Menu.Frame", "black");
  resource.mstyle.hilite =
    readDatabaseTexture("menu.hilite", "Menu.Hilite", "white");
  resource.mstyle.t_text =
    readDatabaseColor("menu.title.textColor", "Menu.Title.TextColor", "black");
  resource.mstyle.f_text =
    readDatabaseColor("menu.frame.textColor", "Menu.Frame.TextColor", "white");
  resource.mstyle.d_text =
    readDatabaseColor("menu.frame.disableColor",
                      "Menu.Frame.DisableColor", "black");
  resource.mstyle.h_text =
    readDatabaseColor("menu.hilite.textColor",
                      "Menu.Hilite.TextColor", "black");

  resource.mstyle.t_justify = LeftJustify;
  if (XrmGetResource(resource.stylerc, "menu.title.justify",
                     "Menu.Title.Justify",
                     &value_type, &value)) {
    if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
      resource.mstyle.t_justify = RightJustify;
    else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
      resource.mstyle.t_justify = CenterJustify;
  }

  resource.mstyle.f_justify = LeftJustify;
  if (XrmGetResource(resource.stylerc, "menu.frame.justify",
                     "Menu.Frame.Justify",
                     &value_type, &value)) {
    if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
      resource.mstyle.f_justify = RightJustify;
    else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
      resource.mstyle.f_justify = CenterJustify;
  }

  resource.mstyle.bullet = Basemenu::Triangle;
  if (XrmGetResource(resource.stylerc, "menu.bullet", "Menu.Bullet",
                     &value_type, &value)) {
    if (! strncasecmp(value.addr, "empty", value.size))
      resource.mstyle.bullet = Basemenu::Empty;
    else if (! strncasecmp(value.addr, "square", value.size))
      resource.mstyle.bullet = Basemenu::Square;
    else if (! strncasecmp(value.addr, "diamond", value.size))
      resource.mstyle.bullet = Basemenu::Diamond;
  }

  resource.mstyle.bullet_pos = Basemenu::Left;
  if (XrmGetResource(resource.stylerc, "menu.bullet.position",
                     "Menu.Bullet.Position", &value_type, &value)) {
    if (! strncasecmp(value.addr, "right", value.size))
      resource.mstyle.bullet_pos = Basemenu::Right;
  }

  // sanity checks
  if (resource.mstyle.frame.texture() == bt::Texture::Parent_Relative) {
    resource.mstyle.frame = bt::Texture("solid flat", getDisplay(),
                                     getScreenNumber(), image_control);
    resource.mstyle.frame.setColor(bt::Color("black", getDisplay(),
                                          getScreenNumber()));
  }

  resource.border_color =
    readDatabaseColor("borderColor", "BorderColor", "black");

  unsigned int uint_value;

  // load bevel, border and handle widths
  resource.handle_width = 6;
  if (XrmGetResource(resource.stylerc, "handleWidth", "HandleWidth",
                     &value_type, &value) &&
      sscanf(value.addr, "%u", &uint_value) == 1 &&
      uint_value <= (getWidth() / 2) && uint_value != 0) {
    resource.handle_width = uint_value;
  }

  resource.border_width = 1;
  if (XrmGetResource(resource.stylerc, "borderWidth", "BorderWidth",
                     &value_type, &value) &&
      sscanf(value.addr, "%u", &uint_value) == 1) {
    resource.border_width = uint_value;
  }

  resource.bevel_width = 3;
  if (XrmGetResource(resource.stylerc, "bevelWidth", "BevelWidth",
                     &value_type, &value) &&
      sscanf(value.addr, "%u", &uint_value) == 1 &&
      uint_value <= (getWidth() / 2) && uint_value != 0) {
    resource.bevel_width = uint_value;
  }

  resource.frame_width = resource.bevel_width;
  if (XrmGetResource(resource.stylerc, "frameWidth", "FrameWidth",
                     &value_type, &value) &&
      sscanf(value.addr, "%u", &uint_value) == 1 &&
      uint_value <= (getWidth() / 2)) {
    resource.frame_width = uint_value;
  }

  if (XrmGetResource(resource.stylerc, "rootCommand", "RootCommand",
                     &value_type, &value)) {
    bt::bexec(value.addr, displayString());
  }

  XrmDestroyDatabase(resource.stylerc);
}


void BScreen::addIcon(BlackboxWindow *w) {
  if (! w) return;

  w->setWorkspace(bt::BSENTINEL);
  w->setWindowNumber(iconList.size());

  iconList.push_back(w);

  std::string title = w->getIconTitle();
  unsigned int len = title.length();
  if (len > 60) {
    std::string::iterator lside = title.begin() + 29;
    unsigned int delta = len - 60;
    delta = (delta > 28) ? 28 : delta;
    std::string::iterator rside = title.end() - delta;
    title.replace(lside, rside, "...");
  }

  iconmenu->insert(title);
  iconmenu->update();
}


void BScreen::removeIcon(BlackboxWindow *w) {
  if (! w) return;

  iconList.remove(w);

  iconmenu->remove(w->getWindowNumber());
  iconmenu->update();

  BlackboxWindowList::iterator it = iconList.begin(),
    end = iconList.end();
  for (int i = 0; it != end; ++it)
    (*it)->setWindowNumber(i++);
}


BlackboxWindow *BScreen::getIcon(unsigned int index) {
  if (index < iconList.size()) {
    BlackboxWindowList::iterator it = iconList.begin();
    while (index-- > 0)
      it = bt::next_it(it);
    return *it;
  }

  return (BlackboxWindow *) 0;
}


unsigned int BScreen::addWorkspace(void) {
  Workspace *wkspc = new Workspace(this, workspacesList.size());
  workspacesList.push_back(wkspc);

  workspacemenu->insert(wkspc->getName(), wkspc->getMenu(),
                        wkspc->getID() + 2);
  workspacemenu->update();

  toolbar->reconfigure();

  blackbox->netwm()->setNumberOfDesktops(getRootWindow(),
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

  workspacemenu->remove(wkspc->getID() + 2);
  workspacemenu->update();

  delete wkspc;

  toolbar->reconfigure();

  blackbox->netwm()->setNumberOfDesktops(getRootWindow(),
                                         workspacesList.size());
  updateDesktopNamesHint();

  return workspacesList.size();
}


void BScreen::changeWorkspaceID(unsigned int id) {
  if (! current_workspace || id == current_workspace->getID()) return;

  current_workspace->hide();

  workspacemenu->setItemSelected(current_workspace->getID() + 2, False);

  current_workspace = getWorkspace(id);

  current_workspace->show();

  workspacemenu->setItemSelected(current_workspace->getID() + 2, True);
  toolbar->redrawWorkspaceLabel(True);

  blackbox->netwm()->setCurrentDesktop(getRootWindow(),
                                       current_workspace->getID());
}


void BScreen::addWindow(Window w) {
  manageWindow(w);
  BlackboxWindow *win = blackbox->findWindow(w);
  if (win)
    updateClientListHint();
}


void BScreen::manageWindow(Window w) {
  XWMHints *wmhint = XGetWMHints(blackbox->getXDisplay(), w);
  if (wmhint && (wmhint->flags & StateHint) &&
      wmhint->initial_state == WithdrawnState) {
    slit->addClient(w);
    return;
  }

  new BlackboxWindow(blackbox, w, this);

  BlackboxWindow *win = blackbox->findWindow(w);
  if (! win)
    return;

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


void
BScreen::raiseWindows(const bt::Netwm::WindowList* const workspace_stack) {
  // the 13 represents the number of blackbox windows such as menus
  const unsigned int workspace_stack_size =
    (workspace_stack) ? workspace_stack->size() : 0;
  std::vector<Window> session_stack(workspace_stack_size +
                                    workspacesList.size() +
                                    rootmenuList.size() + 13);
  std::back_insert_iterator<std::vector<Window> > it(session_stack);

  XRaiseWindow(blackbox->getXDisplay(), iconmenu->getWindowID());
  *(it++) = iconmenu->getWindowID();

  WorkspaceList::iterator wit = workspacesList.begin();
  const WorkspaceList::iterator w_end = workspacesList.end();
  for (; wit != w_end; ++wit)
    *(it++) = (*wit)->getMenu()->getWindowID();

  *(it++) = workspacemenu->getWindowID();

  *(it++) = configmenu->getFocusmenu()->getWindowID();
  *(it++) = configmenu->getPlacementmenu()->getWindowID();
  *(it++) = configmenu->getWindowID();

  *(it++) = slit->getMenu()->getDirectionmenu()->getWindowID();
  *(it++) = slit->getMenu()->getPlacementmenu()->getWindowID();
  *(it++) = slit->getMenu()->getWindowID();

  *(it++) = toolbar->getMenu()->
                          getPlacementmenu()->getWindowID();
  *(it++) = toolbar->getMenu()->getWindowID();

  RootmenuList::iterator rit = rootmenuList.begin();
  for (; rit != rootmenuList.end(); ++rit)
    *(it++) = (*rit)->getWindowID();
  *(it++) = rootmenu->getWindowID();

  if (toolbar->isOnTop())
    *(it++) = toolbar->getWindowID();

  if (slit->isOnTop())
    *(it++) = slit->getWindowID();

  if (workspace_stack_size)
    std::copy(workspace_stack->rbegin(), workspace_stack->rend(), it);

  XRestackWindows(blackbox->getXDisplay(), &session_stack[0],
                  session_stack.size());

  updateClientListStackingHint();
}


#ifdef    HAVE_STRFTIME
void BScreen::saveStrftimeFormat(const std::string& format) {
  resource.strftime_format = format;
}
#endif // HAVE_STRFTIME


void BScreen::addWorkspaceName(const std::string& name) {
  workspaceNames.push_back(name);
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
  if (id < workspaceNames.size())
    return workspaceNames[id];
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


void BScreen::propagateWindowName(const BlackboxWindow *bw) {
  if (bw->isIconic()) {
    iconmenu->changeItemLabel(bw->getWindowNumber(), bw->getIconTitle());
    iconmenu->update();
  } else {
    Clientmenu *clientmenu = getWorkspace(bw->getWorkspaceNumber())->getMenu();
    std::string title = bw->getTitle();
    unsigned int len = title.length();
    if (len > 60) {
      std::string::iterator lside = title.begin() + 29;
      unsigned int delta = len - 60;
      delta = (delta > 28) ? 28 : delta;
      std::string::iterator rside = title.end() - delta;
      title.replace(lside, rside, "...");
    }

    clientmenu->changeItemLabel(bw->getWindowNumber(), title);
    clientmenu->update();

    if (blackbox->getFocusedWindow() == bw)
      toolbar->redrawWindowLabel(True);
  }
}


void BScreen::nextFocus(void) const {
  BlackboxWindow *focused = blackbox->getFocusedWindow(),
    *next = focused;

  if (focused &&
      focused->getScreen()->getScreenNumber() == getScreenNumber() &&
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
      focused->getScreen()->getScreenNumber() == getScreenNumber() &&
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
  if (focused->getScreen()->getScreenNumber() == getScreenNumber()) {
    Workspace *workspace = getWorkspace(focused->getWorkspaceNumber());
    workspace->raiseWindow(focused);
  }
}


void BScreen::InitMenu(void) {
  if (rootmenu) {
    rootmenuList.clear();

    while (rootmenu->getCount())
      rootmenu->remove(0);
  } else {
    rootmenu = new Rootmenu(this);
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

            rootmenu->setLabel(label);
            defaultMenu = parseMenuFile(menu_file, rootmenu);
            break;
          }
        }
      }
      fclose(menu_file);
    }
  }

  if (defaultMenu) {
    rootmenu->setInternalMenu();
    rootmenu->insert(bt::i18n(ScreenSet, Screenxterm, "xterm"),
                     BScreen::Execute,
                     bt::i18n(ScreenSet, Screenxterm, "xterm"));
    rootmenu->insert(bt::i18n(ScreenSet, ScreenRestart, "Restart"),
                     BScreen::Restart);
    rootmenu->insert(bt::i18n(ScreenSet, ScreenExit, "Exit"),
                     BScreen::Exit);
    rootmenu->setLabel(bt::i18n(BasemenuSet, BasemenuBlackboxMenu,
                            "Blackbox Menu"));
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
      menu->insert(label);

      break;

    case 421: // exec
      if (! (*label && *command)) {
        fprintf(stderr, bt::i18n(ScreenSet, ScreenEXECError,
                             "BScreen::parseMenuFile: [exec] error, "
                             "no menu label and/or command defined\n"));
        continue;
      }

      menu->insert(label, BScreen::Execute, command);

      break;

    case 442: // exit
      if (! *label) {
        fprintf(stderr, bt::i18n(ScreenSet, ScreenEXITError,
                             "BScreen::parseMenuFile: [exit] error, "
                             "no menu label defined\n"));
        continue;
      }

      menu->insert(label, BScreen::Exit);

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

      menu->insert(label, BScreen::SetStyle, style.c_str());
    }
      break;

    case 630: // config
      if (! *label) {
        fprintf(stderr, bt::i18n(ScreenSet, ScreenCONFIGError,
                             "BScreen::parseMenufile: [config] error, "
                             "no label defined"));
        continue;
      }

      menu->insert(label, configmenu);

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

      Rootmenu *submenu = new Rootmenu(this);

      if (*command)
        submenu->setLabel(command);
      else
        submenu->setLabel(label);

      parseMenuFile(file, submenu);
      submenu->update();
      menu->insert(label, submenu);
      rootmenuList.push_back(submenu);
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
        menu->insert(label, BScreen::RestartOther, command);
      else
        menu->insert(label, BScreen::Restart);
    }

      break;

    case 845: { // reconfig
      if (! *label) {
        fprintf(stderr,
                bt::i18n(ScreenSet, ScreenRECONFIGError,
                     "BScreen::parseMenuFile: [reconfig] error, "
                     "no menu label defined\n"));
        continue;
      }

      menu->insert(label, BScreen::Reconfigure);
    }

      break;

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

      if (newmenu)
        stylesmenu = new Rootmenu(this);
      else
        stylesmenu = menu;

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
        const std::string& fname = *it;

        if (fname[fname.size()-1] == '~')
          continue;

        std::string style = stylesdir;
        style += '/';
        style += fname;

        if (! stat(style.c_str(), &statbuf) && S_ISREG(statbuf.st_mode))
          stylesmenu->insert(fname, BScreen::SetStyle, style);
      }

      stylesmenu->update();

      if (newmenu) {
        stylesmenu->setLabel(label);
        menu->insert(label, stylesmenu);
        rootmenuList.push_back(stylesmenu);
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

      menu->insert(label, workspacemenu);
    }
      break;
    }
  }

  return ((menu->getCount() == 0) ? True : False);
}


void BScreen::shutdown(void) {
  XSelectInput(blackbox->getXDisplay(), getRootWindow(), NoEventMask);
  XSync(blackbox->getXDisplay(), False);

  while(! windowList.empty())
    unmanageWindow(windowList.front(), True);

  slit->shutdown();
}


void BScreen::showPosition(int x, int y) {
  if (! geom_visible) {
    XMoveResizeWindow(blackbox->getXDisplay(), geom_window,
                      (getWidth() - geom_w) / 2,
                      (getHeight() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(blackbox->getXDisplay(), geom_window);
    XRaiseWindow(blackbox->getXDisplay(), geom_window);

    geom_visible = True;
  }

  char label[1024];

  sprintf(label, bt::i18n(ScreenSet, ScreenPositionFormat,
                      "X: %4d x Y: %4d"), x, y);

  XClearWindow(blackbox->getXDisplay(), geom_window);

  bt::Pen pen(resource.wstyle.l_text_focus, resource.wstyle.font);
  if (bt::i18n.multibyte()) {
    XmbDrawString(blackbox->getXDisplay(), geom_window,
                  resource.wstyle.fontset, pen.gc(),
                  resource.bevel_width, resource.bevel_width -
                  resource.wstyle.fontset_extents->max_ink_extent.y,
                  label, strlen(label));
  } else {
    XDrawString(blackbox->getXDisplay(), geom_window,
                pen.gc(), resource.bevel_width,
                resource.wstyle.font->ascent + resource.bevel_width,
                label, strlen(label));
  }
}


void BScreen::showGeometry(unsigned int gx, unsigned int gy) {
  if (! geom_visible) {
    XMoveResizeWindow(blackbox->getXDisplay(), geom_window,
                      (getWidth() - geom_w) / 2,
                      (getHeight() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(blackbox->getXDisplay(), geom_window);
    XRaiseWindow(blackbox->getXDisplay(), geom_window);

    geom_visible = True;
  }

  char label[1024];

  sprintf(label, bt::i18n(ScreenSet, ScreenGeometryFormat,
                      "W: %4d x H: %4d"), gx, gy);

  XClearWindow(blackbox->getXDisplay(), geom_window);

  bt::Pen pen(resource.wstyle.l_text_focus, resource.wstyle.font);
  if (bt::i18n.multibyte()) {
    XmbDrawString(blackbox->getXDisplay(), geom_window,
                  resource.wstyle.fontset, pen.gc(),
                  resource.bevel_width, resource.bevel_width -
                  resource.wstyle.fontset_extents->max_ink_extent.y,
                  label, strlen(label));
  } else {
    XDrawString(blackbox->getXDisplay(), geom_window,
                pen.gc(), resource.bevel_width,
                resource.wstyle.font->ascent +
                resource.bevel_width, label, strlen(label));
  }
}


void BScreen::hideGeometry(void) {
  if (geom_visible) {
    XUnmapWindow(blackbox->getXDisplay(), geom_window);
    geom_visible = False;
  }
}


void BScreen::addStrut(bt::Netwm::Strut *strut) {
  strutList.push_back(strut);
  area_is_dirty = True;
  if (! timer->isTiming()) timer->start();
}


void BScreen::removeStrut(bt::Netwm::Strut *strut) {
  strutList.remove(strut);
  area_is_dirty = True;
  if (! timer->isTiming()) timer->start();
}


void BScreen::updateStrut(void) {
  area_is_dirty = True;
  if (! timer->isTiming()) timer->start();
}


const bt::Rect& BScreen::availableArea(void) {
  if (doFullMax())
    return getRect(); // return the full screen
  if (area_is_dirty)
    updateAvailableArea();
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
  new_area.setSize(getWidth() - (current.left + current.right),
                   getHeight() - (current.top + current.bottom));
  area_is_dirty = False;
  if (timer->isTiming()) timer->stop();

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


void BScreen::clientMessageEvent(const XClientMessageEvent * const event) {
  if (event->format != 32) return;

  if (event->message_type == blackbox->netwm()->numberOfDesktops()) {
    unsigned int number = event->data.l[0];
    unsigned int wkspc_count = getWorkspaceCount();
    if (number > wkspc_count) {
      for (; number != wkspc_count; --number)
        addWorkspace();
    } else if (number < wkspc_count) {
      for (; number != wkspc_count; ++number)
        removeLastWorkspace();
    }
  } else if (event->message_type == blackbox->netwm()->desktopNames()) {
    getDesktopNames();
  } else if (event->message_type == blackbox->netwm()->currentDesktop()) {
    unsigned int workspace = event->data.l[0];
    if (workspace < getWorkspaceCount() &&
        workspace != getCurrentWorkspaceID())
      changeWorkspaceID(workspace);
  }
}


void BScreen::buttonPressEvent(const XButtonEvent * const event) {
  if (event->button == 1) {
    if (! isRootColormapInstalled())
      image_control->installRootColormap();

    if (workspacemenu->isVisible())
      workspacemenu->hide();

    if (rootmenu->isVisible())
      rootmenu->hide();
  } else if (event->button == 2) {
    int mx = event->x_root - (workspacemenu->getWidth() / 2);
    int my = event->y_root - (workspacemenu->getTitleHeight() / 2);

    if (mx < 0) mx = 0;
    if (my < 0) my = 0;

    if (mx + workspacemenu->getWidth() > getWidth())
      mx = getWidth() - workspacemenu->getWidth() - getBorderWidth();

    if (my + workspacemenu->getHeight() > getHeight())
      my = getHeight() - workspacemenu->getHeight() - getBorderWidth();

    workspacemenu->move(mx, my);

    if (! workspacemenu->isVisible()) {
      workspacemenu->removeParent();
      workspacemenu->show();
    }
  } else if (event->button == 3) {
    int mx = event->x_root - (rootmenu->getWidth() / 2);
    int my = event->y_root - (rootmenu->getTitleHeight() / 2);

    if (mx < 0) mx = 0;
    if (my < 0) my = 0;

    if (mx + rootmenu->getWidth() > getWidth())
      mx = getWidth() - rootmenu->getWidth() - getBorderWidth();

    if (my + rootmenu->getHeight() > getHeight())
      my = getHeight() - rootmenu->getHeight() - getBorderWidth();

    rootmenu->move(mx, my);

    if (! rootmenu->isVisible()) {
      blackbox->checkMenu();
      rootmenu->show();
    }
  }
}

void BScreen::configureRequestEvent(const XConfigureRequestEvent * const event) {
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

  XConfigureWindow(blackbox->getXDisplay(), event->window,
                   event->value_mask, &xwc);
}


void BScreen::toggleFocusModel(FocusModel model) {
  std::for_each(windowList.begin(), windowList.end(),
                std::mem_fun(&BlackboxWindow::ungrabButtons));

  if (model == SloppyFocus) {
    saveSloppyFocus(True);
  } else {
    saveSloppyFocus(False);
    saveAutoRaise(False);
    saveClickRaise(False);
  }

  std::for_each(windowList.begin(), windowList.end(),
                std::mem_fun(&BlackboxWindow::grabButtons));
}


bt::Texture BScreen::readDatabaseTexture(const std::string &rname,
                                      const std::string &rclass,
                                      const std::string &default_color) {
  bt::Texture texture;
  XrmValue value;
  char *value_type;

  if (XrmGetResource(resource.stylerc, rname.c_str(), rclass.c_str(),
                     &value_type, &value))
    texture = bt::Texture(value.addr);
  else
    texture.setTexture(bt::Texture::Solid | bt::Texture::Flat);

  // associate this texture with this screen
  texture.setDisplay(getDisplay(), getScreenNumber());
  texture.setImageControl(image_control);

  texture.setColor(readDatabaseColor(rname + ".color", rclass + ".Color",
                                     default_color));
  texture.setColorTo(readDatabaseColor(rname + ".colorTo", rclass + ".ColorTo",
                                       default_color));

  return texture;
}


bt::Color BScreen::readDatabaseColor(const std::string &rname,
                                  const std::string &rclass,
				  const std::string &default_color) {
  bt::Color color;
  XrmValue value;
  char *value_type;
  if (XrmGetResource(resource.stylerc, rname.c_str(), rclass.c_str(),
                     &value_type, &value))
    color = bt::Color(value.addr, getDisplay(), getScreenNumber());
  else
    color = bt::Color(default_color, getDisplay(), getScreenNumber());
  return color;
}


XFontSet BScreen::readDatabaseFontSet(const std::string &rname,
                                      const std::string &rclass) {
  char *defaultFont = "fixed";

  bool load_default = True;
  XrmValue value;
  char *value_type;
  XFontSet fontset = 0;
  if (XrmGetResource(resource.stylerc, rname.c_str(), rclass.c_str(),
                     &value_type, &value) &&
      (fontset = createFontSet(value.addr))) {
    load_default = False;
  }

  if (load_default) {
    fontset = createFontSet(defaultFont);

    if (! fontset) {
      fprintf(stderr,
              bt::i18n(ScreenSet, ScreenDefaultFontLoadFail,
                   "BScreen::setCurrentStyle(): couldn't load default font.\n"));
      exit(2);
    }
  }

  return fontset;
}


XFontStruct *BScreen::readDatabaseFont(const std::string &rname,
                                       const std::string &rclass) {
  char *defaultFont = "fixed";

  bool load_default = False;
  XrmValue value;
  char *value_type;
  XFontStruct *font = 0;
  if (XrmGetResource(resource.stylerc, rname.c_str(), rclass.c_str(),
                     &value_type, &value)) {
    if ((font = XLoadQueryFont(blackbox->getXDisplay(), value.addr)) == NULL) {
      fprintf(stderr,
              bt::i18n(ScreenSet, ScreenFontLoadFail,
                   "BScreen::setCurrentStyle(): couldn't load font '%s'\n"),
              value.addr);

      load_default = True;
    }
  } else {
    load_default = True;
  }

  if (load_default) {
    font = XLoadQueryFont(blackbox->getXDisplay(), defaultFont);
    if (font == NULL) {
      fprintf(stderr,
              bt::i18n(ScreenSet, ScreenDefaultFontLoadFail,
                   "BScreen::setCurrentStyle(): couldn't load default font.\n"));
      exit(2);
    }
  }

  return font;
}


#ifndef    HAVE_STRCASESTR
static const char * strcasestr(const char *str, const char *ptn) {
  const char *s2, *p2;
  for(; *str; str++) {
    for(s2=str,p2=ptn; ; s2++,p2++) {
      if (!*p2) return str;
      if (toupper(*s2) != toupper(*p2)) break;
    }
  }
  return NULL;
}
#endif // HAVE_STRCASESTR


static const char *getFontElement(const char *pattern, char *buf,
                                  int bufsiz, ...) {
  const char *p, *v;
  char *p2;
  va_list va;

  va_start(va, bufsiz);
  buf[bufsiz-1] = 0;
  buf[bufsiz-2] = '*';
  while((v = va_arg(va, char *)) != NULL) {
    p = strcasestr(pattern, v);
    if (p) {
      strncpy(buf, p+1, bufsiz-2);
      p2 = strchr(buf, '-');
      if (p2) *p2=0;
      va_end(va);
      return p;
    }
  }
  va_end(va);
  strncpy(buf, "*", bufsiz);
  return NULL;
}


static const char *getFontSize(const char *pattern, int *size) {
  const char *p;
  const char *p2=NULL;
  int n=0;

  for (p=pattern; 1; p++) {
    if (!*p) {
      if (p2!=NULL && n>1 && n<72) {
        *size = n; return p2+1;
      } else {
        *size = 16; return NULL;
      }
    } else if (*p=='-') {
      if (n>1 && n<72 && p2!=NULL) {
        *size = n;
        return p2+1;
      }
      p2=p; n=0;
    } else if (*p>='0' && *p<='9' && p2!=NULL) {
      n *= 10;
      n += *p-'0';
    } else {
      p2=NULL; n=0;
    }
  }
}


XFontSet BScreen::createFontSet(const std::string &fontname) {
  XFontSet fs;
  char **missing, *def = "-";
  int nmissing, pixel_size = 0, buf_size = 0;
  char weight[FONT_ELEMENT_SIZE], slant[FONT_ELEMENT_SIZE];

  fs = XCreateFontSet(blackbox->getXDisplay(),
                      fontname.c_str(), &missing, &nmissing, &def);
  if (fs && (! nmissing))
    return fs;

  const char *nfontname = fontname.c_str();
#ifdef    HAVE_SETLOCALE
  if (! fs) {
    if (nmissing) XFreeStringList(missing);

    setlocale(LC_CTYPE, "C");
    fs = XCreateFontSet(blackbox->getXDisplay(), fontname.c_str(),
                        &missing, &nmissing, &def);
    setlocale(LC_CTYPE, "");
  }
#endif // HAVE_SETLOCALE

  if (fs) {
    XFontStruct **fontstructs;
    char **fontnames;
    XFontsOfFontSet(fs, &fontstructs, &fontnames);
    nfontname = fontnames[0];
  }

  getFontElement(nfontname, weight, FONT_ELEMENT_SIZE,
                 "-medium-", "-bold-", "-demibold-", "-regular-", NULL);
  getFontElement(nfontname, slant, FONT_ELEMENT_SIZE,
                 "-r-", "-i-", "-o-", "-ri-", "-ro-", NULL);
  getFontSize(nfontname, &pixel_size);

  if (! strcmp(weight, "*"))
    strncpy(weight, "medium", FONT_ELEMENT_SIZE);
  if (! strcmp(slant, "*"))
    strncpy(slant, "r", FONT_ELEMENT_SIZE);
  if (pixel_size < 3)
    pixel_size = 3;
  else if (pixel_size > 97)
    pixel_size = 97;

  buf_size = strlen(nfontname) + (FONT_ELEMENT_SIZE * 2) + 64;
  char *pattern2 = new char[buf_size];
  sprintf(pattern2,
           "%s,"
           "-*-*-%s-%s-*-*-%d-*-*-*-*-*-*-*,"
           "-*-*-*-*-*-*-%d-*-*-*-*-*-*-*,*",
           nfontname, weight, slant, pixel_size, pixel_size);
  nfontname = pattern2;

  if (nmissing)
    XFreeStringList(missing);
  if (fs)
    XFreeFontSet(blackbox->getXDisplay(), fs);

  fs = XCreateFontSet(blackbox->getXDisplay(), nfontname, &missing,
                      &nmissing, &def);

  delete [] pattern2;

  return fs;
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

  blackbox->netwm()->setWorkarea(getRootWindow(), workarea, wkspc_count);

  delete [] workarea;
}


void BScreen::updateDesktopNamesHint(void) const {
  std::string names;

  WorkspaceList::const_iterator it = workspacesList.begin(),
    end = workspacesList.end();
  for (; it != end; ++it)
    names += (*it)->getName() + '\0';

  blackbox->netwm()->setDesktopNames(getRootWindow(), names);
}


void BScreen::updateClientListHint(void) const {
  if (windowList.empty()) {
    blackbox->netwm()->removeProperty(getRootWindow(),
                                      blackbox->netwm()->clientList());
    return;
  }

  bt::Netwm::WindowList clientList(windowList.size());

  std::transform(windowList.begin(), windowList.end(), clientList.begin(),
                 std::mem_fun(&BlackboxWindow::getClientWindow));

  blackbox->netwm()->setClientList(getRootWindow(), clientList);
}


void BScreen::updateClientListStackingHint(void) const {
  bt::Netwm::WindowList stack;

  WorkspaceList::const_iterator it = workspacesList.begin(),
    end = workspacesList.end();
  for (; it != end; ++it)
    (*it)->updateClientListStacking(stack);

  if (stack.empty()) {
    blackbox->netwm()->removeProperty(getRootWindow(),
                                      blackbox->netwm()->clientListStacking());
    return;
  }

  blackbox->netwm()->setClientListStacking(getRootWindow(), stack);
}


void BScreen::getDesktopNames(void) {
  bt::Netwm::UTF8StringList names;
  if(! blackbox->netwm()->readDesktopNames(getRootWindow(), names))
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


void BScreen::timeout(void) {
  if (area_is_dirty)
    updateAvailableArea();
}
