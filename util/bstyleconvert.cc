// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// bstyleconvert - a Blackbox style conversion utility
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

#include "Resource.hh"

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


std::string normalize(const std::string& input) {
  std::string output;
  std::string::const_iterator it = input.begin(), end = input.end();
  for (; it != end; ++it)
    output.push_back(tolower(*it));
  return output;
}


struct Style {
  struct Window {
    std::string font, title_focus, title_focus_color, title_focus_colorTo,
      title_unfocus, title_unfocus_color, title_unfocus_colorTo,
      label_focus, label_focus_color, label_focus_colorTo,
      label_unfocus, label_unfocus_color, label_unfocus_colorTo,
      label_focus_textColor, label_unfocus_textColor,
      handle_focus, handle_focus_color, handle_focus_colorTo,
      handle_unfocus, handle_unfocus_color, handle_unfocus_colorTo,
      handle_height,
      grip_focus, grip_focus_color, grip_focus_colorTo,
      grip_unfocus, grip_unfocus_color, grip_unfocus_colorTo,
      button_focus, button_focus_color, button_focus_colorTo,
      button_unfocus, button_unfocus_color, button_unfocus_colorTo,
      button_pressed, button_pressed_color, button_pressed_colorTo,
      button_focus_picColor, button_unfocus_picColor,
      frame_focusColor, frame_unfocusColor, frame_width, alignment;
  };
  struct Toolbar {
    std::string font, frame, frame_color, frame_colorTo,
      label, label_color, label_colorTo,
      window, window_color, window_colorTo,
      clock, clock_color, clock_colorTo,
      button, button_color, button_colorTo,
      button_pressed, button_pressed_color, button_pressed_colorTo,
      label_text, window_text, clock_text, button_pic, alignment;
  };
  struct Menu {
    std::string title_font, frame_font, title, title_color, title_colorTo,
      frame, frame_color, frame_colorTo, active, active_color, active_colorTo,
      title_text, frame_text, frame_foreground, disabledColor, active_text,
      title_alignment, frame_alignment, bullet, bullet_pos;
  };

  Window window;
  Toolbar toolbar;
  Menu menu;

  std::string border_color, border_width, bevel_width, rootCommand;

  void read(const bt::Resource& res);
};


