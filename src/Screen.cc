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

#include "Screen.hh"
#include "Clientmenu.hh"
#include "Configmenu.hh"
#include "Iconmenu.hh"
#include "Slit.hh"
#include "Rootmenu.hh"
#include "Toolbar.hh"
#include "Window.hh"
#include "WindowGroup.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"
#include "../nls/blackbox-nls.hh"

#include <Pen.hh>
#include <PixmapCache.hh>
#include <i18n.hh>

#include <sys/stat.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>


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
  opGC(0), screen_info(bb->display().screenInfo(scrn)), blackbox(bb),
  _resource(bb->resource().screenResource(scrn)) {

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
  if (! managed) return;

  fprintf(stderr, bt::i18n(ScreenSet, ScreenManagingScreen,
                           "BScreen::BScreen: managing screen %d "
                           "using visual 0x%lx, depth %d\n"),
          screen_info.screenNumber(),
          XVisualIDFromVisual(screen_info.visual()),
          screen_info.depth());

  blackbox->insertEventHandler(screen_info.rootWindow(), this);

  rootmenu = (Rootmenu *) 0;

  XDefineCursor(blackbox->XDisplay(), screen_info.rootWindow(),
                blackbox->resource().sessionCursor());

  // start off full screen, top left.
  usableArea.setSize(screen_info.width(), screen_info.height());

  LoadStyle();

  updateOpGC();

  geom_pixmap = None;
  geom_visible = False;
  geom_window = None;

  updateGeomWindow();

  workspacemenu =
    new Workspacemenu(*blackbox, screen_info.screenNumber(), this);
  iconmenu =
    new Iconmenu(*blackbox, screen_info.screenNumber(), this);
  configmenu =
    new Configmenu(*blackbox, screen_info.screenNumber(), this);

  if (_resource.numberOfWorkspaces() == 0) // there is always 1 workspace
    _resource.saveWorkspaces(1);

  workspacemenu->insertIconMenu(iconmenu);
  for (unsigned int i = 0; i < _resource.numberOfWorkspaces(); ++i) {
    Workspace *wkspc = new Workspace(this, i);
    workspacesList.push_back(wkspc);
    workspacemenu->insertWorkspace(wkspc);
  }

  current_workspace = workspacesList.front();
  current_workspace_id = current_workspace->id();
  workspacemenu->setWorkspaceChecked(current_workspace_id, true);

  // the Slit will be created on demand
  _slit = 0;

  _toolbar = new Toolbar(this);

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

  destroySlit();
  destroyToolbar();

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


void BScreen::updateGeomWindow(void) {
  const char *s =
    bt::i18n(ScreenSet, ScreenPositionLength, "0: 0000 x 0: 0000");
  bt::Rect geomr =
    bt::textRect(screen_info.screenNumber(), _resource.windowStyle()->font, s);
  geom_w = geomr.width() + (_resource.bevelWidth() * 2);
  geom_h = geomr.height() + (_resource.bevelWidth() * 2);

  if (geom_window == None) {
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


void BScreen::updateOpGC(void) {
  XGCValues gcv;
  unsigned long gc_value_mask = GCForeground;
  if (! bt::i18n.multibyte()) gc_value_mask |= GCFont;

  gcv.foreground = WhitePixel(blackbox->XDisplay(),
                              screen_info.screenNumber())
                   ^ BlackPixel(blackbox->XDisplay(),
                                screen_info.screenNumber());
  gcv.function = GXxor;
  gcv.subwindow_mode = IncludeInferiors;
  if (! opGC)
    opGC = XCreateGC(blackbox->XDisplay(), screen_info.rootWindow(),
                     GCForeground | GCFunction | GCSubwindowMode, &gcv);
  else
    XChangeGC(blackbox->XDisplay(), opGC,
              GCForeground | GCFunction | GCSubwindowMode, &gcv);
}


void BScreen::reconfigure(void) {
  LoadStyle();

  updateOpGC();

  updateGeomWindow();

  workspacemenu->reconfigure();
  iconmenu->reconfigure();

  InitMenu();
  raiseWindows((WindowStack*) 0);
  rootmenu->reconfigure();

  configmenu->reconfigure();

  if (_toolbar) _toolbar->reconfigure();
  if (_slit) _slit->reconfigure();

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


void BScreen::LoadStyle(void) {
  _resource.loadStyle(this, blackbox->resource().styleFilename());

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

  Workspace *wkspc = workspacesList.back();
  workspacesList.pop_back();

  if (current_workspace->id() == wkspc->id())
    changeWorkspaceID(current_workspace->id() - 1);

  wkspc->transferWindows(*(workspacesList.back()));

  workspacemenu->removeWorkspace(wkspc->id());

  delete wkspc;

  if (_toolbar) _toolbar->reconfigure();

  blackbox->netwm().setNumberOfDesktops(screen_info.rootWindow(),
                                         workspacesList.size());
  updateDesktopNamesHint();

  return workspacesList.size();
}


void BScreen::changeWorkspaceID(unsigned int id) {
  if (! current_workspace || id == current_workspace->id()) return;

  current_workspace->hide();

  workspacemenu->setWorkspaceChecked(current_workspace_id, false);

  current_workspace = getWorkspace(id);
  current_workspace_id = current_workspace->id();

  current_workspace->show();

  workspacemenu->setWorkspaceChecked(current_workspace_id, true);
  if (_toolbar) _toolbar->redrawWorkspaceLabel();

  blackbox->netwm().setCurrentDesktop(screen_info.rootWindow(),
                                       current_workspace->id());
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
    if (!_slit) createSlit();
    _slit->addClient(w);
    return;
  }

  (void) new BlackboxWindow(blackbox, w, this);
  BlackboxWindow *win = blackbox->findWindow(w);
  if (! win)
    return;

  Workspace* wkspc =
    (win->getWorkspaceNumber() >= _resource.numberOfWorkspaces()) ?
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

  if (_toolbar && _toolbar->isOnTop())
    *(it++) = _toolbar->getWindowID();

  if (_slit && _slit->isOnTop())
    *(it++) = _slit->getWindowID();

  if (workspace_stack_size)
    std::copy(workspace_stack->rbegin(), workspace_stack->rend(), it);

  if (! session_stack.empty()) {
    XRaiseWindow(blackbox->XDisplay(), session_stack[0]);
    XRestackWindows(blackbox->XDisplay(), &session_stack[0],
                    session_stack.size());
  }

  updateClientListStackingHint();
}


void BScreen::reassociateWindow(BlackboxWindow *w, unsigned int wkspc_id) {
  if (! w) return;

  if (wkspc_id == bt::BSENTINEL)
    wkspc_id = current_workspace->id();

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
    Clientmenu *clientmenu = getWorkspace(w->getWorkspaceNumber())->menu();
    clientmenu->changeItem(w->getWindowNumber(),
                           bt::ellideText(w->getTitle(), 60, "..."));

    if (_toolbar && blackbox->getFocusedWindow() == w)
      _toolbar->redrawWindowLabel();
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
      current_workspace->windowCount() > 1) {
    do {
      next = current_workspace->getNextWindowInList(next);
    } while(next != focused && ! next->setInputFocus());

    if (next != focused)
      current_workspace->raiseWindow(next);
  } else if (current_workspace->windowCount() > 0) {
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
      current_workspace->windowCount() > 1) {
    do {
      next = current_workspace->getPrevWindowInList(next);
    } while(next != focused && ! next->setInputFocus());

    if (next != focused)
      current_workspace->raiseWindow(next);
  } else if (current_workspace->windowCount() > 0) {
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

  if (blackbox->resource().menuFilename()) {
    FILE *menu_file = fopen(blackbox->resource().menuFilename(), "r");

    if (!menu_file) {
      perror(blackbox->resource().menuFilename());
    } else {
      if (feof(menu_file)) {
        fprintf(stderr, bt::i18n(ScreenSet, ScreenEmptyMenuFile,
                                 "%s: Empty menu file"),
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

  if (_slit) _slit->shutdown();
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
        workspace != getCurrentWorkspaceID())
      changeWorkspaceID(workspace);
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
    if ((*wit)->name() != *it)
      (*wit)->setName(*it);
  }

  if (names.size() < workspacesList.size())
    updateDesktopNamesHint();
}


BlackboxWindow* BScreen::getWindow(unsigned int workspace, unsigned int id) {
  return getWorkspace(workspace)->window(id);
}


void BScreen::createSlit(void) {
  assert(_slit == 0);

  _slit = new Slit(this);
  raiseWindows(0);
}


void BScreen::destroySlit(void) {
  delete _slit;
  _slit = 0;
}


void BScreen::createToolbar(void) {
  assert(_toolbar == 0);

  _toolbar = new Toolbar(this);
  raiseWindows(0);
}


void BScreen::destroyToolbar(void) {
  delete _toolbar;
  _toolbar = 0;
}
