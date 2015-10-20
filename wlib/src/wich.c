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
#include <stdbool.h>
#include <wich.h>
#include "persistent_vector.h"

static const int MAX_ROOTS = 1024;
static int sp = -1; // grow upwards; inc then set for push.
static heap_object **roots[MAX_ROOTS];

#ifndef REFCOUNTING
void REF(heap_object *x) { }
void DEREF(heap_object *x) { }
#endif

#if defined(PLAIN) || defined(REFCOUNTING)
PVector *PVector_alloc(size_t length) {
	PVector *p = (PVector *)calloc(1, sizeof(PVector) + length * sizeof(PVectorFatNodeElem));
	p->length = length;
	return p;
}
PVectorFatNodeElem *PVectorFatNodeElem_alloc() {
	PVectorFatNodeElem *p = (PVectorFatNodeElem *)calloc(1, sizeof(PVectorFatNodeElem));
	return p;
}
String *String_alloc(size_t length) {
	String *p = (String *)calloc(1, sizeof(String) + length * sizeof(char));
	p->length = length;
	return p;
}
#endif

// There is a general assumption that support routines follow same
// ref counting convention as Wich code: functions REF their heap args and
// then DEREF before returning; it is the responsibility of the caller
// to REF/DEREF heap return values

PVector_ptr Vector_new(double *data, size_t n)
{
	return PVector_new(data, n);
}

PVector_ptr Vector_copy(PVector_ptr v)
{
	REF(v.vector);
	PVector_ptr result = PVector_copy(v);
	DEREF(v.vector); // might free(v)
	return result;
}

PVector_ptr Vector_empty(size_t n)
{
	PVector_ptr v = PVector_init(0.0, n);
	return v;
}

PVector_ptr Vector_from_int(int value, size_t len) {
	PVector_ptr result = PVector_init(value, len);
	return result;
}

PVector_ptr Vector_from_float(double value, size_t len) {
	PVector_ptr result = PVector_init(value, len);
	return result;
}

PVector_ptr Vector_add(PVector_ptr a, PVector_ptr b)
{
	REF(a.vector);
	REF(b.vector);
	int i;
	if ( a.vector==NULL || b.vector==NULL || a.vector->length!=b.vector->length ) return (PVector_ptr){-1,NULL};
	size_t n = a.vector->length;
	PVector_ptr c = PVector_init(0, n);
	for (i=0; i<n; i++) c.vector->nodes[i].data = ith(a, i) + ith(b, i);
	DEREF(a.vector);
	DEREF(b.vector);
	return c;
}

PVector_ptr Vector_sub(PVector_ptr a, PVector_ptr b)
{
	REF(a.vector);
	REF(b.vector);
	int i;
	if ( a.vector==NULL || b.vector==NULL || a.vector->length!=b.vector->length ) return (PVector_ptr){-1,NULL};
	size_t n = a.vector->length;
	PVector_ptr  c = PVector_init(0, n);
	for (i=0; i<n; i++) c.vector->nodes[i].data = ith(a, i) - ith(b, i);
	DEREF(a.vector);
	DEREF(b.vector);
	return c;
}

PVector_ptr Vector_mul(PVector_ptr a, PVector_ptr b)
{
	REF(a.vector);
	REF(b.vector);
	int i;
	if ( a.vector==NULL || b.vector==NULL || a.vector->length!=b.vector->length ) return (PVector_ptr){-1,NULL};
	size_t n = a.vector->length;
	PVector_ptr  c = PVector_init(0, n);
	for (i=0; i<n; i++) c.vector->nodes[i].data = ith(a, i) * ith(b, i);
	DEREF(a.vector);
	DEREF(b.vector);
	return c;
}

PVector_ptr Vector_div(PVector_ptr a, PVector_ptr b)
{
	REF(a.vector);
	REF(b.vector);
	int i;
	if ( a.vector==NULL || b.vector==NULL || a.vector->length!=b.vector->length ) return (PVector_ptr){-1,NULL};
	size_t n = a.vector->length;
	PVector_ptr  c = PVector_init(0, n);
	for (i=0; i<n; i++) c.vector->nodes[i].data = ith(a, i) / ith(b, i);
	DEREF(a.vector);
	DEREF(b.vector);
	return c;
}

