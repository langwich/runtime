#ifndef RUNTIME_MARK_AND_COMPACT_H_
#define RUNTIME_MARK_AND_COMPACT_H_

#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char *name;               // "class" name of instances of this type; useful for debugging
	uint16_t num_ptr_fields;  // how many managed pointers (pointers into the heap) in this object
	uint16_t field_offsets[]; // list of offsets base of object to fields that are managed ptrs
} object_metadata;

/* stuff that every instance in the heap must have at the beginning (unoptimized) */
typedef struct heap_object {
	object_metadata *metadata;
	uint32_t size;      // total size including header information used by each heap_object
	bool marked;	    // used during the mark phase of garbage collection
	struct heap_object *forwarded; 			// where we've moved this object during collection
//	char data[];        // nothing allocated; just a label to location of actual instance data
} heap_object;

// GC interface

typedef struct {
	void *start_of_heap;    // first byte of heap
	void *end_of_heap;      // last byte of heap
	void *next_free;        // next addr where we'll allocate an object
	int heap_size;          // total space obtained from OS
	int busy;               // num allocated objects
	int computed_busy_size; // total allocated size computed by walking heap
	int busy_size;          // size from calculation
	int free_size;          // size from calculation
} Heap_Info;

/* Initialize a heap with a certain size for use with the garbage collector */
extern void gc_init(int size);

/* Announce you are done with the heap managed by the garbage collector */
extern void gc_shutdown();

/* Perform a mark-and-compact garbage collection, moving all live objects
 * to the start of the heap.
 */
extern void gc();
extern heap_object *gc_alloc(object_metadata *metadata, size_t size);
extern void gc_add_root(void **p);

extern Heap_Info get_heap_info();

#define gc_begin_func()		int __save = gc_num_roots()
#define gc_end_func()		gc_set_num_roots(__save)

// GC internals; peek into internals for testing and hidden use in macros

extern int gc_num_live_objects();
extern void gc_debug(bool debug);
extern char *gc_get_state();
extern long gc_heap_highwater();
extern int gc_num_roots();
extern void gc_set_num_roots(int roots);

static const size_t WORD_SIZE_IN_BYTES = sizeof(void *);
static const size_t ALIGN_MASK = WORD_SIZE_IN_BYTES - 1;

/* Pad size n to include header */
static inline size_t size_with_header(size_t n) {
	return n + sizeof(heap_object);
}

/* Align size n to nearest word size boundary (4 or 8) */
static inline size_t align_to_word_boundary(size_t n) {
	return (n & ALIGN_MASK) == 0 ? n : (n + WORD_SIZE_IN_BYTES) & ~ALIGN_MASK;
}

/* Convert a user request for n bytes into a size in bytes that has all necessary
 * header room and is word aligned.
 */
static inline size_t request2size(size_t n) {
	return align_to_word_boundary(size_with_header(n));
}


#ifdef __cplusplus
}
#endif

#endif