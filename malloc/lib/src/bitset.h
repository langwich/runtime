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

#ifndef MALLOC_BITSET_H
#define MALLOC_BITSET_H

#include <stddef.h>
#include <stdbool.h>

// TODO: length of WORD and WORD is really machine dependent
typedef unsigned long long      WORD;// one full chunk covers 512 bytes in the heap.
typedef unsigned char           U1;
typedef __uint32_t              U32;

#define WORD_SIZE_IN_BYTE       (sizeof(WORD))
#define BIT_NUM                 8
#define WORD_SIZE_IN_BIT        (WORD_SIZE_IN_BYTE * BIT_NUM)
#define WORD(x)                 ((WORD *)x)
#define BITSET_NON              ((WORD) ~0x0)// we can never get that much memory
#define ALIGN_MASK_IN_BYTE      (WORD_SIZE_IN_BYTE - 1)
#define ALIGN_MASK_IN_BIT       (WORD_SIZE_IN_BIT - 1)

#define BC_ONE                  0xFFFFFFFFFFFFFFFF
#define BC_LEFTMOST_MASK        0x8000000000000000
#define ALIGN_WORD_BOUNDARY(n)  ((n & ALIGN_MASK_IN_BYTE) == 0 ? n : (n + WORD_SIZE_IN_BYTE) & ~ALIGN_MASK_IN_BYTE)
#define ALIGN_CHUNK_BOUNDARY(n) ((n & ALIGN_MASK_IN_BIT) == 0 ? n : (n + WORD_SIZE_IN_BIT) & ~ALIGN_MASK_IN_BIT)


/* this struct holds the actual data of
 * the bitset.
 */
typedef struct {
	WORD *m_bc;
	size_t m_nbc;
} bitset;

// set a single bit at index
static inline void bs_set(bitset *bs, size_t index) {
	bs->m_bc[index / WORD_SIZE_IN_BIT] |= (1LL << (WORD_SIZE_IN_BIT - 1 - index % WORD_SIZE_IN_BIT));
}

// clear a single bit at index
static inline void bs_clear(bitset *bs, size_t index) {
	bs->m_bc[index / WORD_SIZE_IN_BIT] &= ~(1LL << (WORD_SIZE_IN_BIT - 1 - index % WORD_SIZE_IN_BIT));
}

// in Core i7 processors the LZCNT instruction
// is implemented using BSR, thus its the index
// of the last set bit.
static inline size_t bs_lzcnt(WORD bchk) {
	return bchk ? WORD_SIZE_IN_BIT - 1 - __builtin_clzll(bchk) : WORD_SIZE_IN_BIT;
}

// in Core i7 processors this intrinsic is translated to
// BSF, thus gives the first index of a set bit.
static inline size_t bs_tzcnt(WORD bchk) {
	return bchk ? __builtin_ctzll(bchk) : WORD_SIZE_IN_BIT;
}

// get num of zero in a chunk
static inline size_t bs_zerocnt(WORD bchk) {
	return WORD_SIZE_IN_BIT - __builtin_popcountll(bchk);
}

// check whether the bit at index is set
static inline bool bs_check_set(bitset *bs, size_t index) {
	return (bool) (bs->m_bc[index / WORD_SIZE_IN_BIT] & (1LL << (WORD_SIZE_IN_BIT - index % WORD_SIZE_IN_BIT - 1)));
}

void bs_init(bitset *, size_t, void *);
size_t bs_nrun(bitset *, size_t);
size_t bs_nrun_from(bitset *, size_t, size_t);
void bs_set_range(bitset *, size_t, size_t);
void bs_clear_range(bitset *, size_t, size_t);
size_t bs_next_zero(bitset *, size_t);
size_t bs_next_one(bitset *, size_t);
int bs_chk_scann(WORD, size_t);
int bs_contain_ones(bitset *, size_t, size_t);

void bs_dump(WORD, int);

#endif //MALLOC_BITSET_H
