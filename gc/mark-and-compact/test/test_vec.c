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
#include "gc.h"
#include "builtin.h"
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
	gc_shutdown();
	verify_heap();
	assert_equal(0, gc_num_roots());
}

void single_vector() {
	Vector *p = Vector_alloc(10);
	assert_addr_not_equal(p, NULL);
	assert_equal(p->length, 10);
	Heap_Info info = get_heap_info();
	assert_addr_equal(p, info.start_of_heap);
	assert_addr_equal(info.next_free, p + sizeof(Vector));
	assert_equal(info.busy_size, sizeof(Vector));
//	assert_equal(p->length)
}

int main(int argc, char *argv[]) {
	cunit_setup = setup;
	cunit_teardown = teardown;

	test_init_shutdown();

	gc_init(HEAP_SIZE);

	test(single_vector);

	gc_shutdown();
}