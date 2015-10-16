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

#ifndef RUNTIME_GC_H
#define RUNTIME_GC_H

extern object_metadata Vector_metadata;
extern object_metadata String_metadata;

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

/* Initialize a heap with a certain size for use with the garbage collector */
extern void gc_init(int size);

/* Announce you are done with the heap managed by the garbage collector */
extern void gc_shutdown();

/* Perform a mark_and_compact garbage collection, moving all live objects
 * to the start of the heap.
 */
extern void gc();
extern heap_object *gc_alloc(object_metadata *metadata, size_t size);
extern void gc_add_root(void **p);
extern void gc_mark();
extern void gc_unmark();

extern Heap_Info get_heap_info();

#endif //RUNTIME_GC_H
