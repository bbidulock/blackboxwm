// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Style.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh at debian.org>
// Copyright (c) 1997 - 2000, 2002 Bradley T Hughes <bhughes at trolltech.com>
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

#ifndef STYLE_HH
#define STYLE_HH

#include <X11/Xlib.h>

#include "Color.hh"
#include "Texture.hh"

#include <string>
using std::string;


class BStyle
{
public:
  BStyle( int scr );
  ~BStyle();

  void setCurrentStyle( const string & );
  const string &currentStyle() const { return currentstyle; }

  enum Justify { Left, Right, Center };

  // menu title
  const XFontSet &menuTitleFontSet() const { return menustyle.titlestyle.fontset; }
  XFontSetExtents *menuTitleFontSetExtents() const { return menustyle.titlestyle.fontset_extents; }
  XFontStruct *menuTitleFont() const { return menustyle.titlestyle.font; }
  Justify menuTitleJustify() const { return menustyle.titlestyle.justify; }
  const BColor &menuTitleTextColor() const { return menustyle.titlestyle.textcolor; }
  BTexture &menuTitle() { return menustyle.titlestyle.texture; }

  // menu frame
  const XFontSet &menuFontSet() const { return menustyle.fontset; }
  XFontSetExtents *menuFontSetExtents() const { return menustyle.fontset_extents; }
  XFontStruct *menuFont() const { return menustyle.font; }
  Justify menuJustify() const { return menustyle.justify; }
  const BColor &menuTextColor() const { return menustyle.textcolor; }
  const BColor &menuHighlightTextColor() const { return menustyle.hitextcolor; }
  const BColor &menuDisabledTextColor() const { return menustyle.disabledcolor; }
  BTexture &menu() { return menustyle.texture; }
  BTexture &menuHighlight() { return menustyle.highlight; }

  // toolbar
  const XFontSet &toolbarFontSet() const { return toolbarstyle.fontset; }
  XFontSetExtents *toolbarFontSetExtents() const { return toolbarstyle.fontset_extents; }
  XFontStruct *toolbarFont() const { return toolbarstyle.font; }
  Justify toolbarJustify() const { return toolbarstyle.justify; }
  const BColor &toolbarWindowLabelTextColor() const { return toolbarstyle.windowlabeltextcolor; }
  const BColor &toolbarWorkspaceLabelTextColor() const { return toolbarstyle.workspacelabeltextcolor; }
  const BColor &toolbarClockTextColor() const { return toolbarstyle.clocktextcolor; }
  const BColor &toolbarButtonPicColor() const { return toolbarstyle.piccolor; }
  BTexture &toolbar() { return toolbarstyle.texture; }
  BTexture &toolbarWindowLabel() { return toolbarstyle.windowlabel; }
  BTexture &toolbarWorkspaceLabel() { return toolbarstyle.workspacelabel; }
  BTexture &toolbarClockLabel() { return toolbarstyle.clocklabel; }
  BTexture &toolbarButton() { return toolbarstyle.button; }
  BTexture &toolbarButtonPressed() { return toolbarstyle.buttonpressed; }

  // window
  const XFontSet &windowFontSet() const { return windowstyle.fontset; }
  XFontSetExtents *windowFontSetExtents() const { return windowstyle.fontset_extents; }
  XFontStruct *windowFont() const { return windowstyle.font; }
  Justify windowJustify() const { return windowstyle.justify; }
  const BColor &windowFrameFocusColor() const { return windowstyle.focus.framecolor; }
  const BColor &windowFrameUnfocusColor() const { return windowstyle.unfocus.framecolor; }
  const BColor &windowLabelFocusTextColor() const { return windowstyle.focus.labeltextcolor; }
  const BColor &windowLabelUnfocusTextColor() const { return windowstyle.unfocus.labeltextcolor; }
  const BColor &windowButtonFocusPicColor() const { return windowstyle.focus.piccolor; }
  const BColor &windowButtonUnfocusPicColor() const { return windowstyle.unfocus.piccolor; }
  BTexture &windowLabelFocus() { return windowstyle.focus.label; }
  BTexture &windowLabelUnfocus() { return windowstyle.unfocus.label; }
  BTexture &windowButtonFocus() { return windowstyle.focus.button; }
  BTexture &windowButtonUnfocus() { return windowstyle.unfocus.button; }
  BTexture &windowButtonPressed() { return windowstyle.buttonpressed; }
  BTexture &windowTitleFocus() { return windowstyle.focus.title; }
  BTexture &windowTitleUnfocus() { return windowstyle.unfocus.title; }
  BTexture &windowHandleFocus() { return windowstyle.focus.handle; }
  BTexture &windowHandleUnfocus() { return windowstyle.unfocus.handle; }
  BTexture &windowGripFocus() { return windowstyle.focus.grip; }
  BTexture &windowGripUnfocus() { return windowstyle.unfocus.grip; }

  // bitmaps
  Pixmap arrowBitmap() const { return bitmap.arrow; }
  Pixmap checkBitmap() const { return bitmap.check; }
  Pixmap iconifyBitmap() const { return bitmap.iconify; }
  Pixmap maximizeBitmap() const { return bitmap.maximize; }
  Pixmap closeBitmap() const { return bitmap.close; }

  // general
  const BColor &borderColor() const { return bordercolor; }
  int bevelWidth() const { return bevel_width; }
  int borderWidth() const { return border_width; }
  int frameWidth() const { return frame_width; }
  int handleWidth() const { return handle_width; }


private:
  BTexture readDatabaseTexture( const string &, const string &, const string & );
  BColor readDatabaseColor( const string &, const string &, const string & );
  XFontSet readDatabaseFontSet( const string &, const string & );
  XFontStruct *readDatabaseFont( const string &, const string & );
  XFontSet createFontSet( const string & );

  struct menustyle {
    struct titlestyle {
      XFontSet fontset;
      XFontSetExtents *fontset_extents;
      XFontStruct *font;
      Justify justify;
      BColor textcolor;
      BTexture texture;
    } titlestyle;

    XFontSet fontset;
    XFontSetExtents *fontset_extents;
    XFontStruct *font;
    Justify justify;
    BColor textcolor, hicolor, hitextcolor, disabledcolor;
    BTexture texture, highlight;
  } menustyle;

  struct toolbarstyle {
    XFontSet fontset;
    XFontSetExtents *fontset_extents;
    XFontStruct *font;
    Justify justify;
    BColor windowlabeltextcolor, workspacelabeltextcolor, clocktextcolor, piccolor;
    BTexture texture, windowlabel, workspacelabel, clocklabel, button, buttonpressed;
  } toolbarstyle;

  struct windowstyle {
    XFontSet fontset;
    XFontSetExtents *fontset_extents;
    XFontStruct *font;
    Justify justify;

    struct focus {
      BColor framecolor, labeltextcolor, piccolor;
      BTexture label, button, title, handle, grip;
    } focus;

    struct unfocus {
      BColor framecolor, labeltextcolor, piccolor;
      BTexture label, button, title, handle, grip;
    } unfocus;

    BTexture buttonpressed;
  } windowstyle;

  struct bitmap {
    Pixmap arrow, check, iconify, maximize, close;
  } bitmap;

  BColor bordercolor;
  int bevel_width, border_width, frame_width, handle_width;
  int screen;
  string currentstyle;
};

#endif // STYLE_HH
