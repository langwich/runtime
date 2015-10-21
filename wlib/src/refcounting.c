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
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>

#include "refcounting.h"
#include "persistent_vector.h"

void DEREF(void *x) {
	if ( x!=NULL ) {
#ifdef DEBUG
		printf("DEREF(%p) has %d refs\n", x, ((heap_object *)x)->refs);
#endif
		heap_object *o = (heap_object *)x;
		o->refs--;
		if ( o->refs==0 ) {
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
				PVector *v = (PVector *)x;
				for (int i=0; i<v->length; i++ ) {
					PVectorFatNode *default_node = &v->nodes[i];
					PVectorFatNodeElem *p = default_node->head;
					// free all nodes in list
					while ( p!=NULL ) {
						pthread_mutex_destroy(&v->nodes[i].lock);
						PVectorFatNodeElem *next = p->next;
						free(p);
						p = next;
					}
				}

				free(o);    // free entire vector of fat nodes
			}
		}
	}
}