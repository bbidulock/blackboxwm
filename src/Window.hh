// Window.hh for Blackbox - an X11 Window manager
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

#ifndef   __Window_hh
#define   __Window_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef    SHAPE
#  include <X11/extensions/shape.h>
#endif // SHAPE

#define MwmHintsFunctions     (1l << 0)
#define MwmHintsDecorations   (1l << 1)

#define MwmFuncAll            (1l << 0)
#define MwmFuncResize         (1l << 1)
#define MwmFuncMove           (1l << 2)
#define MwmFuncIconify        (1l << 3)
#define MwmFuncMaximize       (1l << 4)
#define MwmFuncClose          (1l << 5)

#define MwmDecorAll           (1l << 0)
#define MwmDecorBorder        (1l << 1)
#define MwmDecorHandle        (1l << 2)
#define MwmDecorTitle         (1l << 3)
#define MwmDecorMenu          (1l << 4)
#define MwmDecorIconify       (1l << 5)
#define MwmDecorMaximize      (1l << 6)

// this structure only contains 3 elements... the Motif 2.0 structure contains
// 5... we only need the first 3... so that is all we will define
typedef struct MwmHints {
  unsigned long flags, functions, decorations;
} MwmHints;

#define PropMwmHintsElements  3

// forward declaration
class BlackboxWindow;

class Blackbox;
class BlackboxIcon;
class Windowmenu;
class BImageControl;
class BScreen;


class BlackboxWindow {
private:
  Blackbox *blackbox;
  BlackboxIcon *icon;
  BScreen *screen;
  Windowmenu *windowmenu;
  
  BImageControl *image_ctrl;
  
  Bool moving, resizing, shaded, maximized, visible, iconic, transient,
    focused, stuck, focusable;
  Display *display;
  Time lastButtonPressTime;
  
  struct client {
    BlackboxWindow *transient_for,  // which window are we a transient for?
      *transient;                   // which window is our transient?
    Bool pre_icccm;		    // hack for pre-icccm clients (xv)
    Window window, window_group;
    
    char *title;
    int x, y, old_bw, title_len, gravx_offset, gravy_offset;
    unsigned int width, height, title_text_w,
      min_width, min_height, max_width, max_height, width_inc, height_inc,
      min_aspect_x, min_aspect_y, max_aspect_x, max_aspect_y,
      base_width, base_height, win_gravity;
    unsigned long initial_state, normal_hint_flags, wm_hint_flags;
    
    MwmHints *mwm_hint;
  } client;
  
  struct decorations {
    Bool titlebar, handle, border, iconify, maximize, close, menu;
  } decorations;
  
  struct functions {
    Bool resize, move, iconify, maximize, close;
  } functions;
  
  struct frame {
    Bool shaped;
    Pixmap utitle, ftitle, uhandle, fhandle, ubutton, fbutton, pbutton,
      uborder, fborder, ruhandle, rfhandle;
    Window window, plate, title, border, handle, close_button, iconify_button,
      maximize_button, resize_handle;

    int x, y, x_resize, y_resize, x_move, y_move, x_grab, y_grab,
      x_maximize, y_maximize, y_border, x_handle;
    unsigned int width, height, title_h, title_w, handle_h, handle_w,
      button_w, button_h, rh_w, rh_h, border_w, border_h, bevel_w, w_maximize,
      h_maximize, resize_label_w;
  } frame;
  
  //char *resizeLabel;
  enum { F_NoInput = 0, F_Passive, F_LocallyActive, F_GloballyActive };
  int focus_mode, window_number, workspace_number;
  
  
protected:
  Bool getState(unsigned long *, unsigned long * = 0);
  Window createToplevelWindow(int, int, unsigned int, unsigned int,
			      unsigned int);
  Window createChildWindow(Window, int ,int, unsigned int, unsigned int,
			   unsigned int);

  void getWMNormalHints(void); 
  void getWMProtocols(void);
  void getWMHints(void);
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
  void restoreGravity(void);
  void setGravityOffsets(void);
  void setState(unsigned long);
  
  
public:
  BlackboxWindow(Blackbox *, BScreen *, Window);
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
#ifdef    SHAPE
  void shapeEvent(XShapeEvent *);
#endif // SHAPE
  
  Bool isTransient(void) { return ((transient) ? True : False); }
  Bool hasTransient(void) { return ((client.transient) ? True : False); }
  Bool isFocused(void) { return focused; }
  Bool isVisible(void) { return visible; }
  Bool isIconic(void) { return iconic; }
  Bool isShaded(void) { return shaded; }
  Bool isIconifiable(void) { return functions.iconify; }
  Bool isMaximizable(void) { return functions.maximize; }
  Bool isResizable(void) { return functions.resize; }
  Bool isClosable(void) { return functions.close; }
  Bool isStuck(void) { return stuck; }
  Bool hasTitlebar(void) { return decorations.titlebar; }
  Bool validateClient(void);
  Bool setInputFocus(void);

  BScreen *getScreen(void) { return screen; }
  
  BlackboxWindow *getTransient(void) { return client.transient; }
  BlackboxWindow *getTransientFor(void) { return client.transient_for; }

  Window getFrameWindow(void) { return frame.window; }
  Window getClientWindow(void) { return client.window; }

  Windowmenu *getWindowmenu(void) { return windowmenu; }
  
  char **getTitle(void) { return &client.title; }
  int getXFrame(void) { return frame.x; }
  int getYFrame(void) { return frame.y; }
  int getXClient(void) { return client.x; }
  int getYClient(void) { return client.y; }
  int getWorkspaceNumber(void) { return workspace_number; }
  int getWindowNumber(void) { return window_number; }
  
  int setWindowNumber(int);
  int setWorkspace(int);

  unsigned int getWidth(void)        { return frame.width; }
  unsigned int getHeight(void)       { return frame.height; }
  unsigned int getClientHeight(void) { return client.height; }
  unsigned int getClientWidth(void)  { return client.width; }
  unsigned int getTitleHeight(void)  { return frame.title_h; }

  void removeIcon(void) { icon = (BlackboxIcon *) 0; }
  void setFocusFlag(Bool);
  void iconify(void);
  void deiconify(Bool = True);
  void close(void);
  void withdraw(void);
  void maximize(unsigned int);
  void shade(void);
  void stick(void);
  void unstick(void);
  void reconfigure(void);
  void installColormap(Bool);
  void restore(void);
  void configure(int, int, unsigned int, unsigned int);
};


#endif // __Window_hh
