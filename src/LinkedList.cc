//
// LinkedList.cc for Blackbox - an X11 Window manager
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "LinkedList.hh"


// *************************************************************************
// Linked List iterator class code
// *************************************************************************

__llist_iterator::__llist_iterator(__llist *l) {
  list = l;
  reset();
}


__llist_iterator::~__llist_iterator(void) {
}


void *__llist_iterator::current(void) {
  return ((node) ? node->data : 0);
}


void __llist_iterator::reset(void) {
  if (list)
    node = list->_first;
}


void __llist_iterator::resetLast(void) {
  if (list)
    node = list->_last;
}

const int __llist_iterator::set(const int index) {
  if (list) {
    if (index < list->elements && index >= 0) {
      node = list->_first;
      
      int i;
      for (i = 0; i < index; i++)
	node = node->next;
      
      return 1;
    }
  }

  node = (__llist_node *) 0;
  return 0;
}


void __llist_iterator::operator++(int) {
  node = ((node) ? node->next : 0);
}


void __llist_iterator::operator--(int) {
  node = ((node) ? node->prev : 0);
}


// *************************************************************************
// Linked List class code
// *************************************************************************

__llist::__llist(void *d) {
  if (! d) {
    _first = (__llist_node *) 0;
    _last = (__llist_node *) 0;
    elements = 0;
  } else {
    _first = new __llist_node;
    _first->data = d;
    _first->next = (__llist_node *) 0;
    _first->prev = (__llist_node *) 0;
    _last = _first;
    elements = 1;
  }
}


__llist::~__llist(void) {
  int i, r = elements;
  for (i = 0; i < r; i++)
    remove(0);
}


const int __llist::insert(void *d, int index) {
  if (! _first) {
    _first = new __llist_node;
    _first->data = d;
    _first->next = (__llist_node *) 0;
    _first->prev = (__llist_node *) 0;
    _last = _first;
  } else {
    // if index is -1... append the data to the end of the list
    if (index == -1) {
      __llist_node *nnode = new __llist_node;
      nnode->data = d;
      nnode->next = (__llist_node *) 0;

      nnode->prev = _last;
      _last->next = nnode;
      _last = nnode;
    } else {
      __llist_node *nnode = new __llist_node, *inode = _first;
      // otherwise... insert the item at the position specified by index
      if (index > elements) return -1;

      int i;
      for (i = 0; i < index; i++)
	inode = inode->next;
      
      if ((! inode) || inode == _last) {
	nnode->data = d;
	nnode->next = (__llist_node *) 0;
	
	nnode->prev = _last;
	_last->next = nnode;
	_last = nnode;
      } else {
	nnode->data = d;
	nnode->next = inode->next;
	nnode->prev = inode;
	
	inode->next->prev = nnode;
	inode->next = nnode;
      }
    }
  }

  return ++elements;
}

    
const int __llist::remove(void *d) {
  // remove list item whose data pointer address matches the pointer address
  // given
  
  __llist_node *rnode = _first;
  
  int i;
  for (i = 0; i < elements; i++) {
    if (rnode->data == d) {
      if (rnode->prev) {
	rnode->prev->next = rnode->next;
	if (rnode->next)
	  rnode->next->prev = rnode->prev;
      } else {
	// we removed the _first item in the list... reflect that removal in
	// the list internals
	_first = rnode->next;
	if (_first)
	  _first->prev = (__llist_node *) 0;
      }
      
      if (rnode == _last) {
	_last = rnode->prev;
	if (_last)
	  _last->next = (__llist_node *) 0;
      }
      
      --elements;
      delete rnode;
      break;
    }
    
    if (rnode)
      rnode = rnode->next;
    else
      return -1;
  }
  
  return i;
}


void *__llist::remove(const int index) {
  if (index < elements && index >= 0) {
    // remove list item at specified index within the list
    
    __llist_node *rnode = _first;
    void *data_return = 0;
    
    int i;
    for (i = 0; i < index; i++)
      rnode = rnode->next;
    
    if (rnode->prev) {
      rnode->prev->next = rnode->next;
      if (rnode->next)
	rnode->next->prev = rnode->prev;
    } else {
      // we removed the _first item in the list... reflect that removal in the
      // list internals
      _first = rnode->next;
      if (_first)
	_first->prev = (__llist_node *) 0;
    }
    
    if (rnode == _last) {
      _last = rnode->prev;
      if (_last)
	_last->next = (__llist_node *) 0;
    }
    
    --elements;
    data_return = rnode->data;
    delete rnode;
    return data_return;
  }
  
  return (void *) 0;
}


void *__llist::find(const int index) {
  if (index < elements && index >= 0) {
    __llist_node *fnode = _first;
    
    int i;
    for (i = 0; i < index; i++)
      fnode = fnode->next;
    
    return fnode->data;
  }
  
  return (void *) 0;
}