void Style::read(const bt::Resource& res) {
  // load bevel, border and handle widths
  border_width = res.read("borderWidth", "BorderWidth", "1");
  bevel_width = res.read("bevelWidth", "BevelWidth", "3");

  // load window config
  window.font =
    res.read("window.font", "Window.Font", "fixed");

  window.title_focus =
    normalize(res.read("window.title.focus",
                       "Window.Title.Focus", "flat solid"));
  window.title_focus_color =
    res.read("window.title.focus.color",
             "Window.Title.Focus.Color", "white");
  window.title_focus_colorTo =
    res.read("window.title.focus.colorTo",
             "Window.Title.Focus.ColorTo", "");

  window.title_unfocus =
    normalize(res.read("window.title.unfocus",
                       "Window.Title.Unfocus", "flat solid"));
  window.title_unfocus_color =
    res.read("window.title.unfocus.color",
             "Window.Title.Unfocus.Color", "black");
  window.title_unfocus_colorTo =
    res.read("window.title.unfocus.colorTo",
             "Window.Title.Unfocus.ColorTo", "");

  window.label_focus =
    normalize(res.read("window.label.focus",
                       "Window.Label.Focus", "flat solid" ));
  window.label_focus_color =
    res.read("window.label.focus.color",
             "Window.Label.Focus.Color", "white");
  window.label_focus_colorTo =
    res.read("window.label.focus.colorTo",
             "Window.Label.Focus.ColorTo", "");

  window.label_focus_textColor  =
    res.read("window.label.focus.textColor",
             "Window.Label.Focus.TextColor", "black");

  window.label_unfocus_textColor =
    res.read("window.label.unfocus.textColor",
             "Window.Label.Unfocus.TextColor", "white");

  window.label_unfocus =
    normalize(res.read("window.label.unfocus",
                       "Window.Label.Unfocus", "flat solid"));
  window.label_unfocus_color =
    res.read("window.label.unfocus.color",
             "Window.Label.Unfocus.Color", "black");
  window.label_unfocus_colorTo =
    res.read("window.label.unfocus.colorTo",
             "Window.Label.Unfocus.ColorTo", "");

  window.handle_focus =
    normalize(res.read("window.handle.focus",
                       "Window.Handle.Focus", "flat solid"));
  window.handle_focus_color =
    res.read("window.handle.focus.color",
             "Window.Handle.Focus.Color", "white");
  window.handle_focus_colorTo =
    res.read("window.handle.focus.colorTo", "Window.Handle.Focus.ColorTo", "");

  window.handle_unfocus =
    normalize(res.read("window.handle.unfocus",
                       "Window.Handle.Unfocus", "flat solid"));
  window.handle_unfocus_color =
    res.read("window.handle.unfocus.color",
             "Window.Handle.Unfocus.Color", "black");
  window.handle_unfocus_colorTo =
    res.read("window.handle.unfocus.colorTo",
             "Window.Handle.Unfocus.ColorTo", "");
  window.handle_height =
    res.read("handleWidth", "HandleWidth", "6");

  window.grip_focus =
    normalize(res.read("window.grip.focus",
                       "Window.Grip.Focus", "flat solid"));
  window.grip_focus_color =
    res.read("window.grip.focus.color",
             "Window.Grip.Focus.Color", "white");
  window.grip_focus_colorTo =
    res.read("window.grip.focus.colorTo",
             "Window.Grip.Focus.ColorTo", "");

  window.grip_unfocus =
    normalize(res.read("window.grip.unfocus",
                       "Window.Grip.Unfocus", "flat solid"));
  window.grip_unfocus_color =
    res.read("window.grip.unfocus.color",
             "Window.Grip.Unfocus.Color", "black");
  window.grip_unfocus_colorTo =
    res.read("window.grip.unfocus.colorTo",
             "Window.Grip.Unfocus.ColorTo", "");

  window.button_focus =
    normalize(res.read("window.button.focus",
                       "Window.Button.Focus", "flat solid"));
  window.button_focus_color =
    res.read("window.button.focus.color",
             "Window.Button.Focus.Color", "white");
  window.button_focus_colorTo =
    res.read("window.button.focus.colorTo",
             "Window.Button.Focus.ColorTo", "");

  window.button_unfocus =
    normalize(res.read("window.button.unfocus",
                       "Window.Button.Unfocus", "flat solid"));
  window.button_unfocus_color =
    res.read("window.button.unfocus.color",
             "Window.Button.Unfocus.Color", "black");
  window.button_unfocus_colorTo =
    res.read("window.button.unfocus.colorTo",
             "Window.Button.Unfocus.ColorTo", "");

  window.button_pressed =
    normalize(res.read("window.button.pressed",
                       "Window.Button.Pressed", "flat solid"));
  window.button_pressed_color =
    res.read("window.button.pressed.color",
             "Window.Button.Pressed.Color", "black");
  window.button_pressed_colorTo =
    res.read("window.button.pressed.colorTo",
             "Window.Button.Pressed.ColorTo", "");

  window.button_focus_picColor  =
    res.read("window.button.focus.picColor",
             "Window.Button.Focus.PicColor", "black");

  window.button_unfocus_picColor =
    res.read("window.button.unfocus.picColor",
             "Window.Button.Unfocus.PicColor", "white");

  window.frame_focusColor =
    res.read("window.frame.focusColor", "Window.Frame.FocusColor", "white");

  window.frame_unfocusColor =
    res.read("window.frame.unfocusColor",
             "Window.Frame.UnfocusColor", "white");
  window.frame_width = res.read("frameWidth", "FrameWidth", bevel_width);

  window.alignment = res.read("window.justify", "Window.Justify", "left");

  // load toolbar config
  toolbar.font = res.read("toolbar.font", "Toolbar.Font", "fixed");

  toolbar.frame = normalize(res.read("toolbar", "Toolbar", "flat solid"));
  toolbar.frame_color =
    res.read("toolbar.color", "Toolbar.Color", "black");
  toolbar.frame_colorTo =
    res.read("toolbar.colorTo", "Toolbar.ColorTo", "");

  toolbar.label =
    normalize(res.read("toolbar.label", "Toolbar.Label", "flat solid"));
  toolbar.label_color =
    res.read("toolbar.label.color", "Toolbar.Label.Color", "black");
  toolbar.label_colorTo =
    res.read("toolbar.label.colorTo", "Toolbar.Label.ColorTo", "");

  toolbar.window =
    normalize(res.read("toolbar.windowLabel",
                       "Toolbar.WindowLabel", "flat solid"));
  toolbar.window_color =
    res.read("toolbar.windowLabel.color",
             "Toolbar.WindowLabel.Color", "black");
  toolbar.window_colorTo =
    res.read("toolbar.windowLabel.colorTo", "Toolbar.WindowLabel.ColorTo", "");

  toolbar.button =
    normalize(res.read("toolbar.button", "Toolbar.Button", "flat solid"));
  toolbar.button_color =
    res.read("toolbar.button.color", "Toolbar.Button.Color", "white");
  toolbar.button_colorTo =
    res.read("toolbar.button.colorTo", "Toolbar.Button.ColorTo", "");

  toolbar.button_pressed =
    normalize(res.read("toolbar.button.pressed",
                       "Toolbar.Button.Pressed", "flat solid"));
  toolbar.button_pressed_color =
    res.read("toolbar.button.pressed.color",
             "Toolbar.Button.Pressed.Color", "black");
  toolbar.button_pressed_colorTo =
    res.read("toolbar.button.pressed.colorTo",
             "Toolbar.Button.Pressed.ColorTo", "");

  toolbar.clock =
    normalize(res.read("toolbar.clock", "Toolbar.Clock", "flat solid"));
  toolbar.clock_color =
    res.read("toolbar.clock.color", "Toolbar.Clock.Color", "black");
  toolbar.clock_colorTo =
    res.read("toolbar.clock.colorTo", "Toolbar.Clock.ColorTo", "");

  toolbar.label_text =
    res.read("toolbar.label.textColor",
             "Toolbar.Label.TextColor", "white");

  toolbar.window_text =
    res.read("toolbar.windowLabel.textColor",
             "Toolbar.WindowLabel.TextColor", "white");

  toolbar.clock_text =
    res.read("toolbar.clock.textColor", "Toolbar.Clock.TextColor", "white");

  toolbar.button_pic =
    res.read("toolbar.button.picColor", "Toolbar.Button.PicColor", "black");

  toolbar.alignment = res.read("toolbar.justify", "Toolbar.Justify", "left");

  // load menu config
  menu.title_font = res.read("menu.title.font", "Menu.Title.Font", "fixed");

  menu.frame_font = res.read("menu.frame.font", "Menu.Frame.Font", "fixed");

  menu.title = normalize(res.read("menu.title", "Menu.Title", "flat solid"));
  menu.title_color =
    res.read("menu.title.color", "Menu.Title.Color", "white");
  menu.title_colorTo =
    res.read("menu.title.colorTo", "Menu.Title.ColorTo", "");

  menu.frame = normalize(res.read("menu.frame", "Menu.Frame", "flat solid"));
  menu.frame_color =
    res.read("menu.frame.color", "Menu.Frame.Color", "black");
  menu.frame_colorTo =
    res.read("menu.frame.colorTo", "Menu.Frame.ColorTo", "");

  menu.active = normalize(res.read("menu.hilite",
                                   "Menu.Hilite", "flat solid"));
  menu.active_color =
    res.read("menu.hilite.color", "Menu.Hilite.Color", "white");
  menu.active_colorTo =
    res.read("menu.hilite.colorTo", "Menu.hilite.ColorTo", "");

  menu.title_text =
    res.read("menu.title.textColor", "Menu.Title.TextColor", "black");

  menu.frame_text =
    res.read("menu.frame.textColor", "Menu.Frame.TextColor", "white");

  menu.frame_foreground =
    res.read("menu.frame.foregroundColor", "Menu.Frame.ForegroundColor",
             menu.frame_text);

  menu.disabledColor =
    res.read("menu.frame.disableColor", "Menu.Frame.DisableColor", "black");

  menu.disabledColor =
    res.read("menu.frame.disabledColor", "Menu.Frame.DisabledColor",
             menu.disabledColor);

  menu.active_text =
    res.read("menu.hilite.textColor", "Menu.Hilite.TextColor", "black");

  menu.title_alignment =
    res.read("menu.title.justify", "Menu.Title.Justify", "left");

  menu.frame_alignment =
    res.read("menu.frame.justify", "Menu.Frame.Justify", "left");

  menu.bullet = res.read("menu.bullet", "Menu.Bullet", "triangle");

  menu.bullet_pos =
    res.read("menu.bullet.position", "Menu.Bullet.Position", "left");

  border_color = res.read("borderColor", "BorderColor", "black");

  // set-background command
  rootCommand = res.read("rootCommand", "RootCommand", "");
}


