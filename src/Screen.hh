// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Screen.hh for Blackbox - an X11 Window manager
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

#ifndef   __Screen_hh
#define   __Screen_hh

#include <X11/Xlib.h>

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

#include <stdio.h>

#include "BaseDisplay.hh"
#include "LinkedList.hh"
#include "Style.hh"
#include "Timer.hh"

// forward declarations
class Blackbox;
class BImageControl;
class Configmenu;
class Iconmenu;
class Netizen;
class Rootmenu;
class Toolbar;
class Workspace;
class Workspacemenu;

#ifdef    SLIT
class Slit;
#endif // SLIT

class BlackboxWindow;


struct NETStrut
{
  unsigned int top, bottom, left, right;

  NETStrut(): top(0), bottom(0), left(0), right(0) {}
};

class BScreen : public ScreenInfo
{
public:
  BScreen( Blackbox *, int );
  ~BScreen();

  void initialize();

  inline BImageControl *getImageControl() { return image_control; }
  inline Rootmenu *getRootmenu() { return rootmenu; }
  inline Toolbar *getToolbar() { return toolbar; }

  inline Workspace *getWorkspace(int w) { return workspacesList->find(w); }
  inline Workspace *getCurrentWorkspace() { return current_workspace; }

  inline Workspacemenu *getWorkspacemenu() { return workspacemenu; }

  int getCurrentWorkspaceID() const;
  inline const int getCount() { return workspacesList->count(); }
  inline const int getIconCount() { return iconList->count(); }

  inline void setRootColormapInstalled(Bool r) { root_colormap_installed = r; }

  BStyle *style() { return &_style; }

  BlackboxWindow *icon(int);

  const XRectangle& availableArea() const;
  void updateAvailableArea();
  void addStrut(NETStrut *strut);

  int addWorkspace();
  int removeLastWorkspace();

  void removeWorkspaceNames();
  void addWorkspaceName(char *);
  void addNetizen(Netizen *);
  void removeNetizen(Window);
  void addIcon(BlackboxWindow *);
  void removeIcon(BlackboxWindow *);
  void changeIconName(BlackboxWindow *);
  char* getNameOfWorkspace(int);
  void changeWorkspaceID(int);
  void raiseWindows(Window *, int);
  void reassociateWindow(BlackboxWindow *, int, Bool);
  void prevFocus();
  void nextFocus();
  void raiseFocus();
  void reconfigure();
  void rereadMenu();
  void shutdown();
  void showPosition(int, int);
  void showGeometry(unsigned int, unsigned int);
  void hideGeometry();

  void updateNetizenCurrentWorkspace();
  void updateNetizenWorkspaceCount();
  void updateNetizenWindowFocus();
  void updateNetizenWindowAdd(Window, unsigned long);
  void updateNetizenWindowDel(Window);
  void updateNetizenConfigNotify(XEvent *);
  void updateNetizenWindowRaise(Window);
  void updateNetizenWindowLower(Window);

  enum { RowSmartPlacement = 1, ColSmartPlacement, CascadePlacement, LeftRight,
         RightLeft, TopBottom, BottomTop };
  enum { RoundBullet = 1, TriangleBullet, SquareBullet, NoBullet };





  // this should all go away


  Bool isToolbarOnTop() const { return resource.toolbar_on_top; }
  Bool doToolbarAutoHide() const { return resource.toolbar_auto_hide; }
  Bool isSloppyFocus() const { return resource.sloppy_focus; }
  Bool isRootColormapInstalled() const { return root_colormap_installed; }
  Bool doAutoRaise() const { return resource.auto_raise; }
  Bool isScreenManaged() const { return managed; }
  Bool doImageDither() const { return resource.image_dither; }
  Bool doOrderedDither() const { return resource.ordered_dither; }
  Bool doOpaqueMove() const { return resource.opaque_move; }
  Bool doFullMax() const { return resource.full_max; }
  Bool doFocusNew() const { return resource.focus_new; }
  Bool doFocusLast() const { return resource.focus_last; }

#ifdef   SLIT
  Bool isSlitOnTop() const { return resource.slit_on_top; }
  Bool doSlitAutoHide() const { return resource.slit_auto_hide; }
  Slit *getSlit() { return slit; }
  const int &getSlitPlacement() const { return resource.slit_placement; }
  const int &getSlitDirection() const { return resource.slit_direction; }
  void saveSlitPlacement(int p) { resource.slit_placement = p; }
  void saveSlitDirection(int d) { resource.slit_direction = d; }
  void saveSlitOnTop(Bool t)    { resource.slit_on_top = t; }
  void saveSlitAutoHide(Bool t) { resource.slit_auto_hide = t; }
#endif // SLIT

