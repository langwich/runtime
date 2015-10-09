#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gc.h"

#define DEBUG 0

#define MAX_ROOTS		100
#define MAX_OBJECTS 	200

static heap_object **_roots[MAX_ROOTS];
static int num_roots = 0; /* index of next free space in _roots for a root */

static int heap_size;
static uint8_t *start_of_heap;
static uint8_t *end_of_heap;
static uint8_t *next_free;

static void gc_mark_live();
static void gc_mark_object(heap_object *p);
static void gc_sweep();

static int  gc_object_size(heap_object *p);
static void gc_compact_object_list();
static void *gc_alloc_space(size_t size);
static void gc_dump();
static bool gc_in_heap(heap_object *p);
static char *gc_viz_heap();
static void gc_free_object(heap_object *p);
static void print_ptr(heap_object *p);
static void print_addr_array(heap_object **array, int len);
static char *long_array_to_str(unsigned long *array, int len);
static unsigned long gc_rel_addr(heap_object *p);
static int addrcmp(const void *a, const void *b);
static void unmark_objects();
static char *ptr_to_str(heap_object *p);

/* Initialize a heap with a certain size for use with the garbage collector */
void gc_init(int size) {
    heap_size = size;
    start_of_heap = malloc((size_t)size); //TODO: should this be morecore()?
    end_of_heap = start_of_heap + size - 1;
    next_free = start_of_heap;
    num_roots = 0;
}

/* Announce you are done with the heap managed by the garbage collector */
void gc_done() {
    free(start_of_heap);
}

void gc_add_addr_of_root(heap_object **p)
{
    _roots[num_roots++] = p;
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
    gc_mark_live(); // fills live_objects
}

extern heap_object *gc_alloc(size_t size, void (*chase_ptrs)(struct _heap_object *p)) {
    heap_object *p = gc_alloc_space(size);

    memset(p, 0, size+sizeof(heap_object));
    p->size = (uint32_t)size;
    p->chase_ptrs = chase_ptrs;
    return p; // spend hour looking for bug; forgot this
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
            if ( gc_in_heap(p) ) {
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
        p->chase_ptrs(p);
    }
}

static bool gc_in_heap(heap_object *p) {
    return p >= (heap_object *) start_of_heap && p <= (heap_object *) end_of_heap;
}

/** Allocate size bytes in the heap; if full, gc() */
static void *gc_alloc_space(size_t size) {
	size = request2size(size);
    if (next_free + size > end_of_heap) {
        gc(); // try to collect        
        if (next_free + size > end_of_heap) { // try again
            return NULL;                      // oh well, no room. puke
        }
    }

    void *p = next_free;
    next_free += size;
    return p;
}

static void unmark_objects() {
    // done with live_objects, wack it
}