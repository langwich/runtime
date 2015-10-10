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

static void gc_mark_live();
static void gc_mark_object(heap_object *p);
static void gc_sweep();

static void *gc_alloc_space(size_t size);

static inline bool ptr_is_in_heap(heap_object *p) {
	return p >= (heap_object *) start_of_heap && p <= (heap_object *) end_of_heap;
}

void gc_debug(bool debug) { DEBUG = debug; }

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

void gc_add_addr_of_root(heap_object **p)
{
    _roots[num_roots++] = p;
}

heap_object *gc_alloc_with_data(object_metadata *metadata, size_t data_size) {
	size_t size = request2size(metadata->size + data_size);  // update size to include header and any variably-sized data
	heap_object *p = gc_alloc_space(size);

	memset(p, 0, size);         // wipe out object's data space and the header
	p->metadata = metadata;     // make sure object knows where its metadata is
	return p;
}

heap_object *gc_alloc(object_metadata *metadata) {
	return gc_alloc_with_data(metadata, 0);
}

/** Allocate size bytes in the heap by bumping high-water mark; if full, gc() and try again.
 *  Size must include any header size and must be word-aligned.
 */
static void *gc_alloc_space(size_t size) {
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

/* Perform a mark-and-compact garbage collection, moving all live objects
 * to the start of the heap. Anything that we don't mark is dead. Unlike
 * mark-n-sweep, we do not walk the garbage. The mark operation
 * results in a temporary array called live_objects with objects in random
 * address order. We sort by address order from low to high in order to
 * compact the heap without stepping on a live object. During one of the
 * passes over the live object list, reset p->marked = 0. I do it here in #5.
 *
 * 1. Walk object graph, marking live objects as with mark-sweep.
 *
 * 2. Then sort live object list by address. We have to compact by walking
 *    the objects in address order, low to high.
 *
 * 3. Next we walk all live objects and compute their forwarding addresses.
 *
 * 4. Alter all roots pointing to live objects to point at forwarding address.
 *
 * 5. Walk the live objects and alter all non-NULL managed pointer fields
 *    to point to the forwarding addresses.
 *
 * 6. Move all live objects to the start of the heap in ascending address order.
 */
void gc() {
    if (DEBUG) printf("gc_compact\n");
//    gc_mark_live(); // fills live_objects
}

int gc_num_roots() {
    return num_roots;
}

void gc_set_num_roots(int roots)
{
    num_roots = roots;
}

/* Walk all roots and traverse object graph. Mark all p->mark=true for
   reachable p.  Fill live_objects[], leaving num_live_objects set at number of live.
 */
static void gc_mark_live() {
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
    if (!p->marked) {
        int i;
        if (DEBUG) printf("mark %p\n", p);
        p->marked = 1;
        // check for tracked heap ptrs in this object
    }
}

static void unmark_objects() {
    // done with live_objects, wack it
}

/* Walk heap jumping by size field of chunk header. Return an info record. */
Heap_Info get_heap_info() {
	heap_object *p = start_of_heap;
	uint32_t busy = 0;
	uint32_t computed_busy_size = 0;
	uint32_t busy_size = (uint32_t)(next_free - start_of_heap);
	uint32_t free_size = (uint32_t)(end_of_heap - next_free + 1);
	while ( p>=start_of_heap && p<next_free ) { // stay inbounds, walking heap
		// track
		busy++;
		computed_busy_size += p->metadata->size;
		p = (heap_object *)((void *) p + p->metadata->size);
	}
	return (Heap_Info){ start_of_heap, end_of_heap, next_free, heap_size,
	                    busy, computed_busy_size, busy_size, free_size };
}
