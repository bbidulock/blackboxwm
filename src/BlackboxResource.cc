// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// BlackboxResource.cc for Blackbox - an X11 Window manager
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

#include "BlackboxResource.hh"
#include "Screen.hh"

// next two needed for their enums
#include "Slit.hh"
#include "Toolbar.hh"

#include <Image.hh>
#include <Menu.hh>
#include <Resource.hh>

#include <X11/Xutil.h>
#include <X11/cursorfont.h>

static const std::string empty_string;


static const int iconify_width  = 9;
static const int iconify_height = 9;
static const unsigned char iconify_bits[] =
  { 0x00, 0x00, 0x82, 0x00, 0xc6, 0x00, 0x6c, 0x00, 0x38,
    0x00, 0x10, 0x00, 0x00, 0x00, 0xff, 0x01, 0xff, 0x01 };

static const int maximize_width  = 9;
static const int maximize_height = 9;
static const unsigned char maximize_bits[] =
  { 0xff, 0x01, 0xff, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xff, 0x01 };

static const int restore_width  = 9;
static const int restore_height = 9;
static const unsigned char restore_bits[] =
  { 0xf8, 0x01, 0xf8, 0x01, 0x08, 0x01, 0x3f, 0x01, 0x3f,
    0x01, 0xe1, 0x01, 0x21, 0x00, 0x21, 0x00, 0x3f, 0x00 };

static const int close_width  = 9;
static const int close_height = 9;
static const unsigned char close_bits[] =
  { 0x83, 0x01, 0xc7, 0x01, 0xee, 0x00, 0x7c, 0x00, 0x38,
    0x00, 0x7c, 0x00, 0xee, 0x00, 0xc7, 0x01, 0x83, 0x01 };


BlackboxResource::BlackboxResource(const std::string& rc): rc_file(rc) {
  screen_resources = 0;
  auto_raise_delay.tv_sec = auto_raise_delay.tv_usec = 0;
  cache_life = cache_max = 0;

  XrmInitialize(); // TODO: move this off to Resource class
}


BlackboxResource::~BlackboxResource(void)
{ delete [] screen_resources; }


void BlackboxResource::load(Blackbox& blackbox) {
  if (screen_resources == 0) {
    screen_resources = new ScreenResource[blackbox.screenCount()];
  }

  bt::Resource res(rc_file);

  menu_file = bt::expandTilde(res.read("session.menuFile",
                                       "Session.MenuFile", DEFAULTMENU));

  bt::Image::setColorsPerChannel(res.read("session.colorsPerChannel",
                                          "Session.ColorsPerChannel",
                                          bt::Image::colorsPerChannel()));

  style_file = bt::expandTilde(res.read("session.styleFile",
                                        "Session.StyleFile", DEFAULTSTYLE));

  double_click_interval = res.read("session.doubleClickInterval",
                                   "Session.DoubleClickInterval", 250l);

  auto_raise_delay.tv_usec = res.read("session.autoRaiseDelay",
                                      "Session.AutoRaiseDelay", 400l);

  auto_raise_delay.tv_sec = auto_raise_delay.tv_usec / 1000;
  auto_raise_delay.tv_usec -= (auto_raise_delay.tv_sec * 1000);
  auto_raise_delay.tv_usec *= 1000;

  cache_life = res.read("session.cacheLife", "Session.CacheLife", 5l);
  cache_life *= 60000;

  cache_max = res.read("session.cacheMax", "Session.CacheMax", 200l);

  bt::DitherMode dither_mode;
  std::string tmp = res.read("session.imageDither", "Session.ImageDither",
                             "OrderedDither");
  if (!strcasecmp("ordered", tmp.c_str()) ||
      !strcasecmp("fast", tmp.c_str()) ||
      !strcasecmp("ordereddither", tmp.c_str()) ||
      !strcasecmp("fastdither", tmp.c_str())) {
    dither_mode = bt::OrderedDither;
  } else if (!strcasecmp("floydsteinberg", tmp.c_str()) ||
             !strcasecmp("quality", tmp.c_str()) ||
             !strcasecmp("diffuse", tmp.c_str()) ||
             !strcasecmp("floydsteinbergdither", tmp.c_str()) ||
             !strcasecmp("qualitydither", tmp.c_str()) ||
             !strcasecmp("diffusedither", tmp.c_str())) {
    dither_mode = bt::FloydSteinbergDither;
  } else if (!strcasecmp("no", tmp.c_str()) ||
             !strcasecmp("nodither", tmp.c_str()) ||
             !strcasecmp("off", tmp.c_str())) {
    dither_mode = bt::NoDither;
  } else {
    dither_mode = bt::OrderedDither;
  }
  bt::Image::setDitherMode(dither_mode);

  cursor.session = XCreateFontCursor(blackbox.XDisplay(), XC_left_ptr);
  cursor.move = XCreateFontCursor(blackbox.XDisplay(), XC_fleur);
  cursor.resize_bottom_left =
    XCreateFontCursor(blackbox.XDisplay(), XC_bottom_left_corner);
  cursor.resize_bottom_right =
    XCreateFontCursor(blackbox.XDisplay(), XC_bottom_right_corner);

  for (unsigned int i = 0; i < blackbox.screenCount(); ++i)
    screen_resources[i].load(res, i);
}


