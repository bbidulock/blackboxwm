// Basemenu.hh for Blackbox - an X11 Window manager
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

#ifndef   __Basemenu_hh
#define   __Basemenu_hh

#include "Widget.hh"

#include <slist>


// new basemenu popup

class Basemenu2 : public Widget
{
public:
    enum Function { Submenu, Custom };

    class Item {
    public:
	enum Type { Default, Separator };
	Item( Type t = Default )
	    : def( t == Default ), sep( t == Separator ),
	      active( false ), title( false ), enable( true ), checked( false ),
	      sub( 0 ), fun( Custom ), idx( -1 ), height( 0 ) { }
	Item( int f )
	    : def( false ), sep( false ),
	      active( false ), title( false ), enable( true ), checked( false ),
	      sub( 0 ), fun( f ), idx( -1 ), height( 0 ) { }
	Item( Basemenu2 *s )
	    : def( false ), sep( false ),
	      active( false ), title( false ), enable( true ), checked( false ),
	      sub( s ), fun( Submenu ), idx( -1 ), height( 0 ) { }

	bool isDefault() const { return def; }
	bool isSeparator() const { return sep; }
	bool isTitle() const { return title; }
	bool isActive() const { return active; }
      	bool isEnabled() const { return enable; }
	bool isChecked() const { return checked; }
	const string &label() const { return lbl; }
	int function() const { return fun; }
	Basemenu2 *submenu() const { return sub; }
	int index() const { return idx; }

    private:
	bool def;
	bool sep;
	bool active;
	bool title;
	bool enable;
	bool checked;
	Basemenu2 *sub;
	int fun;
	int idx;
	int height;
	string lbl;

	friend class Basemenu2;
    };

    Basemenu2( int scr );
    virtual ~Basemenu2();

    int insert( const string &label, const Item &item = Item::Default, int index = -1 );
    int insertSeparator() { return insert( string(), Item::Separator ); }
    void change( int index, const string &label, const Item &item = Item::Default );
    void remove( int index );

    int count() const { return items.size(); }

    void setItemEnabled( int, bool );
    bool isItemEnabled( int ) const;

    void setItemChecked( int, bool );
    bool isItemChecked( int ) const;

    void showTitle();
    void hideTitle();

    virtual void popup( int, int, bool = true );
    virtual void popup( const Point &, bool = true );
    virtual void hide();

protected:
    virtual void setActiveItem( const Rect &, Item & );
    virtual void showSubmenu( const Rect &, const Item & );
    virtual void updateSize();

    virtual void buttonPressEvent( XEvent * );
    virtual void buttonReleaseEvent( XEvent * );
    virtual void pointerMotionEvent( XEvent * );
    virtual void enterEvent( XEvent * );
    virtual void leaveEvent( XEvent * );
    virtual void exposeEvent( XEvent *);
    virtual void mapEvent( XEvent * );
    virtual void unmapEvent( XEvent * );
    virtual void configureEvent( XEvent * );

    virtual void titleClicked( const Point &, int );
    virtual void itemClicked( const Point &, const Item &, int );

private:
    void drawTitle();
    void drawItem( const Rect &, const Item & );
    void hideAll();

    Pixmap title_pixmap, items_pixmap, highlight_pixmap;
    Rect title_rect;
    Rect items_rect;
    slist<Item> items;
    Basemenu2 *parent_menu, *current_submenu;
    int motion;
    int rows, cols;
    int itemw;
    int indent;
    bool show_title;
    bool size_dirty;
    bool pressed;
    bool title_pressed;
};




// old basemenu

#include <X11/Xlib.h>

class Blackbox;
class BImageControl;
class BScreen;
class Basemenu;
class BasemenuItem;
#include "LinkedList.hh"


class Basemenu {
private:
  LinkedList<BasemenuItem> *menuitems;
  Blackbox *blackbox;
  Basemenu *parent;
  BImageControl *image_ctrl;
  BScreen *screen;

  Bool moving, visible, movable, torn, internal_menu, title_vis, shifted,
    hide_tree;
  int which_sub, which_press, which_sbl, alignment;

