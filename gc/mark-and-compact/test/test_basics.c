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
#include <builtin.h>
#include <gc.h>
#include "cunit.h"

const size_t HEAP_SIZE = 2000;

Heap_Info verify_heap() {
	Heap_Info info = get_heap_info();
	assert_equal(info.heap_size, HEAP_SIZE);
	assert_equal(info.busy_size, info.computed_busy_size);
	assert_equal(info.heap_size, info.busy_size+info.free_size);
	return info;
}

static void setup()		{ gc_init(HEAP_SIZE); }
static void teardown()	{ verify_heap(); gc_shutdown(); }

void test_init_shutdown() {
	gc_init(HEAP_SIZE);
	verify_heap();
	assert_equal(gc_num_live_objects(), 0);
	gc_shutdown();
	verify_heap();
	assert_equal(0, gc_num_roots());
}

void alloc_single_vector() {
	Vector *p = Vector_alloc(10);
	assert_addr_not_equal(p, NULL);
	assert_equal(gc_num_live_objects(), 0); // no roots into heap
	assert_equal(p->length, 10);
	size_t expected_size = align_to_word_boundary(sizeof(Vector) + p->length * sizeof(double));
	assert_equal(p->header.size, expected_size);
	assert_str_equal(p->header.metadata->name, "Vector");

	Heap_Info info = get_heap_info();
	assert_addr_equal(p, info.start_of_heap);
	assert_addr_equal(info.next_free, ((void *)p) + expected_size);
	assert_equal(info.busy_size, expected_size);
}

void alloc_single_string() {
	String *p = String_alloc(10);
	assert_addr_not_equal(p, NULL);
	assert_equal(gc_num_live_objects(), 0); // no roots into heap
	assert_equal(p->length, 10);
	size_t expected_size = align_to_word_boundary(sizeof(String) + p->length * sizeof(char));
	assert_equal(p->header.size, expected_size);
	assert_str_equal(p->header.metadata->name, "String");

	Heap_Info info = get_heap_info();
	assert_addr_equal(p, info.start_of_heap);
	assert_addr_equal(info.next_free, ((void *)p) + expected_size);
	assert_equal(info.busy_size, expected_size);
}

void alloc_two_vectors() {
	Vector *p = Vector_alloc(10);
	assert_equal(gc_num_live_objects(), 0); // no roots into heap
	Vector *q = Vector_alloc(5);
	assert_equal(gc_num_live_objects(), 0); // no roots into heap
	assert_addr_not_equal(p, NULL);
	assert_addr_not_equal(q, NULL);
	assert_addr_not_equal(p,q);

	Heap_Info info = get_heap_info();
	size_t p_expected_size = align_to_word_boundary(sizeof(Vector) + p->length * sizeof(double));
	size_t q_expected_size = align_to_word_boundary(sizeof(Vector) + q->length * sizeof(double));
	assert_addr_equal(p, info.start_of_heap);
	assert_addr_equal(info.next_free, ((void *)q) + q_expected_size);
	assert_equal(info.busy_size, p_expected_size + q_expected_size);
	assert_equal(info.free_size, info.heap_size - (p_expected_size + q_expected_size));
}

void gc_after_single_vector_no_roots() {
	Vector *p = Vector_alloc(10);
	assert_equal(gc_num_live_objects(), 0); // no roots into heap
	gc();
	assert_equal(gc_num_live_objects(), 0);
}

void gc_after_single_vector_one_root() {
	gc_debug(true);
	Vector *p = Vector_alloc(10);
	gc_add_root(&p);
	assert_equal(gc_num_live_objects(), 1);
	gc();
	assert_equal(gc_num_live_objects(), 1); // still there as p points at it
	gc_debug(false);
	Heap_Info info = get_heap_info();
	size_t expected_size = align_to_word_boundary(sizeof(Vector) + p->length * sizeof(double));
	assert_equal(info.busy_size, expected_size);
	assert_equal(info.free_size, info.heap_size - expected_size);
}

int main(int argc, char *argv[]) {
	cunit_setup = setup;
	cunit_teardown = teardown;

	test_init_shutdown();

	gc_init(HEAP_SIZE);

	test(alloc_single_vector);
	test(alloc_single_string);

	test(alloc_two_vectors);

	test(gc_after_single_vector_no_roots);

	test(gc_after_single_vector_one_root);

	gc_shutdown();

	return 0;
}