// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Window.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2005
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

// make sure we get bt::textPropertyToString()
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "Window.hh"
#include "Screen.hh"
#include "WindowGroup.hh"
#include "Windowmenu.hh"
#include "Workspace.hh"
#include "blackbox.hh"

#include <Pen.hh>
#include <PixmapCache.hh>
#include <Unicode.hh>

#include <X11/Xatom.h>
#ifdef SHAPE
#  include <X11/extensions/shape.h>
#endif

#include <assert.h>

/*
  sometimes C++ is a pain in the ass... it gives us stuff like the
  default copy constructor and assignment operator (which does member
  my member copy/assignment), but what we don't get is a default
  comparison operator... how fucked up is that?

  the language is designed to handle this:

  struct foo { int a; int b };
  foo a = { 0 , 0 }, b = a;
  foo c;
  c = a;

  BUT, BUT, BUT, forget about doing this:

  a = findFoo();
  if (c == a)
     return; // nothing to do
  c = a;

  ARGH!@#)(!*@#)(@#*$(!@#
*/
bool operator==(const WMNormalHints &x, const WMNormalHints &y);


// Event mask used for managed client windows.
const unsigned long client_window_event_mask =
  (PropertyChangeMask | StructureNotifyMask);


/*
 * Returns the appropriate WindowType based on the _NET_WM_WINDOW_TYPE
 */
static WindowType window_type_from_atom(const bt::EWMH &ewmh, Atom atom) {
  if (atom == ewmh.wmWindowTypeDialog())
    return WindowTypeDialog;
  if (atom == ewmh.wmWindowTypeDesktop())
    return WindowTypeDesktop;
  if (atom == ewmh.wmWindowTypeDock())
    return WindowTypeDock;
  if (atom == ewmh.wmWindowTypeMenu())
    return WindowTypeMenu;
  if (atom == ewmh.wmWindowTypeSplash())
    return WindowTypeSplash;
  if (atom == ewmh.wmWindowTypeToolbar())
    return WindowTypeToolbar;
  if (atom == ewmh.wmWindowTypeUtility())
    return WindowTypeUtility;
  return WindowTypeNormal;
}


/*
 * Determine the appropriate decorations and functions based on the
 * given properties and hints.
 */
static void update_decorations(WindowDecorationFlags &decorations,
                               WindowFunctionFlags &functions,
                               bool transient,
                               const EWMH &ewmh,
                               const MotifHints &motifhints,
                               const WMNormalHints &wmnormal,
                               const WMProtocols &wmprotocols) {
  decorations = AllWindowDecorations;
  functions   = AllWindowFunctions;

  // transients should be kept on the same workspace are their parents
  if (transient)
    functions &= ~WindowFunctionChangeWorkspace;

  // modify the window decorations/functions based on window type
  switch (ewmh.window_type) {
  case WindowTypeDialog:
    decorations &= ~(WindowDecorationIconify |
                     WindowDecorationMaximize);
    functions   &= ~(WindowFunctionShade |
                     WindowFunctionIconify |
                     WindowFunctionMaximize |
                     WindowFunctionChangeLayer |
                     WindowFunctionFullScreen);
    break;

  case WindowTypeDesktop:
  case WindowTypeDock:
  case WindowTypeSplash:
    decorations = NoWindowDecorations;
    functions   = NoWindowFunctions;
    break;

  case WindowTypeToolbar:
  case WindowTypeMenu:
    decorations &= ~(WindowDecorationHandle |
                     WindowDecorationGrip |
                     WindowDecorationBorder |
                     WindowDecorationIconify |
                     WindowDecorationMaximize);
    functions   &= ~(WindowFunctionResize |
                     WindowFunctionShade |
                     WindowFunctionIconify |
                     WindowFunctionMaximize |
                     WindowFunctionFullScreen);
    break;

  case WindowTypeUtility:
    decorations &= ~(WindowDecorationIconify |
                     WindowDecorationMaximize);
    functions   &= ~(WindowFunctionShade |
                     WindowFunctionIconify |
                     WindowFunctionMaximize);
    break;

  default:
    break;
  }

  // mask away stuff turned off by Motif hints
  decorations &= motifhints.decorations;
  functions   &= motifhints.functions;

  // disable shade if we do not have a titlebar
  if (!(decorations & WindowDecorationTitlebar))
    functions &= ~WindowFunctionShade;

  // disable grips and maximize if we have a fixed size window
  if ((wmnormal.flags & (PMinSize|PMaxSize)) == (PMinSize|PMaxSize)
      && wmnormal.max_width <= wmnormal.min_width
      && wmnormal.max_height <= wmnormal.min_height) {
    decorations &= ~(WindowDecorationMaximize |
                     WindowDecorationGrip);
    functions   &= ~(WindowFunctionResize |
                     WindowFunctionMaximize);
  }

  // cannot close if client doesn't understand WM_DELETE_WINDOW
  if (!wmprotocols.wm_delete_window) {
    decorations &= ~WindowDecorationClose;
    functions   &= ~WindowFunctionClose;
  }
}


/*
 * Calculate the frame margin based on the given decorations and
 * style.
 */
static
bt::EWMH::Strut update_margin(WindowDecorationFlags decorations,
                              const WindowStyle &style) {
  bt::EWMH::Strut margin;

  const unsigned int bw = ((decorations & WindowDecorationBorder)
                           ? style.frame_border_width
                           : 0u);
  margin.top = margin.bottom = margin.left = margin.right = bw;

  if (decorations & WindowDecorationTitlebar)
    margin.top += style.title_height - bw;

  if (decorations & WindowDecorationHandle)
    margin.bottom += style.handle_height - bw;

  return margin;
}


/*
 * Add specified window to the appropriate window group, creating a
 * new group if necessary.
 */
static BWindowGroup *update_window_group(Window window_group,
                                         Blackbox *blackbox,
                                         BlackboxWindow *win) {
  BWindowGroup *group = win->findWindowGroup();
  if (!group) {
    new BWindowGroup(blackbox, window_group);
    group = win->findWindowGroup();
    assert(group != 0);
  }
  group->addWindow(win);
  return group;
}


/*
 * Calculate the size of the frame window and constrain it to the
 * size specified by the size hints of the client window.
 *
 * 'rect' refers to the geometry of the frame in pixels.
 */
enum Corner {
  TopLeft,
  TopRight,
  BottomLeft,
  BottomRight
};
static bt::Rect constrain(const bt::Rect &rect,
                          const bt::EWMH::Strut &margin,
                          const WMNormalHints &wmnormal,
                          Corner corner) {
  bt::Rect r;

  // 'rect' represents the requested frame size, we need to strip
  // 'margin' off and constrain the client size
  r.setCoords(rect.left() + static_cast<signed>(margin.left),
              rect.top() + static_cast<signed>(margin.top),
              rect.right() - static_cast<signed>(margin.right),
              rect.bottom() - static_cast<signed>(margin.bottom));

  unsigned int dw = r.width(), dh = r.height();

  const unsigned int base_width = (wmnormal.base_width
                                   ? wmnormal.base_width
                                   : wmnormal.min_width),
                    base_height = (wmnormal.base_height
                                   ? wmnormal.base_height
                                   : wmnormal.min_height);

  // fit to min/max size
  if (dw < wmnormal.min_width)
    dw = wmnormal.min_width;
  if (dh < wmnormal.min_height)
    dh = wmnormal.min_height;

  if (dw > wmnormal.max_width)
    dw = wmnormal.max_width;
  if (dh > wmnormal.max_height)
    dh = wmnormal.max_height;

  assert(dw >= base_width && dh >= base_height);

  // fit to size increments
  if (wmnormal.flags & PResizeInc) {
    dw = (((dw - base_width) / wmnormal.width_inc)
          * wmnormal.width_inc) + base_width;
    dh = (((dh - base_height) / wmnormal.height_inc)
          * wmnormal.height_inc) + base_height;
  }

  /*
   * honor aspect ratios (based on twm which is based on uwm)
   *
   * The math looks like this:
   *
   * minAspectX    dwidth     maxAspectX
   * ---------- <= ------- <= ----------
   * minAspectY    dheight    maxAspectY
   *
   * If that is multiplied out, then the width and height are
   * invalid in the following situations:
   *
   * minAspectX * dheight > minAspectY * dwidth
   * maxAspectX * dheight < maxAspectY * dwidth
   *
   */
  if (wmnormal.flags & PAspect) {
    unsigned int delta;
    const unsigned int min_asp_x = wmnormal.min_aspect_x,
                       min_asp_y = wmnormal.min_aspect_y,
                       max_asp_x = wmnormal.max_aspect_x,
                       max_asp_y = wmnormal.max_aspect_y,
                           w_inc = wmnormal.width_inc,
                           h_inc = wmnormal.height_inc;
    if (min_asp_x * dh > min_asp_y * dw) {
      delta = ((min_asp_x * dh / min_asp_y - dw) * w_inc) / w_inc;
      if (dw + delta <= wmnormal.max_width) {
        dw += delta;
      } else {
        delta = ((dh - (dw * min_asp_y) / min_asp_x) * h_inc) / h_inc;
        if (dh - delta >= wmnormal.min_height) dh -= delta;
      }
    }
    if (max_asp_x * dh < max_asp_y * dw) {
      delta = ((max_asp_y * dw / max_asp_x - dh) * h_inc) / h_inc;
      if (dh + delta <= wmnormal.max_height) {
        dh += delta;
      } else {
        delta = ((dw - (dh * max_asp_x) / max_asp_y) * w_inc) / w_inc;
        if (dw - delta >= wmnormal.min_width) dw -= delta;
      }
    }
  }

  r.setSize(dw, dh);

  // add 'margin' back onto 'r'
  r.setCoords(r.left() - margin.left, r.top() - margin.top,
              r.right() + margin.right, r.bottom() + margin.bottom);

  // move 'r' to the specified corner
  int dx = rect.right() - r.right();
  int dy = rect.bottom() - r.bottom();

  switch (corner) {
  case TopLeft:
    // nothing to do
    break;

  case TopRight:
    r.setPos(r.x() + dx, r.y());
    break;

  case BottomLeft:
    r.setPos(r.x(), r.y() + dy);
    break;

  case BottomRight:
    r.setPos(r.x() + dx, r.y() + dy);
    break;
  }

  return r;
}


/*
 * Positions 'rect' according to the client window geometry and window
 * gravity.
 */
static bt::Rect applyGravity(const bt::Rect &rect,
                             const bt::EWMH::Strut &margin,
                             int gravity) {
  bt::Rect r;

  // apply horizontal window gravity
  switch (gravity) {
  default:
  case NorthWestGravity:
  case SouthWestGravity:
  case WestGravity:
    r.setX(rect.x());
    break;

  case NorthGravity:
  case SouthGravity:
  case CenterGravity:
    r.setX(rect.x() - (margin.left + margin.right) / 2);
    break;

  case NorthEastGravity:
  case SouthEastGravity:
  case EastGravity:
    r.setX(rect.x() - (margin.left + margin.right) + 2);
    break;

  case ForgetGravity:
  case StaticGravity:
    r.setX(rect.x() - margin.left);
    break;
  }

  // apply vertical window gravity
  switch (gravity) {
  default:
  case NorthWestGravity:
  case NorthEastGravity:
  case NorthGravity:
    r.setY(rect.y());
    break;

  case CenterGravity:
  case EastGravity:
  case WestGravity:
    r.setY(rect.y() - ((margin.top + margin.bottom) / 2));
    break;

  case SouthWestGravity:
  case SouthEastGravity:
  case SouthGravity:
    r.setY(rect.y() - (margin.bottom + margin.top) + 2);
    break;

  case ForgetGravity:
  case StaticGravity:
    r.setY(rect.y() - margin.top);
    break;
  }

  r.setSize(rect.width() + margin.left + margin.right,
            rect.height() + margin.top + margin.bottom);
  return r;
}


/*
 * The reverse of the applyGravity function.
 *
 * Positions 'rect' according to the frame window geometry and window
 * gravity.
 */
static bt::Rect restoreGravity(const bt::Rect &rect,
                               const bt::EWMH::Strut &margin,
                               int gravity) {
  bt::Rect r;

  // restore horizontal window gravity
  switch (gravity) {
  default:
  case NorthWestGravity:
  case SouthWestGravity:
  case WestGravity:
    r.setX(rect.x());
    break;

  case NorthGravity:
  case SouthGravity:
  case CenterGravity:
    r.setX(rect.x() + (margin.left + margin.right) / 2);
    break;

  case NorthEastGravity:
  case SouthEastGravity:
  case EastGravity:
    r.setX(rect.x() + (margin.left + margin.right) - 2);
    break;

  case ForgetGravity:
  case StaticGravity:
    r.setX(rect.x() + margin.left);
    break;
  }

  // restore vertical window gravity
  switch (gravity) {
  default:
  case NorthWestGravity:
  case NorthEastGravity:
  case NorthGravity:
    r.setY(rect.y());
    break;

  case CenterGravity:
  case EastGravity:
  case WestGravity:
    r.setY(rect.y() + (margin.top + margin.bottom) / 2);
    break;

  case SouthWestGravity:
  case SouthEastGravity:
  case SouthGravity:
    r.setY(rect.y() + (margin.top + margin.bottom) - 2);
    break;

  case ForgetGravity:
  case StaticGravity:
    r.setY(rect.y() + margin.top);
    break;
  }

  r.setSize(rect.width() - margin.left - margin.right,
            rect.height() - margin.top - margin.bottom);
  return r;
}


static bt::ustring readWMName(Blackbox *blackbox, Window window) {
  bt::ustring name;

  if (!blackbox->ewmh().readWMName(window, name) || name.empty()) {
    XTextProperty text_prop;

    if (XGetWMName(blackbox->XDisplay(), window, &text_prop)) {
      name = bt::toUnicode(bt::textPropertyToString(blackbox->XDisplay(),
                                                    text_prop));
      XFree((char *) text_prop.value);
    }
  }

  if (name.empty())
    name = bt::toUnicode("Unnamed");

  return name;
}


static bt::ustring readWMIconName(Blackbox *blackbox, Window window) {
  bt::ustring name;

  if (!blackbox->ewmh().readWMIconName(window, name) || name.empty()) {
    XTextProperty text_prop;
    if (XGetWMIconName(blackbox->XDisplay(), window, &text_prop)) {
      name = bt::toUnicode(bt::textPropertyToString(blackbox->XDisplay(),
                                                    text_prop));
      XFree((char *) text_prop.value);
    }
  }

  if (name.empty())
    return bt::ustring();

  return name;
}


