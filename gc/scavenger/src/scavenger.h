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
#ifndef RUNTIME_SCAVENGER_H_
#define RUNTIME_SCAVENGER_H_

#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

//static const uint32_t MAGIC_NUMBER = 123456789;

/* stuff that every instance in the heap must have at the beginning (unoptimized) */
typedef struct heap_object {
	struct _object_metadata *metadata;
	uint32_t size;      // total size including header information used by each heap_object
//	bool marked;	    // no mark phase for scavenger
	struct heap_object *forwarded; 			// where we've moved this object into heap_1
} heap_object;

extern long gc_heap_highwater();
extern bool ptr_is_in_heap_0(heap_object *p);
extern int gc_num_live_objects();
extern int gc_count_roots();

#ifdef __cplusplus
}
#endif

#endif  //RUNTIME_SCAVENGER_H