typedef std::vector<std::string> StringVector;
void string_split(const std::string& input, StringVector& pieces) {
  std::string::const_iterator it = input.begin(), end = input.end(), pos = it;
  for (; it != end; ++it) {
    if (isspace(*it) && it != pos) {
      pieces.push_back(std::string(pos, it));
      pos = it;
    }
  }
  pieces.push_back(std::string(pos, end));
}


void writeComment(std::ofstream& stream, const std::string& text) {
  stream << "! " << text << '\n';
}


void writeValue(std::ofstream& stream, const std::string& key,
                const std::string& value) {
  std::string new_key = key + ':';
  int len = 40 - new_key.length();
  assert(len > 0);
  std::string padding(len, ' ');
  stream << new_key << padding << value << '\n';
}


void writeTexture(std::ofstream& stream,
                  const std::string& name, const std::string& texture,
                  const std::string& color, const std::string& colorTo,
                  const std::string &border_width = std::string(),
                  const std::string &border_color = std::string()) {
  if (border_width.empty())
    writeValue(stream, name, texture);
  else
    writeValue(stream, name, texture + " border");

  if (texture != "parentrelative") {
    std::string tmp = "     ";
    tmp += name;
    tmp += ".color";
    writeValue(stream, tmp, color);

    StringVector pieces;
    string_split(texture, pieces);

    // if a gradient, store colorTo as well
    if (std::find(pieces.begin(), pieces.end(), "solid") == pieces.end()) {
      tmp = "     ";
      tmp += name;
      tmp += ".colorTo";
      writeValue(stream, tmp, colorTo);
    }

    // store border width/color
    if (! (border_width.empty() || border_color.empty())) {
      writeValue(stream, name + ".borderWidth", border_width);
      writeValue(stream, name + ".borderColor", border_color);
    }
  }
}