static EWMH readEWMH(const bt::EWMH &bewmh,
                     Window window,
                     int currentWorkspace) {
  EWMH ewmh;
  ewmh.window_type  = WindowTypeNormal;
  ewmh.workspace    = 0; // initialized properly below
  ewmh.modal        = false;
  ewmh.maxv         = false;
  ewmh.maxh         = false;
  ewmh.shaded       = false;
  ewmh.skip_taskbar = false;
  ewmh.skip_pager   = false;
  ewmh.hidden       = false;
  ewmh.fullscreen   = false;
  ewmh.above        = false;
  ewmh.below        = false;

  // note: wm_name and wm_icon_name are read separately

  bool ret;

  bt::EWMH::AtomList atoms;
  ret = bewmh.readWMWindowType(window, atoms);
  if (ret) {
    bt::EWMH::AtomList::iterator it = atoms.begin(), end = atoms.end();
    for (; it != end; ++it) {
      if (bewmh.isSupportedWMWindowType(*it)) {
        ewmh.window_type = ::window_type_from_atom(bewmh, *it);
        break;
      }
    }
  }

  atoms.clear();
  ret = bewmh.readWMState(window, atoms);
  if (ret) {
    bt::EWMH::AtomList::iterator it = atoms.begin(), end = atoms.end();
    for (; it != end; ++it) {
      Atom state = *it;
      if (state == bewmh.wmStateModal()) {
        ewmh.modal = true;
      } else if (state == bewmh.wmStateMaximizedVert()) {
        ewmh.maxv = true;
      } else if (state == bewmh.wmStateMaximizedHorz()) {
        ewmh.maxh = true;
      } else if (state == bewmh.wmStateShaded()) {
        ewmh.shaded = true;
      } else if (state == bewmh.wmStateSkipTaskbar()) {
        ewmh.skip_taskbar = true;
      } else if (state == bewmh.wmStateSkipPager()) {
        ewmh.skip_pager = true;
      } else if (state == bewmh.wmStateHidden()) {
        /*
          ignore _NET_WM_STATE_HIDDEN if present, the wm sets this
          state, not the application
        */
      } else if (state == bewmh.wmStateFullscreen()) {
        ewmh.fullscreen = true;
      } else if (state == bewmh.wmStateAbove()) {
        ewmh.above = true;
      } else if (state == bewmh.wmStateBelow()) {
        ewmh.below = true;
      }
    }
  }

  switch (ewmh.window_type) {
  case WindowTypeDesktop:
  case WindowTypeDock:
    // these types should occupy all workspaces by default
    ewmh.workspace = bt::BSENTINEL;
    break;

  default:
    if (!bewmh.readWMDesktop(window, ewmh.workspace))
      ewmh.workspace = currentWorkspace;
    break;
  } //switch

  return ewmh;
}


/*
 * Returns the MotifWM hints for the specified window.
 */
static MotifHints readMotifWMHints(Blackbox *blackbox, Window window) {
  MotifHints motif;
  motif.decorations = AllWindowDecorations;
  motif.functions   = AllWindowFunctions;

  /*
    this structure only contains 3 elements, even though the Motif 2.0
    structure contains 5, because we only use the first 3
  */
  struct PropMotifhints {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
  };
  static const unsigned int PROP_MWM_HINTS_ELEMENTS = 3u;
  enum { // MWM flags
    MWM_HINTS_FUNCTIONS   = 1<<0,
    MWM_HINTS_DECORATIONS = 1<<1
  };
  enum { // MWM functions
    MWM_FUNC_ALL      = 1<<0,
    MWM_FUNC_RESIZE   = 1<<1,
    MWM_FUNC_MOVE     = 1<<2,
    MWM_FUNC_MINIMIZE = 1<<3,
    MWM_FUNC_MAXIMIZE = 1<<4,
    MWM_FUNC_CLOSE    = 1<<5
  };
  enum { // MWM decorations
    MWM_DECOR_ALL      = 1<<0,
    MWM_DECOR_BORDER   = 1<<1,
    MWM_DECOR_RESIZEH  = 1<<2,
    MWM_DECOR_TITLE    = 1<<3,
    MWM_DECOR_MENU     = 1<<4,
    MWM_DECOR_MINIMIZE = 1<<5,
    MWM_DECOR_MAXIMIZE = 1<<6
  };

  Atom atom_return;
  PropMotifhints *prop = 0;
  int format;
  unsigned long num, len;
  int ret = XGetWindowProperty(blackbox->XDisplay(), window,
                               blackbox->motifWmHintsAtom(), 0,
                               PROP_MWM_HINTS_ELEMENTS, False,
                               blackbox->motifWmHintsAtom(), &atom_return,
                               &format, &num, &len,
                               (unsigned char **) &prop);

  if (ret != Success || !prop || num != PROP_MWM_HINTS_ELEMENTS) {
    if (prop) XFree(prop);
    return motif;
  }

  if (prop->flags & MWM_HINTS_FUNCTIONS) {
    if (prop->functions & MWM_FUNC_ALL) {
      motif.functions = AllWindowFunctions;
    } else {
      // default to the functions that cannot be set through
      // _MOTIF_WM_HINTS
      motif.functions = (WindowFunctionShade
                         | WindowFunctionChangeWorkspace
                         | WindowFunctionChangeLayer
                         | WindowFunctionFullScreen);

      if (prop->functions & MWM_FUNC_RESIZE)
        motif.functions |= WindowFunctionResize;
      if (prop->functions & MWM_FUNC_MOVE)
        motif.functions |= WindowFunctionMove;
      if (prop->functions & MWM_FUNC_MINIMIZE)
        motif.functions |= WindowFunctionIconify;
      if (prop->functions & MWM_FUNC_MAXIMIZE)
        motif.functions |= WindowFunctionMaximize;
      if (prop->functions & MWM_FUNC_CLOSE)
        motif.functions |= WindowFunctionClose;
    }
  }

  if (prop->flags & MWM_HINTS_DECORATIONS) {
    if (prop->decorations & MWM_DECOR_ALL) {
      motif.decorations = AllWindowDecorations;
    } else {
      motif.decorations = NoWindowDecorations;

      if (prop->decorations & MWM_DECOR_BORDER)
        motif.decorations |= WindowDecorationBorder;
      if (prop->decorations & MWM_DECOR_RESIZEH) {
        motif.decorations |= (WindowDecorationHandle |
                              WindowDecorationGrip);
      }
      if (prop->decorations & MWM_DECOR_TITLE) {
        motif.decorations |= (WindowDecorationTitlebar |
                              WindowDecorationClose);
      }
      if (prop->decorations & MWM_DECOR_MINIMIZE)
        motif.decorations |= WindowDecorationIconify;
      if (prop->decorations & MWM_DECOR_MAXIMIZE)
        motif.decorations |= WindowDecorationMaximize;
    }
  }

  XFree(prop);

  return motif;
}


/*
 * Returns the value of the WM_HINTS property.  If the property is not
 * set, a set of default values is returned instead.
 */
static WMHints readWMHints(Blackbox *blackbox, Window window) {
  WMHints wmh;
  wmh.accept_focus = false;
  wmh.window_group = None;
  wmh.initial_state = NormalState;

  XWMHints *wmhint = XGetWMHints(blackbox->XDisplay(), window);
  if (!wmhint) return wmh;

  if (wmhint->flags & InputHint)
    wmh.accept_focus = (wmhint->input == True);
  if (wmhint->flags & StateHint)
    wmh.initial_state = wmhint->initial_state;
  if (wmhint->flags & WindowGroupHint)
    wmh.window_group = wmhint->window_group;

  XFree(wmhint);

  return wmh;
}


/*
 * Returns the value of the WM_NORMAL_HINTS property.  If the property
 * is not set, a set of default values is returned instead.
 */
static WMNormalHints readWMNormalHints(Blackbox *blackbox,
                                       Window window,
                                       const bt::ScreenInfo &screenInfo) {
  WMNormalHints wmnormal;
  wmnormal.flags = 0;
  wmnormal.min_width    = wmnormal.min_height   = 1u;
  wmnormal.width_inc    = wmnormal.height_inc   = 1u;
  wmnormal.min_aspect_x = wmnormal.min_aspect_y = 1u;
  wmnormal.max_aspect_x = wmnormal.max_aspect_y = 1u;
  wmnormal.base_width   = wmnormal.base_height  = 0u;
  wmnormal.win_gravity  = NorthWestGravity;

  /*
    use the full screen, not the strut modified size. otherwise when
    the availableArea changes max_width/height will be incorrect and
    lead to odd rendering bugs.
  */
  const bt::Rect &rect = screenInfo.rect();
  wmnormal.max_width = rect.width();
  wmnormal.max_height = rect.height();

  XSizeHints sizehint;
  long unused;
  if (! XGetWMNormalHints(blackbox->XDisplay(), window, &sizehint, &unused))
    return wmnormal;

  wmnormal.flags = sizehint.flags;

  if (sizehint.flags & PMinSize) {
    if (sizehint.min_width > 0)
      wmnormal.min_width  = sizehint.min_width;
    if (sizehint.min_height > 0)
      wmnormal.min_height = sizehint.min_height;
  }

  if (sizehint.flags & PMaxSize) {
    if (sizehint.max_width > static_cast<signed>(wmnormal.min_width))
      wmnormal.max_width  = sizehint.max_width;
    else
      wmnormal.max_width  = wmnormal.min_width;

    if (sizehint.max_height > static_cast<signed>(wmnormal.min_height))
      wmnormal.max_height = sizehint.max_height;
    else
      wmnormal.max_height = wmnormal.min_height;
  }

  if (sizehint.flags & PResizeInc) {
    wmnormal.width_inc  = sizehint.width_inc;
    wmnormal.height_inc = sizehint.height_inc;
  }

  if (sizehint.flags & PAspect) {
    wmnormal.min_aspect_x = sizehint.min_aspect.x;
    wmnormal.min_aspect_y = sizehint.min_aspect.y;
    wmnormal.max_aspect_x = sizehint.max_aspect.x;
    wmnormal.max_aspect_y = sizehint.max_aspect.y;
  }

  if (sizehint.flags & PBaseSize) {
    wmnormal.base_width  = sizehint.base_width;
    wmnormal.base_height = sizehint.base_height;

    // sanity checks
    wmnormal.min_width  = std::max(wmnormal.min_width,  wmnormal.base_width);
    wmnormal.min_height = std::max(wmnormal.min_height, wmnormal.base_height);
  }

  if (sizehint.flags & PWinGravity)
    wmnormal.win_gravity = sizehint.win_gravity;

  return wmnormal;
}


/*
 * Retrieve which Window Manager Protocols are supported by the client
 * window.
 */
static WMProtocols readWMProtocols(Blackbox *blackbox,
                                   Window window) {
  WMProtocols protocols;
  protocols.wm_delete_window = false;
  protocols.wm_take_focus    = false;

  Atom *proto;
  int num_return = 0;

  if (XGetWMProtocols(blackbox->XDisplay(), window,
                      &proto, &num_return)) {
    for (int i = 0; i < num_return; ++i) {
      if (proto[i] == blackbox->wmDeleteWindowAtom()) {
        protocols.wm_delete_window = true;
      } else if (proto[i] == blackbox->wmTakeFocusAtom()) {
        protocols.wm_take_focus = true;
      }
    }
    XFree(proto);
  }

  return protocols;
}


/*
 * Reads the value of the WM_TRANSIENT_FOR property and returns a
 * pointer to the transient parent for this window.  If the
 * WM_TRANSIENT_FOR is missing or invalid, this function returns 0.
 *
 * 'client.wmhints' should be properly updated before calling this
 * function.
 *
 * Note: a return value of ~0ul signifies a window that should be
 * transient but has no discernible parent.
 */
static Window readTransientInfo(Blackbox *blackbox,
                                Window window,
                                const bt::ScreenInfo &screenInfo,
                                const WMHints &wmhints) {
  Window trans_for = None;

  if (!XGetTransientForHint(blackbox->XDisplay(), window, &trans_for)) {
    // WM_TRANSIENT_FOR hint not set
    return 0;
  }

  if (trans_for == window) {
    // wierd client... treat this window as a normal window
    return 0;
  }

  if (trans_for == None || trans_for == screenInfo.rootWindow()) {
    /*
      this is a violation of the ICCCM, yet the EWMH allows this as a
      way to signify a group transient.
    */
    trans_for = wmhints.window_group;
  }

  return trans_for;
}


static bool readState(unsigned long &current_state,
                      Blackbox *blackbox,
                      Window window) {
  current_state = NormalState;

  Atom atom_return;
  bool ret = false;
  int foo;
  unsigned long *state, ulfoo, nitems;

  if ((XGetWindowProperty(blackbox->XDisplay(), window,
                          blackbox->wmStateAtom(),
                          0l, 2l, False, blackbox->wmStateAtom(),
                          &atom_return, &foo, &nitems, &ulfoo,
                          (unsigned char **) &state) != Success) ||
      (! state)) {
    return false;
  }

  if (nitems >= 1) {
    current_state = static_cast<unsigned long>(state[0]);
    ret = true;
  }

  XFree((void *) state);

  return ret;
}


static void clearState(Blackbox *blackbox, Window window) {
  XDeleteProperty(blackbox->XDisplay(), window, blackbox->wmStateAtom());

  const bt::EWMH& ewmh = blackbox->ewmh();
  ewmh.removeProperty(window, ewmh.wmDesktop());
  ewmh.removeProperty(window, ewmh.wmState());
  ewmh.removeProperty(window, ewmh.wmAllowedActions());
  ewmh.removeProperty(window, ewmh.wmVisibleName());
  ewmh.removeProperty(window, ewmh.wmVisibleIconName());
}


/*
 * Initializes the class with default values/the window's set initial values.
 */
