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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>

#if defined(MARK_AND_SWEEP)
#include <mark_and_sweep.h>
#include <gc.h>
#elif defined(MARK_AND_COMPACT)
#include <mark_and_compact.h>
#include <gc.h>
#elif defined(REFCOUNTING)
#include <refcounting.h>
#else // PLAIN
typedef struct {} heap_object; // no extra header info needed
#endif

#include <persistent_vector.h>

typedef struct string {
	heap_object metadata;
	size_t length;
	char str[];
	/* the string starts at the end of fixed fields; this field
	 * does not take any room in the structure; it's really just a
	 * label for the element beyond the length field. So, there is no
	 * need set this field. You must, however, copy strings into it.
	 * You cannot set p->str = "foo";
	 * Must terminate with '\0';
	 */
} String;

String *String_new(char *s);
String *String_from_char(char c);
String *String_add(String *s, String *t);
String *String_copy(String *s);
String *String_from_vector(PVector_ptr vector);
String *String_from_int(int value);
String *String_from_float(float value);

bool String_eq(String *s, String *t);
bool String_neq(String *s, String *t);
bool String_gt(String *s, String *t);
bool String_ge(String *s, String *t);
bool String_lt(String *s, String *t);
bool String_le(String *s, String *t);
void print_string(String *s);

PVector_ptr Vector_empty(size_t n);
PVector_ptr Vector_copy(PVector_ptr v);
PVector_ptr Vector_new(double *data, size_t n);
PVector_ptr Vector_append(PVector_ptr a, double value);
PVector_ptr Vector_append_vector(PVector_ptr a, PVector_ptr b);
PVector_ptr Vector_from_int(int value, size_t len);
PVector_ptr Vector_from_float(double value, size_t len);

PVector_ptr Vector_add(PVector_ptr a, PVector_ptr b);
PVector_ptr Vector_sub(PVector_ptr a, PVector_ptr b);
PVector_ptr Vector_mul(PVector_ptr a, PVector_ptr b);
PVector_ptr Vector_div(PVector_ptr a, PVector_ptr b);

void print_vector(PVector_ptr a);

// Following malloc/free are the hook where we create our own malloc/free or use the system's
void *wich_malloc(size_t nbytes);
void wich_free(heap_object *p);

// These allocator functions use different implementation according to compiler flags.
PVector *PVector_alloc(size_t length);
PVectorFatNodeElem *PVectorFatNodeElem_alloc();
String *String_alloc(size_t length);

static void
handle_sys_errors(int errno)
{
    char *signame = "UNKNOWN";

    if (errno == SIGSEGV)
        signame = "SIGSEGV";
    else if (errno == SIGBUS)
        signame = "SIGBUS";
    fprintf(stderr, "Wich is confused; signal %s (%d)\n", signame, errno);
    exit(errno);
}

static inline void setup_error_handlers() {
	signal(SIGSEGV, handle_sys_errors);
	signal(SIGBUS, handle_sys_errors);
}

static inline void COPY_ON_WRITE(void *x) {
//	if ( x!=NULL && ((heap_object *)x)->refs > 1 ) {
//		((heap_object *)x)->refs--;
//		x = Vector_copy(x);
//		((heap_object *)x)->refs = 1;
//	}
}