// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Timer.cc for Blackbox - An X11 Window Manager
// Copyright (c) 2001 - 2004 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2004
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

#include "Timer.hh"

#include <sys/time.h>


bt::timeval::timeval(const ::timeval &t)
  : tv_sec(t.tv_sec), tv_usec(t.tv_usec)
{ }


bt::timeval &bt::timeval::operator=(const ::timeval &t)
{ return (*this = timeval(t)); }


bt::timeval::operator ::timeval() const {
  ::timeval ret = { tv_sec, tv_usec };
  return ret;
}


bt::timeval bt::normalizeTimeval(const timeval &tm) {
  timeval ret = tm;

  while (ret.tv_usec < 0) {
    if (ret.tv_sec > 0) {
      --ret.tv_sec;
      ret.tv_usec += 1000000;
    } else {
      ret.tv_usec = 0;
    }
  }

  if (ret.tv_usec >= 1000000) {
    ret.tv_sec += ret.tv_usec / 1000000;
    ret.tv_usec %= 1000000;
  }

  if (ret.tv_sec < 0) ret.tv_sec = 0;

  return ret;
}


bt::Timer::Timer(TimerQueueManager *m, TimeoutHandler *h) {
  manager = m;
  handler = h;

  recur = timing = false;
}


bt::Timer::~Timer(void) {
  if (timing)
    stop();
}


void bt::Timer::setTimeout(long t) {
  _timeout.tv_sec = t / 1000;
  _timeout.tv_usec = t % 1000;
  _timeout.tv_usec *= 1000;
}


void bt::Timer::setTimeout(const timeval &t) {
  _timeout.tv_sec = t.tv_sec;
  _timeout.tv_usec = t.tv_usec;
}


void bt::Timer::start(void) {
  ::timeval s;
  gettimeofday(&s, 0);
  _start = s;

  if (!timing) {
    timing = true;
    manager->addTimer(this);
  }
}


void bt::Timer::stop(void) {
  timing = false;

  manager->removeTimer(this);
}


void bt::Timer::halt(void)
{ timing = false; }


void bt::Timer::fireTimeout(void)
{
  if (handler)
    handler->timeout(this);
}


bt::timeval bt::Timer::timeRemaining(const timeval &tm) const {
  timeval ret = endpoint();

  ret.tv_sec  -= tm.tv_sec;
  ret.tv_usec -= tm.tv_usec;

  return bt::normalizeTimeval(ret);
}


bt::timeval bt::Timer::endpoint(void) const {
  timeval ret;

  ret.tv_sec = _start.tv_sec + _timeout.tv_sec;
  ret.tv_usec = _start.tv_usec + _timeout.tv_usec;

  return bt::normalizeTimeval(ret);
}


bool bt::Timer::shouldFire(const timeval &tm) const {
  timeval end = endpoint();

  return !((tm.tv_sec < end.tv_sec) ||
           (tm.tv_sec == end.tv_sec && tm.tv_usec < end.tv_usec));
}
