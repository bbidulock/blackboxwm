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

  unsigned int maxcolors = res.read("session.maximumColors",
                                    "Session.MaximumColors",
                                    ~0u);
  if (maxcolors != ~0u)
    bt::Image::setMaximumColors(maxcolors);

  style_file = bt::expandTilde(res.read("session.styleFile",
                                        "Session.StyleFile", DEFAULTSTYLE));

  double_click_interval = res.read("session.doubleClickInterval",
                                   "Session.DoubleClickInterval", 250l);

  auto_raise_delay.tv_usec = res.read("session.autoRaiseDelay",
                                      "Session.AutoRaiseDelay", 400l);

  auto_raise_delay.tv_sec = auto_raise_delay.tv_usec / 1000;
  auto_raise_delay.tv_usec -= (auto_raise_delay.tv_sec * 1000);
  auto_raise_delay.tv_usec *= 1000;

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
  cursor.resize_top_left =
    XCreateFontCursor(blackbox.XDisplay(), XC_top_left_corner);
  cursor.resize_bottom_left =
    XCreateFontCursor(blackbox.XDisplay(), XC_bottom_left_corner);
  cursor.resize_top_right =
    XCreateFontCursor(blackbox.XDisplay(), XC_top_right_corner);
  cursor.resize_bottom_right =
    XCreateFontCursor(blackbox.XDisplay(), XC_bottom_right_corner);

  for (unsigned int i = 0; i < blackbox.screenCount(); ++i)
    screen_resources[i].load(res, i);
}


void BlackboxResource::save(Blackbox& blackbox) {
  bt::Resource res(rc_file);

  res.write("session.menuFile", menuFilename());

  res.write("session.styleFile", styleFilename());

  res.write("session.maximumColors",  bt::Image::maximumColors());

  res.write("session.doubleClickInterval", double_click_interval);

  res.write("session.autoRaiseDelay", ((auto_raise_delay.tv_sec * 1000ul) +
                                       (auto_raise_delay.tv_usec / 1000ul)));

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

  sprintf(rc_string, "session.screen%u.opaqueResize", number);
  res.write(rc_string, wconfig.opaque_resize);

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

  std::vector<bt::ustring>::const_iterator it = workspaces.begin(),
    end = workspaces.end();
  bt::ustring save_string = *it++;
  for (; it != end; ++it) {
    save_string += ',';
    save_string += *it;
  }

  sprintf(rc_string, "session.screen%u.workspaceNames", number);
  res.write(rc_string, bt::toLocale(save_string).c_str());

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

  sprintf(name_lookup,  "session.screen%u.opaqueResize", screen);
  sprintf(class_lookup, "Session.screen%u.OpaqueResize", screen);
  wconfig.opaque_resize = res.read(name_lookup, class_lookup, false);

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
  bt::ustring workspace_names =
    bt::toUnicode(res.read(name_lookup, class_lookup));
  if (!workspace_names.empty()) {
    bt::ustring::const_iterator it = workspace_names.begin();
    const bt::ustring::const_iterator end = workspace_names.end();
    for (;;) {
      const bt::ustring::const_iterator i =
        std::find(it, end, static_cast<bt::ustring::value_type>(','));
      workspaces.push_back(bt::ustring(it, i));
      it = i;
      if (it == end) break;
      ++it;
    }
  }
}