  int getNumberOfWorkspaces() const { return resource.workspaces; }
  int getToolbarPlacement() const { return resource.toolbar_placement; }
  int getToolbarWidthPercent() const { return resource.toolbar_width_percent; }
  int getPlacementPolicy() const { return resource.placement_policy; }
  int getEdgeSnapThreshold() const { return resource.edge_snap_threshold; }
  int getRowPlacementDirection() const { return resource.row_direction; }
  int getColPlacementDirection() const { return resource.col_direction; }

  void saveSloppyFocus(Bool s) { resource.sloppy_focus = s; }
  void saveAutoRaise(Bool a) { resource.auto_raise = a; }
  void saveWorkspaces(int w) { resource.workspaces = w; }
  void saveToolbarOnTop(Bool r) { resource.toolbar_on_top = r; }
  void saveToolbarAutoHide(Bool r) { resource.toolbar_auto_hide = r; }
  void saveToolbarWidthPercent(int w) { resource.toolbar_width_percent = w; }
  void saveToolbarPlacement(int p) { resource.toolbar_placement = p; }
  void savePlacementPolicy(int p) { resource.placement_policy = p; }
  void saveRowPlacementDirection(int d) { resource.row_direction = d; }
  void saveColPlacementDirection(int d) { resource.col_direction = d; }
  void saveEdgeSnapThreshold(int t) { resource.edge_snap_threshold = t; }
  void saveImageDither(Bool d) { resource.image_dither = d; }
  void saveOpaqueMove(Bool o) { resource.opaque_move = o; }
  void saveFullMax(Bool f) { resource.full_max = f; }
  void saveFocusNew(Bool f) { resource.focus_new = f; }
  void saveFocusLast(Bool f) { resource.focus_last = f; }

#ifdef    HAVE_STRFTIME
  char *getStrftimeFormat() { return resource.strftime_format; }
  void saveStrftimeFormat(char *);
#else // !HAVE_STRFTIME
  int getDateFormat() { return resource.date_format; }
  void saveDateFormat(int f) { resource.date_format = f; }
  Bool isClock24Hour() { return resource.clock24hour; }
  void saveClock24Hour(Bool c) { resource.clock24hour = c; }
#endif // HAVE_STRFTIME

protected:
  Bool parseMenuFile(FILE *, Rootmenu *);
  void InitMenu();

private:
  Blackbox *blackbox;

  Bool root_colormap_installed, managed;

  BImageControl *image_control;
  Configmenu *configmenu;
  Iconmenu *iconmenu;
  Rootmenu *rootmenu;

  LinkedList<Netizen> *netizenList;
  LinkedList<BlackboxWindow> *iconList;

#ifdef    SLIT
  Slit *slit;
#endif // SLIT

  Toolbar *toolbar;
  Workspace *current_workspace;
  Workspacemenu *workspacemenu;

  unsigned int geom_w, geom_h;
  unsigned long event_mask;

  XRectangle usableArea;

  LinkedList<NETStrut> *strutList;
  LinkedList<char> *workspaceNames;
  LinkedList<Workspace> *workspacesList;

  struct resource {
    Bool toolbar_on_top, toolbar_auto_hide, sloppy_focus, auto_raise,
      auto_edge_balance, image_dither, ordered_dither, opaque_move, full_max,
      focus_new, focus_last;

    int workspaces, toolbar_placement, toolbar_width_percent, placement_policy,
      edge_snap_threshold, row_direction, col_direction;

#ifdef    SLIT
    Bool slit_on_top, slit_auto_hide;
    int slit_placement, slit_direction;
#endif // SLIT

#ifdef    HAVE_STRFTIME
    char *strftime_format;
#else // !HAVE_STRFTIME
    Bool clock24hour;
    int date_format;
#endif // HAVE_STRFTIME

  } resource;

  BStyle _style;
};


#endif // __Screen_hh