void BlackboxResource::save(Blackbox& blackbox) {
  bt::Resource res(rc_file);

  res.write("session.menuFile", menuFilename());

  res.write("session.styleFile", styleFilename());

  res.write("session.colorsPerChannel",  bt::Image::colorsPerChannel());

  res.write("session.doubleClickInterval", double_click_interval);

  res.write("session.autoRaiseDelay", ((auto_raise_delay.tv_sec * 1000ul) +
                                       (auto_raise_delay.tv_usec / 1000ul)));

  res.write("session.cacheLife", cache_life / 60000);

  res.write("session.cacheMax", cache_max);

  const char* tmp;
  switch (bt::Image::ditherMode()) {
  case bt::OrderedDither:        tmp = "OrderedDither";        break;
  case bt::FloydSteinbergDither: tmp = "FloydSteinbergDither"; break;
  default:                       tmp = "NoDither";             break;
  }
  res.write("session.imageDither", tmp);

  for (unsigned int i = 0; i < blackbox.screenCount(); ++i)
    screen_resources[i].save(res, blackbox.screenNumber(i));

  res.save(rc_file);
}


ScreenResource::ScreenResource(void) { }
ScreenResource::~ScreenResource(void) { }
ScreenResource::WindowStyle::WindowStyle(void) { }
ScreenResource::WindowStyle::~WindowStyle(void) { }
ScreenResource::ToolbarStyle::ToolbarStyle(void) { }
ScreenResource::ToolbarStyle::~ToolbarStyle(void) { }


