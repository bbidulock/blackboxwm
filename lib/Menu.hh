// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Menu.hh for Blackbox - an X11 Window manager
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

#ifndef __Menu_hh
#define __Menu_hh

extern "C" {
#include <X11/Xresource.h>
}

#include "Color.hh"
#include "EventHandler.hh"
#include "Font.hh"
#include "Texture.hh"
#include "Timer.hh"
#include "Util.hh"

#include <bitset>
#include <list>



namespace bt {

  class Application;
  class ImageControl;
  class Menu;

  class MenuItem
  {
  public:
    enum Type { Normal, Separator };
    inline MenuItem(Type t, const std::string& l = std::string())
      : sub(0), lbl(l), ident(~0u), indx(~0u), height(0),
        separator(t == Separator),
        active(0), title(0), enabled(1), checked(0) { }
    inline MenuItem(Menu *s, const std::string& l)
      : sub(s), lbl(l), ident(~0u), indx(~0u), height(0), separator(0),
        active(0), title(0), enabled(1), checked(0) { }

    inline bool isSeparator(void) const { return bool(separator); }
    inline bool isActive(void) const { return bool(active); }
    inline bool isTitle(void) const { return bool(title); }
    inline bool isEnabled(void) const { return bool(enabled); }
    inline bool isChecked(void) const { return bool(checked); }

    inline unsigned int id(void) const { return ident; }
    inline unsigned int index(void) const { return indx; }

    inline Menu *submenu(void) const { return sub; }

    inline const std::string& label(void) const { return lbl; }

  private:
    Menu *sub;
    std::string lbl;
    unsigned int ident;
    unsigned int indx;
    unsigned int height;
    unsigned int separator : 1;
    unsigned int active    : 1;
    unsigned int title     : 1;
    unsigned int enabled   : 1;
    unsigned int checked   : 1;

    friend class Menu;
  };


  class Resource;

  class MenuStyle : public NoCopy
  {
  public:
    static MenuStyle *get(Application &app, unsigned int screen,
                          ImageControl *imagecontrol);

    void load(const Resource &resource);

    // fixed metrics
    unsigned int separatorHeight(void) const;
    unsigned int titleMargin(void) const;
    unsigned int frameMargin(void) const;
    unsigned int itemMargin(void) const;

    // textures
    inline const Texture &titleTexture(void) const
    { return title.texture; }
    inline const Texture &frameTexture(void) const
    { return frame.texture; }

    // colors
    inline const bt::Color &titleForegroundColor(void) const
    { return title.foreground; }
    inline const bt::Color &titleTextColor(void) const
    { return title.text; }
    inline const bt::Color &frameForegroundColor(void) const
    { return frame.foreground; }
    inline const bt::Color &frameTextColor(void) const
    { return frame.text; }

    // fonts
    inline const Font &titleFont(void) const
    { return title.font; }
    inline const Font &frameFont(void) const
    { return frame.font; }

    // convenience
    Pixmap titlePixmap(unsigned int width, unsigned int height,
                       Pixmap oldpixmap);
    Pixmap framePixmap(unsigned int width, unsigned int height,
                       Pixmap oldpixmap);
    Pixmap activePixmap(unsigned int width, unsigned int height,
                        Pixmap oldpixmap);

    // bitmaps
    Pixmap arrowBitmap() const { return bitmap.arrow; }
    Pixmap checkBitmap() const { return bitmap.check; }
    Pixmap closeBitmap() const { return bitmap.close; }

    // size calculations
    Rect titleRect(const std::string &text) const;
    Rect itemRect(const MenuItem &item) const;

    // drawing
    void drawTitle(Window window, const Rect &rect,
                   const std::string &text) const;
    void drawItem(Window window, const Rect &rect,
                  const MenuItem &item, Pixmap activePixmap) const;

  private:
    MenuStyle(Application &app, unsigned int screen,
              ImageControl *imagecontrol);
    ~MenuStyle(void);

