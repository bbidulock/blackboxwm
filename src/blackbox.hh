//
// blackbox.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@arn.net
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

#ifndef __Blackbox_hh
#define __Blackbox_hh
#define __blackbox_version "beta zero point three four point four"

#include <X11/Xlib.h>
#include <X11/Xresource.h>

// forward declarations
class Blackbox;
class WorkspaceManager;
class Basemenu;
class Rootmenu;
class BlackboxIcon;
class BlackboxWindow;
class Application;

#include "LinkedList.hh"
#include "graphics.hh"

#include <stdio.h>


class Blackbox {
private:
  // internal structures
  typedef struct GroupSearch {
    BlackboxWindow *data;
    Window window;
  } GroupSearch;

  typedef struct WindowSearch {
    BlackboxWindow *data;
    Window window;
  } WindowSearch;

  typedef struct MenuSearch {
    Basemenu *data;
    Window window;
  } MenuSearch;

  typedef struct WSManagerSearch {
    WorkspaceManager *data;
    Window window;
  } WSManagerSearch;

  typedef struct ApplicationSearch {
    Application *data;
    Window window;
  } ApplicationSearch;

  struct cursor {
    Cursor session, move;
  } cursor;

  struct resource {
    struct font {
      XFontStruct *title, *menu;
    } font;

    struct wsm {
      BColor windowColor, windowColorTo, buttonColor, buttonColorTo,
	labelColor, labelColorTo, clockColor, clockColorTo, textColor;
      unsigned long windowTexture, buttonTexture, labelTexture, clockTexture;
    } wsm;

    struct win {
      BColor focusColor, focusColorTo, unfocusColor, unfocusColorTo,
	frameColor, focusTextColor, unfocusTextColor;
      unsigned long decorTexture, handleTexture, frameTexture, buttonTexture;
    } win;
    
    struct menu {
      BColor titleColor, titleColorTo, frameColor, frameColorTo,
	highlightColor, titleTextColor, frameTextColor, hiTextColor;
      unsigned long titleTexture, frameTexture;
    } menu;

    struct icon {
      BColor color, colorTo, textColor;
      unsigned long texture;
    } icon;

    Bool opaqueMove, imageDither, clock24hour;
    BColor borderColor;
    XrmDatabase blackboxrc;
    char *menuFile;
    int workspaces, justification, cpc8bpp;
    unsigned int handleWidth, bevelWidth;
  } resource;

  struct shape {
    Bool extensions;
    int event_basep, error_basep;
  } shape;

  // context linked lists
  LinkedList<WindowSearch> *windowSearchList;
  LinkedList<MenuSearch> *menuSearchList;
  LinkedList<WSManagerSearch> *wsManagerSearchList;
  LinkedList<GroupSearch> *groupSearchList;
  LinkedList<ApplicationSearch> *appSearchList;

  // internal variables for operation
  Rootmenu *rootmenu;
  WorkspaceManager *wsManager;

  Atom _XA_WM_COLORMAP_WINDOWS, _XA_WM_PROTOCOLS, _XA_WM_STATE,
    _XA_WM_DELETE_WINDOW, _XA_WM_TAKE_FOCUS, _BLACKBOX_MESSAGE,
    _BLACKBOX_CONTROL;
  Bool startup, shutdown, reconfigure;
  Display *display;
  GC opGC;
  Visual *v;
  Window root;
  XColor *colors_8bpp;
  char *display_name;
  char **b_argv;
  int bpp, depth, screen, event_mask, focus_window_number, b_argc;
  unsigned int xres, yres;


protected:
  // internal initialization
  void InitMenu(void);
  void InitColor(void);
  void LoadDefaults(void);

  // X resource database lookups
  unsigned long readDatabaseTexture(char *, char *);
  Bool readDatabaseColor(char *, char *, BColor *);

  // event processing and dispatching
  void ProcessEvent(XEvent *);

  // internal routines
  void do_reconfigure(void);

  // value lookups and retrieval
  Bool parseMenuFile(FILE *, Rootmenu *);


public:
  Blackbox(int, char **, char * = 0);
  ~Blackbox(void);

  // member pointers
  Rootmenu *Menu(void) { return rootmenu; }
  WorkspaceManager *workspaceManager(void) { return wsManager; }

  // window validation
  Bool validateWindow(Window);

  // context lookup routines
  Basemenu *searchMenu(Window);
  BlackboxWindow *searchWindow(Window);
  WorkspaceManager *searchWSManager(Window);
  BlackboxWindow *searchGroup(Window, BlackboxWindow *);
  Application *searchApp(Window);

  // reassociated a window with the current workspace
  void reassociateWindow(BlackboxWindow *);

  // context list save/remove methods
  void saveMenuSearch(Window, Basemenu *);
  void saveWindowSearch(Window, BlackboxWindow *);
  void saveWSManagerSearch(Window, WorkspaceManager *);
  void saveGroupSearch(Window, BlackboxWindow *);
  void saveAppSearch(Window, Application *);
  void removeMenuSearch(Window);
  void removeWindowSearch(Window);
  void removeWSManagerSearch(Window);
  void removeGroupSearch(Window);
  void removeAppSearch(Window);

  // main event loop
  void EventLoop(void);

  // window manager functions
  void Exit(void);
  void Restart(char * = 0);
  void Reconfigure(void);
  void Shutdown(Bool = True);