void ScreenResource::save(bt::Resource& res, BScreen* screen) {
  char rc_string[128];
  char *placement = (char *) 0;
  unsigned int number = screen->screenNumber();

  switch (sconfig.placement) {
  case Slit::TopLeft: placement = "TopLeft"; break;
  case Slit::CenterLeft: placement = "CenterLeft"; break;
  case Slit::BottomLeft: placement = "BottomLeft"; break;
  case Slit::TopCenter: placement = "TopCenter"; break;
  case Slit::BottomCenter: placement = "BottomCenter"; break;
  case Slit::TopRight: placement = "TopRight"; break;
  case Slit::BottomRight: placement = "BottomRight"; break;
  case Slit::CenterRight: default: placement = "CenterRight"; break;
  }

  sprintf(rc_string, "session.screen%u.slit.placement", number);
  res.write(rc_string, placement);

  sprintf(rc_string, "session.screen%u.slit.direction", number);
  res.write(rc_string, (sconfig.direction == Slit::Horizontal) ?
            "Horizontal" : "Vertical");

  sprintf(rc_string, "session.screen%u.slit.onTop", number);
  res.write(rc_string, sconfig.on_top);

  sprintf(rc_string, "session.screen%u.slit.autoHide", number);
  res.write(rc_string, sconfig.auto_hide);

  sprintf(rc_string, "session.screen%u.placementIgnoresShaded", number);
  res.write(rc_string, wconfig.ignore_shaded);

  sprintf(rc_string, "session.screen%u.fullMaximization", number);
  res.write(rc_string, wconfig.full_max);

  sprintf(rc_string, "session.screen%u.focusNewWindows", number);
  res.write(rc_string, wconfig.focus_new);

  sprintf(rc_string, "session.screen%u.focusLastWindow", number);
  res.write(rc_string, wconfig.focus_last);

  sprintf(rc_string, "session.screen%u.opaqueMove", number);
  res.write(rc_string, wconfig.opaque_move);

  sprintf(rc_string, "session.screen%u.disableBindingsWithScrollLock",
	  number);
  res.write(rc_string, allow_scroll_lock);

  sprintf(rc_string,  "session.screen%u.enableToolbar", number);
  res.write(rc_string, enable_toolbar);

  sprintf(rc_string, "session.screen%u.rowPlacementDirection", number);
  res.write(rc_string,
            (wconfig.row_direction == LeftRight) ? "LeftToRight" :
                                                   "RightToLeft");

  sprintf(rc_string, "session.screen%u.colPlacementDirection", number);
  res.write(rc_string,
            (wconfig.col_direction == TopBottom) ? "TopToBottom" :
                                                   "BottomToTop");

  switch (wconfig.placement_policy) {
  case CascadePlacement:
    placement = "CascadePlacement";
    break;
  case ColSmartPlacement:
    placement = "ColSmartPlacement";
    break;
  case RowSmartPlacement:
  default:
    placement = "RowSmartPlacement";
    break;
  }
  sprintf(rc_string, "session.screen%u.windowPlacement", number);
  res.write(rc_string, placement);

  std::string fmodel;
  if (! wconfig.sloppy_focus) {
    fmodel = "ClickToFocus";
  } else {
    fmodel = "SloppyFocus";
    if (wconfig.auto_raise) fmodel += " AutoRaise";
    if (wconfig.click_raise) fmodel += " ClickRaise";
  }

  sprintf(rc_string, "session.screen%u.focusModel", number);
  res.write(rc_string, fmodel.c_str());

  sprintf(rc_string, "session.screen%u.workspaces", number);
  res.write(rc_string, workspace_count);

  sprintf(rc_string, "session.screen%u.toolbar.onTop", number);
  res.write(rc_string, tconfig.on_top);

  sprintf(rc_string, "session.screen%u.toolbar.autoHide", number);
  res.write(rc_string, tconfig.auto_hide);

  switch (tconfig.placement) {
  case Toolbar::TopLeft: placement = "TopLeft"; break;
  case Toolbar::BottomLeft: placement = "BottomLeft"; break;
  case Toolbar::TopCenter: placement = "TopCenter"; break;
  case Toolbar::TopRight: placement = "TopRight"; break;
  case Toolbar::BottomRight: placement = "BottomRight"; break;
  case Toolbar::BottomCenter: default: placement = "BottomCenter"; break;
  }

  sprintf(rc_string, "session.screen%u.toolbar.placement", number);
  res.write(rc_string, placement);

  std::vector<std::string>::const_iterator it = workspaces.begin(),
    end = workspaces.end();
  std::string save_string = *it++;
  for (; it != end; ++it) {
    save_string += ',';
    save_string += *it;
  }

  sprintf(rc_string, "session.screen%u.workspaceNames", number);
  res.write(rc_string, save_string.c_str());

  // these options can not be modified at runtime currently

  sprintf(rc_string, "session.screen%u.strftimeFormat", number);
  res.write(rc_string, tconfig.strftime_format.c_str());

  sprintf(rc_string, "session.screen%u.edgeSnapThreshold", number);
  res.write(rc_string, wconfig.edge_snap_threshold);

  sprintf(rc_string, "session.screen%u.toolbar.widthPercent", number);
  res.write(rc_string, tconfig.width_percent);
}