BlackboxWindow::BlackboxWindow(Blackbox *b, Window w, BScreen *s) {
  // fprintf(stderr, "BlackboxWindow size: %d bytes\n",
  //         sizeof(BlackboxWindow));

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::BlackboxWindow(): creating 0x%lx\n", w);
#endif // DEBUG

  /*
    set timer to zero... it is initialized properly later, so we check
    if timer is zero in the destructor, and assume that the window is not
    fully constructed if timer is zero...
  */
  timer = (bt::Timer*) 0;
  blackbox = b;
  client.window = w;
  _screen = s;
  lastButtonPressTime = 0;

  /*
    the server needs to be grabbed here to prevent client's from sending
    events while we are in the process of managing their window.
    We hold the grab until after we are done moving the window around.
  */

  blackbox->XGrabServer();

  // fetch client size and placement
  XWindowAttributes wattrib;
  if (! XGetWindowAttributes(blackbox->XDisplay(),
                             client.window, &wattrib) ||
      ! wattrib.screen || wattrib.override_redirect) {
#ifdef    DEBUG
    fprintf(stderr,
            "BlackboxWindow::BlackboxWindow(): XGetWindowAttributes failed\n");
#endif // DEBUG

    blackbox->XUngrabServer();
    delete this;
    return;
  }

  // set the eventmask early in the game so that we make sure we get
  // all the events we are interested in
  XSetWindowAttributes attrib_set;
  attrib_set.event_mask = ::client_window_event_mask;
  attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask |
                                     ButtonMotionMask;
  XChangeWindowAttributes(blackbox->XDisplay(), client.window,
                          CWEventMask|CWDontPropagate, &attrib_set);

  client.colormap = wattrib.colormap;
  window_number = bt::BSENTINEL;
  client.strut = 0;
  /*
    set the initial size and location of client window (relative to the
    _root window_). This position is the reference point used with the
    window's gravity to find the window's initial position.
  */
  client.rect.setRect(wattrib.x, wattrib.y, wattrib.width, wattrib.height);
  client.premax = client.rect;
  client.old_bw = wattrib.border_width;
  client.current_state = NormalState;

  frame.window = frame.plate = frame.title = frame.handle = None;
  frame.close_button = frame.iconify_button = frame.maximize_button = None;
  frame.right_grip = frame.left_grip = None;
  frame.utitle = frame.ftitle = frame.uhandle = frame.fhandle = None;
  frame.ulabel = frame.flabel = frame.ubutton = frame.fbutton = None;
  frame.pbutton = frame.ugrip = frame.fgrip = None;

  timer = new bt::Timer(blackbox, this);
  timer->setTimeout(blackbox->resource().autoRaiseDelay());

  client.title = ::readWMName(blackbox, client.window);
  client.icon_title = ::readWMIconName(blackbox, client.window);

  // get size, aspect, minimum/maximum size, ewmh and other hints set
  // by the client
  client.ewmh = ::readEWMH(blackbox->ewmh(), client.window,
                           _screen->currentWorkspace());
  client.motif = ::readMotifWMHints(blackbox, client.window);
  client.wmhints = ::readWMHints(blackbox, client.window);
  client.wmnormal = ::readWMNormalHints(blackbox, client.window,
                                        _screen->screenInfo());
  client.wmprotocols = ::readWMProtocols(blackbox, client.window);
  client.transient_for = ::readTransientInfo(blackbox, client.window,
                                             _screen->screenInfo(),
                                             client.wmhints);

  if (client.wmhints.window_group != None)
    (void) ::update_window_group(client.wmhints.window_group, blackbox, this);

  if (isTransient()) {
    // add ourselves to our transient_for
    BlackboxWindow *win = findTransientFor();
    if (win) {
      win->addTransient(this);
      client.ewmh.workspace = win->workspace();
      setLayer(win->layer());
    } else if (isGroupTransient()) {
      BWindowGroup *group = findWindowGroup();
      if (group)
        group->addTransient(this);
    } else {
      // broken client
      client.transient_for = 0;
    }
  }

  client.state.visible = false;
  client.state.iconic = false;
  client.state.moving = false;
  client.state.resizing = false;
  client.state.focused = false;

  switch (windowType()) {
  case WindowTypeDesktop:
    setLayer(StackingList::LayerDesktop);
    break;

  case WindowTypeDock:
    setLayer(StackingList::LayerAbove);
    // fallthrough intended

  default:
    if (client.ewmh.above)
      setLayer(StackingList::LayerAbove);
    else if (client.ewmh.below)
      setLayer(StackingList::LayerBelow);
    break;
  } // switch

  ::update_decorations(client.decorations,
                       client.functions,
                       isTransient(),
                       client.ewmh,
                       client.motif,
                       client.wmnormal,
                       client.wmprotocols);

  // sanity checks
  if (client.wmhints.initial_state == IconicState
      && !hasWindowFunction(WindowFunctionIconify))
    client.wmhints.initial_state = NormalState;
  if (isMaximized() && !hasWindowFunction(WindowFunctionMaximize))
    client.ewmh.maxv = client.ewmh.maxh = false;
  if (isFullScreen() && !hasWindowFunction(WindowFunctionFullScreen))
    client.ewmh.fullscreen = false;

  bt::EWMH::Strut strut;
  if (blackbox->ewmh().readWMStrut(client.window, &strut)) {
    client.strut = new bt::EWMH::Strut;
    *client.strut = strut;
    _screen->addStrut(client.strut);
  }

  frame.window = createToplevelWindow();
  blackbox->insertEventHandler(frame.window, this);

  frame.plate = createChildWindow(frame.window, NoEventMask);
  blackbox->insertEventHandler(frame.plate, this);

  if (client.decorations & WindowDecorationTitlebar)
    createTitlebar();

  if (client.decorations & WindowDecorationHandle)
    createHandle();

  // apply the size and gravity to the frame
  const WindowStyle &style = _screen->resource().windowStyle();
  frame.margin = ::update_margin(client.decorations, style);
  frame.rect = ::applyGravity(client.rect,
                              frame.margin,
                              client.wmnormal.win_gravity);

  associateClientWindow();

  blackbox->insertEventHandler(client.window, this);
  blackbox->insertWindow(client.window, this);
  blackbox->insertWindow(frame.plate, this);

  // preserve the window's initial state on first map, and its current
  // state across a restart
  if (!readState(client.current_state, blackbox, client.window))
    client.current_state = client.wmhints.initial_state;

  if (client.state.iconic) {
    // prepare the window to be iconified
    client.current_state = IconicState;
    client.state.iconic = False;
  } else if (workspace() != bt::BSENTINEL &&
             workspace() != _screen->currentWorkspace()) {
    client.current_state = WithdrawnState;
  }

  blackbox->XUngrabServer();

  grabButtons();

  XMapSubwindows(blackbox->XDisplay(), frame.window);

  if (isFullScreen()) {
    client.ewmh.fullscreen = false; // trick setFullScreen into working
    setFullScreen(true);
  } else {
    if (isMaximized()) {
      remaximize();
    } else {
      bt::Rect r = frame.rect;
      // trick configure into working
      frame.rect = bt::Rect();
      configure(r);
    }

    if (isShaded()) {
      client.ewmh.shaded = false;
      unsigned long save_state = client.current_state;
      setShaded(true);

      /*
        At this point in the life of a window, current_state should only be set
        to IconicState if the window was an *icon*, not if it was shaded.
      */
      if (save_state != IconicState)
        client.current_state = save_state;
    }
  }
}


BlackboxWindow::~BlackboxWindow(void) {
#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::~BlackboxWindow: destroying 0x%lx\n",
          client.window);
#endif // DEBUG

  if (! timer) // window not managed...
    return;

  if (client.state.moving || client.state.resizing) {
    _screen->hideGeometry();
    XUngrabPointer(blackbox->XDisplay(), blackbox->XTime());
  }

  delete timer;

  if (client.strut) {
    _screen->removeStrut(client.strut);
    delete client.strut;
  }

  BWindowGroup *group = findWindowGroup();

  if (isTransient()) {
  // remove ourselves from our transient_for
    BlackboxWindow *win = findTransientFor();
    if (win) {
      win->removeTransient(this);
    } else if (isGroupTransient()) {
      if (group)
        group->removeTransient(this);
    }
    client.transient_for = 0;
  }

  if (group)
    group->removeWindow(this);

  if (frame.title)
    destroyTitlebar();

  if (frame.handle)
    destroyHandle();

  blackbox->removeEventHandler(client.window);
  blackbox->removeWindow(client.window);

  blackbox->removeEventHandler(frame.plate);
  blackbox->removeWindow(frame.plate);
  XDestroyWindow(blackbox->XDisplay(), frame.plate);

  blackbox->removeEventHandler(frame.window);
  XDestroyWindow(blackbox->XDisplay(), frame.window);
}


/*
 * Creates a new top level window, with a given location, size, and border
 * width.
 * Returns: the newly created window
 */
Window BlackboxWindow::createToplevelWindow(void) {
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWColormap | CWOverrideRedirect | CWEventMask;

  attrib_create.colormap = _screen->screenInfo().colormap();
  attrib_create.override_redirect = True;
  attrib_create.event_mask = EnterWindowMask | LeaveWindowMask;

  return XCreateWindow(blackbox->XDisplay(),
                       _screen->screenInfo().rootWindow(), 0, 0, 1, 1, 0,
                       _screen->screenInfo().depth(), InputOutput,
                       _screen->screenInfo().visual(),
                       create_mask, &attrib_create);
}


/*
 * Creates a child window, and optionally associates a given cursor with
 * the new window.
 */
Window BlackboxWindow::createChildWindow(Window parent,
                                         unsigned long event_mask,
                                         Cursor cursor) {
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWEventMask;

  attrib_create.event_mask = event_mask;

  if (cursor) {
    create_mask |= CWCursor;
    attrib_create.cursor = cursor;
  }

  return XCreateWindow(blackbox->XDisplay(), parent, 0, 0, 1, 1, 0,
                       _screen->screenInfo().depth(), InputOutput,
                       _screen->screenInfo().visual(),
                       create_mask, &attrib_create);
}


/*
 * Reparents the client window into the newly created frame.
 *
 * Note: the server must be grabbed before calling this function.
 */
void BlackboxWindow::associateClientWindow(void) {
  XSetWindowBorderWidth(blackbox->XDisplay(), client.window, 0);
  XChangeSaveSet(blackbox->XDisplay(), client.window, SetModeInsert);

  XSelectInput(blackbox->XDisplay(), frame.plate,
               FocusChangeMask | SubstructureRedirectMask);

  XSelectInput(blackbox->XDisplay(), client.window,
               client_window_event_mask & ~StructureNotifyMask);
  XReparentWindow(blackbox->XDisplay(), client.window, frame.plate, 0, 0);
  XSelectInput(blackbox->XDisplay(), client.window, client_window_event_mask);

#ifdef    SHAPE
  if (blackbox->hasShapeExtensions()) {
    XShapeSelectInput(blackbox->XDisplay(), client.window,
                      ShapeNotifyMask);

    Bool shaped = False;
    int foo;
    unsigned int ufoo;

    XShapeQueryExtents(blackbox->XDisplay(), client.window, &shaped,
                       &foo, &foo, &ufoo, &ufoo, &foo, &foo, &foo,
                       &ufoo, &ufoo);
    client.state.shaped = shaped;
  }
#endif // SHAPE
}


void BlackboxWindow::decorate(void) {
  const WindowStyle &style = _screen->resource().windowStyle();
  if (client.decorations & WindowDecorationTitlebar) {
    // render focused button texture
    frame.fbutton =
      bt::PixmapCache::find(_screen->screenNumber(),
                            style.focus.button,
                            style.button_width,
                            style.button_width,
                            frame.fbutton);

    // render unfocused button texture
    frame.ubutton =
      bt::PixmapCache::find(_screen->screenNumber(),
                            style.unfocus.button,
                            style.button_width,
                            style.button_width,
                            frame.ubutton);

    // render pressed button texture
    frame.pbutton =
      bt::PixmapCache::find(_screen->screenNumber(),
                            style.pressed,
                            style.button_width,
                            style.button_width,
                            frame.pbutton);

    // render focused titlebar texture
    frame.ftitle =
      bt::PixmapCache::find(_screen->screenNumber(),
                            style.focus.title,
                            frame.rect.width(),
                            style.title_height,
                            frame.ftitle);

    // render unfocused titlebar texture
    frame.utitle =
      bt::PixmapCache::find(_screen->screenNumber(),
                            style.unfocus.title,
                            frame.rect.width(),
                            style.title_height,
                            frame.utitle);

    // render focused label texture
    frame.flabel =
      bt::PixmapCache::find(_screen->screenNumber(),
                            style.focus.label,
                            frame.label_w,
                            style.label_height,
                            frame.flabel);

    // render unfocused label texture
    frame.ulabel =
      bt::PixmapCache::find(_screen->screenNumber(),
                            style.unfocus.label,
                            frame.label_w,
                            style.label_height,
                            frame.ulabel);
  }

  XSetWindowBorder(blackbox->XDisplay(), frame.plate,
                   style.frame_border.pixel(_screen->screenNumber()));

  if (client.decorations & WindowDecorationHandle) {
    frame.fhandle =
      bt::PixmapCache::find(_screen->screenNumber(),
                            style.focus.handle,
                            frame.rect.width(),
                            style.handle_height,
                            frame.fhandle);

    frame.uhandle =
      bt::PixmapCache::find(_screen->screenNumber(),
                            style.unfocus.handle,
                            frame.rect.width(),
                            style.handle_height,
                            frame.uhandle);
  }

  if (client.decorations & WindowDecorationGrip) {
    frame.fgrip =
      bt::PixmapCache::find(_screen->screenNumber(),
                            style.focus.grip,
                            style.grip_width,
                            style.handle_height,
                            frame.fgrip);

    frame.ugrip =
      bt::PixmapCache::find(_screen->screenNumber(),
                            style.unfocus.grip,
                            style.grip_width,
                            style.handle_height,
                            frame.ugrip);
  }
}


void BlackboxWindow::createHandle(void) {
  frame.handle = createChildWindow(frame.window,
                                   ButtonPressMask | ButtonReleaseMask |
                                   ButtonMotionMask | ExposureMask);
  blackbox->insertEventHandler(frame.handle, this);

  if (client.decorations & WindowDecorationGrip)
    createGrips();
}


void BlackboxWindow::destroyHandle(void) {
  if (frame.left_grip || frame.right_grip)
    destroyGrips();

  if (frame.fhandle) bt::PixmapCache::release(frame.fhandle);
  if (frame.uhandle) bt::PixmapCache::release(frame.uhandle);

  frame.fhandle = frame.uhandle = None;

  blackbox->removeEventHandler(frame.handle);
  XDestroyWindow(blackbox->XDisplay(), frame.handle);
  frame.handle = None;
}


void BlackboxWindow::createGrips(void) {
  frame.left_grip =
    createChildWindow(frame.handle,
                      ButtonPressMask | ButtonReleaseMask |
                      ButtonMotionMask | ExposureMask,
                      blackbox->resource().cursors().resize_bottom_left);
  blackbox->insertEventHandler(frame.left_grip, this);

  frame.right_grip =
    createChildWindow(frame.handle,
                      ButtonPressMask | ButtonReleaseMask |
                      ButtonMotionMask | ExposureMask,
                      blackbox->resource().cursors().resize_bottom_right);
  blackbox->insertEventHandler(frame.right_grip, this);
}


void BlackboxWindow::destroyGrips(void) {
  if (frame.fgrip) bt::PixmapCache::release(frame.fgrip);
  if (frame.ugrip) bt::PixmapCache::release(frame.ugrip);

  frame.fgrip = frame.ugrip = None;

  blackbox->removeEventHandler(frame.left_grip);
  blackbox->removeEventHandler(frame.right_grip);

  XDestroyWindow(blackbox->XDisplay(), frame.left_grip);
  XDestroyWindow(blackbox->XDisplay(), frame.right_grip);
  frame.left_grip = frame.right_grip = None;
}


void BlackboxWindow::createTitlebar(void) {
  frame.title = createChildWindow(frame.window,
                                  ButtonPressMask | ButtonReleaseMask |
                                  ButtonMotionMask | ExposureMask);
  frame.label = createChildWindow(frame.title,
                                  ButtonPressMask | ButtonReleaseMask |
                                  ButtonMotionMask | ExposureMask);
  blackbox->insertEventHandler(frame.title, this);
  blackbox->insertEventHandler(frame.label, this);

  if (client.decorations & WindowDecorationIconify) createIconifyButton();
  if (client.decorations & WindowDecorationMaximize) createMaximizeButton();
  if (client.decorations & WindowDecorationClose) createCloseButton();
}


void BlackboxWindow::destroyTitlebar(void) {
  if (frame.close_button)
    destroyCloseButton();

  if (frame.iconify_button)
    destroyIconifyButton();

  if (frame.maximize_button)
    destroyMaximizeButton();

  if (frame.fbutton) bt::PixmapCache::release(frame.fbutton);
  if (frame.ubutton) bt::PixmapCache::release(frame.ubutton);
  if (frame.pbutton) bt::PixmapCache::release(frame.pbutton);
  if (frame.ftitle)  bt::PixmapCache::release(frame.ftitle);
  if (frame.utitle)  bt::PixmapCache::release(frame.utitle);
  if (frame.flabel)  bt::PixmapCache::release(frame.flabel);
  if (frame.ulabel)  bt::PixmapCache::release(frame.ulabel);

  frame.fbutton = frame.ubutton = frame.pbutton =
   frame.ftitle = frame.utitle =
   frame.flabel = frame.ulabel = None;

  blackbox->removeEventHandler(frame.title);
  blackbox->removeEventHandler(frame.label);

  XDestroyWindow(blackbox->XDisplay(), frame.label);
  XDestroyWindow(blackbox->XDisplay(), frame.title);
  frame.title = frame.label = None;
}


