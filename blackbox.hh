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
#define __blackbox_version "zero point two two point zero beta"

#include <X11/Xlib.h>
#include <X11/Xresource.h>

// forward declaration
class Blackbox;

#include "graphics.hh"
#include "icon.hh"
#include "llist.hh"
#include "menu.hh"
#include "window.hh"
#include "workspace.hh"


// *************************************************************************
// SessionMenu - root window menu for starting applications and accessing
//               various window manager functions
// *************************************************************************

class SessionMenu : public BaseMenu {
private:
  Blackbox *blackbox;
  Bool default_menu;


protected:  
  virtual void itemPressed(int, int);
  virtual void itemReleased(int, int);
  virtual void titlePressed(int);
  virtual void titleReleased(int);


public:
  SessionMenu(Blackbox *);
  virtual ~SessionMenu(void);
  
  void defaultMenu(void) { default_menu = True; }
  int remove(int);
};


class Blackbox {
private:
  // internal structures
  struct __reconfigure__dialog__ {
    BlackboxWindow *dialog;
    Bool visible;
    GC dialogGC;
    Window window, text_window, yes_button, no_button;
    char *DialogText[6];
    int text_w, text_h, line_h;
  } ReconfigureDialog;

  typedef struct __window_search__ {
    BlackboxWindow *data;
    Window window;
  } WindowSearch;

  typedef struct __icon_search__ {
    BlackboxIcon *data;
    Window window;
  } IconSearch;

  typedef struct __menu_search__ {
    BaseMenu *data;
    Window window;
  } MenuSearch;

  typedef struct __ws_manger_search__ {
    WorkspaceManager *data;
    Window window;
  } WSManagerSearch;

  struct cursor {
    Cursor session, move;
  } cursor;

  struct resource {
    struct color {
      BColor frame, toolbox, toolbox_to, focus, focus_to, unfocus, unfocus_to,
	menu, menu_to, imenu, imenu_to, button, button_to, ftext, utext,
	mtext, mitext, ptext, itext, ttext;
    } color;
    
    struct font {
      XFontStruct *title, *menu, *icon;
    } font;
    
    struct texture {
      int toolbox, window, button, menu, imenu, pimenu;
    } texture;

    Bool prompt_reconfigure;
    char *menuFile;
    int workspaces, orientation, justification;
  } resource;

  struct shape {
    Bool extensions;
    int event_basep, error_basep;
  } shape;

  // context linked lists
  llist<WindowSearch> *window_search_list;
  llist<MenuSearch> *menu_search_list;
  llist<IconSearch> *icon_search_list;
  llist<WSManagerSearch> *wsmanager_search_list;

  // internal variables for operation
  SessionMenu *rootmenu;
  WorkspaceManager *ws_manager;

  Atom _XA_WM_COLORMAP_WINDOWS, _XA_WM_PROTOCOLS, _XA_WM_STATE,
    _XA_WM_DELETE_WINDOW, _XA_WM_TAKE_FOCUS;
  Bool startup, shutdown, reconfigure;
  Display *display;
  GC opGC;
  Visual *v;
  Window root;
  XColor *colors_8bpp;
  char *display_name;
  char **b_argv;
  int depth, screen, event_mask, focus_window_number, b_argc;
  unsigned int xres, yres;


protected:
  // internal initialization
  void InitMenu(void);
  void InitColor(void);
  void LoadDefaults(void);
  void createAutoConfigDialog(void);

  // event processing and dispatching
  void ProcessEvent(XEvent *);

  // internal routines
  void Dissociate(void);
  void do_reconfigure(void);

  // value lookups and retrieval
  void readMenuDatabase(SessionMenu *, XrmValue *, XrmDatabase *);
  unsigned long getColor(const char *);
  unsigned long getColor(const char *, unsigned char *, unsigned char *,
			 unsigned char *);


public:
  Blackbox(int, char **, char * = 0);
  ~Blackbox(void);

  // member pointers
  SessionMenu *Rootmenu(void) { return rootmenu; }
  WorkspaceManager *WSManager(void) { return ws_manager; }

  // context lookup routines
  BaseMenu *searchMenu(Window);
  BlackboxWindow *searchWindow(Window);
  BlackboxIcon *searchIcon(Window);
  WorkspaceManager *searchWSManager(Window);

