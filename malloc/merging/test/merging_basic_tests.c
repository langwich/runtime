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
#include "merging.h"
#include "cunit.h"

const size_t HEAP_SIZE = 2000;

extern void freelist_init(uint32_t max_heap_size);
extern void freelist_shutdown();

Heap_Info verify_heap() {
	Heap_Info info = get_heap_info();
	assert_equal(info.heap_size, HEAP_SIZE);
	assert_equal(info.heap_size, info.busy_size+info.free_size);
	return info;
}

static void setup()		{ freelist_init(HEAP_SIZE); }
static void teardown()	{ verify_heap(); freelist_shutdown(); }

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
	assert_equal(info.free, 1); // 1 free chunk after merging
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

//three consecutive chunks allocated
void  three_malloc(){
    void *p = get_heap_base();
    Busy_Header *b_1 = malloc(100);
    assert_addr_equal(b_1, p);   //b_1 is at the start of heap
    assert_equal(chunksize(b_1), request2size(100));
    Busy_Header *b_2 = malloc(200);
    Busy_Header *b1_next = find_next(b_1);
    assert_addr_equal(b1_next,b_2);     //b_2 is after b_1
    assert_equal(chunksize(b_2), request2size(200));
    Busy_Header *b_3 = malloc(300);
    Busy_Header *b2_next = find_next(b_2);
    assert_addr_equal(find_next(b_2),b_3);     //b_3 is after b_2
    assert_equal(chunksize(b_3), request2size(300));

    Heap_Info info = verify_heap();
    assert_equal(info.busy, 3);
    assert_equal(info.free, 1);
    assert_equal(info.free_size, HEAP_SIZE - info.busy_size);
}

//free the chunk in the middle of three busy chunks
void free_without_merging() {
	three_malloc();
    Busy_Header *b_1 = get_heap_base();
    Busy_Header *b_2 = find_next(b_1);
    Busy_Header *b_3 = find_next(b_2);

	Free_Header *freelist0 = get_freelist(); //get the head of freelist, which is at the end of allocated chunks
	assert_addr_equal(freelist0, find_next(b_3));
	free(b_2);   //free the chunk in the middle, which becomes the head of the new freelist
	Free_Header *freelist1 = get_freelist();  //get the new freelist

	assert_addr_not_equal(freelist1, b_1);
	assert_addr_not_equal(freelist0, freelist1);
    assert_addr_equal(freelist1, b_2);
    assert_addr_equal(freelist1->next, freelist0); //two free chunks in the list
    assert_equal(chunksize(b_1) + chunksize(b_2) + chunksize(b_3) + chunksize(freelist0), HEAP_SIZE);
    assert_equal(chunksize(b_1) + chunksize(b_3) + chunksize(freelist1) + chunksize(freelist0), HEAP_SIZE);

	Heap_Info info = verify_heap();
	assert_equal(info.busy, 2);
	assert_equal(info.busy_size, chunksize(b_1) + chunksize(b_3));
	assert_equal(info.free, 2); // 2 free chunks not next to each other
	assert_equal(info.free_size, chunksize(freelist1) + chunksize(freelist1->next));
}

//free the middle chunk, than the first chunk, leading to a merge with the head of the list
void merge_with_head() {
	free_without_merging(); //free middle chunk first

	void *p = get_heap_base(); // the first allocated chunk on the heap
	Free_Header *freelist0 = get_freelist(); //get the head of freelist, which is the middle chunk
    assert_addr_not_equal(freelist0, p);
    Free_Header *next = freelist0->next;
    free(p);   //free the chunk before head of the free list, need to merge
	Free_Header *freelist1 = get_freelist();  //get the new freelist

	assert_addr_equal(freelist1, p); // new head is at the base of heap
	assert_addr_not_equal(freelist1, freelist0);
	assert_addr_not_equal(freelist1->next, freelist0);
    assert_addr_equal(freelist1->next, next);
    assert_addr_equal(freelist1->prev, NULL);
    assert_addr_equal(next->next, NULL);
    assert_addr_equal(next->prev, freelist1);

	Heap_Info info = verify_heap();
	assert_equal(info.busy, 1);
	assert_equal(info.free, 2);
	assert_equal(info.free_size, HEAP_SIZE - info.busy_size);
	assert_equal(freelist1->next->size, info.free_size-freelist1->size);
    assert_equal(find_next(find_next(freelist1)), freelist1->next);
}

