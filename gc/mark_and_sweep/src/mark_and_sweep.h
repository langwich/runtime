#ifndef RUNTIME_MARK_AND_SWEEP_H
#define RUNTIME_MARK_AND_SWEEP_H

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
} heap_object;

// GC interface

typedef struct {
    void *start_of_heap;    // first byte of heap
    void *end_of_heap;      // last byte of heap
    int heap_size;          // total space obtained from OS
    int busy;               // num allocated _objects (computed by walking heap)
    int live;               // num live _objects (computed by walking heap)
    int computed_busy_size; // total allocated size computed by walking heap
    int computed_free_size;
} Heap_Info;

typedef struct _Free_Header {//maintain a free list for unallocated chunks, also for sweep action
    int size;                //size of current chunk
    struct _Free_Header *next;//point to next free chunk
}Free_Header;

/* Initialize a heap with a certain size for use with the garbage collector */
extern void gc_init(int size);

/* Announce you are done with the heap managed by the garbage collector */
extern void gc_shutdown();

/* Perform a mark_and_compact garbage collection, moving all live _objects
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
extern int gc_num_alloc_objects();
extern void gc_debug(bool debug);
extern int gc_num_roots();
extern void gc_set_num_roots(int roots);
extern void foreach_live(void (*action)(heap_object *));
extern void foreach_object(void (*action)(heap_object *));


#ifdef __cplusplus
}
#endif

#endif //RUNTIME_MARK_AND_SWEEP_H
