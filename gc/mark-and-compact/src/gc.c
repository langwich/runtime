#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gc.h"
#include "morecore.h"

static bool DEBUG = false;

#define MAX_ROOTS		100

/* Track every pointer into the heap; includes globals, args, and locals */
static heap_object **_roots[MAX_ROOTS];

/* index of next free space in _roots for a root */
static int num_roots = 0;

static size_t heap_size;
static void *start_of_heap;
static void *end_of_heap;
static void *next_free;

static void gc_mark();
static void gc_mark_object(heap_object *p);
static void gc_sweep();

static void *gc_raw_alloc(size_t size);
static void update_roots();
static void foreach_live(void (*action)(heap_object *));
static void gc_chase_ptr_fields(const heap_object *p);
static void update_ptr_fields(heap_object *p);

//static int  gc_object_size(heap_object *p);
//static void gc_dump();
//static bool ptr_is_in_heap(heap_object *p);
//static char *gc_viz_heap();
//static void gc_free_object(heap_object *p);
//static void print_ptr(heap_object *p);
//static void print_addr_array(heap_object **array, int len);
//static char *long_array_to_str(unsigned long *array, int len);
//static unsigned long gc_rel_addr(heap_object *p);
//static int addrcmp(const void *a, const void *b);
//static void unmark_objects();
//static char *ptr_to_str(heap_object *p);

/* Initialize a heap with a certain size for use with the garbage collector */
void gc_init(int size) {
    heap_size = size;
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
    _roots[num_roots++] = (heap_object **)p;
}

/* Allocate an object per the indicated size, which must included heap_object / header info.
 * The object is zeroed out and the header is initialized.
 */
heap_object *gc_alloc(object_metadata *metadata, size_t size) {
	size = align_to_word_boundary(size);
	heap_object *p = gc_raw_alloc(size);

	memset(p, 0, size);         // wipe out object's data space and the header
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

static inline void realloc_object(heap_object *p) { p->forwarded = gc_raw_alloc(p->size); }
static inline void move_to_forwarding_addr(heap_object *p) { memcpy(p->forwarded, p, p->size); }

static inline bool ptr_is_in_heap(heap_object *p) {
	return p >= (heap_object *) start_of_heap && p <= (heap_object *) end_of_heap;
}

void gc_debug(bool debug) { DEBUG = debug; }

/* Perform a mark-and-compact garbage collection, moving all live objects
 * to the start of the heap. Anything that we don't mark is dead. Unlike
 * mark-n-sweep, we do not walk the garbage. The mark operation
 * results in a temporary array called live_objects with objects in random
 * address order. We sort by address order from low to high in order to
 * compact the heap without stepping on a live object. During one of the
 * passes over the live object list, reset p->marked = 0. I do it here in #4.
 *
 * 1. Walk object graph, marking live objects as with mark-sweep.
 *
 * 2. Next we walk all live objects and compute their forwarding addresses.
 *
 * 3. Alter all roots pointing to live objects to point at forwarding address.
 *
 * 4. Walk the live objects and alter all non-NULL managed pointer fields
 *    to point to the forwarding addresses.
 *
 * 5. Move all live objects to the start of the heap in ascending address order.
 */
void gc() {
    if (DEBUG) printf("gc_compact\n");

	gc_mark();

	next_free = start_of_heap; // reset to have 0 allocated space so first live object is at start_of_heap
	foreach_live(realloc_object);   // for each marked (live) object, record forwarding address

	update_roots();

	foreach_live(update_ptr_fields);

	// move objects to compact heap
	foreach_live(move_to_forwarding_addr);
}

/* Alter roots to point at new location of live objects (compacted) */
void update_roots() {
	for (int i = 0; i < num_roots; i++) {
		if (DEBUG) printf("move root[%d]=%p\n", i, _roots[i]);
		heap_object *p = *_roots[i];
		if (p != NULL && p->marked) {
			*_roots[i] = p->forwarded; // update root to point at new address
		}
	}
}

void update_ptr_fields(heap_object *p) {
	p->marked = 0; // reset marked bit in preparation for next GC event
	int f;
	if (DEBUG) printf("move ptr fields of %s@%p\n", p->metadata->name, p);
	for (f = 0; f < p->metadata->num_ptr_fields; f++) {
		int offset_of_ptr_field = p->metadata->field_offsets[f];
		void *ptr_to_ptr_field = ((void *) p) + offset_of_ptr_field;
		heap_object **ptr_to_obj_ptr_field = (heap_object **) ptr_to_ptr_field;
		heap_object *target_obj = *ptr_to_obj_ptr_field;
		if (target_obj != NULL) {
			*ptr_to_obj_ptr_field = target_obj->forwarded;
		}
	}
}

int gc_num_roots() {
    return num_roots;
}

void gc_set_num_roots(int roots)
{
    num_roots = roots;
}

/* Walk all roots and traverse object graph. Mark all p->mark=true for
   reachable p.
 */
static void gc_mark() {
    for (int i = 0; i < num_roots; i++) {
        if (DEBUG) printf("root[%d]=%p\n", i, _roots[i]);
        heap_object *p = *_roots[i];
        if (p != NULL) {
            if (DEBUG) printf("root=%p\n", p);
            if ( ptr_is_in_heap(p) ) {
                gc_mark_object(p);
            }
        }
    }
}

/* recursively walk object graph starting from p. */
static void gc_mark_object(heap_object *p) {
	if ( !p->marked ) {
		if (DEBUG) printf("mark %p\n", p);
		p->marked = 1;
		gc_chase_ptr_fields(p);
	}
}

void gc_chase_ptr_fields(const heap_object *p) {// check for tracked heap ptrs in this object
	for (int i = 0; i < p->metadata->num_ptr_fields; i++) {
		int offset_of_ptr_field = p->metadata->field_offsets[i];
		void *ptr_to_ptr_field = ((void *)p) + offset_of_ptr_field;
		heap_object **ptr_to_obj_ptr_field = (heap_object **) ptr_to_ptr_field;
		heap_object *target_obj = *ptr_to_obj_ptr_field;
		if (target_obj != NULL) {
			gc_mark_object(target_obj);
		}
	}
}

static void unmark_objects() {
	// done with live_objects, wack it
}

/* Walk heap jumping by size field of chunk header. Return an info record. */
Heap_Info get_heap_info() {
	heap_object *p = start_of_heap;
	int busy = 0;
	int computed_busy_size = 0;
	int busy_size = (uint32_t)(next_free - start_of_heap);
	int free_size = (uint32_t)(end_of_heap - next_free + 1);
	while ( p>=start_of_heap && p<next_free ) { // stay inbounds, walking heap
		// track
		busy++;
		computed_busy_size += p->size;
		p = (heap_object *)(((void *)p) + p->size);
	}
	return (Heap_Info){ start_of_heap, end_of_heap, next_free, (uint32_t)heap_size,
	                    busy, computed_busy_size, busy_size, free_size };
}

/* Apply function action to each marked (live) object in the heap */
void foreach_live(void (*action)(heap_object *)) {
	heap_object *p = start_of_heap;
	while (p >= start_of_heap && p < next_free) { // for each marked (live) object, record forwarding address
		if (p->marked) {
			action(p);
		}
		p = (heap_object *) (((void *) p) + p->size);
	}
}
