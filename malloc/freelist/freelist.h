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
#include "morecore.h"

/* On my mac laptop Intel Core i7, i get with 32-bit size:
sizeof(Busy_Header) == 4
sizeof(Free_Header) == 16. hmm... it word aligns to 64 bits so next is 8 byte boundary

All chunks must start at 4 or 8-byte boundary. I.e., size & ALIGN_MASK should be 0.

Useful resource has been Doug Lea's malloc(): http://g.oswego.edu/pub/misc/malloc-2.6.6.c
*/

typedef struct _Busy_Header {
	uint32_t size;         // 31 bits for size and 1 bit for inuse/free; includes header data
	unsigned char mem[]; // nothing allocated; just a label to location after size
} Busy_Header;

typedef struct _Free_Header {
	uint32_t size;
	struct _Free_Header *next; // lives inside user data area when free but not when in use
} Free_Header;

typedef struct {
	int heap_size;    // total space obtained from OS
	int busy;
	int busy_size;
	int free;
	int free_size;
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
static inline uint32_t chunksize(void *p) { return ((Busy_Header *)p)->size & SIZEMASK; }

void freelist_init(uint32_t max_heap_size);
void freelist_shutdown();
Free_Header *get_freelist();
void *get_heap_base();
Heap_Info get_heap_info();