void BlackboxWindow::createCloseButton(void) {
  if (frame.title != None) {
    frame.close_button = createChildWindow(frame.title,
                                           ButtonPressMask |
                                           ButtonReleaseMask |
                                           ButtonMotionMask | ExposureMask);
    blackbox->insertEventHandler(frame.close_button, this);
  }
}


void BlackboxWindow::destroyCloseButton(void) {
  blackbox->removeEventHandler(frame.close_button);
  XDestroyWindow(blackbox->XDisplay(), frame.close_button);
  frame.close_button = None;
}


void BlackboxWindow::createIconifyButton(void) {
  if (frame.title != None) {
    frame.iconify_button = createChildWindow(frame.title,
                                             ButtonPressMask |
                                             ButtonReleaseMask |
                                             ButtonMotionMask | ExposureMask);
    blackbox->insertEventHandler(frame.iconify_button, this);
  }
}


void BlackboxWindow::destroyIconifyButton(void) {
  blackbox->removeEventHandler(frame.iconify_button);
  XDestroyWindow(blackbox->XDisplay(), frame.iconify_button);
  frame.iconify_button = None;
}


void BlackboxWindow::createMaximizeButton(void) {
  if (frame.title != None) {
    frame.maximize_button = createChildWindow(frame.title,
                                              ButtonPressMask |
                                              ButtonReleaseMask |
                                              ButtonMotionMask | ExposureMask);
    blackbox->insertEventHandler(frame.maximize_button, this);
  }
}


void BlackboxWindow::destroyMaximizeButton(void) {
  blackbox->removeEventHandler(frame.maximize_button);
  XDestroyWindow(blackbox->XDisplay(), frame.maximize_button);
  frame.maximize_button = None;
}


void BlackboxWindow::positionButtons(bool redecorate_label) {
  // we need to use signed ints here to detect windows that are too small
  const WindowStyle &style = _screen->resource().windowStyle();
  const int extra = style.title_margin == 0 ?
                    style.focus.button.borderWidth() : 0,
               bw = style.button_width + style.title_margin
                    - extra,
               by = style.title_margin +
                    style.focus.title.borderWidth();
  int lx = by, lw = frame.rect.width() - by;

  if (client.decorations & WindowDecorationIconify) {
    if (frame.iconify_button == None) createIconifyButton();

    XMoveResizeWindow(blackbox->XDisplay(), frame.iconify_button, by, by,
                      style.button_width, style.button_width);
    XMapWindow(blackbox->XDisplay(), frame.iconify_button);

    lx += bw;
    lw -= bw;
  } else if (frame.iconify_button) {
    destroyIconifyButton();
  }

  int bx = frame.rect.width() - bw
           - style.focus.title.borderWidth() - extra;

  if (client.decorations & WindowDecorationClose) {
    if (frame.close_button == None) createCloseButton();

    XMoveResizeWindow(blackbox->XDisplay(), frame.close_button, bx, by,
                      style.button_width, style.button_width);
    XMapWindow(blackbox->XDisplay(), frame.close_button);

    bx -= bw;
    lw -= bw;
  } else if (frame.close_button) {
    destroyCloseButton();
  }

  if (client.decorations & WindowDecorationMaximize) {
    if (frame.maximize_button == None) createMaximizeButton();

    XMoveResizeWindow(blackbox->XDisplay(), frame.maximize_button, bx, by,
                      style.button_width, style.button_width);
    XMapWindow(blackbox->XDisplay(), frame.maximize_button);

    bx -= bw;
    lw -= bw;
  } else if (frame.maximize_button) {
    destroyMaximizeButton();
  }

  if (lw > by) {
    frame.label_w = lw - by;
    XMoveResizeWindow(blackbox->XDisplay(), frame.label, lx, by,
                      frame.label_w, style.label_height);
    XMapWindow(blackbox->XDisplay(), frame.label);

    if (redecorate_label) {
      frame.flabel =
        bt::PixmapCache::find(_screen->screenNumber(),
                              style.focus.label,
                              frame.label_w, style.label_height,
                              frame.flabel);
      frame.ulabel =
        bt::PixmapCache::find(_screen->screenNumber(),
                              style.unfocus.label,
                              frame.label_w, style.label_height,
                              frame.ulabel);
    }

    const bt::ustring ellided =
      bt::ellideText(client.title, frame.label_w, bt::toUnicode("..."),
                     _screen->screenNumber(), style.font);

    if (ellided != client.visible_title) {
      client.visible_title = ellided;
      blackbox->ewmh().setWMVisibleName(client.window, client.visible_title);
    }
  } else {
    frame.label_w = 1;
    XUnmapWindow(blackbox->XDisplay(), frame.label);
  }

  redrawLabel();
  redrawAllButtons();
}


void BlackboxWindow::reconfigure(void) {
  const WindowStyle &style = _screen->resource().windowStyle();
  if (isMaximized()) {
    // update the frame margin in case the style has changed
    frame.margin = ::update_margin(client.decorations, style);

    // make sure maximized windows have the correct size after a style
    // change
    remaximize();
  } else {
    // get the client window geometry as if it was unmanaged
    bt::Rect r = frame.rect;
    if (client.ewmh.shaded) {
      r.setHeight(client.rect.height() + frame.margin.top
                  + frame.margin.bottom);
    }
    r = ::restoreGravity(r, frame.margin, client.wmnormal.win_gravity);

    // update the frame margin in case the style has changed
    frame.margin = ::update_margin(client.decorations, style);

    // get the frame window geometry from the client window geometry
    // calculated above
    r = ::applyGravity(r, frame.margin, client.wmnormal.win_gravity);
    if (client.ewmh.shaded) {
      frame.rect = r;

      positionWindows();
      decorate();

      // keep the window shaded
      frame.rect.setHeight(style.title_height);
      XResizeWindow(blackbox->XDisplay(), frame.window,
                    frame.rect.width(), frame.rect.height());
    } else {
      // trick configure into working
      frame.rect = bt::Rect();
      configure(r);
    }
  }

  ungrabButtons();
  grabButtons();
}


void BlackboxWindow::grabButtons(void) {
  if (blackbox->resource().focusModel() == ClickToFocusModel
      || blackbox->resource().clickRaise())
    // grab button 1 for changing focus/raising
    blackbox->grabButton(Button1, 0, frame.plate, True, ButtonPressMask,
                         GrabModeSync, GrabModeSync, frame.plate, None,
                         blackbox->resource().allowScrollLock());

  if (hasWindowFunction(WindowFunctionMove))
    blackbox->grabButton(Button1, Mod1Mask, frame.window, True,
                         ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                         GrabModeAsync, frame.window,
                         blackbox->resource().cursors().move,
                         blackbox->resource().allowScrollLock());
  if (hasWindowFunction(WindowFunctionResize))
    blackbox->grabButton(Button3, Mod1Mask, frame.window, True,
                         ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                         GrabModeAsync, frame.window,
                         None, blackbox->resource().allowScrollLock());
  // alt+middle lowers the window
  blackbox->grabButton(Button2, Mod1Mask, frame.window, True,
                       ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
                       frame.window, None,
                       blackbox->resource().allowScrollLock());
}


void BlackboxWindow::ungrabButtons(void) {
  blackbox->ungrabButton(Button1, 0, frame.plate);
  blackbox->ungrabButton(Button1, Mod1Mask, frame.window);
  blackbox->ungrabButton(Button2, Mod1Mask, frame.window);
  blackbox->ungrabButton(Button3, Mod1Mask, frame.window);
}


void BlackboxWindow::positionWindows(void) {
  const WindowStyle &style = _screen->resource().windowStyle();
  const unsigned int bw = (hasWindowDecoration(WindowDecorationBorder)
                           ? style.frame_border_width
                           : 0);

  XMoveResizeWindow(blackbox->XDisplay(), frame.plate,
                    frame.margin.left - bw,
                    frame.margin.top - bw,
                    client.rect.width(), client.rect.height());
  XSetWindowBorderWidth(blackbox->XDisplay(), frame.plate, bw);
  XMoveResizeWindow(blackbox->XDisplay(), client.window,
                    0, 0, client.rect.width(), client.rect.height());
  // ensure client.rect contains the real location
  client.rect.setPos(frame.rect.left() + frame.margin.left,
                     frame.rect.top() + frame.margin.top);

  if (client.decorations & WindowDecorationTitlebar) {
    if (frame.title == None) createTitlebar();

    XMoveResizeWindow(blackbox->XDisplay(), frame.title,
                      0, 0, frame.rect.width(), style.title_height);

    positionButtons();
    XMapSubwindows(blackbox->XDisplay(), frame.title);
    XMapWindow(blackbox->XDisplay(), frame.title);
  } else if (frame.title) {
    destroyTitlebar();
  }

  if (client.decorations & WindowDecorationHandle) {
    if (frame.handle == None) createHandle();

    // use client.rect here so the value is correct even if shaded
    XMoveResizeWindow(blackbox->XDisplay(), frame.handle,
                      0, client.rect.height() + frame.margin.top,
                      frame.rect.width(), style.handle_height);

    if (client.decorations & WindowDecorationGrip) {
      if (frame.left_grip == None || frame.right_grip == None) createGrips();

      XMoveResizeWindow(blackbox->XDisplay(), frame.left_grip, 0, 0,
                        style.grip_width, style.handle_height);

      const int nx = frame.rect.width() - style.grip_width;
      XMoveResizeWindow(blackbox->XDisplay(), frame.right_grip, nx, 0,
                        style.grip_width, style.handle_height);

      XMapSubwindows(blackbox->XDisplay(), frame.handle);
    } else {
      destroyGrips();
    }

    XMapWindow(blackbox->XDisplay(), frame.handle);
  } else if (frame.handle) {
    destroyHandle();
  }
}


/*
 * This function is responsible for updating both the client and the
 * frame rectangles.  According to the ICCCM a client message is not
 * sent for a resize, only a move.
 */
void BlackboxWindow::configure(int dx, int dy,
                               unsigned int dw, unsigned int dh) {
  bool send_event = ((frame.rect.x() != dx || frame.rect.y() != dy) &&
                     ! client.state.moving);

  if (dw != frame.rect.width() || dh != frame.rect.height()) {
    frame.rect.setRect(dx, dy, dw, dh);

    if (frame.rect.right() <= 0 || frame.rect.bottom() <= 0)
      frame.rect.setPos(0, 0);

    client.rect.setCoords(frame.rect.left() + frame.margin.left,
                          frame.rect.top() + frame.margin.top,
                          frame.rect.right() - frame.margin.right,
                          frame.rect.bottom() - frame.margin.bottom);

#ifdef SHAPE
    if (client.state.shaped)
      configureShape();
#endif // SHAPE

    XMoveResizeWindow(blackbox->XDisplay(), frame.window,
                      frame.rect.x(), frame.rect.y(),
                      frame.rect.width(), frame.rect.height());

    positionWindows();
    decorate();
    redrawWindowFrame();
  } else {
    frame.rect.setPos(dx, dy);

    XMoveWindow(blackbox->XDisplay(), frame.window,
                frame.rect.x(), frame.rect.y());
    /*
      we may have been called just after an opaque window move, so
      even though the old coords match the new ones no ConfigureNotify
      has been sent yet.  There are likely other times when this will
      be relevant as well.
    */
    if (! client.state.moving) send_event = True;
  }

  if (send_event) {
    // if moving, the update and event will occur when the move finishes
    client.rect.setPos(frame.rect.left() + frame.margin.left,
                       frame.rect.top() + frame.margin.top);

    XEvent event;
    event.type = ConfigureNotify;

    event.xconfigure.display = blackbox->XDisplay();
    event.xconfigure.event = client.window;
    event.xconfigure.window = client.window;
    event.xconfigure.x = client.rect.x();
    event.xconfigure.y = client.rect.y();
    event.xconfigure.width = client.rect.width();
    event.xconfigure.height = client.rect.height();
    event.xconfigure.border_width = client.old_bw;
    event.xconfigure.above = frame.window;
    event.xconfigure.override_redirect = False;

    XSendEvent(blackbox->XDisplay(), client.window, False,
               StructureNotifyMask, &event);
  }
}


#ifdef SHAPE
void BlackboxWindow::configureShape(void) {
  XShapeCombineShape(blackbox->XDisplay(), frame.window, ShapeBounding,
                     frame.margin.left, frame.margin.top,
                     client.window, ShapeBounding, ShapeSet);

  int num = 0;
  XRectangle xrect[2];

  const WindowStyle &style = _screen->resource().windowStyle();
  if (client.decorations & WindowDecorationTitlebar) {
    xrect[0].x = xrect[0].y = 0;
    xrect[0].width = frame.rect.width();
    xrect[0].height = style.title_height;
    ++num;
  }

  if (client.decorations & WindowDecorationHandle) {
    xrect[1].x = 0;
    xrect[1].y = client.rect.height() + frame.margin.top;
    xrect[1].width = frame.rect.width();
    xrect[1].height = style.handle_height;
    ++num;
  }

  XShapeCombineRectangles(blackbox->XDisplay(), frame.window,
                          ShapeBounding, 0, 0, xrect, num,
                          ShapeUnion, Unsorted);
}
#endif // SHAPE


void BlackboxWindow::addTransient(BlackboxWindow *win)
{ client.transientList.push_front(win); }


void BlackboxWindow::removeTransient(BlackboxWindow *win)
{ client.transientList.remove(win); }


BlackboxWindow *BlackboxWindow::findTransientFor(void) const {
  BlackboxWindow *win = 0;
  if (isTransient()) {
    win = blackbox->findWindow(client.transient_for);
    if (win && win->_screen != _screen)
      win = 0;
  }
  return win;
}


/*
  walk up to either 1) a non-transient window 2) a group transient,
  watching out for a circular chain

  this function returns zero for non-transient windows
*/
BlackboxWindow *BlackboxWindow::findNonTransientParent(void) const {
  BlackboxWindowList seen;
  seen.push_back(const_cast<BlackboxWindow *>(this));

  BlackboxWindow *w = findTransientFor();
  if (!w)
    return 0;

  while (w->isTransient() && !w->isGroupTransient()) {
    seen.push_back(w);
    BlackboxWindow * const tmp = w->findTransientFor();
    if (!tmp)
      break;
    if (std::find(seen.begin(), seen.end(), tmp) != seen.end()) {
      // circular transient chain
      break;
    }
    w = tmp;
  }
  return w;
}


/*
  Returns a list of all transients.  This is recursive, so it returns
  all transients of transients as well.
*/
BlackboxWindowList BlackboxWindow::buildFullTransientList(void) const {
  BlackboxWindowList all = client.transientList;
  BlackboxWindowList::const_iterator it = client.transientList.begin(),
                                    end = client.transientList.end();
  for (; it != end; ++it) {
    BlackboxWindowList x = (*it)->buildFullTransientList();
    all.splice(all.end(), x);
  }
  return all;
}


