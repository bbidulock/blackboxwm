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

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xresource.h>

#ifdef    TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else // !TIME_WITH_SYS_TIME
#  ifdef    HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else // !HAVE_SYS_TIME_H
#    include <time.h>
#  endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME
}

#include <list>
#include <vector>
typedef std::vector<Window> WindowStack;

#include "Color.hh"
#include "Texture.hh"
#include "Configmenu.hh"
#include "Iconmenu.hh"
#include "Netwm.hh"
#include "Workspacemenu.hh"
#include "blackbox.hh"

// forward declaration
class Rootmenu;
class Slit;
class Workspace;

struct WindowStyle {
  bt::Color l_text_focus, l_text_unfocus, b_pic_focus,
    b_pic_unfocus;
  bt::Texture f_focus, f_unfocus, t_focus, t_unfocus, l_focus, l_unfocus,
    h_focus, h_unfocus, b_focus, b_unfocus, b_pressed, g_focus, g_unfocus;
  bt::Font font;
  bt::Alignment alignment;
  unsigned int handle_width, frame_width;
};

struct ToolbarStyle {
  bt::Color l_text, w_text, c_text, b_pic;
  bt::Texture toolbar, label, window, button, pressed, clock;
  bt::Font font;
  bt::Alignment alignment;
};


typedef std::list<BlackboxWindow*> BlackboxWindowList;
typedef std::list<Window> WindowList;


class BScreen : public bt::NoCopy, public bt::EventHandler {
private:
  bool root_colormap_installed, managed, geom_visible;
  GC opGC;
  Pixmap geom_pixmap;
  Window geom_window;

  const bt::ScreenInfo& screen_info;
  Blackbox *blackbox;
  Configmenu *configmenu;
  Iconmenu *iconmenu;
  Rootmenu *rootmenu;

  BlackboxWindowList iconList, windowList;

  Slit *slit;
  Toolbar *toolbar;
  Workspace *current_workspace;
  unsigned int current_workspace_id;
  Workspacemenu *workspacemenu;

  unsigned int geom_w, geom_h;
  unsigned long event_mask;

  bt::Rect usableArea;

  typedef std::list<bt::Netwm::Strut*> StrutList;
  StrutList strutList;
  typedef std::vector<std::string> WorkspaceNamesList;
  WorkspaceNamesList workspaceNames;
  typedef std::vector<Workspace*> WorkspaceList;
  WorkspaceList workspacesList;

  struct screen_resource {
    WindowStyle wstyle;
    ToolbarStyle tstyle;

    bool toolbar_on_top, toolbar_auto_hide, sloppy_focus, auto_raise,
      auto_edge_balance, image_dither, ordered_dither, opaque_move, full_max,
      ignore_shaded, focus_new, focus_last, click_raise, allow_scroll_lock;
    bt::Color border_color;
    XrmDatabase stylerc;

    unsigned int workspaces;
    int toolbar_placement, toolbar_width_percent, placement_policy,
      edge_snap_threshold, row_direction, col_direction;

    bool slit_on_top, slit_auto_hide;
    int slit_placement, slit_direction;

    unsigned int bevel_width, border_width;

    std::string strftime_format;
  } resource;

  bool parseMenuFile(FILE *file, Rootmenu *menu);

  void InitMenu(void);
  void LoadStyle(void);

  void manageWindow(Window w);
  void unmanageWindow(BlackboxWindow *w, bool remap);

  void updateAvailableArea(void);
  void updateWorkareaHint(void) const;

public:
  enum { RowSmartPlacement = 400, ColSmartPlacement, CascadePlacement,
         LeftRight, RightLeft, TopBottom, BottomTop };
  enum { RoundBullet = 1, TriangleBullet, SquareBullet, NoBullet };
  enum { Restart = 1, RestartOther, Exit, Shutdown, Execute, Reconfigure,
         WindowShade, WindowIconify, WindowMaximize, WindowClose, WindowRaise,
         WindowLower, WindowKill, SetStyle };
  enum FocusModel { SloppyFocus, ClickToFocus };

  BScreen(Blackbox *bb, unsigned int scrn);
  ~BScreen(void);

  // information about the screen
  const bt::ScreenInfo &screenInfo(void) const { return screen_info; }
  unsigned int screenNumber(void) const { return screen_info.screenNumber(); }

  bool isToolbarOnTop(void) const
  { return resource.toolbar_on_top; }
  bool doToolbarAutoHide(void) const
  { return resource.toolbar_auto_hide; }
  bool isSloppyFocus(void) const
  { return resource.sloppy_focus; }
  bool isRootColormapInstalled(void) const
  { return root_colormap_installed; }
  bool doAutoRaise(void) const { return resource.auto_raise; }
  bool doClickRaise(void) const { return resource.click_raise; }
  bool isScreenManaged(void) const { return managed; }
  bool doImageDither(void) const
  { return resource.image_dither; }
  bool doOrderedDither(void) const
  { return resource.ordered_dither; }
  bool doOpaqueMove(void) const { return resource.opaque_move; }
  bool doFullMax(void) const { return resource.full_max; }
  bool placementIgnoresShaded(void) const { return resource.ignore_shaded; }
  bool doFocusNew(void) const { return resource.focus_new; }
  bool doFocusLast(void) const { return resource.focus_last; }
  bool allowScrollLock(void) const { return resource.allow_scroll_lock;}

  const GC &getOpGC(void) const { return opGC; }
  Blackbox *getBlackbox(void) { return blackbox; }
  bt::Color *getBorderColor(void) { return &resource.border_color; }
  Rootmenu *getRootmenu(void) { return rootmenu; }

