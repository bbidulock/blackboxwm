// LinkedList.cc for Blackbox - an X11 Window manager
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

#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#include "LinkedList.hh"

#include <stdio.h>


__llist_iterator::__llist_iterator(__llist *l) {
  // initialize the iterator...
  list = l;

  if (list) {
    if (list->iterator && list->iterator != this) {
      fprintf(stderr,
              "\nFATAL:  more than one iterator assigned to list %p\n", list);
      abort();
    }    

    list->iterator = this;
  }

  reset();
}


__llist_iterator::~__llist_iterator(void) {
  if (list && list->iterator == this)
    list->iterator = (__llist_iterator *) 0; 
}


void *__llist_iterator::current(void) {
  // return the current node data... if any
  return ((node) ? node->getData() : 0);
}


void __llist_iterator::reset(void) {
  // update the iterator's current node to the first node in the linked list
  if (list)
    node = list->_first;
}


const int __llist_iterator::set(const int index) {
  // set the current node to index
  if (list) {
    if (index < list->elements && index >= 0 && list->_first) {
      node = list->_first;
      
      for (register int i = 0; i < index; i++)
	node = node->getNext();
      
      return 1;
    }
  }
  
  node = (__llist_node *) 0;
  return 0;
}


void __llist_iterator::operator++(void) {
  // iterate to the next node in the list...
  node = ((node) ? node->getNext() : 0);
}     


void __llist_iterator::operator++(int) {
  // iterate to the next node in the list...
  node = ((node) ? node->getNext() : 0);
}


__llist::__llist(void *d) {
  // initialize the linked list...
  _first = (__llist_node *) 0;
  _last = (__llist_node *) 0;
  iterator = (__llist_iterator *) 0;
  elements = 0;
  
  if (d) insert(d);
}


__llist::~__llist(void) {
  // remove all the items in the list...
  for (register int i = 0, r = elements; i < r; i++)
    remove(0);

  if (iterator && iterator->list == this) iterator->list = (__llist *) 0;
}


const int __llist::insert(void *d, int index) {
  // insert item into linked list at specified index...
  
  if ((! _first) || (! _last)) {
    // list is empty... insert the item as the first item, regardless of the
    // index given
    _first = new __llist_node;
    _first->setData(d);
    _first->setNext((__llist_node *) 0);
    _last = _first;
  } else {
    if (index == 0) {
      // if index is 0... prepend the data on the list
      __llist_node *nnode = new __llist_node;
      
      nnode->setData(d);
      nnode->setNext(_first);
      
      _first = nnode;
    } else if ((index == -1) || (index == elements)) {
      // if index is -1... append the data on the list
      __llist_node *nnode = new __llist_node;
      
      nnode->setData(d);
      nnode->setNext((__llist_node *) 0);
      _last->setNext(nnode);

      _last = nnode;
    } else if (index < elements) {
      // otherwise... insert the item at the position specified by index      
      __llist_node *nnode = new __llist_node, *inode = _first->getNext();
      
      if (! nnode)
	return -1;
      
      nnode->setData(d);
      
      for (register int i = 1; i < index; i++)
	if (inode)
	  inode = inode->getNext();
	else {
	  delete nnode;
	  return -1;
	}
      
      if ((! inode) || inode == _last) {
	nnode->setNext((__llist_node *) 0);
	_last->setNext(nnode);
	
	_last = nnode;
      } else {
	nnode->setNext(inode->getNext());
	inode->setNext(nnode);
      }
    }
  }
  
  return ++elements;
}


const int __llist::remove(void *d) {
  // remove list item whose data pointer address matches the pointer address
  // given

  if ((! _first) || (! _last))
    return -1;
  else if (_first->getData() == d) {
    // remove the first item in the list...
    __llist_node *node = _first;
    _first = _first->getNext();

    if (iterator && iterator->node == node)
        iterator->reset();
    
    --elements;
    delete node;
    return 0;
  } else {
    // iterate through the list and remove the first occurance of the item
    
    // NOTE: we don't validate _first in this assignment, because it is checked
    // for validity above...
    __llist_node *rnode = _first->getNext(), *prev = _first;
    
    for (register int i = 1; i < elements; i++)
      if (rnode)
	if (rnode->getData() == d) {
	  // we found the item... update the previous node and delete the
	  // now useless rnode...
	  prev->setNext(rnode->getNext());
	  
	  if (rnode == _last)
	    _last = prev;

          if (iterator && iterator->node == rnode)
              iterator->node = prev;

	  --elements;
	  delete rnode;
	  return i;
	} else {
	  prev = rnode;
	  rnode = rnode->getNext();
	}
    
    return -1;
  }
}


void *__llist::remove(const int index) {
  if (index >= elements || index < 0 || (! _first) || (! _last))
    return (void *) 0;

  // remove list item at specified index within the list
  if (index == 0) {
    // remove the first item in the list...
    __llist_node *node = _first;
    void *data_return = _first->getData();
    
    _first = _first->getNext();

    if (iterator && iterator->node == node)
        iterator->reset();

    --elements;
    delete node;

    return data_return;
  } else {
    __llist_node *rnode = _first->getNext(), *prev = _first;
    void *data_return = (void *) 0;
    
    for (register int i = 1; i < index; i++)
      if (rnode) {
	prev = rnode;
	rnode = rnode->getNext();
      } else
	return (void *) 0;
    
    if (! rnode) return (void *) 0;
    
    prev->setNext(rnode->getNext());
    data_return = rnode->getData();
    
    if (rnode == _last)
      _last = prev;

    if (iterator && iterator->node == rnode)
        iterator->node = prev;

    --elements;
    data_return = rnode->getData();
    delete rnode;
    return data_return;
  }
  
  return (void *) 0;
}


void *__llist::find(const int index) {
  if (index >= elements || index < 0 || (! _first) || (! _last))
    return (void *) 0;

  if (index == 0) {
    // return the first item
    return first();
  } else if (index == (elements - 1)) {
    // return the last item
    return last();
  } else {
    __llist_node *fnode = _first->getNext();
    
    for (register int i = 1; i < index; i++)
      if (fnode)
	fnode = fnode->getNext();
      else
	return (void *) 0;
    
    return fnode->getData();
  }
  
  return (void *) 0;
}


void *__llist::first(void) {
  if (_first)
    return _first->getData();
  
  return (void *) 0;
}


void *__llist::last(void) {
  if (_last)
    return _last->getData();
  
  return (void *) 0;
}
