// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Screen.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2005
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
#include "Slitmenu.hh"
#include "Toolbar.hh"
#include "Toolbarmenu.hh"
#include "Window.hh"
#include "WindowGroup.hh"
#include "Windowmenu.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"

#include <Pen.hh>
#include <PixmapCache.hh>
#include <Unicode.hh>

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
  screen_info(bb->display().screenInfo(scrn)), _blackbox(bb),
  _resource(bb->resource().screenResource(scrn))
{
  running = true;
  XErrorHandler old = XSetErrorHandler((XErrorHandler) anotherWMRunning);
  XSelectInput(screen_info.display().XDisplay(),
               screen_info.rootWindow(),
               PropertyChangeMask |
               StructureNotifyMask |
               SubstructureRedirectMask |
	       ButtonPressMask);

  XSync(screen_info.display().XDisplay(), False);
  XSetErrorHandler((XErrorHandler) old);

  managed = running;
  if (! managed) {
    fprintf(stderr,
            "%s: another window manager is already running on display '%s'\n",
            _blackbox->applicationName().c_str(),
            DisplayString(_blackbox->XDisplay()));
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
  printf("%s: managing screen %u using %s visual 0x%lx, depth %d\n",
         _blackbox->applicationName().c_str(), screen_info.screenNumber(),
         visual_classes[screen_info.visual()->c_class],
         XVisualIDFromVisual(screen_info.visual()), screen_info.depth());

  _blackbox->insertEventHandler(screen_info.rootWindow(), this);

  cascade_x = cascade_y = ~0;

  _rootmenu = 0;
  _windowmenu = 0;

  XDefineCursor(_blackbox->XDisplay(), screen_info.rootWindow(),
                _blackbox->resource().cursors().pointer);

  // start off full screen, top left.
  usableArea.setSize(screen_info.width(), screen_info.height());

  LoadStyle();

  geom_pixmap = None;
  geom_visible = False;
  geom_window = None;
  updateGeomWindow();

  empty_window =
    XCreateSimpleWindow(_blackbox->XDisplay(), screen_info.rootWindow(),
                        0, 0, screen_info.width(), screen_info.height(), 0,
                        0l, 0l);
  XSetWindowBackgroundPixmap(_blackbox->XDisplay(), empty_window, None);

  no_focus_window =
    XCreateSimpleWindow(_blackbox->XDisplay(), screen_info.rootWindow(),
                        screen_info.width(), screen_info.height(), 1, 1,
                        0, 0l, 0l);
  XSelectInput(_blackbox->XDisplay(), no_focus_window, NoEventMask);
  XMapWindow(_blackbox->XDisplay(), no_focus_window);

  _iconmenu =
    new Iconmenu(*_blackbox, screen_info.screenNumber(), this);
  _slitmenu =
    new Slitmenu(*_blackbox, screen_info.screenNumber(), this);
  _toolbarmenu =
    new Toolbarmenu(*_blackbox, screen_info.screenNumber(), this);
  _workspacemenu =
    new Workspacemenu(*_blackbox, screen_info.screenNumber(), this);

  configmenu =
    new Configmenu(*_blackbox, screen_info.screenNumber(), this);

  if (_resource.workspaceCount() == 0) // there is always 1 workspace
    _resource.setWorkspaceCount(1);

  _workspacemenu->insertIconMenu(_iconmenu);
  for (unsigned int i = 0; i < _resource.workspaceCount(); ++i) {
    Workspace *wkspc = new Workspace(this, i);
    workspacesList.push_back(wkspc);
    _workspacemenu->insertWorkspace(wkspc);
  }

  current_workspace = workspacesList.front()->id();
  _workspacemenu->setWorkspaceChecked(current_workspace, true);

  // the Slit will be created on demand
  _slit = 0;

  _toolbar = 0;
  if (_resource.toolbarOptions().enabled) {
    _toolbar = new Toolbar(this);
    _stackingList.insert(_toolbar);
  }

  InitMenu();

  const bt::EWMH& ewmh = _blackbox->ewmh();
  /*
    ewmh requires the window manager to set a property on a window it creates
    which is the id of that window.  We must also set an equivalent property
    on the root window.  Then we must set _NET_WM_NAME on the child window
    to be the name of the wm.
  */
  ewmh.setSupportingWMCheck(screen_info.rootWindow(), geom_window);
  ewmh.setSupportingWMCheck(geom_window, geom_window);
  ewmh.setWMName(geom_window, bt::toUnicode("Blackbox"));

  ewmh.setCurrentDesktop(screen_info.rootWindow(), 0);
  ewmh.setNumberOfDesktops(screen_info.rootWindow(),
                           workspacesList.size());
  ewmh.setDesktopGeometry(screen_info.rootWindow(),
                          screen_info.width(), screen_info.height());
  ewmh.setDesktopViewport(screen_info.rootWindow(), 0, 0);
  ewmh.setActiveWindow(screen_info.rootWindow(), None);
  updateWorkareaHint();
  updateDesktopNamesHint();

  Atom supported[] = {
    ewmh.clientList(),
    ewmh.clientListStacking(),
    ewmh.numberOfDesktops(),
    // _NET_DESKTOP_GEOMETRY is not supported
    // _NET_DESKTOP_VIEWPORT is not supported
    ewmh.currentDesktop(),
    ewmh.desktopNames(),
    ewmh.activeWindow(),
    ewmh.workarea(),
    // _NET_VIRTUAL_ROOTS   is not supported
    // _NET_SHOWING_DESKTOP is not supported

    ewmh.closeWindow(),
    ewmh.moveresizeWindow(),
    // _NET_WM_MOVERESIZE is not supported

    ewmh.wmName(),
    ewmh.wmVisibleName(),
    ewmh.wmIconName(),
    ewmh.wmVisibleIconName(),
    ewmh.wmDesktop(),

    ewmh.wmWindowType(),
    ewmh.wmWindowTypeDesktop(),
    ewmh.wmWindowTypeDock(),
    ewmh.wmWindowTypeToolbar(),
    ewmh.wmWindowTypeMenu(),
    ewmh.wmWindowTypeUtility(),
    ewmh.wmWindowTypeSplash(),
    ewmh.wmWindowTypeDialog(),
    ewmh.wmWindowTypeNormal(),

    ewmh.wmState(),
    ewmh.wmStateModal(),
    // _NET_WM_STATE_STICKY is not supported
    ewmh.wmStateMaximizedVert(),
    ewmh.wmStateMaximizedHorz(),
    ewmh.wmStateShaded(),
    ewmh.wmStateSkipTaskbar(),
    ewmh.wmStateSkipPager(),
    ewmh.wmStateHidden(),
    ewmh.wmStateFullscreen(),
    ewmh.wmStateAbove(),
    ewmh.wmStateBelow(),

    ewmh.wmAllowedActions(),
    ewmh.wmActionMove(),
    ewmh.wmActionResize(),
    ewmh.wmActionMinimize(),
    ewmh.wmActionShade(),
    // _NET_WM_ACTION_STICK is not supported
    ewmh.wmActionMaximizeHorz(),
    ewmh.wmActionMaximizeVert(),
    ewmh.wmActionFullscreen(),
    ewmh.wmActionChangeDesktop(),
    ewmh.wmActionClose(),

    ewmh.wmStrut()
    // _NET_WM_STRUT_PARTIAL is not supported
    // _NET_WM_ICON_GEOMETRY is not supported
    // _NET_WM_ICON          is not supported
    // _NET_WM_PID           is not supported
    // _NET_WM_HANDLED_ICONS is not supported
    // _NET_WM_USER_TIME     is not supported

    // _NET_WM_PING          is not supported
  };

  ewmh.setSupported(screen_info.rootWindow(), supported,
                     sizeof(supported) / sizeof(Atom));

  _blackbox->XGrabServer();

  unsigned int i, j, nchild;
  Window r, p, *children;
  XQueryTree(_blackbox->XDisplay(), screen_info.rootWindow(), &r, &p,
             &children, &nchild);

  // preen the window list of all icon windows... for better dockapp support
  for (i = 0; i < nchild; i++) {
    if (children[i] == None || children[i] == no_focus_window)
      continue;

    XWMHints *wmhints = XGetWMHints(_blackbox->XDisplay(),
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
    if (children[i] == None || children[i] == no_focus_window)
      continue;

    XWindowAttributes attrib;
    if (XGetWindowAttributes(_blackbox->XDisplay(), children[i], &attrib)) {
      if (attrib.override_redirect) continue;

      if (attrib.map_state != IsUnmapped) {
        manageWindow(children[i]);
      }
    }
  }

  XFree(children);

  _blackbox->XUngrabServer();

  updateClientListHint();
  restackWindows();
}


BScreen::~BScreen(void) {
  if (! managed) return;

  _blackbox->removeEventHandler(screen_info.rootWindow());

  bt::PixmapCache::release(geom_pixmap);

  if (geom_window != None)
    XDestroyWindow(_blackbox->XDisplay(), geom_window);
  XDestroyWindow(_blackbox->XDisplay(), empty_window);

  std::for_each(workspacesList.begin(), workspacesList.end(),
                bt::PointerAssassin());

  delete _rootmenu;
  delete configmenu;

  delete _workspacemenu;
  delete _iconmenu;

  delete _windowmenu;

  delete _slit;
  delete _toolbar;

  _blackbox->ewmh().removeProperty(screen_info.rootWindow(),
                                   _blackbox->ewmh().supportingWMCheck());
  _blackbox->ewmh().removeProperty(screen_info.rootWindow(),
                                   _blackbox->ewmh().supported());
  _blackbox->ewmh().removeProperty(screen_info.rootWindow(),
                                   _blackbox->ewmh().numberOfDesktops());
  _blackbox->ewmh().removeProperty(screen_info.rootWindow(),
                                   _blackbox->ewmh().desktopGeometry());
  _blackbox->ewmh().removeProperty(screen_info.rootWindow(),
                                   _blackbox->ewmh().desktopViewport());
  _blackbox->ewmh().removeProperty(screen_info.rootWindow(),
                                   _blackbox->ewmh().currentDesktop());
  _blackbox->ewmh().removeProperty(screen_info.rootWindow(),
                                   _blackbox->ewmh().activeWindow());
  _blackbox->ewmh().removeProperty(screen_info.rootWindow(),
                                   _blackbox->ewmh().workarea());
}


void BScreen::updateGeomWindow(void) {
  const WindowStyle &style = _resource.windowStyle();
  bt::Rect geomr =
    bt::textRect(screen_info.screenNumber(), style.font,
                 bt::toUnicode("m:mmmm    m:mmmm"));

  geom_w = geomr.width() + (style.label_margin * 2);
  geom_h = geomr.height() + (style.label_margin * 2);

  if (geom_window == None) {
    XSetWindowAttributes setattrib;
    unsigned long mask = CWColormap | CWSaveUnder;
    setattrib.colormap = screen_info.colormap();
    setattrib.save_under = True;

    geom_window =
      XCreateWindow(_blackbox->XDisplay(), screen_info.rootWindow(),
                    0, 0, geom_w, geom_h, 0, screen_info.depth(), InputOutput,
                    screen_info.visual(), mask, &setattrib);
  }

  const bt::Texture &texture =
    (style.focus.label.texture() == bt::Texture::Parent_Relative)
    ? style.focus.title
    : style.focus.label;
  geom_w += (texture.borderWidth() * 2);
  geom_h += (texture.borderWidth() * 2);
  geom_pixmap = bt::PixmapCache::find(screen_info.screenNumber(),
                                      texture,
                                      geom_w,
                                      geom_h,
                                      geom_pixmap);
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
  _rootmenu->reconfigure();
  _workspacemenu->reconfigure();
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
  _rootmenu->reconfigure();
}


void BScreen::LoadStyle(void) {
  _resource.loadStyle(this, _blackbox->resource().styleFilename());

  if (! _resource.rootCommand().empty())
    bt::bexec(_resource.rootCommand(), screen_info.displayString());
}


void BScreen::addWorkspace(void) {
  Workspace *wkspc = new Workspace(this, workspacesList.size());
  workspacesList.push_back(wkspc);
  _workspacemenu->insertWorkspace(wkspc);

  _blackbox->ewmh().setNumberOfDesktops(screen_info.rootWindow(),
                                         workspacesList.size());
  updateDesktopNamesHint();
}


void BScreen::removeLastWorkspace(void) {
  if (workspacesList.size() == 1)
    return;

  Workspace *workspace = workspacesList.back();

  BlackboxWindowList::iterator it = windowList.begin();
  const BlackboxWindowList::iterator end = windowList.end();
  for (; it != end; ++it) {
    BlackboxWindow * const win = *it;
    if (win->workspace() == workspace->id())
      win->setWorkspace(workspace->id() - 1);
  }

  if (current_workspace == workspace->id())
    setCurrentWorkspace(workspace->id() - 1);

  _workspacemenu->removeWorkspace(workspace->id());
  workspacesList.pop_back();
  delete workspace;

  _blackbox->ewmh().setNumberOfDesktops(screen_info.rootWindow(),
                                         workspacesList.size());
  updateDesktopNamesHint();
}


void BScreen::setCurrentWorkspace(unsigned int id) {
  if (id == current_workspace)
    return;

  assert(id < workspacesList.size());

  _blackbox->XGrabServer();

  // show the empty window... this will prevent unnecessary exposure
  // of the root window
  XMapWindow(_blackbox->XDisplay(), empty_window);

  BlackboxWindow * const focused_window = _blackbox->focusedWindow();

  {
    _workspacemenu->setWorkspaceChecked(current_workspace, false);

    // withdraw windows in reverse order to minimize the number of
    // Expose events
    StackingList::const_reverse_iterator it = _stackingList.rbegin();
    const StackingList::const_reverse_iterator end = _stackingList.rend();
    for (; it != end; ++it) {
      BlackboxWindow *win = dynamic_cast<BlackboxWindow *>(*it);
      if (win && win->workspace() == current_workspace)
        win->hide();
    }

    Workspace *workspace = findWorkspace(current_workspace);
    assert(workspace != 0);
    if (focused_window && focused_window->workspace() != bt::BSENTINEL) {
      // remember the window that last had focus
      workspace->setFocusedWindow(focused_window);
    } else {
      workspace->clearFocusedWindow();
    }
  }

  current_workspace = id;

  {
    _workspacemenu->setWorkspaceChecked(current_workspace, true);

    StackingList::const_iterator it = _stackingList.begin();
    const StackingList::const_iterator end = _stackingList.end();
    for (; it != end; ++it) {
      BlackboxWindow *win = dynamic_cast<BlackboxWindow *>(*it);
      if (win && win->workspace() == current_workspace)
        win->show();
    }

    const BlackboxResource &res = _blackbox->resource();
    if (res.focusLastWindowOnWorkspace()
        && (!focused_window
            || focused_window->workspace() != bt::BSENTINEL
            // these window types shouldn't keep focus
            || focused_window->windowType() == WindowTypeDesktop
            || focused_window->windowType() == WindowTypeDock)) {
      Workspace *workspace = findWorkspace(current_workspace);
      assert(workspace != 0);

      if (workspace->focusedWindow()) {
        // focus the window that last had focus
        workspace->focusedWindow()->setInputFocus();
      } else {
        // focus the top-most window in the stack
        for (it = _stackingList.begin(); it != end; ++it) {
          BlackboxWindow * const tmp = dynamic_cast<BlackboxWindow *>(*it);
          if (!tmp
              || !tmp->isVisible()
              || (tmp->workspace() != current_workspace
                  && tmp->workspace() != bt::BSENTINEL))
            continue;
          if (tmp->setInputFocus())
            break;
        }
      }
    }
  }

  _blackbox->ewmh().setCurrentDesktop(screen_info.rootWindow(),
                                      current_workspace);

  XUnmapWindow(_blackbox->XDisplay(), empty_window);

  _blackbox->XUngrabServer();
}


void BScreen::addWindow(Window w) {
  manageWindow(w);
  BlackboxWindow * const win = _blackbox->findWindow(w);
  if (!win) return;

  updateClientListHint();

  // focus the new window if appropriate
  switch (win->windowType()) {
  case WindowTypeDesktop:
  case WindowTypeDock:
    // these types should not be focused when managed
    break;

  default:
    if (!_blackbox->startingUp() &&
        (!_blackbox->activeScreen() || _blackbox->activeScreen() == this) &&
        (win->isTransient() || _blackbox->resource().focusNewWindows())) {
      XSync(_blackbox->XDisplay(), False); // make sure the frame is mapped..
      win->setInputFocus();
      break;
    }
  }
}


void BScreen::manageWindow(Window w) {
  XWMHints *wmhints = XGetWMHints(_blackbox->XDisplay(), w);
  bool slit_client = (wmhints && (wmhints->flags & StateHint) &&
                      wmhints->initial_state == WithdrawnState);
  if (wmhints) XFree(wmhints);

  if (slit_client) {
    if (!_slit) createSlit();
    _slit->addClient(w);
    return;
  }

  (void) new BlackboxWindow(_blackbox, w, this);
  // verify that we have managed the window
  BlackboxWindow *win = _blackbox->findWindow(w);
  if (! win) return;

  if (win->workspace() >= workspaceCount() &&
      win->workspace() != bt::BSENTINEL)
    win->setWorkspace(current_workspace);

  Workspace *workspace = findWorkspace(win->workspace());
  if (!workspace) {
    win->setWorkspace(bt::BSENTINEL);
    win->setWindowNumber(bt::BSENTINEL);
  } else {
    workspace->addWindow(win);
  }

  bool place_window = true;
  if (_blackbox->startingUp()
      || ((win->isTransient() || win->windowType() == WindowTypeDesktop
           || (win->wmNormalHints().flags & (PPosition|USPosition)))
          && win->clientRect().intersects(screen_info.rect())))
    place_window = false;
  if (place_window) placeWindow(win);

  windowList.push_back(win);

  // insert window at the top of the stack
  _stackingList.insert(win);
  if (!_blackbox->startingUp())
    restackWindows();

  // 'show' window in the appropriate state
  switch (_blackbox->startingUp()
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
}


void BScreen::releaseWindow(BlackboxWindow *w) {
  unmanageWindow(w);
  updateClientListHint();
  updateClientListStackingHint();
}


void BScreen::unmanageWindow(BlackboxWindow *win) {
  win->restore();

  if (win->isIconic()) {
    _iconmenu->removeItem(win->windowNumber());
  } else {
    Workspace *workspace = findWorkspace(win->workspace());
    if (workspace) {
      workspace->menu()->removeItem(win->windowNumber());
      if (workspace->focusedWindow() == win)
        workspace->clearFocusedWindow();
    }
  }

  if (_blackbox->running() && win->isFocused()) {
    // pass focus to the next appropriate window
    if (focusFallback(win)) {
      // focus is going somewhere, but we want to avoid dangling pointers
      _blackbox->forgetFocusedWindow();
    } else {
      // explicitly clear the focus
      _blackbox->setFocusedWindow(0);
    }
  }

  windowList.remove(win);
  _stackingList.remove(win);

  delete win;
}


bool BScreen::focusFallback(const BlackboxWindow *win) {
  Workspace *workspace = findWorkspace(win->workspace());
  if (!workspace)
    workspace = findWorkspace(current_workspace);

  if (workspace->id() != current_workspace)
    return false;

  BWindowGroup *group = win->findWindowGroup();
  if (group) {
    // focus the top-most window in the group
    BlackboxWindowList::const_iterator git = group->windows().begin(),
                                      gend = group->windows().end();
    StackingList::iterator it = _stackingList.begin(),
                          end = _stackingList.end();
    for (; it != end; ++it) {
      BlackboxWindow * const tmp = dynamic_cast<BlackboxWindow *>(*it);
      if (!tmp
          || tmp == win
          || std::find(git, gend, tmp) == gend
          || !tmp->isVisible()
          || (tmp->workspace() != current_workspace
              && tmp->workspace() != bt::BSENTINEL))
        continue;
      if (tmp->setInputFocus())
        return true;
    }
  }

  const BlackboxWindow * const zero = 0;

  if (win) {
    if (win->isTransient()) {
      BlackboxWindow * const tmp = win->findTransientFor();
      if (tmp
          && tmp->isVisible()
          && (tmp->workspace() == current_workspace
              || tmp->workspace() == bt::BSENTINEL)
          && tmp->setInputFocus())
        return true;
    }

    // try to focus the top-most window in the same layer as win
    StackingList::iterator it = _stackingList.layer(win->layer()),
                          end = std::find(it, _stackingList.end(), zero);
    assert(it != _stackingList.end() && end != _stackingList.end());
    for (; it != end; ++it) {
      BlackboxWindow * const tmp = dynamic_cast<BlackboxWindow *>(*it);
      if (!tmp)
        break;
      if (tmp == win
          || !tmp->isVisible()
          || (tmp->workspace() != current_workspace
              && tmp->workspace() != bt::BSENTINEL))
        continue;
      if (tmp->setInputFocus())
        return true;
    }
  }

  // focus the top-most window in the stack
  StackingList::iterator it = _stackingList.begin(),
                        end = _stackingList.end();
  for (; it != end; ++it) {
    BlackboxWindow * const tmp = dynamic_cast<BlackboxWindow *>(*it);
    if (!tmp
        || tmp == win
        || !tmp->isVisible()
        || (tmp->workspace() != current_workspace
            && tmp->workspace() != bt::BSENTINEL))
      continue;
    if (tmp->setInputFocus())
      return true;
  }

  return false;
}


/*
  Returns a list of all transients.  This is recursive, so it returns
  all transients of transients as well.
*/
static
BlackboxWindowList buildTransientList(unsigned int current_workspace,
                                      const BlackboxWindow * const win) {
  const BlackboxWindowList &transients = win->transients();
  BlackboxWindowList all = transients;
  BlackboxWindowList::const_iterator it = transients.begin(),
                                    end = transients.end();
  for (; it != end; ++it) {
    const BlackboxWindow * const tmp = *it;
    if (tmp->workspace() == current_workspace
        || tmp->workspace() == bt::BSENTINEL) {
      BlackboxWindowList x = buildTransientList(current_workspace, tmp);
      all.splice(all.end(), x);
    }
  }
  return all;
}


/*
  Raises all windows, preserving the existing stacking order.  Each
  window is placed at the top of the layer it currently occupies (with
  transients above).
*/
static
void raiseGroup(unsigned int current_workspace,
                StackingList &stackingList,
                BWindowGroup *group) {
  BlackboxWindowList windows = group->windows();
  int layer = StackingList::LayerNormal;
  for (; layer < StackingList::LayerDesktop; ++layer) {
    StackingList::iterator
      it, top = stackingList.layer(StackingList::Layer(layer));
    const StackingList::iterator begin = stackingList.begin(),
                                   end = stackingList.end();

    // 'top' points to the top of the layer, we need to start from the
    // bottom of the layer
    it = std::find(top, end, (StackEntity *) 0);
    assert(it != end);

    if (!(*top)) {
      // nothing in layer
      break;
    }

    // walk up the layer, raising all windows in the group
    for (--it; it != begin; --it) {
      if (!(*it)) {
        // top of layer
        break;
      }

      BlackboxWindow *tmp = dynamic_cast<BlackboxWindow *>(*it);
      if (!tmp
          || (tmp->workspace() != current_workspace
              && tmp->workspace() != bt::BSENTINEL)) {
        // entity is not a window, or window is not visible on the
        // current_workspace
        continue;
      }

      const BlackboxWindowList::iterator wend = windows.end();
      BlackboxWindowList::iterator wit;
      if ((wit = std::find(windows.begin(), wend, tmp)) == wend)
          continue;

      // found a window in this layer, raise it
      ++it;
      stackingList.raise(tmp);
      // don't bother looking at this window again
      windows.erase(wit);
    }
  }
}


/*
  Raise all transients, preserving the existing stacking order.
*/
static
void raiseTransients(StackingList::iterator top,
                     StackingList &stackingList,
                     BlackboxWindowList &transients) {
  if (transients.empty())
    return;
  // 'top' points to the top of the layer, we need to start from the bottom
  StackingList::iterator begin = stackingList.begin(),
                           end = stackingList.end(),
                            it = std::find(top, end, (StackEntity *) 0);
  assert(it != end);
  for (--it; it != begin; --it) {
    if (it == top) {
      // top of layer
      break;
    }
    BlackboxWindow *tmp = dynamic_cast<BlackboxWindow *>(*it);
    if (!tmp)
      continue;
    BlackboxWindowList::iterator wit = transients.begin(),
                                wend = transients.end();
    wit = std::find(wit, wend, tmp);
    if (wit != wend) {
      // found a transient in this layer, raise it
      ++it;
      stackingList.raise(tmp);
      // don't bother looking at this window again
      transients.erase(wit);
    }
  }
}


/*
  Raises the specified stacking entity.  If the entity is a window,
  all transients are also raised (preserving their stacking order).
  If the window is part of a group, the entire group is raised (also
  preserving the stacking order) befor raising the specified window.
*/
static
StackingList::iterator raiseWindow(unsigned int current_workspace,
                                   StackingList &stackingList,
                                   StackEntity *entity) {
  BlackboxWindow *win = dynamic_cast<BlackboxWindow *>(entity);
  BWindowGroup *group = 0;
  if (win) {
    if (win->isFullScreen() && win->layer() != StackingList::LayerFullScreen) {
      // move full-screen windows over all other windows when raising
      win->changeLayer(StackingList::LayerFullScreen);
      return stackingList.end();
    }
  }

  StackingList::iterator layer = stackingList.layer(entity->layer());
  if (*layer == entity) {
    // already on top of the layer
    return stackingList.end();
  }

  if (win) {
    group = win->findWindowGroup();
    if (group) {
      // raise all windows in the group before raising 'win'
      ::raiseGroup(current_workspace, stackingList, group);
    } else {
      // raise non-transient parent before raising transient
      BlackboxWindow *tmp = win->findNonTransientParent();
      if (tmp != win)
        (void) ::raiseWindow(current_workspace, stackingList, tmp);
    }
  }

  // raise the entity
  StackingList::iterator top = stackingList.raise(entity),
                         end = stackingList.end();
  assert(top != end);
  if (win) {
    // ... with all transients above it
    BlackboxWindowList transients = buildTransientList(current_workspace, win);
    raiseTransients(top, stackingList, transients);

    // ... and group transients on top
    if (group && !win->isGroupTransient()) {
      const BlackboxWindow * const w = win->findNonTransientParent();
      if (!w || !w->isGroupTransient()) {
        BlackboxWindowList groupTransients = group->transients();
        BlackboxWindowList::const_iterator wit = groupTransients.begin(),
                                          wend = groupTransients.end();
        for (; wit != wend; ++wit) {
          const BlackboxWindow * const tmp = *wit;
          if (tmp->workspace() == current_workspace
              || tmp->workspace() == bt::BSENTINEL) {
            BlackboxWindowList x = buildTransientList(current_workspace, tmp);
            groupTransients.splice(groupTransients.end(), x);
          }
        }
        raiseTransients(top, stackingList, groupTransients);
      }
    }
  }
  if (!group)
    return top;

  StackingList::iterator it = std::find(top, end, (StackEntity *) 0);
  assert(it != end);
  return it;
}


/*
  Raises the stack entity as above and then updates the stacking on
  the X server.  The EWMH stacking hint is also updated.
 */
void BScreen::raiseWindow(StackEntity *entity) {
  StackingList::iterator top = ::raiseWindow(current_workspace,
                                             _stackingList,
                                             entity),
                       begin = _stackingList.begin(),
                         end = _stackingList.end();
  if (top == end) {
    // no need to raise entity
    return;
  } else if (!(*top)) {
    // need to restack all windows
    restackWindows();
    return;
  }

  // find the first entity above us (if any)
  StackEntity *above = 0;
  StackingList::iterator layer = _stackingList.layer(entity->layer()),
                            it = layer;
  for (--it; it != begin; --it) {
    if (*it) {
      above = *it;
      break;
    }
  }

  // build the stack
  WindowStack stack;
  if (above) {
    // found another entity above the one we are raising
    stack.push_back(above->windowID());
  } else {
    // keep everying under empty_window
    stack.push_back(empty_window);
  }

  // go to the layer boundary above 'top'
  it = top;
  for (--it; *it && it != begin; --it)
    ;

  // put all windows from the layer boundary to 'top' into the stack
  for (++it; *it && it != top; ++it) {
    assert(it != end);
    stack.push_back((*it)->windowID());
  }
  stack.push_back((*top)->windowID());

  XRestackWindows(_blackbox->XDisplay(), &stack[0], stack.size());
  updateClientListStackingHint();
}


void BScreen::lowerWindow(StackEntity *entity) {
  BlackboxWindow *win = dynamic_cast<BlackboxWindow *>(entity);
  BWindowGroup *group = 0;
  if (win) {
    group = win->findWindowGroup();
  }

  // find window at the bottom of the layer (if any)
  StackEntity *above = 0;
  {
    StackingList::const_iterator it, layer = _stackingList.layer(entity->layer()),
                                       end = _stackingList.end();
    it = std::find(layer, end, (StackEntity *) 0);
    assert(it != end);

    if (*--it == entity) {
      // entity already on bottom of layer
      return;
    }

    for (; it != layer; --it) {
      if (*it && *it != entity) {
        above = *it;
        break;
      }
    }
  }

  if (win) {
    if (group && win->isGroupTransient()) {
      lowerTransients(group->transients());
      BlackboxWindowList::const_iterator it = group->windows().begin(),
                                        end = group->windows().end();
      for (; it != end; ++it) {
        BlackboxWindow * const tmp = *it;
        if (tmp == win || tmp->isTransient())
          continue;
        lowerTransients(tmp->transients());
        _stackingList.lower(tmp);
      }
    } else {
      if (group)
        lowerTransients(group->transients());
      lowerTransients(win->transients());
      _stackingList.lower(win);
    }
  } else {
    _stackingList.lower(entity);
  }

  WindowStack stack;
  bool lower = true;

  if (above) {
    // found another entity above the one we are lowering
    stack.push_back(above->windowID());
    lower = false;
  }

  if (win) {
    if (group && win->isGroupTransient()) {
      stackTransients(group->transients(), stack);
      BlackboxWindowList::const_iterator it = group->windows().begin(),
                                        end = group->windows().end();
      for (; it != end; ++it) {
        BlackboxWindow * const tmp = *it;
        if (tmp->isTransient())
          continue;
        stackTransients(tmp->transients(), stack);
        stack.push_back(tmp->windowID());
      }
    } else {
      if (group)
        stackTransients(group->transients(), stack);
      stackTransients(win->transients(), stack);
      stack.push_back(win->windowID());
    }
  } else {
    stack.push_back(entity->windowID());
  }

  if (lower)
    XLowerWindow(_blackbox->XDisplay(), stack.front());
  XRestackWindows(_blackbox->XDisplay(), &stack[0], stack.size());

  updateClientListStackingHint();
}


void BScreen::lowerTransients(const BlackboxWindowList &transients) {
  if (transients.empty())
    return;
  BlackboxWindowList::const_iterator it = transients.begin(),
                                    end = transients.end();
  for (; it != end; ++it) {
    BlackboxWindow * const tmp = *it;
    if (tmp->workspace() == current_workspace
        || tmp->workspace() == bt::BSENTINEL) {
      lowerTransients(tmp->transients());
      _stackingList.lower(tmp);
    }
  }
}


// recursively stack transients in top-to-bottom order
void BScreen::stackTransients(const BlackboxWindowList &transients,
                              WindowStack &stack) {
  if (transients.empty())
    return;
  BlackboxWindowList::const_iterator it = transients.begin(),
                                    end = transients.end();
  for (; it != end; ++it) {
    BlackboxWindow * const tmp = *it;
    if (tmp->workspace() == current_workspace
        || tmp->workspace() == bt::BSENTINEL) {
      stackTransients(tmp->transients(), stack);
      stack.push_back(tmp->windowID());
    }
  }
}


void BScreen::restackWindows(void) {
  WindowStack stack;
  stack.push_back(empty_window);

  StackingList::const_iterator it, end = _stackingList.end();
  for (it = _stackingList.begin(); it != end; ++it)
    if (*it) stack.push_back((*it)->windowID());

  XRestackWindows(_blackbox->XDisplay(), &stack[0], stack.size());

  updateClientListStackingHint();
}


void BScreen::propagateWindowName(const BlackboxWindow * const win) {
  if (win->isIconic()) {
    _iconmenu->changeItem(win->windowNumber(),
                          bt::ellideText(win->iconTitle(), 60,
                                         bt::toUnicode("...")));
  } else if (win->workspace() != bt::BSENTINEL) {
    Workspace *workspace = findWorkspace(win->workspace());
    assert(workspace != 0);
    workspace->menu()->changeItem(win->windowNumber(),
                                  bt::ellideText(win->title(), 60,
                                                 bt::toUnicode("...")));
  }

  if (_toolbar && _blackbox->focusedWindow() == win)
    _toolbar->redrawWindowLabel();
}


void BScreen::nextFocus(void) {
  BlackboxWindow *focused = _blackbox->focusedWindow(),
                    *next = 0;
  BlackboxWindowList::iterator it, end = windowList.end();

  if (focused && focused->workspace() == current_workspace &&
      focused->screen()->screen_info.screenNumber() ==
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
  BlackboxWindow *focused = _blackbox->focusedWindow(),
                    *next = 0;
  BlackboxWindowList::reverse_iterator it, end = windowList.rend();

  if (focused && focused->workspace() == current_workspace &&
      focused->screen()->screen_info.screenNumber() ==
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
  BlackboxWindow *focused = _blackbox->focusedWindow();
  if (! focused || focused->screen() != this)
    return;

  raiseWindow(focused);
}


void BScreen::InitMenu(void) {
  if (_rootmenu) {
    _rootmenu->clear();
  } else {
    _rootmenu = new Rootmenu(*_blackbox, screen_info.screenNumber(), this);
    _rootmenu->showTitle();
  }
  bool defaultMenu = True;

  if (_blackbox->resource().menuFilename()) {
    FILE *menu_file = fopen(_blackbox->resource().menuFilename(), "r");

    if (!menu_file) {
      perror(_blackbox->resource().menuFilename());
    } else {
      if (feof(menu_file)) {
        fprintf(stderr, "%s: menu file '%s' is empty\n",
                _blackbox->applicationName().c_str(),
                _blackbox->resource().menuFilename());
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
              if (line[i] == '(')
                index = 0;
              else if (line[i] == ')')
                break;
              else if (index++ >= 0) {
                if (line[i] == '\\' && i < len - 1) i++;
                label[index - 1] = line[i];
              }
            }

            if (index == -1) index = 0;
            label[index] = '\0';

            _rootmenu->setTitle(bt::toUnicode(label));
            defaultMenu = parseMenuFile(menu_file, _rootmenu);
            break;
          }
        }
      }
      fclose(menu_file);
    }
  }

  if (defaultMenu) {
    _rootmenu->setTitle(bt::toUnicode("_Blackbox"));

    _rootmenu->insertFunction(bt::toUnicode("xterm"),
                              BScreen::Execute, "xterm");
    _rootmenu->insertFunction(bt::toUnicode("Restart"),
                              BScreen::Restart);
    _rootmenu->insertFunction(bt::toUnicode("Exit"),
                              BScreen::Exit);
  } else {
    _blackbox->saveMenuFilename(_blackbox->resource().menuFilename());
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
      menu->insertItem(bt::toUnicode(label));

      break;

    case 421: // exec
      if (! (*label && *command)) {
        fprintf(stderr,
                "%s: [exec] error, no menu label and/or command defined\n",
                _blackbox->applicationName().c_str());
        continue;
      }

      menu->insertFunction(bt::toUnicode(label), BScreen::Execute, command);
      break;

    case 442: // exit
      if (! *label) {
        fprintf(stderr, "%s: [exit] error, no menu label defined\n",
                _blackbox->applicationName().c_str());
        continue;
      }

      menu->insertFunction(bt::toUnicode(label), BScreen::Exit);
      break;

    case 561: { // style
      if (! (*label && *command)) {
        fprintf(stderr,
                "%s: [style] error, no menu label and/or filename defined\n",
                _blackbox->applicationName().c_str());
        continue;
      }

      std::string style = bt::expandTilde(command);
      menu->insertFunction(bt::toUnicode(label),
                           BScreen::SetStyle, style.c_str());
      break;
    }

    case 630: // config
      if (! *label) {
        fprintf(stderr, "%s: [config] error, no label defined",
                _blackbox->applicationName().c_str());
        continue;
      }

      menu->insertItem(bt::toUnicode(label), configmenu);
      break;

    case 740: { // include
      if (! *label) {
        fprintf(stderr, "%s: [include] error, no filename defined\n",
                _blackbox->applicationName().c_str());
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
                _blackbox->applicationName().c_str(), newfile.c_str());
        break;
      }

      if (! feof(submenufile)) {
        if (! parseMenuFile(submenufile, menu))
          _blackbox->saveMenuFilename(newfile);

        fclose(submenufile);
      }
    }

      break;

    case 767: { // submenu
      if (! *label) {
        fprintf(stderr, "%s: [submenu] error, no menu label defined\n",
                _blackbox->applicationName().c_str());
        continue;
      }

      Rootmenu *submenu =
        new Rootmenu(*_blackbox, screen_info.screenNumber(), this);
      submenu->showTitle();

      if (*command)
        submenu->setTitle(bt::toUnicode(command));
      else
        submenu->setTitle(bt::toUnicode(label));

      parseMenuFile(file, submenu);
      menu->insertItem(bt::toUnicode(label), submenu);
    }

      break;

    case 773: { // restart
      if (! *label) {
        fprintf(stderr, "%s: [restart] error, no menu label defined\n",
                _blackbox->applicationName().c_str());
        continue;
      }

      if (*command) {
        menu->insertFunction(bt::toUnicode(label),
                             BScreen::RestartOther, command);
      } else {
        menu->insertFunction(bt::toUnicode(label), BScreen::Restart);
      }
      break;
    }

    case 845: { // reconfig
      if (! *label) {
        fprintf(stderr, "%s: [reconfig] error, no menu label defined\n",
                _blackbox->applicationName().c_str());
        continue;
      }

      menu->insertFunction(bt::toUnicode(label), BScreen::Reconfigure);
      break;
    }

    case 995:    // stylesdir
    case 1113: { // stylesmenu
      bool newmenu = ((key == 1113) ? True : False);

      if (! *label || (! *command && newmenu)) {
        fprintf(stderr,
                "%s: [stylesdir/stylesmenu] error, no directory defined\n",
                _blackbox->applicationName().c_str());
        continue;
      }

      char *directory = ((newmenu) ? command : label);

      std::string stylesdir = bt::expandTilde(directory);

      struct stat statbuf;

      if (stat(stylesdir.c_str(), &statbuf) == -1) {
        fprintf(stderr,
                "%s: [stylesdir/stylesmenu] error, '%s' does not exist\n",
                _blackbox->applicationName().c_str(), stylesdir.c_str());
        continue;
      }
      if (! S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr,
                "%s: [stylesdir/stylesmenu] error, '%s' is not a directory\n",
                _blackbox->applicationName().c_str(), stylesdir.c_str());
        continue;
      }

      Rootmenu *stylesmenu;

      if (newmenu) {
        stylesmenu =
          new Rootmenu(*_blackbox, screen_info.screenNumber(),this);
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

          stylesmenu->insertFunction(bt::toUnicode(fname),
                                     BScreen::SetStyle, style);
        }
      }

      if (newmenu) {
        stylesmenu->setTitle(bt::toUnicode(label));
        menu->insertItem(bt::toUnicode(label), stylesmenu);
      }

      _blackbox->saveMenuFilename(stylesdir);
    }
      break;

    case 1090: { // workspaces
      if (! *label) {
        fprintf(stderr, "%s: [workspaces] error, no menu label defined\n",
                _blackbox->applicationName().c_str());
        continue;
      }

      menu->insertItem(bt::toUnicode(label), _workspacemenu);
      break;
    }
    } // switch
  }

  return (menu->count() == 0);
}