BWindowGroup *BlackboxWindow::findWindowGroup(void) const {
  BWindowGroup *group = 0;
  if (client.wmhints.window_group)
    group = blackbox->findWindowGroup(client.wmhints.window_group);
  return group;
}


void BlackboxWindow::setWorkspace(unsigned int new_workspace) {
  client.ewmh.workspace = new_workspace;
  blackbox->ewmh().setWMDesktop(client.window, client.ewmh.workspace);
}


void BlackboxWindow::changeWorkspace(unsigned int new_workspace,
                                     ChangeWorkspaceOption how) {
  if (client.ewmh.workspace == new_workspace)
    return;

  if (isTransient()) {
    BlackboxWindow *win = findTransientFor();
    if (win) {
      if (win->workspace() != new_workspace) {
        win->changeWorkspace(new_workspace, how);
        return;
      }
    }
  } else {
    assert(hasWindowFunction(WindowFunctionChangeWorkspace));
  }

  Workspace *ws;
  if (workspace() != bt::BSENTINEL) {
    ws = _screen->findWorkspace(workspace());
    assert(ws != 0);
    ws->removeWindow(this);
  }

  if (new_workspace != bt::BSENTINEL) {
    ws = _screen->findWorkspace(new_workspace);
    assert(ws != 0);
    ws->addWindow(this);
  }

  switch (how) {
  case StayOnCurrentWorkspace:
    if (isVisible() && workspace() != bt::BSENTINEL
        && workspace() != _screen->currentWorkspace()) {
      hide();
    } else if (!isVisible()
               && (workspace() == bt::BSENTINEL
                   || workspace() == _screen->currentWorkspace())) {
      show();
    }
    break;

  case SwitchToNewWorkspace:
    /*
      we will change to the new workspace soon, so force this window
      to be visible
    */
    show();
    break;
  }

  // change workspace on all transients
  if (!client.transientList.empty()) {
    BlackboxWindowList::iterator it = client.transientList.begin(),
                                end = client.transientList.end();
    for (; it != end; ++it)
      (*it)->changeWorkspace(new_workspace, how);
  }
}


void BlackboxWindow::changeLayer(StackingList::Layer new_layer) {
  if (layer() == new_layer)
    return;

  bool restack = false;
  if (isTransient()) {
    BlackboxWindow *win = findTransientFor();
    if (win) {
      if (win->layer() != new_layer) {
        win->changeLayer(new_layer);
        return;
      } else {
        restack = true;
      }
    }
  } else {
    assert(hasWindowFunction(WindowFunctionChangeLayer));
    restack = true;
  }

  _screen->stackingList().changeLayer(this, new_layer);

  if (!client.transientList.empty()) {
    BlackboxWindowList::iterator it = client.transientList.begin();
    const BlackboxWindowList::iterator end = client.transientList.end();
    for (; it != end; ++it)
      (*it)->changeLayer(new_layer);
  }

  if (restack)
    _screen->restackWindows();
}


bool BlackboxWindow::setInputFocus(void) {
  if (!isVisible())
    return false;
  if (client.state.focused)
    return true;

  switch (windowType()) {
  case WindowTypeDock:
    /*
      Many docks have auto-hide features similar to the toolbar and
      slit... we don't want these things to be moved to the center of
      the screen when switching to an empty workspace.
    */
    break;

  default:
    {  const bt::Rect &scr = _screen->screenInfo().rect();
      if (!frame.rect.intersects(scr)) {
        // client is outside the screen, move it to the center
        configure(scr.x() + (scr.width() - frame.rect.width()) / 2,
                  scr.y() + (scr.height() - frame.rect.height()) / 2,
                  frame.rect.width(), frame.rect.height());
      }
      break;
    }
  }

  /*
    pass focus to any modal transients, giving modal group transients
    higher priority
  */
  BWindowGroup *group = findWindowGroup();
  if (group && !group->transients().empty()) {
    BlackboxWindowList::const_iterator it = group->transients().begin(),
                                      end = group->transients().end();
    for (; it != end; ++it) {
      BlackboxWindow * const tmp = *it;
      if (!tmp->isVisible() || !tmp->isModal())
        continue;
      if (tmp == this) {
        // we are the newest modal group transient
        break;
      }
      if (isTransient()) {
        if (tmp == findNonTransientParent()) {
          // we are a transient of the modal group transient
          break;
        }
      }
      return tmp->setInputFocus();
    }
  }

  if (!client.transientList.empty()) {
    BlackboxWindowList::const_iterator it = client.transientList.begin(),
                                      end = client.transientList.end();
    for (; it != end; ++it) {
      BlackboxWindow * const tmp = *it;
      if (tmp->isVisible() && tmp->isModal())
        return tmp->setInputFocus();
    }
  }

  switch (windowType()) {
  case WindowTypeDock:
    return false;
  default:
    break;
  }

  XSetInputFocus(blackbox->XDisplay(), client.window,
                 RevertToPointerRoot, blackbox->XTime());

  if (client.wmprotocols.wm_take_focus) {
    XEvent ce;
    ce.xclient.type = ClientMessage;
    ce.xclient.message_type = blackbox->wmProtocolsAtom();
    ce.xclient.display = blackbox->XDisplay();
    ce.xclient.window = client.window;
    ce.xclient.format = 32;
    ce.xclient.data.l[0] = blackbox->wmTakeFocusAtom();
    ce.xclient.data.l[1] = blackbox->XTime();
    ce.xclient.data.l[2] = 0l;
    ce.xclient.data.l[3] = 0l;
    ce.xclient.data.l[4] = 0l;
    XSendEvent(blackbox->XDisplay(), client.window, False, NoEventMask, &ce);
  }

  return true;
}


void BlackboxWindow::show(void) {
  if (client.state.visible)
    return;

  if (client.state.iconic)
    _screen->removeIcon(this);

  client.state.iconic = false;
  client.state.visible = true;
  setState(isShaded() ? IconicState : NormalState);

  XMapWindow(blackbox->XDisplay(), client.window);
  XMapSubwindows(blackbox->XDisplay(), frame.window);
  XMapWindow(blackbox->XDisplay(), frame.window);

  if (!client.transientList.empty()) {
    BlackboxWindowList::iterator it = client.transientList.begin(),
                                end = client.transientList.end();
    for (; it != end; ++it)
      (*it)->show();
  }

#ifdef DEBUG
  int real_x, real_y;
  Window child;
  XTranslateCoordinates(blackbox->XDisplay(), client.window,
                        _screen->screenInfo().rootWindow(),
                        0, 0, &real_x, &real_y, &child);
  fprintf(stderr, "%s -- assumed: (%d, %d), real: (%d, %d)\n", title().c_str(),
          client.rect.left(), client.rect.top(), real_x, real_y);
  assert(client.rect.left() == real_x && client.rect.top() == real_y);
#endif
}


void BlackboxWindow::hide(void) {
  if (!client.state.visible)
    return;

  client.state.visible = false;
  setState(client.state.iconic ? IconicState : client.current_state);

  XUnmapWindow(blackbox->XDisplay(), frame.window);

  /*
   * we don't want this XUnmapWindow call to generate an UnmapNotify
   * event, so we need to clear the event mask on client.window for a
   * split second.  HOWEVER, since X11 is asynchronous, the window
   * could be destroyed in that split second, leaving us with a ghost
   * window... so, we need to do this while the X server is grabbed
   */
  blackbox->XGrabServer();
  XSelectInput(blackbox->XDisplay(), client.window,
               client_window_event_mask & ~StructureNotifyMask);
  XUnmapWindow(blackbox->XDisplay(), client.window);
  XSelectInput(blackbox->XDisplay(), client.window, client_window_event_mask);
  blackbox->XUngrabServer();
}


void BlackboxWindow::close(void) {
  assert(hasWindowFunction(WindowFunctionClose));

  XEvent ce;
  ce.xclient.type = ClientMessage;
  ce.xclient.message_type = blackbox->wmProtocolsAtom();
  ce.xclient.display = blackbox->XDisplay();
  ce.xclient.window = client.window;
  ce.xclient.format = 32;
  ce.xclient.data.l[0] = blackbox->wmDeleteWindowAtom();
  ce.xclient.data.l[1] = blackbox->XTime();
  ce.xclient.data.l[2] = 0l;
  ce.xclient.data.l[3] = 0l;
  ce.xclient.data.l[4] = 0l;
  XSendEvent(blackbox->XDisplay(), client.window, False, NoEventMask, &ce);
}


void BlackboxWindow::activate(void) {
  if (workspace() != bt::BSENTINEL
      && workspace() != _screen->currentWorkspace())
    _screen->setCurrentWorkspace(workspace());
  if (client.state.iconic)
    show();
  if (client.ewmh.shaded)
    setShaded(false);
  if (setInputFocus())
    _screen->raiseWindow(this);
}


void BlackboxWindow::iconify(void) {
  if (client.state.iconic)
    return;

  if (isTransient()) {
    BlackboxWindow *win = findTransientFor();
    if (win) {
      if (!win->isIconic()) {
        win->iconify();
        return;
      }
    }
  } else {
    assert(hasWindowFunction(WindowFunctionIconify));
  }

  _screen->addIcon(this);

  client.state.iconic = true;
  hide();

  // iconify all transients
  if (!client.transientList.empty()) {
    BlackboxWindowList::iterator it = client.transientList.begin(),
                                end = client.transientList.end();
    for (; it != end; ++it)
      (*it)->iconify();
  }
}


void BlackboxWindow::maximize(unsigned int button) {
  assert(hasWindowFunction(WindowFunctionMaximize));

  // any maximize operation always unshades
  client.ewmh.shaded = false;
  frame.rect.setHeight(client.rect.height() + frame.margin.top
                       + frame.margin.bottom);

  if (isMaximized()) {
    // restore from maximized
    client.ewmh.maxh = client.ewmh.maxv = false;

    if (!isFullScreen()) {
      /*
        when a resize is begun, maximize(0) is called to clear any
        maximization flags currently set.  Otherwise it still thinks
        it is maximized.  so we do not need to call configure()
        because resizing will handle it
      */
      if (! client.state.resizing) {
        bt::Rect r = ::applyGravity(client.premax,
                                    frame.margin,
                                    client.wmnormal.win_gravity);
        r = ::constrain(r, frame.margin, client.wmnormal, TopLeft);
        // trick configure into working
        frame.rect = bt::Rect();
        configure(r);
      }

      redrawAllButtons(); // in case it is not called in configure()
    }

    updateEWMHState();
    updateEWMHAllowedActions();
    return;
  }

  switch (button) {
  case 1:
    client.ewmh.maxh = true;
    client.ewmh.maxv = true;
    break;

  case 2:
    client.ewmh.maxh = false;
    client.ewmh.maxv = true;
    break;

  case 3:
    client.ewmh.maxh = true;
    client.ewmh.maxv = false;
    break;

  default:
    assert(0);
    break;
  }

  if (!isFullScreen()) {
    // go go gadget-maximize!
    bt::Rect r = _screen->availableArea();

    if (!client.ewmh.maxh) {
      r.setX(frame.rect.x());
      r.setWidth(frame.rect.width());
    }
    if (!client.ewmh.maxv) {
      r.setY(frame.rect.y());
      r.setHeight(frame.rect.height());
    }

    // store the current frame geometry, so that we can restore it later
    client.premax = ::restoreGravity(frame.rect,
                                     frame.margin,
                                     client.wmnormal.win_gravity);

    r = ::constrain(r, frame.margin, client.wmnormal, TopLeft);
    // trick configure into working
    frame.rect = bt::Rect();
    configure(r);
  }

  updateEWMHState();
  updateEWMHAllowedActions();
}


// re-maximizes the window to take into account availableArea changes
void BlackboxWindow::remaximize(void) {
  if (isShaded()) return;

  unsigned int button = 0u;
  if (client.ewmh.maxv) {
    button = (client.ewmh.maxh) ? 1u : 2u;
  } else if (client.ewmh.maxh) {
    button = (client.ewmh.maxv) ? 1u : 3u;
  }

  // trick maximize() into working
  client.ewmh.maxh = client.ewmh.maxv = false;

  const bt::Rect tmp = client.premax;
  maximize(button);
  client.premax = tmp;
}


void BlackboxWindow::setShaded(bool shaded) {
  assert(hasWindowFunction(WindowFunctionShade));

  if (client.ewmh.shaded == shaded)
    return;

  client.ewmh.shaded = shaded;
  if (!isShaded()) {
    if (isMaximized()) {
      remaximize();
    } else {
      // set the frame rect to the normal size
      frame.rect.setHeight(client.rect.height() + frame.margin.top +
                           frame.margin.bottom);

      XResizeWindow(blackbox->XDisplay(), frame.window,
                    frame.rect.width(), frame.rect.height());
    }

    setState(NormalState);
  } else {
    // set the frame rect to the shaded size
    const WindowStyle &style = _screen->resource().windowStyle();
    frame.rect.setHeight(style.title_height);

    XResizeWindow(blackbox->XDisplay(), frame.window,
                  frame.rect.width(), frame.rect.height());

    setState(IconicState);
  }
}


void BlackboxWindow::setFullScreen(bool b) {
  assert(hasWindowFunction(WindowFunctionFullScreen));

  if (client.ewmh.fullscreen == b)
    return;

  // any fullscreen operation always unshades
  client.ewmh.shaded = false;
  frame.rect.setHeight(client.rect.height() + frame.margin.top
                       + frame.margin.bottom);

  bool refocus = isFocused();
  client.ewmh.fullscreen = b;
  if (isFullScreen()) {
    // go go gadget-fullscreen!
    if (!isMaximized())
      client.premax = ::restoreGravity(frame.rect,
                                       frame.margin,
                                       client.wmnormal.win_gravity);

    // modify decorations, functions and frame margin
    client.decorations = NoWindowDecorations;
    client.functions &= ~(WindowFunctionMove |
                          WindowFunctionResize |
                          WindowFunctionShade);
    const WindowStyle &style = _screen->resource().windowStyle();
    frame.margin = ::update_margin(client.decorations, style);

    bt::Rect r = ::constrain(_screen->screenInfo().rect(),
                             frame.margin,
                             client.wmnormal,
                             TopLeft);
    // trick configure() into working
    frame.rect = bt::Rect();
    configure(r);

    if (isVisible())
      changeLayer(StackingList::LayerFullScreen);

    updateEWMHState();
    updateEWMHAllowedActions();
  } else {
    // restore from fullscreen
    ::update_decorations(client.decorations,
                         client.functions,
                         isTransient(),
                         client.ewmh,
                         client.motif,
                         client.wmnormal,
                         client.wmprotocols);
    const WindowStyle &style = _screen->resource().windowStyle();
    frame.margin = ::update_margin(client.decorations, style);

    if (isVisible())
      changeLayer(StackingList::LayerNormal);

    if (isMaximized()) {
      remaximize();
    } else {
      bt::Rect r = ::applyGravity(client.premax,
                                  frame.margin,
                                  client.wmnormal.win_gravity);
      r = ::constrain(r, frame.margin, client.wmnormal, TopLeft);

      // trick configure into working
      frame.rect = bt::Rect();
      configure(r);

      updateEWMHState();
      updateEWMHAllowedActions();
    }
  }

  ungrabButtons();
  grabButtons();

  if (refocus)
    (void) setInputFocus();
}