void ScreenResource::load(bt::Resource& res, unsigned int screen) {
  char name_lookup[128], class_lookup[128];

  // window settings and behavior

  sprintf(name_lookup,  "session.screen%u.placementIgnoresShaded", screen);
  sprintf(class_lookup, "Session.screen%u.placementIgnoresShaded", screen);
  wconfig.ignore_shaded = res.read(name_lookup, class_lookup, true);

  sprintf(name_lookup,  "session.screen%u.fullMaximization", screen);
  sprintf(class_lookup, "Session.screen%u.FullMaximization", screen);
  wconfig.full_max = res.read(name_lookup, class_lookup, false);

  sprintf(name_lookup,  "session.screen%u.focusNewWindows", screen);
  sprintf(class_lookup, "Session.screen%u.FocusNewWindows", screen);
  wconfig.focus_new = res.read(name_lookup, class_lookup, false);

  sprintf(name_lookup,  "session.screen%u.focusLastWindow", screen);
  sprintf(class_lookup, "Session.screen%u.focusLastWindow", screen);
  wconfig.focus_last = res.read(name_lookup, class_lookup, false);

  std::string tmp;

  sprintf(name_lookup,  "session.screen%u.rowPlacementDirection", screen);
  sprintf(class_lookup, "Session.screen%u.RowPlacementDirection", screen);
  tmp = res.read(name_lookup, class_lookup, "lefttoright");
  wconfig.row_direction =
    (strcasecmp(tmp.c_str(), "righttoleft") == 0) ? RightLeft : LeftRight;

  sprintf(name_lookup,  "session.screen%u.colPlacementDirection", screen);
  sprintf(class_lookup, "Session.screen%u.ColPlacementDirection", screen);
  tmp = res.read(name_lookup, class_lookup, "toptobottom");
  wconfig.col_direction =
    (strcasecmp(tmp.c_str(), "bottomtotop") == 0) ? BottomTop : TopBottom;

  sprintf(name_lookup,  "session.screen%u.windowPlacement", screen);
  sprintf(class_lookup, "Session.screen%u.WindowPlacement", screen);
  tmp = res.read(name_lookup, class_lookup, "RowSmartPlacement");
  if (strcasecmp(tmp.c_str(), "ColSmartPlacement") == 0)
    wconfig.placement_policy = ColSmartPlacement;
  else if (strcasecmp(tmp.c_str(), "CascadePlacement") == 0)
    wconfig.placement_policy = CascadePlacement;
  else
    wconfig.placement_policy = RowSmartPlacement;

  sprintf(name_lookup,  "session.screen%u.focusModel", screen);
  sprintf(class_lookup, "Session.screen%u.FocusModel", screen);
  wconfig.sloppy_focus = True;
  wconfig.auto_raise = False;
  wconfig.click_raise = False;
  tmp = res.read(name_lookup, class_lookup, "SloppyFocus");
  if (tmp.find("ClickToFocus") != std::string::npos) {
    wconfig.sloppy_focus = False;
  } else {
    // must be sloppy
    if (tmp.find("AutoRaise") != std::string::npos)
      wconfig.auto_raise = True;
    if (tmp.find("ClickRaise") != std::string::npos)
      wconfig.click_raise = True;
  }

  sprintf(name_lookup,  "session.screen%u.edgeSnapThreshold", screen);
  sprintf(class_lookup, "Session.screen%u.EdgeSnapThreshold", screen);
  wconfig.edge_snap_threshold = res.read(name_lookup, class_lookup, 0);

  sprintf(name_lookup,  "session.screen%u.opaqueMove", screen);
  sprintf(class_lookup, "Session.screen%u.OpaqueMove", screen);
  wconfig.opaque_move = res.read(name_lookup, class_lookup, false);

  // toolbar settings

  sprintf(name_lookup,  "session.screen%u.toolbar.widthPercent", screen);
  sprintf(class_lookup, "Session.screen%u.Toolbar.WidthPercent", screen);
  tconfig.width_percent = res.read(name_lookup, class_lookup, 66);

  sprintf(name_lookup, "session.screen%u.toolbar.placement", screen);
  sprintf(class_lookup, "Session.screen%u.Toolbar.Placement", screen);
  tmp = res.read(name_lookup, class_lookup, "BottomCenter");
  if (! strcasecmp(tmp.c_str(), "TopLeft"))
    tconfig.placement = Toolbar::TopLeft;
  else if (! strcasecmp(tmp.c_str(), "BottomLeft"))
    tconfig.placement = Toolbar::BottomLeft;
  else if (! strcasecmp(tmp.c_str(), "TopCenter"))
    tconfig.placement = Toolbar::TopCenter;
  else if (! strcasecmp(tmp.c_str(), "TopRight"))
    tconfig.placement = Toolbar::TopRight;
  else if (! strcasecmp(tmp.c_str(), "BottomRight"))
    tconfig.placement = Toolbar::BottomRight;
  else
    tconfig.placement = Toolbar::BottomCenter;

  sprintf(name_lookup,  "session.screen%u.toolbar.onTop", screen);
  sprintf(class_lookup, "Session.screen%u.Toolbar.OnTop", screen);
  tconfig.on_top = res.read(name_lookup, class_lookup, false);

  sprintf(name_lookup,  "session.screen%u.toolbar.autoHide", screen);
  sprintf(class_lookup, "Session.screen%u.Toolbar.autoHide", screen);
  tconfig.auto_hide = res.read(name_lookup, class_lookup, false);

  sprintf(name_lookup,  "session.screen%u.strftimeFormat", screen);
  sprintf(class_lookup, "Session.screen%u.StrftimeFormat", screen);
  tconfig.strftime_format = res.read(name_lookup, class_lookup, "%I:%M %p");

  // slit settings
  sprintf(name_lookup, "session.screen%u.slit.placement", screen);
  sprintf(class_lookup, "Session.screen%u.Slit.Placement", screen);
  tmp = res.read(name_lookup, class_lookup, "CenterRight");
  if (! strcasecmp(tmp.c_str(), "TopLeft"))
    sconfig.placement = Slit::TopLeft;
  else if (! strcasecmp(tmp.c_str(), "CenterLeft"))
    sconfig.placement = Slit::CenterLeft;
  else if (! strcasecmp(tmp.c_str(), "BottomLeft"))
    sconfig.placement = Slit::BottomLeft;
  else if (! strcasecmp(tmp.c_str(), "TopCenter"))
    sconfig.placement = Slit::TopCenter;
  else if (! strcasecmp(tmp.c_str(), "BottomCenter"))
    sconfig.placement = Slit::BottomCenter;
  else if (! strcasecmp(tmp.c_str(), "TopRight"))
    sconfig.placement = Slit::TopRight;
  else if (! strcasecmp(tmp.c_str(), "BottomRight"))
    sconfig.placement = Slit::BottomRight;
  else
    sconfig.placement = Slit::CenterRight;

  sprintf(name_lookup, "session.screen%u.slit.direction", screen);
  sprintf(class_lookup, "Session.screen%u.Slit.Direction", screen);
  tmp = res.read(name_lookup, class_lookup, "Vertical");
  if (! strcasecmp(tmp.c_str(), "Horizontal"))
    sconfig.direction = Slit::Horizontal;
  else
    sconfig.direction = Slit::Vertical;

  sprintf(name_lookup, "session.screen%u.slit.onTop", screen);
  sprintf(class_lookup, "Session.screen%u.Slit.OnTop", screen);
  sconfig.on_top = res.read(name_lookup, class_lookup, false);

  sprintf(name_lookup, "session.screen%u.slit.autoHide", screen);
  sprintf(class_lookup, "Session.screen%u.Slit.AutoHide", screen);
  sconfig.auto_hide = res.read(name_lookup, class_lookup, false);

  // general screen settings

  sprintf(name_lookup,  "session.screen%u.disableBindingsWithScrollLock",
          screen);
  sprintf(class_lookup, "Session.screen%u.disableBindingsWithScrollLock",
          screen);
  allow_scroll_lock = res.read(name_lookup, class_lookup, false);

  sprintf(name_lookup,  "session.screen%u.enableToolbar", screen);
  sprintf(class_lookup, "Session.screen%u.enableToolbar", screen);
  enable_toolbar = res.read(name_lookup, class_lookup, true);

  sprintf(name_lookup,  "session.screen%u.workspaces", screen);
  sprintf(class_lookup, "Session.screen%u.Workspaces", screen);
  workspace_count = res.read(name_lookup, class_lookup, 1);

  if (! workspaces.empty())
    workspaces.clear();

  sprintf(name_lookup,  "session.screen%u.workspaceNames", screen);
  sprintf(class_lookup, "Session.screen%u.WorkspaceNames", screen);
  tmp = res.read(name_lookup, class_lookup);
  if (! tmp.empty()) {
    std::string::const_iterator it = tmp.begin(), end = tmp.end();
    while (1) {
      std::string::const_iterator i = std::find(it, end, ',');
      std::string csv(it, i);
      workspaces.push_back(csv);
      it = i;
      if (it == end) break;
      ++it;
    }
  }
}


