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
#include "Font.hh"
#include "Iconmenu.hh"
#include "Image.hh"
#include "Menu.hh"
#include "Netwm.hh"
#include "Pen.hh"
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
  screen_info(bb->getScreenInfo(scrn)), blackbox(bb) {

  event_mask = ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
               SubstructureRedirectMask | ButtonPressMask | ButtonReleaseMask;

  XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);
  XSelectInput(screen_info.getDisplay().XDisplay(),
               screen_info.getRootWindow(), event_mask);
  XSync(screen_info.getDisplay().XDisplay(), False);
  XSetErrorHandler((XErrorHandler) old);

  managed = running;
  if (! managed) return;

  fprintf(stderr, bt::i18n(ScreenSet, ScreenManagingScreen,
                           "BScreen::BScreen: managing screen %d "
                           "using visual 0x%lx, depth %d\n"),
          screen_info.getScreenNumber(),
          XVisualIDFromVisual(screen_info.getVisual()),
          screen_info.getDepth());

  blackbox->insertEventHandler(screen_info.getRootWindow(), this);

  rootmenu = 0;
  resource.stylerc = 0;

  geom_pixmap = None;

  timer = new bt::Timer(blackbox, this);
  timer->setTimeout(750l); // once every 1.5 seconds
  timer->start();

  XDefineCursor(blackbox->getXDisplay(), screen_info.getRootWindow(),
                blackbox->getSessionCursor());

  // start off full screen, top left.
  usableArea.setSize(screen_info.getWidth(), screen_info.getHeight());
  area_is_dirty = False;

  image_control =
    new bt::ImageControl(blackbox, screen_info.getDisplay(), &screen_info,
                         blackbox->getCacheLife(), blackbox->getCacheMax());
  image_control->installRootColormap();
  root_colormap_installed = True;

  blackbox->load_rc(this);

  LoadStyle();

  XGCValues gcv;
  unsigned long gc_value_mask = GCForeground;
  if (! bt::i18n.multibyte()) gc_value_mask |= GCFont;

  gcv.foreground = WhitePixel(blackbox->getXDisplay(),
                              screen_info.getScreenNumber())
                   ^ BlackPixel(blackbox->getXDisplay(),
                                screen_info.getScreenNumber());
  gcv.function = GXxor;
  gcv.subwindow_mode = IncludeInferiors;
  opGC = XCreateGC(blackbox->getXDisplay(), screen_info.getRootWindow(),
                   GCForeground | GCFunction | GCSubwindowMode, &gcv);

  const char *s =
    bt::i18n(ScreenSet, ScreenPositionLength, "0: 0000 x 0: 0000");
  bt::Rect geomr = bt::textRect(resource.wstyle.font, s);
  geom_w = geomr.width() + (resource.bevel_width * 2);
  geom_h = geomr.height() + (resource.bevel_width * 2);

  XSetWindowAttributes setattrib;
  unsigned long mask = CWBorderPixel | CWColormap | CWSaveUnder;
  setattrib.border_pixel =
    resource.border_color.pixel(screen_info.getScreenNumber());
  setattrib.colormap = screen_info.getColormap();
  setattrib.save_under = True;

  geom_window = XCreateWindow(blackbox->getXDisplay(),
                              screen_info.getRootWindow(),
                              0, 0, geom_w, geom_h, resource.border_width,
                              screen_info.getDepth(), InputOutput,
                              screen_info.getVisual(),
                              mask, &setattrib);
  geom_visible = False;

  bt::Texture* texture = &(resource.wstyle.l_focus);
  geom_pixmap =
    texture->render(blackbox->getDisplay(), screen_info.getScreenNumber(),
                    *image_control,
                    geom_w, geom_h, geom_pixmap);
  if (geom_pixmap == ParentRelative) {
    texture = &(resource.wstyle.t_focus);
    geom_pixmap =
      texture->render(blackbox->getDisplay(), screen_info.getScreenNumber(),
                      *image_control,
                      geom_w, geom_h, geom_pixmap);
  }
  if (! geom_pixmap)
    XSetWindowBackground(blackbox->getXDisplay(), geom_window,
                         texture->color().pixel(screen_info.getScreenNumber()));
  else
    XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                               geom_window, geom_pixmap);

  workspacemenu =
    new Workspacemenu(*blackbox, screen_info.getScreenNumber(), this);
  iconmenu =
    new Iconmenu(*blackbox, screen_info.getScreenNumber(), this);
  configmenu =
    new Configmenu(*blackbox, screen_info.getScreenNumber(), this);

  Workspace *wkspc = (Workspace *) 0;
  if (resource.workspaces != 0) {
    for (unsigned int i = 0; i < resource.workspaces; ++i) {
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

  workspacemenu->insertSeparator();
  workspacemenu->insertItem(bt::i18n(IconSet, IconIcons, "Icons"),
                            iconmenu, 499u);

  current_workspace = workspacesList.front();
  current_workspace_id = current_workspace->getID();
  workspacemenu->setItemChecked(current_workspace->getID(), true);

  workspaceNames.clear();

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
  netwm.setSupportingWMCheck(screen_info.getRootWindow(), geom_window);
  netwm.setSupportingWMCheck(geom_window, geom_window);
  netwm.setWMName(geom_window, "Blackbox");

  netwm.setNumberOfDesktops(screen_info.getRootWindow(),
                             workspacesList.size());
  netwm.setDesktopGeometry(screen_info.getRootWindow(),
                            screen_info.getWidth(), screen_info.getHeight());
  netwm.setActiveWindow(screen_info.getRootWindow(), None);
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

  netwm.setSupported(screen_info.getRootWindow(), supported, 46);

  unsigned int i, j, nchild;
  Window r, p, *children;
  XQueryTree(blackbox->getXDisplay(), screen_info.getRootWindow(), &r, &p,
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

  blackbox->removeEventHandler(screen_info.getRootWindow());

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

  blackbox->netwm().removeProperty(screen_info.getRootWindow(),
                                   blackbox->netwm().supportingWMCheck());
  blackbox->netwm().removeProperty(screen_info.getRootWindow(),
                                    blackbox->netwm().supported());
  blackbox->netwm().removeProperty(screen_info.getRootWindow(),
                                    blackbox->netwm().numberOfDesktops());
  blackbox->netwm().removeProperty(screen_info.getRootWindow(),
                                    blackbox->netwm().desktopGeometry());
  blackbox->netwm().removeProperty(screen_info.getRootWindow(),
                                    blackbox->netwm().currentDesktop());
  blackbox->netwm().removeProperty(screen_info.getRootWindow(),
                                    blackbox->netwm().activeWindow());
  blackbox->netwm().removeProperty(screen_info.getRootWindow(),
                                    blackbox->netwm().workarea());

  XFreeGC(blackbox->getXDisplay(), opGC);
}


void BScreen::reconfigure(void) {
  LoadStyle();

  XGCValues gcv;
  unsigned long gc_value_mask = GCForeground;
  if (! bt::i18n.multibyte()) gc_value_mask |= GCFont;

  gcv.foreground = WhitePixel(blackbox->getXDisplay(),
                              screen_info.getScreenNumber())
                   ^ BlackPixel(blackbox->getXDisplay(),
                                screen_info.getScreenNumber());
  gcv.function = GXxor;
  gcv.subwindow_mode = IncludeInferiors;
  XChangeGC(blackbox->getXDisplay(), opGC,
            GCForeground | GCFunction | GCSubwindowMode, &gcv);

  const char *s =
    bt::i18n(ScreenSet, ScreenPositionLength, "0: 0000 x 0: 0000");
  bt::Rect geomr = bt::textRect(resource.wstyle.font, s);
  geom_w = geomr.width() + (resource.bevel_width * 2);
  geom_h = geomr.height() + (resource.bevel_width * 2);

  bt::Texture* texture = &(resource.wstyle.l_focus);
  geom_pixmap = texture->render(blackbox->getDisplay(),
                                screen_info.getScreenNumber(),
                                *image_control,
                                geom_w, geom_h, geom_pixmap);
  if (geom_pixmap == ParentRelative) {
    texture = &(resource.wstyle.t_focus);
    geom_pixmap = texture->render(blackbox->getDisplay(),
                                  screen_info.getScreenNumber(),
                                  *image_control,
                                  geom_w, geom_h, geom_pixmap);
  }
  if (! geom_pixmap)
    XSetWindowBackground(blackbox->getXDisplay(), geom_window,
                         texture->color().pixel(screen_info.getScreenNumber()));
  else
    XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                               geom_window, geom_pixmap);

  XSetWindowBorderWidth(blackbox->getXDisplay(), geom_window,
                        resource.border_width);
  XSetWindowBorder(blackbox->getXDisplay(), geom_window,
                   resource.border_color.pixel(screen_info.getScreenNumber()));

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

  image_control->timeout();
}


void BScreen::rereadMenu(void) {
  InitMenu();
  raiseWindows((WindowStack*) 0);

  rootmenu->reconfigure();
}


void BScreen::LoadStyle(void) {
  const bt::Display &display = blackbox->getDisplay();
  unsigned int screen = screen_info.getScreenNumber();

  // use the user selected style
  bt::Resource res(std::string(blackbox->getStyleFilename()));
  if (! res.valid())
    res.load(std::string(DEFAULTSTYLE));

  // load bevel, border and handle widths
  std::string wstr = res.read("borderWidth", "BorderWidth", "1");
  resource.border_width =
    static_cast<unsigned int>(strtoul(wstr.c_str(), 0, 0));
  wstr = res.read("bevelWidth", "BevelWidth", "3");
  resource.bevel_width =
    static_cast<unsigned int>(strtoul(wstr.c_str(), 0, 0));

  // load menu style
  bt::MenuStyle::get(*blackbox, screen, image_control)->load(res);

  // load fonts
  resource.wstyle.font.setFontName(res.read("window.font", "Window.Font",
                                            "fixed"));
  resource.tstyle.font.setFontName(res.read("toolbar.font", "Toolbar.Font",
                                            "fixed"));

  // load window config
  resource.wstyle.t_focus =
    bt::textureResource(display, screen, res,
                        "window.title.focus", "Window.Title.Focus",
                        "white");
  resource.wstyle.t_unfocus =
    bt::textureResource(display, screen, res,
                        "window.title.unfocus", "Window.Title.Unfocus",
                        "black");
  resource.wstyle.l_focus =
    bt::textureResource(display, screen, res,
                        "window.label.focus", "Window.Label.Focus",
                        "white");
  resource.wstyle.l_unfocus =
    bt::textureResource(display, screen, res,
                        "window.label.unfocus", "Window.Label.Unfocus",
                        "black");
  resource.wstyle.h_focus =
    bt::textureResource(display, screen, res,
                        "window.handle.focus", "Window.Handle.Focus",
                        "white");
  resource.wstyle.h_unfocus =
    bt::textureResource(display, screen, res,
                        "window.handle.unfocus", "Window.Handle.Unfocus",
                        "black");

  wstr = res.read("window.handleWidth", "Window.HandleWidth", "6");
  resource.wstyle.handle_width =
    static_cast<unsigned int>(strtoul(wstr.c_str(), 0, 0));

  resource.wstyle.g_focus =
    bt::textureResource(display, screen, res,
                        "window.grip.focus", "Window.Grip.Focus",
                        "white");
  resource.wstyle.g_unfocus =
    bt::textureResource(display, screen, res,
                        "window.grip.unfocus", "Window.Grip.Unfocus",
                        "black");
  resource.wstyle.b_focus =
    bt::textureResource(display, screen, res,
                        "window.button.focus", "Window.Button.Focus",
                        "white");
  resource.wstyle.b_unfocus =
    bt::textureResource(display, screen, res,
                        "window.button.unfocus", "Window.Button.Unfocus",
                        "black");
  resource.wstyle.b_pressed =
    bt::textureResource(display, screen, res,
                        "window.button.pressed", "Window.Button.Pressed",
                        "black");

  // we create the window.frame texture by hand because it exists only to
  // make the code cleaner and is not actually used for display
  bt::Color color;
  color = bt::Color::namedColor(display, screen,
                                res.read("window.frame.focusColor",
                                         "Window.Frame.FocusColor",
                                         "white"));
  resource.wstyle.f_focus.setDescription("solid flat");
  resource.wstyle.f_focus.setColor(color);

  color = bt::Color::namedColor(display, screen,
                                res.read("window.frame.unfocusColor",
                                         "Window.Frame.UnfocusColor",
                                         "white"));
  resource.wstyle.f_unfocus.setDescription("solid flat");
  resource.wstyle.f_unfocus.setColor(color);

  wstr = res.read("window.frameWidth", "Window.FrameWidth",
                  bt::itostring(resource.bevel_width));
  resource.wstyle.frame_width =
    static_cast<unsigned int>(strtoul(wstr.c_str(), 0, 0));

  resource.wstyle.l_text_focus =
    bt::Color::namedColor(display, screen,
                          res.read("window.label.focus.textColor",
                                   "Window.Label.Focus.TextColor",
                                   "black"));
  resource.wstyle.l_text_unfocus =
    bt::Color::namedColor(display, screen,
                          res.read("window.label.unfocus.textColor",
                                   "Window.Label.Unfocus.TextColor",
                                   "white"));
  resource.wstyle.b_pic_focus =
    bt::Color::namedColor(display, screen,
                          res.read("window.button.focus.picColor",
                                   "Window.Button.Focus.PicColor",
                                   "black"));
  resource.wstyle.b_pic_unfocus =
    bt::Color::namedColor(display, screen,
                          res.read("window.button.unfocus.picColor",
                                   "Window.Button.Unfocus.PicColor",
                                   "white"));

  resource.wstyle.alignment =
    bt::alignResource(res, "window.alignment", "Window.Alignment");

  // load toolbar config
  resource.tstyle.toolbar =
    bt::textureResource(display, screen, res,
                        "toolbar", "Toolbar",
                        "black");
  resource.tstyle.label =
    bt::textureResource(display, screen, res,
                        "toolbar.label", "Toolbar.Label",
                        "black");
  resource.tstyle.window =
    bt::textureResource(display, screen, res,
                        "toolbar.windowLabel", "Toolbar.WindowLabel",
                        "black");
  resource.tstyle.button =
    bt::textureResource(display, screen, res,
                        "toolbar.button", "Toolbar.Button",
                        "white");
  resource.tstyle.pressed =
    bt::textureResource(display, screen, res,
                        "toolbar.button.pressed", "Toolbar.Button.Pressed",
                        "black");
  resource.tstyle.clock =
    bt::textureResource(display, screen, res,
                        "toolbar.clock", "Toolbar.Clock",
                        "black");

  resource.tstyle.l_text =
    bt::Color::namedColor(display, screen,
                          res.read("toolbar.label.textColor",
                                   "Toolbar.Label.TextColor",
                                   "white"));
  resource.tstyle.w_text =
    bt::Color::namedColor(display, screen,
                          res.read("toolbar.windowLabel.textColor",
                                   "Toolbar.WindowLabel.TextColor",
                                   "white"));
  resource.tstyle.c_text =
    bt::Color::namedColor(display, screen,
                          res.read("toolbar.clock.textColor",
                                   "Toolbar.Clock.TextColor",
                                   "white"));
  resource.tstyle.b_pic =
    bt::Color::namedColor(display, screen,
                          res.read("toolbar.button.picColor",
                                   "Toolbar.Button.PicColor",
                                   "black"));
  resource.border_color =
    bt::Color::namedColor(display, screen,
                          res.read("borderColor",
                                   "BorderColor",
                                   "black"));

  resource.tstyle.alignment =
    bt::alignResource(res, "toolbar.alignment", "Toolbar.Alignment");

  std::string root_command = res.read("rootCommand", "RootCommand");
  if (! root_command.empty())
    bt::bexec(root_command, displayString());

  // sanity checks
  if (resource.wstyle.t_focus.texture() == bt::Texture::Parent_Relative)
    resource.wstyle.t_focus = resource.wstyle.f_focus;
  if (resource.wstyle.t_unfocus.texture() == bt::Texture::Parent_Relative)
    resource.wstyle.t_unfocus = resource.wstyle.f_unfocus;
  if (resource.wstyle.h_focus.texture() == bt::Texture::Parent_Relative)
    resource.wstyle.h_focus = resource.wstyle.f_focus;
  if (resource.wstyle.h_unfocus.texture() == bt::Texture::Parent_Relative)
    resource.wstyle.h_unfocus = resource.wstyle.f_unfocus;

  if (resource.tstyle.toolbar.texture() == bt::Texture::Parent_Relative) {
    resource.tstyle.toolbar.setTexture(bt::Texture::Flat |
                                       bt::Texture::Solid);
    resource.tstyle.toolbar.setColor(bt::Color(0, 0, 0));
  }

  XrmDestroyDatabase(resource.stylerc);
  resource.stylerc = 0;
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

  blackbox->netwm().setNumberOfDesktops(screen_info.getRootWindow(),
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

  blackbox->netwm().setNumberOfDesktops(screen_info.getRootWindow(),
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
  toolbar->redrawWorkspaceLabel(True);

  blackbox->netwm().setCurrentDesktop(screen_info.getRootWindow(),
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

  Workspace* wkspc = (win->getWorkspaceNumber() > getWorkspaceCount()) ?
    current_workspace : getWorkspace(win->getWorkspaceNumber());

  bool place_window = True;
  if (blackbox->isStartup() ||
      ((win->isTransient() || win->normalHintFlags() & (PPosition|USPosition))
       && win->clientRect().intersects(screen_info.getRect())))
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
    XRaiseWindow(blackbox->getXDisplay(), session_stack[0]);
    XRestackWindows(blackbox->getXDisplay(), &session_stack[0],
                    session_stack.size());
  }

  updateClientListStackingHint();
}


void BScreen::saveStrftimeFormat(const std::string& format) {
  resource.strftime_format = format;
}


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


void BScreen::propagateWindowName(const BlackboxWindow *w) {
  if (! w->isIconic()) {
    Clientmenu *clientmenu = getWorkspace(w->getWorkspaceNumber())->getMenu();
    clientmenu->changeItem(w->getWindowNumber(),
                           bt::ellideText(w->getTitle(), 60, "..."));

    if (blackbox->getFocusedWindow() == w)
      toolbar->redrawWindowLabel(True);
  } else {
    iconmenu->changeItem(w->getWindowNumber(), w->getIconTitle());
  }
}


void BScreen::nextFocus(void) const {
  BlackboxWindow *focused = blackbox->getFocusedWindow(),
    *next = focused;

  if (focused &&
      focused->getScreen()->screen_info.getScreenNumber() ==
      screen_info.getScreenNumber() &&
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
      focused->getScreen()->screen_info.getScreenNumber() ==
      screen_info.getScreenNumber() &&
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
  if (focused->getScreen()->screen_info.getScreenNumber() ==
      screen_info.getScreenNumber()) {
    Workspace *workspace = getWorkspace(focused->getWorkspaceNumber());
    workspace->raiseWindow(focused);
  }
}


void BScreen::InitMenu(void) {
  if (rootmenu) {
    rootmenu->clear();
  } else {
    rootmenu = new Rootmenu(*blackbox, screen_info.getScreenNumber(), this);
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
        new Rootmenu(*blackbox, screen_info.getScreenNumber(), this);
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
          new Rootmenu(*blackbox, screen_info.getScreenNumber(),this);
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
        const std::string& fname = *it;

        if (fname[fname.size()-1] == '~')
          continue;

        std::string style = stylesdir;
        style += '/';
        style += fname;

        if (! stat(style.c_str(), &statbuf) && S_ISREG(statbuf.st_mode))
          stylesmenu->insertFunction(fname, BScreen::SetStyle, style);
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
  XSelectInput(blackbox->getXDisplay(), screen_info.getRootWindow(),
               NoEventMask);
  XSync(blackbox->getXDisplay(), False);

  while(! windowList.empty())
    unmanageWindow(windowList.back(), True);

  slit->shutdown();
}


void BScreen::showPosition(int x, int y) {
  if (! geom_visible) {
    XMoveResizeWindow(blackbox->getXDisplay(), geom_window,
                      (screen_info.getWidth() - geom_w) / 2,
                      (screen_info.getHeight() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(blackbox->getXDisplay(), geom_window);
    XRaiseWindow(blackbox->getXDisplay(), geom_window);

    geom_visible = True;
  }

  char label[1024];
  sprintf(label, bt::i18n(ScreenSet, ScreenPositionFormat,
                      "X: %4d x Y: %4d"), x, y);

  XClearWindow(blackbox->getXDisplay(), geom_window);

  bt::Pen pen(screen_info.getScreenNumber(), resource.wstyle.l_text_focus);
  bt::Rect rect(resource.bevel_width, resource.bevel_width,
                geom_w - (resource.bevel_width * 2),
                geom_h - (resource.bevel_width * 2));
  bt::drawText(resource.wstyle.font, pen, geom_window, rect,
               resource.wstyle.alignment, label);
}


void BScreen::showGeometry(unsigned int gx, unsigned int gy) {
  if (! geom_visible) {
    XMoveResizeWindow(blackbox->getXDisplay(), geom_window,
                      (screen_info.getWidth() - geom_w) / 2,
                      (screen_info.getHeight() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(blackbox->getXDisplay(), geom_window);
    XRaiseWindow(blackbox->getXDisplay(), geom_window);

    geom_visible = True;
  }

  char label[1024];

  sprintf(label, bt::i18n(ScreenSet, ScreenGeometryFormat,
                          "W: %4d x H: %4d"), gx, gy);

  XClearWindow(blackbox->getXDisplay(), geom_window);

  bt::Pen pen(screen_info.getScreenNumber(), resource.wstyle.l_text_focus);
  bt::Rect rect(resource.bevel_width, resource.bevel_width,
                geom_w - (resource.bevel_width * 2),
                geom_h - (resource.bevel_width * 2));
  bt::drawText(resource.wstyle.font, pen, geom_window, rect,
               resource.wstyle.alignment, label);
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
    return screen_info.getRect(); // return the full screen
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
  new_area.setSize(screen_info.getWidth() - (current.left + current.right),
                   screen_info.getHeight() - (current.top + current.bottom));
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
    if (! isRootColormapInstalled())
      image_control->installRootColormap();
  } else if (event->button == 2) {
    workspacemenu->popup(event->x_root, event->y_root);
  } else if (event->button == 3) {
    rootmenu->popup(event->x_root, event->y_root);
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

  blackbox->netwm().setWorkarea(screen_info.getRootWindow(),
                                 workarea, wkspc_count);

  delete [] workarea;
}


void BScreen::updateDesktopNamesHint(void) const {
  std::string names;

  WorkspaceList::const_iterator it = workspacesList.begin(),
    end = workspacesList.end();
  for (; it != end; ++it)
    names += (*it)->getName() + '\0';

  blackbox->netwm().setDesktopNames(screen_info.getRootWindow(), names);
}


void BScreen::updateClientListHint(void) const {
  if (windowList.empty()) {
    blackbox->netwm().removeProperty(screen_info.getRootWindow(),
                                      blackbox->netwm().clientList());
    return;
  }

  bt::Netwm::WindowList clientList(windowList.size());

  std::transform(windowList.begin(), windowList.end(), clientList.begin(),
                 std::mem_fun(&BlackboxWindow::getClientWindow));

  blackbox->netwm().setClientList(screen_info.getRootWindow(), clientList);
}


void BScreen::updateClientListStackingHint(void) const {
  bt::Netwm::WindowList stack;

  WorkspaceList::const_iterator it = workspacesList.begin(),
    end = workspacesList.end();
  for (; it != end; ++it)
    (*it)->updateClientListStacking(stack);

  if (stack.empty()) {
    blackbox->netwm().removeProperty(screen_info.getRootWindow(),
                                      blackbox->netwm().clientListStacking());
    return;
  }

  blackbox->netwm().setClientListStacking(screen_info.getRootWindow(), stack);
}


void BScreen::getDesktopNames(void) {
  bt::Netwm::UTF8StringList names;
  if(! blackbox->netwm().readDesktopNames(screen_info.getRootWindow(), names))
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


BlackboxWindow* BScreen::getWindow(unsigned int workspace, unsigned int id) {
  return getWorkspace(workspace)->getWindow(id);
}


void BScreen::setWorkspaceName(unsigned int workspace,
                               const std::string& name) {
  getWorkspace(workspace)->setName(name);
}
