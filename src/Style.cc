#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include "Style.hh"
#include "blackbox.hh"
#include "i18n.hh"

#include <stdio.h>

#ifdef    HAVE_STDARG_H
#  include <stdarg.h>
#endif // HAVE_STDARG_H

#ifndef   FONT_ELEMENT_SIZE
#define   FONT_ELEMENT_SIZE 50
#endif // FONT_ELEMENT_SIZE

#ifndef   MAXPATHLEN
#define   MAXPATHLEN 255
#endif // MAXPATHLEN

#define check_width 7
#define check_height 7
static unsigned char check_bits[] = {
   0x40, 0x60, 0x71, 0x3b, 0x1f, 0x0e, 0x04};


BStyle::BStyle( int scr )
    : screen( scr )
{
    windowstyle.fontset = 0;
    toolbarstyle.fontset = 0;
    menustyle.fontset = 0;
    menustyle.titlestyle.fontset = 0;
    windowstyle.font = 0;
    toolbarstyle.font = 0;
    menustyle.font = 0;
    menustyle.titlestyle.font = 0;
}

BStyle::~BStyle()
{
    if ( windowstyle.fontset )
	XFreeFontSet( *BaseDisplay::instance(), windowstyle.fontset );
    if ( toolbarstyle.fontset )
	XFreeFontSet( *BaseDisplay::instance(), toolbarstyle.fontset );
    if ( menustyle.fontset )
	XFreeFontSet( *BaseDisplay::instance(), menustyle.fontset );
    if ( menustyle.titlestyle.fontset )
	XFreeFontSet( *BaseDisplay::instance(), menustyle.titlestyle.fontset );
    if ( windowstyle.font )
	XFreeFont( *BaseDisplay::instance(), windowstyle.font );
    if ( toolbarstyle.font )
	XFreeFont( *BaseDisplay::instance(), toolbarstyle.font );
    if ( menustyle.font )
	XFreeFont( *BaseDisplay::instance(), menustyle.font );
    if ( menustyle.titlestyle.font )
	XFreeFont( *BaseDisplay::instance(), menustyle.titlestyle.font );
}

static XrmDatabase stylerc = 0;