void BlackboxWindow::redrawWindowFrame(void) const {
  if (client.decorations & WindowDecorationTitlebar) {
    redrawTitle();
    redrawLabel();
    redrawAllButtons();
  }

  if (client.decorations & WindowDecorationHandle) {
    redrawHandle();

    if (client.decorations & WindowDecorationGrip)
      redrawGrips();
  }
}


void BlackboxWindow::setFocused(bool focused) {
  if (focused == client.state.focused)
    return;

  client.state.focused = isVisible() ? focused : false;

  if (isVisible()) {
    redrawWindowFrame();

    if (client.state.focused) {
      XInstallColormap(blackbox->XDisplay(), client.colormap);
    } else {
      if (client.ewmh.fullscreen && layer() != StackingList::LayerBelow)
        changeLayer(StackingList::LayerBelow);
    }
  }
}


void BlackboxWindow::setState(unsigned long new_state) {
  client.current_state = new_state;

  unsigned long state[2];
  state[0] = client.current_state;
  state[1] = None;
  XChangeProperty(blackbox->XDisplay(), client.window,
                  blackbox->wmStateAtom(), blackbox->wmStateAtom(), 32,
                  PropModeReplace, (unsigned char *) state, 2);

  updateEWMHState();
  updateEWMHAllowedActions();
}


void BlackboxWindow::updateEWMHState() {
  const bt::EWMH& ewmh = blackbox->ewmh();

  // set _NET_WM_STATE
  bt::EWMH::AtomList atoms;
  if (isModal())
    atoms.push_back(ewmh.wmStateModal());
  if (isShaded())
    atoms.push_back(ewmh.wmStateShaded());
  if (isIconic())
    atoms.push_back(ewmh.wmStateHidden());
  if (isFullScreen())
    atoms.push_back(ewmh.wmStateFullscreen());
  if (client.ewmh.maxh)
    atoms.push_back(ewmh.wmStateMaximizedHorz());
  if (client.ewmh.maxv)
    atoms.push_back(ewmh.wmStateMaximizedVert());
  if (client.ewmh.skip_taskbar)
    atoms.push_back(ewmh.wmStateSkipTaskbar());
  if (client.ewmh.skip_pager)
    atoms.push_back(ewmh.wmStateSkipPager());

  switch (layer()) {
  case StackingList::LayerAbove:
    atoms.push_back(ewmh.wmStateAbove());
    break;
  case StackingList::LayerBelow:
    atoms.push_back(ewmh.wmStateBelow());
    break;
  default:
    break;
  }

  if (atoms.empty())
    ewmh.removeProperty(client.window, ewmh.wmState());
  else
    ewmh.setWMState(client.window, atoms);
}


void BlackboxWindow::updateEWMHAllowedActions() {
  const bt::EWMH& ewmh = blackbox->ewmh();

  // set _NET_WM_ALLOWED_ACTIONS
  bt::EWMH::AtomList atoms;
  if (! client.state.iconic) {
    if (hasWindowFunction(WindowFunctionChangeWorkspace))
      atoms.push_back(ewmh.wmActionChangeDesktop());

    if (hasWindowFunction(WindowFunctionIconify))
      atoms.push_back(ewmh.wmActionMinimize());

    if (hasWindowFunction(WindowFunctionShade))
      atoms.push_back(ewmh.wmActionShade());

    if (hasWindowFunction(WindowFunctionMove))
      atoms.push_back(ewmh.wmActionMove());

    if (hasWindowFunction(WindowFunctionResize))
      atoms.push_back(ewmh.wmActionResize());

    if (hasWindowFunction(WindowFunctionMaximize)) {
      atoms.push_back(ewmh.wmActionMaximizeHorz());
      atoms.push_back(ewmh.wmActionMaximizeVert());
    }

    atoms.push_back(ewmh.wmActionFullscreen());
  }

  if (hasWindowFunction(WindowFunctionClose))
    atoms.push_back(ewmh.wmActionClose());

  if (atoms.empty())
    ewmh.removeProperty(client.window, ewmh.wmAllowedActions());
  else
    ewmh.setWMAllowedActions(client.window, atoms);
}


void BlackboxWindow::redrawTitle(void) const {
  const WindowStyle &style = _screen->resource().windowStyle();
  const bt::Rect u(0, 0, frame.rect.width(), style.title_height);
  bt::drawTexture(_screen->screenNumber(),
                  (client.state.focused
                   ? style.focus.title
                   : style.unfocus.title),
                  frame.title, u, u,
                  (client.state.focused
                   ? frame.ftitle
                   : frame.utitle));
}


void BlackboxWindow::redrawLabel(void) const {
  const WindowStyle &style = _screen->resource().windowStyle();
  bt::Rect u(0, 0, frame.label_w, style.label_height);
  Pixmap p = (client.state.focused ? frame.flabel : frame.ulabel);
  if (p == ParentRelative) {
    const bt::Texture &texture =
      (isFocused() ? style.focus.title : style.unfocus.title);
    int offset = texture.borderWidth();
    if (client.decorations & WindowDecorationIconify)
      offset += style.button_width + style.title_margin;

    const bt::Rect t(-(style.title_margin + offset),
                     -(style.title_margin + texture.borderWidth()),
                     frame.rect.width(), style.title_height);
    bt::drawTexture(_screen->screenNumber(), texture, frame.label, t, u,
                    (client.state.focused ? frame.ftitle : frame.utitle));
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    (client.state.focused
                     ? style.focus.label
                     : style.unfocus.label),
                    frame.label, u, u, p);
  }

  const bt::Pen pen(_screen->screenNumber(),
                    ((client.state.focused)
                     ? style.focus.text
                     : style.unfocus.text));
  u.setCoords(u.left()  + style.label_margin,
              u.top() + style.label_margin,
              u.right() - style.label_margin,
              u.bottom() - style.label_margin);
  bt::drawText(style.font, pen, frame.label, u,
               style.alignment, client.visible_title);
}


void BlackboxWindow::redrawAllButtons(void) const {
  if (frame.iconify_button) redrawIconifyButton();
  if (frame.maximize_button) redrawMaximizeButton();
  if (frame.close_button) redrawCloseButton();
}


void BlackboxWindow::redrawIconifyButton(bool pressed) const {
  const WindowStyle &style = _screen->resource().windowStyle();
  const bt::Rect u(0, 0, style.button_width, style.button_width);
  Pixmap p = (pressed ? frame.pbutton :
              (client.state.focused ? frame.fbutton : frame.ubutton));
  if (p == ParentRelative) {
    const bt::Texture &texture =
      (isFocused() ? style.focus.title : style.unfocus.title);
    const bt::Rect t(-(style.title_margin + texture.borderWidth()),
                     -(style.title_margin + texture.borderWidth()),
                     frame.rect.width(), style.title_height);
    bt::drawTexture(_screen->screenNumber(), texture, frame.iconify_button,
                    t, u, (client.state.focused
                           ? frame.ftitle
                           : frame.utitle));
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    (pressed ? style.pressed :
                     (client.state.focused ? style.focus.button :
                      style.unfocus.button)),
                    frame.iconify_button, u, u, p);
  }

  const bt::Pen pen(_screen->screenNumber(),
                    (client.state.focused
                     ? style.focus.foreground
                     : style.unfocus.foreground));
  bt::drawBitmap(style.iconify, pen, frame.iconify_button, u);
}


void BlackboxWindow::redrawMaximizeButton(bool pressed) const {
  const WindowStyle &style = _screen->resource().windowStyle();
  const bt::Rect u(0, 0, style.button_width, style.button_width);
  Pixmap p = (pressed ? frame.pbutton :
              (client.state.focused ? frame.fbutton : frame.ubutton));
  if (p == ParentRelative) {
    const bt::Texture &texture =
      (isFocused() ? style.focus.title : style.unfocus.title);
    int button_w = style.button_width
                   + style.title_margin + texture.borderWidth();
    if (client.decorations & WindowDecorationClose)
      button_w *= 2;
    const bt::Rect t(-(frame.rect.width() - button_w),
                     -(style.title_margin + texture.borderWidth()),
                     frame.rect.width(), style.title_height);
    bt::drawTexture(_screen->screenNumber(), texture, frame.maximize_button,
                    t, u, (client.state.focused
                           ? frame.ftitle
                           : frame.utitle));
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    (pressed ? style.pressed :
                     (client.state.focused ? style.focus.button :
                      style.unfocus.button)),
                    frame.maximize_button, u, u, p);
  }

  const bt::Pen pen(_screen->screenNumber(),
                    (client.state.focused
                     ? style.focus.foreground
                     : style.unfocus.foreground));
  bt::drawBitmap(isMaximized() ? style.restore : style.maximize,
                 pen, frame.maximize_button, u);
}


void BlackboxWindow::redrawCloseButton(bool pressed) const {
  const WindowStyle &style = _screen->resource().windowStyle();
  const bt::Rect u(0, 0, style.button_width, style.button_width);
  Pixmap p = (pressed ? frame.pbutton :
              (client.state.focused ? frame.fbutton : frame.ubutton));
  if (p == ParentRelative) {
    const bt::Texture &texture =
      (isFocused() ? style.focus.title : style.unfocus.title);
    const int button_w = style.button_width +
                         style.title_margin +
                         texture.borderWidth();
    const bt::Rect t(-(frame.rect.width() - button_w),
                     -(style.title_margin + texture.borderWidth()),
                     frame.rect.width(), style.title_height);
    bt::drawTexture(_screen->screenNumber(),texture, frame.close_button, t, u,
                    (client.state.focused ? frame.ftitle : frame.utitle));
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    (pressed ? style.pressed :
                     (client.state.focused ? style.focus.button :
                      style.unfocus.button)),
                    frame.close_button, u, u, p);
  }

  const bt::Pen pen(_screen->screenNumber(),
                    (client.state.focused
                     ? style.focus.foreground
                     : style.unfocus.foreground));
  bt::drawBitmap(style.close, pen, frame.close_button, u);
}


void BlackboxWindow::redrawHandle(void) const {
  const WindowStyle &style = _screen->resource().windowStyle();
  const bt::Rect u(0, 0, frame.rect.width(), style.handle_height);
  bt::drawTexture(_screen->screenNumber(),
                  (client.state.focused ? style.focus.handle :
                                          style.unfocus.handle),
                  frame.handle, u, u,
                  (client.state.focused ? frame.fhandle : frame.uhandle));
}


void BlackboxWindow::redrawGrips(void) const {
  const WindowStyle &style = _screen->resource().windowStyle();
  const bt::Rect u(0, 0, style.grip_width, style.handle_height);
  Pixmap p = (client.state.focused ? frame.fgrip : frame.ugrip);
  if (p == ParentRelative) {
    bt::Rect t(0, 0, frame.rect.width(), style.handle_height);
    bt::drawTexture(_screen->screenNumber(),
                    (client.state.focused ? style.focus.handle :
                                            style.unfocus.handle),
                    frame.right_grip, t, u, p);

    t.setPos(-(frame.rect.width() - style.grip_width), 0);
    bt::drawTexture(_screen->screenNumber(),
                    (client.state.focused ? style.focus.handle :
                                            style.unfocus.handle),
                    frame.right_grip, t, u, p);
  } else {
    bt::drawTexture(_screen->screenNumber(),
                    (client.state.focused ? style.focus.grip :
                                            style.unfocus.grip),
                    frame.left_grip, u, u, p);

    bt::drawTexture(_screen->screenNumber(),
                    (client.state.focused ? style.focus.grip :
                                            style.unfocus.grip),
                    frame.right_grip, u, u, p);
  }
}


void
BlackboxWindow::clientMessageEvent(const XClientMessageEvent * const event) {
  if (event->format != 32)
    return;

  const bt::EWMH& ewmh = blackbox->ewmh();

  if (event->message_type == blackbox->wmChangeStateAtom()) {
    if (event->data.l[0] == IconicState) {
      if (hasWindowFunction(WindowFunctionIconify))
        iconify();
    } else if (event->data.l[0] == NormalState) {
      activate();
    }
  } else if (event->message_type == ewmh.activeWindow()) {
    activate();
  } else if (event->message_type == ewmh.closeWindow()) {
    if (hasWindowFunction(WindowFunctionClose))
      close();
  } else if (event->message_type == ewmh.moveresizeWindow()) {
    XConfigureRequestEvent request;
    request.window = event->window;
    request.x = event->data.l[1];
    request.y = event->data.l[2];
    request.width = event->data.l[3];
    request.height = event->data.l[4];
    request.value_mask = CWX | CWY | CWWidth | CWHeight;

    const int old_gravity = client.wmnormal.win_gravity;
    if (event->data.l[0] != 0)
      client.wmnormal.win_gravity = event->data.l[0];

    configureRequestEvent(&request);

    client.wmnormal.win_gravity = old_gravity;
  } else if (event->message_type == ewmh.wmDesktop()) {
    if (hasWindowFunction(WindowFunctionChangeWorkspace)) {
      const unsigned int new_workspace = event->data.l[0];
      changeWorkspace(new_workspace);
    }
  } else if (event->message_type == ewmh.wmState()) {
    Atom action = event->data.l[0],
          first = event->data.l[1],
         second = event->data.l[2];

    if (first == ewmh.wmStateModal() || second == ewmh.wmStateModal()) {
      if ((action == ewmh.wmStateAdd() ||
           (action == ewmh.wmStateToggle() && ! client.ewmh.modal)) &&
          isTransient())
        client.ewmh.modal = true;
      else
        client.ewmh.modal = false;
    }

    if (hasWindowFunction(WindowFunctionMaximize)) {
      int max_horz = 0, max_vert = 0;

      if (first == ewmh.wmStateMaximizedHorz() ||
          second == ewmh.wmStateMaximizedHorz()) {
        max_horz = ((action == ewmh.wmStateAdd()
                     || (action == ewmh.wmStateToggle()
                         && !client.ewmh.maxh))
                    ? 1 : -1);
      }

      if (first == ewmh.wmStateMaximizedVert() ||
          second == ewmh.wmStateMaximizedVert()) {
        max_vert = ((action == ewmh.wmStateAdd()
                     || (action == ewmh.wmStateToggle()
                         && !client.ewmh.maxv))
                    ? 1 : -1);
      }

      if (max_horz != 0 || max_vert != 0) {
        if (isMaximized())
          maximize(0);
        unsigned int button = 0u;
        if (max_horz == 1 && max_vert != 1)
          button = 3u;
        else if (max_vert == 1 && max_horz != 1)
          button = 2u;
        else if (max_vert == 1 && max_horz == 1)
          button = 1u;
        if (button)
          maximize(button);
      }
    }

    if (hasWindowFunction(WindowFunctionShade)) {
      if (first == ewmh.wmStateShaded() ||
          second == ewmh.wmStateShaded()) {
        if (action == ewmh.wmStateRemove())
          setShaded(false);
        else if (action == ewmh.wmStateAdd())
          setShaded(true);
        else if (action == ewmh.wmStateToggle())
          setShaded(!isShaded());
      }
    }

    if (first == ewmh.wmStateSkipTaskbar()
        || second == ewmh.wmStateSkipTaskbar()
        || first == ewmh.wmStateSkipPager()
        || second == ewmh.wmStateSkipPager()) {
      if (first == ewmh.wmStateSkipTaskbar()
          || second == ewmh.wmStateSkipTaskbar()) {
        client.ewmh.skip_taskbar = (action == ewmh.wmStateAdd()
                                    || (action == ewmh.wmStateToggle()
                                        && !client.ewmh.skip_taskbar));
      }
      if (first == ewmh.wmStateSkipPager()
          || second == ewmh.wmStateSkipPager()) {
        client.ewmh.skip_pager = (action == ewmh.wmStateAdd()
                                  || (action == ewmh.wmStateToggle()
                                      && !client.ewmh.skip_pager));
      }
      // we do nothing with skip_*, but others might... we should at
      // least make sure these are present in _NET_WM_STATE
      updateEWMHState();
    }

    if (first == ewmh.wmStateHidden() ||
        second == ewmh.wmStateHidden()) {
      /*
        ignore _NET_WM_STATE_HIDDEN, the wm sets this state, not the
        application
      */
    }

    if (hasWindowFunction(WindowFunctionFullScreen)) {
      if (first == ewmh.wmStateFullscreen() ||
          second == ewmh.wmStateFullscreen()) {
        if (action == ewmh.wmStateAdd() ||
            (action == ewmh.wmStateToggle() &&
             ! client.ewmh.fullscreen)) {
          setFullScreen(true);
        } else if (action == ewmh.wmStateToggle() ||
                   action == ewmh.wmStateRemove()) {
          setFullScreen(false);
        }
      }
    }

    if (hasWindowFunction(WindowFunctionChangeLayer)) {
      if (first == ewmh.wmStateAbove() ||
          second == ewmh.wmStateAbove()) {
        if (action == ewmh.wmStateAdd() ||
            (action == ewmh.wmStateToggle() &&
             layer() != StackingList::LayerAbove)) {
          changeLayer(StackingList::LayerAbove);
        } else if (action == ewmh.wmStateToggle() ||
                   action == ewmh.wmStateRemove()) {
          changeLayer(StackingList::LayerNormal);
        }
      }

      if (first == ewmh.wmStateBelow() ||
          second == ewmh.wmStateBelow()) {
        if (action == ewmh.wmStateAdd() ||
            (action == ewmh.wmStateToggle() &&
             layer() != StackingList::LayerBelow)) {
          changeLayer(StackingList::LayerBelow);
        } else if (action == ewmh.wmStateToggle() ||
                   action == ewmh.wmStateRemove()) {
          changeLayer(StackingList::LayerNormal);
        }
      }
    }
  }
}


