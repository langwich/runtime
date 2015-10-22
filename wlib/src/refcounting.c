/*
The MIT License (MIT)

Copyright (c) 2015 Terence Parr, Hanzhou Shi, Shuai Yuan, Yuanyuan Zhang

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <stdlib.h>

#include "wich.h"
#include "persistent_vector.h"
#include "refcounting.h"

// WARNING: refcounting is not synchronized so concurrency is not supported
//          but we must still destroy mutex's created by persistent vector lib
//          during free.

static const int MAX_ROOTS = 1024;
static int sp = -1; // grow upwards; inc then set for push.
static heap_object **roots[MAX_ROOTS];

PVector *PVector_alloc(size_t length) {
	PVector *p = (PVector *)calloc(1, sizeof(PVector) + length * sizeof(PVectorFatNode));
	p->metadata.refs = 0;
	p->metadata.type = REFCOUNT_VECTOR_TYPE;
	p->length = length;
//	printf("PVector %p\n", p);
	return p;
}
PVectorFatNodeElem *PVectorFatNodeElem_alloc() {
	PVectorFatNodeElem *p = (PVectorFatNodeElem *)calloc(1, sizeof(PVectorFatNodeElem));
//	printf("fat node elem %p\n", p);
	return p;
}

String *String_alloc(size_t length) {
//	printf("sizeof(String) == %d, len=%d\n", sizeof(String), length);
	String *p = (String *)calloc(1, sizeof(String) + (length+1) * sizeof(char));
	p->metadata.refs = 0;
	p->metadata.type = REFCOUNT_STRING_TYPE;
//	printf("String addr %p\n", p);
	p->length = length;
	return p;
}

void free_object(heap_object *o) {
	if ( o->type==REFCOUNT_STRING_TYPE ) {
#ifdef DEBUG
		printf("free(%p) string %s\n", x, (char *)o);
#endif
		free(o);
	}
	else if (o->type == REFCOUNT_VECTOR_TYPE) {
#ifdef DEBUG
		printf("free(%p) vector\n", x);
#endif
		PVector *v = (PVector *)o;
		for (int i=0; i<v->length; i++) {
			PVectorFatNode *default_node = &v->nodes[i];
			PVectorFatNodeElem *p = default_node->head;
			// free all nodes in this fat node list
			while ( p!=NULL ) {
				PVectorFatNodeElem *next = p->next;
				free(p);
				p = next;
			}
			default_node->head = NULL;                  // make sure no other threads try to free this list
		}

		free(o);    // free entire vector of fat nodes
	}
}

int _heap_sp() { return sp; }
void _set_sp(int _sp) { sp = _sp; }

/* Announce a heap reference so we can _deref() all before exiting a function */
void _heapvar(heap_object **p) {
	roots[++sp] = p;
}

/* DEREF the stack a to b inclusive */
void _deref(int a, int b) {
#ifdef DEBUG
	printf("deref(%d,%d)\n", a,b);
#endif
	for (int i=a; i<=b; i++) {
		heap_object **addr_of_root = roots[i];
		heap_object *root = *addr_of_root;
#ifdef DEBUG
		printf("deref %p\n", root);
#endif
		DEREF(root);
	}
}