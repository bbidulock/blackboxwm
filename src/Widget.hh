#ifndef WIDGET_HH
#define WIDGET_HH

#include <X11/Xlib.h>

#include "LinkedList.hh"
#include "Util.hh"

#include <hash_map>
#include <string>

class BColor;


class Widget
{
public:
    enum Type { Normal, Popup, OverrideRedirect };

    // create a toplevel with this constructor
    Widget( int, Type = Normal );
    // create a child with this constructor
    Widget( Widget * );
    virtual ~Widget();

    Window windowID() const { return win; }

    Widget *parent() const { return _parent; }

    int screen() const { return _screen; }

    Type type() const { return _type; }

    int x() const { return _rect.x(); }
    int y() const { return _rect.y(); }
    Point pos() const { return _rect.pos(); }
    void move( int, int );
    void move( const Point & );

    int width() const { return _rect.width(); }
    int height() const { return _rect.height(); }
    Size size() const { return _rect.size(); }
    virtual void resize( int, int );
    virtual void resize( const Size & );

    const Rect &rect() const { return _rect; }
    virtual void setGeometry( int, int, int, int );
    virtual void setGeometry( const Point &, const Size & );
    virtual void setGeometry( const Rect & );

    bool isVisible() const { return visible; }
    virtual void show();
    virtual void hide();

    bool hasFocus() const { return focused; }
    virtual void setFocus();

    const string &title() const { return _title; }
    virtual void setTitle( const string & );

    bool grabMouse();
    void ungrabMouse();

    bool grabKeyboard();
    void ungrabKeyboard();

    void setBackgroundColor( const BColor & );

protected:
    virtual void buttonPressEvent( XEvent * );
    virtual void buttonReleaseEvent( XEvent * );
    virtual void pointerMotionEvent( XEvent * );
    virtual void keyPressEvent( XEvent * );
    virtual void keyReleaseEvent( XEvent * );
    virtual void configureEvent( XEvent * );
    virtual void mapEvent( XEvent * );
    virtual void unmapEvent( XEvent * );
    virtual void focusInEvent( XEvent * );
    virtual void focusOutEvent( XEvent * );
    virtual void exposeEvent( XEvent * );
    virtual void enterEvent( XEvent * );
    virtual void leaveEvent( XEvent * );

private:
    void create();
    void insertChild( Widget * );
    void removeChild( Widget * );

    Window win;
    Widget *_parent;
    LinkedList<Widget> _children;
    Type _type;
    Rect _rect;
    string _title;
    bool visible;
    bool focused;
    bool grabbedMouse;
    bool grabbedKeyboard;
    int _screen;

    friend class BaseDisplay;
    static hash_map<Window,Widget*> mapper;
};

#endif // WIDGET_HH