void BlackboxWindow::unmapNotifyEvent(const XUnmapEvent * const event) {
  if (event->window != client.window)
    return;

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::unmapNotifyEvent() for 0x%lx\n",
          client.window);
#endif // DEBUG

  _screen->releaseWindow(this);
}


void
BlackboxWindow::destroyNotifyEvent(const XDestroyWindowEvent * const event) {
  if (event->window != client.window)
    return;

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::destroyNotifyEvent() for 0x%lx\n",
          client.window);
#endif // DEBUG

  _screen->releaseWindow(this);
}


void BlackboxWindow::reparentNotifyEvent(const XReparentEvent * const event) {
  if (event->window != client.window || event->parent == frame.plate)
    return;

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::reparentNotifyEvent(): reparent 0x%lx to "
          "0x%lx.\n", client.window, event->parent);
#endif // DEBUG

  /*
    put the ReparentNotify event back into the queue so that
    BlackboxWindow::restore(void) can do the right thing
  */
  XEvent replay;
  replay.xreparent = *event;
  XPutBackEvent(blackbox->XDisplay(), &replay);

  _screen->releaseWindow(this);
}


void BlackboxWindow::propertyNotifyEvent(const XPropertyEvent * const event) {
#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::propertyNotifyEvent(): for 0x%lx\n",
          client.window);
#endif

  switch(event->atom) {
  case XA_WM_TRANSIENT_FOR: {
    if (isTransient()) {
      // remove ourselves from our transient_for
      BlackboxWindow *win = findTransientFor();
      if (win) {
        win->removeTransient(this);
      } else if (isGroupTransient()) {
        BWindowGroup *group = findWindowGroup();
        if (group)
          group->removeTransient(this);
      }
    }

    // determine if this is a transient window
    client.transient_for = ::readTransientInfo(blackbox,
                                               client.window,
                                               _screen->screenInfo(),
                                               client.wmhints);

    if (isTransient()) {
      BlackboxWindow *win = findTransientFor();
      if (win) {
        // add ourselves to our new transient_for
        win->addTransient(this);
        changeWorkspace(win->workspace());
        changeLayer(win->layer());
      } else if (isGroupTransient()) {
        BWindowGroup *group = findWindowGroup();
        if (group)
          group->addTransient(this);
      }
    }

    ::update_decorations(client.decorations,
                         client.functions,
                         isTransient(),
                         client.ewmh,
                         client.motif,
                         client.wmnormal,
                         client.wmprotocols);

    reconfigure();
    break;
  }

  case XA_WM_HINTS: {
    // remove from current window group
    BWindowGroup *group = findWindowGroup();
    if (group) {
      if (isTransient() && isGroupTransient())
        group->removeTransient(this);
      group->removeWindow(this);
      group = 0;
    }

    client.wmhints = ::readWMHints(blackbox, client.window);

    if (client.wmhints.window_group != None) {
      // add to new window group
      group = ::update_window_group(client.wmhints.window_group,
                                    blackbox,
                                    this);
      if (isTransient() && isGroupTransient()) {
        if (group)
          group->addTransient(this);
      }
    }
    break;
  }

  case XA_WM_ICON_NAME: {
    client.icon_title = ::readWMIconName(blackbox, client.window);
    if (client.state.iconic)
      _screen->propagateWindowName(this);
    break;
  }

  case XA_WM_NAME: {
    client.title = ::readWMName(blackbox, client.window);

    client.visible_title =
      bt::ellideText(client.title, frame.label_w, bt::toUnicode("..."),
                     _screen->screenNumber(),
                     _screen->resource().windowStyle().font);
    blackbox->ewmh().setWMVisibleName(client.window, client.visible_title);

    if (client.decorations & WindowDecorationTitlebar)
      redrawLabel();

    _screen->propagateWindowName(this);
    break;
  }

  case XA_WM_NORMAL_HINTS: {
    WMNormalHints wmnormal = ::readWMNormalHints(blackbox, client.window,
                                                 _screen->screenInfo());
    if (wmnormal == client.wmnormal) {
      // apps like xv and GNU emacs seem to like to repeatedly set
      // this property over and over
      break;
    }

    client.wmnormal = wmnormal;

    ::update_decorations(client.decorations,
                         client.functions,
                         isTransient(),
                         client.ewmh,
                         client.motif,
                         client.wmnormal,
                         client.wmprotocols);

    reconfigure();
    break;
  }

  default: {
    if (event->atom == blackbox->wmProtocolsAtom()) {
      client.wmprotocols = ::readWMProtocols(blackbox, client.window);

      ::update_decorations(client.decorations,
                           client.functions,
                           isTransient(),
                           client.ewmh,
                           client.motif,
                           client.wmnormal,
                           client.wmprotocols);

      reconfigure();
    } else if (event->atom == blackbox->motifWmHintsAtom()) {
      client.motif = ::readMotifWMHints(blackbox, client.window);

      ::update_decorations(client.decorations,
                           client.functions,
                           isTransient(),
                           client.ewmh,
                           client.motif,
                           client.wmnormal,
                           client.wmprotocols);

      reconfigure();
    } else if (event->atom == blackbox->ewmh().wmStrut()) {
      if (! client.strut) {
        client.strut = new bt::EWMH::Strut;
        _screen->addStrut(client.strut);
      }

      blackbox->ewmh().readWMStrut(client.window, client.strut);
      if (client.strut->left || client.strut->right ||
          client.strut->top || client.strut->bottom) {
        _screen->updateStrut();
      } else {
        _screen->removeStrut(client.strut);
        delete client.strut;
      }
    }

    break;
  }
  } // switch
}


void BlackboxWindow::exposeEvent(const XExposeEvent * const event) {
#ifdef DEBUG
  fprintf(stderr, "BlackboxWindow::exposeEvent() for 0x%lx\n", client.window);
#endif

  if (frame.title == event->window)
    redrawTitle();
  else if (frame.label == event->window)
    redrawLabel();
  else if (frame.close_button == event->window)
    redrawCloseButton();
  else if (frame.maximize_button == event->window)
    redrawMaximizeButton();
  else if (frame.iconify_button == event->window)
    redrawIconifyButton();
  else if (frame.handle == event->window)
    redrawHandle();
  else if (frame.left_grip == event->window ||
           frame.right_grip == event->window)
    redrawGrips();
}


void BlackboxWindow::configureRequestEvent(const XConfigureRequestEvent *
                                           const event) {
  if (event->window != client.window || client.state.iconic)
    return;

  if (event->value_mask & CWBorderWidth)
    client.old_bw = event->border_width;

  if (event->value_mask & (CWX | CWY | CWWidth | CWHeight)) {
    bt::Rect req = frame.rect;

    if (event->value_mask & (CWX | CWY)) {
      req = ::restoreGravity(req, frame.margin, client.wmnormal.win_gravity);

      if (event->value_mask & CWX)
        req.setX(event->x);
      if (event->value_mask & CWY)
        req.setY(event->y);

      req = ::applyGravity(req, frame.margin, client.wmnormal.win_gravity);
    }

    if (event->value_mask & (CWWidth | CWHeight)) {
      if (event->value_mask & CWWidth)
        req.setWidth(event->width + frame.margin.left + frame.margin.right);
      if (event->value_mask & CWHeight)
        req.setHeight(event->height + frame.margin.top + frame.margin.bottom);
    }

    configure(req);
  }

  if (event->value_mask & CWStackMode) {
    switch (event->detail) {
    case Below:
    case BottomIf:
      _screen->lowerWindow(this);
      break;

    case Above:
    case TopIf:
    default:
      _screen->raiseWindow(this);
      break;
    }
  }
}


void BlackboxWindow::buttonPressEvent(const XButtonEvent * const event) {
#ifdef DEBUG
  fprintf(stderr, "BlackboxWindow::buttonPressEvent() for 0x%lx\n",
          client.window);
#endif

  if (frame.maximize_button == event->window) {
    if (event->button < 4)
      redrawMaximizeButton(true);
  } else if (frame.iconify_button == event->window) {
    if (event->button == 1)
      redrawIconifyButton(true);
  } else if (frame.close_button == event->window) {
    if (event->button == 1)
      redrawCloseButton(true);
  } else {
    if (event->button == 1
        || (event->button == 3 && event->state == Mod1Mask)) {
      frame.grab_x = event->x_root - frame.rect.x();
      frame.grab_y = event->y_root - frame.rect.y();

      _screen->raiseWindow(this);

      if (! client.state.focused)
        (void) setInputFocus();
      else
        XInstallColormap(blackbox->XDisplay(), client.colormap);

      if (frame.plate == event->window) {
        XAllowEvents(blackbox->XDisplay(), ReplayPointer, event->time);
      } else if ((frame.title == event->window
                  || frame.label == event->window)
                 && hasWindowFunction(WindowFunctionShade)) {
        if ((event->time - lastButtonPressTime <=
             blackbox->resource().doubleClickInterval()) ||
            event->state == ControlMask) {
          lastButtonPressTime = 0;
          setShaded(!isShaded());
        } else {
          lastButtonPressTime = event->time;
        }
      }
    } else if (event->button == 2) {
      _screen->lowerWindow(this);
    } else if (event->button == 3) {
      const int extra = _screen->resource().windowStyle().frame_border_width;
      const bt::Rect rect(client.rect.x() - extra,
                          client.rect.y() - extra,
                          client.rect.width() + (extra * 2),
                          client.rect.height() + (extra * 2));

      Windowmenu *windowmenu = _screen->windowmenu(this);
      windowmenu->popup(event->x_root, event->y_root, rect);
    }
  }
}


void BlackboxWindow::buttonReleaseEvent(const XButtonEvent * const event) {
#ifdef DEBUG
  fprintf(stderr, "BlackboxWindow::buttonReleaseEvent() for 0x%lx\n",
          client.window);
#endif

  const WindowStyle &style = _screen->resource().windowStyle();
  if (event->window == frame.maximize_button) {
    if (event->button < 4) {
      if (bt::within(event->x, event->y,
                     style.button_width, style.button_width)) {
        maximize(event->button);
        _screen->raiseWindow(this);
      } else {
        redrawMaximizeButton();
      }
    }
  } else if (event->window == frame.iconify_button) {
    if (event->button == 1) {
      if (bt::within(event->x, event->y,
                     style.button_width, style.button_width))
        iconify();
      else
        redrawIconifyButton();
    }
  } else if (event->window == frame.close_button) {
    if (event->button == 1) {
      if (bt::within(event->x, event->y,
                     style.button_width, style.button_width))
        close();
      redrawCloseButton();
    }
  } else if (client.state.moving) {
    finishMove();
  } else if (client.state.resizing) {
    finishResize();
  } else if (event->window == frame.window) {
    if (event->button == 2 && event->state == Mod1Mask)
      XUngrabPointer(blackbox->XDisplay(), blackbox->XTime());
  }
}


void BlackboxWindow::motionNotifyEvent(const XMotionEvent * const event) {
#ifdef DEBUG
  fprintf(stderr, "BlackboxWindow::motionNotifyEvent() for 0x%lx\n",
          client.window);
#endif

  if (hasWindowFunction(WindowFunctionMove)
      && !client.state.resizing
      && event->state & Button1Mask
      && (frame.title == event->window || frame.label == event->window
          || frame.handle == event->window || frame.window == event->window)) {
    if (! client.state.moving)
      startMove();
    else
      continueMove(event->x_root, event->y_root);
  } else if (hasWindowFunction(WindowFunctionResize)
             && (event->state & Button1Mask
                 && (event->window == frame.right_grip
                     || event->window == frame.left_grip))
             || (event->state & Button3Mask
                 && event->state & Mod1Mask
                 && event->window == frame.window)) {
    if (!client.state.resizing)
      startResize(event->window);
    else
      continueResize(event->x_root, event->y_root);
  }
}


void BlackboxWindow::enterNotifyEvent(const XCrossingEvent * const event) {
  if (event->window != frame.window || event->mode != NotifyNormal)
    return;

  if (blackbox->resource().focusModel() == ClickToFocusModel || !isVisible())
    return;

  switch (windowType()) {
  case WindowTypeDesktop:
  case WindowTypeDock:
    // these types cannot be focused w/ sloppy focus
    return;

  default:
    break;
  }

  XEvent next;
  bool leave = False, inferior = False;

  while (XCheckTypedWindowEvent(blackbox->XDisplay(), event->window,
                                LeaveNotify, &next)) {
    if (next.type == LeaveNotify && next.xcrossing.mode == NotifyNormal) {
      leave = True;
      inferior = (next.xcrossing.detail == NotifyInferior);
    }
  }

  if ((! leave || inferior) && ! isFocused())
    (void) setInputFocus();

  if (blackbox->resource().autoRaise())
    timer->start();
}


