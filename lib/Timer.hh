// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Timer.hh for Blackbox - An X11 Window Manager
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

#ifndef   __Timer_hh
#define   __Timer_hh

#include "Util.hh"

#include <algorithm>
#include <queue>
#include <vector>

// forward declare to avoid the header
struct timeval;

namespace bt {

  // use a wrapper class to avoid the header as well
  struct timeval {
    long tv_sec;
    long tv_usec;

    inline timeval(void)
      : tv_sec(0l), tv_usec(0l)
    { }
    inline timeval(long s, long u)
      : tv_sec(s), tv_usec(u)
    { }

    bool operator<(const timeval &);
    timeval operator+(const timeval &);
    timeval &operator+=(const timeval &tv);
    timeval operator-(const timeval &);
    timeval &operator-=(const timeval &tv);

    // POSIX<->bt conversion
    timeval(const ::timeval &);
    timeval &operator=(const ::timeval &);
    operator ::timeval() const;
  };

  timeval normalizeTimeval(const timeval &tm);

  // forward declaration
  class TimerQueueManager;
  class Timer;

  class TimeoutHandler {
  public:
    virtual void timeout(Timer *t) = 0;
  };

  class Timer: public NoCopy {
  private:
    TimerQueueManager *manager;
    TimeoutHandler *handler;
    bool timing, recur;

    timeval _start, _timeout;

  public:
    Timer(TimerQueueManager *m, TimeoutHandler *h);
    virtual ~Timer(void);

    void fireTimeout(void);

    inline bool isTiming(void) const
    { return timing; }
    inline bool isRecurring(void) const
    { return recur; }

    inline const timeval &timeout(void) const
    { return _timeout; }
    inline const timeval &startTime(void) const
    { return _start; }

    // adjust the start time by the given offset... this is done when
    // the clock rolls back
    void adjustStartTime(const timeval &offset);

    timeval timeRemaining(const timeval &tm) const;
    bool shouldFire(const timeval &tm) const;
    timeval endpoint(void) const;

    inline void recurring(bool b)
    { recur = b; }

    void setTimeout(long t);
    void setTimeout(const timeval &t);

    void start(void);  // manager acquires timer
    void stop(void);   // manager releases timer
    void halt(void);   // halts the timer

    inline bool operator<(const Timer& other) const
    { return shouldFire(other.endpoint()); }
  };

  template <class _Tp, class _Sequence, class _Compare>
  class _timer_queue: protected std::priority_queue<_Tp, _Sequence, _Compare> {
  public:
    typedef std::priority_queue<_Tp, _Sequence, _Compare> _Base;

    inline _timer_queue(void)
      : _Base()
    { }
    inline ~_timer_queue(void)
    { }

    inline void release(const _Tp& value) {
      _Base::c.erase(std::remove(_Base::c.begin(), _Base::c.end(), value),
                     _Base::c.end());
      // after removing the item we need to make the heap again
      std::make_heap(_Base::c.begin(), _Base::c.end(), _Base::comp);
    }
    inline bool empty(void) const
    { return _Base::empty(); }
    inline size_t size(void) const
    { return _Base::size(); }
    inline void push(const _Tp& value)
    { _Base::push(value); }
    inline void pop(void)
    { _Base::pop(); }
    inline const _Tp& top(void) const
    { return _Base::top(); }
  private:
    // no copying!
    _timer_queue(const _timer_queue&);
    _timer_queue& operator=(const _timer_queue&);
  };

  struct TimerLessThan {
    inline bool operator()(const Timer* const l, const Timer* const r) const
    { return *r < *l; }
  };

  typedef _timer_queue<Timer*, std::vector<Timer*>, TimerLessThan> TimerQueue;

  class TimerQueueManager {
  public:
    virtual void addTimer(Timer* timer) = 0;
    virtual void removeTimer(Timer* timer) = 0;
  };

} // namespace bt

#endif // __Timer_hh
