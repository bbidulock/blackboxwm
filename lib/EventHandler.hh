// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// EventHandler.hh for Blackbox - an X11 Window manager
// Copyright (c) 2002 Brad Hughes (bhughes@tcac.net),
//                    Sean 'Shaleh' Perry <shaleh@debian.org>
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

extern "C" {
#include <X11/Xlib.h>
#ifdef SHAPE
#  include <X11/extensions/shape.h>
#endif // SHAPE
}

class EventHandler
{
public:
  virtual ~EventHandler(void) { }

  // Mouse button press.
  virtual void buttonPressEvent(const XButtonEvent * const) { }
  // Mouse button release.
  virtual void buttonReleaseEvent(const XButtonEvent * const) { }
  // Mouse movement.
  virtual void motionNotifyEvent(const XMotionEvent * const) { }
  // Key press.
  virtual void keyPressEvent(const XKeyEvent * const) { }
  // Key release.
  virtual void keyReleaseEvent(const XKeyEvent * const) { }

  // Window configure (size, position, stacking, etc.).
  virtual void configureNotifyEvent(const XConfigureEvent * const) { }
  // Window shown.
  virtual void mapNotifyEvent(const XMapEvent * const) { }
  // Window hidden.
  virtual void unmapNotifyEvent(const XUnmapEvent * const) { }
  // Window reparented.
  virtual void reparentNotifyEvent(const XReparentEvent * const) { }
  // Window destroyed.
  virtual void destroyNotifyEvent(const XDestroyWindowEvent * const) { }

  // Mouse entered window.
  virtual void enterNotifyEvent(const XCrossingEvent * const) { }
  // Mouse left window.
  virtual void leaveNotifyEvent(const XCrossingEvent * const) { }

  // Window needs repainting.
  virtual void exposeEvent(const XExposeEvent * const) { }

  // Window property changed/added/deleted.
  virtual void propertyNotifyEvent(const XPropertyEvent * const) { }

  // Window size/position/stacking/etc. change request.
  virtual void configureRequestEvent(const XConfigureRequestEvent * const) { }

  // Message passing
  virtual void clientMessageEvent(const XClientMessageEvent * const) {}

#ifdef    SHAPE
  // Window shape changed.
  virtual void shapeEvent(const XShapeEvent * const) { }
#endif // SHAPE

protected:
  inline EventHandler(void) { }
};

#endif // __EventHandler_hh
