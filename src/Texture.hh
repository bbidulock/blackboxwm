#ifndef TEXTURE_HH
#define TEXTURE_HH

#include "Color.hh"
#include "Util.hh"

// bevel options
#define BImage_Flat		(1l<<1)
#define BImage_Sunken		(1l<<2)
#define BImage_Raised		(1l<<3)

// textures
#define BImage_Solid		(1l<<4)
#define BImage_Gradient		(1l<<5)

// gradients
#define BImage_Horizontal	(1l<<6)
#define BImage_Vertical		(1l<<7)
#define BImage_Diagonal		(1l<<8)
#define BImage_CrossDiagonal	(1l<<9)
#define BImage_Rectangle	(1l<<10)
#define BImage_Pyramid		(1l<<11)
#define BImage_PipeCross	(1l<<12)
#define BImage_Elliptic		(1l<<13)

// bevel types
#define BImage_Bevel1		(1l<<14)
#define BImage_Bevel2		(1l<<15)

// inverted image
#define BImage_Invert		(1l<<16)

// parent relative image
#define BImage_ParentRelative   (1l<<17)

#ifdef    INTERLACE
// fake interlaced image
#  define BImage_Interlaced	(1l<<18)
#endif // INTERLACE


class BTexture {
public:
    BTexture( int scr = -1 );
    BTexture( const string &, int scr = -1 );
    ~BTexture();

    const BColor &color(void) const { return c; }
    void setColor( const BColor &cc ) { c = cc; }

    const BColor &colorTo(void) const { return ct; }
    void setColorTo( const BColor &cc ) { ct = cc; }

    const BColor &lightColor(void) const { return lc; }
    void setLightColor( const BColor &cc ) { lc = cc; }

    const BColor &shadowColor(void) const { return sc; }
    void setShadowColor( const BColor &cc ) { sc = cc; }

    unsigned long texture(void) const { return t; }
    void setTexture(unsigned long tt) { t = tt; }
    void addTexture(unsigned long tt) { t |= tt; }

    BTexture &operator=( const BTexture &tt );
    bool operator==( const BTexture &tt )
    {
	return ( c == tt.c  && ct == tt.ct && lc == tt.lc && sc == tt.sc && t == tt.t );
    }
    bool operator!=( const BTexture &tt )
    {
	return ( ! operator==( tt ) );
    }

    int screen() const { return scrn; }
    void setScreen( int scr );

    const string &description() const { return descr; }
    void setDescription( const string & );

    Pixmap render( const Size &, Pixmap old = 0 );

private:
    BColor c, ct, lc, sc;
    string descr;
    unsigned long t;
    int scrn;
};

#endif // TEXTURE_HH
