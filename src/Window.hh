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
#include "Screen.hh"

#include <Netwm.hh>


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

enum WindowFunction {
  WindowFunctionResize          = 1<<0,
  WindowFunctionMove            = 1<<1,
  WindowFunctionShade           = 1<<2,
  WindowFunctionIconify         = 1<<3,
  WindowFunctionMaximize        = 1<<4,
  WindowFunctionClose           = 1<<5,
  WindowFunctionChangeWorkspace = 1<<6,
  WindowFunctionChangeLayer     = 1<<7,
  WindowFunctionFullScreen      = 1<<8,
  NoWindowFunctions             = 0,
  AllWindowFunctions            = (WindowFunctionResize |
                                   WindowFunctionMove |
                                   WindowFunctionShade |
                                   WindowFunctionIconify |
                                   WindowFunctionMaximize |
                                   WindowFunctionClose |
                                   WindowFunctionChangeWorkspace |
                                   WindowFunctionChangeLayer |
                                   WindowFunctionFullScreen)
};
typedef unsigned short WindowFunctionFlags;

enum WindowDecoration {
  WindowDecorationTitlebar = 1<<0,
  WindowDecorationHandle   = 1<<1,
  WindowDecorationGrip     = 1<<2,
  WindowDecorationBorder   = 1<<3,
  WindowDecorationIconify  = 1<<4,
  WindowDecorationMaximize = 1<<5,
  WindowDecorationClose    = 1<<6,
  NoWindowDecorations      = 0,
  AllWindowDecorations     = (WindowDecorationTitlebar |
                              WindowDecorationHandle |
                              WindowDecorationGrip |
                              WindowDecorationBorder |
                              WindowDecorationIconify |
                              WindowDecorationMaximize |
                              WindowDecorationClose)
};
typedef unsigned char WindowDecorationFlags;

struct EWMH {
  WindowType window_type;
  unsigned int workspace;
  unsigned int modal        : 1;
  unsigned int maxv         : 1;
  unsigned int maxh         : 1;
  unsigned int shaded       : 1;
  unsigned int skip_taskbar : 1;
  unsigned int skip_pager   : 1;
  unsigned int hidden       : 1;
  unsigned int fullscreen   : 1;
  unsigned int above        : 1;
  unsigned int below        : 1;
};
struct MotifHints {
  WindowDecorationFlags decorations;
  WindowFunctionFlags functions;
};
struct WMHints {
  bool accept_focus;
  Window window_group;
  unsigned long initial_state;
};
struct WMNormalHints {
  long flags;
  unsigned int min_width, min_height;
  unsigned int max_width, max_height;
  unsigned int width_inc, height_inc;
  unsigned int min_aspect_x, min_aspect_y;
  unsigned int max_aspect_x, max_aspect_y;
  unsigned int base_width, base_height;
  unsigned int win_gravity;
};
struct WMProtocols {
  unsigned int wm_delete_window : 1;
  unsigned int wm_take_focus    : 1;
};