void convert(const Style& style, const char* const filename) {
  std::string new_style(filename);
  new_style += ".70";

  std::ofstream fout(new_style.c_str());

  writeComment(fout, "***** Menu *****");
  writeValue(fout, "menu.frame.font", style.menu.frame_font);
  writeValue(fout, "menu.title.font", style.menu.title_font);
  fout << '\n';
  writeTexture(fout, "menu.title", style.menu.title,
               style.menu.title_color, style.menu.title_colorTo,
               style.border_width, style.border_color);
  writeValue(fout, "menu.title.textColor", style.menu.title_text);
  fout << '\n';
  writeTexture(fout, "menu.frame", style.menu.frame,
               style.menu.frame_color, style.menu.frame_colorTo,
               style.border_width, style.border_color);
  writeValue(fout, "menu.frame.textColor", style.menu.frame_text);
  writeValue(fout, "menu.frame.foregroundColor", style.menu.frame_foreground);
  fout << '\n';
  writeTexture(fout, "menu.active", style.menu.active,
               style.menu.active_color, style.menu.active_colorTo,
               style.border_width, style.border_color);
  writeValue(fout, "menu.active.textColor", style.menu.active_text);
  fout << '\n';
  writeComment(fout, "for 0.6x compatibility");
  writeTexture(fout, "menu.hilite", style.menu.active,
               style.menu.active_color, style.menu.active_colorTo);
  writeValue(fout, "menu.hilite.textColor", style.menu.active_text);
  fout << '\n';
  writeValue(fout, "menu.bullet", style.menu.bullet);
  writeValue(fout, "menu.bullet.position", style.menu.bullet_pos);
  fout << '\n';
  writeValue(fout, "menu.frame.disabledColor", style.menu.disabledColor);
  writeComment(fout, "for 0.6x compatibility");
  writeValue(fout, "menu.frame.disableColor", style.menu.disabledColor);
  fout << '\n';
  writeValue(fout, "menu.frame.alignment", style.menu.frame_alignment);
  writeValue(fout, "menu.title.alignment", style.menu.title_alignment);
  fout << '\n';
  writeComment(fout, "for 0.6x compatibility");
  writeValue(fout, "menu.frame.justify", style.menu.frame_alignment);
  writeValue(fout, "menu.title.justify", style.menu.title_alignment);
  fout << "\n\n";

  writeComment(fout, "***** Window *****");
  writeValue(fout, "window.font", style.window.font);
  fout << '\n';
  writeComment(fout, "focused window");
  writeTexture(fout, "window.title.focus", style.window.title_focus,
               style.window.title_focus_color,
               style.window.title_focus_colorTo);
  fout << '\n';
  writeTexture(fout, "window.label.focus", style.window.label_focus,
               style.window.label_focus_color,
               style.window.label_focus_colorTo);
  writeValue(fout, "window.label.focus.textColor",
             style.window.label_focus_textColor);
  fout << '\n';
  writeTexture(fout, "window.handle.focus", style.window.handle_focus,
               style.window.handle_focus_color,
               style.window.handle_focus_colorTo);
  fout << '\n';
  writeTexture(fout, "window.grip.focus", style.window.grip_focus,
               style.window.grip_focus_color,
               style.window.grip_focus_colorTo);
  fout << '\n';
  writeTexture(fout, "window.button.focus", style.window.button_focus,
               style.window.button_focus_color,
               style.window.button_focus_colorTo);
  writeValue(fout, "window.button.focus.picColor",
             style.window.button_focus_picColor);
  fout << '\n';
  writeTexture(fout, "window.button.pressed", style.window.button_pressed,
               style.window.button_pressed_color,
               style.window.button_pressed_colorTo);
  fout << '\n';
  writeValue(fout, "window.frame.focusColor", style.window.frame_focusColor);
  fout << '\n';
  writeComment(fout, "unfocused window");
  writeTexture(fout, "window.title.unfocus", style.window.title_unfocus,
               style.window.title_unfocus_color,
               style.window.title_unfocus_colorTo);
  fout << '\n';
  writeTexture(fout, "window.label.unfocus", style.window.label_unfocus,
               style.window.label_unfocus_color,
               style.window.label_unfocus_colorTo);
  writeValue(fout, "window.label.unfocus.textColor",
             style.window.label_unfocus_textColor);
  fout << '\n';
  writeTexture(fout, "window.handle.unfocus", style.window.handle_unfocus,
               style.window.handle_unfocus_color,
               style.window.handle_unfocus_colorTo);
  fout << '\n';
  writeTexture(fout, "window.grip.unfocus", style.window.grip_unfocus,
               style.window.grip_unfocus_color,
               style.window.grip_unfocus_colorTo);
  fout << '\n';
  writeTexture(fout, "window.button.unfocus", style.window.button_unfocus,
               style.window.button_unfocus_color,
               style.window.button_unfocus_colorTo);
  writeValue(fout, "window.button.unfocus.picColor",
             style.window.button_unfocus_picColor);
  fout << '\n';
  writeValue(fout, "window.frame.unfocusColor",
             style.window.frame_unfocusColor);
  fout << '\n';
  writeValue(fout, "window.alignment", style.window.alignment);
  writeValue(fout, "window.frameWidth", style.window.frame_width);
  writeValue(fout, "window.handleHeight", style.window.handle_height);
  fout << '\n';
  writeComment(fout, "for 0.6x compatibility");
  writeValue(fout, "window.justify", style.window.alignment);
  fout << '\n';

  writeComment(fout, "***** toolbar *****");
  writeValue(fout, "toolbar.font", style.toolbar.font);
  writeTexture(fout, "toolbar", style.toolbar.frame,
               style.toolbar.frame_color, style.toolbar.frame_colorTo);
  fout << '\n';
  writeTexture(fout, "toolbar.label", style.toolbar.label,
               style.toolbar.label_color, style.toolbar.label_colorTo);
  writeValue(fout, "toolbar.label.textColor", style.toolbar.label_text);
  fout << '\n';
  writeTexture(fout, "toolbar.windowLabel", style.toolbar.window,
               style.toolbar.window_color, style.toolbar.window_colorTo);
  writeValue(fout, "toolbar.windowLabel.textColor", style.toolbar.window_text);
  fout << '\n';
  writeTexture(fout, "toolbar.clock", style.toolbar.clock,
               style.toolbar.clock_color, style.toolbar.clock_colorTo);
  writeValue(fout, "toolbar.clock.textColor", style.toolbar.clock_text);
  fout << '\n';
  writeTexture(fout, "toolbar.button", style.toolbar.button,
               style.toolbar.button_color, style.toolbar.button_colorTo);
  fout << '\n';
  writeTexture(fout, "toolbar.button.pressed", style.toolbar.button_pressed,
               style.toolbar.button_pressed_color,
               style.toolbar.button_pressed_colorTo);
  fout << '\n';
  writeValue(fout, "toolbar.button.picColor", style.toolbar.button_pic);
  fout << '\n';
  writeValue(fout, "toolbar.alignment", style.toolbar.alignment);
  fout << '\n';
  writeComment(fout, "for 0.6x compatibility");
  writeValue(fout, "toolbar.justify", style.toolbar.alignment);
  fout << '\n';

  writeComment(fout, "***** the rest *****");
  writeValue(fout, "borderColor", style.border_color);
  writeValue(fout, "borderWidth", style.border_width);
  writeValue(fout, "bevelWidth", style.bevel_width);
  writeComment(fout, " for 0.6x compatibility");
  writeValue(fout, "handleWidth", style.window.handle_height);
  writeValue(fout, "frameWidth", style.window.frame_width);
  if (! style.rootCommand.empty())
    writeValue(fout, "rootCommand", style.rootCommand);
}


int main(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    bt::Resource resource(argv[i]);
    if (resource.valid()) {
      std::cout << "Processing: " << argv[i] << '\n';
      Style style;
      style.read(resource);
      convert(style, argv[i]);
    }
  }

  exit(0);
}
