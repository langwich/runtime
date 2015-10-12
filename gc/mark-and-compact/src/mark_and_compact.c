#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mark_and_compact.h>
#include <wich.h>
#include <morecore.h>

static void mark();
static void mark_object(heap_object *p);

static void *gc_raw_alloc(size_t size);
static void update_roots();
static void foreach_live(void (*action)(heap_object *));
static void gc_chase_ptr_fields(const heap_object *p);
static void update_ptr_fields(heap_object *p);

// --------------------------------- D A T A ---------------------------------

static bool DEBUG = false;

static const int MAX_ROOTS = 1000; // obviously this is ok only for the educational purpose of this code

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


// --------------------------------- C o l l e c t i o n ---------------------------------

static inline void realloc_object(heap_object *p) {
	void *q = next_free_forwarding; // bump-ptr-allocation
	next_free_forwarding += p->size;
	p->forwarded = q; // p now knows where it will end up after compacting
}

static inline void move_to_forwarding_addr(heap_object *p) { memcpy(p->forwarded, p, p->size); }

static inline void move_object_to_forwarding_addr(heap_object *p) {
	update_ptr_fields(p);        	// reset ptr fields to point to forwarding address
	move_to_forwarding_addr(p);		// move objects to compact heap
	p->marked = false;        		// reset marked bit for live object trace during next collection
}

static inline bool ptr_is_in_heap(heap_object *p) {
	return p >= (heap_object *) start_of_heap && p <= (heap_object *) end_of_heap;
}

static inline void unmark_object(heap_object *p) { p->marked = false; }

/* Perform a mark-and-compact garbage collection, moving all live objects
 * to the start of the heap. Anything that we don't mark is dead.
 *
 * 1. Walk object graph starting from roots, marking live objects.
 *
 * 2. Walk all live objects and compute their forwarding addresses starting from start_of_heap.
 *
 * 3. For each live object:
 * 		a) alter all non-NULL managed pointer fields to point to the forwarding addresses.
 * 		b) physically move object to forwarding address towards front of heap
 * 		c) unmark object
 *
 * 4. Alter all non-NULL roots to point to the object's forwarding address.
 */
void gc() {
    if (DEBUG) printf("gc_compact\n");

	mark();

	// reallocate all live objects starting from start_of_heap
	next_free_forwarding = start_of_heap;
	foreach_live(realloc_object);  		// for each marked (live) object, record forwarding address
	next_free = next_free_forwarding;	// next object to be allocated would occur here

	// Now that we know where to move objects, update and move objects
	foreach_live(move_object_to_forwarding_addr);

	update_roots();						// make sure all roots point at new object addresses
}

/* Alter roots to point at new location of live objects (compacted) */
static void update_roots() {
	for (int i = 0; i < num_roots; i++) {
		if (DEBUG) printf("move root[%d]=%p\n", i, _roots[i]);
		heap_object *p = *_roots[i];
		if ( p!=NULL ) {
			*_roots[i] = p->forwarded;	// update root to point at new address
		}
	}
}

static void update_ptr_fields(heap_object *p) {
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

// --------------------------------- M a r k (T r a c e)  O b j e c t s ---------------------------------

/* Walk all roots and traverse object graph. Mark all p->mark=true for
   reachable p.
 */
static void mark() {
    for (int i = 0; i < num_roots; i++) {
        if (DEBUG) printf("root[%d]=%p\n", i, _roots[i]);
        heap_object *p = *_roots[i];
        if (p != NULL) {
            if (DEBUG) printf("root=%p\n", p);
            if ( ptr_is_in_heap(p) ) {
				mark_object(p);
            }
        }
    }
}

static void unmark() { foreach_live(unmark_object); }

int gc_num_live_objects() {
	mark();

	int n = 0;
	void *p = start_of_heap;
	while (p >= start_of_heap && p < next_free) { // for each marked (live) object, record forwarding address
		if (((heap_object *)p)->marked) {
			n++;
		}
		p = p + ((heap_object *)p)->size;
	}

	unmark();
	return n;
}

/* recursively walk object graph starting from p. */
static void mark_object(heap_object *p) {
	if ( !p->marked ) {
		if (DEBUG) printf("mark %p\n", p);
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
	                    busy, computed_busy_size, busy_size, free_size };
}

/* Apply function action to each marked (live) object in the heap */
void foreach_live(void (*action)(heap_object *)) {
	void *p = start_of_heap;
	while (p >= start_of_heap && p < next_free) { // for each marked (live) object
		if (((heap_object *)p)->marked) {
			action(p);
		}
		p = p + ((heap_object *)p)->size;
	}
}

object_metadata Vector_metadata = {
		"Vector",
		0
};

object_metadata String_metadata = {
		"String",
		0
};

Vector *Vector_alloc(size_t length) {
	Vector *p = (Vector *)gc_alloc(&Vector_metadata, sizeof(Vector) + length * sizeof(double));
	p->length = length;
	return p;
}

String *String_alloc(size_t length) {
	String *p = (String *)gc_alloc(&String_metadata, sizeof(String) + length * sizeof(char));
	p->length = length;
	return p;
}