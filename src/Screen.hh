// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Screen.hh for Blackbox - an X11 Window manager
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

#ifndef   __Screen_hh
#define   __Screen_hh

#include "StackingList.hh"
#include "blackbox.hh"

#include <Display.hh>
#include <EventHandler.hh>
#include <Netwm.hh>

#include <stdio.h>

#include <list>
#include <vector>

// forward declarations
class BlackboxWindow;
class Configmenu;
class Iconmenu;
class Rootmenu;
class Slit;
class Slitmenu;
class Toolbar;
class Toolbarmenu;
class Windowmenu;
class Workspace;
class Workspacemenu;

namespace bt {
  class ScreenInfo;
}


class BScreen : public bt::NoCopy, public bt::EventHandler {
private:
  bool managed, geom_visible;
  Pixmap geom_pixmap;
  Window geom_window;
  Window empty_window;
  Window no_focus_window;

  const bt::ScreenInfo& screen_info;
  Blackbox *blackbox;
  Configmenu *configmenu;
  Iconmenu *_iconmenu;
  Rootmenu *rootmenu;
  Slitmenu *_slitmenu;
  Toolbarmenu *_toolbarmenu;
  Windowmenu *_windowmenu;
  Workspacemenu *workspacemenu;

  BlackboxWindowList windowList;
  StackingList stackingList;
  unsigned int current_workspace;

  Slit *_slit;
  Toolbar *_toolbar;

  unsigned int geom_w, geom_h;

  bt::Rect usableArea;

  typedef std::list<bt::Netwm::Strut*> StrutList;
  StrutList strutList;
  typedef std::vector<Workspace*> WorkspaceList;
  WorkspaceList workspacesList;

  ScreenResource& _resource;

  void updateGeomWindow(void);

  bool parseMenuFile(FILE *file, Rootmenu *menu);

  void InitMenu(void);
  void LoadStyle(void);

  void manageWindow(Window w);
  void unmanageWindow(BlackboxWindow *win);
  bool focusFallback(const BlackboxWindow *win);

  void placeWindow(BlackboxWindow *win);
  bool cascadePlacement(bt::Rect& win, const bt::Rect& avail);
  bool smartPlacement(unsigned int workspace, bt::Rect& win,
                      const bt::Rect& avail);
  unsigned int cascade_x, cascade_y;

  void raiseTransients(const BlackboxWindow * const win);
  void lowerTransients(const BlackboxWindow * const win);
  void stackTransients(const BlackboxWindow * const win, WindowStack &stack);

  void updateAvailableArea(void);
  void updateWorkareaHint(void) const;

public:
  enum { Restart = 1, RestartOther, Exit, Shutdown, Execute, Reconfigure,
         SetStyle };
  enum FocusModel { SloppyFocus, ClickToFocus };

  BScreen(Blackbox *bb, unsigned int scrn);
  ~BScreen(void);

  inline Window noFocusWindow() const
  { return no_focus_window; }

  // information about the screen
  const bt::ScreenInfo &screenInfo(void) const { return screen_info; }
  unsigned int screenNumber(void) const { return screen_info.screenNumber(); }

  ScreenResource& resource(void) { return _resource; }
  void saveResource(void) { blackbox->resource().save(*blackbox); }

  bool isScreenManaged(void) const { return managed; }

  Blackbox *getBlackbox(void) { return blackbox; }

  Rootmenu *getRootmenu(void) { return rootmenu; }

  Workspacemenu *getWorkspacemenu(void) { return workspacemenu; }

  inline Iconmenu *iconmenu(void) const
  { return _iconmenu; }
  inline Slitmenu *slitmenu(void) const
  { return _slitmenu; }
  inline Toolbarmenu *toolbarmenu(void) const
  { return _toolbarmenu; }

  Windowmenu *windowmenu(BlackboxWindow *win);

  inline Slit *slit(void) const
  { return _slit; }
  void createSlit(void);
  void destroySlit(void);

  inline Toolbar *toolbar(void) const
  { return _toolbar; }
  void createToolbar(void);
  void destroyToolbar(void);

  Workspace *getWorkspace(unsigned int index) const;

  inline unsigned int workspaceCount(void) const
  { return workspacesList.size(); }
  inline unsigned int currentWorkspace(void) const
  { return current_workspace; }
  void setCurrentWorkspace(unsigned int id);

  const bt::Rect& availableArea(void);
  void addStrut(bt::Netwm::Strut *strut);
  void removeStrut(bt::Netwm::Strut *strut);
  void updateStrut(void);

  void updateClientListHint(void) const;
  void updateClientListStackingHint(void) const;
  void updateDesktopNamesHint(void) const;
  void getDesktopNames(void);

  void addWorkspace(void);
  void removeLastWorkspace(void);

  void addWindow(Window w);
  void releaseWindow(BlackboxWindow *win);
  BlackboxWindow* getWindow(unsigned int workspace, unsigned int id);

  void raiseWindow(StackEntity *entity);
  void lowerWindow(StackEntity *entity);
  void restackWindows(void);

  void changeLayer(StackEntity *entity, StackingList::Layer new_layer);
  void changeWorkspace(BlackboxWindow *win, unsigned int id);

  void addIcon(BlackboxWindow *win);
  void removeIcon(BlackboxWindow *win);
  BlackboxWindow *icon(unsigned int id);

  void nextFocus(void);
  void prevFocus(void);
  void raiseFocus(void);

  void propagateWindowName(const BlackboxWindow * const win);

  void reconfigure(void);
  void toggleFocusModel(FocusModel model);
  void rereadMenu(void);
  void shutdown(void);

  enum GeometryType { Position, Size };
  void showGeometry(GeometryType type, const bt::Rect &rect);
  void hideGeometry(void);

  void clientMessageEvent(const XClientMessageEvent * const event);
  void buttonPressEvent(const XButtonEvent * const event);
  void propertyNotifyEvent(const XPropertyEvent * const event);
};

#endif // __Screen_hh
