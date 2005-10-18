// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// BlackboxResource.cc for Blackbox - an X11 Window manager
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

#include "BlackboxResource.hh"

#include "blackbox.hh"

#include <Image.hh>
#include <Resource.hh>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>


BlackboxResource::BlackboxResource(const std::string& rc): rc_file(rc) {
  screen_resources = 0;
  auto_raise_delay.tv_sec = auto_raise_delay.tv_usec = 0;
}


BlackboxResource::~BlackboxResource(void)
{ delete [] screen_resources; }


void BlackboxResource::load(Blackbox& blackbox) {
  if (screen_resources == 0) {
    screen_resources = new ScreenResource[blackbox.screenCount()];
  }

  bt::Resource res(rc_file);

  menu_file = bt::expandTilde(res.read("session.menuFile",
                                       "Session.MenuFile",
                                       DEFAULTMENU));

  style_file = bt::expandTilde(res.read("session.styleFile",
                                        "Session.StyleFile",
                                        DEFAULTSTYLE));

  unsigned int maxcolors = res.read("session.maximumColors",
                                    "Session.MaximumColors",
                                    ~0u);
  if (maxcolors != ~0u)
    bt::Image::setMaximumColors(maxcolors);

  double_click_interval = res.read("session.doubleClickInterval",
                                   "Session.DoubleClickInterval",
                                   250l);

  auto_raise_delay.tv_usec = res.read("session.autoRaiseDelay",
                                      "Session.AutoRaiseDelay",
                                      400l);

  auto_raise_delay.tv_sec = auto_raise_delay.tv_usec / 1000;
  auto_raise_delay.tv_usec -= (auto_raise_delay.tv_sec * 1000);
  auto_raise_delay.tv_usec *= 1000;

  bt::DitherMode dither_mode;
  std::string str = res.read("session.imageDither",
                             "Session.ImageDither",
                             "OrderedDither");
  if (!strcasecmp("ordered", str.c_str()) ||
      !strcasecmp("fast", str.c_str()) ||
      !strcasecmp("ordereddither", str.c_str()) ||
      !strcasecmp("fastdither", str.c_str())) {
    dither_mode = bt::OrderedDither;
  } else if (!strcasecmp("floydsteinberg", str.c_str()) ||
             !strcasecmp("quality", str.c_str()) ||
             !strcasecmp("diffuse", str.c_str()) ||
             !strcasecmp("floydsteinbergdither", str.c_str()) ||
             !strcasecmp("qualitydither", str.c_str()) ||
             !strcasecmp("diffusedither", str.c_str())) {
    dither_mode = bt::FloydSteinbergDither;
  } else if (!strcasecmp("no", str.c_str()) ||
             !strcasecmp("nodither", str.c_str()) ||
             !strcasecmp("off", str.c_str())) {
    dither_mode = bt::NoDither;
  } else {
    dither_mode = bt::OrderedDither;
  }
  bt::Image::setDitherMode(dither_mode);

  _cursors.pointer =
    XCreateFontCursor(blackbox.XDisplay(), XC_left_ptr);
  _cursors.move =
    XCreateFontCursor(blackbox.XDisplay(), XC_fleur);
  _cursors.resize_top_left =
    XCreateFontCursor(blackbox.XDisplay(), XC_top_left_corner);
  _cursors.resize_bottom_left =
    XCreateFontCursor(blackbox.XDisplay(), XC_bottom_left_corner);
  _cursors.resize_top_right =
    XCreateFontCursor(blackbox.XDisplay(), XC_top_right_corner);
  _cursors.resize_bottom_right =
    XCreateFontCursor(blackbox.XDisplay(), XC_bottom_right_corner);

  // window options
  str = res.read("session.focusModel",
                 "Session.FocusModel",
                 res.read("session.screen0.focusModel",
                          "Session.Screen0.FocusModel",
                          "ClickToFocus"));
  if (str.find("ClickToFocus") != std::string::npos) {
    focus_model = ClickToFocusModel;
    auto_raise = false;
    click_raise = false;
  } else {
    focus_model = SloppyFocusModel;
    auto_raise = (str.find("AutoRaise") != std::string::npos);
    click_raise = (str.find("ClickRaise") != std::string::npos);
  }

  str = res.read("session.windowPlacement",
                 "Session.WindowPlacement",
                 res.read("session.screen0.windowPlacement",
                          "Session.Screen0.WindowPlacement",
                          "RowSmartPlacement"));
  if (strcasecmp(str.c_str(), "ColSmartPlacement") == 0)
    window_placement_policy = ColSmartPlacement;
  else if (strcasecmp(str.c_str(), "CenterPlacement") == 0)
    window_placement_policy = CenterPlacement;
  else if (strcasecmp(str.c_str(), "CascadePlacement") == 0)
    window_placement_policy = CascadePlacement;
  else
    window_placement_policy = RowSmartPlacement;

  str = res.read("session.rowPlacementDirection",
                 "Session.RowPlacementDirection",
                 res.read("session.screen0.rowPlacementDirection",
                          "Session.Screen0.RowPlacementDirection",
                          "lefttoright"));
  row_direction =
    (strcasecmp(str.c_str(), "righttoleft") == 0) ? RightLeft : LeftRight;

  str = res.read("session.colPlacementDirection",
                 "Session.ColPlacementDirection",
                 res.read("session.screen0.colPlacementDirection",
                          "Session.Screen0.ColPlacementDirection",
                          "toptobottom"));
  col_direction =
    (strcasecmp(str.c_str(), "bottomtotop") == 0) ? BottomTop : TopBottom;

  ignore_shaded =
    res.read("session.placementIgnoresShaded",
             "Session.placementIgnoresShaded",
             true);

  opaque_move =
    res.read("session.opaqueMove",
             "Session.OpaqueMove",
             true);
  opaque_resize =
    res.read("session.opaqueResize",
             "Session.OpaqueResize",
             true);
  full_max =
    res.read("session.fullMaximization",
             "Session.FullMaximization",
             res.read("session.screen0.fullMaximization",
                      "Session.Screen0.FullMaximization",
                      false));
  focus_new_windows =
    res.read("session.focusNewWindows",
             "Session.FocusNewWindows",
             res.read("session.screen0.focusNewWindows",
                      "Session.Screen0.FocusNewWindows",
                      true));
  focus_last_window_on_workspace =
    res.read("session.focusLastWindow",
             "Session.focusLastWindow",
             res.read("session.screen0.focusLastWindow",
                      "Session.Screen0.focusLastWindow",
                      true));
  change_workspace_with_mouse_wheel =
    res.read("session.changeWorkspaceWithMouseWheel",
             "session.changeWorkspaceWithMouseWheel",
             true);
  shade_window_with_mouse_wheel =
    res.read("session.shadeWindowWithMouseWheel",
             "session.shadeWindowWithMouseWheel",
             true);
  toolbar_actions_with_mouse_wheel =
    res.read("session.toolbarActionsWithMouseWheel",
             "session.toolbarActionsWithMouseWheel",
             true);
  allow_scroll_lock =
    res.read("session.disableBindingsWithScrollLock",
             "Session.disableBindingsWithScrollLock",
             res.read("session.screen0.disableBindingsWithScrollLock",
                      "Session.Screen0.disableBindingsWithScrollLock",
                      false));
  edge_snap_threshold =
    res.read("session.edgeSnapThreshold",
             "Session.EdgeSnapThreshold",
             res.read("session.screen0.edgeSnapThreshold",
                      "Session.Screen0.EdgeSnapThreshold",
                      0));
  window_snap_threshold =
    res.read("session.windowSnapThreshold",
             "Session.windowSnapThreshold",
             0);

  for (unsigned int i = 0; i < blackbox.screenCount(); ++i)
    screen_resources[i].load(res, i);
}


