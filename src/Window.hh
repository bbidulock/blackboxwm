//
// AssociatedWindow.hh for Blackbox - an X11 Window manager
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

#ifndef __AssociatedWindow_hh
#define __AssociatedWindow_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef SHAPE
#  include <X11/extensions/shape.h>
#endif // SHAPE

#ifdef MWM_HINTS

#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)

#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)

typedef struct MwmHints {
  unsigned long flags, functions, decorations, junk, junk2;
} MwmHints;

#endif // MWM_HINTS

// forward declaration
class BlackboxWindow;

class Blackbox;
class BlackboxIcon;
class Windowmenu;

#include "graphics.hh"


class BlackboxWindow {
private:
  Blackbox *blackbox;
  BlackboxIcon *icon;
  Windowmenu *windowmenu;
  
  Bool moving, resizing, shaded, maximized, visible, iconic, transient,
    focused, resizable, stuck;
  Display *display;
  
  struct client {
    BlackboxWindow *transient_for,  // which window are we a transient for?
      *transient;                   // which window is our transient?
    Window window, window_group;
    
    char *title;
    int x, y;
    unsigned int width, height,
      base_w, base_h,
      inc_w, inc_h,
      min_w, min_h, max_w, max_h,
      min_ax, min_ay, max_ax, max_ay;
    long normal_hint_flags, wmhint_flags, initial_state;

#ifdef MWM_HINTS
    MwmHints mwm_hints;
#endif // MWM_HINTS
  } client;
  
  struct frame {
    Bool shaped;
    GC ftextGC, utextGC;
    Pixmap utitle, ftitle, uhandle, fhandle, fbutton, ubutton, pbutton;
    Window window, title, border, handle, close_button, iconify_button,
      maximize_button, resize_handle;
    int x, y, x_resize, y_resize, x_move, y_move, x_grab, y_grab;
    unsigned int width, height, title_h, title_w, handle_h, handle_w,
      button_w, button_h, rh_w, rh_h, border_w, border_h, bevel_w;
  } frame;

  struct protocols {
    Atom WMDeleteWindow, WMProtocols;
    Bool WM_DELETE_WINDOW, WM_TAKE_FOCUS, WM_COLORMAP_WINDOWS;
  } protocols;

  char *resizeLabel;
  enum { F_NoInput = 0, F_Passive, F_LocallyActive, F_GloballyActive };
  int focus_mode, window_number, workspace_number;


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

  Bool validateClient(void);
  Bool fetchWMState(unsigned long *);


public:
  BlackboxWindow(Blackbox *, Window);
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
#endif // SHAPE

  Bool setInputFocus(void);
  int setWindowNumber(int);
  int setWorkspace(int);
  void setFocusFlag(Bool);
  void iconifyWindow(Bool = True);
  void deiconifyWindow(Bool = True);
  void closeWindow(void);
  void withdrawWindow(Bool = True);
  void maximizeWindow(void);
  void shadeWindow(void);
  void unstickWindow(void);
  void Reconfigure(void);

  BlackboxWindow *Transient(void) { return client.transient; }
  BlackboxWindow *TransientFor(void) { return client.transient_for; }
  Bool isTransient(void) { return ((transient) ? True : False); }
  Bool hasTransient(void) { return ((client.transient) ? True : False); }
  Bool isFocused(void) { return focused; }
  Bool isVisible(void) { return visible; }
  Bool isIconic(void) { return iconic; }
  Bool isResizable(void) { return resizable; }
  Bool isClosable(void) { return protocols.WM_DELETE_WINDOW; }
  Bool isStuck(void) { return stuck; }
  Window frameWindow(void) { return frame.window; }
  Window clientWindow(void) { return client.window; }
  char **Title(void) { return &client.title; }
  int XFrame(void) { return frame.x; }
  int YFrame(void) { return frame.y; }
  int XClient(void) { return client.x; }
  int YClient(void) { return client.y; }
  int workspace(void) { return workspace_number; }
  int windowNumber(void) { return window_number; }
  unsigned int clientHeight(void) { return client.height; }
  unsigned int clientWidth(void) { return client.width; }
  void removeIcon(void) { icon = NULL; }
  void stickWindow(Bool s) { stuck = s; }

  Windowmenu *Menu(void) { return windowmenu; }
};


#endif // __AssociatedWindow_hh
