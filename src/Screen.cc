// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Screen.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2004 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2004
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

#include "Screen.hh"
#include "Clientmenu.hh"
#include "Configmenu.hh"
#include "Iconmenu.hh"
#include "Rootmenu.hh"
#include "Slit.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "WindowGroup.hh"
#include "Windowmenu.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"

#include <Pen.hh>
#include <PixmapCache.hh>

#include <X11/Xutil.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>


static bool running = true;
static int anotherWMRunning(Display *, XErrorEvent *) {
  running = false;
  return -1;
}


BScreen::BScreen(Blackbox *bb, unsigned int scrn) :
  screen_info(bb->display().screenInfo(scrn)), blackbox(bb),
  _resource(bb->resource().screenResource(scrn))
{
  running = true;
  XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);
  XSelectInput(screen_info.display().XDisplay(),
               screen_info.rootWindow(),
               ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
               StructureNotifyMask | // this really should go away
               SubstructureRedirectMask |
	       ButtonPressMask | ButtonReleaseMask);

  XSync(screen_info.display().XDisplay(), False);
  XSetErrorHandler((XErrorHandler) old);

  managed = running;
  if (! managed) {
    fprintf(stderr,
            "%s: another window manager is already running on display '%s'\n",
            blackbox->applicationName().c_str(),
            DisplayString(blackbox->XDisplay()));
    return;
  }

  static const char *visual_classes[] = {
    "StaticGray",
    "GrayScale",
    "StaticColor",
    "PseudoColor",
    "TrueColor",
    "DirectColor"
  };
  fprintf(stderr, "%s: managing screen %u using %s visual 0x%lx, depth %d\n",
          blackbox->applicationName().c_str(), screen_info.screenNumber(),
          visual_classes[screen_info.visual()->c_class],
          XVisualIDFromVisual(screen_info.visual()), screen_info.depth());

  blackbox->insertEventHandler(screen_info.rootWindow(), this);

  cascade_x = cascade_y = ~0;

  rootmenu = 0;
  _windowmenu = 0;

  XDefineCursor(blackbox->XDisplay(), screen_info.rootWindow(),
                blackbox->resource().sessionCursor());

  // start off full screen, top left.
  usableArea.setSize(screen_info.width(), screen_info.height());

  LoadStyle();

  geom_pixmap = None;
  geom_visible = False;
  geom_window = None;

  empty_window =
    XCreateSimpleWindow(blackbox->XDisplay(), screen_info.rootWindow(),
                        0, 0, screen_info.width(), screen_info.height(), 0,
                        0l, 0l);
  XSetWindowBackgroundPixmap(blackbox->XDisplay(), empty_window, None);

  updateGeomWindow();

  configmenu =
    new Configmenu(*blackbox, screen_info.screenNumber(), this);
  _iconmenu =
    new Iconmenu(*blackbox, screen_info.screenNumber(), this);
  workspacemenu =
    new Workspacemenu(*blackbox, screen_info.screenNumber(), this);

  if (_resource.numberOfWorkspaces() == 0) // there is always 1 workspace
    _resource.saveWorkspaces(1);

  workspacemenu->insertIconMenu(_iconmenu);
  for (unsigned int i = 0; i < _resource.numberOfWorkspaces(); ++i) {
    Workspace *wkspc = new Workspace(this, i);
    workspacesList.push_back(wkspc);
    workspacemenu->insertWorkspace(wkspc);
  }

  current_workspace = workspacesList.front()->id();
  workspacemenu->setWorkspaceChecked(current_workspace, true);

  // the Slit will be created on demand
  _slit = 0;

  _toolbar = 0;
  if (_resource.isToolbarEnabled()) {
    _toolbar = new Toolbar(this);
    stackingList.insert(_toolbar);
  }

  InitMenu();

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
  netwm.setDesktopViewport(screen_info.rootWindow(), 0, 0);
  netwm.setActiveWindow(screen_info.rootWindow(), None);
  updateWorkareaHint();
  updateDesktopNamesHint();

  Atom supported[] = {
    netwm.clientList(),
    netwm.clientListStacking(),
    netwm.numberOfDesktops(),
    // _NET_DESKTOP_GEOMETRY is not supported
    // _NET_DESKTOP_VIEWPORT is not supported
    netwm.currentDesktop(),
    netwm.desktopNames(),
    netwm.activeWindow(),
    netwm.workarea(),
    // _NET_VIRTUAL_ROOTS   is not supported
    // _NET_SHOWING_DESKTOP is not supported

    netwm.closeWindow(),
    netwm.moveresizeWindow(),
    // _NET_WM_MOVERESIZE is not supported

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
    // _NET_WM_STATE_STICKY is not supported
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
    // _NET_WM_ACTION_STICK is not supported
    netwm.wmActionMaximizeHorz(),
    netwm.wmActionMaximizeVert(),
    netwm.wmActionFullscreen(),
    netwm.wmActionChangeDesktop(),
    netwm.wmActionClose(),

    netwm.wmStrut()
    // _NET_WM_STRUT_PARTIAL is not supported
    // _NET_WM_ICON_GEOMETRY is not supported
    // _NET_WM_ICON          is not supported
    // _NET_WM_PID           is not supported
    // _NET_WM_HANDLED_ICONS is not supported
    // _NET_WM_USER_TIME     is not supported

    // _NET_WM_PING          is not supported
  };

  netwm.setSupported(screen_info.rootWindow(), supported,
                     sizeof(supported) / sizeof(Atom));

  blackbox->XGrabServer();

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

  blackbox->XUngrabServer();

  updateClientListHint();
  restackWindows();
}


