//
// Screen.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 1999 by Brad Hughes, bhughes@tcac.net
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// (See the included file COPYING / GPL-2.0)
//

#ifndef   __Screen_hh
#define   __Screen_hh

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include "LinkedList.hh"


typedef struct windowResource {
  struct decoration {
    BColor focusTextColor, unfocusTextColor;
    BTexture focusTexture, unfocusTexture;
  } decoration;
  
  struct button {
    BTexture focusTexture, unfocusTexture, pressedTexture;
  } button;
  
  struct frame {
    BTexture texture;
  } frame;
} windowResource;


typedef struct toolbarResource {
  struct toolbar {
    BColor textColor;
    BTexture texture;
  } toolbar;
  
  struct label {
    BTexture texture;
  } label;
  
  struct button {
    BTexture texture, pressedTexture;
  } button;
  
  struct clock {
    BTexture texture;
  } clock;
} toolbarResource;


typedef struct menuResource {
  struct title {
    BColor textColor;
    BTexture texture;
  } title;
  
  struct frame {
    BColor hiColor, textColor, hiTextColor;
    BTexture texture;
  } frame;
} menuResource;


class BScreen {
private:
  Bool root_colormap_installed, managed;
  Display *display;
  GC opGC, wfocusGC, wunfocusGC, mtitleGC, mframeGC, mhiGC, mhbgGC;
  Visual *visual;
  Window root_window;
  
  Blackbox *blackbox;
  BImageControl *image_control;
  Iconmenu *iconmenu;
  Rootmenu *rootmenu;

#ifdef    SLIT
  Slit *slit;
#endif // SLIT

  Toolbar *toolbar;
  Workspace *current_workspace;
  Workspacemenu *workspacemenu;
  
  int depth, screen_number;
  unsigned int xres, yres;
  unsigned long event_mask;
  
  LinkedList<char> *workspaceNames;
  LinkedList<Workspace> *workspacesList;

#ifdef    KDE
  LinkedList<Window> *kwm_module_list;
  LinkedList<Window> *kwm_window_list;
#endif // KDE

  struct resource {
    struct font {
      XFontStruct *title, *menu;
    } font;

    windowResource wres;
    toolbarResource tres;
    menuResource mres;

    Bool opaque_move, toolbar_on_top, sloppy_focus, auto_raise;
    BColor border_color;
    XrmDatabase stylerc;
    
    int workspaces, justify, menu_justify, toolbar_width_percent,
      placement_policy;

#ifdef    SLIT
    int slit_placement;
#endif // SLIT
    
    unsigned int handle_width, bevel_width;
    
#ifdef    HAVE_STRFTIME
    char *strftime_format;
#else //  HAVE_STRFTIME
    Bool clock24hour;
    int date_format;
#endif // HAVE_STRFTIME
    
  } resource;
  
  
protected:
  Bool parseMenuFile(FILE *, Rootmenu *);

  void readDatabaseTexture(char *, char *, BTexture *);
  void readDatabaseColor(char *, char *, BColor *);
  void InitMenu(void);
  void LoadStyle(void);
  
  
public:
  BScreen(Blackbox *, int);
  ~BScreen(void);
  
  Bool isToolbarOnTop(void)          { return resource.toolbar_on_top; }
  Bool isSloppyFocus(void)           { return resource.sloppy_focus; }
  Bool doAutoRaise(void)             { return resource.auto_raise; }
  Bool doOpaqueMove(void)            { return resource.opaque_move; }
  Bool isScreenManaged(void)         { return managed; }
  Bool isRootColormapInstalled(void) { return root_colormap_installed; }
  Display *getDisplay(void)          { return display; }
  GC getOpGC(void)                   { return opGC; }
  GC getWindowFocusGC(void)          { return wfocusGC; }
  GC getWindowUnfocusGC(void)        { return wunfocusGC; }
  GC getMenuTitleGC(void)            { return mtitleGC; }
  GC getMenuFrameGC(void)            { return mframeGC; }
  GC getMenuHiGC(void)               { return mhiGC; }
  GC getMenuHiBGGC(void)             { return mhbgGC; }
  Visual *getVisual(void)            { return visual; }
  Window getRootWindow(void)         { return root_window; }
  XFontStruct *getTitleFont(void)    { return resource.font.title; }
  XFontStruct *getMenuFont(void)     { return resource.font.menu; }
  