void BStyle::setCurrentStyle( const string &newstyle )
{
    currentstyle = newstyle;
    stylerc = XrmGetFileDatabase( currentstyle.c_str() );
    if (! stylerc)
	stylerc = XrmGetFileDatabase(DEFAULTSTYLE);

    // load fonts/fontsets
    if ( windowstyle.fontset )
	XFreeFontSet( *BaseDisplay::instance(), windowstyle.fontset );
    if ( toolbarstyle.fontset )
	XFreeFontSet( *BaseDisplay::instance(), toolbarstyle.fontset );
    if ( menustyle.fontset )
	XFreeFontSet( *BaseDisplay::instance(), menustyle.fontset );
    if ( menustyle.titlestyle.fontset )
	XFreeFontSet( *BaseDisplay::instance(), menustyle.titlestyle.fontset );
    windowstyle.fontset = 0;
    toolbarstyle.fontset = 0;
    menustyle.fontset = 0;
    menustyle.titlestyle.fontset = 0;
    if ( windowstyle.font )
	XFreeFont( *BaseDisplay::instance(), windowstyle.font );
    if ( toolbarstyle.font )
	XFreeFont( *BaseDisplay::instance(), toolbarstyle.font );
    if ( menustyle.font )
	XFreeFont( *BaseDisplay::instance(), menustyle.font );
    if ( menustyle.titlestyle.font )
	XFreeFont( *BaseDisplay::instance(), menustyle.titlestyle.font );
    windowstyle.font = 0;
    toolbarstyle.font = 0;
    menustyle.font = 0;
    menustyle.titlestyle.font = 0;

    if (i18n->multibyte()) {
	windowstyle.fontset = readDatabaseFontSet("window.font", "Window.Font" );
       	toolbarstyle.fontset = readDatabaseFontSet("toolbar.font", "Toolbar.Font" );
	menustyle.fontset = readDatabaseFontSet("menu.frame.font", "Menu.Frame.Font" );
	menustyle.titlestyle.fontset =
	    readDatabaseFontSet("menu.title.font", "Menu.Title.Font" );

	windowstyle.fontset_extents = XExtentsOfFontSet( windowstyle.fontset );
	toolbarstyle.fontset_extents = XExtentsOfFontSet( toolbarstyle.fontset );
	menustyle.fontset_extents = XExtentsOfFontSet( menustyle.fontset );
	menustyle.titlestyle.fontset_extents =
	    XExtentsOfFontSet( menustyle.titlestyle.fontset );
    } else {
	windowstyle.font = readDatabaseFont("window.font", "Window.Font" );
	toolbarstyle.font = readDatabaseFont("toolbar.font", "Toolbar.Font" );
	menustyle.font = readDatabaseFont("menu.frame.font", "Menu.Frame.Font" );
	menustyle.titlestyle.font =
	    readDatabaseFont("menu.title.font", "Menu.Title.Font" );
    }

    // load window config
    windowstyle.focus.title =
	readDatabaseTexture("window.title.focus", "Window.Title.Focus", "white" );

    windowstyle.unfocus.title =
	readDatabaseTexture("window.title.unfocus", "Window.Title.Unfocus", "black" );
    windowstyle.focus.label =
	readDatabaseTexture("window.label.focus", "Window.Label.Focus", "white" );
    windowstyle.unfocus.label =
	readDatabaseTexture("window.label.unfocus", "Window.Label.Unfocus", "black" );
    windowstyle.focus.handle =
	readDatabaseTexture("window.handle.focus", "Window.Handle.Focus", "white" );
    windowstyle.unfocus.handle =
	readDatabaseTexture("window.handle.unfocus", "Window.Handle.Unfocus", "black" );
    windowstyle.focus.grip =
	readDatabaseTexture("window.grip.focus", "Window.Grip.Focus", "white" );
    windowstyle.unfocus.grip =
	readDatabaseTexture("window.grip.unfocus", "Window.Grip.Unfocus", "black" );
    windowstyle.focus.button =
	readDatabaseTexture("window.button.focus", "Window.Button.Focus", "white" );
    windowstyle.unfocus.button =
	readDatabaseTexture("window.button.unfocus", "Window.Button.Unfocus", "black" );
    windowstyle.buttonpressed =
	readDatabaseTexture("window.button.pressed", "Window.Button.Pressed", "white" );
    windowstyle.focus.framecolor =
	readDatabaseColor("window.frame.focusColor",
			  "Window.Frame.FocusColor", "white" );
    windowstyle.unfocus.framecolor =
	readDatabaseColor("window.frame.unfocusColor",
			  "Window.Frame.UnfocusColor", "black" );
    windowstyle.focus.labeltextcolor =
	readDatabaseColor("window.label.focus.textColor",
			  "Window.Label.Focus.TextColor", "white" );
    windowstyle.unfocus.labeltextcolor =
	readDatabaseColor("window.label.unfocus.textColor",
			  "Window.Label.Unfocus.TextColor", "white" );
    windowstyle.focus.piccolor =
	readDatabaseColor("window.button.focus.picColor",
			  "Window.Button.Focus.PicColor", "black" );
    windowstyle.unfocus.piccolor =
	readDatabaseColor("window.button.unfocus.picColor",
			  "Window.Button.Unfocus.PicColor", "white" );

    // load toolbar config
    toolbarstyle.texture =
	readDatabaseTexture("toolbar", "Toolbar", "black" );
    toolbarstyle.workspacelabel =
	readDatabaseTexture("toolbar.label", "Toolbar.Label", "black" );
    toolbarstyle.windowlabel =
	readDatabaseTexture("toolbar.windowLabel", "Toolbar.WindowLabel", "black" );
    toolbarstyle.button =
	readDatabaseTexture("toolbar.button", "Toolbar.Button", "white" );
    toolbarstyle.buttonpressed =
	readDatabaseTexture("toolbar.button.pressed",
			    "Toolbar.Button.Pressed", "white" );
    toolbarstyle.clocklabel =
	readDatabaseTexture("toolbar.clock", "Toolbar.Clock", "black" );
    toolbarstyle.workspacelabeltextcolor =
	readDatabaseColor("toolbar.label.textColor",
			  "Toolbar.Label.TextColor", "white" );
    toolbarstyle.windowlabeltextcolor =
	readDatabaseColor("toolbar.windowLabel.textColor",
			  "Toolbar.WindowLabel.TextColor", "white" );
    toolbarstyle.clocktextcolor =
	readDatabaseColor("toolbar.clock.textColor",
			  "Toolbar.Clock.TextColor", "white" );
    toolbarstyle.piccolor =
	readDatabaseColor("toolbar.button.picColor",
			  "Toolbar.Button.PicColor", "black" );

    // load menu config
    menustyle.titlestyle.texture =
	readDatabaseTexture("menu.title", "Menu.Title", "white" );
    menustyle.titlestyle.textcolor =
	readDatabaseColor("menu.title.textColor", "Menu.Title.TextColor", "black" );
    menustyle.texture =
	readDatabaseTexture("menu.frame", "Menu.Frame", "black" );
    menustyle.highlight =
	readDatabaseTexture("menu.hilite", "Menu.Hilite", "white" );
    menustyle.textcolor =
	readDatabaseColor("menu.frame.textColor", "Menu.Frame.TextColor", "white" );
    menustyle.disabledcolor =
	readDatabaseColor("menu.frame.disableColor",
			  "Menu.Frame.DisableColor", "black" );
    menustyle.hitextcolor =
	readDatabaseColor("menu.hilite.textColor", "Menu.Hilite.TextColor", "black" );

    // others
    bordercolor = readDatabaseColor("borderColor", "BorderColor", "black" );

    XrmValue value;
    char *value_type;

    if (XrmGetResource(stylerc, "window.justify", "Window.Justify",
		       &value_type, &value)) {
	if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
	    windowstyle.justify = BStyle::Right;
	else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
	    windowstyle.justify = BStyle::Center;
	else
	    windowstyle.justify = BStyle::Left;
    } else
	windowstyle.justify = BStyle::Left;

    if (XrmGetResource(stylerc, "toolbar.justify",
		       "Toolbar.Justify", &value_type, &value)) {
	if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
	    toolbarstyle.justify = BStyle::Right;
	else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
	    toolbarstyle.justify = BStyle::Center;
	else
	    toolbarstyle.justify = BStyle::Left;
    } else
	toolbarstyle.justify = BStyle::Left;

    if (XrmGetResource(stylerc, "menu.title.justify",
		       "Menu.Title.Justify",
		       &value_type, &value)) {
	if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
	    menustyle.titlestyle.justify = BStyle::Right;
	else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
	    menustyle.titlestyle.justify = BStyle::Center;
	else
	    menustyle.titlestyle.justify = BStyle::Left;
    } else
	menustyle.titlestyle.justify = BStyle::Left;

    if (XrmGetResource(stylerc, "menu.frame.justify",
		       "Menu.Frame.Justify",
		       &value_type, &value)) {
	if (strstr(value.addr, "right") || strstr(value.addr, "Right"))
	    menustyle.justify = BStyle::Right;
	else if (strstr(value.addr, "center") || strstr(value.addr, "Center"))
	    menustyle.justify = BStyle::Center;
	else
	    menustyle.justify = BStyle::Left;
    } else
	menustyle.justify = BStyle::Left;


    /*
      if (XrmGetResource(stylerc, "menu.bullet", "Menu.Bullet",
      &value_type, &value)) {
      if (! strncasecmp(value.addr, "empty", value.size))
      menustyle.bullet = Basemenu::Empty;
      else if (! strncasecmp(value.addr, "square", value.size))
      menustyle.bullet = Basemenu::Square;
      else if (! strncasecmp(value.addr, "diamond", value.size))
      menustyle.bullet = Basemenu::Diamond;
      else
      menustyle.bullet = Basemenu::Triangle;
      } else
      menustyle.bullet = Basemenu::Triangle;

      if (XrmGetResource(stylerc, "menu.bullet.position",
      "Menu.Bullet.Position", &value_type, &value)) {
      if (! strncasecmp(value.addr, "right", value.size))
      menustyle.bullet_pos = Basemenu::Right;
      else
      menustyle.bullet_pos = Basemenu::Left;
      } else
      menustyle.bullet_pos = Basemenu::Left;
    */


    // load bevel, border and handle widths
    if (XrmGetResource(stylerc, "handleWidth", "HandleWidth",
		       &value_type, &value)) {
	if (sscanf(value.addr, "%u", &handle_width) != 1 || handle_width > 100 ||
	    handle_width == 0)
	    handle_width = 6;
    } else
	handle_width = 6;

    if (XrmGetResource(stylerc, "borderWidth", "BorderWidth",
		       &value_type, &value)) {
	if (sscanf(value.addr, "%u", &border_width) != 1)
	    border_width = 1;
    } else
	border_width = 1;

    if (XrmGetResource(stylerc, "bevelWidth", "BevelWidth",
		       &value_type, &value)) {
	if (sscanf(value.addr, "%u", &bevel_width) != 1 || bevel_width > 100 ||
	    bevel_width == 0)
	    bevel_width = 3;
    } else
	bevel_width = 3;


    if (XrmGetResource(stylerc, "frameWidth", "FrameWidth",
		       &value_type, &value)) {
	if (sscanf(value.addr, "%u", &frame_width) != 1 || frame_width > 100 )
	    frame_width = bevel_width;
    } else
	frame_width = bevel_width;

    // load and exec root command
    if (XrmGetResource(stylerc,
		       "rootCommand",
		       "RootCommand", &value_type, &value)) {
#ifndef    __EMX__
	char displaystring[MAXPATHLEN];
	sprintf( displaystring, "DISPLAY=%s",
		 DisplayString( Blackbox::instance()->x11Display() ) );
	sprintf( displaystring + strlen( displaystring ) - 1, "%d", screen );

	bexec(value.addr, displaystring);
#else //   __EMX__
	spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", value.addr, NULL);
#endif // !__EMX__
    }

    XrmDestroyDatabase(stylerc);
    stylerc = 0;
}