//five consecutive chunks allocated
void  five_malloc() {
    void *p = get_heap_base();
    Busy_Header *b_1 = malloc(100);
    assert_addr_equal(b_1, p);   //b_1 is at the start of heap
    assert_equal(chunksize(b_1), request2size(100));
    Busy_Header *b_2 = malloc(200);
    Busy_Header *b1_next = find_next(b_1);
    assert_addr_equal(b1_next,b_2);     //b_2 is after b_1
    assert_equal(chunksize(b_2), request2size(200));
    Busy_Header *b_3 = malloc(300);
    Busy_Header *b2_next = find_next(b_2);
    assert_addr_equal(find_next(b_2),b_3);     //b_3 is after b_2
    assert_equal(chunksize(b_3), request2size(300));
    Busy_Header *b_4 = malloc(400);
    Busy_Header *b3_next = find_next(b_3);
    assert_addr_equal(find_next(b_3),b_4);     //b_4 is after b_3
    assert_equal(chunksize(b_4), request2size(400));
    Busy_Header *b_5 = malloc(500);
    Busy_Header *b4_next = find_next(b_4);
    assert_addr_equal(find_next(b_4),b_5);     //b_5 is after b_4
    assert_equal(chunksize(b_5), request2size(500));

    Heap_Info info = verify_heap();
    assert_equal(info.busy, 5);
    assert_equal(info.free, 1);
    assert_equal(info.free_size, HEAP_SIZE - info.busy_size);
}

//free the first chunk, then free last chunk to merge with the end of free list
void merge_with_end() {
    three_malloc();
    Busy_Header *b_1 = get_heap_base();
    Busy_Header *b_2 = find_next(b_1);
    Busy_Header *b_3 = find_next(b_2);

    free(b_1);   //free the first chunk
    Free_Header *freelist0 = get_freelist();  //get the new freelist
    assert_addr_equal(freelist0, b_1);
    assert_addr_equal(freelist0->next, find_next(b_3));

    free(b_3);  //free the last chunk, merge
    Free_Header *freelist1 = get_freelist();
    assert_addr_equal(freelist1, b_3);
    assert_addr_equal(freelist1->next, b_1);
    assert_addr_equal(freelist1->prev, NULL);
    assert_addr_equal(freelist1->next->prev, freelist1);
    assert_addr_equal(freelist1->next->next, NULL);

    Heap_Info info = verify_heap();
    assert_equal(info.busy, 1);
    assert_equal(info.free, 2);
    assert_equal(info.free_size, HEAP_SIZE - info.busy_size);
    assert_equal(freelist1->next->size, b_1->size);
    assert_equal(find_next(find_next(b_1)), freelist1);
}