  // window context operations
  void addWindow(BlackboxWindow *);
  void removeWindow(BlackboxWindow *);
  void reassociateWindow(BlackboxWindow *);
  void raiseWindow(BlackboxWindow *);
  void lowerWindow(BlackboxWindow *);

  // context list save/remove methods
  void saveMenuSearch(Window, BaseMenu *);
  void saveWindowSearch(Window, BlackboxWindow *);
  void saveIconSearch(Window, BlackboxIcon *);
  void saveWSManagerSearch(Window, WorkspaceManager *);
  void removeMenuSearch(Window);
  void removeWindowSearch(Window);
  void removeIconSearch(Window);
  void removeWSManagerSearch(Window);

  // update workspace to reflect changes in window names
  void updateWorkspace(int);

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

  XColor *Colors8bpp(void) { return colors_8bpp; }

  XFontStruct *titleFont(void) { return resource.font.title; }
  XFontStruct *menuFont(void) { return resource.font.menu; }
  XFontStruct *iconFont(void) { return resource.font.icon; }

  int Depth(void) { return depth; }

  unsigned int XResolution(void) { return xres; }
  unsigned int YResolution(void) { return yres; }

  // textures and colors for configuration
  int toolboxTexture(void) { return resource.texture.toolbox; }
  int windowTexture(void) { return resource.texture.window; }
  int buttonTexture(void) { return resource.texture.button; }
  int menuTexture(void) { return resource.texture.menu; }
  int menuItemTexture(void) { return resource.texture.imenu; }
  int menuItemPressedTexture(void) { return resource.texture.pimenu; }

  const BColor &frameColor(void) const { return resource.color.frame; }
  const BColor &toolboxColor(void) const { return resource.color.toolbox; }
  const BColor &toolboxToColor(void) const
    { return resource.color.toolbox_to; }
  const BColor &focusColor(void) const { return resource.color.focus; }
  const BColor &focusToColor(void) const { return resource.color.focus_to; }
  const BColor &unfocusColor(void) const { return resource.color.unfocus; }
  const BColor &unfocusToColor(void) const
    { return resource.color.unfocus_to; }
  const BColor &buttonColor(void) const { return resource.color.button; }
  const BColor &buttonToColor(void) const { return resource.color.button_to; }
  const BColor &menuColor(void) const { return resource.color.menu; }
  const BColor &menuToColor(void) const { return resource.color.menu_to; }
  const BColor &menuItemColor(void) const { return resource.color.imenu; }
  const BColor &menuItemToColor(void) const
    { return resource.color.imenu_to; }
  const BColor &focusTextColor(void) const { return resource.color.ftext; }
  const BColor &unfocusTextColor(void) const { return resource.color.utext; }
  const BColor &menuTextColor(void) const { return resource.color.mtext; }
  const BColor &menuItemTextColor(void) const
    { return resource.color.mitext; }
  const BColor &menuPressedTextColor(void) const
    { return resource.color.ptext; }
  const BColor &iconTextColor(void) const { return resource.color.itext; }
  const BColor &toolboxTextColor(void) const { return resource.color.ttext; }

  // controls for arrangement of decorations
  const int Orientation(void) const { return resource.orientation; }
  const int Justification(void) const { return resource.justification; }
  
  // public constants
  enum { B_Restart = 1, B_RestartOther, B_Exit, B_Shutdown, B_Execute,
	 B_Reconfigure,
	 B_WindowShade, B_WindowClose, B_WindowMaximize, B_WindowIconify,
	 B_WindowRaise, B_WindowLower, B_WindowSendToWorkspace };
  enum { B_TextureRSolid = 1, B_TextureSSolid, B_TextureFSolid,
	 B_TextureRHGradient, B_TextureSHGradient, B_TextureFHGradient,
	 B_TextureRVGradient, B_TextureSVGradient, B_TextureFVGradient,
	 B_TextureRDGradient, B_TextureSDGradient, B_TextureFDGradient };
  enum { B_LeftHandedUser = 1, B_RightHandedUser,
	 B_LeftJustify, B_RightJustify, B_CenterJustify };
};


#endif
