// Color.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2002 Brad Hughes (bhughes@trolltech.com)
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "Color.hh"
#include "BaseDisplay.hh"

#include <stdio.h>

BColor::ColorCache BColor::colorcache;
bool BColor::cleancache = false;


BColor::BColor( int scr = -1 )
    : allocated( false ), r( -1 ), g( -1 ), b( -1 ), p( 0 ), scrn( scr )
{
}

BColor::BColor( int rr, int gg, int bb, int scr = -1 )
    : allocated( false ), r( rr ), g( gg ), b( bb ), p( 0 ), scrn( scr )
{
}

BColor::BColor( const string &name, int scr = -1 )
    : allocated( false ), r( -1 ), g( -1 ), b( -1 ), p( 0 ), scrn( scr ),
      colorname( name )
{
    parseColorName();
}

BColor::~BColor()
{
    deallocate();
}

void BColor::setScreen( int scr )
{
    if ( scr == scrn )
	return;

    deallocate();

    scrn = scr;
    if ( ! colorname.empty() )
	parseColorName();
}

unsigned long BColor::pixel() const
{
    if ( ! allocated ) {
	// mutable
	BColor *that = (BColor *) this;
	that->allocate();
    }

    return p;
}

void BColor::parseColorName()
{
    if ( colorname.empty() ) {
	fprintf( stderr, "BColor: empty colorname, cannot parse (using black)\n" );
	setRGB( 0, 0, 0 );
    }

    if ( scrn == -1 )
	scrn = DefaultScreen( BaseDisplay::instance()->x11Display() );

    BaseDisplay *display = BaseDisplay::instance();
    Colormap colormap = display->screenInfo( scrn )->colormap();

    // get rgb values from colorname
    XColor xcol;
    xcol.red = 0;
    xcol.green = 0;
    xcol.blue = 0;
    xcol.pixel = 0;

    if (! XParseColor(*display, colormap, colorname.c_str(), &xcol)) {
	fprintf(stderr, "BColor::allocate: color parse error: \"%s\"\n",
		colorname.c_str());
	setRGB( 0, 0, 0 );
	return;
    }

    setRGB( xcol.red >> 8, xcol.green >> 8, xcol.blue >> 8 );
}

void BColor::allocate()
{
    if ( scrn == -1 ) {
	abort();
	scrn = DefaultScreen( BaseDisplay::instance()->x11Display() );
    }

    BaseDisplay *display = BaseDisplay::instance();
    Colormap colormap = display->screenInfo( scrn )->colormap();

    if ( ! isValid() ) {
	if ( colorname.empty() ) {
	    fprintf( stderr, "BColor: cannot allocate invalid color (using black)\n" );
	    setRGB( 0, 0, 0 );
	} else
	    parseColorName();
    }

    // see if we have allocated this color before
    ColorCache::iterator it = colorcache.find( rgb( r, g, b, scrn ) );
    if ( it != colorcache.end() ) {
	// found
	allocated = true;
	p = (*it).second.p;
	(*it).second.count++;
	return;
    }

    // allocate color from rgb values
    XColor xcol;
    xcol.red =   r | r << 8;
    xcol.green = g | g << 8;
    xcol.blue =  b | b << 8;
    xcol.pixel = 0;

    if (! XAllocColor(*display, colormap, &xcol)) {
	fprintf(stderr, "BColor::allocate: color alloc error: rgb:%x/%x/%x\n",
		r, g, b );
	xcol.pixel = 0;
    }

    p = xcol.pixel;
    allocated = true;

    std::pair<rgb,pixelref> pr( rgb( r, g, b, scrn ), pixelref( p ) );
    colorcache.insert( pr );

    if ( cleancache )
	doCacheCleanup();
}

void BColor::deallocate()
{
    if ( ! allocated )
	return;

    ColorCache::iterator it = colorcache.find( rgb( r, g, b, scrn ) );
    if ( it != colorcache.end() )
	if ( (*it).second.count < 1 )
	    (*it).second.count = 0;
	else
	    (*it).second.count--;

    if ( cleancache )
	doCacheCleanup();

    allocated = false;
}

BColor &BColor::operator=( const BColor &c )
{
    deallocate();

    setRGB( c.r, c.g, c.b );
    colorname = c.colorname;
    scrn = c.scrn;
    return *this;
}

void BColor::cleanupColorCache()
{
    cleancache = true;
}

void BColor::doCacheCleanup()
{
    BaseDisplay *display = BaseDisplay::instance();

    unsigned long *pixels = new unsigned long[ colorcache.size() ];
    int i, count;

    for ( i = 0; i < display->getNumberOfScreens(); i++ ) {
	count = 0;
	ColorCache::iterator it = colorcache.begin();
	while ( it != colorcache.end() ) {
	    if ( (*it).second.count != 0 || (*it).first.screen != i ) {
		it++;
		continue;
	    }

	    pixels[ count++ ] = (*it).second.p;
	    ColorCache::iterator it2 = it;
	    it++;
	    colorcache.erase( it2 );
	}

	if ( count > 0 )
	    XFreeColors( *display, display->screenInfo( i )->colormap(),
			 pixels, count, 0 );
    }

    delete [] pixels;
    cleancache = false;
}