void BScreen::shutdown(void) {
  XSelectInput(_blackbox->XDisplay(), screen_info.rootWindow(),
               NoEventMask);
  XSync(_blackbox->XDisplay(), False);

  while(! windowList.empty())
    unmanageWindow(windowList.back());

  if (_slit)
    _slit->shutdown();
}


void BScreen::showGeometry(GeometryType type, const bt::Rect &rect) {
  if (! geom_visible) {
    XMoveResizeWindow(_blackbox->XDisplay(), geom_window,
                      (screen_info.width() - geom_w) / 2,
                      (screen_info.height() - geom_h) / 2, geom_w, geom_h);
    XMapWindow(_blackbox->XDisplay(), geom_window);
    XRaiseWindow(_blackbox->XDisplay(), geom_window);

    geom_visible = True;
  }

  char label[80];
  switch (type) {
  case Position:
    sprintf(label, "X:%4d    Y:%4d", rect.x(), rect.y());
    break;
  case Size:
    sprintf(label, "W:%4u    H:%4u", rect.width(), rect.height());
    break;
  default:
    assert(0);
  }

  const WindowStyle &style = _resource.windowStyle();
  const bt::Texture &texture =
    (style.focus.label.texture() == bt::Texture::Parent_Relative)
    ? style.focus.title
    : style.focus.label;
  const bt::Rect u(0, 0, geom_w, geom_h);
  const bt::Rect t(style.label_margin,
                   style.label_margin,
                   geom_w - (style.label_margin * 2),
                   geom_h - (style.label_margin * 2));
  const bt::Pen pen(screen_info.screenNumber(), style.focus.text);
  bt::drawTexture(screen_info.screenNumber(), texture, geom_window,
                  u, u, geom_pixmap);
  bt::drawText(style.font, pen, geom_window, t, style.alignment,
               bt::toUnicode(label));
}


