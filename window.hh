//
// window.hh for Blackbox - an X11 Window manager
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

#ifndef _blackbox_window_hh
#define _blackbox_window_hh

#include "menu.hh"
#include "workspace.hh"

class BlackboxIcon;
class BlackboxSession;
class BlackboxWindow;
class BlackboxWindowMenu;
class SendToWorkspaceMenu;


class BlackboxWindowMenu : public BlackboxMenu {
private:
  BlackboxWindow *window;
  BlackboxSession *session;
  SendToWorkspaceMenu *send_to_menu;

  friend BlackboxWindow;


protected:
  virtual void titlePressed(int);
  virtual void titleReleased(int);
  virtual void itemPressed(int, int);
  virtual void itemReleased(int, int);


public:
  BlackboxWindowMenu(BlackboxWindow *, BlackboxSession *);
  ~BlackboxWindowMenu(void);

  void showMenu(void);
  void hideMenu(void);
  void moveMenu(int, int);
  void Reconfigure(void);
};


class SendToWorkspaceMenu : public BlackboxMenu {
private:
  BlackboxWindow *window;
  WorkspaceManager *ws_manager;
  
  friend BlackboxWindow;


protected:
  virtual void titlePressed(int);
  virtual void titleReleased(int);
  virtual void itemPressed(int, int);
  virtual void itemReleased(int, int);


public:
  SendToWorkspaceMenu(BlackboxWindow *, BlackboxSession *);

  void updateMenu(void);
  void showMenu(void);
  void hideMenu(void);
  void moveMenu(int, int);
};


class BlackboxWindow {
private:
  BlackboxSession *session;
  BlackboxIcon *icon;
  BlackboxWindowMenu *window_menu;
  Display *display;

  struct client {
    Atom WMDelete, WMProtocols;
    BlackboxWindow *transient_for,  /* which window are we a transient for? */
      *transient;                   /* which window is our transient? */
    Window window, icon_window, window_group;

    char *title, *app_name, *app_class;
    int x, y, x_move, y_move;
    unsigned int width, height,
      base_w, base_h,
      inc_w, inc_h,
      min_w, min_h, max_w, max_h,
      min_ax, min_ay, max_ax, max_ay;
    long hint_flags;
  } client;

  struct frame {
    Bool shaped;
    GC ftextGC, utextGC;
    Pixmap utitle, ftitle, uhandle, fhandle, rhandle, button, pbutton;
    Window window, title, handle, close_button, iconify_button,
      maximize_button, resize_handle;
    int x, y;
    unsigned int width, height, title_h, title_w, handle_h, handle_w,
      button_w, button_h, x_resize, y_resize;
  } frame;

  struct {
    unsigned int
      WM_DELETE_WINDOW:1,
      WM_TAKE_FOCUS:1,
      WM_COLORMAP_WINDOWS:1;
  } Protocols;

  enum { F_NoInput = 0, F_Passive, F_LocallyActive, F_GloballyActive };
  int focus_mode, window_number, workspace_number;

  unsigned int
    do_handle:1,
    do_close:1,
    do_iconify:1,
    do_maximize:1,
    moving:1,
    resizing:1,
    shaded:1,
    maximized:1,

    visible:1,
    iconic:1,
    transient:1,
    focused:1,
    icccm_compliant:1,
    menu_visible:1,
    extra:2; /* keep data aligned better */


protected:
  Bool getWMNormalHints(XSizeHints *);
  Bool getWMProtocols(void);
  Bool getWMHints(void);
  Window createToplevelWindow(int, int, unsigned int, unsigned int,
			      unsigned int);
  Window createChildWindow(Window, int ,int, unsigned int, unsigned int,
			   unsigned int);

  void associateClientWindow(void);
  void createDecorations(void);
  void positionButtons(void);
  void createCloseButton(void);
  void createIconifyButton(void);
  void createMaximizeButton(void);
  void drawTitleWin(void);
  void drawAllButtons(void);
  void drawCloseButton(Bool);
  void drawIconifyButton(Bool);
  void drawMaximizeButton(Bool);
  void configureWindow(int, int, unsigned int, unsigned int);


public:
  BlackboxWindow(BlackboxSession *, Window, Bool = False);
  ~BlackboxWindow(void);

  void buttonPressEvent(XButtonEvent *);
  void buttonReleaseEvent(XButtonEvent *);
  void motionNotifyEvent(XMotionEvent *);
  void destroyNotifyEvent(XDestroyWindowEvent *);
  void mapRequestEvent(XMapRequestEvent *);
  void mapNotifyEvent(XMapEvent *);
  void unmapNotifyEvent(XUnmapEvent *);
  void propertyNotifyEvent(Atom);
  void exposeEvent(XExposeEvent *);
  void configureRequestEvent(XConfigureRequestEvent *);
#ifdef SHAPE
  void shapeEvent(XShapeEvent *);
#endif

  Bool setInputFocus(void);
  int setWindowNumber(int);
  int setWorkspace(int);
  void setFocusFlag(Bool);
  void iconifyWindow(void);
  void deiconifyWindow(void);
  void closeWindow(void);
  void withdrawWindow(void);
  void maximizeWindow(void);
  void shadeWindow(void);
  void Reconfigure(void);

  BlackboxWindow *Transient(void) { return client.transient; }
  BlackboxWindow *TransientFor(void) { return client.transient_for; }
  Bool isTransient(void) { return ((transient) ? True : False); }
  Bool hasTransient(void) { return ((client.transient) ? True : False); }
  Bool isFocused(void) { return focused; }
  Bool isVisible(void) { return visible; }
  Bool isIconic(void) { return iconic; }
  Bool resizable(void) { return (do_maximize|do_handle); }
  Window frameWindow(void) { return frame.window; }
  Window clientWindow(void) { return client.window; }
  char **Title(void) { return &client.title; }
  int XFrame(void) { return frame.x; }
  int YFrame(void) { return frame.y; }
  int workspace(void) { return workspace_number; }
  int windowNumber(void) { return window_number; }
  int setMenuVisible(Bool v) { return menu_visible = v; }
  unsigned int clientHeight(void) { return client.height; }
  unsigned int clientWidth(void) { return client.width; }
  void removeIcon(void) { icon = NULL; }
};


#endif
