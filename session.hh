//
// session.hh for Blackbox - an X11 Window manager
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

#ifndef _blackbox_session_hh
#define _blackbox_session_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#include <stdio.h>
#include "llist.hh"
#include "menu.hh"
#include "graphics.hh"

class BlackboxIcon;
class BlackboxSession;
class BlackboxWindow;
class SessionMenu;
class WorkspaceManager;


// root window menu... for starting user commands
class SessionMenu : public BlackboxMenu {
private:
  BlackboxSession *session;


protected:  
  virtual void itemPressed(int, int);
  virtual void itemReleased(int, int);
  virtual void titlePressed(int);
  virtual void titleReleased(int);
  void drawSubmenu(int);


public:
  SessionMenu(BlackboxSession *);
  virtual ~SessionMenu(void);
  
  Window windowID(void);
  int insert(char *, void (*)());
  int insert(char *, int, char * = 0);
  int insert(char *, SessionMenu *);
  int remove(int);
  void showMenu(void);
  void hideMenu(void);
  void moveMenu(int, int);
  void updateMenu(void);
};


// class for managing a single X session...
class BlackboxSession {
private:
  unsigned int
    b1Pressed:1,
    b2Pressed:1,
    b3Pressed:1,
    startup:1,
    extra:4;       // keep data aligned... but does gcc have #pragma packed?
  
  SessionMenu *rootmenu;
  WorkspaceManager *ws_manager;

  Atom _XA_WM_COLORMAP_WINDOWS, _XA_WM_PROTOCOLS, _XA_WM_STATE,
    _XA_WM_DELETE_WINDOW, _XA_WM_TAKE_FOCUS;

  Bool shutdown;
  Display *display;
  GC opGC;
  Visual *v;
  Window root;
  XColor *colors_8bpp;

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
    BlackboxMenu *data;
    Window window;
  } MenuSearch;

  typedef struct __ws_manger_search__ {
    WorkspaceManager *data;
    Window window;
  } WSManagerSearch;

  llist<WindowSearch> *window_search_list;
  llist<MenuSearch> *menu_search_list;
  llist<IconSearch> *icon_search_list;
  llist<WSManagerSearch> *wsmanager_search_list;

  struct cursor {
    Cursor session, move;
  } cursor;

  struct resource {
    struct color {
      BColor frame, toolbox, toolbox_to, focus, focus_to, unfocus, unfocus_to,
	menu, menu_to, imenu, imenu_to, button, button_to, ftext, utext, mtext,
	mitext, ptext, itext, ttext;
    } color;
    
    struct font {
      XFontStruct *title, *menu, *icon;
    } font;
    
    struct texture {
      int toolbox, window, button, menu, imenu, pimenu;
    } texture;

    Bool prompt_reconfigure;
    char *menuFile;
    int workspaces, orientation;
  } resource;

  struct shape {
    Bool extensions;
    int event_basep, error_basep;
  } shape;

  int depth, screen, event_mask, focus_window_number;
  unsigned int xres, yres;

  friend SessionMenu;


protected:
  void InitScreen(void);
  void InitMenu(void);
  void parseSubMenu(FILE *, SessionMenu *);
  void InitColor(void);
  void ProcessEvent(XEvent *);
  void Reconfigure(void);
  void createAutoConfigDialog(void);


public:
  BlackboxSession(char *);
  ~BlackboxSession();
  
  BlackboxMenu *searchMenu(Window);
  BlackboxWindow *searchWindow(Window);
  BlackboxIcon *searchIcon(Window);
  WorkspaceManager *searchWSManager(Window);
  unsigned long getColor(const char *);
  unsigned long getColor(const char *, unsigned char *, unsigned char *,
			 unsigned char *);
  void addWindow(BlackboxWindow *);
  void removeWindow(BlackboxWindow *);
  void reassociateWindow(BlackboxWindow *);
  void raiseWindow(BlackboxWindow *);
  void lowerWindow(BlackboxWindow *);
  void updateWorkspace(int);
  void EventLoop(void);
  void Restart(void);
  void Exit(void);
  void Shutdown(void);
  void LoadDefaults(void);
  void Dissociate(void);
  void saveMenuSearch(Window, BlackboxMenu *);
  void saveWindowSearch(Window, BlackboxWindow *);
  void saveIconSearch(Window, BlackboxIcon *);
  void saveWSManagerSearch(Window, WorkspaceManager *);
  void removeMenuSearch(Window);
  void removeWindowSearch(Window);
  void removeIconSearch(Window);
  void removeWSManagerSearch(Window);

  // various informative functions about the current X session
  Atom StateAtom(void) { return _XA_WM_STATE; }
  Atom DeleteAtom(void) { return _XA_WM_DELETE_WINDOW; }
  Atom ProtocolsAtom(void) { return _XA_WM_PROTOCOLS; }
  Atom FocusAtom(void) { return _XA_WM_TAKE_FOCUS; }
  Atom ColormapAtom(void) { return _XA_WM_COLORMAP_WINDOWS; }
  Bool button1Pressed() const { return b1Pressed; }
  Bool button2Pressed() const { return b2Pressed; }
  Bool button3Pressed() const { return b3Pressed; }
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

  // pointers to the sessions controlling members
  SessionMenu *menu(void) { return rootmenu; }
  WorkspaceManager *WSManager(void) { return ws_manager; }

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
  const BColor &menuItemTextColor(void) const { return resource.color.mitext; }
  const BColor &menuPressedTextColor(void)
    const { return resource.color.ptext; }
  const BColor &iconTextColor(void) const { return resource.color.itext; }
  const BColor &toolboxTextColor(void) const { return resource.color.ttext; }

  // controls arrangement of decorations for left and right handed users
  const int Orientation(void) const { return resource.orientation; }
  
  // constants for internal workings of blackbox
  enum { B_Restart = 1, B_RestartOther, B_Exit, B_Shutdown, B_Execute,
	 B_Reconfigure,
	 B_WindowShade, B_WindowClose, B_WindowMaximize, B_WindowIconify,
	 B_WindowRaise, B_WindowLower, B_WindowSendToWorkspace };
  enum { B_TextureRSolid = 1, B_TextureSSolid, B_TextureFSolid,
	 B_TextureRHGradient, B_TextureSHGradient, B_TextureFHGradient,
	 B_TextureRVGradient, B_TextureSVGradient, B_TextureFVGradient,
	 B_TextureRDGradient, B_TextureSDGradient, B_TextureFDGradient };
  enum { B_LeftHandedUser = 1, B_RightHandedUser };
};


#endif