//free the first chunk, then free last chunk to merge with the end of free list
void merge_with_middle() {
    five_malloc();
    Busy_Header *b_1 = get_heap_base();
    Busy_Header *b_2 = find_next(b_1);
    Busy_Header *b_3 = find_next(b_2);
    Busy_Header *b_4 = find_next(b_3);
    Busy_Header *b_5 = find_next(b_4);

    size_t size_3 = b_3->size & SIZEMASK;
    size_t size_4 = b_4->size & SIZEMASK;

    free(b_4);
    free(b_2);
    Free_Header *freelist0 = get_freelist();  //get the new freelist at b_2->b_4->after b_5
    Free_Header *last = find_next(b_5);
    assert_addr_equal(freelist0, b_2);
    assert_addr_equal(freelist0->next, b_4);
    assert_addr_equal(freelist0->prev, NULL);
    assert_addr_equal(((Free_Header*)b_4) ->next, last);
    assert_addr_equal(((Free_Header*)b_4) ->prev, b_2);
    assert_addr_equal(last->next, NULL);
    assert_addr_equal(last->prev, b_4);

    free(b_3); //merge with b_4
    Free_Header *freelist1 = get_freelist(); //get new free list at b_3;
    assert_addr_equal(freelist1, b_3);
    assert_equal(freelist1->size & SIZEMASK, size_3 + size_4);

    assert_addr_equal(freelist1->next, b_2); //freelist1 ->freelist0->last
    assert_addr_equal(freelist1->prev, NULL);
    assert_addr_equal(freelist0->next, last);
    assert_addr_equal(freelist0->prev, freelist1);
    assert_addr_equal(last->next, NULL);
    assert_addr_equal(last->prev, freelist0);

    Heap_Info info = verify_heap();
    assert_equal(info.busy, 2);
    assert_equal(info.free, 3);
    assert_equal(info.free_size, HEAP_SIZE - info.busy_size);
    assert_equal(freelist1->next->size + freelist0->next->size, info.free_size-freelist1->size);
}
//allocate 10 consecutive chunks, free from the back to the front
//should fuse the heap to one free chunk
void fuse_to_one() {
    Busy_Header* b[10];
    for(int i = 0; i < 10; i++){
        b[i] = malloc(100);
        assert_addr_equal(get_freelist(), find_next(b[i]));
        assert_addr_equal(get_freelist()->prev, NULL);
    }
    Free_Header *freelist0 = get_freelist();
    for( int i = 9; i >= 0 ; i--){
        free(b[i]);
        assert_addr_equal(get_freelist(), b[i]);
        assert_addr_equal(get_freelist()->prev, NULL);
    }
    Heap_Info info = verify_heap();
    assert_equal(info.busy, 0);
    assert_equal(info.free, 1);
    assert_equal(info.busy_size + info.free_size, get_heap_info().heap_size);
    assert_addr_equal(find_next(get_freelist()), get_heap_base() + info.heap_size);
    assert_addr_equal(freelist0->next, NULL);
}

void long_freelist() {
    Busy_Header* b[20];
    for(int i = 0; i < 20; i++){
        b[i] = malloc(i);
        assert_addr_equal(get_freelist(), find_next(b[i]));
        assert_addr_equal(get_freelist()->prev, NULL);
    }


    Free_Header *freelist0 = get_freelist();
    for( int i = 0; i <20 ; i++){
        free(b[i]);
        assert_addr_equal(get_freelist(), b[i]);
        if (i+1 < 20)
            assert_addr_equal(find_next(get_freelist()), b[i+1]);
        assert_addr_equal(get_freelist()->prev, NULL);

    }
    //last free causes a merge
    Heap_Info info = verify_heap();
    assert_equal(info.busy, 0);
    assert_equal(info.free, 20);
    assert_equal(info.busy_size + info.free_size, get_heap_info().heap_size); // out of heap
    assert_addr_equal(get_freelist(), b[19]);
    assert_addr_equal(find_next(get_freelist()), get_heap_base() + info.heap_size); //out of heap
    assert_addr_equal(((Free_Header *)b[0])->next, NULL); //end

}

//skip first free chunk, which is not big enough
void search_along_list() {
    printf("after allocation:\n");
    Busy_Header* b[4];
    for(int i = 0; i < 4; i++){
        b[i] = malloc(450);
        printf("b[%d]: %p\n", i, b[i]);
    }
    assert_equal(b[0]->size & SIZEMASK, request2size(450));
    assert_equal(b[1]->size & SIZEMASK, request2size(450));
    assert_equal(b[2]->size & SIZEMASK, request2size(450));
    assert_equal(b[3]->size & SIZEMASK, request2size(450));
    Free_Header* f4 = find_next(b[3]);
    printf("head: %p\n", f4);

    Heap_Info info = verify_heap();
    assert_equal(info.busy, 4);
    assert_equal(info.free, 1);
    assert_equal(info.busy_size, 1824);
    assert_equal(info.free_size, 176);
    assert_addr_equal(get_freelist(), f4);
    assert_addr_equal(get_freelist()->next, NULL);
    assert_addr_equal(get_freelist()->prev, NULL);

    //after malloc, free b3, b0
    free(b[3]);
    free(b[0]);
    Busy_Header *skip = malloc(600);

    printf("after malloc(600)\n");
    Free_Header *head = get_freelist();
    print_both_ways(head);

    info = verify_heap();
    assert_equal(info.busy, 3);
    assert_equal(info.free, 2);
    assert_equal(info.busy_size + info.free_size, get_heap_info().heap_size);
}

