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
#include <string.h>

#include "scavenger.h"
#include <gc.h>
#include <morecore.h>

static void *gc_raw_alloc(size_t size);
static void update_root(heap_object *p, int i);
static void gc_scavenge();
static void forward_object(heap_object *p);
static void forward_ptr_fields(const heap_object *p);


static bool DEBUG = false;

static const int MAX_ROOTS = 100000;

/* Track every pointer into the heap; includes globals, args, and locals */
static heap_object **_roots[MAX_ROOTS];

/* index of next free space in _roots for a root */
static int num_roots = 0;

static size_t heap_size;
static void *heap_0;  // the heap where alloc happens
static void *end_of_heap_0;
static void *next_free;
static void *heap_1;  // the heap where live objects are copied to
static void *end_of_heap_1;
static void *next_free_forwarding; // next_free in heap_1


// --------------------------------- G C  I n i t  &  R o o t  M g m t ---------------------------------

void gc_debug(bool debug) { DEBUG = debug; }

/* Initialize two heaps with a certain size for use with the garbage collector */
void gc_init(int size) {
	if (heap_0 != NULL || heap_1 != NULL ) { gc_shutdown(); }
    heap_size = (size_t)size;
    heap_0 = morecore((size_t)size);	// init heap_0
    end_of_heap_0 = heap_0 + size - 1;
    next_free = heap_0;
    num_roots = 0;

	heap_size = (size_t)size;	//init heap_1
	heap_1 = morecore((size_t)size);
	end_of_heap_1 = heap_1 + size - 1;
	next_free_forwarding = heap_1;
}

/* Announce you are done with the heaps managed by the garbage collector */
void gc_shutdown() {
	dropcore(heap_0, heap_size);
	dropcore(heap_1, heap_size);
}

void gc_add_root(void **p)
{
	if ( num_roots<MAX_ROOTS ) {
		_roots[num_roots++] = (heap_object **) p;
	}
}

int gc_num_roots() {
	return num_roots;
}


int gc_count_roots(){
	int c = 0;
	for (int i = 0 ; i < MAX_ROOTS; i++){
		if (_roots[i] != NULL  && *_roots[i] != NULL)
			c++;
	}
	return c;
}
void gc_set_num_roots(int roots)
{
	num_roots = roots;
}

// --------------------------------- A l l o c a t i o n ---------------------------------

heap_object *gc_alloc(object_metadata *metadata, size_t size) {
	if (heap_0 == NULL ) {
		gc_init(DEFAULT_MAX_HEAP_SIZE);  // init heap_0 and heap_1
	}
	size = align_to_word_boundary(size);
	heap_object *p = gc_raw_alloc(size);

	if ( p==NULL ) return NULL;

	memset(p, 0, size);         // wipe out object's data space and the header
	p->metadata = metadata;     // make sure object knows where its metadata is
	p->size = (uint32_t)size;
	return p;
}

/** Allocate size bytes in the heap by bumping high-water mark; if full, gc() and try again.
 *  Size must include any header size and must be word-aligned.
 */
static void *gc_raw_alloc(size_t size) {
	if (next_free + size > end_of_heap_0) {
		gc(); // try to collect
		if (next_free + size > end_of_heap_0) { // try again
			return NULL;                      // oh well, no room. puke
		}
	}

	void *p = next_free; // bump-ptr-allocation
	next_free += size;
	return p;
}


// --------------------------------- F o r w a r d i n g ---------------------------------

bool ptr_is_in_heap_0(heap_object *p) {
	return  p >= (heap_object *) heap_0 &&
			p <= (heap_object *) end_of_heap_0;
	//p->magic == MAGIC_NUMBER;
}

bool ptr_is_in_heap_1(heap_object *p) {
	return  p >= (heap_object *) heap_1 &&
			p <= (heap_object *) end_of_heap_1;
	//p->magic == MAGIC_NUMBER;
}

//calculate the new position in heap_1
static inline void realloc_object(heap_object *p) {
	if (ptr_is_in_heap_1(p))   //already collected in heap_1
		return;
	void *q = next_free_forwarding; // bump-ptr-allocation
	next_free_forwarding += p->size;
	p->forwarded = q; // p->forwarded records the new position in heap_1
}

//copy the object to forwarding address
static inline void move_to_forwarding_addr(heap_object *p) {
	if ( ptr_is_in_heap_0(p) && ptr_is_in_heap_1(p->forwarded) ) {
		memcpy(p->forwarded, p, p->size);
	}
}


void gc() {
	if (DEBUG) printf("GC-SCAVENGE\n");
	gc_scavenge();

	//after scavenging, reset the heaps to be ready for next round of gc
	void *tmp_start = heap_1;  //swap heap_0 and heap_1 pointers
	heap_1 = heap_0;
	heap_0 = tmp_start;
	void *tmp_end = end_of_heap_1;  //swap heap_0 and heap_1 pointers
	end_of_heap_1 = end_of_heap_0;
	end_of_heap_0 = tmp_end;
	next_free = next_free_forwarding;  // next object to be allocated would occur here
	next_free_forwarding = heap_1;  //ready for next round of gc

	if (DEBUG) printf("DONE GC\n");
}

