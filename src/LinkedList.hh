//
// LinkedList.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@tcac.net
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

#ifndef __LinkedList_hh
#define __LinkedList_hh


typedef struct __llist_node {
  __llist_node *prev, *next;
  void *data;
} __llist_node;


// forward declaration
class __llist;


class __llist_iterator {
private:
  __llist *list;
  __llist_node *node;


protected:
  __llist_iterator(__llist *);
  ~__llist_iterator(void);

  const int set(const int);

  void *current(void);
  void reset(void);
  void resetLast(void);

  void operator++(int);
  void operator--(int);
};


class __llist {
private:
  int elements;
  __llist_node *_first, *_last;

  friend __llist_iterator;


protected:
  __llist(void * = 0);
  ~__llist(void);

  const int count(void) const { return elements; }
  const int empty(void) const { return (elements == 0); }
  const int insert(void *, int = -1);
  const int remove(void *);

  void *find(const int);
  void *remove(const int);
  void *first(void) { return _first->data; }
  void *last(void) { return _last->data; }
};


template <class Z>
class LinkedListIterator : public __llist_iterator {
public:
  LinkedListIterator(__llist *d = 0) : __llist_iterator(d) { }
  ~LinkedListIterator(void) { }

  Z *current(void) { return (Z *) __llist_iterator::current(); }

  const int set(const int i) { return __llist_iterator::set(i); }

  void reset(void) { __llist_iterator::reset(); }
  void resetLast(void) { __llist_iterator::resetLast(); }

  void operator++(int a) { __llist_iterator::operator++(a); }
  void operator--(int a) { __llist_iterator::operator--(a); }
};


template <class Z>
class LinkedList : public __llist {
public:
  LinkedList(Z *d = 0) : __llist(d) { }
  ~LinkedList(void) { }
  
  Z *find(const int i) { return (Z *) __llist::find(i); }
  Z *remove(const int i) { return (Z *) __llist::remove(i); }
  Z *first(void) { return (Z *) __llist::first(); }
  Z *last(void) { return (Z *) __llist::last(); }
  
  const int count(void) const { return __llist::count(); }
  const int empty(void) const { return __llist::empty(); }
  const int insert(Z *d, int i = -1) { return __llist::insert((void *) d, i); }
  const int remove(Z *d) { return __llist::remove((void *) d); }
};


#endif // __LinkedList_hh