    Application &_app;
    unsigned int _screen;
    ImageControl *_imagecontrol;
    struct _title {
      Texture texture;
      Color foreground;
      Color text;
      Font font;
      Alignment alignment;
    } title;
    struct _frame {
      Texture texture;
      Color foreground;
      Color text;
      Color disabled;
      Font font;
      Alignment alignment;
    } frame;
    struct _active {
      Texture texture;
      Color foreground;
      Color text;
      // font and alignment used from frame above
    } active;
    struct _bitmap {
      Pixmap arrow;
      Pixmap check;
      Pixmap close;
    } bitmap;
    unsigned int margin_w;

    static MenuStyle **styles;
  };


  class Menu : public EventHandler, public NoCopy
  {
  public:
    Menu(Application &app, unsigned int screen);
    virtual ~Menu(void);

    inline Window windowID(void) const { return _window; }

    unsigned int insertItem(const MenuItem &item, unsigned int id = ~0u,
                            unsigned int index = ~0u);
    unsigned int insertItem(const std::string& label,
                            unsigned int id = ~0u, unsigned int index = ~0u);
    unsigned int insertItem(const std::string& lavel, Menu *submenu,
                            unsigned int id = ~0u, unsigned int index = ~0u);
    void insertSeparator(unsigned int index = ~0u);

    void setItemEnabled(unsigned int id, bool enabled);
    bool isItemEnabled(unsigned int id) const;

    void setItemChecked(unsigned int id, bool checked);
    bool isItemChecked(unsigned int id) const;

    void removeItem(unsigned int id);
    void removeIndex(unsigned int index);
    void clear(void);

    inline unsigned int count(void) const { return items.size(); }

    inline const std::string& title(void) const { return _title; }
    inline void setTitle(const std::string& newtitle) { _title = newtitle; }
    void showTitle(void);
    void hideTitle(void);

    inline bool isVisible(void) const { return _visible; }
    virtual void popup(int x, int y, bool centered = true);
    void move(int x, int y);
    virtual void show(void);
    virtual void hide(void);

    virtual void refresh(void) { }
    virtual void reconfigure(void);

    inline bool autoDelete(void) const { return _auto_delete; }
    inline void setAutoDelete(bool ad) { _auto_delete = ad; }

  protected:
    virtual void updateSize(void);

    virtual void buttonPressEvent(const XButtonEvent * const event);
    virtual void buttonReleaseEvent(const XButtonEvent * const event);
    virtual void motionNotifyEvent(const XMotionEvent * const event);
    virtual void leaveNotifyEvent(const XCrossingEvent * const event);
    virtual void exposeEvent(const XExposeEvent * const event);
    virtual void keyPressEvent(const XKeyEvent * const event);

    virtual void titleClicked(unsigned int button);
    virtual void itemClicked(unsigned int id, unsigned int button);

    virtual void hideAll(void);

  private:
    typedef std::list<MenuItem> ItemList;
    typedef std::bitset<500> IdSet;

    unsigned int verifyId(unsigned int id = ~0u);
    void activateItem(const Rect &rect, MenuItem &item);
    void deactivateItem(const Rect &rect, MenuItem &item);
    void activateIndex(unsigned int index);
    void showActiveSubmenu(void);

    ItemList::iterator &find_item(unsigned int id) const;

    Application &_app;
    unsigned int _screen;

    Window _window;
    Pixmap _tpixmap, _fpixmap, _apixmap;

    Rect _rect;  // entire menu
    Rect _trect; // title
    Rect _frect; // frame
    Rect _irect; // items inside the frame

    Timer _timer;
    std::string _title;

    ItemList items;
    IdSet idset;

    Menu *_parent_menu;
    Menu *_active_submenu;
    unsigned int _motion;
    unsigned int _itemw;
    unsigned int _active_index;
    bool _auto_delete;
    bool _pressed;
    bool _title_pressed;
    bool _size_dirty;
    bool _show_title;
    bool _visible;
  };


} // namespace bt

#endif // __Menu_hh
