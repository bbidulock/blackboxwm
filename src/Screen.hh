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

#include "BaseDisplay.hh"
#include "blackbox.hh"
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


class BScreen : public ScreenInfo {
private:
  Bool root_colormap_installed, managed, geom_visible;
  GC opGC, wfocusGC, wunfocusGC, mtitleGC, mframeGC, mhiGC, mhbgGC;
  Pixmap geom_pixmap;
  Window geom_window;
  
  Blackbox *blackbox;
  BImageControl *image_control;
  Iconmenu *iconmenu;
  Rootmenu *rootmenu;

  LinkedList<Rootmenu> *rootmenuList;
  
#ifdef    SLIT
  Slit *slit;
#endif // SLIT

  Toolbar *toolbar;
  Workspace *current_workspace;
  Workspacemenu *workspacemenu;
  
  unsigned int geom_w, geom_h;
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

    Bool toolbar_on_top, sloppy_focus, auto_raise;
    BColor border_color;
    XrmDatabase stylerc;
    
    int workspaces, justify, menu_justify, toolbar_width_percent,
      placement_policy, menu_bullet_style, menu_bullet_pos;

#ifdef    SLIT
    int slit_placement;
#endif // SLIT
    
    unsigned int handle_width, bevel_width, border_width;
    
#ifdef    HAVE_STRFTIME
    char *strftime_format;
#else // !HAVE_STRFTIME
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
  Bool isRootColormapInstalled(void) { return root_colormap_installed; }
  Bool doAutoRaise(void)             { return resource.auto_raise; }
  Bool isScreenManaged(void)         { return managed; }
  GC getOpGC()                       { return opGC; }
  GC getWindowFocusGC(void)          { return wfocusGC; }
  GC getWindowUnfocusGC(void)        { return wunfocusGC; }
  GC getMenuTitleGC(void)            { return mtitleGC; }
  GC getMenuFrameGC(void)            { return mframeGC; }
  GC getMenuHiGC(void)               { return mhiGC; }
  GC getMenuHiBGGC(void)             { return mhbgGC; }
  XFontStruct *getTitleFont(void)    { return resource.font.title; }
  XFontStruct *getMenuFont(void)     { return resource.font.menu; }
  
  Blackbox *getBlackbox(void)            { return blackbox; }
  BColor *getBorderColor(void)           { return &resource.border_color; }
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

  const int getJustification(void) const     { return resource.justify; }
  const int getMenuJustification(void) const { return resource.menu_justify; }
  const unsigned int getHandleWidth(void)    { return resource.handle_width; }
  const unsigned int getBevelWidth(void)     { return resource.bevel_width; }
  const unsigned int getBorderWidth(void)    { return resource.border_width; }

  int addWorkspace(void);
  int removeLastWorkspace(void);
  int getCount(void)               { return workspacesList->count(); }
  int getCurrentWorkspaceID(void);
  int getNumberOfWorkspaces(void)  { return resource.workspaces; }
  int getToolbarWidthPercent(void) { return resource.toolbar_width_percent; }
  int getPlacementPolicy(void)     { return resource.placement_policy; }
  int getBulletStyle(void)         { return resource.menu_bullet_style; }
  int getBulletPosition(void)      { return resource.menu_bullet_pos; }

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
  void reconfigure(void);
  void rereadMenu(void);
  void shutdown(void);
  void showPosition(int, int);
  void showGeometry(unsigned int, unsigned int);
  void hideGeometry(void);
  void setRootColormapInstalled(Bool r) { root_colormap_installed = r; }

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
#else // !HAVE_STRFTIME
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
  enum { LeftJustify = 1, RightJustify, CenterJustify };
  enum { RoundBullet = 1, TriangleBullet, SquareBullet, NoBullet };
  enum { Restart = 1, RestartOther, Exit, Shutdown, Execute, Reconfigure,
         WindowShade, WindowIconify, WindowMaximize, WindowClose, WindowRaise,
         WindowLower, WindowStick, WindowKill, SetStyle };

#ifdef    SLIT
  enum { TopLeft = 1, CenterLeft, BottomLeft, TopRight, CenterRight,
	 BottomRight };
#endif // SLIT

};


#endif // __Screen_hh
