// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Timer.cc for Blackbox - An X11 Window Manager
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

extern "C" {
#ifdef    TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else // !TIME_WITH_SYS_TIME
#  ifdef    HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else // !HAVE_SYS_TIME_H
#    include <time.h>
#  endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME
}

#include "Timer.hh"
#include "Util.hh"


bt::Timer::Timer(TimerQueueManager *m, TimeoutHandler *h) {
  manager = m;
  handler = h;

  recur = timing = False;
}


bt::Timer::~Timer(void) {
  if (timing) stop();
}


void bt::Timer::setTimeout(long t) {
  _timeout.tv_sec = t / 1000;
  _timeout.tv_usec = t % 1000;
  _timeout.tv_usec *= 1000;
}


void bt::Timer::setTimeout(const ::timeval &t) {
  _timeout.tv_sec = t.tv_sec;
  _timeout.tv_usec = t.tv_usec;
}


void bt::Timer::start(void) {
  gettimeofday(&_start, 0);

  if (! timing) {
    timing = True;
    manager->addTimer(this);
  }
}


void bt::Timer::stop(void) {
  timing = False;

  manager->removeTimer(this);
}


void bt::Timer::halt(void) {
  timing = False;
}


void bt::Timer::fireTimeout(void) {
  if (handler)
    handler->timeout(this);
}


::timeval bt::Timer::timeRemaining(const ::timeval &tm) const {
  ::timeval ret = endpoint();

  ret.tv_sec  -= tm.tv_sec;
  ret.tv_usec -= tm.tv_usec;

  return bt::normalizeTimeval(ret);
}


::timeval bt::Timer::endpoint(void) const {
  ::timeval ret;

  ret.tv_sec = _start.tv_sec + _timeout.tv_sec;
  ret.tv_usec = _start.tv_usec + _timeout.tv_usec;

  return bt::normalizeTimeval(ret);
}


bool bt::Timer::shouldFire(const ::timeval &tm) const {
  ::timeval end = endpoint();

  return !((tm.tv_sec < end.tv_sec) ||
           (tm.tv_sec == end.tv_sec && tm.tv_usec < end.tv_usec));
}
