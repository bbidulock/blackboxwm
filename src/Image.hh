// Image.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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

#ifndef   __Image_hh
#define   __Image_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "Color.hh"
#include "LinkedList.hh"
#include "Timer.hh"

class ScreenInfo;

class BTexture;
class BImageControl;


class BImage {
public:
    BImage(BImageControl *, unsigned int, unsigned int);
    ~BImage(void);

    Pixmap render(const BTexture &);
    Pixmap render_solid(const BTexture &);
    Pixmap render_gradient(const BTexture &);

protected:
    Pixmap renderPixmap(void);
    XImage *renderXImage(void);

    void invert(void);
    void bevel1(void);
    void bevel2(void);
    void dgradient(void);
    void egradient(void);
    void hgradient(void);
    void pgradient(void);
    void rgradient(void);
    void vgradient(void);
    void cdgradient(void);
    void pcgradient(void);

private:
    BImageControl *control;

#ifdef    INTERLACE
    Bool interlaced;
#endif // INTERLACE

    XColor *colors;

    BColor from, to;
    int red_offset, green_offset, blue_offset, red_bits, green_bits, blue_bits;
    int ncolors, cpc, cpccpc;
    unsigned char *red, *green, *blue, *red_table, *green_table, *blue_table;
    unsigned int width, height, *xtable, *ytable;
};


class BImageControl : public TimeoutHandler {
private:
    Bool dither;
    BaseDisplay *display;
    ScreenInfo *screeninfo;
#ifdef    TIMEDCACHE
    BTimer *timer;
#endif // TIMEDCACHE

    Colormap colormap;

    Window window;
    XColor *colors;
    int colors_per_channel, ncolors, screen_number, screen_depth,
	bits_per_pixel, red_offset, green_offset, blue_offset,
	red_bits, green_bits, blue_bits;
    unsigned char red_color_table[256], green_color_table[256],
	blue_color_table[256];
    unsigned int *grad_xbuffer, *grad_ybuffer, grad_buffer_width,
	grad_buffer_height;
    unsigned long *sqrt_table, cache_max;

    typedef struct Cache {
	Pixmap pixmap;

	unsigned int count, width, height;
	unsigned long pixel1, pixel2, texture;
    } Cache;

    LinkedList<Cache> *cache;


protected:
    Pixmap searchCache(unsigned int, unsigned int, unsigned long,
		       const BColor &, const BColor &);


public:
    BImageControl(BaseDisplay *, ScreenInfo *, Bool = False, int = 4,
		  unsigned long = 300000l, unsigned long = 200l);
    virtual ~BImageControl(void);

    const Bool &doDither(void) { return dither; }

    ScreenInfo *getScreenInfo(void) { return screeninfo; }

    const Window &getDrawable(void) const { return window; }

    const int &getBitsPerPixel(void) const { return bits_per_pixel; }
    const int &getDepth(void) const { return screen_depth; }
    const int &getColorsPerChannel(void) const { return colors_per_channel; }

    unsigned long getSqrt(unsigned int);

    Pixmap renderImage(unsigned int, unsigned int, const BTexture &);

    void installRootColormap(void);
    void removeImage(Pixmap);
    void getColorTables(unsigned char **, unsigned char **, unsigned char **,
			int *, int *, int *, int *, int *, int *);
    void getXColorTable(XColor **, int *);
    void getGradientBuffers(unsigned int, unsigned int,
			    unsigned int **, unsigned int **);
    void setDither(Bool d) { dither = d; }
    void setColorsPerChannel(int);

    virtual void timeout(void);
};


#endif // __Image_hh

