// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// BlackboxResource.hh for Blackbox - an X11 Window manager
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

#ifndef   __BlackboxResource_hh
#define   __BlackboxResource_hh

#include <Bitmap.hh>
#include <Color.hh>
#include <Font.hh>
#include <Texture.hh>
#include <Timer.hh>
#include <Unicode.hh>
#include <Util.hh>

#include <vector>

class BScreen;
class Blackbox;

enum FocusModel {
  SloppyFocusModel,
  ClickToFocusModel
};

enum WindowPlacementPolicy {
  RowSmartPlacement = 400,
  ColSmartPlacement,
  CascadePlacement,
  LeftRight,
  RightLeft,
  TopBottom,
  BottomTop
};

struct ToolbarOptions {
  bool enabled;
  int placement;
  bool always_on_top, auto_hide;
  int width_percent;
  std::string strftime_format;
};

struct SlitOptions {
  int direction;
  int placement;
  bool always_on_top, auto_hide;
};

struct WindowStyle {
  struct {
    bt::Color text, foreground;
    bt::Texture title, label, button, handle, grip;
  } focus, unfocus;
  bt::Alignment alignment;
  bt::Bitmap iconify, maximize, restore, close;
  bt::Color frame_border;
  bt::Font font;
  bt::Texture pressed;
  unsigned int title_margin, label_margin, button_margin,
    frame_border_width, handle_height;

  // calculated
  unsigned int title_height, label_height, button_width, grip_width;
};

struct ToolbarStyle {
  bt::Bitmap left, right;
  bt::Color slabel_text, wlabel_text, clock_text, foreground;
  bt::Texture toolbar, slabel, wlabel, clock, button, pressed;
  bt::Font font;
  bt::Alignment alignment;
  unsigned int frame_margin, label_margin, button_margin;

  // calculated extents
  unsigned int toolbar_height, label_height, button_width, hidden_height;
};

struct SlitStyle {
  bt::Texture slit;
  unsigned int margin;
};

class ScreenResource : public bt::NoCopy {
public:
  void loadStyle(BScreen* screen, const std::string& style);
  void load(bt::Resource& res, unsigned int screen);
  void save(bt::Resource& res, BScreen* screen);

  inline const ToolbarOptions &toolbarOptions(void) const
  { return _toolbarOptions; }
  inline const SlitOptions &slitOptions(void) const
  { return _slitOptions; }

  inline const WindowStyle &windowStyle(void) const
  { return _windowStyle; }
  inline const ToolbarStyle &toolbarStyle(void) const
  { return _toolbarStyle; }
  inline const SlitStyle &slitStyle(void) const
  { return _slitStyle; }

  // screen options
  inline unsigned int numberOfWorkspaces(void) const
  { return workspace_count; }
  inline const std::string& rootCommand(void) const
  { return root_command; }
  const bt::ustring &nameOfWorkspace(unsigned int i) const;

  inline void saveWorkspaces(unsigned int w)
  { workspace_count = w; }
  void saveWorkspaceName(unsigned int w, const bt::ustring &name);

private:
  ToolbarOptions _toolbarOptions;
  SlitOptions _slitOptions;

  WindowStyle _windowStyle;
  ToolbarStyle _toolbarStyle;
  SlitStyle _slitStyle;

  unsigned int workspace_count;
  std::string root_command;
  std::vector<bt::ustring> workspaces;
};

class BlackboxResource: public bt::NoCopy {
private:
  ScreenResource* screen_resources;

  struct BCursor {
    Cursor session, move, resize_top_left, resize_bottom_left,
      resize_top_right, resize_bottom_right;
  };
  BCursor cursor;

  std::string menu_file, style_file, rc_file;
  Time double_click_interval;
  bt::timeval auto_raise_delay;

