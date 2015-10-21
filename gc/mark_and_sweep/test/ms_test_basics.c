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
#include <wich.h>
#include <gc.h>
#include <cunit.h>

const size_t HEAP_SIZE = 2000;

Heap_Info verify_heap() {
	Heap_Info info = get_heap_info();
	assert_equal(info.heap_size, HEAP_SIZE);
	assert_equal(info.heap_size, info.computed_busy_size + info.computed_free_size);
	return info;
}

static int __save_root_count = 0;

static void setup() {
	__save_root_count = gc_num_roots();
	gc_init(HEAP_SIZE);
}

static void teardown() {
	gc_set_num_roots(__save_root_count);
	verify_heap();
	gc_shutdown();
}

void test_init_shutdown() {
	gc_init(HEAP_SIZE);
	verify_heap();
	assert_equal(gc_num_live_objects(), 0);
	assert_equal(0, gc_num_roots());
}

void alloc_single_vector() {
	PVector *p = PVector_alloc(10);
	assert_addr_not_equal(p, NULL);
	assert_equal(gc_num_live_objects(), 0); // no roots into heap
	assert_equal(p->length, 10);
	size_t expected_size = align_to_word_boundary(sizeof(PVector) + p->length * sizeof(PVectorFatNode));
	assert_equal(p->metadata.size, expected_size);
	assert_str_equal(p->metadata.metadata->name, "PVector");

	Heap_Info info = get_heap_info();
	assert_addr_equal(p, info.start_of_heap);
	assert_equal(info.computed_busy_size, expected_size);
}

void alloc_single_string() {
	String *p = String_alloc(10);
	assert_addr_not_equal(p, NULL);
	assert_equal(gc_num_live_objects(), 0); // no roots into heap
	assert_equal(p->length, 10);
	size_t expected_size = align_to_word_boundary(sizeof(String) + p->length * sizeof(char));
	assert_equal(p->metadata.size, expected_size);
	assert_str_equal(p->metadata.metadata->name, "String");

	Heap_Info info = get_heap_info();
	assert_addr_equal(p, info.start_of_heap);
	assert_equal(info.computed_busy_size, expected_size);
}

void alloc_two_vectors() {
	PVector *p = PVector_alloc(10);
	assert_equal(gc_num_live_objects(), 0); // no roots into heap
	PVector *q = PVector_alloc(5);
	assert_equal(gc_num_live_objects(), 0); // no roots into heap
	assert_equal(gc_num_alloc_objects(), 2);
	assert_addr_not_equal(p, NULL);
	assert_addr_not_equal(q, NULL);
	assert_addr_not_equal(p, q);

	Heap_Info info = get_heap_info();
	size_t p_expected_size = align_to_word_boundary(sizeof(PVector) + p->length * sizeof(PVectorFatNode));
	size_t q_expected_size = align_to_word_boundary(sizeof(PVector) + q->length * sizeof(PVectorFatNode));
	assert_addr_equal(p, info.start_of_heap);
	assert_equal(info.computed_busy_size, p_expected_size + q_expected_size);
	assert_equal(info.computed_free_size, info.heap_size - (p_expected_size + q_expected_size));
}

void gc_after_single_vector_no_roots() {
	PVector_alloc(10);
	assert_equal(gc_num_live_objects(), 0); // no roots into heap
	gc();
	assert_equal(gc_num_live_objects(), 0);
	assert_equal(gc_num_alloc_objects(), 1);
}

void gc_after_single_vector_one_root() {
	PVector *p = PVector_alloc(10);
	gc_add_root((void **) &p);
	assert_equal(gc_num_live_objects(), 1);
	gc();
	assert_equal(gc_num_live_objects(), 1); // still there as p points at it
	gc();
	assert_equal(gc_num_live_objects(), 1); // still there as p points at it
	Heap_Info info = get_heap_info();
	size_t expected_size = align_to_word_boundary(sizeof(PVector) + p->length * sizeof(PVectorFatNode));
	assert_equal(info.computed_busy_size, expected_size);
	assert_equal(info.computed_free_size, info.heap_size - expected_size);
}

void gc_after_single_vector_one_root_then_kill_ptr() {
	PVector *p = PVector_alloc(10);
	gc_add_root((void **) &p);
	assert_equal(gc_num_live_objects(), 1);
	gc();
	assert_equal(gc_num_live_objects(), 1); // still there as p points at it

	p = NULL;
	gc();
	assert_equal(gc_num_live_objects(), 0);

	Heap_Info info = get_heap_info();
	assert_equal(info.busy, 0);
	assert_equal(info.computed_busy_size, 0);
	assert_equal(info.computed_free_size, info.heap_size);
}

void gc_after_single_vector_two_roots() {
	printf("hello!");
	PVector *p;
	PVector *q;
	gc_add_root((void **) &p);
	gc_add_root((void **) &q);

	p = PVector_alloc(10);
	q = p;

	assert_equal(gc_num_live_objects(), 1);
	gc();
	assert_equal(gc_num_live_objects(), 1); // still there as p,q point at it

	p = NULL;
	gc();
	assert_equal(gc_num_live_objects(), 1); // still a ptr

	q = NULL;
	gc();
	assert_equal(gc_num_live_objects(), 0); // no more roots into heap

	Heap_Info info = get_heap_info();
	assert_equal(info.computed_busy_size, 0);
	assert_equal(info.computed_free_size, info.heap_size);
}

void gc_compacts_vectors() {
	gc_begin_func();

	const int N = 5;
	PVector *v[N];
	for (int i = 0; i < N; i++) { gc_add_root((void **) &v[i]); }

	for (int i = 0; i < N; i++) { v[i] = PVector_alloc(i + 1); }

	assert_equal(gc_num_live_objects(), N);
	gc();
	assert_equal(gc_num_live_objects(), N); // everybody still there

	// kill a few and see if it compacts
	v[0] = NULL;
	v[2] = NULL;
	v[3] = NULL;
	// v[1], v[4] should slide to front of heap

	gc();
	assert_equal(gc_num_live_objects(), 2);
	assert_equal(v[1]->length, 1 + 1);
	assert_equal(v[4]->length, 4 + 1);

	gc_end_func(); // indicate all roots are gone

	gc();
	assert_equal(gc_num_live_objects(), 0);
}

int main(int argc, char *argv[]) {
	cunit_setup = setup;
	cunit_teardown = teardown;

	gc_debug(false);

	test_init_shutdown();

	test(alloc_single_vector);
	test(alloc_single_string);
	test(alloc_two_vectors);
	test(gc_after_single_vector_no_roots);
	test(gc_after_single_vector_one_root);
	test(gc_after_single_vector_one_root_then_kill_ptr);
	test(gc_after_single_vector_two_roots);
	test(gc_compacts_vectors);

	return 0;
}

