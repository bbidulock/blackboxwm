// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// ScreenResource.ccfor Blackbox - an X11 Window manager
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

#include "ScreenResource.hh"

#include "Screen.hh"
#include "Slit.hh"
#include "Toolbar.hh"

#include <Menu.hh>
#include <Resource.hh>

#include <assert.h>


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


void ScreenResource::save(bt::Resource& res, BScreen* screen) {
  char rc_string[128];
  char *placement = (char *) 0;
  unsigned int number = screen->screenNumber();

  switch (_slitOptions.placement) {
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
  res.write(rc_string, (_slitOptions.direction == Slit::Horizontal) ?
            "Horizontal" : "Vertical");

  sprintf(rc_string, "session.screen%u.slit.onTop", number);
  res.write(rc_string, _slitOptions.always_on_top);

  sprintf(rc_string, "session.screen%u.slit.autoHide", number);
  res.write(rc_string, _slitOptions.auto_hide);


  sprintf(rc_string,  "session.screen%u.enableToolbar", number);
  res.write(rc_string, _toolbarOptions.enabled);

  sprintf(rc_string, "session.screen%u.toolbar.onTop", number);
  res.write(rc_string, _toolbarOptions.always_on_top);

  sprintf(rc_string, "session.screen%u.toolbar.autoHide", number);
  res.write(rc_string, _toolbarOptions.auto_hide);

  switch (_toolbarOptions.placement) {
  case Toolbar::TopLeft: placement = "TopLeft"; break;
  case Toolbar::BottomLeft: placement = "BottomLeft"; break;
  case Toolbar::TopCenter: placement = "TopCenter"; break;
  case Toolbar::TopRight: placement = "TopRight"; break;
  case Toolbar::BottomRight: placement = "BottomRight"; break;
  case Toolbar::BottomCenter: default: placement = "BottomCenter"; break;
  }

  sprintf(rc_string, "session.screen%u.toolbar.placement", number);
  res.write(rc_string, placement);

  sprintf(rc_string, "session.screen%u.workspaces", number);
  res.write(rc_string, workspace_count);

  std::vector<bt::ustring>::const_iterator it = workspace_names.begin(),
                                          end = workspace_names.end();
  bt::ustring save_string = *it++;
  for (; it != end; ++it) {
    save_string += ',';
    save_string += *it;
  }

  sprintf(rc_string, "session.screen%u.workspaceNames", number);
  res.write(rc_string, bt::toLocale(save_string).c_str());

  // these options can not be modified at runtime currently

  sprintf(rc_string, "session.screen%u.toolbar.widthPercent", number);
  res.write(rc_string, _toolbarOptions.width_percent);

  sprintf(rc_string, "session.screen%u.strftimeFormat", number);
  res.write(rc_string, _toolbarOptions.strftime_format.c_str());
}

void ScreenResource::load(bt::Resource& res, unsigned int screen) {
  char name_lookup[128], class_lookup[128];
  std::string str;

  // toolbar settings
  sprintf(name_lookup,  "session.screen%u.enableToolbar", screen);
  sprintf(class_lookup, "Session.screen%u.enableToolbar", screen);
  _toolbarOptions.enabled = res.read(name_lookup, class_lookup, true);

  sprintf(name_lookup,  "session.screen%u.toolbar.widthPercent", screen);
  sprintf(class_lookup, "Session.screen%u.Toolbar.WidthPercent", screen);
  _toolbarOptions.width_percent = res.read(name_lookup, class_lookup, 66);

  sprintf(name_lookup, "session.screen%u.toolbar.placement", screen);
  sprintf(class_lookup, "Session.screen%u.Toolbar.Placement", screen);
  str = res.read(name_lookup, class_lookup, "BottomCenter");
  if (! strcasecmp(str.c_str(), "TopLeft"))
    _toolbarOptions.placement = Toolbar::TopLeft;
  else if (! strcasecmp(str.c_str(), "BottomLeft"))
    _toolbarOptions.placement = Toolbar::BottomLeft;
  else if (! strcasecmp(str.c_str(), "TopCenter"))
    _toolbarOptions.placement = Toolbar::TopCenter;
  else if (! strcasecmp(str.c_str(), "TopRight"))
    _toolbarOptions.placement = Toolbar::TopRight;
  else if (! strcasecmp(str.c_str(), "BottomRight"))
    _toolbarOptions.placement = Toolbar::BottomRight;
  else
    _toolbarOptions.placement = Toolbar::BottomCenter;

  sprintf(name_lookup,  "session.screen%u.toolbar.onTop", screen);
  sprintf(class_lookup, "Session.screen%u.Toolbar.OnTop", screen);
  _toolbarOptions.always_on_top = res.read(name_lookup, class_lookup, false);

  sprintf(name_lookup,  "session.screen%u.toolbar.autoHide", screen);
  sprintf(class_lookup, "Session.screen%u.Toolbar.autoHide", screen);
  _toolbarOptions.auto_hide = res.read(name_lookup, class_lookup, false);

  sprintf(name_lookup,  "session.screen%u.strftimeFormat", screen);
  sprintf(class_lookup, "Session.screen%u.StrftimeFormat", screen);
  _toolbarOptions.strftime_format =
    res.read(name_lookup, class_lookup, "%I:%M %p");

  // slit settings
  sprintf(name_lookup, "session.screen%u.slit.placement", screen);
  sprintf(class_lookup, "Session.screen%u.Slit.Placement", screen);
  str = res.read(name_lookup, class_lookup, "CenterRight");
  if (! strcasecmp(str.c_str(), "TopLeft"))
    _slitOptions.placement = Slit::TopLeft;
  else if (! strcasecmp(str.c_str(), "CenterLeft"))
    _slitOptions.placement = Slit::CenterLeft;
  else if (! strcasecmp(str.c_str(), "BottomLeft"))
    _slitOptions.placement = Slit::BottomLeft;
  else if (! strcasecmp(str.c_str(), "TopCenter"))
    _slitOptions.placement = Slit::TopCenter;
  else if (! strcasecmp(str.c_str(), "BottomCenter"))
    _slitOptions.placement = Slit::BottomCenter;
  else if (! strcasecmp(str.c_str(), "TopRight"))
    _slitOptions.placement = Slit::TopRight;
  else if (! strcasecmp(str.c_str(), "BottomRight"))
    _slitOptions.placement = Slit::BottomRight;
  else
    _slitOptions.placement = Slit::CenterRight;

  sprintf(name_lookup, "session.screen%u.slit.direction", screen);
  sprintf(class_lookup, "Session.screen%u.Slit.Direction", screen);
  str = res.read(name_lookup, class_lookup, "Vertical");
  if (! strcasecmp(str.c_str(), "Horizontal"))
    _slitOptions.direction = Slit::Horizontal;
  else
    _slitOptions.direction = Slit::Vertical;

  sprintf(name_lookup, "session.screen%u.slit.onTop", screen);
  sprintf(class_lookup, "Session.screen%u.Slit.OnTop", screen);
  _slitOptions.always_on_top = res.read(name_lookup, class_lookup, false);

  sprintf(name_lookup, "session.screen%u.slit.autoHide", screen);
  sprintf(class_lookup, "Session.screen%u.Slit.AutoHide", screen);
  _slitOptions.auto_hide = res.read(name_lookup, class_lookup, false);

  // general screen settings

  sprintf(name_lookup,  "session.screen%u.workspaces", screen);
  sprintf(class_lookup, "Session.screen%u.Workspaces", screen);
  workspace_count = res.read(name_lookup, class_lookup, 4);

  if (! workspace_names.empty())
    workspace_names.clear();

  sprintf(name_lookup,  "session.screen%u.workspaceNames", screen);
  sprintf(class_lookup, "Session.screen%u.WorkspaceNames", screen);
  bt::ustring ustr = bt::toUnicode(res.read(name_lookup, class_lookup));
  if (!ustr.empty()) {
    bt::ustring::const_iterator it = ustr.begin();
    const bt::ustring::const_iterator end = ustr.end();
    for (;;) {
      const bt::ustring::const_iterator i =
        std::find(it, end, static_cast<bt::ustring::value_type>(','));
      workspace_names.push_back(bt::ustring(it, i));
      it = i;
      if (it == end)
        break;
      ++it;
    }
  }
}


void ScreenResource::loadStyle(BScreen* screen, const std::string& style) {
  const bt::Display& display = screen->blackbox()->display();
  unsigned int screen_num = screen->screenNumber();

  // use the user selected style
  bt::Resource res(style);
  if (!res.valid())
    res.load(DEFAULTSTYLE);

  // load menu style
  bt::MenuStyle::get(*screen->blackbox(), screen_num)->load(res);

  // load window style
  _windowStyle.font.setFontName(res.read("window.font", "Window.Font"));

  _windowStyle.iconify.load(screen_num, iconify_bits,
                      iconify_width, iconify_height);
  _windowStyle.maximize.load(screen_num, maximize_bits,
                       maximize_width, maximize_height);
  _windowStyle.restore.load(screen_num, restore_bits,
                      restore_width, restore_height);
  _windowStyle.close.load(screen_num, close_bits,
                    close_width, close_height);

  // focused window style
  _windowStyle.focus.text =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.label.focus.textColor",
                                   "Window.Label.Focus.TextColor",
                                   "black"));
  _windowStyle.focus.foreground =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.button.focus.foregroundColor",
                                   "Window.Button.Focus.ForegroundColor",
                                   res.read("window.button.focus.picColor",
                                            "Window.Button.Focus.PicColor",
                                            "black")));
  _windowStyle.focus.title =
    bt::textureResource(display, screen_num, res,
                        "window.title.focus",
                        "Window.Title.Focus",
                        "white");
  _windowStyle.focus.label =
    bt::textureResource(display, screen_num, res,
                        "window.label.focus",
                        "Window.Label.Focus",
                        "white");
  _windowStyle.focus.button =
    bt::textureResource(display, screen_num, res,
                        "window.button.focus",
                        "Window.Button.Focus",
                        "white");
  _windowStyle.focus.handle =
    bt::textureResource(display, screen_num, res,
                        "window.handle.focus",
                        "Window.Handle.Focus",
                        "white");
  _windowStyle.focus.grip =
    bt::textureResource(display, screen_num, res,
                        "window.grip.focus",
                        "Window.Grip.Focus",
                        "white");
  _windowStyle.focus.frame_border =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.frame.focus.borderColor",
                                   "Window.Frame.Focus.BorderColor",
                                   "white"));

  // unfocused window style
  _windowStyle.unfocus.text =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.label.unfocus.textColor",
                                   "Window.Label.Unfocus.TextColor",
                                   "white"));
  _windowStyle.unfocus.foreground =
    bt::Color::namedColor(display, screen_num,
                          res.read("window.button.unfocus.foregroundColor",
                                   "Window.Button.Unfocus.ForegroundColor",
                                   res.read("window.button.unfocus.picColor",
                                            "Window.Button.Unfocus.PicColor",
                                            "white")));
  _windowStyle.unfocus.title =
    bt::textureResource(display, screen_num, res,
                        "window.title.unfocus",
                        "Window.Title.Unfocus",
                        "black");
  _windowStyle.unfocus.label =
    bt::textureResource(display, screen_num, res,
                        "window.label.unfocus",
                        "Window.Label.Unfocus",
                        "black");
  _windowStyle.unfocus.button =
    bt::textureResource(display, screen_num, res,
                        "window.button.unfocus",
                        "Window.Button.Unfocus",
                        "black");
  _windowStyle.unfocus.handle =
    bt::textureResource(display, screen_num, res,
                        "window.handle.unfocus",
                        "Window.Handle.Unfocus",
                        "black");
  _windowStyle.unfocus.grip =
    bt::textureResource(display, screen_num, res,
                        "window.grip.unfocus",
                        "Window.Grip.Unfocus",
                        "black");
  _windowStyle.unfocus.frame_border =
    bt::Color::namedColor(display, screen_num,
			  res.read("window.frame.unfocus.borderColor",
                                   "Window.Frame.Unfocus.BorderColor",
                                   "black"));

  _windowStyle.pressed =
    bt::textureResource(display, screen_num, res,
                        "window.button.pressed",
                        "Window.Button.Pressed",
                        "black");

  _windowStyle.alignment =
    bt::alignResource(res, "window.alignment", "Window.Alignment");

  _windowStyle.title_margin =
    res.read("window.title.marginWidth", "Window.Title.MarginWidth", 2);
  _windowStyle.label_margin =
    res.read("window.label.marginWidth", "Window.Label.MarginWidth", 2);
  _windowStyle.button_margin =
    res.read("window.button.marginWidth", "Window.Button.MarginWidth", 2);
  _windowStyle.frame_border_width =
    res.read("window.frame.borderWidth", "Window.Frame.BorderWidth", 1);
  _windowStyle.handle_height =
    res.read("window.handleHeight", "Window.HandleHeight", 6);

  // the height of the titlebar is based upon the height of the font being
  // used to display the window's title
  _windowStyle.button_width =
    std::max(std::max(std::max(std::max(_windowStyle.iconify.width(),
                                        _windowStyle.iconify.height()),
                               std::max(_windowStyle.maximize.width(),
                                        _windowStyle.maximize.height())),
                      std::max(_windowStyle.restore.width(),
                               _windowStyle.restore.height())),
             std::max(_windowStyle.close.width(),
                      _windowStyle.close.height())) +
    ((std::max(_windowStyle.focus.button.borderWidth(),
               _windowStyle.unfocus.button.borderWidth()) +
      _windowStyle.button_margin) * 2);
  _windowStyle.label_height =
    std::max(bt::textHeight(screen_num, _windowStyle.font) +
             ((std::max(_windowStyle.focus.label.borderWidth(),
                        _windowStyle.unfocus.label.borderWidth()) +
               _windowStyle.label_margin) * 2),
             _windowStyle.button_width);
  _windowStyle.button_width = std::max(_windowStyle.button_width,
                                       _windowStyle.label_height);
  _windowStyle.title_height =
    _windowStyle.label_height +
    ((std::max(_windowStyle.focus.title.borderWidth(),
               _windowStyle.unfocus.title.borderWidth()) +
      _windowStyle.title_margin) * 2);
  _windowStyle.grip_width = (_windowStyle.button_width * 2);
  _windowStyle.handle_height +=
    (std::max(_windowStyle.focus.handle.borderWidth(),
              _windowStyle.unfocus.handle.borderWidth()) * 2);

  // load toolbar style
  _toolbarStyle.font.setFontName(res.read("toolbar.font", "Toolbar.Font"));

  _toolbarStyle.toolbar =
    bt::textureResource(display, screen_num, res,
                        "toolbar",
                        "Toolbar",
                        "white");
  _toolbarStyle.slabel =
    bt::textureResource(display, screen_num, res,
                        "toolbar.label",
                        "Toolbar.Label",
                        "white");
  _toolbarStyle.wlabel =
    bt::textureResource(display, screen_num, res,
                        "toolbar.windowLabel",
                        "Toolbar.Label",
                        "white");
  _toolbarStyle.button =
    bt::textureResource(display, screen_num, res,
                        "toolbar.button",
                        "Toolbar.Button",
                        "white");
  _toolbarStyle.pressed =
    bt::textureResource(display, screen_num, res,
                        "toolbar.button.pressed",
                        "Toolbar.Button.Pressed",
                        "black");

  _toolbarStyle.clock =
    bt::textureResource(display, screen_num, res,
                        "toolbar.clock",
                        "Toolbar.Label",
                        "white");

  _toolbarStyle.slabel_text =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.label.textColor",
                                   "Toolbar.Label.TextColor",
                                   "black"));
  _toolbarStyle.wlabel_text =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.windowLabel.textColor",
                                   "Toolbar.Label.TextColor",
                                   "black"));
  _toolbarStyle.clock_text =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.clock.textColor",
                                   "Toolbar.Label.TextColor",
                                   "black"));
  _toolbarStyle.foreground =
    bt::Color::namedColor(display, screen_num,
                          res.read("toolbar.button.foregroundColor",
                                   "Toolbar.Button.ForegroundColor",
                                   res.read("toolbar.button.picColor",
                                            "Toolbar.Button.PicColor",
                                            "black")));
  _toolbarStyle.alignment =
    bt::alignResource(res, "toolbar.alignment", "Toolbar.Alignment");

  _toolbarStyle.frame_margin =
    res.read("toolbar.marginWidth", "Toolbar.MarginWidth", 2);
  _toolbarStyle.label_margin =
    res.read("toolbar.label.marginWidth", "Toolbar.Label.MarginWidth", 2);
  _toolbarStyle.button_margin =
    res.read("toolbar.button.marginWidth", "Toolbar.Button.MarginWidth", 2);

  const bt::Bitmap &left = bt::Bitmap::leftArrow(screen_num),
                  &right = bt::Bitmap::rightArrow(screen_num);
  _toolbarStyle.button_width =
    std::max(std::max(left.width(), left.height()),
             std::max(right.width(), right.height()))
    + ((_toolbarStyle.button.borderWidth() + _toolbarStyle.button_margin) * 2);
  _toolbarStyle.label_height =
    std::max(bt::textHeight(screen_num, _toolbarStyle.font)
             + ((std::max(std::max(_toolbarStyle.slabel.borderWidth(),
                                   _toolbarStyle.wlabel.borderWidth()),
                          _toolbarStyle.clock.borderWidth())
                 + _toolbarStyle.label_margin) * 2),
             _toolbarStyle.button_width);
  _toolbarStyle.button_width = std::max(_toolbarStyle.button_width,
                                        _toolbarStyle.label_height);
  _toolbarStyle.toolbar_height = _toolbarStyle.label_height
                                 + ((_toolbarStyle.toolbar.borderWidth()
                                     + _toolbarStyle.frame_margin) * 2);
  _toolbarStyle.hidden_height =
    std::max(_toolbarStyle.toolbar.borderWidth()
             + _toolbarStyle.frame_margin, 1u);

  // load slit style
  _slitStyle.slit = bt::textureResource(display,
                                        screen_num,
                                        res,
                                        "slit",
                                        "Slit",
                                        _toolbarStyle.toolbar);
  _slitStyle.margin = res.read("slit.marginWidth", "Slit.MarginWidth", 2);

  const std::string rc_file = screen->blackbox()->resource().rcFilename();
  root_command =
      bt::Resource(rc_file).read("rootCommand",
                                 "RootCommand",
                                 res.read("rootCommand",
                                          "RootCommand"));

  // sanity checks
  bt::Texture flat_black;
  flat_black.setDescription("flat solid");
  flat_black.setColor1(bt::Color(0, 0, 0));

  if (_windowStyle.focus.title.texture() == bt::Texture::Parent_Relative)
    _windowStyle.focus.title = flat_black;
  if (_windowStyle.unfocus.title.texture() == bt::Texture::Parent_Relative)
    _windowStyle.unfocus.title = flat_black;
  if (_windowStyle.focus.handle.texture() == bt::Texture::Parent_Relative)
    _windowStyle.focus.handle = flat_black;
  if (_windowStyle.unfocus.handle.texture() == bt::Texture::Parent_Relative)
    _windowStyle.unfocus.handle = flat_black;

  if (_toolbarStyle.toolbar.texture() == bt::Texture::Parent_Relative)
    _toolbarStyle.toolbar = flat_black;

  if (_slitStyle.slit.texture() == bt::Texture::Parent_Relative)
    _slitStyle.slit = flat_black;
}

const bt::ustring ScreenResource::workspaceName(unsigned int i) const {
  // handle both requests for new workspaces beyond what we started with
  // and for those that lack a name
  if (i > workspace_count || i >= workspace_names.size())
    return bt::ustring();
  return workspace_names[i];
}

void ScreenResource::setWorkspaceName(unsigned int i,
                                      const bt::ustring &name) {
    if (i >= workspace_names.size()) {
        workspace_names.reserve(i + 1);
        workspace_names.insert(workspace_names.begin() + i, name);
    } else {
        workspace_names[i] = name;
    }
}