BScreen::~BScreen(void) {
  if (! managed) return;

  blackbox->removeEventHandler(screen_info.rootWindow());

  bt::PixmapCache::release(geom_pixmap);

  if (geom_window != None)
    XDestroyWindow(blackbox->XDisplay(), geom_window);
  XDestroyWindow(blackbox->XDisplay(), empty_window);

  std::for_each(workspacesList.begin(), workspacesList.end(),
                bt::PointerAssassin());

  delete configmenu;
  delete _iconmenu;
  delete rootmenu;
  delete workspacemenu;
  delete _windowmenu;

  delete _slit;
  delete _toolbar;

  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                   blackbox->netwm().supportingWMCheck());
  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                   blackbox->netwm().supported());
  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                   blackbox->netwm().numberOfDesktops());
  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                   blackbox->netwm().desktopGeometry());
  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                   blackbox->netwm().desktopViewport());
  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                   blackbox->netwm().currentDesktop());
  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                   blackbox->netwm().activeWindow());
  blackbox->netwm().removeProperty(screen_info.rootWindow(),
                                   blackbox->netwm().workarea());
}


void BScreen::updateGeomWindow(void) {
  bt::Rect geomr = bt::textRect(screen_info.screenNumber(),
                                _resource.windowStyle()->font,
                                "m:mmmm    m:mmmm");
  geom_w = geomr.width() + (_resource.bevelWidth() * 2);
  geom_h = geomr.height() + (_resource.bevelWidth() * 2);

  if (geom_window == None) {
    XSetWindowAttributes setattrib;
    unsigned long mask = CWBorderPixel | CWColormap | CWSaveUnder;
    setattrib.border_pixel =
      _resource.borderColor()->pixel(screen_info.screenNumber());
    setattrib.colormap = screen_info.colormap();
    setattrib.save_under = True;

    geom_window =
      XCreateWindow(blackbox->XDisplay(), screen_info.rootWindow(),
                    0, 0, geom_w, geom_h, _resource.borderWidth(),
                    screen_info.depth(), InputOutput,
                    screen_info.visual(), mask, &setattrib);
  } else {
    XSetWindowBorderWidth(blackbox->XDisplay(), geom_window,
                          _resource.borderWidth());
    XSetWindowBorder(blackbox->XDisplay(), geom_window,
                     _resource.borderColor()->pixel(screen_info.screenNumber()));
  }

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
}


void BScreen::reconfigure(void) {
  LoadStyle();

  updateGeomWindow();

  if (_toolbar) _toolbar->reconfigure();
  if (_slit) _slit->reconfigure();

  {
    BlackboxWindowList::iterator it = windowList.begin(),
                                end = windowList.end();
    for (; it != end; ++it)
      if (*it) (*it)->reconfigure();
  }

  InitMenu();

  configmenu->reconfigure();
  rootmenu->reconfigure();
  workspacemenu->reconfigure();
  if (_windowmenu)
    _windowmenu->reconfigure();

  {
    WorkspaceList::iterator it = workspacesList.begin(),
                           end = workspacesList.end();
    for (; it != end; ++it)
      (*it)->menu()->reconfigure();
  }
}


void BScreen::rereadMenu(void) {
  InitMenu();
  rootmenu->reconfigure();
}


void BScreen::LoadStyle(void) {
  _resource.loadStyle(this, blackbox->resource().styleFilename());

  if (! _resource.rootCommand().empty())
    bt::bexec(_resource.rootCommand(), screen_info.displayString());
}


unsigned int BScreen::addWorkspace(void) {
  Workspace *wkspc = new Workspace(this, workspacesList.size());
  workspacesList.push_back(wkspc);

  workspacemenu->insertWorkspace(wkspc);

  if (_toolbar) _toolbar->reconfigure();

  blackbox->netwm().setNumberOfDesktops(screen_info.rootWindow(),
                                        workspacesList.size());
  updateDesktopNamesHint();

  return workspacesList.size();
}


unsigned int BScreen::removeLastWorkspace(void) {
  if (workspacesList.size() == 1)
    return 1;

  Workspace *workspace = workspacesList.back();
  workspacesList.pop_back();

  if (current_workspace == workspace->id())
    setCurrentWorkspace(workspace->id() - 1);

  BlackboxWindowList::iterator it, end = windowList.end();
  for (it = windowList.begin(); it != end; ++it) {
    BlackboxWindow *win = *it;
    if (win->workspace() == workspace->id())
      win->setWorkspace(workspace->id() - 1);
  }

  workspacemenu->removeWorkspace(workspace->id());
  delete workspace;

  if (_toolbar) _toolbar->reconfigure();

  blackbox->netwm().setNumberOfDesktops(screen_info.rootWindow(),
                                         workspacesList.size());
  updateDesktopNamesHint();

  return workspacesList.size();
}


void BScreen::setCurrentWorkspace(unsigned int id) {
  if (id == current_workspace) return;

  assert(id < workspacesList.size());

  blackbox->XGrabServer();

  // show the empty window... this will prevent unnecessary exposure
  // of the root window
  XMapWindow(blackbox->XDisplay(), empty_window);

  {
    workspacemenu->setWorkspaceChecked(current_workspace, false);

    // withdraw windows in reverse order to minimize the number of
    // Expose events
    StackingList::reverse_iterator it, end = stackingList.rend();
    for (it = stackingList.rbegin(); it != end; ++it) {
      BlackboxWindow *win = dynamic_cast<BlackboxWindow *>(*it);
      if (win && win->workspace() == current_workspace)
        win->hide();
    }
  }

  current_workspace = id;

  {
    workspacemenu->setWorkspaceChecked(current_workspace, true);

    StackingList::iterator it, end = stackingList.end();
    for (it = stackingList.begin(); it != end; ++it) {
      BlackboxWindow *win = dynamic_cast<BlackboxWindow *>(*it);
      if (win && win->workspace() == current_workspace)
        win->show();
    }
  }

  blackbox->netwm().setCurrentDesktop(screen_info.rootWindow(),
                                      current_workspace);

  XUnmapWindow(blackbox->XDisplay(), empty_window);

  blackbox->XUngrabServer();
}


