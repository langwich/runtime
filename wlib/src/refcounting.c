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
#include <pthread.h>

#include "wich.h"

static const int MAX_ROOTS = 1024;
static int sp = -1; // grow upwards; inc then set for push.
static heap_object **roots[MAX_ROOTS];

/* A global lock for synchronizing ref counting. We want a lock per object but
 * we want to free when refs goes to 0 but we can't destroy lock in an object
 * that could have other threads waiting. A subtle and hideous bug.
 * This is terribly inefficient but ref counting isn't my favorite in any case.
 * Also in threaded environment, locking on every REF/DEREF is just plain
 * ridiculous so let's make it easy on ourselves.
 */
static pthread_mutex_t refcounting_global_lock = PTHREAD_MUTEX_INITIALIZER;

PVector *PVector_alloc(size_t length) {
	PVector *p = (PVector *)calloc(1, sizeof(PVector) + length * sizeof(PVectorFatNode));
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
	String *p = (String *)calloc(1, sizeof(String) + length * sizeof(char));
	p->length = length;
//	printf("String %p\n", p);
	return p;
}

void free_object(heap_object *o) {
	if ( o->type==STRING ) {
#ifdef DEBUG
		printf("free(%p) string %s\n", x, (char *)o);
#endif
		free(o);
	}
	else if ( o->type==VECTOR ) {
#ifdef DEBUG
		printf("free(%p) vector\n", x);
#endif
		PVector *v = (PVector *)o;
		for (int i=0; i<v->length; i++) {
			PVectorFatNode *default_node = &v->nodes[i];
			pthread_mutex_lock(&default_node->lock);  // lock while we destroy the list
			PVectorFatNodeElem *p = default_node->head;
			// free all nodes in this fat node list
			// note: there is no need to lock with ref counting on PVectorFatNodeElem's; they are not exposed to programmer
			while ( p!=NULL ) {
				PVectorFatNodeElem *next = p->next;
				free(p);
				p = next;
			}
			default_node->head = NULL;                  // make sure no other threads try to free this list
			pthread_mutex_unlock(&default_node->lock);  // destroy requires that it be unlocked
			pthread_mutex_destroy(&default_node->lock); // destroy lock used in each fat node
		}

		free(o);    // free entire vector of fat nodes
	}
}

void DEREF(void *x) {
	if ( x!=NULL ) {
#ifdef DEBUG
		printf("DEREF(%p) has %d refs\n", x, ((heap_object *)x)->refs);
#endif
		// atomically decrement the ref count
		heap_object *o = (heap_object *)x;
		pthread_mutex_lock(&refcounting_global_lock);
		o->refs--;
		if ( o->refs==0 ) {
			// Exactly one thread will find its reference count going to zero; any other waiting threads
			// would find their reference count going negative. In this way only one thread will free this object.
			free_object(o); // destroys lock so no unlock after this call
		}
		pthread_mutex_unlock(&refcounting_global_lock);
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