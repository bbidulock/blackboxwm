// LinkedList.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 1999 by Brad Hughes, bhughes@tcac.net
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

#ifndef   __LinkedList_hh
#define   __LinkedList_hh


class __llist_node {
private:
  __llist_node *next;
  void *data;

protected:

public:
  __llist_node(void) { next = (__llist_node *) 0; data = (void *) 0; }

  inline __llist_node *getNext(void) { return next; }

  inline void *getData(void) { return data; }
  inline void setData(void *d) { data = d; }
  inline void setNext(__llist_node *n) { next = n; }
};


// forward declaration
class __llist;


class __llist_iterator {
private:
  __llist *list;
  __llist_node *node;

  friend __llist;


protected:
  __llist_iterator(__llist *);
  ~__llist_iterator(void);

  const int set(const int);

  void *current(void);
  void reset(void);

  void operator++(void);
  void operator++(int);
};


class __llist {
private:
  int elements;
  __llist_node *_first, *_last;
  __llist_iterator *iterator;

  friend __llist_iterator;


protected:
  __llist(void * = 0);
  ~__llist(void);

  inline const int count(void) const { return elements; }
  inline const int empty(void) const { return (elements == 0); }

  const int insert(void *, int = -1);
  const int remove(void *);

  void *find(const int);
  void *remove(const int);
  void *first(void);
  void *last(void);
};


template <class Z>
class LinkedListIterator : public __llist_iterator {
public:
  LinkedListIterator(__llist *d = 0) : __llist_iterator(d) { return; }

  inline Z *current(void) { return (Z *) __llist_iterator::current(); }

  inline const int set(const int i) { return __llist_iterator::set(i); }

  inline void reset(void) { __llist_iterator::reset(); }
 
  inline void operator++(void) { __llist_iterator::operator++(); } 
  inline void operator++(int) { __llist_iterator::operator++(0); }
};


template <class Z>
class LinkedList : public __llist {
public:
  LinkedList(Z *d = 0) : __llist(d) { return; }
  
  inline Z *find(const int i) { return (Z *) __llist::find(i); }
  inline Z *remove(const int i) { return (Z *) __llist::remove(i); }
  inline Z *first(void) { return (Z *) __llist::first(); }
  inline Z *last(void) { return (Z *) __llist::last(); }
  
  inline const int count(void) const { return __llist::count(); }
  inline const int empty(void) const { return __llist::empty(); }
  inline const int insert(Z *d, int i = -1)
    { return __llist::insert((void *) d, i); }
  inline const int remove(Z *d) { return __llist::remove((void *) d); }
};


#endif // __LinkedList_hh