void BScreen::addWindow(Window w) {
  manageWindow(w);
  BlackboxWindow *win = blackbox->findWindow(w);
  if (win) updateClientListHint();
}


void BScreen::manageWindow(Window w) {
  XWMHints *wmhints = XGetWMHints(blackbox->XDisplay(), w);
  bool slit_client = (wmhints && (wmhints->flags & StateHint) &&
                      wmhints->initial_state == WithdrawnState);
  if (wmhints) XFree(wmhints);

  if (slit_client) {
    if (!_slit) createSlit();
    _slit->addClient(w);
    return;
  }

  (void) new BlackboxWindow(blackbox, w, this);
  // verify that we have managed the window
  BlackboxWindow *win = blackbox->findWindow(w);
  if (! win) return;

  if (win->workspace() >= _resource.numberOfWorkspaces() &&
      win->workspace() != bt::BSENTINEL)
    win->setWorkspace(current_workspace);

  Workspace *workspace = getWorkspace(win->workspace());
  if (!workspace) {
    win->setWorkspace(bt::BSENTINEL);
    win->setWindowNumber(bt::BSENTINEL);
  } else {
    workspace->addWindow(win);
  }

  bool place_window = true;
  if (blackbox->startingUp()
      || ((win->isTransient() || win->windowType() == WindowTypeDesktop
           || (win->wmNormalHints().flags & (PPosition|USPosition)))
          && win->clientRect().intersects(screen_info.rect())))
    place_window = false;
  if (place_window) placeWindow(win);

  windowList.push_back(win);

  // insert window at the top of the stack
  stackingList.insert(win);
  if (!blackbox->startingUp())
    restackWindows();

  // 'show' window in the appropriate state
  switch (blackbox->startingUp()
          ? win->currentState()
          : win->wmHints().initial_state) {
  case IconicState:
    win->iconify();
    break;

  case WithdrawnState:
    win->hide();
    break;

  default:
    win->show();
    break;
  } // switch

  // focus the new window if appropriate
  switch (win->windowType()) {
  case WindowTypeDesktop:
  case WindowTypeDock:
    // these types should not be focused when managed
    break;

  default:
    if (!blackbox->startingUp() &&
        (!blackbox->activeScreen() || blackbox->activeScreen() == this) &&
        (win->isTransient() || resource().doFocusNew())) {
      XSync(blackbox->XDisplay(), False); // make sure the frame is mapped..
      win->setInputFocus();
      break;
    }
  }
}


void BScreen::releaseWindow(BlackboxWindow *w) {
  unmanageWindow(w);
  updateClientListHint();
  updateClientListStackingHint();
}


void BScreen::unmanageWindow(BlackboxWindow *win) {
  win->restore();

  // pass focus to the next appropriate window
  if (win->isFocused() && blackbox->running())
    focusFallback(win);

  if (blackbox->getFocusedWindow() == win)
    blackbox->setFocusedWindow((BlackboxWindow *) 0);

  if (win->isIconic()) {
    _iconmenu->removeItem(win->windowNumber());
  } else {
    Workspace *workspace = getWorkspace(win->workspace());
    if (workspace)
      workspace->menu()->removeItem(win->windowNumber());
  }

  windowList.remove(win);
  stackingList.remove(win);

  /*
    some managed windows can also be window group controllers.  when
    unmanaging such windows, we should also delete the window group.
  */
  BWindowGroup *group = blackbox->findWindowGroup(win->getClientWindow());
  delete group;

  delete win;
}


void BScreen::raiseWindow(StackEntity *entity) {
  StackEntity *top = entity;

  BlackboxWindow *win = dynamic_cast<BlackboxWindow *>(top);
  if (win) {
    // walk up the transient_for's to the window that is not a transient
    while (win->isTransient() && win->getTransientFor())
      win = win->getTransientFor();

    if (win->isFullScreen() && win->layer() != StackingList::LayerFullScreen) {
      // move full-screen windows over all other windows when raising
      changeLayer(win, StackingList::LayerFullScreen);
      return;
    }

    top = win;
  }

  // find the first window above us (if any)
  StackEntity *above = 0;
  {
    StackingList::const_iterator begin = stackingList.begin(),
                                 layer = stackingList.layer(top->layer()),
                                    it = layer;
    if (*it == top) {
      // entity already on top of layer
      return;
    }

    for (--it; it != begin; --it) {
      if (*it) {
        above = *it;
        break;
      }
    }
  }

  // restack the window
  stackingList.raise(top);
  if (win) {
    // ... with all transients above
    raiseTransients(win);
  }

  WindowStack stack;
  bool raise = true;

  if (above) {
    // found another entity above the one we are raising
    stack.push_back(above->windowID());
    raise = false;
  }

  if (win) {
    // put transients into window stack
    stackTransients(win, stack);
  }
  stack.push_back(top->windowID());

  if (raise) {
    // WindowStack doesn't have push_front, so do this the hardway
    WindowStack stack2;
    stack2.reserve(stack.size() + 1);

    stack2.push_back(empty_window);
    const WindowStack::iterator &end = stack.end();
    WindowStack::iterator it;
    for (it= stack.begin(); it != end; ++it)
      stack2.push_back(*it);

    stack = stack2;
  }
  XRestackWindows(blackbox->XDisplay(), &stack[0], stack.size());

  updateClientListStackingHint();
}


