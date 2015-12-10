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

/*
 * A DEREF(v) will have to deallocate all element linked-lists as well.
 * We can make DEREF_String and DEREF_Vector.
 */

typedef struct _PVectorFatNodeElem {
	heap_object metadata;       // fat node elements must meet be allocated and deallocated like any other
	int version;
	double data;
	struct _PVectorFatNodeElem *next;
} PVectorFatNodeElem;

/* A "fat node".  PVectors have an array of these that act as head pointers
 * and hold the value used to create the overall vector. This value is the default value
 * if next==NULL or version not found in list.
 */
typedef struct _PVectorFatNode {
	double data;                // Default/initial value
	PVectorFatNodeElem *head;   // the start of the fat node element linked list
} PVectorFatNode;

typedef struct {
	heap_object metadata;
	int version_count;          // could use time() but avoids system call
	size_t length;              // number of doubles (our vectors are fixed in length like arrays)
	PVectorFatNode nodes[];     // a vector of fat nodes
} PVector;

typedef struct {                // can't use "PVector *" since we need a versioned pointer
	int version;
	PVector *vector;
} PVector_ptr;

PVector_ptr PVector_copy(PVector_ptr vptr);
PVector_ptr PVector_init(double val, size_t n);
PVector_ptr PVector_new(double *data, size_t n);
void print_pvector(PVector_ptr a);
double ith(PVector_ptr vptr, int i);
void set_ith(PVector_ptr vptr, int i, double value);
char *PVector_as_string(PVector_ptr a);

#endif
