//
// llist.cc for Blackbox - an X11 Window manager
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

#define _GNU_SOURCE
#include "llist.hh"

#include <stdio.h>


// *************************************************************************
// Linked list class code
// *************************************************************************
//
// allocations:
// dynamic number of __llist_nodes
//
// *************************************************************************

__llist::__llist(void *d) {
  if (d == 0) {
    first = (__llist_node *) 0;
    last = (__llist_node *) 0;
    elements = 0;
  } else {
    first = new __llist_node;
    first->data = d;
    first->next = (__llist_node *) 0;
    last = first;
    elements = 1;
  }
}


__llist::~__llist(void) {
  int i, r = elements;
  for (i = 0; i < r; ++i)
    remove(0);
}


const int __llist::insert(void *d) {
  if (! first) {
    first = new __llist_node;
    first->data = d;
    first->next = (__llist_node *) 0;
    last = first;
  } else {
    // this is going to resemble a stack push operation
    __llist_node *node = new __llist_node;
    node->data = d;
    node->next = first;
    first = node;
  }

  return ++elements;
}


const int __llist::append(void *d) {
  if (! first) {
    first = new __llist_node;
    first->data = d;
    first->next = (__llist_node *) 0;
    last = first;
  } else {
    // append the data to the end of the list
    __llist_node *nnode = new __llist_node;
    nnode->data = d;
    nnode->next = (__llist_node *) 0;

    last->next = nnode;
    last = nnode;
  }

  return ++elements;
}


const int __llist::remove(void *d) {
  int i;
  __llist_node *prev = (__llist_node *) 0, *rnode = first;

  for (i = 0; i < elements; ++i) {
    if (rnode->data == d) {
      if (prev) prev->next = rnode->next;
      else first = rnode->next;

      if (rnode == last) last = prev;

      --elements;
      delete rnode;
      break;
    }

    prev = rnode;
    rnode = rnode->next;
  }

  return i;
}


const int __llist::remove(const int index) {
  int i;
  __llist_node *prev = (__llist_node *) 0, *rnode = first;

  for (i = 0; i < index; ++i) {
    prev = rnode;
    rnode = rnode->next;
  }

  if (prev) prev->next = rnode->next;
  else first = rnode->next;

  if (rnode == last) last = prev;

  --elements;
  delete rnode;
  return i;
}

void __llist::outputlist(void) {
  int i;
  __llist_node *pnode = first;

  for (i = 0; i < elements; ++i) {
    printf("node %d: address 0x%lx - data 0x%lx / next 0x%lx\n", i,
	   (unsigned long) pnode, (unsigned long) pnode->data,
	   (unsigned long) pnode->next);
    pnode = pnode->next;
  }

  fflush(stdout);
}


void *__llist::at(int index) {
  if (index < elements) {
    int i;
    __llist_node *anode = first;

    for (i = 0; i < index; ++i)
      anode = anode->next;

    return anode->data;
  }

  return (void *) 0;
}