BTexture BStyle::readDatabaseTexture( const string &rname, const string &rclass,
				      const string &default_color )
{
    BTexture texture;
    XrmValue value;
    char *value_type;

    if (XrmGetResource(stylerc, rname.c_str(), rclass.c_str(), &value_type, &value))
	texture = BTexture( value.addr );
    else
	texture.setTexture(BImage_Solid | BImage_Flat);

    if (texture.texture() & BImage_Solid) {
	texture.setColor( readDatabaseColor( rname + ".color",
					     rclass + ".Color",
					     default_color) );

#ifdef    INTERLACE
	texture.setColorTo( readDatabaseColor( rname + ".colorTo",
					       rclass + ".ColorTo",
					       default_color) );
#endif // INTERLACE

	unsigned char r, g, b;
	r = texture.color().red() | (texture.color().red() >> 1);
	g = texture.color().green() | (texture.color().green() >> 1);
	b = texture.color().blue() | (texture.color().blue() >> 1);
	texture.setLightColor( BColor( r, g, b ) );

	r = (texture.color().red() >> 2) | (texture.color().red() >> 1);
	g = (texture.color().green() >> 2) | (texture.color().green() >> 1);
	b = (texture.color().blue() >> 2) | (texture.color().blue() >> 1);
	texture.setShadowColor( BColor( r, g, b ) );
    } else if (texture.texture() & BImage_Gradient) {
	texture.setColor( readDatabaseColor( rname + ".color",
					     rclass + ".Color",
					     default_color) );
	texture.setColorTo( readDatabaseColor( rname + ".colorTo",
					       rclass + ".ColorTo",
					       default_color ) );
    }

    texture.setScreen( screen );
    return texture;
}

