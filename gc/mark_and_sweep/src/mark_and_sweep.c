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

#include <mark_and_sweep.h>
#include <gc.h>
#include <morecore.h>


static void mark();
static void mark_object(heap_object *p);
static void unmark_object(heap_object *p);
static void sweep();
static void free_object(heap_object *p);
static void *gc_raw_alloc(size_t size);
static void *gc_alloc_from_freelist(size_t size);
static void gc_chase_ptr_fields(const heap_object *p);
static bool already_in_freelist(heap_object *p);

static bool DEBUG = false;

static const int MAX_ROOTS = 1000;

static heap_object **_roots[MAX_ROOTS];
static int num_roots = 0;

static size_t heap_size;
static void *start_of_heap;
static void *end_of_heap;
static void *free_list;
static void *alloc_bump_ptr;


void gc_debug(bool debug) { DEBUG = debug; }

/* Initialize a heap with a certain size for use with the garbage collector */
void gc_init(int size) {
    if ( start_of_heap!=NULL ) { gc_shutdown(); }
    heap_size = (size_t)size;
    start_of_heap = morecore((size_t)size);
    end_of_heap = start_of_heap + size - 1;
    alloc_bump_ptr = start_of_heap;
    free_list = NULL;
    num_roots = 0;
}

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

    memset(p, 0, size);
    p->metadata = metadata;
    p->size = (uint32_t)size;
    return p;
}

static void *gc_raw_alloc(size_t size) {
    if (alloc_bump_ptr + size > end_of_heap) {
        void *object = gc_alloc_from_freelist(size);
        if (NULL == object) {
            gc();
            object = gc_alloc_from_freelist(size);
            if (object == NULL) {
                if (DEBUG) printf("memory is full");
                return NULL;
            }
            return object;
        }
    }
    void *p = alloc_bump_ptr;
    alloc_bump_ptr += size;
    return p;
}


static void *gc_alloc_from_freelist(size_t size) {

    heap_object *p = free_list;
    heap_object *prev = NULL;
    while (p != NULL && size != p->size && p->size < size) {
        prev = p;
        p = p->next;
    }
    if (p == NULL) return p;

    heap_object *nextchunk;
    if (p->size == size) {
        nextchunk = p->next;
    }
    else {
        heap_object *q = (heap_object *) (((char *) p) + size);
        q->size = p->size - size;
        q->next = p->next;
        nextchunk = q;
    }
    p->size = size;
    if (p == free_list) {
        free_list = nextchunk;
    }
    else {
        prev->next = nextchunk;
    }
    return p;
}


bool ptr_is_in_heap(heap_object *p) {
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

void unmark() { foreach_object(unmark_object); }

static void mark_object(heap_object *p) {
    if ( !p->marked ) {
        if (DEBUG) printf("mark    %s@%p (0x%x bytes)\n", p->metadata->name, p, p->size);
        p->marked = true;
        gc_chase_ptr_fields(p);
    }
}

static void gc_chase_ptr_fields(const heap_object *p) {
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
    void *p = start_of_heap;
    while (p >= start_of_heap && p < alloc_bump_ptr) {
        if (((heap_object *)p)->marked) {
            n++;
        }
        p = p + ((heap_object *)p)->size;
    }
    unmark();
    return n;
}


static void unmark_object(heap_object *p) { p->marked = false; }

static void sweep() {
    void *p = start_of_heap;
    while (p >= start_of_heap && p < alloc_bump_ptr) {
        heap_object * o = ((heap_object *)p);
        if (o->marked) {
            unmark_object(o);
        }
        else {
            free_object(o);
        }
        size_t size = ((heap_object *)p)->size;
        p = p + size;
    }
}

static void free_object(heap_object *p) {
    if (!already_in_freelist(p)){
        p->next = free_list;
        free_list = p;
        if (DEBUG) {
            printf("sweep object@%p\n", p);
        }
    }
}

static bool already_in_freelist(heap_object *p) {
    heap_object * ptr = free_list;
    while (ptr != NULL) {
        if (p == ptr) {
            return true;
        }
        ptr = ptr->next;
    }
    return false;
}

Heap_Info get_heap_info() {
    void *p = start_of_heap;
    int busy = 0;
    int live = gc_num_live_objects();
    int computed_busy_size = 0; //size of chunks was allocated
    int computed_free_size = 0;

    while ( p>=start_of_heap && p<alloc_bump_ptr ) { // stay inbounds, walking heap

        if (already_in_freelist((heap_object *)p)) {
            computed_free_size += ((heap_object *)p)->size;
        }
        else {
            busy++;
            computed_busy_size += ((heap_object *)p)->size;
        }
        p = p + ((heap_object *)p)->size;
    }
    computed_free_size += (uint32_t)(end_of_heap - alloc_bump_ptr + 1);

    return (Heap_Info){ start_of_heap, end_of_heap, alloc_bump_ptr,(uint32_t)heap_size,
                        busy, live, computed_busy_size, computed_free_size,computed_busy_size,computed_free_size};
}

void foreach_object(void (*action)(heap_object *)) {
    void *p = start_of_heap;
    while (p >= start_of_heap && p < alloc_bump_ptr) { // for each object in the heap currently allocated
        size_t size = ((heap_object *)p)->size;
        action(p);
        p = p + size;
    }
}