void BlackboxResource::save(Blackbox& blackbox) {
  bt::Resource res;

  {
    if (bt::Resource(rc_file).read("session.cacheLife",
                                   "Session.CacheLife",
                                   -1) == -1) {
      res.merge(rc_file);
    } else {
      // we are converting from 0.65.0 to 0.70.0, let's take the liberty
      // of generating a brand new rc file to make sure we throw out
      // undeeded entries
    }
  }

  res.write("session.menuFile", menuFilename());

  res.write("session.styleFile", styleFilename());

  res.write("session.maximumColors",  bt::Image::maximumColors());

  res.write("session.doubleClickInterval", double_click_interval);

  res.write("session.autoRaiseDelay", ((auto_raise_delay.tv_sec * 1000ul) +
                                       (auto_raise_delay.tv_usec / 1000ul)));

  std::string str;
  switch (bt::Image::ditherMode()) {
  case bt::OrderedDither:        str = "OrderedDither";        break;
  case bt::FloydSteinbergDither: str = "FloydSteinbergDither"; break;
  default:                       str = "NoDither";             break;
  }
  res.write("session.imageDither", str);

  // window options
  switch (focus_model) {
  case SloppyFocusModel:
  default:
    str = "SloppyFocus";
    if (auto_raise)
      str += " AutoRaise";
    if (click_raise)
      str += " ClickRaise";
    break;
  case ClickToFocusModel:
    str = "ClickToFocus";
    break;
  }
  res.write("session.focusModel", str);

  switch (window_placement_policy) {
  case CascadePlacement:
    str = "CascadePlacement";
    break;
  case CenterPlacement:
    str = "CenterPlacement";
    break;
  case ColSmartPlacement:
    str = "ColSmartPlacement";
    break;
  case RowSmartPlacement:
  default:
    str = "RowSmartPlacement";
    break;
  }
  res.write("session.windowPlacement", str);
  res.write("session.rowPlacementDirection",
            (row_direction == LeftRight)
            ? "LeftToRight"
            : "RightToLeft");
  res.write("session.colPlacementDirection",
            (col_direction == TopBottom)
            ? "TopToBottom"
            : "BottomToTop");

  res.write("session.placementIgnoresShaded", ignore_shaded);

  res.write("session.opaqueMove", opaque_move);
  res.write("session.opaqueResize", opaque_resize);
  res.write("session.fullMaximization", full_max);
  res.write("session.focusNewWindows", focus_new_windows);
  res.write("session.focusLastWindow", focus_last_window_on_workspace);
  res.write("session.changeWorkspaceWithMouseWheel",
            change_workspace_with_mouse_wheel);
  res.write("session.shadeWindowWithMouseWheel",
            shade_window_with_mouse_wheel);
  res.write("session.toolbarActionsWithMouseWheel",
            toolbar_actions_with_mouse_wheel);
  res.write("session.disableBindingsWithScrollLock", allow_scroll_lock);
  res.write("session.edgeSnapThreshold", edge_snap_threshold);
  res.write("session.windowSnapThreshold", window_snap_threshold);

  for (unsigned int i = 0; i < blackbox.screenCount(); ++i)
    screen_resources[i].save(res, blackbox.screenNumber(i));

  res.save(rc_file);
}


