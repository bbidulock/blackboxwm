// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// bstyleconvert - a Blackbox style conversion utility
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

#include "Resource.hh"
#include "Util.hh"

extern "C" {
#include <assert.h>
}

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>


std::string justifyToAlignment(const std::string &justify)
{
  std::string alignment = bt::tolower(justify);
  std::string::size_type at = alignment.find("justify");
  if (at != std::string::npos)
    alignment.resize(at);
  return alignment;
}


struct OldTexture {
  std::string appearance;
  std::string color1, color2;
};

struct OldStyle {
  struct {
    std::string label_focus_textColor, label_unfocus_textColor;
    std::string button_focus_picColor, button_unfocus_picColor;
    std::string frame_focusColor, frame_unfocusColor;

    OldTexture title_focus, title_unfocus;
    OldTexture label_focus, label_unfocus;
    OldTexture handle_focus, handle_unfocus;
    OldTexture button_focus, button_unfocus, button_pressed;
    OldTexture grip_focus, grip_unfocus;

    std::string font;
    std::string justify;
  } window;

  struct {
    std::string label_textColor;
    std::string windowLabel_textColor;
    std::string clock_textColor;
    std::string button_picColor;

    OldTexture toolbar;
    OldTexture label;
    OldTexture windowLabel;
    OldTexture clock;
    OldTexture button;
    OldTexture button_pressed;

    std::string font;
    std::string justify;
  } toolbar;

  struct {
    std::string title_textColor;
    std::string frame_textColor;
    std::string frame_disableColor;
    std::string hilite_textColor;

    OldTexture title;
    OldTexture frame;
    OldTexture hilite;

    std::string title_font;
    std::string title_justify;

    std::string frame_font;
    std::string frame_justify;
  } menu;

  std::string borderColor, borderWidth;
  std::string bevelWidth;
  std::string frameWidth;
  std::string handleWidth;
  std::string rootCommand;
};


struct NewTexture {
  std::string appearance;
  std::string color1;
  std::string color2;
  std::string borderWidth;
  std::string borderColor;

  NewTexture &operator=(const OldTexture &oldTexture);
  void addBorder(const std::string &_borderWidth,
                 const std::string &_borderColor);
};

NewTexture &NewTexture::operator=(const OldTexture &oldTexture)
{
  appearance = bt::tolower(oldTexture.appearance);
  color1 = oldTexture.color1;
  color2 = oldTexture.color2;
  return *this;
}

void NewTexture::addBorder(const std::string &_borderWidth,
                           const std::string &_borderColor)
{
  appearance += " border";
  borderWidth = _borderWidth;
  borderColor = _borderColor;
}

struct NewStyle {
  struct {
    NewTexture title;
    NewTexture frame;
    NewTexture active;

    std::string title_foregroundColor;
    std::string title_textColor;
    std::string title_font;
    std::string title_alignment;
    std::string title_marginWidth;

    std::string frame_foregroundColor;
    std::string frame_textColor;
    std::string frame_disabledColor;
    std::string frame_font;
    std::string frame_alignment;
    std::string frame_marginWidth;

    std::string active_foregroundColor;
    std::string active_textColor;
  } menu;

  struct {
    NewTexture slit;

    std::string marginWidth;
  } slit;

  struct {
    NewTexture toolbar;
    NewTexture label;
    NewTexture windowLabel;
    NewTexture clock;
    NewTexture button;
    NewTexture button_pressed;

    std::string label_textColor;
    std::string windowLabel_textColor;
    std::string clock_textColor;
    std::string button_foregroundColor;
    std::string alignment;
    std::string font;

    std::string marginWidth;
    std::string label_marginWidth;
    std::string button_marginWidth;
  } toolbar;

  struct {
    NewTexture title_focus, title_unfocus;
    NewTexture label_focus, label_unfocus;
    NewTexture button_focus, button_unfocus;
    NewTexture handle_focus, handle_unfocus;
    NewTexture grip_focus, grip_unfocus;
    NewTexture button_pressed;

