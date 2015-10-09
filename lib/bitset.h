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

typedef unsigned long long      BITCHUNK;// one full chunk covers 512 bytes in the heap.
typedef unsigned char           U1;
typedef __uint32_t              U32;

#define WORD(x)                 ((unsigned long long *)x)
#define BITSET_NON              ((BITCHUNK) ~0x0)// we can never get that much memory
#define BIT_NUM                 8
#define WORD_SIZE               (sizeof(void *))
#define ALIGN_MASK              (WORD_SIZE - 1)
#define CHUNK_SIZE              (sizeof(BITCHUNK))// usually it's the same as WORD_SIZE on 64-bit machines.
#define CHK_IN_BIT              (CHUNK_SIZE * BIT_NUM)
#define BC_ONE                  0xFFFFFFFFFFFFFFFF
#define BC_LEFTMOST_MASK        0x8000000000000000

#define ALIGN_WORD_BOUNDARY(n)  ((n & ALIGN_MASK) == 0 ? n : (n + WORD_SIZE) & ~ALIGN_MASK)

/*
 * This table returns the bit index of n consecutive
 * zeros in a byte of the bitmap, if this position
 * doesn't exist, -1 is returned.
 *
 * e.g. For ff_lup[128][n], the table gives
 * 1 for n in [0,7) meaning in 128 (10000000) the first
 * index of N consecutive bits is 1 for N in [1,8) and
 * apparently we cannot get 8 consecutive 0s in 128.
 */
#define LUP_ROW 256
#define LUP_COL 8
static int ff_lup[LUP_ROW][LUP_COL];
static int lz_lup[LUP_ROW];
static int tz_lup[LUP_ROW];

/*
 * the initial masks with n leading 0s (from left).
 */
static unsigned char n_lz_mask[LUP_COL] = {0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00};
static unsigned char n_tz_mask[LUP_COL] = {0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};
static BITCHUNK right_masks[65];
static BITCHUNK left_masks[65];

/* this struct holds the actual data of
 * the bitset.
 */
typedef struct {
	BITCHUNK *m_bc;
	size_t m_nbc;
} bitset;

void bs_init(bitset *, size_t, void *);
size_t bs_nrun(bitset *, size_t);
int bs_set1(bitset *, size_t, size_t);
int bs_set0(bitset *, size_t, size_t);
int bs_chk_scann(BITCHUNK, size_t);
int bs_contain_ones(bitset *, size_t, size_t);

void bs_dump(BITCHUNK, int);

#endif //MALLOC_BITSET_H
