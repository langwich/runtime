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

// TODO: uses calloc for now to begin impl

PVector_ptr PVector_init(double val, size_t n) {
	PVector *v = PVector_alloc(n);
	v->version_count = -1; // first version is 0
	PVector_ptr p = {++v->version_count, v};
	for (int i = 0; i < n; i++) {
		v->nodes[i].version = INVALID_VERSION; // indicates this node has default value.
		v->nodes[i].data = val;
	}
	return p;
}

PVector_ptr PVector_new(double *data, size_t n) {
	PVector *v = PVector_alloc(n);
	v->version_count = -1; // first version is 0
	PVector_ptr p = {++v->version_count, v};
	for (int i = 0; i < n; i++) {
		v->nodes[i].version = INVALID_VERSION; // indicates this node has default value.
		v->nodes[i].data = data[i];
	}
	return p;
}

double ith(PVector_ptr v, int i) {
	if ( i>=0 && i<v.vector->length ) {
		vec_fat_node *default_node = &v.vector->nodes[i];
		// Look for value associated with this version in list first
		vec_fat_node *p = default_node->next;
		while ( p!=NULL ) {
			if ( p->version==v.version ) {
				return p->data;
			}
			p = p->next;
		}
		// not found? return default value
		return default_node->data;
	}
	return NAN;
}

void set_ith(PVector_ptr v, int i, double value) {
	if ( i>=0 && i<v.vector->length ) {
		vec_fat_node *default_node = &v.vector->nodes[i];
		vec_fat_node *p = default_node->next; // can never set default value in head fat node after creation
		while (p != NULL) {
			if ( p->version==v.version ) {
				p->data = value;
				return;
			}
			p = p->next;
		}
		// if not found, we create a new fat node with (version,value)
		vec_fat_node *q = calloc(1, sizeof(vec_fat_node));
		q->version = v.version;
		q->data = value;
		q->next = default_node->next;
		default_node->next = q;
	}
}

char *PVector_as_string(PVector_ptr a) {
	char *s = calloc(a.vector->length*20, sizeof(char));
	char buf[50];
	strcat(s, "[");
	for (int i=0; i<a.vector->length; i++) {
		if ( i>0 ) strcat(s, ", ");
		sprintf(buf, "%1.2f", a.vector->nodes[i].data);
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

