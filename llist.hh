//
// llist.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@arn.net
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// (See the included file COPYING / GPL-2.0)
//

#ifndef _blackbox_llist_hh
#define _blackbox_llist_hh


typedef struct __llist_node {
  __llist_node *next;
  void *data;
} __llist_node;


class __llist;


class __llist_iterator {
private:
  __llist *list;
  __llist_node *node;

protected:
  __llist_iterator(__llist *);
  ~__llist_iterator(void);

  void *current(void);
  void reset(void);

  void operator++(int);
};


class __llist {
private:
  int elements;
  __llist_node *first, *last;

  friend __llist_iterator;


protected:
  __llist(void * = 0);
  ~__llist(void);

  const int count(void) const { return elements; }
  const int empty(void) const { return (elements == 0); }

  void *find(const int);
  const int insert(void *);
  const int remove(void *);
  void *remove(const int);
  const int append(void *);
  void outputlist(void);
};


template <class Z>
class llist_iterator : public __llist_iterator {
public:
  llist_iterator(__llist *d = 0) : __llist_iterator(d) { }
  ~llist_iterator(void) { }

  Z *current(void) { return (Z *) __llist_iterator::current(); }
  void reset(void) { __llist_iterator::reset(); }

  void operator++(int a) { __llist_iterator::operator++(a); }
};


template <class Z>
class llist : public __llist {
public:
  llist(Z *d = 0) : __llist(d) { }
  ~llist(void) { }

  const int count(void) const { return __llist::count(); }
  const int empty(void) const { return __llist::empty(); }

  Z *find(const int i) { return (Z *) __llist::find(i); }
  const int insert(Z *d) { return __llist::insert((void *) d); }
  const int remove(Z *d) { return __llist::remove((void *) d); }
  Z *remove(const int i) { return (Z *) __llist::remove(i); }
  const int append(Z *d) { return __llist::append((void *) d); }
  void outputlist(void) { __llist::outputlist(); }
};


#endif
