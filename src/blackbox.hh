//
// blackbox.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@tcac.net
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

#include "../version.h"

#include <X11/Xlib.h>
#include <X11/Xresource.h>

class Basemenu;
class BlackboxWindow;
class BlackboxIcon;
class Iconmenu;
class Rootmenu;
class Toolbar;
class Workspace;
class Workspacemenu;

#include "Image.hh"
#include "LinkedList.hh"

#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif


class Blackbox {
private:
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
    Bool image_dither, colormap_focus_follows_mouse;
    Time double_click_interval, auto_raise_delay_sec, auto_raise_delay_usec;

    char *menu_file, *style_file;
    int colors_per_channel;
  } resource;

  struct shape {
    Bool extensions;
    int event_basep, error_basep;
  } shape;

  LinkedList<WindowSearch> *windowSearchList;
  LinkedList<MenuSearch> *menuSearchList;
  LinkedList<ToolbarSearch> *toolbarSearchList;
  LinkedList<GroupSearch> *groupSearchList;
  LinkedList<BScreen> *screenList;

  BlackboxWindow *focused_window, *auto_raise_window;

  Atom xa_wm_colormap_windows, xa_wm_protocols, xa_wm_state,
    xa_wm_delete_window, xa_wm_take_focus, xa_wm_change_state,
    motif_wm_hints;

#ifdef    KDE
  Atom kwm_current_desktop, kwm_number_of_desktops, kwm_active_window,
    kwm_win_iconified, kwm_win_sticky, kwm_win_maximized, kwm_win_decoration,
    kwm_win_icon, kwm_win_desktop, kwm_win_frame_geometry, kwm_command,
    kwm_do_not_manage, kwm_activate_window, kwm_running,
    kwm_module, kwm_module_init, kwm_module_initialized,
    kwm_module_desktop_change, kwm_module_win_change, kwm_window_region_1,
    kwm_module_desktop_number_change, kwm_module_desktop_name_change,
    kwm_module_win_add, kwm_module_win_remove;
#endif // KDE

  Bool _startup, _shutdown, _reconfigure, auto_raise_pending;
  Display *display;
  char *display_name;
  char **argv;
  int argc, number_of_screens, server_grabs;


protected:
  void load_rc(void);
  void save_rc(void);

  void process_event(XEvent *);

  void do_reconfigure(void);


public:
  Blackbox(int, char **, char * = 0);
  ~Blackbox(void);

  Atom getWMChangeStateAtom(void) { return xa_wm_change_state; }
  Atom getWMStateAtom(void)       { return xa_wm_state; }
  Atom getWMDeleteAtom(void)      { return xa_wm_delete_window; }
  Atom getWMProtocolsAtom(void)   { return xa_wm_protocols; }
  Atom getWMFocusAtom(void)       { return xa_wm_take_focus; }
  Atom getWMColormapAtom(void)    { return xa_wm_colormap_windows; }

  Atom getMotifWMHintsAtom(void) { return motif_wm_hints; }

#ifdef    KDE

  Atom getKWMRunningAtom(void)             { return kwm_running; }
  Atom getKWMModuleAtom(void)              { return kwm_module; }
  Atom getKWMModuleInitAtom(void)          { return kwm_module_init; }
  Atom getKWMModuleInitializedAtom(void)   { return kwm_module_initialized; }
  Atom getKWMCurrentDesktopAtom(void)      { return kwm_current_desktop; }
  Atom getKWMNumberOfDesktopsAtom(void)    { return kwm_number_of_desktops; }

  Atom getKWMWinStickyAtom(void)           { return kwm_win_sticky; }
  Atom getKWMWinIconifiedAtom(void)        { return kwm_win_iconified; }
  Atom getKWMWinMaximizedAtom(void)        { return kwm_win_maximized; }
  Atom getKWMWinDesktopAtom(void)          { return kwm_win_desktop; }
  Atom getKWMWindowRegion1Atom(void)       { return kwm_window_region_1; }

  Atom getKWMModuleDesktopChangeAtom(void)
    { return kwm_module_desktop_change; }
  Atom getKWMModuleWinChangeAtom(void)     { return kwm_module_win_change; }
  Atom getKWMModuleDesktopNameChangeAtom(void)
    { return kwm_module_desktop_name_change; }
  Atom getKWMModuleWinAddAtom(void)        { return kwm_module_win_add; }
  Atom getKWMModuleWinRemoveAtom(void)     { return kwm_module_win_remove; }
  Atom getKWMModuleDesktopNumberChangeAtom(void)
    { return kwm_module_desktop_number_change; }

#endif // KDE

  Basemenu *searchMenu(Window);

  BlackboxWindow *searchGroup(Window, BlackboxWindow *);
  BlackboxWindow *searchWindow(Window);
  BlackboxWindow *getFocusedWindow(void) { return focused_window; }

  Bool hasShapeExtensions(void) { return shape.extensions; }
  Bool hasImageDither(void)     { return resource.image_dither; }
  Bool isStartup(void)          { return _startup; }
  Bool validateWindow(Window);

  Cursor getSessionCursor(void) { return cursor.session; }
  Cursor getMoveCursor(void)    { return cursor.move; }

  Display *getDisplay(void) { return display; }

  Time getDoubleClickInterval(void) { return resource.double_click_interval; }

  Toolbar *searchToolbar(Window);

  BScreen *getScreen(int);
  BScreen *searchScreen(Window);

  char *getStyleFilename(void) { return resource.style_file; }
  char *getMenuFilename(void)  { return resource.menu_file; }

  int getColorsPerChannel(void) { return resource.colors_per_channel; }
  int getNumberOfScreens(void)  { return number_of_screens; }

  void load_rc(BScreen *);
  void saveStyleFilename(char *);
  void grab(void);
  void ungrab(void);
  void saveMenuSearch(Window, Basemenu *);
  void saveWindowSearch(Window, BlackboxWindow *);
  void saveToolbarSearch(Window, Toolbar *);
  void saveGroupSearch(Window, BlackboxWindow *);
  void removeMenuSearch(Window);
  void removeWindowSearch(Window);
  void removeToolbarSearch(Window);
  void removeGroupSearch(Window);
  void eventLoop(void);
  void exit(void);
  void restart(char * = 0);
  void reconfigure(void);
  void shutdown(void);

  enum { B_Restart = 1, B_RestartOther, B_Exit, B_Shutdown, B_Execute,
	 B_Reconfigure, B_ExecReconfigure, B_WindowShade, B_WindowIconify,
	 B_WindowMaximize, B_WindowClose, B_WindowRaise, B_WindowLower,
	 B_WindowStick, B_WindowKill, B_SetStyle };
  enum { B_LeftJustify = 1, B_RightJustify, B_CenterJustify };

#ifndef HAVE_STRFTIME
  enum { B_AmericanDate = 1, B_EuropeanDate };
#endif
};


#endif // __blackbox_hh
