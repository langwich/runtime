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
#ifndef RUNTIME_PERSISTENT_VECTOR_H
#define RUNTIME_PERSISTENT_VECTOR_H

static const unsigned INVALID_VERSION = (unsigned)-1;

/*
 * A DEREF(v) will have to deallocate all element linked-lists as well.
 * We can make DEREF and DEREF_Vector I guess.  Maybe that implies
 * we need DEREF_String for consistency.
 */

/* A node in a "fat node" list.  PVectors have an array of these that act as head pointers
 * and hold the value used to create the overall vector. This value is the default value
 * if next==NULL or version not found in list. It has an impossible -1 version
 * number so even the first reference to the vector will create a new vec_fat_node to
 * store a changed value.
 */
typedef struct _vec_fat_node {
	int version;
	double data;
	struct _vec_fat_node *next;
} vec_fat_node;

typedef struct {
	heap_object metadata;
	int version_count;      // could use time() but avoids system call
	size_t length;          // number of doubles (our vectors are fixed in length like arrays)
	vec_fat_node nodes[];   // a label to the start of the data part of vector
} PVector;

typedef struct {            // can't use "PVector *" since we need a versioned pointer
	int version;
	PVector *vector;
} PVector_ptr;

static inline PVector_ptr PVector_copy(PVector_ptr v) {
	return (PVector_ptr){++v.vector->version_count, v.vector};
}

PVector_ptr PVector_init(double val, size_t n);
PVector_ptr PVector_new(double *data, size_t n);
void print_pvector(PVector_ptr a);
double ith(PVector_ptr v, int i);
void set_ith(PVector_ptr v, int i, double value);
char *PVector_as_string(PVector_ptr a);

#endif
