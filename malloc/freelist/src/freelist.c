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

//#define DEBUG

typedef struct {
	Free_Header *freelist; // Pointer to the first free chunk in heap
	void *base;			   // point to data obtained from OS
} Free_List_Heap;

static Free_List_Heap heap;

static int heap_size;

static Free_Header *nextfree(uint32_t size);

void freelist_init(uint32_t max_heap_size) {
#ifdef DEBUG
	printf("allocate heap size == %d\n", max_heap_size);
	printf("sizeof(Busy_Header) == %zu\n", sizeof(Busy_Header));
	printf("sizeof(Free_Header) == %zu\n", sizeof(Free_Header));
	printf("BUSY_BIT == %x\n", BUSY_BIT);
	printf("SIZEMASK == %x\n", SIZEMASK);
#endif
	heap_size = max_heap_size;
	heap.base = morecore(max_heap_size);
#ifdef DEBUG
	if ( heap==NULL ) {
		fprintf(stderr, "Cannot allocate %zu bytes of memory for heap\n",
				max_heap_size + sizeof(Busy_Header));
	}
	else {
		fprintf(stderr, "morecore returns %p\n", heap);
	}
#endif
	heap.freelist = heap.base;
	heap.freelist->size = max_heap_size & SIZEMASK; // mask off upper bit to say free
	heap.freelist->next = NULL;
}

void freelist_shutdown() {
	dropcore(heap.base, ((Free_Header *)heap.base)->size);
}

void *malloc(size_t size) {
	// TODO: 	if ( heap==NULL ) freelist_init(...)
	if (heap.freelist == NULL) {
		return NULL; // out of heap
	}
	uint32_t n = (uint32_t) size & SIZEMASK;
	n = (uint32_t) align_to_word_boundary(size_with_header(n));
	Free_Header *chunk = nextfree(n);
	Busy_Header *b = (Busy_Header *) chunk;
	b->size |= BUSY_BIT; // get busy! turn on busy bit at top of size field
	return b;
}

/* Free chunk p by adding to head of free list */
void free(void *p) {
	if (p == NULL) return;
	void *start_of_heap = get_heap_base();
	void *end_of_heap = start_of_heap + heap_size - 1; // last valid address of heap
	if ( p<start_of_heap || p>end_of_heap ) {
#ifdef DEBUG
		fprintf(stderr, "free of non-heap address %p\n", p);
#endif
		return;
	}
	Free_Header *q = (Free_Header *) p;
	if ( !(q->size & BUSY_BIT) ) { // stale pointer? If already free'd better not try to free it again
#ifdef DEBUG
		fprintf(stderr, "free of stale pointer %p\n", p);
#endif
		return;
	}
	q->next = heap.freelist;
	q->size &= SIZEMASK; // turn off busy bit
	heap.freelist = q;
}

/** Find first free chunk that fits size else NULL if no chunk big enough.
 *  Split a bigger chunk if no exact fit, setting size of split chunk returned.
 *  size arg must be big enough to hold Free_Header and padded to 4 or 8
 *  bytes depending on word size.
 */
static Free_Header *nextfree(uint32_t size) {
	Free_Header *p = heap.freelist;
	Free_Header *prev = NULL;
	/* Scan until one of:
	    1. end of free list
	    2. exact size match between chunk and size
	    3. chunk size big enough to split; there is space for size +
	       another Free_Header (MIN_CHUNK_SIZE) for the new free chunk.
	 */
	while (p != NULL && size != p->size && p->size < size + MIN_CHUNK_SIZE) {
		prev = p;
		p = p->next;
	}
	if (p == NULL) return p;    // no chunk big enough

	Free_Header *nextchunk;
	if (p->size == size) {      // if exact fit
		nextchunk = p->next;
	}
	else {                      // split p into p', q
		Free_Header *q = (Free_Header *) (((char *) p) + size);
		q->size = p->size - size; // q is remainder of memory after allocating p'
		q->next = p->next;
		nextchunk = q;
	}

	p->size = size;

	// add nextchunk to free list
	if (p == heap.freelist) {       // head of free list is our chunk
		heap.freelist = nextchunk;
	}
	else {
		prev->next = nextchunk;
	}

	return p;
}

Free_Header *get_freelist() { return heap.freelist; }
void *get_heap_base()		{ return heap.base; }

/* Walk heap jumping by size field of chunk header. Return an info record. */
Heap_Info get_heap_info() {
	void *heap = get_heap_base();			  // should be allocated chunk
	void *end_of_heap = heap + heap_size - 1; // last valid address of heap
	Busy_Header *p = heap;
	uint32_t busy = 0;
	uint32_t free = 0;
	uint32_t busy_size = 0;
	uint32_t free_size = 0;
	while ( p>=heap && p<=end_of_heap ) { // stay inbounds, walking heap
		// track
		if ( p->size & BUSY_BIT ) {
			busy++;
			busy_size += chunksize(p);
		}
		else {
			free++;
			free_size += chunksize(p);
		}
		p = (Busy_Header *)((char *) p + chunksize(p));
	}
	return (Heap_Info){heap_size, busy, busy_size, free, free_size};
}
