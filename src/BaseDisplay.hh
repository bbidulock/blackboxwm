// BaseDisplay.hh for Blackbox - an X11 Window manager
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
  
#ifndef   __BaseDisplay_hh
#define   __BaseDisplay_hh

#include <X11/Xlib.h>
#include <X11/Xatom.h>

// forward declaration
class BaseDisplay;
class ScreenInfo;

#include "LinkedList.hh"     
#include "Timer.hh"

#define NETAttribShaded      (1l << 0)
#define NETAttribMaxHoriz    (1l << 1)
#define NETAttribMaxVert     (1l << 2)
#define NETAttribOmnipresent (1l << 3)
#define NETAttribWorkspace   (1l << 4)
#define NETAttribStack       (1l << 5)
#define NETAttribDecoration  (1l << 6)

#define NETStackTop          (0)
#define NETStackNormal       (1)
#define NETStackBottom       (2)

#define NETDecorNone         (0)
#define NETDecorNormal	     (1)
#define NETDecorTiny         (2)
#define NETDecorTool         (3)

typedef struct _net_hints {
  unsigned long flags, attrib, workspace, stack, decoration;
} NetHints;

typedef struct _net_attributes {
  unsigned long flags, attrib, workspace, stack;
  int premax_x, premax_y;
  unsigned int premax_w, premax_h;
} NetAttributes;

#define PropNetHintsElements	(5)
#define PropNetAttributesElements (8)


class BaseDisplay {
private:
  struct cursor {
    Cursor session, move, ll_angle, lr_angle;
  } cursor;

  struct shape {
    Bool extensions;
    int event_basep, error_basep;
  } shape;

  Atom xa_wm_colormap_windows, xa_wm_protocols, xa_wm_state,
    xa_wm_delete_window, xa_wm_take_focus, xa_wm_change_state,
    motif_wm_hints;

  // NETAttributes
  Atom net_attributes, net_change_attributes, net_hints;

  // NETStructureMessages
  Atom net_structure_messages, net_notify_startup,
    net_notify_window_add, net_notify_window_del,
    net_notify_window_focus, net_notify_current_workspace,
    net_notify_workspace_count, net_notify_window_raise,
    net_notify_window_lower;

  // message_types for client -> wm messages
  Atom net_change_workspace, net_change_window_focus,
    net_cycle_window_focus;

  Bool _startup, _shutdown;
  Display *display;
  LinkedList<ScreenInfo> *screenInfoList;
  LinkedList<BTimer> *timerList;

  char *display_name, *application_name;
  int number_of_screens, server_grabs, colors_per_channel;


protected:
  // pure virtual function... you must override this
  virtual void process_event(XEvent *) = 0;


public:
  BaseDisplay(char *, char * = 0);
  virtual ~BaseDisplay(void);

  inline const Atom &getWMChangeStateAtom(void) const
    { return xa_wm_change_state; }
  inline const Atom &getWMStateAtom(void) const
    { return xa_wm_state; }
  inline const Atom &getWMDeleteAtom(void) const
    { return xa_wm_delete_window; }
  inline const Atom &getWMProtocolsAtom(void) const
    { return xa_wm_protocols; }
  inline const Atom &getWMTakeFocusAtom(void) const
    { return xa_wm_take_focus; }
  inline const Atom &getWMColormapAtom(void) const
    { return xa_wm_colormap_windows; }
  inline const Atom &getMotifWMHintsAtom(void) const
    { return motif_wm_hints; }

  // this atom is for normal app->WM hints about decorations, stacking,
  // starting workspace etc...
  inline const Atom &getNETHintsAtom(void) const
    { return net_hints;}

  // these atoms are for normal app->WM interaction beyond the scope of the
  // ICCCM...
  inline const Atom &getNETAttributesAtom(void) const
    { return net_attributes; }
  inline const Atom &getNETChangeAttributesAtom(void) const
    { return net_change_attributes; }
  
