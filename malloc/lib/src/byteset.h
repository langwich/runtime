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

#ifndef MALLOC_BYTESET_H
#define MALLOC_BYTESET_H

#include <stddef.h>
#include <string.h>

typedef unsigned char           U1;
typedef __uint32_t              U32;

#define WORD_SIZE_IN_BYTE       (sizeof(void *))
#define NOT_FOUND       ((size_t)~0x0)
#define ALIGN_MASK      (WORD_SIZE_IN_BYTE - 1)

#define WORD(x)                 ((unsigned long long *)x)
#define ALIGN_WORD_BOUNDARY(n)  ((n & ALIGN_MASK) == 0 ? n : (n + WORD_SIZE_IN_BYTE) & ~ALIGN_MASK)

typedef struct {
	size_t num_words;
	char *bytes;
} byset;

void byset_init(byset *, size_t, void *);
size_t byset_nrun(byset *, size_t);
int byset_contain_ones(byset *, size_t, size_t);

/*
 * Set the bytes in [lo,hi] (both inclusive) to 1.
 */
static inline void byset_set1(byset *pbys, size_t lo, size_t hi) {
	memset(pbys->bytes + lo, '1', hi - lo + 1);
}
/*
 * Set the bytes in [lo,hi] (both inclusive) to 0.
 */
static inline void byset_set0(byset *pbys, size_t lo, size_t hi) {
	memset(pbys->bytes + lo, '0', hi - lo + 1);
}

#endif //MALLOC_BYTESET_H