void ScreenResource::loadStyle(BScreen* screen, const std::string& style) {
  const bt::Display& display = screen->getBlackbox()->display();
  unsigned int screen_num = screen->screenNumber();

  // use the user selected style
  bt::Resource res(style);
  if (! res.valid())
    res.load(DEFAULTSTYLE);

  // load bevel and border widths
  border_width = res.read("borderWidth", "BorderWidth", 1);
  bevel_width = res.read("bevelWidth", "BevelWidth", 3);

  // load menu style
  bt::MenuStyle::get(*screen->getBlackbox(), screen_num)->load(res);

  // load fonts
  wstyle.font.setFontName(res.read("window.font", "Window.Font", "fixed"));
  tstyle.font.setFontName(res.read("toolbar.font", "Toolbar.Font", "fixed"));

  // load window config
  wstyle.iconify.load(screen_num, iconify_bits,
                      iconify_width, iconify_height);
  wstyle.maximize.load(screen_num, maximize_bits,
                       maximize_width, maximize_height);
  wstyle.restore.load(screen_num, restore_bits,
                      restore_width, restore_height);
  wstyle.close.load(screen_num, close_bits,
                    close_width, close_height);

  wstyle.t_focus =
    bt::textureResource(display, screen_num, res,
                        "window.title.focus", "Window.Title.Focus",
                        "white");
  wstyle.t_unfocus =
    bt::textureResource(display, screen_num, res,
                        "window.title.unfocus", "Window.Title.Unfocus",
                        "black");
  wstyle.l_focus =
    bt::textureResource(display, screen_num, res,
                        "window.label.focus", "Window.Label.Focus",
                        "white");
  wstyle.l_unfocus =
    bt::textureResource(display, screen_num, res,
                        "window.label.unfocus", "Window.Label.Unfocus",
                        "black");
  wstyle.h_focus =
    bt::textureResource(display, screen_num, res,
                        "window.handle.focus", "Window.Handle.Focus",
                        "white");
  wstyle.h_unfocus =
    bt::textureResource(display, screen_num, res,
                        "window.handle.unfocus", "Window.Handle.Unfocus",
                        "black");

  wstyle.handle_height =
    res.read("window.handleHeight", "Window.HandleHeight", 6);

  wstyle.bevel_width = bevel_width;

  // the height of the titlebar is based upon the height of the font being
  // used to display the window's title
  wstyle.label_height = bt::textHeight(screen_num, wstyle.font) + 2;
  wstyle.title_height = wstyle.label_height + wstyle.bevel_width * 2;
  wstyle.button_width = wstyle.label_height - 2;
  wstyle.grip_width = wstyle.button_width * 2;

  wstyle.g_focus = bt::textureResource(display, screen_num, res,
                                       "window.grip.focus",
                                       "Window.Grip.Focus",
                                       "white");
  wstyle.g_unfocus = bt::textureResource(display, screen_num, res,
                                         "window.grip.unfocus",
                                         "Window.Grip.Unfocus",
                                         "black");
  wstyle.b_focus =
    bt::textureResource(display, screen_num, res,
                        "window.button.focus", "Window.Button.Focus",
                        "white");
  wstyle.b_unfocus =
    bt::textureResource(display, screen_num, res,
                        "window.button.unfocus", "Window.Button.Unfocus",
                        "black");
  wstyle.b_pressed =
    bt::textureResource(display, screen_num, res,
                        "window.button.pressed", "Window.Button.Pressed",
                        "black");

  // we create the window.frame texture by hand because it exists only to
  // make the code cleaner and is not actually used for display
  bt::Color color;
  color = bt::Color::namedColor(display, screen_num,
                                res.read("window.frame.focusColor",
                                         "Window.Frame.FocusColor",
                                         "white"));
  wstyle.f_focus.setDescription("solid flat");
  wstyle.f_focus.setColor(color);

  color = bt::Color::namedColor(display, screen_num,
                                res.read("window.frame.unfocusColor",
                                         "Window.Frame.UnfocusColor",
                                         "white"));
  wstyle.f_unfocus.setDescription("solid flat");
  wstyle.f_unfocus.setColor(color);

  wstyle.frame_width = res.read("window.frameWidth", "Window.FrameWidth",
                                bevel_width);

  wstyle.l_text_focus =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.label.focus.textColor",
                                   "Window.Label.Focus.TextColor",
                                   "black"));
  wstyle.l_text_unfocus =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.label.unfocus.textColor",
                                   "Window.Label.Unfocus.TextColor",
                                   "white"));
  wstyle.b_pic_focus =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.button.focus.picColor",
                                   "Window.Button.Focus.PicColor",
                                   "black"));
  wstyle.b_pic_unfocus =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.button.unfocus.picColor",
                                   "Window.Button.Unfocus.PicColor",
                                   "white"));

  wstyle.alignment =
    bt::alignResource(res, "window.alignment", "Window.Alignment");

  // load toolbar config
  tstyle.toolbar = bt::textureResource(display, screen_num, res,
                                       "toolbar", "Toolbar",
                                       "black");
  tstyle.label = bt::textureResource(display, screen_num, res,
                                     "toolbar.label", "Toolbar.Label",
                                     "black");
  tstyle.window =
    bt::textureResource(display, screen_num, res,
                        "toolbar.windowLabel", "Toolbar.WindowLabel",
                        "black");
  tstyle.button =
    bt::textureResource(display, screen_num, res,
                        "toolbar.button", "Toolbar.Button",
                        "white");
  tstyle.pressed =
    bt::textureResource(display, screen_num, res,
                        "toolbar.button.pressed", "Toolbar.Button.Pressed",
                        "black");
  tstyle.clock =
    bt::textureResource(display, screen_num, res,
                        "toolbar.clock", "Toolbar.Clock", "black");

  tstyle.l_text =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.label.textColor",
                                   "Toolbar.Label.TextColor",
                                   "white"));
  tstyle.w_text =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.windowLabel.textColor",
                                   "Toolbar.WindowLabel.TextColor",
                                   "white"));
  tstyle.c_text =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.clock.textColor",
                                   "Toolbar.Clock.TextColor",
                                   "white"));
  tstyle.b_pic =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.button.picColor",
                                   "Toolbar.Button.PicColor",
                                   "black"));
  tstyle.alignment =
    bt::alignResource(res, "toolbar.alignment", "Toolbar.Alignment");

  border_color = bt::Color::namedColor(display, screen_num,
                                       res.read("borderColor",
                                                "BorderColor",
                                                "black"));

  root_command = res.read("rootCommand", "RootCommand");

  // sanity checks
  if (wstyle.t_focus.texture() == bt::Texture::Parent_Relative)
    wstyle.t_focus = wstyle.f_focus;
  if (wstyle.t_unfocus.texture() == bt::Texture::Parent_Relative)
    wstyle.t_unfocus = wstyle.f_unfocus;
  if (wstyle.h_focus.texture() == bt::Texture::Parent_Relative)
    wstyle.h_focus = wstyle.f_focus;
  if (wstyle.h_unfocus.texture() == bt::Texture::Parent_Relative)
    wstyle.h_unfocus = wstyle.f_unfocus;

  if (tstyle.toolbar.texture() == bt::Texture::Parent_Relative) {
    tstyle.toolbar.setTexture(bt::Texture::Flat | bt::Texture::Solid);
    tstyle.toolbar.setColor(bt::Color(0, 0, 0));
  }
}


const std::string& ScreenResource::nameOfWorkspace(unsigned int i) const {
  // handle both requests for new workspaces beyond what we started with
  // and for those that lack a name
  if (i > workspace_count || i >= workspaces.size())
    return empty_string;

  return workspaces[i];
}


void ScreenResource::saveWorkspaceName(unsigned int w,
                                       const std::string& name) {
  if (w < workspaces.size()) {
    workspaces[w] = name;
  } else {
    workspaces.reserve(w + 1);
    workspace_count = w + 1;
    workspaces.insert(workspaces.begin() + w, name);
  }
}