/* Alter the root to point at new location */
static void update_root(heap_object *p, int i) {
	if (DEBUG) {
		printf("UPDATE ROOT\n");
		if (p->forwarded != p) {
			printf("move root[%d]=%p -> %s@%p (0x%x bytes) to %p\n",
				   i,
				   *_roots[i],
				   p->metadata->name,
				   p,
				   p->size,
				   p->forwarded);
		}
	}
	*_roots[i] = p->forwarded;	// update root to point at new address
}

// ---------------------------------Scavenge and Forward Live Objects to Heap_1 ---------------------------------
void gc_scavenge() {
	if (DEBUG) printf("SCAVENGING...\n");
	if (DEBUG) printf("heap_0 : %p\nend of heap_0 : %p\nheap_1 : %p\nend of heap_1 : %p\n",
					  heap_0, end_of_heap_0, heap_1, end_of_heap_1);
    for (int i = 0; i < num_roots; i++) {
        heap_object *p = *_roots[i];
		if (DEBUG) printf("roots[%d] = %p, p->forwarded = %p\n", i, p, p->forwarded );

		if (p == NULL){
			if (DEBUG) printf("roots[%d] = NULL, pointer has been killed\n", i);
			continue;
		}

		else if (ptr_is_in_heap_1(p))  { // in heap_1 and root has been updated
			if (DEBUG) printf("roots[%d] = %p,  is already in heap_1, root has been updated\n", i, p);
			continue;
		}

		else if ( ptr_is_in_heap_0(p) ) {
			if(ptr_is_in_heap_1(p->forwarded)) {// in heap_1 but root has not been updated
				if (DEBUG) printf("roots[%d] = %p,  p->forwarded = %p, is already in heap_1, "
										  "but root has not been updated\n", i, p, p->forwarded);
				update_root(p, i);
				continue;
			}

			if (DEBUG) printf("root[%d]=%p -> %s@%p (0x%x bytes)\n", i, *_roots[i], p->metadata->name, p, p->size);
			if (DEBUG) printf("Start of %s@%p, end of %s@%p, size: (0x%x bytes)\n", p->metadata->name, p,p->metadata->name,((void *)p)+p->size, p->size);

			forward_object(p);	  //recursively forward all pointed field objects
			update_root(p, i);
		}
		else  {
			if (DEBUG) printf("root[%d]=%p -> %p INVALID\n", i, _roots[i], p);
		}
    }
}

/* recursively walk object graph starting from p. */
static void forward_object(heap_object *p) {
	if (DEBUG) printf("realloc_object @ %p\n", p);
	realloc_object(p);
	if (DEBUG) printf("move_object @ %p to %p\n", p, p->forwarded);
	move_to_forwarding_addr(p);
	if (DEBUG) printf("forward ptr fields @ %p\n", p->forwarded);
	forward_ptr_fields(p->forwarded); // update ptr fields of the relocated object and forward them
}

static void forward_ptr_fields(const heap_object *p) {// check for tracked heap ptrs in this object
	for (int i = 0; i < p->metadata->num_ptr_fields; i++) {
		int offset_of_ptr_field = p->metadata->field_offsets[i];
		void *ptr_to_ptr_field = ((void *)p) + offset_of_ptr_field;
		heap_object **ptr_to_obj_ptr_field = (heap_object **) ptr_to_ptr_field;
		heap_object *target_obj = *ptr_to_obj_ptr_field;
		if (target_obj != NULL ) {
			if (!ptr_is_in_heap_1(target_obj->forwarded))  //field object hasn't been forwarded
				forward_object(target_obj);
			*ptr_to_obj_ptr_field = target_obj->forwarded; //update ptr field
		}
	}
}

// --------------------------------- S u p p o r t ---------------------------------

Heap_Info get_heap_info() {
	void *p = heap_0;
	int busy = 0;
	int live = gc_num_live_objects();
	int computed_busy_size = 0;
	int busy_size = (uint32_t)(next_free - heap_0);
	int free_size = (uint32_t)(end_of_heap_0 - next_free + 1);
	while (p >= heap_0 && p < next_free ) { // stay inbounds, walking heap
		// track
		busy++;
		computed_busy_size += ((heap_object *)p)->size;
		p = p + ((heap_object *)p)->size;
	}
	return (Heap_Info){heap_0, end_of_heap_0, next_free, (uint32_t)heap_size,
	                   busy, live, computed_busy_size, 0, busy_size, free_size };
}

void foreach_object(void (*action)(heap_object *)) {
	void *p = heap_0;
	while (p >= heap_0 && p < next_free) { // for each object in the heap currently allocated
		size_t size = ((heap_object *)p)->size;
		action(p);
		p = p + size;
	}
}
//this has to be called after gc()
int gc_num_live_objects() {
	int n = 0;
	void *p = heap_0;
	while (p >= heap_0 && p < next_free) { // for each marked (live) object, record forwarding address
		//if(ptr_is_in_heap_1(((heap_object *)p)->forwarded))
		n++;
		p = p + ((heap_object *)p)->size;
	}
	return n;
}