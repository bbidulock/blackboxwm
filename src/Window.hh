// Window.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef   __Window_hh
#define   __Window_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef    SHAPE
#  include <X11/extensions/shape.h>
#endif // SHAPE

// forward declaration
class BlackboxWindow;

#include "BaseDisplay.hh"
#include "Timer.hh"
#include "Windowmenu.hh"

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


class BlackboxWindow : public TimeoutHandler {
private:
  BImageControl *image_ctrl;
  Blackbox *blackbox;
  Bool moving, resizing, shaded, maximized, visible, iconic, transient,
    focused, stuck, modal, send_focus_message, managed;
  BScreen *screen;
  BTimer *timer;
  Display *display;
  BlackboxAttributes blackbox_attrib;

  Time lastButtonPressTime;
  Windowmenu *windowmenu;

  int focus_mode, window_number, workspace_number;
  unsigned long current_state;

  struct _client {
    BlackboxWindow *transient_for,  // which window are we a transient for?
      *transient;                   // which window is our transient?
    Window window, window_group;

    char *title, *icon_title;
    int x, y, old_bw, title_len;
    unsigned int width, height, title_text_w,
      min_width, min_height, max_width, max_height, width_inc, height_inc,
      min_aspect_x, min_aspect_y, max_aspect_x, max_aspect_y,
      base_width, base_height, win_gravity;
    unsigned long initial_state, normal_hint_flags, wm_hint_flags;

    MwmHints *mwm_hint;
    BlackboxHints *blackbox_hint;
  } client;

  struct _decorations {
    Bool titlebar, handle, border, iconify, maximize, close, menu;
  } decorations;

  struct _functions {
    Bool resize, move, iconify, maximize, close;
  } functions;

  struct _frame {
    Bool shaped;
    unsigned long ulabel_pixel, flabel_pixel, utitle_pixel,
      ftitle_pixel, uhandle_pixel, fhandle_pixel, ubutton_pixel,
      fbutton_pixel, pbutton_pixel, uborder_pixel, fborder_pixel,
      ugrip_pixel, fgrip_pixel;
    Pixmap ulabel, flabel, utitle, ftitle, uhandle, fhandle,
      ubutton, fbutton, pbutton, uborder, fborder, ugrip, fgrip;
    Window window, plate, title, label, border, handle,
      close_button, iconify_button, maximize_button,
      right_grip, left_grip;

    int x, y, resize_x, resize_y, move_x, move_y, grab_x, grab_y,
      y_border, y_handle;
    unsigned int width, height, title_h, title_w, label_w, label_h,
      handle_h, handle_w, button_w, button_h, grip_w, grip_h,
      border_w, border_h, bevel_w, resize_w, resize_h, snap_w, snap_h;
  } frame;

  enum { F_NoInput = 0, F_Passive, F_LocallyActive, F_GloballyActive };


protected:
  Bool getState(void);
  Window createToplevelWindow(int, int, unsigned int, unsigned int,
			      unsigned int);
  Window createChildWindow(Window, Cursor = None);

  void getWMName(void);
  void getWMIconName(void);
  void getWMNormalHints(void);
  void getWMProtocols(void);
  void getWMHints(void);
  void getMWMHints(void);
  void getBlackboxHints(void);
  void setNetWMAttributes(void);
  void associateClientWindow(void);
  void decorate(void);
  void positionButtons(void);
  void positionWindows(void);
  void createCloseButton(void);
  void createIconifyButton(void);
  void createMaximizeButton(void);
  void redrawLabel(void);
  void redrawAllButtons(void);
  void redrawCloseButton(Bool);
  void redrawIconifyButton(Bool);
  void redrawMaximizeButton(Bool);
  void restoreGravity(void);
  void setGravityOffsets(void);
  void setState(unsigned long);
  void upsize(void);
  void downsize(void);
  void right_fixsize(int * = 0, int * = 0);
  void left_fixsize(int * = 0, int * = 0);


public:
  BlackboxWindow(Blackbox *, Window, BScreen * = (BScreen *) 0);
  virtual ~BlackboxWindow(void);

  inline const Bool isTransient(void) const
  { return ((transient) ? True : False); }
  inline const Bool hasTransient(void) const
  { return ((client.transient) ? True : False); }
  inline const Bool &isFocused(void) const { return focused; }
  inline const Bool &isVisible(void) const { return visible; }
  inline const Bool &isIconic(void) const { return iconic; }
  inline const Bool &isShaded(void) const { return shaded; }
  inline const Bool &isMaximized(void) const { return maximized; }
  inline const Bool &isIconifiable(void) const { return functions.iconify; }
  inline const Bool &isMaximizable(void) const { return functions.maximize; }
  inline const Bool &isResizable(void) const { return functions.resize; }
  inline const Bool &isClosable(void) const { return functions.close; }
  inline const Bool &isStuck(void) const { return stuck; }
  inline const Bool &hasTitlebar(void) const { return decorations.titlebar; }

  inline BScreen *getScreen(void) { return screen; }

  inline BlackboxWindow *getTransient(void) { return client.transient; }
  inline BlackboxWindow *getTransientFor(void) { return client.transient_for; }

  inline const Window &getFrameWindow(void) const { return frame.window; }
  inline const Window &getClientWindow(void) const { return client.window; }

  inline Windowmenu *getWindowmenu(void) { return windowmenu; }

  inline char **getTitle(void) { return &client.title; }
  inline char **getIconTitle(void) { return &client.icon_title; }
  inline const int &getXFrame(void) const { return frame.x; }
  inline const int &getYFrame(void) const { return frame.y; }
  inline const int &getXClient(void) const { return client.x; }
  inline const int &getYClient(void) const { return client.y; }
  inline const int &getWorkspaceNumber(void) const { return workspace_number; }
  inline const int &getWindowNumber(void) const { return window_number; }

  inline const unsigned int &getWidth(void) const { return frame.width; }
  inline const unsigned int &getHeight(void) const { return frame.height; }
  inline const unsigned int &getClientHeight(void) const
  { return client.height; }
  inline const unsigned int &getClientWidth(void) const
  { return client.width; }
  inline const unsigned int &getTitleHeight(void) const
  { return frame.title_h; }

  inline void setWindowNumber(int n) { window_number = n; }
  
  Bool validateClient(void);
  Bool setInputFocus(void);

  void setFocusFlag(Bool);
  void iconify(void);
  void deiconify(Bool = True, Bool = True);
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
  void setWorkspace(int n);
  void changeBlackboxHints(BlackboxHints *);
  void restoreAttributes(void);

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

  virtual void timeout(void);
};


#endif // __Window_hh