//fit in the first free chunk of the list
void exact_fit() {
    printf("after allocation:\n");
    Busy_Header* b[4];
    for(int i = 0; i < 4; i++){
        b[i] = malloc(450);
        printf("b[%d]: %p\n", i, b[i]);
    }
    assert_equal(b[0]->size & SIZEMASK, request2size(450));
    assert_equal(b[1]->size & SIZEMASK, request2size(450));
    assert_equal(b[2]->size & SIZEMASK, request2size(450));
    assert_equal(b[3]->size & SIZEMASK, request2size(450));
    Free_Header* f4 = find_next(b[3]);
    printf("head: %p\n", f4);

    Heap_Info info = verify_heap();
    assert_equal(info.busy, 4);
    assert_equal(info.free, 1);
    assert_equal(info.busy_size, 1824);
    assert_equal(info.free_size, 176);
    assert_addr_equal(get_freelist(), f4);
    assert_addr_equal(get_freelist()->next, NULL);
    assert_addr_equal(get_freelist()->prev, NULL);

    free(b[2]);

    info = verify_heap();
    assert_equal(info.busy, 3);
    assert_equal(info.free, 2);
    assert_equal(info.busy_size, 1368);
    assert_equal(info.free_size, 632);
    assert_addr_equal(get_freelist(), b[2]);
    assert_addr_equal(get_freelist()->next, f4);
    assert_addr_equal(get_freelist()->next->next, NULL);
    assert_addr_equal(f4->prev, b[2]);
    assert_addr_equal(f4->prev->prev, NULL);
    printf("after free b[2]\n");
    Free_Header *head = get_freelist();
    print_both_ways(head);


    free(b[0]);
    info = verify_heap();
    assert_equal(info.busy, 2);
    assert_equal(info.free, 3);
    assert_equal(info.busy_size, 912);
    assert_equal(info.free_size, 1088);
    assert_addr_equal(get_freelist(), b[0]);
    assert_addr_equal(get_freelist()->next, b[2]);
    assert_addr_equal(get_freelist()->next->next, f4);
    assert_addr_equal(get_freelist()->next->next->next, NULL);
    assert_addr_equal(f4->prev, b[2]);
    assert_addr_equal(f4->prev->prev, b[0]);
    assert_addr_equal(f4->prev->prev->prev, NULL);

    printf("after free b[0]\n");
    head = get_freelist();
    print_both_ways(head);

    Busy_Header *fit = malloc(450);
    printf("after malloc(450)\n");
    head = get_freelist();
    print_both_ways(head);
    assert_addr_equal(fit, get_heap_base());

    info = verify_heap();
    assert_equal(info.busy, 3);
    assert_equal(info.free, 2);
    assert_addr_equal(get_freelist(), b[2]);
    assert_addr_equal(get_freelist()->next, f4);
    assert_addr_equal(get_freelist()->next->next, NULL);
    assert_addr_equal(f4->prev, b[2]);
    assert_addr_equal(f4->prev->prev, NULL);
}