BColor BStyle::readDatabaseColor( const string &rname, const string &rclass,
				  const string &default_color)
{
    BColor color;
    XrmValue value;
    char *value_type;
    if (XrmGetResource(stylerc, rname.c_str(), rclass.c_str(), &value_type, &value))
	color = BColor( value.addr );
    else
	color = BColor( default_color );
    color.setScreen( screen );
    return color;
}

XFontSet BStyle::readDatabaseFontSet( const string &rname, const string &rclass )
{
    static char *defaultFont = "fixed";

    Bool load_default = False;
    XrmValue value;
    char *value_type;
    XFontSet fontset = 0;
    if ( XrmGetResource( stylerc, rname.c_str(), rclass.c_str(),
			 &value_type, &value ) ) {
	if ( ! ( fontset = createFontSet( value.addr ) ) )
	    load_default = True;
    } else
	load_default = True;

    if (load_default) {
	fontset = createFontSet(defaultFont);

	if (! fontset) {
	    fprintf(stderr, i18n->getMessage(ScreenSet, ScreenDefaultFontLoadFail,
					     "BStyle::setCurrentStyle(): couldn't load default font.\n"));
	    exit(2);
	}
    }

    return fontset;
}

XFontStruct *BStyle::readDatabaseFont( const string &rname, const string &rclass )
{
    static char *defaultFont = "fixed";

    Bool load_default = False;
    XrmValue value;
    char *value_type;
    XFontStruct *font = 0;
    if (XrmGetResource(stylerc, rname.c_str(), rclass.c_str(), &value_type, &value)) {
	if ( ( font = XLoadQueryFont( *BaseDisplay::instance(), value.addr ) ) == NULL ) {
	    fprintf(stderr, i18n->getMessage(ScreenSet, ScreenFontLoadFail,
					     "BStyle::setCurrentStyle(): couldn't load font '%s'\n"),
		    value.addr);

	    load_default = True;
	}
    } else
	load_default = True;


    if (load_default) {
	if ( ( font = XLoadQueryFont( *BaseDisplay::instance(), defaultFont ) ) == NULL ) {
	    fprintf(stderr, i18n->getMessage(ScreenSet, ScreenDefaultFontLoadFail,
					     "BStyle::setCurrentStyle(): couldn't load default font.\n"));
	    exit(2);
	}
    }

    return font;
}