  struct _menu {
    Pixmap frame_pixmap, title_pixmap, hilite_pixmap, sel_pixmap;
    Window window, frame, title;

    char *label;
    int x, y, x_move, y_move, x_shift, y_shift, sublevels, persub, minsub,
      grab_x, grab_y;
    unsigned int width, height, title_h, frame_h, item_w, item_h, bevel_w,
      bevel_h;
  } menu;


protected:
  inline BasemenuItem *find(int index) { return menuitems->find(index); }
  inline void setTitleVisibility(Bool b) { title_vis = b; }
  inline void setMovable(Bool b) { movable = b; }
  inline void setHideTree(Bool h) { hide_tree = h; }
  inline void setMinimumSublevels(int m) { menu.minsub = m; }

  virtual void itemSelected(int, int) = 0;
  virtual void drawItem(int, Bool = False, Bool = False,
			int = -1, int = -1, unsigned int = 0,
			unsigned int = 0);
  virtual void redrawTitle();
  virtual void internal_hide(void);


public:
  Basemenu(BScreen *);
  virtual ~Basemenu(void);

    Window getWindowID() const { return menu.window; }

  inline const Bool &isTorn(void) const { return torn; }
  inline const Bool &isVisible(void) const { return visible; }

  inline const char *getLabel(void) const { return menu.label; }

  int insert(const char *, int = 0, const char * = (const char *) 0, int = -1);
  int insert(const char **, int = -1, int = 0);
  int insert(const char *, Basemenu *, int = -1);
  int remove(int);

  inline const int &getX(void) const { return menu.x; }
  inline const int &getY(void) const { return menu.y; }
  inline int getCount(void) { return menuitems->count(); }
  inline const int &getCurrentSubmenu(void) const { return which_sub; }

  unsigned int width(void) const { return menu.width; }
  unsigned int height(void) const { return menu.height; }
  unsigned int getTitleHeight(void) const { return menu.title_h; }

  inline void setInternalMenu(void) { internal_menu = True; }
  inline void setAlignment(int a) { alignment = a; }
  inline void setTorn(void) { torn = True; }
  inline void removeParent(void)
    { if (internal_menu) parent = (Basemenu *) 0; }

  Bool hasSubmenu(int);
  Bool isItemSelected(int);
  Bool isItemEnabled(int);

  void buttonPressEvent(XButtonEvent *);
  void buttonReleaseEvent(XButtonEvent *);
  void motionNotifyEvent(XMotionEvent *);
  void enterNotifyEvent(XCrossingEvent *);
  void leaveNotifyEvent(XCrossingEvent *);
  void exposeEvent(XExposeEvent *);

  void reconfigure(void);
  void setLabel(const char *n);
  void move(int, int);
  void update(void);
  void setItemSelected(int, Bool);
  void setItemEnabled(int, Bool);

  virtual void drawSubmenu(int);
  virtual void show(void);
  virtual void hide(void);

  enum { AlignDontCare = 1, AlignTop, AlignBottom };
  enum { Right = 1, Left };
  enum { Empty = 0, Square, Triangle, Diamond };
};


class BasemenuItem {
private:
  Basemenu *s;
  const char **u, *l, *e;
  int f, enabled, selected;

  friend class Basemenu;

protected:

public:
  BasemenuItem(const char *lp, int fp, const char *ep = (const char *) 0):
    s(0), u(0), l(lp), e(ep), f(fp), enabled(1), selected(0) {}

  BasemenuItem(const char *lp, Basemenu *mp): s(mp), u(0), l(lp), e(0), f(0),
					      enabled(1), selected(0) {}

  BasemenuItem(const char **up, int fp): s(0), u(up), l(0), e(0), f(fp),
					 enabled(1), selected(0) {}

  inline const char *exec(void) const { return e; }
  inline const char *label(void) const { return l; }
  inline const char **ulabel(void) const { return u; }
  inline const int &function(void) const { return f; }
  inline Basemenu *submenu(void) { return s; }

  inline const int &isEnabled(void) const { return enabled; }
  inline void setEnabled(int e) { enabled = e; }
  inline const int &isSelected(void) const { return selected; }
  inline void setSelected(int s) { selected = s; }
};


#endif // __Basemenu_hh