static char *Vector_as_string(PVector_ptr a) // not called from Wich so no REF/DEREF
{
	return PVector_as_string(a);
}

void print_vector(PVector_ptr a)
{
	REF(a.vector);
	print_pvector(a);
	DEREF(a.vector);
}

String *String_new(char *orig)
{
	String *s = String_alloc(strlen(orig));
	strcpy(s->str, orig);
	return s;
}

String *String_from_char(char c)
{
	char buf[2] = {c, '\0'};
	return String_new(buf);
}

String *String_from_vector(PVector_ptr v) {
	if (v.vector == NULL) return NULL;
	char *s = calloc(v.vector->length*20, sizeof(char));
	char buf[50];
	for (int i=0; i<v.vector->length; i++) {
		sprintf(buf, "%d", (int)ith(v, i));
		strcat(s, buf);
	}
	return String_new(s);
}

String *String_from_int(int value) {
	char *s = calloc(20, sizeof(char));
	char buf[50];
	sprintf(buf,"%d",value);
	strcat(s, buf);
	return String_new(s);
}

String *String_from_float(float value) {
	char *s = calloc(20, sizeof(char));
	char buf[50];
	sprintf(buf,"%1.2f",value);
	strcat(s, buf);
	return String_new(s);
}

void print_string(String *a)
{
	REF(a);
	printf("%s\n", a->str);
	DEREF(a);
}

String *String_add(String *s, String *t)
{
	if ( s==NULL ) return t; // don't REF/DEREF as we might free our return value
	if ( t==NULL ) return s;
	REF(s);
	REF(t);
	size_t n = strlen(s->str) + strlen(t->str);
	String *u = String_alloc(n);
	strcpy(u->str, s->str);
	strcat(u->str, t->str);
	DEREF(s);
	DEREF(t);
	return u;
}

bool String_eq(String *s, String *t) {
	if (strlen(s->str) != strlen(t->str)) {
		return false;
	}else {
		size_t n = strlen(s->str);
		int i;
		for (i = 0; i < n; i++) {
			if(s->str[i] != t->str[i]) {
				return false;
			}
		}
	}
	return true;
}

bool String_neq(String *s, String *t) {
	return !String_eq(s,t);
}

bool String_gt(String *s, String *t) {
	size_t len_s = strlen(s->str);
	size_t len_t = strlen(t->str);
	if (len_s > len_t) { return true;}
	else if (len_s < len_t) {return false;}
	else {
		int i;
		for (i = 0; i < len_s; i++) {
			if(s->str[i] <= t->str[i]) {
				return false;
			}
		}
		return true;
	}
}

bool String_ge(String *s, String *t) {
	size_t len_s = strlen(s->str);
	size_t len_t = strlen(t->str);
	if (len_s > len_t) { return true;}
	else if (len_s < len_t) {return false;}
	else {
		int i;
		for (i = 0; i < len_s; i++) {
			if(s->str[i] < t->str[i]) {
				return false;
			}
		}
		return true;
	}
}
bool String_lt(String *s, String *t) {
	size_t len_s = strlen(s->str);
	size_t len_t = strlen(t->str);
	if (len_s < len_t) { return true;}
	else if (len_s > len_t) {return false;}
	else {
		int i;
		for (i = 0; i < len_s; i++) {
			if(s->str[i] >= t->str[i]) {
				return false;
			}
		}
		return true;
	}
}
bool String_le(String *s, String *t) {
	size_t len_s = strlen(s->str);
	size_t len_t = strlen(t->str);
	if (len_s < len_t) { return true;}
	else if (len_s > len_t) {return false;}
	else {
		int i;
		for (i = 0; i < len_s; i++) {
			if(s->str[i] > t->str[i]) {
				return false;
			}
		}
		return true;
	}
}
/** We don't have data aggregates like structs so no need to free nested pointers. */
void wich_free(heap_object *p)
{
	free(p);
}

void *wich_malloc(size_t nbytes)
{
	return malloc(nbytes);
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
