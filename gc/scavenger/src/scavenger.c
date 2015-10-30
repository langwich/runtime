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
int gc_num_live_objects();
size_t compute_busy_size();



// --------------------------------- D A T A ---------------------------------

static bool DEBUG = false;

static const int MAX_ROOTS = 100000; // obviously this is ok only for the educational purpose of this code

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
static void *next_free_fowarding; // next_free in heap_1


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
	next_free_fowarding = heap_1;
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

void gc_set_num_roots(int roots)
{
	num_roots = roots;
}

// --------------------------------- A l l o c a t i o n ---------------------------------

/* Allocate an object per the indicated size, which must included heap_object / header info.
 * The object is zeroed out and the header is initialized.
 */
heap_object *gc_alloc(object_metadata *metadata, size_t size) {
	if (heap_0 == NULL ) { gc_init(DEFAULT_MAX_HEAP_SIZE); }
	size = align_to_word_boundary(size);
	heap_object *p = gc_raw_alloc(size);

	if ( p==NULL ) return NULL;

	memset(p, 0, size);         // wipe out object's data space and the header
//	p->magic = MAGIC_NUMBER;    // a safety measure; if not magic num, we didn't alloc
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


// --------------------------------- C o l l e c t i o n ---------------------------------

static inline void realloc_object(heap_object *p) {
	void *q = next_free_fowarding; // bump-ptr-allocation
	next_free_fowarding += p->size;
	p->forwarded = q; // p->forwarded records the new position in heap_1
	if (DEBUG) printf("end of heap_0: %p\n", end_of_heap_0);
	if (DEBUG) printf("end of heap_1: %p\n", end_of_heap_1);
	if (DEBUG) printf("end of heap_0: %p\n", end_of_heap_0);
	if (DEBUG) printf("end of heap_1: %p\n", end_of_heap_1);
	if (DEBUG) if ( p->forwarded!=p ) printf("forward %p to %s@%p (0x%x bytes)\n", p, p->metadata->name, p->forwarded, p->size);

}

static inline void move_to_forwarding_addr(heap_object *p) {
	if (DEBUG) if ( p->forwarded!=p ) printf("    move %p to %p (0x%x bytes)\n", p, p->forwarded, p->size);
	if ( p->forwarded!=p ) {
	//	printf("p = %p, p->forwarded = %p\n", p, p->forwarded );

		memcpy(p->forwarded, p, p->size);
	}
}

/*static inline void move_live_objects_to_forwarding_addr(heap_object *p) {
	if ( p->marked ) {
		if (DEBUG) printf("live %s@%p (0x%x bytes)\n", p->metadata->name, p, p->size);
		p->marked = false;              // reset *before* move or you're changing old object
		move_to_forwarding_addr(p);     // move objects to compact heap
	}
	else {
		if (DEBUG) printf("dead %s@%p (0x%x bytes)\n", p->metadata->name, p, p->size);
		p->magic = 0;
	}
}*/

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

//static inline void unmark_object(heap_object *p) { p->marked = false; }

/* Perform a mark_and_compact garbage collection, moving all live objects
 * to the start of the heap. Anything that we don't mark is dead.
 *
 * 1. Walk object graph starting from roots, marking live objects.
 *
 * 2. Walk all live objects and compute their forwarding addresses starting from start_of_heap.
 *
 * 3. Alter all non-NULL roots to point to the object's forwarding address.
 *
 * 4. For each live object:
 * 	    a) alter all non-NULL managed pointer fields to point to the forwarding addresses.
 * 		b) unmark object
 *
 * 5. Physically move object to forwarding address towards front of heap and reset marked.
 *
 *    This phase must be last as the object stores the forwarding address. When we move,
 *    we overwrite objects and could kill a forwarding address in a live object.
 */
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
	next_free = next_free_fowarding;  // next object to be allocated would occur here
	next_free_fowarding = heap_1;  //ready for next round of gc

	if (DEBUG) printf("DONE GC\n");
}

/* Alter the root to point at new location */
static void update_root(heap_object *p, int i) {
	if (DEBUG) {
		printf("UPDATE ROOT\n");
		if (p->forwarded != p) {
			printf("move root[%d]=%p -> %s@%p (0x%x bytes) to %p\n",
				   i,
				   _roots[i],
				   p->metadata->name,
				   p,
				   p->size,
				   p->forwarded);
		}
	}
	*_roots[i] = p->forwarded;	// update root to point at new address
}
/*
static void update_ptr_fields(heap_object *p) {
	int f;
	if (DEBUG) printf("update %d ptr fields of %s@%p\n", p->metadata->num_ptr_fields, p->metadata->name, p);
	for (f = 0; f < p->metadata->num_ptr_fields; f++) {
		int offset_of_ptr_field = p->metadata->field_offsets[f];
		void *ptr_to_ptr_field = ((void *) p) + offset_of_ptr_field;
		heap_object **ptr_to_obj_ptr_field = (heap_object **) ptr_to_ptr_field;
		heap_object *target_obj = *ptr_to_obj_ptr_field;
		if (target_obj != NULL) {
			if (DEBUG) {
				if ( target_obj->forwarded!=target_obj ) {
					printf("    update ptr (offset %d) from %p to %p\n",
					       offset_of_ptr_field,
					       target_obj,
					       target_obj->forwarded);
				}
			}
			*ptr_to_obj_ptr_field = target_obj->forwarded;
		}
	}
}*/

