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

#include "wich.h"
#include "persistent_vector.h"

/*
 * Per "Making Data Structures Persistent"
 * in JOURNAL OF COMPUTER AND SYSTEM SCIENCES Vol. 38, No. 1, February 1989
 * by Driscoll, Sarnak, Sleator, and Tarjan:
 *
 * "The structure is partially persistent if all versions can be
 *  accessed but only the newest version can be modified, and fully
 *  persistent if every version can be both accessed and modified."
 *
 * "Our first idea is to record all changes made to node fields in the
 * nodes themselves, without erasing old values of the fields. This
 * requires that we allow nodes to become arbitrarily “fat,” i.e., to
 * hold an arbitrary number of values of each field. To be more
 * precise, each fat node will contain the same information and
 * pointer fields as an ephemeral node (holding original field
 * values), along with space for an arbitrary number of extra field
 * values. Each extra field value has an associated field name and a
 * version stamp. The version stamp indicates the version in which the
 * named field was changed to have the specified value. In addition,
 * each fat node has its own version stamp, indicating the version in
 * which the node was created."
 */

static void inline vector_index_error(int i) {
	fprintf(stderr, "VectorIndexOutOfRange: %d\n",i);
}

PVector_ptr PVector_init(double val, size_t n) {
	PVector *v = PVector_alloc(n);
	v->version_count = -1; // first version is 0
	PVector_ptr p = {++v->version_count, v};
	for (int i = 0; i < n; i++) {
		v->nodes[i].data = val;
		v->nodes[i].head = NULL;
	}
	return p;
}

PVector_ptr PVector_new(double *data, size_t n) {
	PVector *v = PVector_alloc(n);
	v->version_count = -1; // first version is 0
	PVector_ptr p = {++v->version_count, v};
	for (int i = 0; i < n; i++) {
		v->nodes[i].data = data[i];
		v->nodes[i].head = NULL;
	}
	return p;
}

double ith(PVector_ptr vptr, int i) {
	if (i<0 || i>= vptr.vector->length) {
		vector_index_error((int)vptr.vector->length);
	}
	PVectorFatNode *default_node = &vptr.vector->nodes[i];
	if ( default_node->head==NULL ) {       // fast path
		return default_node->data;          // return default value if no version list
	}
	// Look for value associated with this version in list first
	PVectorFatNodeElem *p = default_node->head;
	while ( p!=NULL ) {
		if ( p->version== vptr.version ) {
			return p->data;
		}
		p = p->next;
	}
	// not found? return default value
	return default_node->data;
}

void set_ith(PVector_ptr vptr, int i, double value) {
	if (i<0 || i>= vptr.vector->length) {
		vector_index_error((int)vptr.vector->length);
		return;
	}
	PVectorFatNode *default_node = &vptr.vector->nodes[i];
	PVectorFatNodeElem *p = default_node->head;  // can never set default value in fat node after creation
	while (p != NULL) {
		if ( p->version== vptr.version ) {       // found our version so let's update it
			p->data = value;
			return;
		}
		p = p->next;
	}
	// if not found, we create a new fat node with (version,value) and make it the head of the list
	PVectorFatNodeElem *q = PVectorFatNodeElem_alloc();
	q->version = vptr.version;
	q->data = value;
	q->next = default_node->head;
	default_node->head = q;
}

char *PVector_as_string(PVector_ptr a) {
	char *s = calloc(a.vector->length*20, sizeof(char));
	char buf[50];
	strcat(s, "[");
	for (int i=0; i<a.vector->length; i++) {
		if ( i>0 ) strcat(s, ", ");
		sprintf(buf, "%1.2f", ith(a, i));
		strcat(s, buf);
	}
	strcat(s, "]");
	return s;
}

void print_pvector(PVector_ptr a) {
	char *vs = PVector_as_string(a);
	printf("%s\n", vs);
	free(vs);
}

