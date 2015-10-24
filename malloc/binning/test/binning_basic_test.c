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
#include "binning.h"
#include "cunit.h"

const size_t HEAP_SIZE = 4000;

Heap_Info verify_heap() {
    Heap_Info info = get_heap_info();
    assert_equal(info.heap_size, HEAP_SIZE);
    assert_equal(info.heap_size, info.busy_size+info.free_size);
    return info;
}

static void setup()		{ heap_init(HEAP_SIZE); }
static void teardown()	{ verify_heap(); heap_shutdown(); }

void malloc0() {
    void *p = malloc(0);
    assert_addr_not_equal(p, NULL);
    assert_equal(chunksize(p), MIN_CHUNK_SIZE);
}

void malloc_word_size() {
    void *p = malloc(sizeof(void *));
    assert_addr_not_equal(p, NULL);
    assert_equal(chunksize(p), MIN_CHUNK_SIZE);
}

void one_malloc() {
    void *p = malloc(100);
    assert_addr_not_equal(p, NULL);
    Free_Header *freelist = get_heap_freelist();
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
    one_malloc();
    void *p0 = get_heap_base();
    Free_Header *freelist0 = get_heap_freelist();
    Busy_Header *p = malloc(200);
    assert_addr_not_equal(p, NULL);
    // check 2nd alloc chunk
    assert_equal(p, freelist0); // should return previous free chunk
    assert_equal(chunksize(p), request2size(200));
    // check remaining free chunk
    Free_Header *freelist1 = get_heap_freelist();
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

void bin_malloc_free_malloc() {  // test bin malloc and free
    void *p = malloc(99); // should split heap into two chunks
    assert_addr_not_equal(p, NULL);
    Free_Header *freelist = get_heap_freelist();
    Busy_Header *heap = get_heap_base();
    assert_addr_not_equal(freelist, heap);
    // check 1st chunk
    assert_equal(p, heap);
    assert_equal(chunksize(p), request2size(99));
    // check 2nd chunk
    assert_equal(freelist->size, HEAP_SIZE-request2size(99));
    assert_addr_equal(freelist->next, NULL);

    Free_Header *freelist1 = get_bin_freelist(99);// freelist of bin should be NULL
    free(p);                                                       // free to bin
    Free_Header *freelist2 = get_bin_freelist(99);// freelist of bin should be have one element p
    assert_addr_not_equal(freelist1,freelist2);
    void *p1 = malloc(99);                                        // this malloc should be from bin
    assert_addr_not_equal(p1, NULL);
    Free_Header *freelist3 = get_bin_freelist(99); // freelist of bin should be NULL
    assert_addr_equal(freelist3,NULL);

    free(p1);
    Free_Header *freelist4 = get_bin_freelist(99); // freelist should have one element p1
    assert_addr_equal(freelist2,freelist4);
}

void malloc_large_size_then_free() {
    Free_Header *freelist = get_heap_freelist();//free list before malloc
    void *p = malloc(2048);
    assert_addr_not_equal(p, NULL);
    assert_equal(chunksize(p), request2size(2048));
    Free_Header *freelist1 = get_heap_freelist(); // free list after malloc
    Busy_Header *heap = get_heap_base();
    assert_addr_equal(p,heap);
    assert_addr_not_equal(heap,freelist1);
    free(p);
    Busy_Header *heap1 = get_heap_base();
    Free_Header *freelist2 = get_heap_freelist(); // free list after free,should back to status before malloc
    assert_addr_equal(freelist,freelist2);
    assert_addr_equal(heap1,freelist2);
}

void bin_split_malloc_test(){
    const int N = 1000;
    void *p = malloc(N); // get chunk from free list
    assert_addr_not_equal(p, NULL);
    Free_Header *freelist = get_bin_freelist(N);
	assert_addr_equal(freelist, NULL); // chunk not in bin N yet
    free(p); // free chunk and add to bin N
    Free_Header *freelist1 = get_bin_freelist(N);
	assert_addr_not_equal(freelist1, NULL); // now we have a chunk
	assert_addr_equal(p, freelist1);

	const int N2 = 512;
    void *p1 = malloc(MAX_BIN_SIZE+10); // get chunk from free list (beyond max bin size)
    assert_addr_not_equal(p1,NULL);
    Free_Header *freelist4 = get_bin_freelist(N-N2);
    void *p2 = malloc(N2);   // should get chunk from bin[request2size(128)-request2size(99)] ???????
    assert_addr_not_equal(p2,NULL);
    Free_Header *freelist2 = get_bin_freelist(N);
    assert_addr_not_equal(freelist1,freelist2);
    assert_addr_equal(freelist2,NULL);
    Free_Header *freelist3 = get_bin_freelist(N-N2);
    assert_addr_not_equal(freelist3,NULL);
    assert_addr_not_equal(freelist3,freelist4);
    free(p1);
}

void free_NULL() {
    free(NULL); // don't crash
}

char buf[] = "hi"; // used by free_random
void free_random() {
    void *heap0 = get_heap_base();
    Free_Header *freelist0 = get_heap_freelist();

    free(buf); // try to free a valid but non-heap data address

    void *heap1 = get_heap_base();
    Free_Header *freelist1 = get_heap_freelist();

    assert_addr_equal(heap1, heap0);
    assert_addr_equal(freelist1, freelist0);
}

void freelist_free_stale() {
    malloc_large_size_then_free();
    Free_Header *freelist = get_heap_freelist();
    free(freelist); // NOT ok; freeing something already free'd
    assert_addr_not_equal(freelist, freelist->next); // make sure we didn't create cycle by freeing twice
}

int main(int argc, char *argv[]) {
    cunit_setup = setup;
    cunit_teardown = teardown;

    test(malloc0);
    test(free_NULL);
    test(malloc_word_size);
    test(one_malloc);
    test(two_malloc);
    test(bin_malloc_free_malloc);
    test(bin_split_malloc_test);
    test(malloc_large_size_then_free);
    test(free_random);
    test(freelist_free_stale);
}