//split chunk
void split_chunk(){
    printf("after allocation:\n");
    Busy_Header* b[4];
    for(int i = 0; i < 4; i++){
        b[i] = malloc(450);
        printf("b[%d]: %p\n", i, b[i]);
    }
    assert_equal(b[0]->size & SIZEMASK, request2size(450));
    assert_equal(b[1]->size & SIZEMASK, request2size(450));
    assert_equal(b[2]->size & SIZEMASK, request2size(450));
    assert_equal(b[3]->size & SIZEMASK, request2size(450));
    Free_Header* f4 = find_next(b[3]);
    printf("head: %p\n", f4);

    Heap_Info info = verify_heap();
    assert_equal(info.busy, 4);
    assert_equal(info.free, 1);
    assert_equal(info.busy_size, 1824);
    assert_equal(info.free_size, 176);
    assert_addr_equal(get_freelist(), f4);
    assert_addr_equal(get_freelist()->next, NULL);
    assert_addr_equal(get_freelist()->prev, NULL);

    free(b[2]);

    info = verify_heap();
    assert_equal(info.busy, 3);
    assert_equal(info.free, 2);
    assert_equal(info.busy_size, 1368);
    assert_equal(info.free_size, 632);
    assert_addr_equal(get_freelist(), b[2]);
    assert_addr_equal(get_freelist()->next, f4);
    assert_addr_equal(get_freelist()->next->next, NULL);
    assert_addr_equal(f4->prev, b[2]);
    assert_addr_equal(f4->prev->prev, NULL);
    printf("after free b[2]\n");
    Free_Header *head = get_freelist();
    print_both_ways(head);


    free(b[0]);
    info = verify_heap();
    assert_equal(info.busy, 2);
    assert_equal(info.free, 3);
    assert_equal(info.busy_size, 912);
    assert_equal(info.free_size, 1088);
    assert_addr_equal(get_freelist(), b[0]);
    assert_addr_equal(get_freelist()->next, b[2]);
    assert_addr_equal(get_freelist()->next->next, f4);
    assert_addr_equal(get_freelist()->next->next->next, NULL);
    assert_addr_equal(f4->prev, b[2]);
    assert_addr_equal(f4->prev->prev, b[0]);
    assert_addr_equal(f4->prev->prev->prev, NULL);

    printf("after free b[0]\n");
    head = get_freelist();
    print_both_ways(head);

    Busy_Header *split = malloc(200);

    printf("after malloc(200)\n");
    head = get_freelist();
    print_both_ways(head);

    assert_addr_equal(split, get_heap_base());
    Free_Header *newhead = get_freelist();
    assert_addr_equal(find_next(split),newhead);
    assert_addr_equal(get_freelist(), find_next(split));
    assert_addr_equal(get_freelist()->next, b[2]);
    assert_addr_equal(get_freelist()->next->next, f4);
    assert_addr_equal(get_freelist()->next->next->next, NULL);
    assert_addr_equal(f4->prev, b[2]);
    assert_addr_equal(f4->prev->prev, newhead);
    assert_addr_equal(f4->prev->prev->prev, NULL);
    info = verify_heap();
    assert_equal(info.busy, 3);
    assert_equal(info.free, 3);
    assert_equal(info.busy_size + info.free_size, get_heap_info().heap_size); // out of heap*/
}

void print_both_ways(Free_Header *f){
    Free_Header *head = f;
    while (f->next != NULL){
        printf("%p->", f);
        f = f->next;
    }
    printf("%p\n", f);
    while (f->prev != NULL){
        printf("%p<-", f);
        f = f->prev;
    }
    printf("%p\n", f);
    assert_addr_equal(f, head);
}

void test_core() {
	void *heap = morecore(HEAP_SIZE);
	assert_addr_not_equal(heap, NULL);
	dropcore(heap, HEAP_SIZE);
}

void test_init_shutdown() {
	freelist_init(HEAP_SIZE);
	assert_addr_equal(get_freelist(), get_heap_base());
	freelist_shutdown();
}

int main(int argc, char *argv[]) {

	cunit_setup = setup;
	cunit_teardown = teardown;

	test_core();
	test_init_shutdown();

	freelist_init(HEAP_SIZE);

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
    test(three_malloc);
    test(five_malloc);
	test(free_without_merging);
	test(merge_with_head);
    test(merge_with_end);
    test(merge_with_middle);
    test(fuse_to_one);
    test(long_freelist);
    test(search_along_list);
    test(exact_fit);
    test(split_chunk);
}