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

#ifndef REFCOUNTING
void REF(heap_object *x) { }
void DEREF(heap_object *x) { }
#endif

#if defined(PLAIN)
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
	String *p = (String *)calloc(1, sizeof(String) + (length+1) * sizeof(char));
	p->length = length;
//	printf("String %p\n", p);
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
	REF((heap_object *)v.vector);
	PVector_ptr result = PVector_copy(v);
	DEREF((heap_object *)v.vector); // might free(v)
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
	REF((heap_object *)a.vector);
	REF((heap_object *)b.vector);
	int i;
	if ( a.vector==NULL || b.vector==NULL || a.vector->length!=b.vector->length ) return NIL_VECTOR;
	size_t n = a.vector->length;
	PVector_ptr c = PVector_init(0, n);
	for (i=0; i<n; i++) c.vector->nodes[i].data = ith(a, i) + ith(b, i); // safe because we have sole ptr to c for now
	DEREF((heap_object *)a.vector);
	DEREF((heap_object *)b.vector);
	return c;
}

PVector_ptr Vector_sub(PVector_ptr a, PVector_ptr b)
{
	REF((heap_object *)a.vector);
	REF((heap_object *)b.vector);
	int i;
	if ( a.vector==NULL || b.vector==NULL || a.vector->length!=b.vector->length ) return NIL_VECTOR;
	size_t n = a.vector->length;
	PVector_ptr  c = PVector_init(0, n);
	for (i=0; i<n; i++) c.vector->nodes[i].data = ith(a, i) - ith(b, i);
	DEREF((heap_object *)a.vector);
	DEREF((heap_object *)b.vector);
	return c;
}

PVector_ptr Vector_mul(PVector_ptr a, PVector_ptr b)
{
	REF((heap_object *)a.vector);
	REF((heap_object *)b.vector);
	int i;
	if ( a.vector==NULL || b.vector==NULL || a.vector->length!=b.vector->length ) return NIL_VECTOR;
	size_t n = a.vector->length;
	PVector_ptr  c = PVector_init(0, n);
	for (i=0; i<n; i++) c.vector->nodes[i].data = ith(a, i) * ith(b, i);
	DEREF((heap_object *)a.vector);
	DEREF((heap_object *)b.vector);
	return c;
}

PVector_ptr Vector_div(PVector_ptr a, PVector_ptr b)
{
	REF((heap_object *)a.vector);
	REF((heap_object *)b.vector);
	int i;
	if ( a.vector==NULL || b.vector==NULL || a.vector->length!=b.vector->length ) return NIL_VECTOR;
	size_t n = a.vector->length;
	PVector_ptr  c = PVector_init(0, n);
	for (i=0; i<n; i++) c.vector->nodes[i].data = ith(a, i) / ith(b, i);
	DEREF((heap_object *)a.vector);
	DEREF((heap_object *)b.vector);
	return c;
}

void print_vector(PVector_ptr a)
{
	REF((heap_object *)a.vector);
	print_pvector(a);
	DEREF((heap_object *)a.vector);
}

String *String_new(char *orig)
{
	String *s = String_alloc(strlen(orig));
//	printf("String_new == %p\n", s);
//	printf("String_new @s->str == %p\n", s->str);
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
	REF((heap_object *)a);
	printf("%s\n", a->str);
	DEREF((heap_object *)a);
}

String *String_add(String *s, String *t)
{
	if ( s==NULL ) return t; // don't REF/DEREF as we might free our return value
	if ( t==NULL ) return s;
	REF((heap_object *)s);
	REF((heap_object *)t);
	size_t n = strlen(s->str) + strlen(t->str);
	String *u = String_alloc(n);
	strcpy(u->str, s->str);
	strcat(u->str, t->str);
	DEREF((heap_object *)s);
	DEREF((heap_object *)t);
	return u;
}

bool String_eq(String *s, String *t) {
	if (strlen(s->str) != strlen(t->str)) {
		return false;
	}
	else {
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

void print_alloc_strategy() {
#ifdef REFCOUNTING
	printf("REFCOUNTING\n");
#elif MARK_AND_SWEEP
	printf("MARK_AND_SWEEP\n");
#elif MARK_AND_COMPACT
	printf("MARK_AND_COMPACT\n");
#elif PLAIN
	printf("PLAIN\n");
#else
	printf("UNKNOWN\n");
#endif
}