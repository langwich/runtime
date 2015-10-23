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
#include "freelist.h"
#include "cunit.h"

const size_t HEAP_SIZE = 2000;

extern void heap_shutdown();

Heap_Info verify_heap() {
	Heap_Info info = get_heap_info();
	assert_equal(info.heap_size, HEAP_SIZE);
	assert_equal(info.heap_size, info.busy_size+info.free_size);
	return info;
}

static void setup()		{ }
static void teardown()	{ verify_heap();
	heap_shutdown(); }

void malloc0() {
	void *p = malloc(0);
	assert_addr_not_equal(p, NULL);
	assert_equal(chunksize(p), MIN_CHUNK_SIZE);
}

void malloc1() {
	void *p = malloc(1);
	assert_addr_not_equal(p, NULL);
	assert_equal(chunksize(p), MIN_CHUNK_SIZE);
}

void malloc_word_size() {
	void *p = malloc(sizeof(void *));
	assert_addr_not_equal(p, NULL);
	assert_equal(chunksize(p), MIN_CHUNK_SIZE);
}

void malloc_2x_word_size() {
	void *p = malloc(2 * sizeof(void *));
	assert_addr_not_equal(p, NULL);
	assert_equal(chunksize(p), 3 * sizeof(void *)); // 2 words + rounded up size field
}

void one_malloc() {
	void *p = malloc(100); // should split heap into two chunks
	assert_addr_not_equal(p, NULL);
	Free_Header *freelist = get_freelist();
	Busy_Header *heap = get_heap_base();
	assert_addr_not_equal(freelist, heap);
	// check 1st chunk
	assert_equal(p, heap);
	assert_equal(chunksize(p), request2size(100));
	// check 2nd chunk
	assert_equal(freelist->size, HEAP_SIZE-request2size(100));
	assert_addr_equal(freelist->next, NULL);

	Heap_Info info = verify_heap();
	assert_equal(info.busy, 1);
	assert_equal(info.busy_size, request2size(100));
	assert_equal(info.free, 1);
	assert_equal(info.free_size, HEAP_SIZE - request2size(100));
}

void two_malloc() {
	one_malloc(); // split heap into two chunks and test for sanity.
	void *p0 = get_heap_base(); // should be first alloc chunk
	Free_Header *freelist0 = get_freelist();
	Busy_Header *p = malloc(200); // now split sole free chunk into two chunks
	assert_addr_not_equal(p, NULL);
	// check 2nd alloc chunk
	assert_equal(p, freelist0); // should return previous free chunk
	assert_equal(chunksize(p), request2size(200));
	// check remaining free chunk
	Free_Header *freelist1 = get_freelist();
	assert_addr_not_equal(freelist0, freelist1);
	assert_addr_not_equal(freelist0, get_heap_base());
	assert_equal(chunksize(freelist1), HEAP_SIZE-request2size(100)-request2size(200));
	assert_equal(chunksize(p0)+chunksize(p)+chunksize(freelist1), HEAP_SIZE);
	assert_addr_equal(freelist1->next, NULL);

	Heap_Info info = verify_heap();
	assert_equal(info.busy, 2);
	assert_equal(info.busy_size, request2size(100) + request2size(200));
	assert_equal(info.free, 1);
	assert_equal(info.free_size, HEAP_SIZE - request2size(100) - request2size(200));
}

void malloc_then_free() {
	one_malloc();
	void *p = get_heap_base(); // should be allocated chunk
	Free_Header *freelist0 = get_freelist();
	free(p);
	Free_Header *freelist1 = get_freelist();
	// allocated chunk is freed and becomes head of new freelist
	assert_addr_equal(freelist1, p);
	assert_addr_equal(freelist1, get_heap_base());
	assert_addr_not_equal(freelist0, freelist1);
	assert_equal(chunksize(freelist1) + chunksize(freelist1->next), HEAP_SIZE);

	Heap_Info info = verify_heap();
	assert_equal(info.busy, 0);
	assert_equal(info.busy_size, 0);
	assert_equal(info.free, 2); // 2 free chunks as we don't do merging
	assert_equal(info.free_size, HEAP_SIZE);
}

void free_NULL() {
	free(NULL); // don't crash
}

char buf[] = "hi"; // used by free_random

void free_random() {
	void *heap0 = get_heap_base();
	Free_Header *freelist0 = get_freelist();

	free(buf); // try to free a valid but non-heap data address

	void *heap1 = get_heap_base();
	Free_Header *freelist1 = get_freelist();

	assert_addr_equal(heap1, heap0);
	assert_addr_equal(freelist1, freelist0);
}

void free_stale() {
	malloc_then_free();
	Free_Header *freelist = get_freelist();
	free(freelist); // NOT ok; freeing something already free'd
	assert_addr_not_equal(freelist, freelist->next); // make sure we didn't create cycle by freeing twice
}

void test_core() {
	void *heap = morecore(HEAP_SIZE);
	assert_addr_not_equal(heap, NULL);
	dropcore(heap, HEAP_SIZE);
}

void test_init_shutdown() {
	heap_init(HEAP_SIZE);
	assert_addr_equal(get_freelist(), get_heap_base());
	heap_shutdown();
}

int main(int argc, char *argv[]) {
//	long pagesize = sysconf(_SC_PAGE_SIZE); // 4096 on my mac laptop
//	printf("pagesize == %ld\n", pagesize);

	cunit_setup = setup;
	cunit_teardown = teardown;

	test_core();
	test_init_shutdown();

	heap_init(HEAP_SIZE);

	test(malloc0);
	test(malloc1);
	test(malloc_word_size);
	test(malloc_2x_word_size);
	test(one_malloc);
	test(two_malloc);
	test(free_NULL);
	test(free_random);
	test(free_stale);
	test(malloc_then_free);
}