// ---------------------------------Scavenge and Forward Live Objects to Heap_1 ---------------------------------
void gc_scavenge() {
	if (DEBUG) printf("SCAVENGING...\n");
    for (int i = 0; i < num_roots; i++) {
        heap_object *p = *_roots[i];
		if (DEBUG) printf("roots[%d] = %p, p->forwarded = %p\n", i, p, p->forwarded );

		if (p == NULL)
			continue;
		if (ptr_is_in_heap_1(p))   // heap_1 already possesses it
			return;
		else if ( ptr_is_in_heap_0(p) ) {  // in heap_0 and needs to be forwarded
			if(ptr_is_in_heap_1(p->forwarded)) {//already forwarded
				update_root(p, i);
				continue;
			}
			if (DEBUG) printf("root[%d]=%p -> %s@%p (0x%x bytes)\n", i, _roots[i], p->metadata->name, p, p->size);
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
	realloc_object(p);
	move_to_forwarding_addr(p);
	forward_ptr_fields(p);
}

static void forward_ptr_fields(const heap_object *p) {// check for tracked heap ptrs in this object
	for (int i = 0; i < p->metadata->num_ptr_fields; i++) {
		int offset_of_ptr_field = p->metadata->field_offsets[i];
		void *ptr_to_ptr_field = ((void *)p) + offset_of_ptr_field;
		heap_object **ptr_to_obj_ptr_field = (heap_object **) ptr_to_ptr_field;
		heap_object *target_obj = *ptr_to_obj_ptr_field;
		if (target_obj != NULL) {
			forward_object(target_obj);
			*ptr_to_obj_ptr_field = target_obj->forwarded; //update ptr field
		}
	}
}

// --------------------------------- S u p p o r t ---------------------------------

/* Checking heap info at the end of scavenging
 */
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
int gc_num_live_objects() {

	int n = 0;
	void *p = heap_0;
	while (p >= heap_0 && p < next_free) { // for each marked (live) object, record forwarding address
		if (((heap_object*)p)->forwarded == p)
			n++;
		p = p + ((heap_object *)p)->size;
	}
	return n;
}
/*int gc_num_live_objects() {
	int n = 0;
	for (int i = 0; i < num_roots; i++) {
		heap_object *p = *_roots[i];
		if (p == NULL)
			continue;
		if (ptr_is_in_heap_1(p) || ptr_is_in_heap_0(p)) {  // heap_1 already possesses it
			n++;
			for (int i = 0; i < p->metadata->num_ptr_fields; i++) {
				int offset_of_ptr_field = p->metadata->field_offsets[i];
				void *ptr_to_ptr_field = ((void *) p) + offset_of_ptr_field;
				heap_object **ptr_to_obj_ptr_field = (heap_object **) ptr_to_ptr_field;
				heap_object *target_obj = *ptr_to_obj_ptr_field;
				if (target_obj != NULL) n++;
			}
		}
		else  {
			if (DEBUG) printf("root[%d]=%p -> %p INVALID\n", i, _roots[i], p);
		}
	}
	return n;
}*/


/*size_t compute_busy_size() {
	size_t busy = 0;
	for (int i = 0; i < num_roots; i++) {
		heap_object *p = *_roots[i];
		if (p == NULL)
			continue;
		if (ptr_is_in_heap_1(p) || ptr_is_in_heap_0(p)) {  // heap_1 already possesses it
			busy += p->size;
			for (int i = 0; i < p->metadata->num_ptr_fields; i++) {
				int offset_of_ptr_field = p->metadata->field_offsets[i];
				void *ptr_to_ptr_field = ((void *) p) + offset_of_ptr_field;
				heap_object **ptr_to_obj_ptr_field = (heap_object **) ptr_to_ptr_field;
				heap_object *target_obj = *ptr_to_obj_ptr_field;
				if (target_obj != NULL) busy+= target_obj->size;
			}
		}
		else  {
			if (DEBUG) printf("root[%d]=%p -> %p INVALID\n", i, _roots[i], p);
		}
	}
	return busy;
}*/
/* Apply function action to each marked (live) object in the heap; assumes live are marked *//*

void foreach_live(void (*action)(heap_object *)) {
	void *p = heap_0;
	while (p >= heap_0 && p < next_free) { // for each marked (live) object
		heap_object *_p = (heap_object *)p;
		size_t size = _p->size;
		if (DEBUG) {
			//if (_p->magic != MAGIC_NUMBER) printf("INVALID ptr %p in foreach_live()\n", _p);
		}
		if ( _p->marked ) {
			action(p);
		}
		p = p + size;
	}
}*/
void foreach_object(void (*action)(heap_object *)) {
	void *p = heap_0;
	while (p >= heap_0 && p < next_free) { // for each object in the heap currently allocated
		size_t size = ((heap_object *)p)->size;
		action(p);
		p = p + size;
	}
}