void BScreen::hideGeometry(void) {
  if (geom_visible) {
    XUnmapWindow(_blackbox->XDisplay(), geom_window);
    geom_visible = False;
  }
}


void BScreen::addStrut(bt::EWMH::Strut *strut) {
  strutList.push_back(strut);
  updateAvailableArea();
}


void BScreen::removeStrut(bt::EWMH::Strut *strut) {
  strutList.remove(strut);
  updateAvailableArea();
}


void BScreen::updateStrut(void) {
  updateAvailableArea();
}


const bt::Rect& BScreen::availableArea(void) {
  if (_blackbox->resource().fullMaximization())
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
  bt::EWMH::Strut current;

  StrutList::const_iterator sit = strutList.begin(), send = strutList.end();

  for(; sit != send; ++sit) {
    const bt::EWMH::Strut* const strut = *sit;
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


Workspace* BScreen::findWorkspace(unsigned int index) const {
  if (index == bt::BSENTINEL)
    return 0;
  assert(index < workspacesList.size());
  return workspacesList[index];
}


void BScreen::clientMessageEvent(const XClientMessageEvent * const event) {
  if (event->format != 32) return;

  if (event->message_type == _blackbox->ewmh().numberOfDesktops()) {
    unsigned int number = event->data.l[0];
    if (number > workspaceCount()) {
      for (; number != workspaceCount(); --number)
        addWorkspace();
    } else if (number < workspaceCount()) {
      for (; number != workspaceCount(); ++number)
        removeLastWorkspace();
    }
  } else if (event->message_type == _blackbox->ewmh().desktopNames()) {
    readDesktopNames();
  } else if (event->message_type == _blackbox->ewmh().currentDesktop()) {
    const unsigned int workspace = event->data.l[0];
    if (workspace < workspaceCount() && workspace != current_workspace)
      setCurrentWorkspace(workspace);
  }
}


void BScreen::buttonPressEvent(const XButtonEvent * const event) {
  if (event->button == 1) {
    /*
      set this screen active.  keygrabs and input focus will stay on
      this screen until the user focuses a window on another screen or
      makes another screen active.
    */
    _blackbox->setActiveScreen(this);
  } else if (event->button == 2) {
    _workspacemenu->popup(event->x_root, event->y_root);
  } else if (event->button == 3) {
    _blackbox->checkMenu();
    _rootmenu->popup(event->x_root, event->y_root);
  } else if (event->button == 4) {
    setCurrentWorkspace(current_workspace < workspacesList.size() - 1
                        ? current_workspace + 1
                        : 0);
  } else if (event->button == 5) {
    setCurrentWorkspace(current_workspace > 0
                        ? current_workspace - 1
                        : workspacesList.size() - 1);
  }
}


void BScreen::propertyNotifyEvent(const XPropertyEvent * const event) {
  if (event->atom == _blackbox->ewmh().activeWindow() && _toolbar)
    _toolbar->redrawWindowLabel();
}


void BScreen::unmapNotifyEvent(const XUnmapEvent * const event) {
  // handle synthetic unmap events according to ICCCM section 4.1.4
  BlackboxWindow *win = _blackbox->findWindow(event->window);
  if (win && event->event == screen_info.rootWindow()
      && !event->from_configure)
    win->unmapNotifyEvent(event);
}


void BScreen::toggleFocusModel(FocusModel model) {
  std::for_each(windowList.begin(), windowList.end(),
                std::mem_fun(&BlackboxWindow::ungrabButtons));

  _blackbox->resource().setFocusModel(model);

  std::for_each(windowList.begin(), windowList.end(),
                std::mem_fun(&BlackboxWindow::grabButtons));
}


void BScreen::updateWorkareaHint(void) const {
  unsigned long *workarea, *tmp;

  tmp = workarea = new unsigned long[workspaceCount() * 4];

  for (unsigned int i = 0; i < workspaceCount(); ++i) {
    tmp[0] = usableArea.x();
    tmp[1] = usableArea.y();
    tmp[2] = usableArea.width();
    tmp[3] = usableArea.height();
    tmp += 4;
  }

  _blackbox->ewmh().setWorkarea(screen_info.rootWindow(),
                                 workarea, workspaceCount());

  delete [] workarea;
}


void BScreen::updateDesktopNamesHint(void) const {
  std::vector<bt::ustring> names(workspacesList.size());
  WorkspaceList::const_iterator it = workspacesList.begin();
  const WorkspaceList::const_iterator end = workspacesList.end();
  for (; it != end; ++it)
    names.push_back((*it)->name());
  _blackbox->ewmh().setDesktopNames(screen_info.rootWindow(), names);
}


void BScreen::updateClientListHint(void) const {
  if (windowList.empty()) {
    _blackbox->ewmh().removeProperty(screen_info.rootWindow(),
                                      _blackbox->ewmh().clientList());
    return;
  }

  bt::EWMH::WindowList clientList(windowList.size());

  std::transform(windowList.begin(), windowList.end(), clientList.begin(),
                 std::mem_fun(&BlackboxWindow::clientWindow));

  _blackbox->ewmh().setClientList(screen_info.rootWindow(), clientList);
}


void BScreen::updateClientListStackingHint(void) const {
  bt::EWMH::WindowList stack;

  // we store windows in top-to-bottom order, but the EWMH wants
  // bottom-to-top...
  StackingList::const_reverse_iterator it = _stackingList.rbegin(),
                                      end = _stackingList.rend();
  for (; it != end; ++it) {
    const BlackboxWindow * const win = dynamic_cast<BlackboxWindow *>(*it);
    if (win) stack.push_back(win->clientWindow());
  }

  if (stack.empty()) {
    _blackbox->ewmh().removeProperty(screen_info.rootWindow(),
                                     _blackbox->ewmh().clientListStacking());
    return;
  }

  _blackbox->ewmh().setClientListStacking(screen_info.rootWindow(), stack);
}


void BScreen::readDesktopNames(void) {
  std::vector<bt::ustring> names;
  if(! _blackbox->ewmh().readDesktopNames(screen_info.rootWindow(), names))
    return;

  std::vector<bt::ustring>::const_iterator it = names.begin();
  const std::vector<bt::ustring>::const_iterator end = names.end();
  WorkspaceList::iterator wit = workspacesList.begin();
  const WorkspaceList::iterator wend = workspacesList.end();

  for (; wit != wend && it != end; ++wit, ++it) {
    if ((*wit)->name() != *it)
      (*wit)->setName(*it);
  }

  if (names.size() < workspacesList.size())
    updateDesktopNamesHint();
}


BlackboxWindow *BScreen::window(unsigned int workspace, unsigned int id) {
  StackingList::const_iterator it = _stackingList.begin(),
                              end = _stackingList.end();
  for (; it != end; ++it) {
    BlackboxWindow * const win = dynamic_cast<BlackboxWindow *>(*it);
    if (win && win->workspace() == workspace && win->windowNumber() == id)
      return win;
  }
  assert(false); // should not happen
  return 0;
}


void BScreen::placeWindow(BlackboxWindow *win) {
  const bt::Rect &avail = availableArea();
  bt::Rect new_win(avail.x(), avail.y(),
                   win->frameRect().width(), win->frameRect().height());
  bool placed = False;

  BlackboxResource &res = _blackbox->resource();
  switch (res.windowPlacementPolicy()) {
  case RowSmartPlacement:
  case ColSmartPlacement:
    placed = smartPlacement(win->workspace(), new_win, avail);
    break;
  default:
    break; // handled below
  } // switch

  if (placed == False) {
    cascadePlacement(new_win, avail);
    cascade_x += _resource.windowStyle().title_height;
    cascade_y += _resource.windowStyle().title_height;
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
  const BlackboxResource &res = _blackbox->resource();
  const bool row_placement =
    (res.windowPlacementPolicy() == RowSmartPlacement);
  const bool leftright =
    (res.rowPlacementDirection() == LeftRight);
  const bool topbottom =
    (res.colPlacementDirection() == TopBottom);
  const bool ignore_shaded = res.placementIgnoresShaded();

  const int left_border   = leftright ? 0 : -2;
  const int top_border    = topbottom ? 0 : -2;
  const int right_border  = leftright ? 2 : 0;
  const int bottom_border = topbottom ? 2 : 0;

  StackingList::const_iterator w_it, w_end;

  /*
    build a sorted vector of x and y grid boundaries

    note: we use one vector to reduce the number of allocations
    std::vector must do.. we allocate as much memory as we would need
    in the worst case scenario and work with that
  */
  std::vector<int> coords(_stackingList.size() * 4 + 4);
  std::vector<int>::iterator
    x_begin = coords.begin(),
    x_end   = x_begin,
    y_begin = coords.begin() + _stackingList.size() * 2 + 2,
    y_end   = y_begin;

  {
    std::vector<int>::iterator x_it = x_begin, y_it = y_begin;

    *x_it++ = avail.left();
    *x_it++ = avail.right();
    x_end += 2;

    *y_it++ = avail.top();
    *y_it++ = avail.bottom();
    y_end += 2;


    for (w_it  = _stackingList.begin(), w_end = _stackingList.end();
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
  std::vector<bool> used_grid(gw * gh);
  std::fill_n(used_grid.begin(), used_grid.size(), false);

  for (w_it = _stackingList.begin(), w_end = _stackingList.end();
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
      // center windows that don't fit (except for small windows)
      const int x =
        static_cast<int>(avail.x() + avail.width() - rect.width()) / 2;
      const int y =
        static_cast<int>(avail.y() + avail.height() - rect.height()) / 2;
      rect.setPos(x, y);
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
  _stackingList.insert(_slit);
  restackWindows();
}


void BScreen::destroySlit(void) {
  assert(_slit != 0);

  _stackingList.remove(_slit);
  delete _slit;
  _slit = 0;
}


void BScreen::createToolbar(void) {
  assert(_toolbar == 0);

  _toolbar = new Toolbar(this);
  _stackingList.insert(_toolbar);
  restackWindows();
}


void BScreen::destroyToolbar(void) {
  assert(_toolbar != 0);

  _stackingList.remove(_toolbar);
  delete _toolbar;
  _toolbar = 0;
}


Windowmenu *BScreen::windowmenu(BlackboxWindow *win) {
  if (!_windowmenu)
    _windowmenu = new Windowmenu(*_blackbox, screen_info.screenNumber());
  _windowmenu->setWindow(win);
  return _windowmenu;
}


void BScreen::addIcon(BlackboxWindow *win) {
  if (win->workspace() != bt::BSENTINEL) {
    Workspace *workspace = findWorkspace(win->workspace());
    assert(workspace != 0);
    workspace->removeWindow(win);
  }

  if (win->isTransient()) {
    BlackboxWindow * const tmp = win->findNonTransientParent();
    if (tmp) {
      win->setWindowNumber(bt::BSENTINEL);
      return;
    }
  }

  const bt::ustring s =
    bt::ellideText(win->iconTitle(), 60, bt::toUnicode("..."));
  int id = _iconmenu->insertItem(s);
  _blackbox->ewmh().setWMVisibleIconName(win->clientWindow(), s);
  win->setWindowNumber(id);
}


void BScreen::removeIcon(BlackboxWindow *win) {
  if (win->windowNumber() != bt::BSENTINEL) {
    _iconmenu->removeItem(win->windowNumber());
    _blackbox->ewmh().removeProperty(win->clientWindow(),
                                     _blackbox->ewmh().wmVisibleIconName());
  }

  Workspace *workspace = findWorkspace(current_workspace);
  assert(workspace != 0);
  workspace->addWindow(win);
}


BlackboxWindow *BScreen::icon(unsigned int id) {
  StackingList::const_iterator it = _stackingList.begin(),
                              end = _stackingList.end();
  for (; it != end; ++it) {
    BlackboxWindow * const win = dynamic_cast<BlackboxWindow *>(*it);
    if (win && win->isIconic() && win->windowNumber() == id)
      return win;
  }
  assert(false); // should not happen
  return 0;
}
