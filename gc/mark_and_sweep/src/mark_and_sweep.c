#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mark_and_sweep.h"
#include <morecore.h>
#include <gc.h>

static void mark();
static void mark_object(heap_object *p);
static void unmark_object(heap_object *p);
static void sweep();
static void free_heapobject(int i, heap_object *p);

static void *gc_raw_alloc(size_t size);
static void *gc_alloc_space(int size);
static void update_roots();
static void gc_chase_ptr_fields(const heap_object *p);
static void update_ptr_fields(heap_object *p);

static bool DEBUG = false;

static const int MAX_ROOTS = 1000; // obviously this is ok only for the educational purpose of this code
static const int MAX_OBJECTS = 10000;
/* Track every pointer into the heap; includes globals, args, and locals */
static heap_object **_roots[MAX_ROOTS];
static heap_object *_objects[MAX_OBJECTS];
/* index of next free space in _roots for a root */
static int num_roots = 0;
static int num_objects = 0;

static size_t heap_size;
static void *start_of_heap;
static void *end_of_heap;
Free_Header *next_free;

void gc_debug(bool debug) { DEBUG = debug; }

/* Initialize a heap with a certain size for use with the garbage collector */
void gc_init(int size) {
    if ( start_of_heap!=NULL ) { gc_shutdown(); }
    heap_size = (size_t)size;
    start_of_heap = morecore((size_t)size);
    end_of_heap = start_of_heap + size - 1;
    next_free = (Free_Header *)start_of_heap;
    next_free->size = size;
    num_roots = 0;
    num_objects = 0;
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

//Allocation
heap_object *gc_alloc(object_metadata *metadata, size_t size) {
    if ( start_of_heap==NULL ) { gc_init(DEFAULT_MAX_HEAP_SIZE); }
    size = align_to_word_boundary(size);
    heap_object *p = gc_raw_alloc(size);

    memset(p, 0, size);         // wipe out object's data space and the header
    p->metadata = metadata;     // make sure object knows where its metadata is
    p->size = (uint32_t)size;
    _objects[num_objects++] = p;
    return p;
}

static void *gc_raw_alloc(size_t size) {
    void *object = gc_alloc_space(size);
    if(NULL == object) {
        gc();
        object = gc_alloc_space(size);
        if (object == NULL) {
            if (DEBUG) printf("memory is full");
            return NULL;
        }
    }
    return object;
}

static void *gc_alloc_space(int size) {
    Free_Header *p = next_free;
    Free_Header *prev = NULL;
    while (p != NULL && size != p->size && p->size < size + sizeof(Free_Header)) {
        prev = p;
        p = p->next;
    }
    if (p == NULL) return p;

    Free_Header *nextchunk;
    if (p->size == size) {
        nextchunk = p->next;
    }
    else {
        Free_Header *q = (Free_Header *) (((char *) p) + size);
        q->size = p->size - size;
        q->next = p->next;
        nextchunk = q;
    }
    p->size = size;
    if (p == next_free) {
        next_free = nextchunk;
    }
    else {
        prev->next = nextchunk;
    }
    return p;
}

static inline bool ptr_is_in_heap(heap_object *p) {
    return  p >= (heap_object *) start_of_heap &&
            p <= (heap_object *) end_of_heap;
}

void gc() {
    if(DEBUG) printf("begin_mark\n");
    mark();
    if(DEBUG) printf("begin_sweep\n");
    sweep();
}

static void mark() {
    for (int i = 0; i < num_roots; i++) {
        if (DEBUG) printf("root[%d]=%p\n", i, _roots[i]);
        heap_object *p = *_roots[i];
        if (p != NULL) {
            if (ptr_is_in_heap(p)) {
                mark_object(p);
            }
        }
        else if ( DEBUG ) {
            if (DEBUG) printf("root[%d]=%p -> %p INVALID\n", i, _roots[i], p);
        }
    }
}

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

int gc_num_live_objects() {
    mark();
    int n = 0;
    for(int i = 0; i < num_objects; i++) {
        heap_object *p = _objects[i];
        if (p!= NULL && p->marked) {
            n++;
            unmark_object(p);
        }
    }
    return n;
}

static void unmark_object(heap_object *p) { p->marked = false; }

int gc_num_alloc_objects() {
    return num_objects;
}

static void sweep() {
    for(int i = 0; i < num_objects; i++) {
        heap_object *p = _objects[i];
        if(p != NULL){
            if (p->marked) { p->marked = false; }
            else {
                if( p != NULL ) {
                    free_heapobject(i, p);
                }
            }
        }
    }
}

static void free_heapobject(int i,heap_object *p) {
    _objects[i] = NULL;
    Free_Header *q  = (Free_Header*)p;
    q->size = p->size;
    q->next = next_free;
    next_free = q;
    if (DEBUG) {
        printf("sweep object@%p\n", q);
    }
}

Heap_Info get_heap_info() {
    int busy = 0;
    int live = gc_num_live_objects();
    int computed_busy_size = 0; //size of chunks was allocated
    int computed_free_size = 0;
    for (int i = 0; i < num_objects; i++) {
        heap_object *p = _objects[i];
        if(p != NULL) {
            busy++;
            computed_busy_size += p->size;
        }
    }
    Free_Header* ptr = next_free;
    while (ptr != NULL) {
        computed_free_size += ptr->size;
        ptr = ptr->next;
    }
    return (Heap_Info){ start_of_heap, end_of_heap, (uint32_t)heap_size,
                        busy, live, computed_busy_size, computed_free_size };
}


