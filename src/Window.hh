// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Window.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2003
//         Bradley T Hughes <bhughes at trolltech.com>
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

#include "BlackboxResource.hh"
#include "StackingList.hh"

#include <EventHandler.hh>
#include <Netwm.hh>
#include <Rect.hh>
#include <Timer.hh>
#include <Util.hh>

class Blackbox;
class Windowmenu;

class BlackboxWindow : public bt::TimeoutHandler, public bt::EventHandler,
                       public bt::NoCopy {
public:
  enum WMFunction {
    Func_Resize   = (1l << 0),
    Func_Move     = (1l << 1),
    Func_Shade    = (1l << 2),
    Func_Iconify  = (1l << 3),
    Func_Maximize = (1l << 4),
    Func_Close    = (1l << 5),
    Func_All      = (Func_Resize |
                     Func_Move |
                     Func_Shade |
                     Func_Iconify |
                     Func_Maximize |
                     Func_Close)
  };
  typedef unsigned char WMFunctionFlags;

  enum WMDecoration {
    Decor_Titlebar = (1l << 0),
    Decor_Handle   = (1l << 1),
    Decor_Grip     = (1l << 2),
    Decor_Border   = (1l << 3),
    Decor_Iconify  = (1l << 4),
    Decor_Maximize = (1l << 5),
    Decor_Close    = (1l << 6),
    Decor_All      = (Decor_Titlebar |
                      Decor_Handle |
                      Decor_Grip |
                      Decor_Border |
                      Decor_Iconify |
                      Decor_Maximize |
                      Decor_Close)
  };
  typedef unsigned char WMDecorationFlags;

  enum WindowType {
    WindowTypeNormal,
    WindowTypeDialog,
    WindowTypeDesktop,
    WindowTypeDock,
    WindowTypeMenu,
    WindowTypeSplash,
    WindowTypeToolbar,
    WindowTypeUtility
  };

private:
  Blackbox *blackbox;
  BScreen *screen;
  bt::Timer *timer;

  Time lastButtonPressTime;  // used for double clicks, when were we clicked
  Windowmenu *windowmenu;

  unsigned int window_number;

  enum FocusMode { F_NoInput = 0, F_Passive,
                   F_LocallyActive, F_GloballyActive };
  enum WMSkip { SKIP_NONE, SKIP_TASKBAR, SKIP_PAGER, SKIP_BOTH };

  struct WMState {
    bool modal,              // is modal? (must be dismissed to continue)
      shaded,                // is shaded?
      iconic,                // is iconified?
      fullscreen,            // is a full screen window
      moving,                // is moving?
      resizing,              // is resizing?
      focused,               // has focus?
      send_focus_message,    // should we send focus messages to our client?
      shaped;                // does the frame use the shape extension?
    unsigned int maximized;  // maximize is special, the number corresponds
                             // with a mouse button
                             // if 0, not maximized
                             // 1 = HorizVert, 2 = Vertical, 3 = Horizontal

    // full screen, above, normal, below, desktop
    StackingList::Layer layer;

    WMSkip skip;             // none, taskbar, pager, both
  };

  struct _client {
    Window window,                  // the client's window
      window_group;
    Colormap colormap;
    BlackboxWindow *transient_for;  // which window are we a transient for?
    BlackboxWindowList transientList; // which windows are our transients?

    std::string title, icon_title;

    bt::Rect rect, premax;

    int old_bw;                       // client's borderwidth

    unsigned int
      min_width, min_height,        // can not be resized smaller
      max_width, max_height,        // can not be resized larger
      width_inc, height_inc,        // increment step
      min_aspect_x, min_aspect_y,   // minimum aspect ratio
      max_aspect_x, max_aspect_y,   // maximum aspect ratio
      base_width, base_height,
      win_gravity;

    unsigned long current_state, normal_hint_flags;
    unsigned int workspace;

    bt::Netwm::Strut *strut;
    FocusMode focus_mode;
    WMState state;
    WindowType window_type;
    WMFunctionFlags functions;
    WMDecorationFlags decorations;
  } client;

  /*
   * client window = the application's window
   * frame window = the window drawn around the outside of the client window
   *                by the window manager which contains items like the
   *                titlebar and close button
   * title = the titlebar drawn above the client window, it displays the
   *         window's name and any buttons for interacting with the window,
   *         such as iconify, maximize, and close
   * label = the window in the titlebar where the title is drawn
   * buttons = maximize, iconify, close
   * handle = the bar drawn at the bottom of the window, which contains the
   *          left and right grips used for resizing the window
   * grips = the smaller reactangles in the handle, one of each side of it.
   *         When clicked and dragged, these resize the window interactively
   * border = the line drawn around the outside edge of the frame window,
   *          between the title, the bordered client window, and the handle.
   *          Also drawn between the grips and the handle
   */

  struct _frame {
    ScreenResource::WindowStyle* style;

    // u -> unfocused, f -> has focus
    unsigned long uborder_pixel, fborder_pixel;
    Pixmap ulabel, flabel, utitle, ftitle, uhandle, fhandle,
      ubutton, fbutton, pbutton, ugrip, fgrip;

    Window window,       // the frame
      plate,             // holds the client
      title,
      label,
      handle,
      close_button, iconify_button, maximize_button,
      right_grip, left_grip;

    /*
     * size and location of the box drawn while the window dimensions or
     * location is being changed, ie. resized or moved
     */
    bt::Rect changing;

    // frame geometry
    bt::Rect rect;

    /*
     * margins between the frame and client, this has nothing to do
     * with netwm, it is simply code reuse for similar functionality
     */
    bt::Netwm::Strut margin;
    int grab_x, grab_y;         // where was the window when it was grabbed?

    unsigned int inside_w, inside_h, // window w/h without border_w
      label_w, mwm_border_w, border_w;
  } frame;

  Window createToplevelWindow();
  Window createChildWindow(Window parent, unsigned long event_mask,
                           Cursor = None);

  void getNetwmHints(void);
  void getWMName(void);
  void getWMIconName(void);
  void getWMNormalHints(void);
  void getWMProtocols(void);
  void getWMHints(void);
  void getMWMHints(void);
  void getTransientInfo(void);

  void setNetWMAttributes(void);

  void associateClientWindow(void);

  void decorate(void);

  void positionButtons(bool redecorate_label = False);
  void positionWindows(void);

  void createTitlebar(void);
  void destroyTitlebar(void);
  void createHandle(void);
  void destroyHandle(void);
  void createGrips(void);
  void destroyGrips(void);
  void createIconifyButton(void);
  void destroyIconifyButton(void);
  void createMaximizeButton(void);
  void destroyMaximizeButton(void);
  void createCloseButton(void);
  void destroyCloseButton(void);

  void redrawWindowFrame(void) const;
  void redrawTitle(void) const;
  void redrawLabel(void) const;
  void redrawAllButtons(void) const;
  void redrawCloseButton(bool pressed) const;
  void redrawIconifyButton(bool pressed) const;
  void redrawMaximizeButton(bool pressed) const;
  void redrawHandle(void) const;
  void redrawGrips(void) const;

  void applyGravity(bt::Rect &r);
  void restoreGravity(bt::Rect &r);

  bool getState(void);
  void setState(unsigned long new_state, bool closing = False);

  void upsize(void);

  void showGeometry(const bt::Rect &r) const;

  enum Corner { TopLeft, TopRight, BottomLeft, BottomRight };
  void constrain(Corner anchor);

public:
  BlackboxWindow(Blackbox *b, Window w, BScreen *s);
  virtual ~BlackboxWindow(void);

  inline bool isTransient(void) const { return client.transient_for != 0; }
  inline bool isFocused(void) const { return client.state.focused; }
  inline bool isVisible(void) const
  { return (! (client.current_state == 0 || client.state.iconic)); }
  inline bool isIconic(void) const { return client.state.iconic; }
  inline bool isShaded(void) const { return client.state.shaded; }
  inline bool isMaximized(void) const { return client.state.maximized; }
  inline bool isModal(void) const { return client.state.modal; }

  inline bool hasFunction(WMFunction func) const
  { return client.functions & func; }
  inline bool hasDecoration(WMDecoration decor) const
  { return client.decorations & decor; }

  inline const BlackboxWindowList &getTransients(void) const
  { return client.transientList; }
  BlackboxWindow *getTransientFor(void) const;

  inline BScreen *getScreen(void) const { return screen; }

  inline Window getFrameWindow(void) const { return frame.window; }
  inline Window getClientWindow(void) const { return client.window; }
  inline Window getGroupWindow(void) const { return client.window_group; }

  inline Windowmenu* getWindowmenu(void) const { return windowmenu; }

  inline const char *getTitle(void) const
  { return client.title.c_str(); }
  inline const char *getIconTitle(void) const
  { return client.icon_title.c_str(); }

  inline unsigned int getWorkspaceNumber(void) const
  { return client.workspace; }
  inline unsigned int getWindowNumber(void) const { return window_number; }

  inline const bt::Rect &frameRect(void) const { return frame.rect; }
  inline const bt::Rect &clientRect(void) const { return client.rect; }

  inline unsigned int getTitleHeight(void) const
  { return frame.style->title_height; }

  inline StackingList::Layer layer(void) const
  { return client.state.layer; }
  void setLayer(StackingList::Layer layer);

  unsigned long normalHintFlags(void) const
  { return client.normal_hint_flags; }

  inline void setWindowNumber(int n) { window_number = n; }

  inline void setModal(bool flag) { client.state.modal = flag; }

  bool validateClient(void) const;
  bool setInputFocus(void);

  void setFocusFlag(bool focus);
  void iconify(void);
  void deiconify(bool reassoc = True, bool raise = True);
  void show(void);
  void close(void);
  void withdraw(void);
  void maximize(unsigned int button);
  void remaximize(void);
  void shade(void);

  inline bool isFullScreen(void) const
  { return client.state.fullscreen; }
  void setFullScreen(bool);

  void reconfigure(void);
  void grabButtons(void);
  void ungrabButtons(void);
  void restore(bool remap);
  void configure(int dx, int dy, unsigned int dw, unsigned int dh);
  inline void configure(const bt::Rect &r)
  { configure(r.x(), r.y(), r.width(), r.height()); }
  void setWorkspace(unsigned int n);

  void clientMessageEvent(const XClientMessageEvent * const ce);
  void buttonPressEvent(const XButtonEvent * const be);
  void buttonReleaseEvent(const XButtonEvent * const re);
  void motionNotifyEvent(const XMotionEvent * const me);
  void destroyNotifyEvent(const XDestroyWindowEvent * const /*unused*/);
  void mapRequestEvent(const XMapRequestEvent * const /*unused*/);
  void unmapNotifyEvent(const XUnmapEvent * const /*unused*/);
  void reparentNotifyEvent(const XReparentEvent * const /*unused*/);
  void propertyNotifyEvent(const XPropertyEvent * const pe);
  void exposeEvent(const XExposeEvent * const ee);
  void configureRequestEvent(const XConfigureRequestEvent * const cr);
  void enterNotifyEvent(const XCrossingEvent * const ce);
  void leaveNotifyEvent(const XCrossingEvent * const /*unused*/);

#ifdef    SHAPE
  void configureShape(void);
  void shapeEvent(const XEvent * const /*unused*/);
#endif // SHAPE

  virtual void timeout(bt::Timer *);
};


#endif // __Window_hh
