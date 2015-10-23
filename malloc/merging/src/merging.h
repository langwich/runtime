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

#ifndef MALLOC_MERGING_H
#define MALLOC_MERGING_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "morecore.h"


typedef struct _Busy_Header {
    uint32_t size;         // 31 bits for size and 1 bit for inuse/free; includes header data
    unsigned char mem[]; // nothing allocated; just a label to location after size
} Busy_Header;

typedef struct _Free_Header {
    uint32_t size; // 31 bits for size and 1bit for inuse/free; size of the entire chunk including header
    struct _Free_Header *next; // points to next free chunk
    struct _Free_Header *prev; // points to previous free chunk
} Free_Header;

typedef struct {
    int heap_size;    // total space obtained from OS
    int busy;         // number of busy chunks
    int busy_size;    // size of busy memory
    int free;         // number of free chunks
    int free_size;    // size of free memory
} Heap_Info;

static const size_t MIN_CHUNK_SIZE = sizeof(Free_Header);
static const size_t WORD_SIZE_IN_BYTES = sizeof(void *);
static const size_t ALIGN_MASK = WORD_SIZE_IN_BYTES - 1;

static const uint32_t BUSY_BIT = ((uint32_t)1) << 31; // the high bit (normally the sign bit)
static const uint32_t SIZEMASK = ~BUSY_BIT;

/* Pad size n to include header */
static inline size_t size_with_header(size_t n) {
    return n + sizeof(Busy_Header) <= MIN_CHUNK_SIZE ? MIN_CHUNK_SIZE : n + sizeof(Busy_Header);
}

/* Align n to nearest word size boundary (4 or 8) */
static inline size_t align_to_word_boundary(size_t n) {
    return (n & ALIGN_MASK) == 0 ? n : (n + WORD_SIZE_IN_BYTES) & ~ALIGN_MASK;
}

/* Convert a user request for n bytes into a size in bytes that has all necessary
 * header room and is word aligned.
 */
static inline size_t request2size(size_t n) {
    return align_to_word_boundary(size_with_header(n));
}

/** Given a free or allocated chunk, how many bytes long is this chunk including header */
static inline uint32_t chunksize(void *p) {
    if (p == NULL)   //null pointer gives a chunk of size 0;
        return 0;
    return ((Busy_Header *)p)->size & SIZEMASK;
}

void freelist_init(uint32_t max_heap_size);
void freelist_shutdown();
Free_Header *get_freelist();
void *get_heap_base();
Heap_Info get_heap_info();

void *malloc(size_t);
void free(void *);
void * find_next(void* p);
void merge_on_free(Free_Header *q, Free_Header *f);
void merge_on_malloc(Free_Header *q, Free_Header *f);
void check_infinite_loop(Free_Header *f, char *msg);
void print_both_ways(Free_Header *f);

#endif //MALLOC_MERGING_H


