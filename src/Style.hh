#ifndef STYLE_HH
#define STYLE_HH

#include <X11/Xlib.h>

#include "Color.hh"
#include "Texture.hh"


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

    BColor bordercolor;
    int bevel_width, border_width, frame_width, handle_width;
    int screen;
    string currentstyle;

    /*
      struct resource {
      Bool toolbar_on_top, toolbar_auto_hide, sloppy_focus, auto_raise,
      auto_edge_balance, image_dither, ordered_dither, opaque_move, full_max,
      focus_new, focus_last;
      BColor border_color;
      XrmDatabase stylerc;

      int workspaces, toolbar_placement, toolbar_width_percent, placement_policy,
      edge_snap_threshold, row_direction, col_direction;

      #ifdef    SLIT
      Bool slit_on_top, slit_auto_hide;
      int slit_placement, slit_direction;
      #endif // SLIT

      unsigned int handle_width, bevel_width, frame_width, border_width;

      #ifdef    HAVE_STRFTIME
      char *strftime_format;
      #else // !HAVE_STRFTIME
      Bool clock24hour;
      int date_format;
      #endif // HAVE_STRFTIME

      } resource;

      struct WindowStyle {
      BColor f_focus, f_unfocus, l_text_focus, l_text_unfocus, b_pic_focus,
      b_pic_unfocus;
      BTexture t_focus, t_unfocus, l_focus, l_unfocus, h_focus, h_unfocus,
      b_focus, b_unfocus, b_pressed, g_focus, g_unfocus;

      XFontSet fontset;
      XFontSetExtents *fontset_extents;
      XFontStruct *font;

      int justify;
      };

      struct ToolbarStyle {
      BColor l_text, w_text, c_text, b_pic;
      BTexture toolbar, label, window, button, pressed, clock;

      XFontSet fontset;
      XFontSetExtents *fontset_extents;
      XFontStruct *font;

      int justify;
      };

      struct MenuStyle {
      BColor t_text, f_text, h_text, d_text;
      BTexture title, frame, hilite;

      XFontSet t_fontset, f_fontset;
      XFontSetExtents *t_fontset_extents, *f_fontset_extents;
      XFontStruct *t_font, *f_font;

      int t_justify, f_justify, bullet, bullet_pos;
      };
    */
};

#endif // STYLE_HH
