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
    Bool image_dither;
    Time double_click_interval;

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

  BlackboxWindow *focused_window;

  Atom _XA_WM_COLORMAP_WINDOWS, _XA_WM_PROTOCOLS, _XA_WM_STATE,
    _XA_WM_DELETE_WINDOW, _XA_WM_TAKE_FOCUS, _XA_WM_CHANGE_STATE,
    _MOTIF_WM_HINTS;
  Bool startup, shutdown, reconfigure;
  Display *display;
  char *display_name;
  char **argv;
  int argc, number_of_screens, server_grabs;


protected:
  void LoadRC(void);
  void SaveRC(void);

  void ProcessEvent(XEvent *);

  void do_reconfigure(void);


public:
  Blackbox(int, char **, char * = 0);
  ~Blackbox(void);

  Atom getChangeStateAtom(void) { return _XA_WM_CHANGE_STATE; }
  Atom getStateAtom(void)       { return _XA_WM_STATE; }
  Atom getDeleteAtom(void)      { return _XA_WM_DELETE_WINDOW; }
  Atom getProtocolsAtom(void)   { return _XA_WM_PROTOCOLS; }
  Atom getFocusAtom(void)       { return _XA_WM_TAKE_FOCUS; }
  Atom getColormapAtom(void)    { return _XA_WM_COLORMAP_WINDOWS; }
  Atom getMwmHintsAtom(void)    { return _MOTIF_WM_HINTS; }

  Basemenu *searchMenu(Window);

  BlackboxWindow *searchGroup(Window, BlackboxWindow *);
  BlackboxWindow *searchWindow(Window);
  BlackboxWindow *getFocusedWindow(void) { return focused_window; }

  Bool hasShapeExtensions(void) { return shape.extensions; }
  Bool hasImageDither(void)     { return resource.image_dither; }
  Bool isStartup(void)          { return startup; }
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

  void LoadRC(BScreen *);
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
  void EventLoop(void);
  void Exit(void);
  void Restart(char * = 0);
  void Reconfigure(void);
  void Shutdown(void);

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