void BScreen::lowerWindow(StackEntity *entity) {
  StackEntity *top = entity;

  BlackboxWindow *win = dynamic_cast<BlackboxWindow *>(top);
  if (win) {
    // walk up the transient_for's to the window that is not a transient
    while (win->isTransient() && win->getTransientFor())
      win = win->getTransientFor();
    top = win;
  }

  // find window at the bottom of the layer (if any)
  StackEntity *above = 0;
  {
    StackingList::const_iterator it, layer = stackingList.layer(top->layer()),
                                       end = stackingList.end();
    it = std::find(layer, end, (StackEntity *) 0);
    assert(it != end);

    if (*--it == top) {
      // entity already on bottom of layer
      return;
    }

    for (; it != layer; --it) {
      if (*it && *it != top) {
        above = *it;
        break;
      }
    }
  }

  // restack the window
  if (win) {
    // ... with all transients above
    lowerTransients(win);
  }
  stackingList.lower(top);

  WindowStack stack;
  bool lower = true;

  if (above) {
    // found another entity above the one we are lowering
    stack.push_back(above->windowID());
    lower = false;
  }

  if (win) {
    // put transients into window stack
    stackTransients(win, stack);
  }
  stack.push_back(top->windowID());

  if (lower)
    XLowerWindow(blackbox->XDisplay(), stack.front());
  XRestackWindows(blackbox->XDisplay(), &stack[0], stack.size());

  updateClientListStackingHint();
}


void BScreen::restackWindows(void) {
  WindowStack stack;
  stack.push_back(empty_window);

  StackingList::const_iterator it, end = stackingList.end();
  for (it = stackingList.begin(); it != end; ++it)
    if (*it) stack.push_back((*it)->windowID());

  XRestackWindows(blackbox->XDisplay(), &stack[0], stack.size());

  updateClientListStackingHint();
}


void BScreen::changeLayer(StackEntity *entity, StackingList::Layer new_layer) {
  stackingList.changeLayer(entity, new_layer);
  restackWindows();
}


void BScreen::changeWorkspace(BlackboxWindow *win, unsigned int id) {
  Workspace *workspace = getWorkspace(win->workspace());
  assert(workspace != 0);

  workspace->removeWindow(win);
  workspace = getWorkspace(id);
  assert(workspace != 0);
  workspace->addWindow(win);

  if (current_workspace != win->workspace())
    win->hide();
  else if (!win->isVisible())
    win->show();
}


void BScreen::propagateWindowName(const BlackboxWindow * const win) {
  Workspace *workspace = getWorkspace(win->workspace());
  if (!workspace) return;

  const std::string s = bt::ellideText(win->getTitle(), 60, "...");
  workspace->menu()->changeItem(win->windowNumber(), s);

  if (_toolbar && blackbox->getFocusedWindow() == win)
    _toolbar->redrawWindowLabel();
}


void BScreen::nextFocus(void) {
  BlackboxWindow *focused = blackbox->getFocusedWindow(),
                    *next = 0;
  BlackboxWindowList::iterator it, end = windowList.end();

  if (focused && focused->workspace() == current_workspace &&
      focused->getScreen()->screen_info.screenNumber() ==
      screen_info.screenNumber()) {
    it = std::find(windowList.begin(), end, focused);
    assert(it != end);
    for (; it != end; ++it) {
      next = *it;
      if (next && next != focused && next->workspace() == current_workspace &&
          next->setInputFocus()) {
        // we found our new focus target
        next->setInputFocus();
        raiseWindow(next);
        break;
      }
      next = 0;
    }
  }
  if (!next) {
    for (it = windowList.begin(); it != end; ++it) {
      next = *it;
      if (next && next->workspace() == current_workspace &&
          next->setInputFocus()) {
        // we found our new focus target
        raiseWindow(next);
        break;
      }
    }
  }
}


void BScreen::prevFocus(void) {
  BlackboxWindow *focused = blackbox->getFocusedWindow(),
                    *next = 0;
  BlackboxWindowList::reverse_iterator it, end = windowList.rend();

  if (focused && focused->workspace() == current_workspace &&
      focused->getScreen()->screen_info.screenNumber() ==
      screen_info.screenNumber()) {
    it = std::find(windowList.rbegin(), end, focused);
    assert(it != end);
    for (; it != end; ++it) {
      next = *it;
      if (next && next != focused && next->workspace() == current_workspace &&
          next->setInputFocus()) {
        // we found our new focus target
        next->setInputFocus();
        raiseWindow(next);
        break;
      }
      next = 0;
    }
  }
  if (!next) {
    for (it = windowList.rbegin(); it != end; ++it) {
      next = *it;
      if (next && next->workspace() == current_workspace &&
          next->setInputFocus()) {
        // we found our new focus target
        raiseWindow(next);
        break;
      }
    }
  }
}


void BScreen::raiseFocus(void) {
  BlackboxWindow *focused = blackbox->getFocusedWindow();
  if (! focused || focused->getScreen() != this)
    return;

  raiseWindow(focused);
}