  Blackbox *getBlackbox(void)            { return blackbox; }
  BImageControl *getImageControl(void)   { return image_control; }
  Rootmenu *getRootmenu(void)            { return rootmenu; }

#ifdef   SLIT
  Slit *getSlit(void)           { return slit; }
  int getSlitPlacement(void)    { return resource.slit_placement; }
  void saveSlitPlacement(int p) { resource.slit_placement = p; }
#endif // SLIT
  
  Toolbar *getToolbar(void)              { return toolbar; }
  Workspace *getWorkspace(int);
  Workspace *getCurrentWorkspace(void)   { return current_workspace; }
  Workspacemenu *getWorkspacemenu(void)  { return workspacemenu; }

  const BColor &getBorderColor(void)         { return resource.border_color; }
  const int getJustification(void) const     { return resource.justify; }
  const int getMenuJustification(void) const { return resource.menu_justify; }
  const unsigned int getHandleWidth(void)    { return resource.handle_width; }
  const unsigned int getBevelWidth(void)     { return resource.bevel_width; }

  int getDepth(void)               { return depth; }
  int getScreenNumber(void)        { return screen_number; }
  int addWorkspace(void);
  int removeLastWorkspace(void);
  int getCount(void)               { return workspacesList->count(); }
  int getCurrentWorkspaceID(void);
  int getNumberOfWorkspaces(void)  { return resource.workspaces; }
  int getToolbarWidthPercent(void) { return resource.toolbar_width_percent; }
  int getPlacementPolicy(void)     { return resource.placement_policy; }

  unsigned int getXRes(void) { return xres; }
  unsigned int getYRes(void) { return yres; }
  
  void getNameOfWorkspace(int, char **);
  void changeWorkspaceID(int);
  void addIcon(BlackboxIcon *i);
  void removeIcon(BlackboxIcon *i);
  void iconUpdate(void);
  void raiseWindows(Window *, int);
  void reassociateWindow(BlackboxWindow *);
  void prevFocus(void);
  void nextFocus(void);
  void raiseFocus(void);
  void setRootColormapInstalled(Bool c) { root_colormap_installed = c; }
  void reconfigure(void);
  void shutdown(void);

  void saveSloppyFocus(Bool s)        { resource.sloppy_focus = s; }
  void saveAutoRaise(Bool a)          { resource.auto_raise = a; }
  void saveWorkspaces(int w)          { resource.workspaces = w; }
  void removeWorkspaceNames(void);
  void addWorkspaceName(char *);
  void saveToolbarOnTop(Bool r)       { resource.toolbar_on_top = r; }
  void saveToolbarWidthPercent(int w) { resource.toolbar_width_percent = w; }
  void savePlacementPolicy(int p)     { resource.placement_policy = p; }
  
#ifdef    HAVE_STRFTIME
  char *getStrftimeFormat(void) { return resource.strftime_format; }
  void saveStrftimeFormat(char *);
#else //  HAVE_STRFTIME
  int getDateFormat(void)       { return resource.date_format; }
  void saveDateFormat(int f)    { resource.date_format = f; }
  Bool isClock24Hour(void)      { return resource.clock24hour; }
  void saveClock24Hour(Bool c)  { resource.clock24hour = b; }
#endif // HAVE_STRFTIME

  windowResource *getWResource(void)  { return &resource.wres; }
  menuResource *getMResource(void)    { return &resource.mres; }
  toolbarResource *getTResource(void) { return &resource.tres; }

#ifdef    KDE
  Bool isKWMModule(Window);
  
  void sendToKWMModules(Atom, XID);
  void sendToKWMModules(XClientMessageEvent *);
  void sendClientMessage(Window, Atom, XID);

  void addKWMModule(Window w);
  void addKWMWindow(Window w);
  void removeKWMWindow(Window w);
  void scanWorkspaceNames(void);
#endif // KDE
  
  enum { SmartPlacement = 1, CascadePlacement };

#ifdef    SLIT
  enum { TopLeft = 1, CenterLeft, BottomLeft, TopRight, CenterRight,
	 BottomRight };
#endif // SLIT

};


#endif // __Screen_hh
