#ifndef RUNTIME_MARK_AND_COMPACT_H_
#define RUNTIME_MARK_AND_COMPACT_H_

#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static const uint32_t MAGIC_NUMBER = 123456789;

typedef struct {
	char *name;               // "class" name of instances of this type; useful for debugging
	uint16_t num_ptr_fields;  // how many managed pointers (pointers into the heap) in this object
	uint16_t field_offsets[]; // list of offsets base of object to fields that are managed ptrs
} object_metadata;

/* stuff that every instance in the heap must have at the beginning (unoptimized) */
typedef struct heap_object {
	uint32_t magic;     // used in debugging
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
	int busy;               // num allocated objects (computed by walking heap)
	int live;               // num live objects (computed by walking heap)
	int computed_busy_size; // total allocated size computed by walking heap
	int busy_size;          // size from calculation
	int free_size;          // size from calculation
} Heap_Info;


#define gc_begin_func()		int __save = gc_num_roots()
#define gc_end_func()		gc_set_num_roots(__save)

// GC internals; peek into internals for testing and hidden use in macros

extern int gc_num_live_objects();
extern void gc_debug(bool debug);
extern char *gc_get_state();
extern long gc_heap_highwater();
extern int gc_num_roots();
extern void gc_set_num_roots(int roots);
extern void foreach_live(void (*action)(heap_object *));
extern void foreach_object(void (*action)(heap_object *));
extern bool ptr_is_in_heap(heap_object *p);


#ifdef __cplusplus
}
#endif

#endif
