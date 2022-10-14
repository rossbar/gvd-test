/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include  <render.h>

    typedef struct nodelistitem nodelistitem_t;

    struct nodelistitem {
	node_t *curr;
	nodelistitem_t *next;
	nodelistitem_t *prev;
    };

    typedef struct {
	nodelistitem_t *first;
	nodelistitem_t *last;
	int sz;
    } nodelist_t;

    extern nodelist_t *mkNodelist(void);
    extern void freeNodelist(nodelist_t *);
    extern int sizeNodelist(nodelist_t * list);

    extern void appendNodelist(nodelist_t *, nodelistitem_t *,
			       Agnode_t * n);

    extern void realignNodelist(nodelist_t * list, nodelistitem_t * n);
    extern void insertNodelist(nodelist_t *, Agnode_t *, Agnode_t *, int);

    extern void reverseAppend(nodelist_t *, nodelist_t *);
    extern nodelist_t *reverseNodelist(nodelist_t * list);
    extern nodelist_t *cloneNodelist(nodelist_t * list);

#ifdef DEBUG
    extern void printNodelist(nodelist_t * list);
#endif

#ifdef __cplusplus
}
#endif