void
BlackboxWindow::leaveNotifyEvent(const XCrossingEvent * const /*unused*/) {
  if (!(blackbox->resource().focusModel() == SloppyFocusModel
        && blackbox->resource().autoRaise()))
    return;

  if (timer->isTiming())
    timer->stop();
}


#ifdef    SHAPE
void BlackboxWindow::shapeEvent(const XEvent * const /*unused*/)
{ if (client.state.shaped) configureShape(); }
#endif // SHAPE


/*
 *
 */
void BlackboxWindow::restore(void) {
  XChangeSaveSet(blackbox->XDisplay(), client.window, SetModeDelete);
  XSelectInput(blackbox->XDisplay(), client.window, NoEventMask);
  XSelectInput(blackbox->XDisplay(), frame.plate, NoEventMask);

  client.state.visible = false;

  /*
    remove WM_STATE unless the we are shutting down (in which case we
    want to make sure we preserve the state across restarts).
  */
  if (!blackbox->shuttingDown()) {
    clearState(blackbox, client.window);
  } else if (isShaded() && !isIconic()) {
    // do not leave a shaded window as an icon unless it was an icon
    setState(NormalState);
  }

  client.rect = ::restoreGravity(frame.rect, frame.margin,
                                 client.wmnormal.win_gravity);

  blackbox->XGrabServer();

  XUnmapWindow(blackbox->XDisplay(), frame.window);
  XUnmapWindow(blackbox->XDisplay(), client.window);

  XSetWindowBorderWidth(blackbox->XDisplay(), client.window, client.old_bw);
  if (isMaximized()) {
    // preserve the original size
    client.rect = client.premax;
    XMoveResizeWindow(blackbox->XDisplay(), client.window,
                      client.premax.x(),
                      client.premax.y(),
                      client.premax.width(),
                      client.premax.height());
  } else {
    XMoveWindow(blackbox->XDisplay(), client.window,
                client.rect.x() - frame.rect.x(),
                client.rect.y() - frame.rect.y());
  }

  blackbox->XUngrabServer();

  XEvent unused;
  if (!XCheckTypedWindowEvent(blackbox->XDisplay(), client.window,
                              ReparentNotify, &unused)) {
    /*
      according to the ICCCM, the window manager is responsible for
      reparenting the window back to root... however, we don't want to
      do this if the window has been reparented by someone else
      (i.e. not us).
    */
    XReparentWindow(blackbox->XDisplay(), client.window,
                    _screen->screenInfo().rootWindow(),
                    client.rect.x(), client.rect.y());
  }

  if (blackbox->shuttingDown())
    XMapWindow(blackbox->XDisplay(), client.window);
}


// timer for autoraise
void BlackboxWindow::timeout(bt::Timer *)
{ _screen->raiseWindow(this); }


void BlackboxWindow::startMove() {
  // begin a move
  XGrabPointer(blackbox->XDisplay(), frame.window, false,
               Button1MotionMask | ButtonReleaseMask,
               GrabModeAsync, GrabModeAsync, None,
               blackbox->resource().cursors().move, blackbox->XTime());

  client.state.moving = true;

  if (! blackbox->resource().opaqueMove()) {
    blackbox->XGrabServer();

    frame.changing = frame.rect;
    _screen->showGeometry(BScreen::Position, frame.changing);

    bt::Pen pen(_screen->screenNumber(), bt::Color(0xff, 0xff, 0xff));
    const int bw = _screen->resource().windowStyle().frame_border_width,
              hw = bw / 2;
    pen.setGCFunction(GXxor);
    pen.setLineWidth(bw);
    pen.setSubWindowMode(IncludeInferiors);
    XDrawRectangle(blackbox->XDisplay(), _screen->screenInfo().rootWindow(),
                   pen.gc(),
                   frame.changing.x() + hw,
                   frame.changing.y() + hw,
                   frame.changing.width() - bw,
                   frame.changing.height() - bw);
  }
}


static
void collisionAdjust(int* x, int* y, unsigned int width, unsigned int height,
                     const bt::Rect& rect, int snap_distance) {
  // window corners
  const int wleft = *x,
           wright = *x + width - 1,
             wtop = *y,
          wbottom = *y + height - 1,

            dleft = abs(wleft - rect.left()),
           dright = abs(wright - rect.right()),
             dtop = abs(wtop - rect.top()),
          dbottom = abs(wbottom - rect.bottom());

  // snap left?
  if (dleft < snap_distance && dleft <= dright)
    *x = rect.left();
  // snap right?
  else if (dright < snap_distance)
    *x = rect.right() - width + 1;

  // snap top?
  if (dtop < snap_distance && dtop <= dbottom)
    *y = rect.top();
  // snap bottom?
  else if (dbottom < snap_distance)
    *y = rect.bottom() - height + 1;
}


void BlackboxWindow::continueMove(int x_root, int y_root) {
  int dx = x_root - frame.grab_x, dy = y_root - frame.grab_y;
  const int snap_distance = blackbox->resource().edgeSnapThreshold();

  if (snap_distance) {
    collisionAdjust(&dx, &dy, frame.rect.width(), frame.rect.height(),
                    _screen->availableArea(), snap_distance);
    if (!blackbox->resource().fullMaximization())
      collisionAdjust(&dx, &dy, frame.rect.width(), frame.rect.height(),
                      _screen->screenInfo().rect(), snap_distance);
  }

  if (blackbox->resource().opaqueMove()) {
    configure(dx, dy, frame.rect.width(), frame.rect.height());
  } else {
    bt::Pen pen(_screen->screenNumber(), bt::Color(0xff, 0xff, 0xff));
    const int bw = _screen->resource().windowStyle().frame_border_width,
              hw = bw / 2;
    pen.setGCFunction(GXxor);
    pen.setLineWidth(bw);
    pen.setSubWindowMode(IncludeInferiors);
    XDrawRectangle(blackbox->XDisplay(), _screen->screenInfo().rootWindow(),
                   pen.gc(),
                   frame.changing.x() + hw,
                   frame.changing.y() + hw,
                   frame.changing.width() - bw,
                   frame.changing.height() - bw);

    frame.changing.setPos(dx, dy);

    XDrawRectangle(blackbox->XDisplay(), _screen->screenInfo().rootWindow(),
                   pen.gc(),
                   frame.changing.x() + hw,
                   frame.changing.y() + hw,
                   frame.changing.width() - bw,
                   frame.changing.height() - bw);
  }

  _screen->showGeometry(BScreen::Position, bt::Rect(dx, dy, 0, 0));
}


void BlackboxWindow::finishMove() {
  XUngrabPointer(blackbox->XDisplay(), blackbox->XTime());

  client.state.moving = false;

  if (!blackbox->resource().opaqueMove()) {
    bt::Pen pen(_screen->screenNumber(), bt::Color(0xff, 0xff, 0xff));
    const int bw = _screen->resource().windowStyle().frame_border_width,
              hw = bw / 2;
    pen.setGCFunction(GXxor);
    pen.setLineWidth(bw);
    pen.setSubWindowMode(IncludeInferiors);
    XDrawRectangle(blackbox->XDisplay(),
                   _screen->screenInfo().rootWindow(),
                   pen.gc(),
                   frame.changing.x() + hw,
                   frame.changing.y() + hw,
                   frame.changing.width() - bw,
                   frame.changing.height() - bw);
    blackbox->XUngrabServer();

    configure(frame.changing);
  } else {
    configure(frame.rect);
  }

  _screen->hideGeometry();
}


void BlackboxWindow::startResize(Window window) {
  if (frame.grab_x < (signed) frame.rect.width() / 2) {
    if (frame.grab_y < (signed) frame.rect.height() / 2)
      frame.corner = BottomRight;
    else
      frame.corner = TopRight;
  } else {
    if (frame.grab_y < (signed) frame.rect.height() / 2)
      frame.corner = BottomLeft;
    else
      frame.corner = TopLeft;
  }

  Cursor cursor = None;
  switch (frame.corner) {
  case TopLeft:
    cursor = blackbox->resource().cursors().resize_bottom_right;
    frame.grab_x = frame.rect.width() - frame.grab_x;
    frame.grab_y = frame.rect.height() - frame.grab_y;
    break;
  case BottomLeft:
    cursor = blackbox->resource().cursors().resize_top_right;
    frame.grab_x = frame.rect.width() - frame.grab_x;
    break;
  case TopRight:
    cursor = blackbox->resource().cursors().resize_bottom_left;
    frame.grab_y = frame.rect.height() - frame.grab_y;
    break;
  case BottomRight:
    cursor = blackbox->resource().cursors().resize_top_left;
    break;
  }

  // begin a resize
  XGrabPointer(blackbox->XDisplay(), window, False,
               ButtonMotionMask | ButtonReleaseMask,
               GrabModeAsync, GrabModeAsync, None, cursor, blackbox->XTime());

  client.state.resizing = true;

  frame.changing = constrain(frame.rect, frame.margin, client.wmnormal,
                             Corner(frame.corner));

  if (!blackbox->resource().opaqueResize()) {
    blackbox->XGrabServer();

    bt::Pen pen(_screen->screenNumber(), bt::Color(0xff, 0xff, 0xff));
    const int bw = _screen->resource().windowStyle().frame_border_width,
              hw = bw / 2;
    pen.setGCFunction(GXxor);
    pen.setLineWidth(bw);
    pen.setSubWindowMode(IncludeInferiors);
    XDrawRectangle(blackbox->XDisplay(),
                   _screen->screenInfo().rootWindow(),
                   pen.gc(),
                   frame.changing.x() + hw,
                   frame.changing.y() + hw,
                   frame.changing.width() - bw,
                   frame.changing.height() - bw);
  } else {
    // unset maximized state when resized
    if (isMaximized())
      maximize(0);
  }

  showGeometry(frame.changing);
}


void BlackboxWindow::continueResize(int x_root, int y_root) {
  // continue a resize
  const bt::Rect curr = frame.changing;

  switch (frame.corner) {
  case TopLeft:
  case BottomLeft:
    frame.changing.setCoords(frame.changing.left(),
                             frame.changing.top(),
                             std::max<signed>(x_root + frame.grab_x,
                                              frame.changing.left()
                                              + (frame.margin.left
                                                 + frame.margin.right + 1)),
                             frame.changing.bottom());
    break;
  case TopRight:
  case BottomRight:
    frame.changing.setCoords(std::min<signed>(x_root - frame.grab_x,
                                              frame.changing.right()
                                              - (frame.margin.left
                                                 + frame.margin.right + 1)),
                             frame.changing.top(),
                             frame.changing.right(),
                             frame.changing.bottom());
    break;
  }

  switch (frame.corner) {
  case TopLeft:
  case TopRight:
    frame.changing.setCoords(frame.changing.left(),
                             frame.changing.top(),
                             frame.changing.right(),
                             std::max<signed>(y_root + frame.grab_y,
                                              frame.changing.top()
                                              + (frame.margin.top
                                                 + frame.margin.bottom + 1)));
    break;
  case BottomLeft:
  case BottomRight:
    frame.changing.setCoords(frame.changing.left(),
                             std::min<signed>(y_root - frame.grab_y,
                                              frame.rect.bottom()
                                              - (frame.margin.top
                                                 + frame.margin.bottom + 1)),
                             frame.changing.right(),
                             frame.changing.bottom());
    break;
  }

  frame.changing = constrain(frame.changing, frame.margin, client.wmnormal,
                             Corner(frame.corner));

  if (curr != frame.changing) {
    if (blackbox->resource().opaqueResize()) {
      configure(frame.changing);
    } else {
      bt::Pen pen(_screen->screenNumber(), bt::Color(0xff, 0xff, 0xff));
      const int bw = _screen->resource().windowStyle().frame_border_width,
                hw = bw / 2;
      pen.setGCFunction(GXxor);
      pen.setLineWidth(bw);
      pen.setSubWindowMode(IncludeInferiors);
      XDrawRectangle(blackbox->XDisplay(),
                     _screen->screenInfo().rootWindow(),
                     pen.gc(),
                     curr.x() + hw,
                     curr.y() + hw,
                     curr.width() - bw,
                     curr.height() - bw);

      XDrawRectangle(blackbox->XDisplay(),
                     _screen->screenInfo().rootWindow(),
                     pen.gc(),
                     frame.changing.x() + hw,
                     frame.changing.y() + hw,
                     frame.changing.width() - bw,
                     frame.changing.height() - bw);
    }

    showGeometry(frame.changing);
  }
}


void BlackboxWindow::finishResize() {

  if (!blackbox->resource().opaqueResize()) {
    bt::Pen pen(_screen->screenNumber(), bt::Color(0xff, 0xff, 0xff));
    const int bw = _screen->resource().windowStyle().frame_border_width,
              hw = bw / 2;
    pen.setGCFunction(GXxor);
    pen.setLineWidth(bw);
    pen.setSubWindowMode(IncludeInferiors);
    XDrawRectangle(blackbox->XDisplay(),
                   _screen->screenInfo().rootWindow(),
                   pen.gc(),
                   frame.changing.x() + hw,
                   frame.changing.y() + hw,
                   frame.changing.width() - bw,
                   frame.changing.height() - bw);

    blackbox->XUngrabServer();

    // unset maximized state when resized
    if (isMaximized())
      maximize(0);
  }

  client.state.resizing = false;

  XUngrabPointer(blackbox->XDisplay(), blackbox->XTime());

  _screen->hideGeometry();

  frame.changing = constrain(frame.changing, frame.margin, client.wmnormal,
                             Corner(frame.corner));
  configure(frame.changing);
}


/*
 * show the geometry of the window based on rectangle r.
 * The logical width and height are used here.  This refers to the user's
 * perception of the window size (for example an xterm resizes in cells,
 * not in pixels).  No extra work is needed if there is no difference between
 * the logical and actual dimensions.
 */
void BlackboxWindow::showGeometry(const bt::Rect &r) const {
  unsigned int w = r.width(), h = r.height();

  // remove the window frame
  w -= frame.margin.left + frame.margin.right;
  h -= frame.margin.top + frame.margin.bottom;

  if (client.wmnormal.flags & PResizeInc) {
    if (client.wmnormal.flags & (PMinSize|PBaseSize)) {
      w -= ((client.wmnormal.base_width)
            ? client.wmnormal.base_width
            : client.wmnormal.min_width);
      h -= ((client.wmnormal.base_height)
            ? client.wmnormal.base_height
            : client.wmnormal.min_height);
    }

    w /= client.wmnormal.width_inc;
    h /= client.wmnormal.height_inc;
  }

  _screen->showGeometry(BScreen::Size, bt::Rect(0, 0, w, h));
}


// see my rant above for an explanation of this operator
bool operator==(const WMNormalHints &x, const WMNormalHints &y) {
  return (x.flags == y.flags
          && x.min_width == y.min_width
          && x.min_height == y.min_height
          && x.max_width == y.max_width
          && x.max_height == y.max_height
          && x.width_inc == y.width_inc
          && x.height_inc == y.height_inc
          && x.min_aspect_x == y.min_aspect_x
          && x.min_aspect_y == y.min_aspect_y
          && x.max_aspect_x == y.max_aspect_x
          && x.max_aspect_y == y.max_aspect_y
          && x.base_width == y.base_width
          && x.base_height == y.base_height
          && x.win_gravity == y.win_gravity);
}