static const char *getFontElement(const char *pattern, char *buf, int bufsiz, ...)
{
    const char *p, *v;
    char *p2;
    va_list va;

    va_start(va, bufsiz);
    buf[bufsiz-1] = 0;
    buf[bufsiz-2] = '*';
    while((v = va_arg(va, char *)) != NULL) {
	p = strcasestr(pattern, v);
	if (p) {
	    strncpy(buf, p+1, bufsiz-2);
	    p2 = strchr(buf, '-');
	    if (p2) *p2=0;
	    va_end(va);
	    return p;
	}
    }
    va_end(va);
    strncpy(buf, "*", bufsiz);
    return NULL;
}

static const char *getFontSize(const char *pattern, int *size)
{
    const char *p;
    const char *p2=NULL;
    int n=0;

    for (p=pattern; 1; p++) {
	if (!*p) {
	    if (p2!=NULL && n>1 && n<72) {
		*size = n; return p2+1;
	    } else {
		*size = 16; return NULL;
	    }
	} else if (*p=='-') {
	    if (n>1 && n<72 && p2!=NULL) {
		*size = n;
		return p2+1;
	    }
	    p2=p; n=0;
	} else if (*p>='0' && *p<='9' && p2!=NULL) {
	    n *= 10;
	    n += *p-'0';
	} else {
	    p2=NULL; n=0;
	}
    }
}

XFontSet BStyle::createFontSet( const string &fontname )
{
    XFontSet fs;
    char **missing, *def = "-";
    int nmissing, pixel_size = 0, buf_size = 0;
    char weight[FONT_ELEMENT_SIZE], slant[FONT_ELEMENT_SIZE];

    fs = XCreateFontSet(*BaseDisplay::instance(),
			fontname.c_str(), &missing, &nmissing, &def);
    if (fs && (! nmissing))
	return fs;

    const char *nfontname = fontname.c_str();
#ifdef    HAVE_SETLOCALE
    if (! fs) {
	if (nmissing) XFreeStringList(missing);

	setlocale(LC_CTYPE, "C");
	fs = XCreateFontSet(*BaseDisplay::instance(), fontname.c_str(),
			    &missing, &nmissing, &def);
	setlocale(LC_CTYPE, "");
    }
#endif // HAVE_SETLOCALE

    if (fs) {
	XFontStruct **fontstructs;
	char **fontnames;
	XFontsOfFontSet(fs, &fontstructs, &fontnames);
	nfontname = fontnames[0];
    }

    getFontElement(nfontname, weight, FONT_ELEMENT_SIZE,
		   "-medium-", "-bold-", "-demibold-", "-regular-", NULL);
    getFontElement(nfontname, slant, FONT_ELEMENT_SIZE,
		   "-r-", "-i-", "-o-", "-ri-", "-ro-", NULL);
    getFontSize(nfontname, &pixel_size);

    if (! strcmp(weight, "*")) strncpy(weight, "medium", FONT_ELEMENT_SIZE);
    if (! strcmp(slant, "*")) strncpy(slant, "r", FONT_ELEMENT_SIZE);
    if (pixel_size < 3) pixel_size = 3;
    else if (pixel_size > 97) pixel_size = 97;

    buf_size = strlen(nfontname) + (FONT_ELEMENT_SIZE * 2) + 64;
    char *pattern2 = new char[buf_size];
    snprintf(pattern2, buf_size - 1,
	     "%s,"
	     "-*-*-%s-%s-*-*-%d-*-*-*-*-*-*-*,"
	     "-*-*-*-*-*-*-%d-*-*-*-*-*-*-*,*",
	     nfontname, weight, slant, pixel_size, pixel_size);
    nfontname = pattern2;

    if (nmissing) XFreeStringList(missing);
    if (fs) XFreeFontSet(*BaseDisplay::instance(), fs);

    fs = XCreateFontSet(*BaseDisplay::instance(), nfontname, &missing, &nmissing, &def);

    delete [] pattern2;

    return fs;
}
