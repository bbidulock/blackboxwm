// Screen.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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

// forward declaration
class BScreen;

#include "BaseDisplay.hh"
#include "Configmenu.hh"
#include "Icon.hh"
#include "LinkedList.hh"
#include "Netizen.hh"
#include "Rootmenu.hh"
#include "Timer.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"
#include "blackbox.hh"

#ifdef    SLIT
#  include "Slit.hh"
#endif // SLIT

typedef struct WindowStyle {
  BColor l_text_focus, l_text_unfocus, b_pic_focus, b_pic_unfocus;
  BTexture t_focus, t_unfocus, l_focus, l_unfocus, h_focus, h_unfocus,
    b_focus, b_unfocus, b_pressed, f_focus, f_unfocus, g_focus, g_unfocus;
  GC l_text_focus_gc, l_text_unfocus_gc, b_pic_focus_gc, b_pic_unfocus_gc;
  XFontStruct *font;

  int justify;
} WindowStyle;

typedef struct ToolbarStyle {
  BColor l_text, w_text, c_text, b_pic;
  BTexture toolbar, label, window, button, pressed, clock;
  GC l_text_gc, w_text_gc, c_text_gc, b_pic_gc;
  XFontStruct *font;

  int justify;
} ToolbarStyle;

typedef struct MenuStyle {
  BColor t_text, f_text, h_text, d_text;
  BTexture title, frame, hilite;
  GC t_text_gc, f_text_gc, h_text_gc, d_text_gc;
  XFontStruct *t_font, *f_font;

  int t_justify, f_justify, bullet, bullet_pos;
} MenuStyle;


class BScreen : public ScreenInfo {
private:
  Bool root_colormap_installed, managed, geom_visible;
  GC opGC;
  Pixmap geom_pixmap;
  Window geom_window;
  
  Blackbox *blackbox;
  BImageControl *image_control;
  Configmenu *configmenu;
  Iconmenu *iconmenu;
  Rootmenu *rootmenu;

  LinkedList<Rootmenu> *rootmenuList;
  LinkedList<Netizen> *netizenList;
  
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
  
  struct resource {
    WindowStyle wstyle;
    ToolbarStyle tstyle;
    MenuStyle mstyle;

    Bool toolbar_on_top, sloppy_focus, auto_raise, auto_edge_balance,
      image_dither, ordered_dither, opaque_move, full_max, focus_new;
    BColor border_color;
    XrmDatabase stylerc;
    
    int workspaces, toolbar_placement, toolbar_width_percent, placement_policy,
      edge_snap_threshold;

#ifdef    SLIT
    Bool slit_on_top;
    int slit_placement, slit_direction;
#endif // SLIT
    
    unsigned int handle_width, bevel_width, border_width, border_width_2x;
    
#ifdef    HAVE_STRFTIME
    char *strftime_format;
#else // !HAVE_STRFTIME
    Bool clock24hour;
    int date_format;
#endif // HAVE_STRFTIME
    
  } resource;
  
  
protected:
  Bool parseMenuFile(FILE *, Rootmenu *);

  void readDatabaseTexture(char *, char *, BTexture *, unsigned long);
  void readDatabaseColor(char *, char *, BColor *, unsigned long);
  void readDatabaseFont(char *, char *, XFontStruct **);
  void InitMenu(void);
  void LoadStyle(void);


public:
  BScreen(Blackbox *, int);
  ~BScreen(void);

  inline const Bool &isToolbarOnTop(void) const
    { return resource.toolbar_on_top; }
  inline const Bool &isSloppyFocus(void) const
    { return resource.sloppy_focus; }
  inline const Bool &isRootColormapInstalled(void) const
    { return root_colormap_installed; }
  inline const Bool &doAutoRaise(void) const { return resource.auto_raise; }
  inline const Bool &isScreenManaged(void) const { return managed; }
  inline const Bool &doImageDither(void) const
    { return resource.image_dither; }
  inline const Bool &doOrderedDither(void) const
    { return resource.ordered_dither; }
  inline const Bool &doOpaqueMove(void) const { return resource.opaque_move; }
  inline const Bool &doFullMax(void) const { return resource.full_max; }
  inline const Bool &doFocusNew(void) const { return resource.focus_new; }
  
  inline const GC &getOpGC() const { return opGC; }
  
  inline Blackbox *getBlackbox(void) { return blackbox; }
  inline BColor *getBorderColor(void) { return &resource.border_color; }
  inline BImageControl *getImageControl(void) { return image_control; }
  inline Rootmenu *getRootmenu(void) { return rootmenu; }

#ifdef   SLIT
  inline const Bool &isSlitOnTop(void) const { return resource.slit_on_top; }
  inline Slit *getSlit(void) { return slit; }
  inline const int &getSlitPlacement(void) const
    { return resource.slit_placement; }
  inline const int &getSlitDirection(void) const
    { return resource.slit_direction; }
  inline void saveSlitPlacement(int p) { resource.slit_placement = p; }
  inline void saveSlitDirection(int d) { resource.slit_direction = d; }
  inline void saveSlitOnTop(Bool t) { resource.slit_on_top = t; }
#endif // SLIT
  
