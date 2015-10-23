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

static Free_Header *freelist;   // Pointer to the first free chunk in heap
static void *heap;		        // point to data obtained from OS
static int heap_size = -1;

static Free_Header *nextfree(uint32_t size);

void heap_init(size_t max_heap_size) {
/*#ifdef DEBUG
    printf("allocate heap size == %d\n", max_heap_size);
    printf("sizeof(Busy_Header) == %zu\n", sizeof(Busy_Header));
    printf("sizeof(Free_Header) == %zu\n", sizeof(Free_Header));
    printf("BUSY_BIT == %x\n", BUSY_BIT);
    printf("SIZEMASK == %x\n", SIZEMASK);
#endif*/
	if ( heap!=NULL ) heap_shutdown();
    heap = morecore(max_heap_size);
	heap_size = (int)max_heap_size;
/*
#ifdef DEBUG
    if ( heap==NULL ) {
        fprintf(stderr, "Cannot allocate %zu bytes of memory for heap\n",
                max_heap_size + sizeof(Busy_Header));
    }
    else {
        fprintf(stderr, "morecore returns %p\n", heap);
    }
#endif
*/
    freelist = heap;
    freelist->size = (uint32_t)max_heap_size & SIZEMASK; // mask off upper bit to say free
    freelist->next = NULL;
    freelist->prev = NULL;
}

void heap_shutdown() {
    dropcore(heap, (size_t)heap_size);
}

void *malloc(size_t size) {
	if ( heap==NULL ) { heap_init(DEFAULT_MAX_HEAP_SIZE); }
    if (freelist == NULL) {
        return NULL; // out of heap
    }
    uint32_t n = (uint32_t) size & SIZEMASK;
    n = (uint32_t) align_to_word_boundary(size_with_header(n));
    Free_Header *chunk = nextfree(n);
    if (chunk == NULL) {
#ifdef DEBUG
        printf("out of heap");
#endif
        return NULL;
    }
    Busy_Header *b = (Busy_Header *) chunk;
    b->size |= BUSY_BIT; // get busy! turn on busy bit at top of size field
    return b;
}

/* Free chunk p, merge with following chunk if free, and add to the head of free list */
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

    q->size &= SIZEMASK; // turn off busy bit

    //check if the following chunk is free
    Free_Header *f = find_next(q);
    if ( !(f->size & BUSY_BIT) ) {   //if next chunk is free, merge
        q->prev = NULL;  //q will be the head
        merge_on_free(q, f);
    }else{                //if next chunk is busy, simply add q to the head
        q->next = freelist;
        q->prev = NULL;
        freelist->prev = q;
/*#ifdef DEBUG
        check_infinite_loop(q, "no merge");
#endif*/
    }
    freelist = q; //set to head
}

void merge_on_free(Free_Header *q, Free_Header *f){
    q->size += f->size;
    //f is both head and tail of the freelist
    if(f->prev == NULL && f->next == NULL) {
        q->next  = NULL;
/*#ifdef DEBUG
        check_infinite_loop(q, "merge with single node");
#endif*/
    }
    //f is the head but not the tail of the freelist
    else if (f->prev == NULL && f->next != NULL) {
        q->next = f->next;
        f->next->prev = q;
/*#ifdef DEBUG
        check_infinite_loop(q, "merge with head);
#endif*/
    }
    //f is the tail but not the head of the freelist
    else if (f->prev != NULL && f->next == NULL) {
        q->next = freelist;
        freelist->prev = q;
        f->prev->next = NULL;
/*#ifdef DEBUG
        check_infinite_loop(q, "merge with tail");
#endif*/
    }
    //f is neither head nor tail of the freelist
    else {
        q->next = freelist;
        freelist->prev = q;
        f->prev->next = f->next;
        f->next->prev = f->prev;
/*#ifdef DEBUG
        check_infinite_loop(q, "merge with a node in the middle");
#endif*/
    }
}