  // various informative functions about the current X session
  Atom StateAtom(void) { return _XA_WM_STATE; }
  Atom DeleteAtom(void) { return _XA_WM_DELETE_WINDOW; }
  Atom ProtocolsAtom(void) { return _XA_WM_PROTOCOLS; }
  Atom FocusAtom(void) { return _XA_WM_TAKE_FOCUS; }
  Atom ColormapAtom(void) { return _XA_WM_COLORMAP_WINDOWS; }

  Bool shapeExtensions(void) { return shape.extensions; }
  Bool Startup(void) { return startup; }

  Cursor sessionCursor(void) { return cursor.session; }
  Cursor moveCursor(void) { return cursor.move; }

  Display *control(void) { return display; }

  GC GCOperations(void) { return opGC; }

  Visual *visual(void) { return v; }

  Window Root(void) { return root; }

  // color alloc and lookup
  Bool imageDither(void) { return resource.imageDither; }
  XColor *Colors8bpp(void) { return colors_8bpp; }
  int cpc8bpp(void) { return resource.cpc8bpp; }
  unsigned long getColor(const char *);
  unsigned long getColor(const char *, unsigned char *, unsigned char *,
			 unsigned char *);

  // session controls
  XFontStruct *titleFont(void) { return resource.font.title; }
  XFontStruct *menuFont(void) { return resource.font.menu; }
  const BColor &borderColor(void) { return resource.borderColor; }
  const unsigned int handleWidth(void) { return resource.handleWidth; }
  const unsigned int bevelWidth(void) { return resource.bevelWidth; }

  // window controls
  unsigned long wDecorTexture(void) { return resource.win.decorTexture; }
  unsigned long wButtonTexture(void) { return resource.win.buttonTexture; }
  unsigned long wHandleTexture(void) { return resource.win.handleTexture; }
  unsigned long wFrameTexture(void) { return resource.win.frameTexture; }
  const BColor &wFrameColor(void) { return resource.win.frameColor; }
  const BColor &wFColor(void) { return resource.win.focusColor; }
  const BColor &wFColorTo(void) { return resource.win.focusColorTo; }
  const BColor &wUColor(void) { return resource.win.unfocusColor; }
  const BColor &wUColorTo(void) { return resource.win.unfocusColorTo; }
  const BColor &wFTextColor(void) { return resource.win.focusTextColor; }
  const BColor &wUTextColor(void) { return resource.win.unfocusTextColor; }

  // menu controls
  unsigned long mTitleTexture(void) { return resource.menu.titleTexture; }
  unsigned long mFrameTexture(void) { return resource.menu.frameTexture; }
  const BColor &mTColor(void) { return resource.menu.titleColor; }
  const BColor &mTColorTo(void) { return resource.menu.titleColorTo; }
  const BColor &mFColor(void) { return resource.menu.frameColor; }
  const BColor &mFColorTo(void) { return resource.menu.frameColorTo; }
  const BColor &mHColor(void) { return resource.menu.highlightColor; }
  const BColor &mTTextColor(void) { return resource.menu.titleTextColor; }
  const BColor &mFTextColor(void) { return resource.menu.frameTextColor; }
  const BColor &mHTextColor(void) { return resource.menu.hiTextColor; }

  // workspace manager controls
  unsigned long sWindowTexture(void) { return resource.wsm.windowTexture; }
  unsigned long sButtonTexture(void) { return resource.wsm.buttonTexture; }
  unsigned long sLabelTexture(void) { return resource.wsm.labelTexture; }
  unsigned long sClockTexture(void) { return resource.wsm.clockTexture; }
  const BColor &sWColor(void) { return resource.wsm.windowColor; }
  const BColor &sWColorTo(void) { return resource.wsm.windowColorTo; }
  const BColor &sLColor(void) { return resource.wsm.labelColor; }
  const BColor &sLColorTo(void) { return resource.wsm.labelColorTo; }
  const BColor &sBColor(void) { return resource.wsm.buttonColor; }
  const BColor &sBColorTo(void) { return resource.wsm.buttonColorTo; }
  const BColor &sCColor(void) { return resource.wsm.clockColor; }
  const BColor &sCColorTo(void) { return resource.wsm.clockColorTo; }
  const BColor &sTextColor(void) { return resource.wsm.textColor; }  
  Bool clock24Hour(void) { return resource.clock24hour; }

  // icon controls
  unsigned long iconTexture(void) { return resource.icon.texture; }
  const BColor &iconColor(void) { return resource.icon.color; }
  const BColor &iconColorTo(void) { return resource.icon.colorTo; }
  const BColor &iTextColor(void) { return resource.icon.textColor; }

  // session information
  int Depth(void) { return depth; }
  int BitsPerPixel(void) { return bpp; }

  unsigned int XResolution(void) { return xres; }
  unsigned int YResolution(void) { return yres; }

  // controls for arrangement of decorations
  const int Justification(void) const { return resource.justification; }
  
  // window move style
  Bool opaqueMove(void) { return resource.opaqueMove; }
  
  // public constants
  enum { B_Restart = 1, B_RestartOther, B_Exit, B_Shutdown, B_Execute,
	 B_Reconfigure, B_ExecReconfigure, B_WindowShade, B_WindowIconify,
	 B_WindowMaximize, B_WindowClose, B_WindowRaise, B_WindowLower,
	 B_WindowStick, B_WindowKill };
  enum { B_LeftJustify, B_RightJustify, B_CenterJustify };
};


#endif // __Blackbox_hh