    std::string frame_focus_borderColor, frame_unfocus_borderColor;
    std::string button_focus_foregroundColor, button_unfocus_foregroundColor;
    std::string label_focus_textColor, label_unfocus_textColor;

    std::string font;
    std::string alignment;

    std::string title_marginWidth;
    std::string label_marginWidth;
    std::string button_marginWidth;
    std::string frame_borderWidth;
    std::string handleHeight;
  } window;

  std::string rootCommand;

  NewStyle &operator=(const OldStyle &oldStyle);
};

// convert old style to new style
NewStyle &
NewStyle::operator=(const OldStyle &oldStyle)
{
  bool hasBorders = (oldStyle.borderWidth != "0");

  // convert old menu style to new menu style
  menu.title = oldStyle.menu.title;
  menu.frame = oldStyle.menu.frame;
  menu.active = oldStyle.menu.hilite;
  if (hasBorders) {
    menu.title.addBorder(oldStyle.borderWidth, oldStyle.borderColor);
    menu.frame.addBorder(oldStyle.borderWidth, oldStyle.borderColor);
  }

  menu.title_foregroundColor = oldStyle.menu.title_textColor;
  menu.title_textColor = oldStyle.menu.title_textColor;
  menu.title_font = oldStyle.menu.title_font;
  menu.title_alignment = justifyToAlignment(oldStyle.menu.title_justify);
  menu.title_marginWidth = oldStyle.bevelWidth;

  menu.frame_foregroundColor = oldStyle.menu.frame_textColor;
  menu.frame_textColor = oldStyle.menu.frame_textColor;
  menu.frame_disabledColor = oldStyle.menu.frame_disableColor;
  menu.frame_font = oldStyle.menu.frame_font;
  menu.frame_alignment = justifyToAlignment(oldStyle.menu.frame_justify);
  menu.frame_marginWidth = oldStyle.bevelWidth;

  menu.active_foregroundColor = oldStyle.menu.hilite_textColor;
  menu.active_textColor = oldStyle.menu.hilite_textColor;

  // convert old toolbar style to new slit style
  slit.slit = oldStyle.toolbar.toolbar;
  if (hasBorders)
    slit.slit.addBorder(oldStyle.borderWidth, oldStyle.borderColor);

  slit.marginWidth = oldStyle.bevelWidth;

  // convert old toolbar style to new toolbar style
  toolbar.toolbar = oldStyle.toolbar.toolbar;
  toolbar.label = oldStyle.toolbar.label;
  toolbar.windowLabel = oldStyle.toolbar.windowLabel;
  toolbar.clock = oldStyle.toolbar.clock;
  toolbar.button = oldStyle.toolbar.button;
  toolbar.button_pressed = oldStyle.toolbar.button_pressed;
  if (hasBorders)
    toolbar.toolbar.addBorder(oldStyle.borderWidth, oldStyle.borderColor);

  toolbar.label_textColor = oldStyle.toolbar.label_textColor;
  toolbar.windowLabel_textColor = oldStyle.toolbar.windowLabel_textColor;
  toolbar.clock_textColor = oldStyle.toolbar.clock_textColor;
  toolbar.button_foregroundColor = oldStyle.toolbar.button_picColor;
  toolbar.alignment = justifyToAlignment(oldStyle.toolbar.justify);
  toolbar.font = oldStyle.toolbar.font;

  toolbar.marginWidth = oldStyle.bevelWidth;
  toolbar.label_marginWidth = oldStyle.bevelWidth;
  toolbar.button_marginWidth = oldStyle.bevelWidth;

  // convert old window style to new window style
  window.title_focus = oldStyle.window.title_focus;
  window.label_focus = oldStyle.window.label_focus;
  window.button_focus = oldStyle.window.button_focus;
  window.handle_focus = oldStyle.window.handle_focus;
  window.grip_focus = oldStyle.window.grip_focus;
  window.title_unfocus = oldStyle.window.title_unfocus;
  window.label_unfocus = oldStyle.window.label_unfocus;
  window.button_unfocus = oldStyle.window.button_unfocus;
  window.handle_unfocus = oldStyle.window.handle_unfocus;
  window.grip_unfocus = oldStyle.window.grip_unfocus;
  window.button_pressed = oldStyle.window.button_pressed;
  if (hasBorders) {
    window.title_focus.addBorder(oldStyle.borderWidth,
                                 oldStyle.borderColor);
    window.title_unfocus.addBorder(oldStyle.borderWidth,
                                   oldStyle.borderColor);
    window.handle_focus.addBorder(oldStyle.borderWidth,
                                  oldStyle.borderColor);
    window.handle_unfocus.addBorder(oldStyle.borderWidth,
                                    oldStyle.borderColor);
    window.grip_focus.addBorder(oldStyle.borderWidth,
                                oldStyle.borderColor);
    window.grip_unfocus.addBorder(oldStyle.borderWidth,
                                  oldStyle.borderColor);
    window.frame_borderWidth = oldStyle.borderWidth;
  } else {
    window.frame_borderWidth = "0";
  }

  window.frame_focus_borderColor = oldStyle.borderColor;
  window.frame_unfocus_borderColor = oldStyle.borderColor;

  window.button_focus_foregroundColor =
    oldStyle.window.button_focus_picColor;
  window.button_unfocus_foregroundColor =
    oldStyle.window.button_unfocus_picColor;

  window.label_focus_textColor = oldStyle.window.label_focus_textColor;
  window.label_unfocus_textColor = oldStyle.window.label_unfocus_textColor;

  window.font = oldStyle.window.font;
  window.alignment = justifyToAlignment(oldStyle.window.justify);

  window.title_marginWidth = oldStyle.bevelWidth;
  window.label_marginWidth = oldStyle.bevelWidth;
  window.button_marginWidth = oldStyle.bevelWidth;
  window.handleHeight = oldStyle.handleWidth;

  // keep the same root command
  rootCommand = oldStyle.rootCommand;

  return *this;
}


