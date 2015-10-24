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

#include <mark_and_compact.h>
#include <gc.h>
#include <morecore.h>

static void *gc_raw_alloc(size_t size);
static void update_roots();
static void gc_chase_ptr_fields(const heap_object *p);
static void update_ptr_fields(heap_object *p);
static void mark_object(heap_object *p);

// --------------------------------- D A T A ---------------------------------

static bool DEBUG = false;

static const int MAX_ROOTS = 100000; // obviously this is ok only for the educational purpose of this code

/* Track every pointer into the heap; includes globals, args, and locals */
static heap_object **_roots[MAX_ROOTS];

/* index of next free space in _roots for a root */
static int num_roots = 0;

static size_t heap_size;
static void *start_of_heap;
static void *end_of_heap;
static void *next_free;
static void *next_free_forwarding; // next_free used during forwarding address computation


// --------------------------------- G C  I n i t  &  R o o t  M g m t ---------------------------------

void gc_debug(bool debug) { DEBUG = debug; }

/* Initialize a heap with a certain size for use with the garbage collector */
void gc_init(int size) {
	if ( start_of_heap!=NULL ) { gc_shutdown(); }
    heap_size = (size_t)size;
    start_of_heap = morecore((size_t)size);
    end_of_heap = start_of_heap + size - 1;
    next_free = start_of_heap;
    num_roots = 0;
}

/* Announce you are done with the heap managed by the garbage collector */
void gc_shutdown() {
    dropcore(start_of_heap, heap_size);
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
	if ( start_of_heap==NULL ) { gc_init(DEFAULT_MAX_HEAP_SIZE); }
	size = align_to_word_boundary(size);
	heap_object *p = gc_raw_alloc(size);

	if ( p==NULL ) return NULL;

	memset(p, 0, size);         // wipe out object's data space and the header
	p->magic = MAGIC_NUMBER;    // a safety measure; if not magic num, we didn't alloc
	p->metadata = metadata;     // make sure object knows where its metadata is
	p->size = (uint32_t)size;
	return p;
}

/** Allocate size bytes in the heap by bumping high-water mark; if full, gc() and try again.
 *  Size must include any header size and must be word-aligned.
 */
static void *gc_raw_alloc(size_t size) {
	if (next_free + size > end_of_heap) {
		gc(); // try to collect
		if (next_free + size > end_of_heap) { // try again
			return NULL;                      // oh well, no room. puke
		}
	}

	void *p = next_free; // bump-ptr-allocation
	next_free += size;
	return p;
}


// --------------------------------- C o l l e c t i o n ---------------------------------

static inline void realloc_object(heap_object *p) {
	void *q = next_free_forwarding; // bump-ptr-allocation
	next_free_forwarding += p->size;
	p->forwarded = q; // p now knows where it will end up after compacting
	if (DEBUG) if ( p->forwarded!=p ) printf("forward %p to %s@%p (0x%x bytes)\n", p, p->metadata->name, p->forwarded, p->size);
}

static inline void move_to_forwarding_addr(heap_object *p) {
	if (DEBUG) if ( p->forwarded!=p ) printf("    move %p to %p (0x%x bytes)\n", p, p->forwarded, p->size);
	if ( p->forwarded!=p ) {
		memcpy(p->forwarded, p, p->size);
	}
}

static inline void move_live_objects_to_forwarding_addr(heap_object *p) {
	if ( p->marked ) {
		if (DEBUG) printf("live %s@%p (0x%x bytes)\n", p->metadata->name, p, p->size);
		p->marked = false;              // reset *before* move or you're changing old object
		move_to_forwarding_addr(p);     // move objects to compact heap
	}
	else {
		if (DEBUG) printf("dead %s@%p (0x%x bytes)\n", p->metadata->name, p, p->size);
		p->magic = 0;
	}
}

bool ptr_is_in_heap(heap_object *p) {
	return  p >= (heap_object *) start_of_heap &&
			p <= (heap_object *) end_of_heap &&
			p->magic == MAGIC_NUMBER;
}

static inline void unmark_object(heap_object *p) { p->marked = false; }

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
    if (DEBUG) printf("GC\n");

	gc_mark();

	// reallocate all live objects starting from start_of_heap
	if (DEBUG) printf("FORWARD\n");
	next_free_forwarding = start_of_heap;
	foreach_live(realloc_object);  		// for each marked (live) object, record forwarding address

	// make sure all roots point at new object addresses
	update_roots();                     // can't move objects before updating roots; roots point at *old* location

	if (DEBUG) printf("UPDATE PTR FIELDS\n");
	foreach_live(update_ptr_fields);

	// Now that we know where to move objects, update and move objects
	if (DEBUG) printf("COMPACT\n");
	foreach_object(move_live_objects_to_forwarding_addr); // also visits the dead to wack p->magic

	// reset highwater mark *after* we've moved everything around; foreach_object() uses next_free
	next_free = next_free_forwarding;	// next object to be allocated would occur here

	if (DEBUG) printf("DONE GC\n");
}

