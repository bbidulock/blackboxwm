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
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/cursorfont.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

#include "debug.hh"
#include "llist.hh"
#include "menu.hh"
#include "graphics.hh"

class BlackboxIcon;
class BlackboxSession;
class BlackboxWindow;
class SessionMenu;
class WorkspaceManager;


class SessionMenu : public BlackboxMenu {
protected:  
  virtual void itemPressed(int, int);
  virtual void itemReleased(int, int);
  virtual void titlePressed(int);
  virtual void titleReleased(int);


public:
  SessionMenu(BlackboxSession *);
  
  void showMenu(void);
  void moveMenu(int, int);
  void updateMenu(void);
  void raiseMenu(void);
  Window windowID(void);

  int insert(char *, void (*)());
  int insert(char *, int, char * = 0);
  int insert(char *, BlackboxMenu *);
};


class BlackboxSession {
private:
  int event_mask;
  unsigned int
    b1Pressed:1,
    b2Pressed:1,
    b3Pressed:1,
    startup:1,
    extra:4;       // keep data aligned... but does gcc have #pragma packed?
  
  SessionMenu *rootmenu;
  Debugger *debug;
  WorkspaceManager *ws_manager;
  
  //
  // X Atoms for ICCCM compliance
  //
  Atom _XA_WM_COLORMAP_WINDOWS, _XA_WM_PROTOCOLS, _XA_WM_STATE,
    _XA_WM_DELETE_WINDOW, _XA_WM_TAKE_FOCUS;

  Bool shutdown, shape_extensions;
  Display *display;
  GC opGC;
  Visual *v;
  Window root;
  XColor *colors_8bpp;

  struct context {
    XContext workspace,
      window,
      icon,
      menu;
  } context;

  struct cursor {
    Cursor session,
      move;
  } cursor;


  struct resource {
    struct color {
      BColor frame,
	frame_to,
	focus,
	focus_to,
	unfocus,
	unfocus_to,
	menu,
	menu_to,
	imenu,
	imenu_to,
	button,
	button_to,
	icon,
	ftext,
	utext;
    } color;
    
    struct font {
      XFontStruct *title,
	*menu,
	*imenu,
	*icon;
    } font;
    
    struct texture {
      int frame,
	button,
	menu,
	imenu,
	pimenu;
    } texture;

    char *menuFile;
    int workspaces;
  } resource;

  int depth, screen;
  uint xres, yres;


protected:
  int anotherWMRunning(Display *, XErrorEvent *);
  int errorHandler(Display *, XErrorEvent *);
  void sig11handler(int);

  void Dissociate(void);
  void InitScreen(void);
  void InitMenu(void);
  void InitColor(void);
  void ProcessEvent(XEvent *);


public:
  BlackboxSession(char *);
  ~BlackboxSession();
  
  Display *control(void) { return display; }
  Window Root(void) { return root; }
  int Depth(void) { return depth; }
  Visual *visual(void) { return v; }
  unsigned int XResolution(void) { return xres; }
  unsigned int YResolution(void) { return yres; }
  Bool shapeExtensions(void) { return shape_extensions; }

  WorkspaceManager *WSManager(void) { return ws_manager; }
  void addIcon(BlackboxIcon *);
  void removeIcon(BlackboxIcon *i);
  void addWindow(BlackboxWindow *);
  void removeWindow(BlackboxWindow *);
  void reassociateWindow(BlackboxWindow *);
  void updateWorkspace(int);
  int iconCount(void);

  XContext wsContext(void) { return context.workspace; }
  XContext iconContext(void) { return context.icon; }
  XContext winContext(void) { return context.window; }
  XContext menuContext(void) { return context.menu; }

  Cursor sessionCursor(void) { return cursor.session; }
  Cursor moveCursor(void) { return cursor.move; }

  XFontStruct *titleFont(void) { return resource.font.title; }
  XFontStruct *menuFont(void) { return resource.font.menu; }

  unsigned long getColor(const char *);
  unsigned long getColor(const char *, unsigned char *, unsigned char *,
			 unsigned char *);
  BlackboxWindow *getWindow(Window);
  BlackboxIcon *getIcon(Window);
  BlackboxMenu *getMenu(Window);
  WorkspaceManager *getWSManager(Window);

  int frameTexture(void) { return resource.texture.frame; }
  int buttonTexture(void) { return resource.texture.button; }
  int menuTexture(void) { return resource.texture.menu; }
  int menuItemTexture(void) { return resource.texture.imenu; }
  int menuItemPressedTexture(void) { return resource.texture.pimenu; }

  const BColor &frameColor(void) const { return resource.color.frame; }
  const BColor &frameToColor(void) const { return resource.color.frame_to; }
  const BColor &focusColor(void) const { return resource.color.focus; }
  const BColor &focusToColor(void) const { return resource.color.focus_to; }
  const BColor &unfocusColor(void) const { return resource.color.unfocus; }
  const BColor &unfocusToColor(void) const
    { return resource.color.unfocus_to; }
  const BColor &buttonColor(void) const { return resource.color.button; }
  const BColor &buttonToColor(void) const { return resource.color.button_to; }
  const BColor &iconColor(void) const { return resource.color.icon; }
  const BColor &menuColor(void) const { return resource.color.menu; }
  const BColor &menuToColor(void) const { return resource.color.menu_to; }
  const BColor &menuItemColor(void) const { return resource.color.imenu; }
  const BColor &menuItemToColor(void) const
    { return resource.color.imenu_to; }
  const BColor &focusTextColor(void) const { return resource.color.ftext; }
  const BColor &unfocusTextColor(void) const { return resource.color.utext; }
  XColor *Colors8bpp(void) { return colors_8bpp; }

  void arrangeIcons(void);
  void EventLoop(void);
  void Restart(void);
  void Exit(void);
  void Shutdown(void);
  Bool Startup(void) { return startup; }
  void LoadDefaults(void);

  Bool button1Pressed() const { return b1Pressed; }
  Bool button2Pressed() const { return b2Pressed; }
  Bool button3Pressed() const { return b3Pressed; }

  Atom StateAtom(void) { return _XA_WM_STATE; }
  Atom DeleteAtom(void) { return _XA_WM_DELETE_WINDOW; }
  Atom ProtocolsAtom(void) { return _XA_WM_PROTOCOLS; }
  Atom FocusAtom(void) { return _XA_WM_TAKE_FOCUS; }
  Atom ColormapAtom(void) { return _XA_WM_COLORMAP_WINDOWS; }

  GC GCOperations(void) { return opGC; }
  
  enum { B_Restart = 1, B_Exit, B_Shutdown, B_Execute , B_WindowClose,
	 B_WindowIconify, B_WindowRaise, B_WindowLower };
  enum { B_TextureRSolid = 1, B_TextureSSolid, B_TextureFSolid,
	 B_TextureRHGradient, B_TextureSHGradient, B_TextureFHGradient,
	 B_TextureRVGradient, B_TextureSVGradient, B_TextureFVGradient,
	 B_TextureRDGradient, B_TextureSDGradient, B_TextureFDGradient };
};


#endif
