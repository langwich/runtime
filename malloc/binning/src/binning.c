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

#include "binning.h"

static void * heap;

/*except bins, we have another freelist to handle the malloc and free request which size over 1024*/
Free_Header *freelist;

/* 1204 bins, each bin has a freelist with fix-sized(size = index of bin) free chunks*/
Free_Header * bin[BIN_SIZE];

static Free_Header *freelist_malloc(uint32_t size);
static Free_Header *next_small_free(uint32_t size);

Free_Header *bin_split_malloc(uint32_t size);

void heap_init() {
//#ifdef DEBUG
//	printf("allocate heap size == %d\n", DEFAULT_MAX_HEAP_SIZE);
//	printf("sizeof(Busy_Header) == %zu\n", sizeof(Busy_Header));
//	printf("sizeof(Free_Header) == %zu\n", sizeof(Free_Header));
//	printf("BUSY_BIT == %x\n", BUSY_BIT);
//	printf("SIZEMASK == %x\n", SIZEMASK);
//#endif
	heap = morecore(DEFAULT_MAX_HEAP_SIZE);
#ifdef DEBUG
	if ( heap == NULL ) {
		fprintf(stderr, "Cannot allocate %d bytes of memory for heap\n",DEFAULT_MAX_HEAP_SIZE);
	}
	else {
		fprintf(stderr, "morecore returns %p\n", heap);
	}
#endif
	freelist = (Free_Header *)heap;
	freelist->size = DEFAULT_MAX_HEAP_SIZE & SIZEMASK; // mask off upper bit to say free
	freelist->next = NULL;
}

/*
* each malloc request first check size, if  size >1024 ,get from free list and return
* else check if have fitted size in bin[size-1],if yes, return
* if not, check if can get from free list,if yes, return (may be need split)
* if not, check if there exist next free chunk in bin[biggersize],if yes, split and return
* if not, return NULL, out of heap error
*/
void *malloc(size_t size) {
	uint32_t n = (uint32_t) size & SIZEMASK;
	size_t actual_size =(uint32_t) align_to_word_boundary(size_with_header(n));
	Busy_Header *b;
	if (actual_size >BIN_SIZE) {
		Free_Header *q = freelist_malloc(actual_size);
		if (q == NULL) {
#ifdef DEBUG
			printf("out of heap");
#endif
			return NULL;
		}
		b = (Busy_Header *)q;
		b->size |= BUSY_BIT;
		return b;
	}
	else {
		Free_Header *q = next_small_free(actual_size);
		if (q == NULL) {
#ifdef DEBUG
			printf("out of heap");
#endif
			return NULL;
		}
		b = (Busy_Header *)q;
		b->size |= BUSY_BIT;
		return b;
	}
	return NULL;
}

/*
* for each free request
* if chunk size over 1024, free it and add to free list (sorted)
* other wise add to bin[chunk_size-1]
*/
void free(void *p) {
	if (p == NULL) return;
	void *start_of_heap = get_heap_base();
	void *end_of_heap = start_of_heap + DEFAULT_MAX_HEAP_SIZE - 1; // last valid address of heap
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
	q->size &= SIZEMASK;
	if (q->size<= BIN_SIZE){
		q->next = bin[q->size-1];
		bin[q->size-1] = q;
	}
	else {
		q->next = freelist;
		freelist = q;
	}
}

static Free_Header *next_small_free(uint32_t size){
	if (bin[size-1] != NULL) {
		Free_Header *chunk = bin[size-1];
		bin[size-1] = bin[size-1]->next;
		return chunk;
	}
	else if (freelist != NULL){
		Free_Header *chunk = freelist_malloc(size);
		if (chunk != NULL){
			return chunk;
		}
		else {
			return bin_split_malloc(size);
		}
	}
	else {
		return bin_split_malloc(size);
	}
}

static Free_Header *freelist_malloc(uint32_t size) {
	Free_Header *p = freelist;
	Free_Header *prev = NULL;
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
	if (p == freelist) {       // head of free list is our chunk
		freelist = nextchunk;
	}
	else {
		prev->next = nextchunk;
	}
	return p;
}

Free_Header *bin_split_malloc(uint32_t size) {
	size_t index = size;
	while (bin[index-1] == NULL && index < BIN_SIZE) {
		index ++;
	}
	if(index == BIN_SIZE) {
		return NULL;
	}
	Free_Header *p = bin[index-1];
	Free_Header *q = (Free_Header *) (((char *) p) + size);
	bin[index-1] = bin[index-1]->next;
	q->size = index - size;
	Free_Header *prev = bin[q->size-1];
	if (prev == NULL) {
		bin[q->size-1] = q;
	}
	else {
		q->next = prev;
		bin[q->size-1] = q;
	}
	return p;
}

void *get_heap_base() { return heap; }

Free_Header *get_heap_freelist() { return freelist;}

Free_Header *get_bin_freelist(uint32_t index) { return bin[index];}


void heap_shutdown() {
	dropcore(heap, DEFAULT_MAX_HEAP_SIZE);
}

Heap_Info get_heap_info() {
	void *heap = get_heap_base();
	void *end_of_heap = heap + DEFAULT_MAX_HEAP_SIZE - 1;
	Busy_Header *p = heap;
	uint32_t busy = 0;
	uint32_t free = 0;
	uint32_t busy_size = 0;
	uint32_t free_size = 0;
	while ( p >= heap && p <= end_of_heap ) {
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
	return (Heap_Info){DEFAULT_MAX_HEAP_SIZE, busy, busy_size, free, free_size};
}