  bool isSlitOnTop(void) const { return resource.slit_on_top; }
  bool doSlitAutoHide(void) const
  { return resource.slit_auto_hide; }
  Slit *getSlit(void) { return slit; }
  int getSlitPlacement(void) const
  { return resource.slit_placement; }
  int getSlitDirection(void) const
  { return resource.slit_direction; }
  void saveSlitPlacement(int p) { resource.slit_placement = p; }
  void saveSlitDirection(int d) { resource.slit_direction = d; }
  void saveSlitOnTop(bool t)    { resource.slit_on_top = t; }
  void saveSlitAutoHide(bool t) { resource.slit_auto_hide = t; }

  Toolbar *getToolbar(void) { return toolbar; }

  Workspace *getWorkspace(unsigned int index) const;
  const std::string& getWorkspaceName(unsigned int index) const;

  Workspace *getCurrentWorkspace(void) { return current_workspace; }

  Workspacemenu *getWorkspacemenu(void) { return workspacemenu; }

  unsigned int getBevelWidth(void) const
  { return resource.bevel_width; }
  unsigned int getBorderWidth(void) const
  { return resource.border_width; }

  unsigned int getCurrentWorkspaceID(void) const
  { return current_workspace_id; }
  unsigned int getWorkspaceCount(void) const
  { return workspacesList.size(); }
  unsigned int getIconCount(void) const { return iconList.size(); }
  unsigned int getNumberOfWorkspaces(void) const
  { return resource.workspaces; }
  int getToolbarPlacement(void) const
  { return resource.toolbar_placement; }
  int getToolbarWidthPercent(void) const
  { return resource.toolbar_width_percent; }
  int getPlacementPolicy(void) const
  { return resource.placement_policy; }
  int getEdgeSnapThreshold(void) const
  { return resource.edge_snap_threshold; }
  int getRowPlacementDirection(void) const
  { return resource.row_direction; }
  int getColPlacementDirection(void) const
  { return resource.col_direction; }

  BlackboxWindow* getWindow(unsigned int workspace, unsigned int id);

  void setRootColormapInstalled(bool r) { root_colormap_installed = r; }
  void saveSloppyFocus(bool s) { resource.sloppy_focus = s; }
  void saveAutoRaise(bool a) { resource.auto_raise = a; }
  void saveClickRaise(bool c) { resource.click_raise = c; }
  void saveWorkspaces(unsigned int w) { resource.workspaces = w; }
  void saveToolbarOnTop(bool r) { resource.toolbar_on_top = r; }
  void saveToolbarAutoHide(bool r) { resource.toolbar_auto_hide = r; }
  void saveToolbarWidthPercent(int w)
  { resource.toolbar_width_percent = w; }
  void saveToolbarPlacement(int p) { resource.toolbar_placement = p; }
  void savePlacementPolicy(int p) { resource.placement_policy = p; }
  void saveRowPlacementDirection(int d) { resource.row_direction = d; }
  void saveColPlacementDirection(int d) { resource.col_direction = d; }
  void saveEdgeSnapThreshold(int t)
  { resource.edge_snap_threshold = t; }
  void saveImageDither(bool d) { resource.image_dither = d; }
  void saveOpaqueMove(bool o) { resource.opaque_move = o; }
  void saveFullMax(bool f) { resource.full_max = f; }
  void savePlacementIgnoresShaded(bool f) { resource.ignore_shaded = f; }
  void saveFocusNew(bool f) { resource.focus_new = f; }
  void saveFocusLast(bool f) { resource.focus_last = f; }
  void saveAllowScrollLock(bool a) { resource.allow_scroll_lock = a; }

  const char *getStrftimeFormat(void)
  { return resource.strftime_format.c_str(); }
  void saveStrftimeFormat(const std::string& format);

  WindowStyle *getWindowStyle(void) { return &resource.wstyle; }
  ToolbarStyle *getToolbarStyle(void) { return &resource.tstyle; }

  BlackboxWindow *getIcon(unsigned int index);

  const bt::Rect& availableArea(void);
  void addStrut(bt::Netwm::Strut *strut);
  void removeStrut(bt::Netwm::Strut *strut);
  void updateStrut(void);

  void updateClientListHint(void) const;
  void updateClientListStackingHint(void) const;
  void updateDesktopNamesHint(void) const;

  unsigned int addWorkspace(void);
  unsigned int removeLastWorkspace(void);
  void addWorkspaceName(const std::string& name);
  void setWorkspaceName(unsigned int workspace, const std::string& name);
  const std::string getNameOfWorkspace(unsigned int id);
  void changeWorkspaceID(unsigned int id);
  void getDesktopNames(void);

  void iconifyWindow(BlackboxWindow *w);
  void removeIcon(BlackboxWindow *w);
  void addWindow(Window w);
  void releaseWindow(BlackboxWindow *w, bool remap);
  void raiseWindow(BlackboxWindow *w);
  void lowerWindow(BlackboxWindow *w);
  void reassociateWindow(BlackboxWindow *w, unsigned int wkspc_id);
  void propagateWindowName(const BlackboxWindow *w);

  void raiseWindows(const bt::Netwm::WindowList* const workspace_stack);

  void nextFocus(void) const;
  void prevFocus(void) const;
  void raiseFocus(void) const;
  void reconfigure(void);
  void toggleFocusModel(FocusModel model);
  void rereadMenu(void);
  void shutdown(void);
  void showPosition(int x, int y);
  void showGeometry(unsigned int gx, unsigned int gy);
  void hideGeometry(void);

  void clientMessageEvent(const XClientMessageEvent * const event);
  void buttonPressEvent(const XButtonEvent * const event);
  void configureRequestEvent(const XConfigureRequestEvent * const event);
};


#endif // __Screen_hh