/* Alter roots to point at new location of live objects (compacted) */
static void update_roots() {
	if (DEBUG) printf("UPDATE ROOTS\n");
	for (int i = 0; i < num_roots; i++) {
		heap_object *p = *_roots[i];
		if ( p!=NULL ) {
			if (DEBUG) {
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
	}
}

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
}

// --------------------------------- M a r k (T r a c e)  O b j e c t s ---------------------------------

/* Walk all roots and traverse object graph. Mark all p->mark=true for
   reachable p.
 */
void gc_mark() {
	if (DEBUG) printf("MARK\n");
    for (int i = 0; i < num_roots; i++) {
        heap_object *p = *_roots[i];
        if ( p != NULL ) {
            if ( ptr_is_in_heap(p) ) {
	            if (DEBUG) printf("root[%d]=%p -> %s@%p (0x%x bytes)\n", i, _roots[i], p->metadata->name, p, p->size);
				mark_object(p);
            }
	        else if ( DEBUG ) {
	            if (DEBUG) printf("root[%d]=%p -> %p INVALID\n", i, _roots[i], p);
            }
        }
    }
}

void gc_unmark() {
	if (DEBUG) printf("UNMARK\n");
	foreach_object(unmark_object);
}

int gc_num_live_objects() {
//	gc_unmark();
	gc_mark();

	int n = 0;
	void *p = start_of_heap;
	while (p >= start_of_heap && p < next_free) { // for each marked (live) object, record forwarding address
		if (((heap_object *)p)->marked) {
			n++;
		}
		p = p + ((heap_object *)p)->size;
	}

	gc_unmark();
	return n;
}

/* recursively walk object graph starting from p. */
static void mark_object(heap_object *p) {
	if ( !p->marked ) {
		if (DEBUG) printf("mark    %s@%p (0x%x bytes)\n", p->metadata->name, p, p->size);
		p->marked = true;
		gc_chase_ptr_fields(p);
	}
}

static void gc_chase_ptr_fields(const heap_object *p) {// check for tracked heap ptrs in this object
	for (int i = 0; i < p->metadata->num_ptr_fields; i++) {
		int offset_of_ptr_field = p->metadata->field_offsets[i];
		void *ptr_to_ptr_field = ((void *)p) + offset_of_ptr_field;
		heap_object **ptr_to_obj_ptr_field = (heap_object **) ptr_to_ptr_field;
		heap_object *target_obj = *ptr_to_obj_ptr_field;
		if (target_obj != NULL) {
			mark_object(target_obj);
		}
	}
}

// --------------------------------- S u p p o r t ---------------------------------

/* Walk heap jumping by size field of chunk header. Return an info record.
 * All data below highwater mark is considered busy as this routine
 * does not do liveness trace.
 */
Heap_Info get_heap_info() {
	void *p = start_of_heap;
	int busy = 0;
	int live = gc_num_live_objects();
	int computed_busy_size = 0;
	int busy_size = (uint32_t)(next_free - start_of_heap);
	int free_size = (uint32_t)(end_of_heap - next_free + 1);
	while ( p>=start_of_heap && p<next_free ) { // stay inbounds, walking heap
		// track
		busy++;
		computed_busy_size += ((heap_object *)p)->size;
		p = p + ((heap_object *)p)->size;
	}
	return (Heap_Info){ start_of_heap, end_of_heap, next_free, (uint32_t)heap_size,
	                    busy, live, computed_busy_size, 0, busy_size, free_size };
}

/* Apply function action to each marked (live) object in the heap; assumes live are marked */
void foreach_live(void (*action)(heap_object *)) {
	void *p = start_of_heap;
	while (p >= start_of_heap && p < next_free) { // for each marked (live) object
		heap_object *_p = (heap_object *)p;
		size_t size = _p->size;
		if (DEBUG) {
			if (_p->magic != MAGIC_NUMBER) printf("INVALID ptr %p in foreach_live()\n", _p);
		}
		if ( _p->marked ) {
			action(p);
		}
		p = p + size;
	}
}

void foreach_object(void (*action)(heap_object *)) {
	void *p = start_of_heap;
	while (p >= start_of_heap && p < next_free) { // for each object in the heap currently allocated
		size_t size = ((heap_object *)p)->size;
		action(p);
		p = p + size;
	}
}

