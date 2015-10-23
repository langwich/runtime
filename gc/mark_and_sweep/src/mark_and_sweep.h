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
    bool marked;        // used during the mark phase of garbage collection
    struct heap_object *next;
} heap_object;

// GC interface

typedef struct {
    void *start_of_heap;
    void *end_of_heap;
    int heap_size;
    int busy;
    int live;
    int computed_busy_size;
    int computed_free_size;
} Heap_Info;

extern void gc_init(int size);

extern void gc_shutdown();

extern void gc();
extern heap_object *gc_alloc(object_metadata *metadata, size_t size);
extern void gc_add_root(void **p);

extern Heap_Info get_heap_info();

#define gc_begin_func()		int __save = gc_num_roots()
#define gc_end_func()		gc_set_num_roots(__save)

extern int gc_num_live_objects();
extern void gc_debug(bool debug);
extern int gc_num_roots();
extern void gc_set_num_roots(int roots);
extern void foreach_object(void (*action)(heap_object *));
extern bool ptr_is_in_heap(heap_object *p);

#ifdef __cplusplus
}
#endif

#endif //RUNTIME_MARK_AND_SWEEP_H