  FocusModel focus_model;
  int window_placement_policy;
  int row_direction, col_direction;
  bool ignore_shaded;
  bool auto_raise;
  bool click_raise;
  bool opaque_move;
  bool opaque_resize;
  bool full_max;
  bool focus_new_windows;
  bool focus_last_window_on_workspace;
  bool allow_scroll_lock;
  unsigned int edge_snap_threshold;

public:
  BlackboxResource(const std::string& rc);
  ~BlackboxResource(void);

  void load(Blackbox& blackbox);
  void save(Blackbox& blackbox);

  inline ScreenResource& screenResource(unsigned int screen)
  { return screen_resources[screen]; }

  inline Cursor sessionCursor(void) const
  { return cursor.session; }
  inline Cursor moveCursor(void) const
  { return cursor.move; }
  inline Cursor resizeTopLeftCursor(void) const
  { return cursor.resize_top_left; }
  inline Cursor resizeBottomLeftCursor(void) const
  { return cursor.resize_bottom_left; }
  inline Cursor resizeTopRightCursor(void) const
  { return cursor.resize_top_right; }
  inline Cursor resizeBottomRightCursor(void) const
  { return cursor.resize_bottom_right; }

  inline const char* rcFilename(void) const
  { return rc_file.c_str(); }
  inline const char* menuFilename(void) const
  { return menu_file.c_str(); }
  inline const char* styleFilename(void) const
  { return style_file.c_str(); }

  inline void saveStyleFilename(const std::string& name)
  { style_file = name; }

  inline Time doubleClickInterval(void) const
  { return double_click_interval; }
  inline const bt::timeval& autoRaiseDelay(void) const
  { return auto_raise_delay; }

  // window focus model
  inline FocusModel focusModel(void) const
  { return focus_model; }
  inline void setFocusModel(FocusModel fm)
  { focus_model = fm; }

  inline bool autoRaise(void) const
  { return auto_raise; }
  inline void setAutoRaise(bool b = true)
  { auto_raise = b; }

  inline bool clickRaise(void) const
  { return click_raise; }
  inline void setClickRaise(bool b = true)
  { click_raise = b; }

  // window placement
  inline int windowPlacementPolicy(void) const
  { return window_placement_policy; }
  inline void setWindowPlacementPolicy(int p)
  { window_placement_policy = p;    }

  inline int rowPlacementDirection(void) const
  { return row_direction; }
  inline void setRowPlacementDirection(int d)
  { row_direction = d; }

  inline int colPlacementDirection(void) const
  { return col_direction; }
  inline void setColPlacementDirection(int d)
  { col_direction = d;       }

  inline bool placementIgnoresShaded(void) const
  { return ignore_shaded; }
  inline void setPlacementIgnoresShaded(bool f)
  { ignore_shaded = f; }

  // other window options
  inline bool opaqueMove(void) const
  { return opaque_move; }
  inline void setOpaqueMove(bool b = true)
  { opaque_move = b; }

  inline bool opaqueResize(void) const
  { return opaque_resize; }
  inline void setOpaqueResize(bool b = true)
  { opaque_resize = b; }

  inline bool fullMaximization(void) const
  { return full_max; }
  inline void setFullMaximization(bool b = true)
  { full_max = b; }

  inline bool focusNewWindows(void) const
  { return focus_new_windows; }
  inline void setFocusNewWindows(bool b = true)
  { focus_new_windows = b; }

  inline bool focusLastWindowOnWorkspace(void) const
  { return focus_last_window_on_workspace; }
  inline void setFocusLastWindowOnWorkspace(bool b = true)
  { focus_last_window_on_workspace = b; }

  inline bool allowScrollLock(void) const
  { return allow_scroll_lock; }
  inline void setAllowScrollLock(bool a)
  { allow_scroll_lock = a; }

  inline unsigned int edgeSnapThreshold(void) const
  { return edge_snap_threshold; }
  inline void setEdgeSnapThreshold(unsigned int t)
  { edge_snap_threshold = t; }
};

#endif