OldTexture readOldTexture(const bt::Resource &res,
                          const std::string &name,
                          const std::string &Name,
                          const std::string &defaultColor)
{
  OldTexture texture;
  texture.appearance =
    res.read(name,
             Name,
             "flat solid");
  texture.color1 =
    res.read(name + ".color",
             Name + ".Color",
             defaultColor);
  texture.color2 =
    res.read(name + ".colorTo",
             Name + ".ColorTo",
             defaultColor);
  return texture;
}

OldStyle readOldStyle(const bt::Resource &res)
{
  OldStyle style;

  // window style
  style.window.label_focus_textColor =
    res.read("window.label.focus.textColor",
             "Window.Label.Focus.TextColor",
             "black");
  style.window.label_unfocus_textColor =
    res.read("window.label.unfocus.textColor",
             "Window.Label.Unfocus.TextColor",
             "white");
  style.window.button_focus_picColor =
    res.read("window.button.focus.picColor",
             "Window.Button.Focus.PicColor",
             "black");
  style.window.button_unfocus_picColor =
    res.read("window.button.unfocus.picColor",
             "Window.Button.Unfocus.PicColor",
             "white");
  style.window.frame_focusColor =
    res.read("window.frame.focusColor",
             "Window.Frame.FocusColor",
             "white");
  style.window.frame_unfocusColor =
    res.read("window.frame.unfocusColor",
             "Window.Frame.UnfocusColor",
             "white");
  style.window.title_focus =
    readOldTexture(res,
                   "window.title.focus",
                   "Window.Title.Focus",
                   "white");
  style.window.title_unfocus =
    readOldTexture(res,
                   "window.title.unfocus",
                   "Window.Title.Unfocus",
                   "black");
  style.window.label_focus =
    readOldTexture(res,
                   "window.label.focus",
                   "Window.Label.Focus",
                   "white");
  style.window.label_unfocus =
    readOldTexture(res,
                   "window.label.unfocus",
                   "Window.Label.Unfocus",
                   "black");
  style.window.handle_focus =
    readOldTexture(res,
                   "window.handle.focus",
                   "Window.Handle.Focus",
                   "white");
  style.window.handle_unfocus =
    readOldTexture(res,
                   "window.handle.unfocus",
                   "Window.Handle.Unfocus",
                   "black");
  style.window.button_focus =
    readOldTexture(res,
                   "window.button.focus",
                   "Window.Button.Focus",
                   "white");
  style.window.button_unfocus =
    readOldTexture(res,
                   "window.button.unfocus",
                   "Window.Button.Unfocus",
                   "black");
  style.window.button_pressed =
    readOldTexture(res,
                   "window.button.pressed",
                   "Window.Button.Pressed",
                   "black");
  style.window.grip_focus =
    readOldTexture(res,
                   "window.grip.focus",
                   "Window.Grip.Focus",
                   "white");
  style.window.grip_unfocus =
    readOldTexture(res,
                   "window.grip.unfocus",
                   "Window.Grip.Unfocus",
                   "black");
  style.window.font =
    res.read("window.font",
             "Window.Font",
             "fixed");
  style.window.justify =
    res.read("window.justify",
             "Window.Justify",
             "LeftJustify");

  // toolbar style
  style.toolbar.label_textColor =
    res.read("toolbar.label.textColor",
             "Toolbar.Label.TextColor",
             "white");
  style.toolbar.windowLabel_textColor =
    res.read("toolbar.windowLabel.textColor",
             "Toolbar.WindowLabel.TextColor",
             "white");
  style.toolbar.clock_textColor =
    res.read("toolbar.clock.textColor",
             "Toolbar.Clock.TextColor",
             "white");
  style.toolbar.button_picColor =
    res.read("toolbar.button.picColor",
             "Toolbar.Button.PicColor",
             "black");
  style.toolbar.toolbar =
    readOldTexture(res,
                   "toolbar",
                   "Toolbar",
                   "black");
  style.toolbar.label =
    readOldTexture(res,
                   "toolbar.label",
                   "Toolbar.Label",
                   "black");
  style.toolbar.windowLabel =
    readOldTexture(res,
                   "toolbar.windowLabel",
                   "Toolbar.WindowLabel",
                   "black");
  style.toolbar.clock =
    readOldTexture(res,
                   "toolbar.clock",
                   "Toolbar.Clock",
                   "black");
  style.toolbar.button =
    readOldTexture(res,
                   "toolbar.button",
                   "Toolbar.Button",
                   "white");
  style.toolbar.button_pressed =
    readOldTexture(res,
                   "toolbar.button.pressed",
                   "Toolbar.Button.Pressed",
                   "black");
  style.toolbar.font =
    res.read("toolbar.font",
             "Toolbar.Font",
             "fixed");
  style.toolbar.justify =
    res.read("toolbar.justify",
             "Toolbar.Justify",
             "LeftJustify");

  // menu style
  style.menu.title_textColor =
    res.read("menu.title.textColor",
             "Menu.Title.TextColor",
             "black");
  style.menu.frame_textColor =
    res.read("menu.frame.textColor",
             "Menu.Frame.TextColor",
             "white");
  style.menu.frame_disableColor =
    res.read("menu.frame.disableColor",
             "Menu.Frame.DisableColor",
             "black");
  style.menu.hilite_textColor =
    res.read("menu.hilite.textColor",
             "Menu.Hilite.TextColor",
             "black");
  style.menu.title =
    readOldTexture(res,
                   "menu.title",
                   "Menu.Title",
                   "white");
  style.menu.frame =
    readOldTexture(res,
                   "menu.frame",
                   "Menu.Frame",
                   "black");
  style.menu.hilite =
    readOldTexture(res,
                   "menu.hilite",
                   "Menu.Hilite",
                   "white");
  style.menu.title_font =
    res.read("menu.title.font",
             "Menu.Title.Font",
             "fixed");
  style.menu.title_justify =
    res.read("menu.title.justify",
             "Menu.Title.Justify",
             "LeftJustify");
  style.menu.frame_font =
    res.read("menu.frame.font",
             "Menu.Frame.Font",
             "fixed");
  style.menu.frame_justify =
    res.read("menu.frame.justify",
             "Menu.Frame.Justify",
             "LeftJustify");

  // rest of the style
  style.borderColor =
    res.read("borderColor",
             "BorderColor",
             "black");
  style.borderWidth =
    res.read("borderWidth",
             "BorderWidth",
             "1");
  style.bevelWidth =
    res.read("bevelWidth",
             "BevelWidth",
             "3");
  style.frameWidth =
    res.read("frameWidth",
             "FrameWidth",
             style.bevelWidth);
  style.handleWidth =
    res.read("handleWidth",
             "HandleWidth",
             "6");
  style.rootCommand =
    res.read("rootCommand",
             "RootCommand");

  return style;
}