//find the following chunk, regardless of busy or free
void* find_next(void* p){
    Busy_Header *b = (Busy_Header *) p;
    uint32_t size = b->size & SIZEMASK;
    Busy_Header *f = (Busy_Header *) ((char *)b + size);
    return f;
}

//This is for debug purpose
void check_infinite_loop(Free_Header *f, char *msg ){
    while(f != NULL){
        printf("%s ", msg);  //print debug msg as printing the linked list
        printf("%p->", f);
        f = f->next;
    }

}

/** Find first free chunk that fits size else NULL if no chunk big enough.
 *  Split a bigger chunk if no exact fit, setting size of split chunk returned.
 *  size arg must be big enough to hold Free_Header and padded to 4 or 8
 *  bytes depending on word size.
 */
static Free_Header *nextfree(uint32_t size) {
    Free_Header *p = freelist;
    /* Scan until one of:
        1. end of free list
        2. exact size match between chunk and size
        3. chunk size big enough to split; there is space for size +
           another Free_Header (MIN_CHUNK_SIZE) for the new free chunk.
     */
    //search along the list for free chunks big enough
    //merge if consecutive free chunks are found
    while (p != NULL && size != p->size && p->size < size + MIN_CHUNK_SIZE) { //not fit
        if (!(((Free_Header *)find_next(p))->size & BUSY_BIT)){
            // merge_on_malloc
            Free_Header *f = (Free_Header *) find_next(p);
            merge_on_malloc(p, f);
            if (f == freelist)  //move freelist forward if f is the head
                freelist = f->next;

            if (size == p->size && p->size >= size + MIN_CHUNK_SIZE)  //re-check size after merge
                break;
        }
        p = p->next;
    }

    if (p == NULL) return p;    // no chunk big enough

    Free_Header *nextchunk;
    if (p->size  == size) {      // if exact fit
        nextchunk = p->next;
        if (nextchunk != NULL) {
            nextchunk->prev = p->prev;
            if (p == freelist) {
                freelist = nextchunk;
            } else {
                p->prev->next = nextchunk;
            }
        }else{
            freelist = NULL; //out of heap
        }
    }
    else {                      // split p into p', q
        Free_Header *q = (Free_Header *) (((char *) p) + size);
        q->size &= SIZEMASK; // turn off busy bit of the remainder
        q->size = p->size - size; // q is remainder of memory after allocating p'
        q->prev = p->prev;

        nextchunk = q;
        if (p->prev == NULL && p->next == NULL) {
            nextchunk->next = NULL;
            freelist = nextchunk;
        } else if (p->prev == NULL && p->next != NULL) {
            nextchunk->next = p->next;
            p->next->prev = nextchunk;
            freelist = nextchunk;
        } else if (p->prev != NULL && p->next == NULL) {
            nextchunk->next = NULL;
            p->prev->next = nextchunk;
        } else {
            nextchunk->next = p->next;
            p->prev->next = nextchunk;
            p->next->prev = nextchunk;
        }
    }
    p->size = size;
    return p;
}

// different from merge_on_free because p and f's positions need to be maintained
void merge_on_malloc(Free_Header *p, Free_Header *f){
    p->size += f->size;
    // p and f are not next to each other in the free list
    if(p->prev != f && p->next != f) {
        if (f->prev != NULL)
            f->prev->next = f->next;
        if (f->next != NULL)
            f->next->prev = f->prev;
/*#ifdef DEBUG
        check_infinite_loop(p, "merge with non-neighbor");
#endif*/
    }
        // p->next = f
    else if (p->next == f) {
        p->next = f->next;
        if (f->next != NULL)
            f->next->prev = p;
/*#ifdef DEBUG
        check_infinite_loop(p, "merge with next");
#endif*/
    }
        // p->prev = f
    else if (p->prev == f) {
        p->prev = f->prev;
        if (f->prev != NULL)
            f->prev->next = p;
/*#ifdef DEBUG
        check_infinite_loop(p, "merge with prev");
#endif*/
    }
}

Free_Header *get_freelist() { return freelist; }
void *get_heap_base()		{ return heap; }

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
