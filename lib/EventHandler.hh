// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// EventHandler.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2005 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2005
//         Bradley T Hughes <bhughes at trolltech.com>
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

#ifndef __EventHandler_hh
#define __EventHandler_hh

#include <X11/Xlib.h>

namespace bt {

  /*
    The abstract event handler.  You must register your event handler
    with the application object.  See
    bt::Application::insertEventHandler() and
    bt::Application::removeEventHandler().
  */
  class EventHandler
  {
  public:
    inline virtual ~EventHandler(void)
    { }

    // Mouse button press.
    inline virtual void buttonPressEvent(const XButtonEvent * const)
    { }
    // Mouse button release.
    inline virtual void buttonReleaseEvent(const XButtonEvent * const)
    { }
    // Mouse movement.
    inline virtual void motionNotifyEvent(const XMotionEvent * const)
    { }
    // Key press.
    inline virtual void keyPressEvent(const XKeyEvent * const)
    { }
    // Key release.
    inline virtual void keyReleaseEvent(const XKeyEvent * const)
    { }

    // Window configure (size, position, stacking, etc.).
    inline virtual void configureNotifyEvent(const XConfigureEvent * const)
    { }
    // Window shown.
    inline virtual void mapNotifyEvent(const XMapEvent * const)
    { }
    // Window hidden.
    inline virtual void unmapNotifyEvent(const XUnmapEvent * const)
    { }
    // Window reparented.
    inline virtual void reparentNotifyEvent(const XReparentEvent * const)
    { }
    // Window destroyed.
    inline virtual void destroyNotifyEvent(const XDestroyWindowEvent * const)
    { }

    // Mouse entered window.
    inline virtual void enterNotifyEvent(const XCrossingEvent * const)
    { }
    // Mouse left window.
    inline virtual void leaveNotifyEvent(const XCrossingEvent * const)
    { }

    // Window needs repainting.
    inline virtual void exposeEvent(const XExposeEvent * const)
    { }

    // Window property changed/added/deleted.
    inline virtual void propertyNotifyEvent(const XPropertyEvent * const)
    { }

    // Message passing.
    inline virtual void clientMessageEvent(const XClientMessageEvent * const)
    { }

    // Window shape changed. (Note: we use XEvent instead of
    // XShapeEvent to avoid the header.)
    inline virtual void shapeEvent(const XEvent * const)
    { }

  protected:
    inline EventHandler(void)
    { }
  };

} // namespace bt

#endif // __EventHandler_hh