void writeTexture(std::ofstream &s,
                  const std::string &name,
                  const NewTexture &texture)
{
  s << name << ".appearance:" << '\t' << texture.appearance << std::endl;
  if (texture.appearance.find("gradient") != std::string::npos) {
    s << name << ".color1:" << '\t' << texture.color1 << std::endl;
  s << name << ".color2:" << '\t' << texture.color2 << std::endl;
  } else {
    s << name << ".backgroundColor:" << '\t' << texture.color1 << std::endl;
  }
  if (texture.appearance.find("border") != std::string::npos) {
    s << name << ".borderWidth:" << '\t' << texture.borderWidth << std::endl;
    s << name << ".borderColor:" << '\t' << texture.borderColor << std::endl;
  }
}

void writeStyle(const std::string &filename, const NewStyle &style)
{
  std::ofstream s(filename.c_str());

  // menu
  writeTexture(s, "menu.title", style.menu.title);
  s << "menu.title.foregroundColor:" << '\t'
    << style.menu.title_foregroundColor << std::endl;
  s << "menu.title.textColor:" << '\t'
    << style.menu.title_textColor << std::endl;
  s << "menu.title.font:" << '\t'
    << style.menu.title_font << std::endl;
  s << "menu.title.alignment:" << '\t'
    << style.menu.title_alignment << std::endl;
  s << "menu.title.marginWidth:" << '\t'
    << style.menu.title_marginWidth << std::endl;
  writeTexture(s, "menu.frame", style.menu.frame);
  s << "menu.frame.foregroundColor:" << '\t'
    << style.menu.frame_foregroundColor << std::endl;
  s << "menu.frame.textColor:" << '\t'
    << style.menu.frame_textColor << std::endl;
  s << "menu.frame.disabledColor:" << '\t'
    << style.menu.frame_disabledColor << std::endl;
  s << "menu.frame.font:" << '\t'
    << style.menu.frame_font << std::endl;
  s << "menu.frame.alignment:" << '\t'
    << style.menu.frame_alignment << std::endl;
  s << "menu.frame.marginWidth:" << '\t'
    << style.menu.frame_marginWidth << std::endl;
  writeTexture(s, "menu.active", style.menu.active);
  s << "menu.active.foregroundColor:" << '\t'
    << style.menu.active_foregroundColor << std::endl;
  s << "menu.active.textColor:" << '\t'
    << style.menu.active_textColor << std::endl;
  s << std::endl;

  //slit
  writeTexture(s, "slit", style.slit.slit);
  s << "slit.marginWidth:" << '\t'
    << style.slit.marginWidth << std::endl;
  s << std::endl;

  // toolbar
  writeTexture(s, "toolbar", style.toolbar.toolbar);
  writeTexture(s, "toolbar.label", style.toolbar.label);
  s << "toolbar.label.textColor:" << '\t'
    << style.toolbar.label_textColor << std::endl;
  s << "toolbar.label.marginWidth:" << '\t'
    << style.toolbar.label_marginWidth << std::endl;
  writeTexture(s, "toolbar.windowLabel", style.toolbar.windowLabel);
  s << "toolbar.windowLabel.textColor:" << '\t'
    << style.toolbar.windowLabel_textColor << std::endl;
  writeTexture(s, "toolbar.clock", style.toolbar.clock);
  s << "toolbar.clock.textColor:" << '\t'
    << style.toolbar.clock_textColor << std::endl;
  writeTexture(s, "toolbar.button", style.toolbar.button);
  s << "toolbar.button.foregroundColor:" << '\t'
    << style.toolbar.button_foregroundColor << std::endl;
  s << "toolbar.button.marginWidth:" << '\t'
    << style.toolbar.button_marginWidth << std::endl;
  writeTexture(s, "toolbar.button.pressed", style.toolbar.button_pressed);
  s << "toolbar.alignment:" << '\t'
    << style.toolbar.alignment << std::endl;
  s << "toolbar.font:" << '\t'
    << style.toolbar.font << std::endl;
  s << "toolbar.marginWidth:" << '\t'
    << style.toolbar.marginWidth << std::endl;
  s << std::endl;

  // window
  writeTexture(s, "window.title.focus", style.window.title_focus);
  writeTexture(s, "window.title.unfocus", style.window.title_unfocus);
  s << "window.title.marginWidth:" << '\t'
    << style.window.title_marginWidth << std::endl;
  writeTexture(s, "window.label.focus", style.window.label_focus);
  writeTexture(s, "window.label.unfocus", style.window.label_unfocus);
  s << "window.label.focus.textColor:" << '\t'
    << style.window.label_focus_textColor << std::endl;
  s << "window.label.unfocus.textColor:" << '\t'
    << style.window.label_unfocus_textColor << std::endl;
  s << "window.label.marginWidth:" << '\t'
    << style.window.label_marginWidth << std::endl;
  writeTexture(s, "window.button.focus", style.window.button_focus);
  writeTexture(s, "window.button.unfocus", style.window.button_unfocus);
  writeTexture(s, "window.button.pressed", style.window.button_pressed);
  s << "window.button.focus.foregroundColor:" << '\t'
    << style.window.button_focus_foregroundColor << std::endl;
  s << "window.button.unfocus.foregroundColor:" << '\t'
    << style.window.button_unfocus_foregroundColor << std::endl;
  s << "window.button.marginWidth:" << '\t'
    << style.window.button_marginWidth << std::endl;
  writeTexture(s, "window.handle.focus", style.window.handle_focus);
  writeTexture(s, "window.handle.unfocus", style.window.handle_unfocus);
  writeTexture(s, "window.grip.focus", style.window.grip_focus);
  writeTexture(s, "window.grip.unfocus", style.window.grip_unfocus);
  s << "window.frame.focus.borderColor:" << '\t'
    << style.window.frame_focus_borderColor << std::endl;
  s << "window.frame.unfocus.borderColor:" << '\t'
    << style.window.frame_unfocus_borderColor << std::endl;
  s << "window.frame.borderWidth:" << '\t'
    << style.window.frame_borderWidth << std::endl;
    s << "window.font:" << '\t'
    << style.window.font << std::endl;
  s << "window.alignment:" << '\t'
    << style.window.alignment << std::endl;
  s << "window.handleHeight:" << '\t'
    << style.window.handleHeight << std::endl;
  s << std::endl;

  // rootcomand
  s << "rootCommand:" << '\t'
    << style.rootCommand << std::endl;
}



int main(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    const std::string filename = argv[i];
    bt::Resource resource(filename);
    if (resource.valid()) {
      std::cout << filename << " -> " << std::flush;
      NewStyle style;
      style = readOldStyle(resource);
      writeStyle(filename + "-new", style);
      std::cout << filename + "-new" << std::endl;
    }
  }
  return 0;
}
