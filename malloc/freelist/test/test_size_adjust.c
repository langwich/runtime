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
#include <stdint.h>
#include <assert.h>

#define MINSIZE				sizeof(Free_Header)
#define WORD_SIZE_IN_BYTES	sizeof(void *)
#define ALIGN_MASK_IN_BYTE            (WORD_SIZE_IN_BYTES-1)

typedef struct _Busy_Header {
	uint32_t size; 	  	  // 31 bits for size and 1 bit for inuse/free; doesn't include header
	unsigned char mem[]; // nothing allocated; just a label to location after size
} Busy_Header;

typedef struct _Free_Header {
	uint32_t size;
	struct _Free_Header *next; // lives inside user data area when free but not when in use
} Free_Header;

/* Pad size n to include header */
static inline size_t size_with_header(size_t n) {
	return n + sizeof(Busy_Header) <= MINSIZE ? MINSIZE : n + sizeof(Busy_Header);
}

/* Align n to nearest word size boundary (4 or 8) */
static inline size_t align_to_word_boundary(size_t n) {
	return (n & ALIGN_MASK_IN_BYTE) == 0 ? n : (n + WORD_SIZE_IN_BYTES) & ~ALIGN_MASK_IN_BYTE;
}

static inline size_t request2size(size_t n) {
	return align_to_word_boundary(size_with_header(n));
}

int main(int argc, char *argv[])
{
//	printf("sizeof(Busy_Header) == %zu\n", sizeof(Busy_Header));
//	printf("sizeof(Free_Header) == %zu\n", sizeof(Free_Header));
	for (int i=0; i<=MINSIZE-sizeof(Busy_Header); i++) {
//		fprintf(stderr, "request2size(%d) == %lu\n", i, request2size((size_t)i));
		assert(request2size((size_t) i) == MINSIZE );
	}
	if ( MINSIZE==16 )
	{
		static const int N = 9;
		static const size_t sizes[2*N] = {				// (request, adjusted size) pairs
			13, 24,
			14, 24,
			15, 24,
			16, 24,
			17, 24,
			18, 24,
			19, 24,
			20, 24,
			21, 32,
		};
		for (int i=0; i < 2*N; i += 2)
		{
//			fprintf(stderr, "request2size(%zu) == %lu\n", sizes[i], request2size(sizes[i]));
			assert(request2size(sizes[i]) == sizes[i+1]);
		}
	}
}