  // these atoms are for window->WM interaction, with more control and
  // information on window "structure"... common examples are
  // notifying apps when windows are raised/lowered... when the user changes
  // workspaces... i.e. "pager talk"
  inline const Atom &getNETStructureMessagesAtom(void) const
    { return net_structure_messages; }

  // *Notify* portions of the NETStructureMessages protocol
  inline const Atom &getNETNotifyStartupAtom(void) const
    { return net_notify_startup; }
  inline const Atom &getNETNotifyWindowAddAtom(void) const
    { return net_notify_window_add; }
  inline const Atom &getNETNotifyWindowDelAtom(void) const
    { return net_notify_window_del; }
  inline const Atom &getNETNotifyWindowFocusAtom(void) const
    { return net_notify_window_focus; }
  inline const Atom &getNETNotifyCurrentWorkspaceAtom(void) const
    { return net_notify_current_workspace; }
  inline const Atom &getNETNotifyWorkspaceCountAtom(void) const
    { return net_notify_workspace_count; }
  inline const Atom &getNETNotifyWindowRaiseAtom(void) const
    { return net_notify_window_raise; }
  inline const Atom &getNETNotifyWindowLowerAtom(void) const
    { return net_notify_window_lower; }

  // atoms to change that request changes to the desktop environment during
  // runtime... these messages can be sent by any client... as the sending
  // client window id is not included in the ClientMessage event...
  inline const Atom &getNETChangeWorkspaceAtom(void) const
    { return net_change_workspace; }
  inline const Atom &getNETChangeWindowFocusAtom(void) const
    { return net_change_window_focus; }
  inline const Atom &getNETCycleWindowFocusAtom(void) const
    { return net_cycle_window_focus; }

  inline ScreenInfo *getScreenInfo(int s)
    { return (ScreenInfo *) screenInfoList->find(s); }

  inline const Bool &hasShapeExtensions(void) const
    { return shape.extensions; }
  inline const Bool &doShutdown(void) const
    { return _shutdown; }
  inline const Bool &isStartup(void) const
    { return _startup; }

  inline const Cursor &getSessionCursor(void) const
    { return cursor.session; }
  inline const Cursor &getMoveCursor(void) const
    { return cursor.move; }
  inline const Cursor &getLowerLeftAngleCursor(void) const
    { return cursor.ll_angle; }
  inline const Cursor &getLowerRightAngleCursor(void) const
    { return cursor.lr_angle; }

  inline Display *getXDisplay(void) { return display; }

  inline const char *getXDisplayName(void) const
    { return (const char *) display_name; }
  inline const char *getApplicationName(void) const
    { return (const char *) application_name; }

  inline const int &getNumberOfScreens(void) const
    { return number_of_screens; }
  inline const int &getShapeEventBase(void) const
    { return shape.event_basep; }

  inline void shutdown(void) { _shutdown = True; }
  inline void run(void) { _startup = _shutdown = False; }

  const Bool validateWindow(Window);

  void grab(void);  
  void ungrab(void);
  void eventLoop(void);
  void addTimer(BTimer *);
  void removeTimer(BTimer *);

  // another pure virtual... this is used to handle signals that BaseDisplay
  // doesn't understand itself
  virtual Bool handleSignal(int) = 0;
};


class ScreenInfo {
private:
  BaseDisplay *basedisplay;
  Visual *visual;
  Window root_window;

  int depth, screen_number;
  unsigned int width, height;


protected:


public:
  ScreenInfo(BaseDisplay *, int);

#ifdef    DEBUG
  ~ScreenInfo(void);
#endif // DEBUG

  inline BaseDisplay *getBaseDisplay(void) { return basedisplay; }
  inline Visual *getVisual(void) { return visual; }
  inline const Window &getRootWindow(void) const { return root_window; }

  inline const int &getDepth(void) const { return depth; }
  inline const int &getScreenNumber(void) const { return screen_number; }

  inline const unsigned int &getWidth(void) const { return width; }
  inline const unsigned int &getHeight(void) const { return height; }
};


#endif // __BaseDisplay_hh
