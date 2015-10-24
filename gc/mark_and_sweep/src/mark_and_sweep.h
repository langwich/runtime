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
#ifndef RUNTIME_MARK_AND_SWEEP_H
#define RUNTIME_MARK_AND_SWEEP_H

#include <stdbool.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif

/* stuff that every instance in the heap must have at the beginning (unoptimized) */
typedef struct heap_object {
    struct _object_metadata *metadata;
    uint32_t size;      // total size including header information used by each heap_object
    bool marked;	    // used during the mark phase of garbage collection
} heap_object;

typedef struct _Free_Header {//maintain a free list for unallocated chunks, also for sweep action
    int size;                //size of current chunk
    struct _Free_Header *next;//point to next free chunk
} Free_Header;

extern int gc_num_alloc_objects();

#ifdef __cplusplus
}
#endif

#endif //RUNTIME_MARK_AND_SWEEP_H
