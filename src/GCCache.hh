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