void BScreen::InitMenu(void) {
  if (rootmenu) {
    rootmenu->clear();
  } else {
    rootmenu = new Rootmenu(*blackbox, screen_info.screenNumber(), this);
    rootmenu->showTitle();
  }
  bool defaultMenu = True;

  if (blackbox->resource().menuFilename()) {
    FILE *menu_file = fopen(blackbox->resource().menuFilename(), "r");

    if (!menu_file) {
      perror(blackbox->resource().menuFilename());
    } else {
      if (feof(menu_file)) {
        fprintf(stderr, "%s: menu file '%s' is empty\n",
                blackbox->applicationName().c_str(),
                blackbox->resource().menuFilename());
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
    rootmenu->setTitle("Blackbox");

    rootmenu->insertFunction("xterm", BScreen::Execute, "xterm");
    rootmenu->insertFunction("Restart", BScreen::Restart);
    rootmenu->insertFunction("Exit", BScreen::Exit);
  } else {
    blackbox->saveMenuFilename(blackbox->resource().menuFilename());
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

    case 328: // sep
      menu->insertSeparator();
      break;

    case 333: // nop
      if (! *label)
        label[0] = '\0';
      menu->insertItem(label);

      break;

    case 421: // exec
      if (! (*label && *command)) {
        fprintf(stderr,
                "%s: [exec] error, no menu label and/or command defined\n",
                blackbox->applicationName().c_str());
        continue;
      }

      menu->insertFunction(label, BScreen::Execute, command);
      break;

    case 442: // exit
      if (! *label) {
        fprintf(stderr, "%s: [exit] error, no menu label defined\n",
                blackbox->applicationName().c_str());
        continue;
      }

      menu->insertFunction(label, BScreen::Exit);
      break;

    case 561: { // style
      if (! (*label && *command)) {
        fprintf(stderr,
                "%s: [style] error, no menu label and/or filename defined\n",
                blackbox->applicationName().c_str());
        continue;
      }

      std::string style = bt::expandTilde(command);
      menu->insertFunction(label, BScreen::SetStyle, style.c_str());
      break;
    }

    case 630: // config
      if (! *label) {
        fprintf(stderr, "%s: [config] error, no label defined",
                blackbox->applicationName().c_str());
        continue;
      }

      menu->insertItem(label, configmenu);
      break;

    case 740: { // include
      if (! *label) {
        fprintf(stderr, "%s: [include] error, no filename defined\n",
                blackbox->applicationName().c_str());
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
        fprintf(stderr, "%s: [include] error: '%s' is not a regular file\n",
                blackbox->applicationName().c_str(), newfile.c_str());
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
        fprintf(stderr, "%s: [submenu] error, no menu label defined\n",
                blackbox->applicationName().c_str());
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
        fprintf(stderr, "%s: [restart] error, no menu label defined\n",
                blackbox->applicationName().c_str());
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
        fprintf(stderr, "%s: [reconfig] error, no menu label defined\n",
                blackbox->applicationName().c_str());
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
                "%s: [stylesdir/stylesmenu] error, no directory defined\n",
                blackbox->applicationName().c_str());
        continue;
      }

      char *directory = ((newmenu) ? command : label);

      std::string stylesdir = bt::expandTilde(directory);

      struct stat statbuf;

      if (stat(stylesdir.c_str(), &statbuf) == -1) {
        fprintf(stderr,
                "%s: [stylesdir/stylesmenu] error, '%s' does not exist\n",
                blackbox->applicationName().c_str(), stylesdir.c_str());
        continue;
      }
      if (! S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr,
                "%s: [stylesdir/stylesmenu] error, '%s' is not a directory\n",
                blackbox->applicationName().c_str(), stylesdir.c_str());
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
        fprintf(stderr, "%s: [workspaces] error, no menu label defined\n",
                blackbox->applicationName().c_str());
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
    unmanageWindow(windowList.back());

  if (_slit)
    _slit->shutdown();
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

  char label[80];
  sprintf(label, "X:%4d    Y:%4d", x, y);

  XClearWindow(blackbox->XDisplay(), geom_window);

  bt::Pen pen(screen_info.screenNumber(),
              _resource.windowStyle()->l_text_focus);
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

  char label[80];
  sprintf(label, "W:%4u    H:%4u", gx, gy);

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
  if (index == bt::BSENTINEL) return 0;
  assert(index < workspacesList.size());
  return workspacesList[index];
}


void BScreen::clientMessageEvent(const XClientMessageEvent * const event) {
  if (event->format != 32) return;

  if (event->message_type == blackbox->netwm().numberOfDesktops()) {
    unsigned int number = event->data.l[0];
    const unsigned int wkspc_count = _resource.numberOfWorkspaces();
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
    const unsigned int workspace = event->data.l[0];
    if (workspace < _resource.numberOfWorkspaces() &&
        workspace != current_workspace)
      setCurrentWorkspace(workspace);
  }
}


void BScreen::buttonPressEvent(const XButtonEvent * const event) {
  if (event->button == 1) {
    XInstallColormap(blackbox->XDisplay(), screen_info.colormap());

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


void BScreen::configureRequestEvent(const XConfigureRequestEvent * const event)
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
    names += (*it)->name() + '\0';

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

  // we store windows in top-to-bottom order, but the EWMH wants
  // bottom-to-top...
  StackingList::const_reverse_iterator it = stackingList.rbegin(),
                                      end = stackingList.rend();
  for (; it != end; ++it) {
    const BlackboxWindow * const win = dynamic_cast<BlackboxWindow *>(*it);
    if (win) stack.push_back(win->getClientWindow());
  }

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
    if ((*wit)->name() != *it)
      (*wit)->setName(*it);
  }

  if (names.size() < workspacesList.size())
    updateDesktopNamesHint();
}


BlackboxWindow *BScreen::getWindow(unsigned int workspace, unsigned int id) {
  StackingList::const_iterator it = stackingList.begin(),
                              end = stackingList.end();
  for (; it != end; ++it) {
    BlackboxWindow * const win = dynamic_cast<BlackboxWindow *>(*it);
    if (win && win->workspace() == workspace && win->windowNumber() == id)
      return win;
  }
  assert(false); // should not happen
  return 0;
}


void BScreen::focusFallback(const BlackboxWindow *old_window) {
  BlackboxWindow *newfocus = 0;

  Workspace *workspace = getWorkspace(old_window->workspace());
  if (!workspace)
    workspace = getWorkspace(current_workspace);

  if (workspace->id() == current_workspace) {
    // The window is on the visible workspace.

    if (old_window && old_window->isTransient()) {
      // 1. if it's a transient, then try to focus its parent
      newfocus = old_window->getTransientFor();

      if (! newfocus ||
          newfocus->isIconic() || // do not focus icons
          newfocus->workspace() != workspace->id() || // or other workspaces
          ! newfocus->setInputFocus())
        newfocus = 0;
    }

    if (! newfocus) {
      // 2. try to focus the top window in the same layer as old_window
      const BlackboxWindow * const zero = 0;
      StackingList::iterator it = stackingList.layer(old_window->layer()),
                            end = std::find(it, stackingList.end(), zero);
      assert(it != stackingList.end() && end != stackingList.end());
      for (; it != end; ++it) {
        BlackboxWindow * const tmp = dynamic_cast<BlackboxWindow *>(*it);
        if (tmp && tmp->setInputFocus()) {
          // we found our new focus target
          newfocus = tmp;
          break;
        }
      }
    }

    if (!newfocus && !stackingList.empty()) {
      // 3. focus the top-most window in the stack (regardless of layer)
      newfocus = dynamic_cast<BlackboxWindow *>(stackingList.front());
    }

    blackbox->setFocusedWindow(newfocus);
  } else {
    // The window is not on the visible workspace.

    if (old_window && workspace->lastFocusedWindow() == old_window) {
      // The window was the last-focus target, so we need to replace it.
      BlackboxWindow *win = (BlackboxWindow *) 0;
      if (! stackingList.empty())
        win = dynamic_cast<BlackboxWindow *>(stackingList.front());
      workspace->setLastFocusedWindow(win);
    }
  }
}


/*
 * puts the transients of win into the stack. windows are stacked
 * above the window before it in the stackvector being iterated,
 * meaning stack[0] is on bottom, stack[1] is above stack[0], stack[2]
 * is above stack[1], etc...
 */
void BScreen::raiseTransients(const BlackboxWindow * const win) {
  if (win->getTransients().empty())
    return; // nothing to do

  // put win's transients in the stack
  BlackboxWindowList::const_reverse_iterator it,
    end = win->getTransients().rend();
  for (it = win->getTransients().rbegin(); it != end; ++it) {
    BlackboxWindow *w = *it;
    if (w->workspace() == current_workspace) {
      stackingList.raise(w);
      raiseTransients(w);
    }
  }
}


void BScreen::lowerTransients(const BlackboxWindow * const win) {
  if (win->getTransients().empty())
    return; // nothing to do

  // put win's transients in the stack
  BlackboxWindowList::const_reverse_iterator it,
    end = win->getTransients().rend();
  for (it = win->getTransients().rbegin(); it != end; ++it) {
    BlackboxWindow *w = *it;
    if (w->workspace() == current_workspace) {
      lowerTransients(w);
      stackingList.lower(w);
    }
  }
}


void BScreen::stackTransients(const BlackboxWindow * const win,
                              WindowStack &stack) {
  const BlackboxWindowList &list = win->getTransients();
  if (list.empty()) return;

  BlackboxWindowList::const_reverse_iterator it, end = list.rend();
  for (it = list.rbegin(); it != end; ++it) {
    stackTransients(*it, stack);
    stack.push_back((*it)->getFrameWindow());
  }
}


void BScreen::placeWindow(BlackboxWindow *win) {
  const bt::Rect &avail = availableArea();
  bt::Rect new_win(avail.x(), avail.y(),
                   win->frameRect().width(), win->frameRect().height());
  bool placed = False;

  switch (_resource.placementPolicy()) {
  case RowSmartPlacement:
  case ColSmartPlacement:
    placed = smartPlacement(win->workspace(), new_win, avail);
    break;
  default:
    break; // handled below
  } // switch

  if (placed == False) {
    cascadePlacement(new_win, avail);
    cascade_x += win->getTitleHeight() +
                 (_resource.borderWidth() * 2);
    cascade_y += win->getTitleHeight() +
                 (_resource.borderWidth() * 2);
  }

  if (new_win.right() > avail.right())
    new_win.setX(avail.left());
  if (new_win.bottom() > avail.bottom())
    new_win.setY(avail.top());

  win->configure(new_win.x(), new_win.y(), new_win.width(), new_win.height());
}


bool BScreen::cascadePlacement(bt::Rect &win,
                               const bt::Rect &avail) {
  if (cascade_x > (avail.width() / 2) ||
      cascade_y > (avail.height() / 2))
    cascade_x = cascade_y = 32;

  if (cascade_x == 32) {
    cascade_x += avail.x();
    cascade_y += avail.y();
  }

  win.setPos(cascade_x, cascade_y);

  if (win.right()  > avail.right() ||
      win.bottom() > avail.bottom()) {
    cascade_x = cascade_y = 32;

    cascade_x += avail.x();
    cascade_y += avail.y();

    win.setPos(cascade_x, cascade_y);
  }

  return True;
}


bool BScreen::smartPlacement(unsigned int workspace, bt::Rect& rect,
                             const bt::Rect& avail) {
  // constants
  const bool row_placement =
    (_resource.placementPolicy() == RowSmartPlacement);
  const bool leftright =
    (_resource.rowPlacementDirection() == LeftRight);
  const bool topbottom =
    (_resource.colPlacementDirection() == TopBottom);
  const bool ignore_shaded = _resource.placementIgnoresShaded();

  const int border_width = _resource.borderWidth();
  const int left_border   = leftright ? 0 : -border_width-1;
  const int top_border    = topbottom ? 0 : -border_width-1;
  const int right_border  = leftright ? border_width+1 : 0;
  const int bottom_border = topbottom ? border_width+1 : 0;

  StackingList::const_iterator w_it, w_end;

  /*
    build a sorted vector of x and y grid boundaries

    note: we use one vector to reduce the number of allocations
    std::vector must do.. we allocate as much memory as we would need
    in the worst case scenario and work with that
  */
  std::vector<int> coords(stackingList.size() * 4 + 4);
  std::vector<int>::iterator
    x_begin = coords.begin(),
    x_end   = x_begin,
    y_begin = coords.begin() + stackingList.size() * 2 + 2,
    y_end   = y_begin;

  {
    std::vector<int>::iterator x_it = x_begin, y_it = y_begin;

    *x_it++ = avail.left();
    *x_it++ = avail.right();
    x_end += 2;

    *y_it++ = avail.top();
    *y_it++ = avail.bottom();
    y_end += 2;


    for (w_it  = stackingList.begin(), w_end = stackingList.end();
         w_it != w_end; ++w_it) {
      const BlackboxWindow * const win =
        dynamic_cast<const BlackboxWindow *>(*w_it);
      if (!win) continue;

      if (win->windowType() == WindowTypeDesktop)
        continue;
      if (win->isIconic())
        continue;
      if (win->workspace() != bt::BSENTINEL && win->workspace() != workspace)
        continue;
      if (ignore_shaded && win->isShaded())
        continue;

      *x_it++ = std::max(win->frameRect().left() + left_border,
                         avail.left());
      *x_it++ = std::min(win->frameRect().right() + right_border,
                         avail.right());
      x_end += 2;

      *y_it++ = std::max(win->frameRect().top() + top_border,
                         avail.top());
      *y_it++ = std::min(win->frameRect().bottom() + bottom_border,
                         avail.bottom());
      y_end += 2;
    }

    assert(x_end <= y_begin);
  }

  std::sort(x_begin, x_end);
  x_end = std::unique(x_begin, x_end);

  std::sort(y_begin, y_end);
  y_end = std::unique(y_begin, y_end);

  // build a distribution grid
  unsigned int gw = x_end - x_begin - 1,
               gh = y_end - y_begin - 1;
  std::bit_vector used_grid(gw * gh);
  std::fill_n(used_grid.begin(), used_grid.size(), false);

  for (w_it = stackingList.begin(), w_end = stackingList.end();
       w_it != w_end; ++w_it) {
    const BlackboxWindow * const win =
      dynamic_cast<const BlackboxWindow *>(*w_it);
    if (!win) continue;

    if (win->windowType() == WindowTypeDesktop)
      continue;
    if (win->isIconic())
      continue;
    if (win->workspace() != bt::BSENTINEL && win->workspace() != workspace)
      continue;
    if (ignore_shaded && win->isShaded())
      continue;

    const int w_left =
      std::max(win->frameRect().left() + left_border,
               avail.left());
    const int w_top =
      std::max(win->frameRect().top() + top_border,
               avail.top());
    const int w_right =
      std::min(win->frameRect().right() + right_border,
               avail.right());
    const int w_bottom =
      std::min(win->frameRect().bottom() + bottom_border,
               avail.bottom());

    // which areas of the grid are used by this window?
    std::vector<int>::iterator l_it = std::find(x_begin, x_end, w_left),
                               r_it = std::find(x_begin, x_end, w_right),
                               t_it = std::find(y_begin, y_end, w_top),
                               b_it = std::find(y_begin, y_end, w_bottom);
    assert(l_it != x_end &&
           r_it != x_end &&
           t_it != y_end &&
           b_it != y_end);

    const unsigned int left   = l_it - x_begin,
                       right  = r_it - x_begin,
                       top    = t_it - y_begin,
                       bottom = b_it - y_begin;

    for (unsigned int gy = top; gy < bottom; ++gy)
      for (unsigned int gx = left; gx < right; ++gx)
        used_grid[(gy * gw) + gx] = true;
  }

  /*
    Attempt to fit the window into any of the empty areas in the grid.
    The exact order is dependent upon the users configuration (as
    shown below).

    row placement:
    - outer -> vertical axis
    - inner -> horizontal axis

    col placement:
    - outer -> horizontal axis
    - inner -> vertical axis
  */

  int gx, gy;
  int &outer = row_placement ? gy : gx;
  int &inner = row_placement ? gx : gy;
  const int outer_delta = row_placement
                          ? (topbottom ? 1 : -1)
                          : (leftright ? 1 : -1);
  const int inner_delta = row_placement
                          ? (leftright ? 1 : -1)
                          : (topbottom ? 1 : -1);
  const int outer_begin = row_placement
                          ? (topbottom ? 0 : static_cast<int>(gh) - 1)
                          : (leftright ? 0 : static_cast<int>(gw) - 1);
  const int outer_end   = row_placement
                          ? (topbottom ? static_cast<int>(gh) : -1)
                          : (leftright ? static_cast<int>(gw) : -1);
  const int inner_begin = row_placement
                          ? (leftright ? 0 : static_cast<int>(gw) - 1)
                          : (topbottom ? 0 : static_cast<int>(gh) - 1);
  const int inner_end   = row_placement
                          ? (leftright ? static_cast<int>(gw) : -1)
                          : (topbottom ? static_cast<int>(gh) : -1);

  bt::Rect where;
  bool fit = false;
  for (outer = outer_begin; ! fit && outer != outer_end;
       outer += outer_delta) {
    for (inner = inner_begin; ! fit && inner != inner_end;
         inner += inner_delta) {
      // see if the window fits in a single unused area
      if (used_grid[(gy * gw) + gx]) continue;

      where.setCoords(*(x_begin + gx), *(y_begin + gy),
                      *(x_begin + gx + 1), *(y_begin + gy + 1));

      if (where.width()  >= rect.width() &&
          where.height() >= rect.height()) {
        fit = true;
        break;
      }

      /*
        see if we neighboring spaces are unused

        TODO: we should grid fit in the same direction as above,
        instead of always right->left and top->bottom
      */
      int gx2 = gx, gy2 = gy;

      if (rect.width() > where.width()) {
        for (gx2 = gx+1; gx2 < static_cast<int>(gw); ++gx2) {
          if (used_grid[(gy * gw) + gx2]) {
            --gx2;
            break;
          }

          where.setCoords(*(x_begin + gx), *(y_begin + gy),
                          *(x_begin + gx2 + 1), *(y_begin + gy2 + 1));

          if (where.width() >= rect.width()) break;
        }

        if (gx2 >= static_cast<int>(gw)) --gx2;
      }

      if (rect.height() > where.height()) {
        for (gy2 = gy; gy2 < static_cast<int>(gh); ++gy2) {
          if (used_grid[(gy2 * gw) + gx]) {
            --gy2;
            break;
          }

          where.setCoords(*(x_begin + gx), *(y_begin + gy),
                          *(x_begin + gx2 + 1), *(y_begin + gy2 + 1));

          if (where.height() >= rect.height()) break;
        }

        if (gy2 >= static_cast<int>(gh)) --gy2;
      }

      if (where.width()  >= rect.width() &&
          where.height() >= rect.height()) {
        fit = true;

        // make sure all spaces are really available
        for (int gy3 = gy; gy3 <= gy2; ++gy3) {
          for (int gx3 = gx; gx3 <= gx2; ++gx3) {
            if (used_grid[(gy3 * gw) + gx3]) {
              fit = false;
              break;
            }
          }
        }
      }
    }
  }

  if (! fit) {
    const int screen_area = avail.width() * avail.height();
    const int window_area = rect.width() * rect.height();
    if (window_area > screen_area / 8) {
      // center windows that don't fix (except for small windows)
      rect.setPos((avail.x() + avail.width() - rect.width()) / 2,
                  (avail.y() + avail.height() - rect.height()) / 2);
      return true;
    }
    return false;
  }

  // adjust the location() based on left/right and top/bottom placement
  if (! leftright)
    where.setX(where.right() - rect.width() + 1);
  if (! topbottom)
    where.setY(where.bottom() - rect.height() + 1);

  rect.setPos(where.x(), where.y());

  return true;
}


void BScreen::createSlit(void) {
  assert(_slit == 0);

  _slit = new Slit(this);
  stackingList.insert(_slit);
  restackWindows();
}


void BScreen::destroySlit(void) {
  assert(_slit != 0);

  stackingList.remove(_slit);
  delete _slit;
  _slit = 0;
}


void BScreen::createToolbar(void) {
  assert(_toolbar == 0);

  _toolbar = new Toolbar(this);
  stackingList.insert(_toolbar);
  restackWindows();
}


void BScreen::destroyToolbar(void) {
  assert(_toolbar != 0);

  stackingList.remove(_toolbar);
  delete _toolbar;
  _toolbar = 0;
}


Windowmenu *BScreen::windowmenu(BlackboxWindow *win) {
  if (!_windowmenu)
    _windowmenu = new Windowmenu(*blackbox, screen_info.screenNumber());
  _windowmenu->setWindow(win);
  return _windowmenu;
}


void BScreen::addIcon(BlackboxWindow *win) {
  if (win->workspace() != bt::BSENTINEL) {
    Workspace *workspace = getWorkspace(win->workspace());
    assert(workspace != 0);
    workspace->removeWindow(win);
  }

  const std::string s = bt::ellideText(win->getIconTitle(), 60, "...");
  int id = _iconmenu->insertItem(s);
  blackbox->netwm().setWMVisibleIconName(win->getClientWindow(), s);
  win->setWindowNumber(id);
}


void BScreen::removeIcon(BlackboxWindow *win) {
  _iconmenu->removeItem(win->windowNumber());
  blackbox->netwm().removeProperty(win->getClientWindow(),
                                   blackbox->netwm().wmVisibleIconName());

  Workspace *workspace = getWorkspace(current_workspace);
  assert(workspace != 0);
  workspace->addWindow(win);
}


BlackboxWindow *BScreen::icon(unsigned int id) {
  StackingList::const_iterator it = stackingList.begin(),
                              end = stackingList.end();
  for (; it != end; ++it) {
    BlackboxWindow * const win = dynamic_cast<BlackboxWindow *>(*it);
    if (win && win->isIconic() && win->windowNumber() == id)
      return win;
  }
  assert(false); // should not happen
  return 0;
}