void ScreenResource::loadStyle(BScreen* screen, const std::string& style) {
  const bt::Display& display = screen->blackbox()->display();
  unsigned int screen_num = screen->screenNumber();

  // use the user selected style
  bt::Resource res(style);
  if (! res.valid())
    res.load(DEFAULTSTYLE);

  // load menu style
  bt::MenuStyle::get(*screen->blackbox(), screen_num)->load(res);

  // load window style
  wstyle.font.setFontName(res.read("window.font", "Window.Font"));

  wstyle.iconify.load(screen_num, iconify_bits,
                      iconify_width, iconify_height);
  wstyle.maximize.load(screen_num, maximize_bits,
                       maximize_width, maximize_height);
  wstyle.restore.load(screen_num, restore_bits,
                      restore_width, restore_height);
  wstyle.close.load(screen_num, close_bits,
                    close_width, close_height);

  // focused window style
  wstyle.focus.text =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.label.focus.textColor",
                                   "Window.Label.Focus.TextColor",
                                   "black"));
  wstyle.focus.foreground =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.button.focus.foregroundColor",
                                   "Window.Button.Focus.Foreground",
                                   res.read("window.button.focus.picColor",
                                            "Window.Button.Focus.PicColor",
                                            "black")));
  wstyle.focus.title =
    bt::textureResource(display, screen_num, res,
                        "window.title.focus",
                        "Window.Title.Focus",
                        "white");
  wstyle.focus.label =
    bt::textureResource(display, screen_num, res,
                        "window.label.focus",
                        "Window.Label.Focus",
                        "white");
  wstyle.focus.button =
    bt::textureResource(display, screen_num, res,
                        "window.button.focus",
                        "Window.Button.Focus",
                        "white");
  wstyle.focus.handle =
    bt::textureResource(display, screen_num, res,
                        "window.handle.focus",
                        "Window.Handle.Focus",
                        "white");
  wstyle.focus.grip =
    bt::textureResource(display, screen_num, res,
                        "window.grip.focus",
                        "Window.Grip.Focus",
                        "white");

  // unfocused window style
  wstyle.unfocus.text =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.label.unfocus.textColor",
                                   "Window.Label.Unfocus.TextColor",
                                   "white"));
  wstyle.unfocus.foreground =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.button.unfocus.foregroundColor",
                                   "Window.Button.Unfocus.Foreground",
                                   res.read("window.button.unfocus.picColor",
                                            "Window.Button.Unfocus.PicColor",
                                            "white")));
  wstyle.unfocus.title =
    bt::textureResource(display, screen_num, res,
                        "window.title.unfocus",
                        "Window.Title.Unfocus",
                        "black");
  wstyle.unfocus.label =
    bt::textureResource(display, screen_num, res,
                        "window.label.unfocus",
                        "Window.Label.Unfocus",
                        "black");
  wstyle.unfocus.button =
    bt::textureResource(display, screen_num, res,
                        "window.button.unfocus",
                        "Window.Button.Unfocus",
                        "black");
  wstyle.unfocus.handle =
    bt::textureResource(display, screen_num, res,
                        "window.handle.unfocus",
                        "Window.Handle.Unfocus",
                        "black");
  wstyle.unfocus.grip =
    bt::textureResource(display, screen_num, res,
                        "window.grip.unfocus",
                        "Window.Grip.Unfocus",
                        "black");

  wstyle.frame_border =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.frame.borderColor",
                                   "Window.Frame.BorderColor",
                                   "black"));
  wstyle.pressed =
    bt::textureResource(display, screen_num, res,
                        "window.button.pressed",
                        "Window.Button.Pressed",
                        "black");

  wstyle.alignment =
    bt::alignResource(res, "window.alignment", "Window.Alignment");

  wstyle.title_margin =
    res.read("window.title.marginWidth", "Window.Title.MarginWidth", 2);
  wstyle.label_margin =
    res.read("window.label.marginWidth", "Window.Label.MarginWidth", 2);
  wstyle.button_margin =
    res.read("window.button.marginWidth", "Window.Button.MarginWidth", 2);
  wstyle.frame_border_width =
    res.read("window.frame.borderWidth", "Window.Frame.BorderWidth", 1);
  wstyle.handle_height =
    res.read("window.handleHeight", "Window.HandleHeight", 6);

  // the height of the titlebar is based upon the height of the font being
  // used to display the window's title
  wstyle.button_width =
    std::max(std::max(std::max(std::max(wstyle.iconify.width(),
                                        wstyle.iconify.height()),
                               std::max(wstyle.maximize.width(),
                                        wstyle.maximize.height())),
                      std::max(wstyle.restore.width(),
                               wstyle.restore.height())),
             std::max(wstyle.close.width(),
                      wstyle.close.height())) +
    ((std::max(wstyle.focus.button.borderWidth(),
               wstyle.unfocus.button.borderWidth()) +
      wstyle.button_margin) * 2);
  wstyle.label_height =
    std::max(bt::textHeight(screen_num, wstyle.font) +
             ((std::max(wstyle.focus.label.borderWidth(),
                        wstyle.unfocus.label.borderWidth()) +
               wstyle.label_margin) * 2),
             wstyle.button_width);
  wstyle.button_width = std::max(wstyle.button_width, wstyle.label_height);
  wstyle.title_height =
    wstyle.label_height +
    ((std::max(wstyle.focus.title.borderWidth(),
               wstyle.unfocus.title.borderWidth()) +
      wstyle.title_margin) * 2);
  wstyle.grip_width = (wstyle.button_width * 2);
  wstyle.handle_height += (std::max(wstyle.focus.handle.borderWidth(),
                                    wstyle.unfocus.handle.borderWidth()) * 2);

  // load toolbar style
  toolbar_style.font.setFontName(res.read("toolbar.font", "Toolbar.Font"));

  toolbar_style.toolbar =
    bt::textureResource(display, screen_num, res,
                        "toolbar",
                        "Toolbar",
                        "white");
  toolbar_style.slabel =
    bt::textureResource(display, screen_num, res,
                        "toolbar.label",
                        "Toolbar.Label",
                        "white");
  toolbar_style.wlabel =
    bt::textureResource(display, screen_num, res,
                        "toolbar.windowLabel",
                        "Toolbar.Label",
                        "white");
  toolbar_style.button =
    bt::textureResource(display, screen_num, res,
                        "toolbar.button",
                        "Toolbar.Button",
                        "white");
  toolbar_style.pressed =
    bt::textureResource(display, screen_num, res,
                        "toolbar.button.pressed",
                        "Toolbar.Button.Pressed",
                        "black");

  toolbar_style.clock =
    bt::textureResource(display, screen_num, res,
                        "toolbar.clock",
                        "Toolbar.Label",
                        "white");

  toolbar_style.slabel_text =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.label.textColor",
                                   "Toolbar.Label.TextColor",
                                   "black"));
  toolbar_style.wlabel_text =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.windowLabel.textColor",
                                   "Toolbar.Label.TextColor",
                                   "black"));
  toolbar_style.clock_text =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.clock.textColor",
                                   "Toolbar.Label.TextColor",
                                   "white"));
  toolbar_style.foreground =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.button.foregroundColor",
                                   "Toolbar.Button.Foreground",
                                   res.read("toolbar.button.picColor",
                                            "Toolbar.Button.PicColor",
                                            "black")));
  toolbar_style.alignment =
    bt::alignResource(res, "toolbar.alignment", "Toolbar.Alignment");

  toolbar_style.frame_margin =
    res.read("toolbar.marginWidth", "Toolbar.MarginWidth", 2);
  toolbar_style.label_margin =
    res.read("toolbar.label.marginWidth", "Toolbar.Label.MarginWidth", 2);
  toolbar_style.button_margin =
    res.read("toolbar.button.marginWidth", "Toolbar.Button.MarginWidth", 2);

  const bt::Bitmap &left = bt::Bitmap::leftArrow(screen_num),
                  &right = bt::Bitmap::rightArrow(screen_num);
  toolbar_style.button_width =
    std::max(std::max(left.width(), left.height()),
             std::max(right.width(), right.height()))
    + ((toolbar_style.button.borderWidth() + toolbar_style.button_margin) * 2);
  toolbar_style.label_height =
    std::max(bt::textHeight(screen_num, toolbar_style.font)
             + ((std::max(std::max(toolbar_style.slabel.borderWidth(),
                                   toolbar_style.wlabel.borderWidth()),
                          toolbar_style.clock.borderWidth())
                 + toolbar_style.label_margin) * 2),
             toolbar_style.button_width);
  toolbar_style.button_width = std::max(toolbar_style.button_width,
                                        toolbar_style.label_height);
  toolbar_style.toolbar_height = toolbar_style.label_height
                                 + ((toolbar_style.toolbar.borderWidth()
                                     + toolbar_style.frame_margin) * 2);
  toolbar_style.hidden_height =
    std::max(toolbar_style.toolbar.borderWidth()
             + toolbar_style.frame_margin, 1u);

  // load slit style
  slit_style.slit = bt::textureResource(display, screen_num, res,
                                        "slit",
                                        "Slit",
                                        "white");
  slit_style.margin = res.read("slit.marginWidth", "Slit.MarginWidth", 2);

  root_command = res.read("rootCommand", "RootCommand");

  // sanity checks
  bt::Texture flat_black;
  flat_black.setDescription("flat solid");
  flat_black.setColor(bt::Color(0, 0, 0));

  if (wstyle.focus.title.texture() == bt::Texture::Parent_Relative)
    wstyle.focus.title = flat_black;
  if (wstyle.unfocus.title.texture() == bt::Texture::Parent_Relative)
    wstyle.unfocus.title = flat_black;
  if (wstyle.focus.handle.texture() == bt::Texture::Parent_Relative)
    wstyle.focus.handle = flat_black;
  if (wstyle.unfocus.handle.texture() == bt::Texture::Parent_Relative)
    wstyle.unfocus.handle = flat_black;

  if (toolbar_style.toolbar.texture() == bt::Texture::Parent_Relative)
    toolbar_style.toolbar = flat_black;

  if (slit_style.slit.texture() == bt::Texture::Parent_Relative)
    slit_style.slit = flat_black;
}


const bt::ustring &ScreenResource::nameOfWorkspace(unsigned int i) const {
  // handle both requests for new workspaces beyond what we started with
  // and for those that lack a name
  if (i > workspace_count || i >= workspaces.size()) {
    static const bt::ustring empty_string;
    return empty_string;
  }
  return workspaces[i];
}


void ScreenResource::saveWorkspaceName(unsigned int w,
                                       const bt::ustring &name) {
  if (w < workspaces.size()) {
    workspaces[w] = name;
  } else {
    workspaces.reserve(w + 1);
    workspace_count = w + 1;
    workspaces.insert(workspaces.begin() + w, name);
  }
}
