// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// GCCache.hh for Blackbox - an X11 Window manager
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

#ifndef GCCACHE_HH
#define GCCACHE_HH

#include <X11/Xlib.h>

class BColor;


class BGCCache {
public:
  class Item; // forward

private:
  class Context {
  public:
    Context() : gc( 0 ), pixel( 0x42000000 ), fontid( 0 ),
                function( 0 ), subwindow( 0 ), used( false ), screen( -1 )
    {
    }

    void set( const BColor &color, XFontStruct *font, int function, int subwindow );
    void set( XFontStruct *font );

    GC gc;
    unsigned long pixel;
    unsigned long fontid;
    int function;
    int subwindow;
    bool used;
    int screen;
  };

  Context *nextContext( int scr );
  void release( Context * );

public:
  class Item {
  public:
    const GC &gc() const { return ctx->gc; }

  private:
    Item() : ctx( 0 ), count( 0 ), hits( 0 ), fault( false ) { }
    Item( const Item & );
    Item &operator=( const Item & );

    Context *ctx;
    int count;
    int hits;
    bool fault;
    friend class BGCCache;
  };

  BGCCache();
  ~BGCCache();

  static BGCCache *instance();

  Item &find( const BColor &color, XFontStruct *font = 0,
              int function = GXcopy, int subwindow = ClipByChildren );
  void release( Item & );

  void purge();

private:
  // this is closely modelled after the Qt GC cache, but with some of the
  // complexity stripped out
  const int context_count;
  Context *contexts;
  const int cache_size;
  const int cache_buckets;
  Item **cache;
};

#endif // GCCACHE_HH