class BlackboxWindow : public StackEntity, public bt::TimeoutHandler,
                       public bt::EventHandler, public bt::NoCopy {
  Blackbox *blackbox;
  BScreen *_screen;
  bt::Timer *timer;

  Time lastButtonPressTime;  // used for double clicks, when were we clicked

  unsigned int window_number;

  struct ClientState {
    unsigned int visible  : 1; // is visible?
    unsigned int iconic   : 1; // is iconified?
    unsigned int moving   : 1; // is moving?
    unsigned int resizing : 1; // is resizing?
    unsigned int focused  : 1; // has focus?
    unsigned int shaped   : 1; // does the frame use the shape extension?
  };

  struct _client {
    Window window;                    // the client's window
    Colormap colormap;
    Window transient_for;             // which window are we a transient for?
    BlackboxWindowList transientList; // which windows are our transients?

    bt::ustring title, visible_title, icon_title;

    bt::Rect rect, premax;

    int old_bw;                     // client's borderwidth

    unsigned long current_state;

    bt::Netwm::Strut *strut;

    WindowFunctionFlags functions;
    WindowDecorationFlags decorations;

    ClientState state;

    EWMH ewmh;
    MotifHints motif;
    WMHints wmhints;
    WMNormalHints wmnormal;
    WMProtocols wmprotocols;
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
    const ScreenResource::WindowStyle *style;

    // u -> unfocused, f -> has focus
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

    unsigned int label_w;       // width of the label
  } frame;

  Window createToplevelWindow();
  Window createChildWindow(Window parent, unsigned long event_mask,
                           Cursor = None);

  bt::ustring readWMName(void);
  bt::ustring readWMIconName(void);

  EWMH readEWMH(void);
  MotifHints readMotifHints(void);
  WMHints readWMHints(void);
  WMNormalHints readWMNormalHints(void);
  WMProtocols readWMProtocols(void);
  Window readTransientInfo(void);

  void associateClientWindow(void);

  void decorate(void);

  void positionButtons(bool redecorate_label = false);
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
  void redrawCloseButton(bool pressed = false) const;
  void redrawIconifyButton(bool pressed = false) const;
  void redrawMaximizeButton(bool pressed = false) const;
  void redrawHandle(void) const;
  void redrawGrips(void) const;

  void applyGravity(bt::Rect &r);
  void restoreGravity(bt::Rect &r);

  bool readState(void);
  void setState(unsigned long new_state);
  void clearState(void);

  void upsize(void);

  void showGeometry(const bt::Rect &r) const;

  enum Corner { TopLeft, TopRight, BottomLeft, BottomRight };
  void constrain(Corner anchor);

public:
  BlackboxWindow(Blackbox *b, Window w, BScreen *s);
  virtual ~BlackboxWindow(void);

  inline bool isTransient(void) const
  { return client.transient_for != 0; }
  inline bool isGroupTransient(void) const
  { return (client.transient_for != 0
            && client.transient_for == client.wmhints.window_group); }
  inline bool isModal(void) const
  { return client.ewmh.modal; }

  inline WindowType windowType(void) const
  { return client.ewmh.window_type; }

  inline bool hasWindowFunction(WindowFunction function) const
  { return client.functions & function; }
  inline bool hasWindowDecoration(WindowDecoration decoration) const
  { return client.decorations & decoration; }

  // ordered newest to oldest
  inline const BlackboxWindowList &transients(void) const
  { return client.transientList; }

  void addTransient(BlackboxWindow *win);
  void removeTransient(BlackboxWindow *win);

  BlackboxWindow *findTransientFor(void) const;
  BlackboxWindow *findNonTransientParent(void) const;

  BWindowGroup *findWindowGroup(void) const;

  inline BScreen *screen(void) const
  { return _screen; }

  // StackEntity interface
  inline Window windowID(void) const
  { return frame.window; }

  inline Window frameWindow(void) const
  { return frame.window; }
  inline Window clientWindow(void) const
  { return client.window; }

  inline const bt::ustring &title(void) const
  { return client.title; }
  inline const bt::ustring &iconTitle(void) const
  { return client.icon_title.empty() ? client.title : client.icon_title; }

  inline unsigned int workspace(void) const
  { return client.ewmh.workspace; }
  void setWorkspace(unsigned int new_workspace);

  inline unsigned int windowNumber(void) const
  { return window_number; }
  inline void setWindowNumber(int n)
  { window_number = n; }

  inline const bt::Rect &frameRect(void) const
  { return frame.rect; }
  inline const bt::Rect &clientRect(void) const
  { return client.rect; }

  const WMHints &wmHints(void) const
  { return client.wmhints; }
  const WMNormalHints &wmNormalHints(void) const
  { return client.wmnormal; }
  const WMProtocols &wmProtocols(void) const
  { return client.wmprotocols; }

  unsigned long currentState(void) const
  { return client.current_state; }

  inline bool isFocused(void) const
  { return client.state.focused; }
  void setFocused(bool focused);
  bool setInputFocus(void);

  inline bool isVisible(void) const
  { return client.state.visible; }
  void show(void);
  void hide(void);
  void close(void);
  void activate(void);

  inline bool isShaded(void) const
  { return client.ewmh.shaded; }
  void setShaded(bool shaded);

  inline bool isIconic(void) const
  { return client.state.iconic; }
  void iconify(void); // call show() to deiconify

  inline bool isMaximized(void) const
  { return client.ewmh.maxh || client.ewmh.maxv; }
  // ### change to setMaximized()?
  void maximize(unsigned int button);
  void remaximize(void);

  inline bool isFullScreen(void) const
  { return client.ewmh.fullscreen; }
  void setFullScreen(bool);

  void reconfigure(void);
  void grabButtons(void);
  void ungrabButtons(void);
  void restore(void);
  void configure(int dx, int dy, unsigned int dw, unsigned int dh);
  inline void configure(const bt::Rect &r)
  { configure(r.x(), r.y(), r.width(), r.height()); }

  void clientMessageEvent(const XClientMessageEvent * const ce);
  void buttonPressEvent(const XButtonEvent * const be);
  void buttonReleaseEvent(const XButtonEvent * const re);
  void motionNotifyEvent(const XMotionEvent * const me);
  void destroyNotifyEvent(const XDestroyWindowEvent * const /*unused*/);
  void unmapNotifyEvent(const XUnmapEvent * const /*unused*/);
  void reparentNotifyEvent(const XReparentEvent * const /*unused*/);
  void propertyNotifyEvent(const XPropertyEvent * const pe);
  void exposeEvent(const XExposeEvent * const ee);
  void configureRequestEvent(const XConfigureRequestEvent * const cr);
  void enterNotifyEvent(const XCrossingEvent * const ce);
  void leaveNotifyEvent(const XCrossingEvent * const /*unused*/);

#ifdef SHAPE
  void configureShape(void);
  void shapeEvent(const XEvent * const /*unused*/);
#endif // SHAPE

  virtual void timeout(bt::Timer *);
};

#endif // __Window_hh