  inline Toolbar *getToolbar(void) { return toolbar; }

  inline Workspace *getWorkspace(int w) { return workspacesList->find(w); }
  inline Workspace *getCurrentWorkspace(void) { return current_workspace; }

  inline Workspacemenu *getWorkspacemenu(void) { return workspacemenu; }
  
  inline const unsigned int &getHandleWidth(void) const
    { return resource.handle_width; }
  inline const unsigned int &getBevelWidth(void) const
    { return resource.bevel_width; }
  inline const unsigned int &getBorderWidth(void) const
    { return resource.border_width; }
  inline const unsigned int &getBorderWidth2x(void) const
    { return resource.border_width_2x; }

  inline const int getCurrentWorkspaceID()
    { return current_workspace->getWorkspaceID(); }
  inline const int getCount(void) { return workspacesList->count(); }
  inline const int &getNumberOfWorkspaces(void) const
    { return resource.workspaces; }
  inline const int &getToolbarPlacement(void) const
    { return resource.toolbar_placement; }
  inline const int &getToolbarWidthPercent(void) const
    { return resource.toolbar_width_percent; }
  inline const int &getPlacementPolicy(void) const
    { return resource.placement_policy; }
  inline const int &getEdgeSnapThreshold(void) const
    { return resource.edge_snap_threshold; }

  inline void setRootColormapInstalled(Bool r) { root_colormap_installed = r; }
  inline void saveSloppyFocus(Bool s) { resource.sloppy_focus = s; }
  inline void saveAutoRaise(Bool a) { resource.auto_raise = a; }
  inline void saveWorkspaces(int w) { resource.workspaces = w; }
  inline void saveToolbarOnTop(Bool r) { resource.toolbar_on_top = r; }
  inline void saveToolbarWidthPercent(int w)
    { resource.toolbar_width_percent = w; }
  inline void saveToolbarPlacement(int p) { resource.toolbar_placement = p; }
  inline void savePlacementPolicy(int p) { resource.placement_policy = p; }
  inline void saveEdgeSnapThreshold(int t)
    { resource.edge_snap_threshold = t; }
  inline void saveImageDither(Bool d) { resource.image_dither = d; }
  inline void saveOpaqueMove(Bool o) { resource.opaque_move = o; }
  inline void saveFullMax(Bool f) { resource.full_max = f; }
  inline void saveFocusNew(Bool f) { resource.focus_new = f; }
  inline void addIcon(BlackboxIcon *icon) { iconmenu->insert(icon); } 
  inline void removeIcon(BlackboxIcon *icon) { iconmenu->remove(icon); }
  inline void iconUpdate(void) { iconmenu->update(); }

#ifdef    HAVE_STRFTIME
  inline char *getStrftimeFormat(void) { return resource.strftime_format; }
  void saveStrftimeFormat(char *);
#else // !HAVE_STRFTIME
  inline int getDateFormat(void) { return resource.date_format; }
  inline void saveDateFormat(int f) { resource.date_format = f; }
  inline Bool isClock24Hour(void) { return resource.clock24hour; }
  inline void saveClock24Hour(Bool c) { resource.clock24hour = b; }
#endif // HAVE_STRFTIME

  inline WindowStyle *getWindowStyle(void) { return &resource.wstyle; }
  inline MenuStyle *getMenuStyle(void) { return &resource.mstyle; }
  inline ToolbarStyle *getToolbarStyle(void) { return &resource.tstyle; }

  int addWorkspace(void);
  int removeLastWorkspace(void);

  void removeWorkspaceNames(void);
  void addWorkspaceName(char *);
  void addNetizen(Netizen *);
  void removeNetizen(Window);
  void getNameOfWorkspace(int, char **);
  void changeWorkspaceID(int);
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
  void updateNetizenCurrentWorkspace(void);
  void updateNetizenWorkspaceCount(void);
  void updateNetizenWindowFocus(void);
  void updateNetizenWindowAdd(Window, unsigned long);
  void updateNetizenWindowDel(Window);
  void updateNetizenConfigNotify(XEvent *);
  void updateNetizenWindowRaise(Window);
  void updateNetizenWindowLower(Window);

  enum { RowSmartPlacement = 1, ColSmartPlacement, CascadePlacement };
  enum { LeftJustify = 1, RightJustify, CenterJustify };
  enum { RoundBullet = 1, TriangleBullet, SquareBullet, NoBullet };
  enum { Restart = 1, RestartOther, Exit, Shutdown, Execute, Reconfigure,
         WindowShade, WindowIconify, WindowMaximize, WindowClose, WindowRaise,
         WindowLower, WindowStick, WindowKill, SetStyle };
};


#endif // __Screen_hh
