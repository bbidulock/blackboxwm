#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include <stdio.h>
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#include "Texture.hh"
#include "BaseDisplay.hh"
#include "Image.hh"
#include "Screen.hh"
#include "blackbox.hh"


BTexture::BTexture( int scr )
    : c( scr ), ct( scr ), lc( scr ), sc( scr ), t( 0 ), scrn( scr )
{
}

BTexture::BTexture( const string &d, int scr )
    : c( scr ), ct( scr ), lc( scr ), sc( scr ), t( 0 ), scrn( scr )
{
    setDescription( d );
}

BTexture::~BTexture()
{
}

void BTexture::setDescription( const string &d )
{
    string tmp( d.length(), '\0' );
    unsigned int i = 0;
    while ( i < d.length() ) {
	tmp[i] = tolower(d[i]);
	i++;
    }
    descr = tmp;

    if (descr.find("parentrelative") != string::npos) {
	setTexture(BImage_ParentRelative);
    } else {
	setTexture(0);

	if (descr.find("solid") != string::npos)
	    addTexture(BImage_Solid);
	else if (descr.find("gradient") != string::npos) {
	    addTexture(BImage_Gradient);
	    if (descr.find("crossdiagonal") != string::npos)
		addTexture(BImage_CrossDiagonal);
	    else if (descr.find("rectangle") != string::npos)
		addTexture(BImage_Rectangle);
	    else if (descr.find("pyramid") != string::npos)
		addTexture(BImage_Pyramid);
	    else if (descr.find("pipecross") != string::npos)
		addTexture(BImage_PipeCross);
	    else if (descr.find("elliptic") != string::npos)
		addTexture(BImage_Elliptic);
	    else if (descr.find("diagonal") != string::npos)
		addTexture(BImage_Diagonal);
	    else if (descr.find("horizontal") != string::npos)
		addTexture(BImage_Horizontal);
	    else if (descr.find("vertical") != string::npos)
		addTexture(BImage_Vertical);
	    else
		addTexture(BImage_Diagonal);
	} else
	    addTexture(BImage_Solid);

	if (descr.find("raised") != string::npos)
	    addTexture(BImage_Raised);
	else if (descr.find("sunken") != string::npos)
	    addTexture(BImage_Sunken);
	else if (descr.find("flat") != string::npos)
	    addTexture(BImage_Flat);
	else
	    addTexture(BImage_Raised);

	if (! (texture() & BImage_Flat))
	    if (descr.find("bevel2") != string::npos)
		addTexture(BImage_Bevel2);
	    else
		addTexture(BImage_Bevel1);

#ifdef    INTERLACE
	if (descr.find("interlaced") != string::npos)
	    addTexture(BImage_Interlaced);
#endif // INTERLACE
    }
}

void BTexture::setScreen( int scr )
{
    if ( scr == scrn )
	return;
    scrn = scr;
    c.setScreen( scrn );
    ct.setScreen( scrn );
    lc.setScreen( scrn );
    sc.setScreen( scrn );
}

BTexture &BTexture::operator=( const BTexture &tt )
{
    c  = tt.c;
    ct = tt.ct;
    lc = tt.lc;
    sc = tt.sc;
    t  = tt.t;
    return *this;
}

Pixmap BTexture::render( const Size &s, Pixmap old )
{
    if ( texture() == ( BImage_Flat | BImage_Solid ) )
	return None;

    if ( screen() == -1 )
	setScreen( DefaultScreen( BaseDisplay::instance()->x11Display() ) );

    BImageControl *ctrl = Blackbox::instance()->screen( screen() )->getImageControl();
    Pixmap ret = ctrl->renderImage( s.width(), s.height(), *this );

    if ( old )
	ctrl->removeImage( old );

    return ret;
}
