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

#ifndef __blackbox_hh
#define __blackbox_hh
#define __blackbox_version "beta zero . four zero . five"

#include <X11/Xlib.h>
#include <X11/Xresource.h>

// forward declarations
class Blackbox;

class Basemenu;
class Rootmenu;
class Toolbar;
class BlackboxWindow;

#include "Image.hh"
#include "LinkedList.hh"

#include <stdio.h>


typedef struct windowResource {
  struct decoration {
    BColor fcolor, fcolorTo, ucolor, ucolorTo, ftextColor, utextColor;
    unsigned long ftexture, utexture;
  } decoration;
  
  struct button {
    BColor fcolor, fcolorTo, ucolor, ucolorTo, pressed, pressedTo;
    unsigned long ftexture, utexture, ptexture;
  } button;

  struct frame {
    BColor color;
    unsigned long texture;
  } frame;
} windowResource;


typedef struct toolbarResource {
  struct toolbar {
    BColor color, colorTo, textColor;
    unsigned long texture;
  } toolbar;
  
  struct label {
    BColor color, colorTo;
    unsigned long texture;
  } label;
  
  struct button {
    BColor color, colorTo, pressed, pressedTo;
    unsigned long texture, ptexture;
  } button;
  
  struct clock {
    BColor color, colorTo;
    unsigned long texture;
  } clock;
} toolbarResource;


typedef struct menuResource {
  struct title {
    BColor color, colorTo, textColor;
    unsigned long texture;
  } title;

  struct frame {
    BColor color, colorTo, hcolor, textColor, htextColor;
    unsigned long texture;
  } frame;
} menuResource;


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

  typedef struct ToolbarSearch {
    Toolbar *data;
    Window window;
  } ToolbarSearch;

  struct cursor {
    Cursor session, move;
  } cursor;

  struct resource {
    struct font {
      XFontStruct *title, *menu;
    } font;

    windowResource wres;
    toolbarResource tres;
    menuResource mres;

    Bool opaqueMove, imageDither, clock24hour, toolbarRaised, sloppyFocus,
      autoRaise;
    BColor borderColor;
    XrmDatabase stylerc;
    char *menuFile, *styleFile;
    int workspaces, justify, menu_justify, colors_per_channel;
    unsigned int handleWidth, bevelWidth;
  } resource;

  struct shape {
    Bool extensions;
    int event_basep, error_basep;
  } shape;

  // context linked lists
  LinkedList<WindowSearch> *windowSearchList;
  LinkedList<MenuSearch> *menuSearchList;
  LinkedList<ToolbarSearch> *toolbarSearchList;
  LinkedList<GroupSearch> *groupSearchList;
  
  // internal variables for operation
  Rootmenu *root_menu;
  Toolbar *tool_bar;

  BImageControl *image_control;

  Atom _XA_WM_COLORMAP_WINDOWS, _XA_WM_PROTOCOLS, _XA_WM_STATE,
    _XA_WM_DELETE_WINDOW, _XA_WM_TAKE_FOCUS, _MOTIF_WM_HINTS;
  Bool startup, shutdown, reconfigure;
  Display *display;
  GC opGC, wfocusGC, wunfocusGC, mtitleGC, mframeGC, mhiGC, mhbgGC;
  Visual *v;
  Window root;
  char *display_name;
  char **b_argv;
  int depth, screen, event_mask, focus_window_number, b_argc,
    red_offset, green_offset, blue_offset;
  unsigned int xres, yres;


protected:
  // internal initialization
  void InitMenu(void);
  void InitColor(void);
  void LoadRC(void);
  void LoadStyle(void);
  void SaveRC(void);

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
  Rootmenu *Menu(void) { return root_menu; }
  Toolbar *toolbar(void) { return tool_bar; }
  BImageControl *imageControl(void) { return image_control; }

  // window validation
  Bool validateWindow(Window);

  // context lookup routines
  Basemenu *searchMenu(Window);
  BlackboxWindow *searchWindow(Window);
  Toolbar *searchToolbar(Window);
  BlackboxWindow *searchGroup(Window, BlackboxWindow *);

  // reassociated a window with the current workspace
  void reassociateWindow(BlackboxWindow *);

  // context list save/remove methods
  void saveMenuSearch(Window, Basemenu *);
  void saveWindowSearch(Window, BlackboxWindow *);
  void saveToolbarSearch(Window, Toolbar *);
  void saveGroupSearch(Window, BlackboxWindow *);
  void removeMenuSearch(Window);
  void removeWindowSearch(Window);
  void removeToolbarSearch(Window);
  void removeGroupSearch(Window);

  // main event loop
  void EventLoop(void);

  // window manager functions
  void setStyle(char *);
  void prevFocus(void);
  void nextFocus(void);
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
  Atom MwmHintsAtom(void) { return _MOTIF_WM_HINTS; }

  Bool shapeExtensions(void) { return shape.extensions; }
  Bool Startup(void) { return startup; }

  Cursor sessionCursor(void) { return cursor.session; }
  Cursor moveCursor(void) { return cursor.move; }

  Display *control(void) { return display; }

  GC OpGC(void) { return opGC; }
  GC WindowFocusGC(void) { return wfocusGC; }
  GC WindowUnfocusGC(void) { return wunfocusGC; }
  GC MenuTitleGC(void) { return mtitleGC; }
  GC MenuFrameGC(void) { return mframeGC; }
  GC MenuHiGC(void) { return mhiGC; }
  GC MenuHiBGGC(void) { return mhbgGC; }

  Visual *visual(void) { return v; }

  Window Root(void) { return root; }

  // session controls
  XFontStruct *titleFont(void) { return resource.font.title; }
  XFontStruct *menuFont(void) { return resource.font.menu; }
  const BColor &borderColor(void) { return resource.borderColor; }
  const unsigned int handleWidth(void) { return resource.handleWidth; }
  const unsigned int bevelWidth(void) { return resource.bevelWidth; }
  Bool clock24Hour(void) { return resource.clock24hour; }
  Bool toolbarRaised(void) { return resource.toolbarRaised; }
  Bool sloppyFocus(void) { return resource.sloppyFocus; }
  Bool imageDither(void) { return resource.imageDither; }
  Bool autoRaise(void) { return resource.autoRaise; }
  Bool opaqueMove(void) { return resource.opaqueMove; }

  // controls for arrangement of decorations
  const int Justification(void) const { return resource.justify; }
  const int MenuJustification(void) const { return resource.menu_justify; }
  
  // resources
  windowResource *wResource(void) { return &resource.wres; }
  menuResource *mResource(void) { return &resource.mres; }
  toolbarResource *tResource(void) { return &resource.tres; }

  // session information
  int Depth(void) { return depth; }
  int Screen(void) { return screen; }
  int colorsPerChannel(void) { return resource.colors_per_channel; }
  unsigned int xRes(void) { return xres; }
  unsigned int yRes(void) { return yres; }

  // public constants
  enum { B_Restart = 1, B_RestartOther, B_Exit, B_Shutdown, B_Execute,
	 B_Reconfigure, B_ExecReconfigure, B_WindowShade, B_WindowIconify,
	 B_WindowMaximize, B_WindowClose, B_WindowRaise, B_WindowLower,
	 B_WindowStick, B_WindowKill, B_SetStyle };
  enum { B_LeftJustify = 1, B_RightJustify, B_CenterJustify };
};


#endif // __blackbox_hh
