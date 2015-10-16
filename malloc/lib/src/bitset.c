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

#include <stddef.h>
#include <unistd.h>
#include <tic.h>
#include "bitset.h"

/*
 * This table returns the bit index of n consecutive
 * zeros in a byte of the bitmap, if this position
 * doesn't exist, -1 is returned.
 *
 * e.g. For lup[128][n], the table gives
 * 1 for n in [0,7) meaning in 128 (10000000) the first
 * index of N consecutive bits is 1 for N in [1,8) and
 * apparently we cannot get 8 consecutive 0s in 128.
 */
#define LUP_ROW 256
#define LUP_COL 8
static int lup[LUP_ROW][LUP_COL];

/*
 * the initial masks with n leading 0s (from left).
 */
static unsigned char lz_mask[LUP_COL] = {0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00};
static unsigned char tz_mask[LUP_COL] = {0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};
static BITCHUNK right_masks[65];
static BITCHUNK left_masks[65];

static void init_lups();
static void init_masks();

/*
 * Initializes a bitset with n chunks.
 * See the header for the definition of
 * a chunk.
 */
void bs_init(bitset *bs, size_t nchks, void *pheap) {
	// initialize all kinds of masks.
	init_lups();
	init_masks();

	bs->m_nbc = nchks;
	bs->m_bc = pheap;
}

/*
 * Dump the bit chunk to fd. Mainly used for debug/test.
 */
void bs_dump(BITCHUNK bc, int fd) {
	for (BITCHUNK i = 1ULL << (CHK_IN_BIT - 1); i > 0; i = i / 2) {
		(bc & i)? write(fd, "1", 1): write(fd, "0", 1);
	}
	write(fd, "\n", 1);
}

/*
 * Returns the index of the first n-run of 0s in the bitset.
 * The index is 0-based. -1 is returned if no n-run of 0s are
 * found.
 *
 * Algorithm:
 * During the scan, the program behave in two modes: cross mode
 * and non-cross mode. During cross mode, we are looking for a
 * run of n consecutive 0s cross the word boundary. And in
 * non-cross mode we expect to get our result within the current
 * word.
 */

size_t bs_nrun(bitset *bs, size_t n) {
	return bs_nrun_from(bs, n, 0);
}


size_t bs_nrun_from(bitset *bs, size_t n, size_t start) {
	// although start_index starts from 0, but it should always
	// skip the first few bits set for the bit score board itself.
	size_t start_index = start;
	size_t end_index = start_index;
	// starts with non-cross mode
	int mode = 0;// 0, non-crossing, 1 leading, 2 trailing (1, 2 are cross modes)
	int one_chk_possible = n > CHK_IN_BIT ? 0 : 1;
	if (!one_chk_possible) mode = 2;

	// current chunk index.
	size_t start_chunk_index = start_index / CHK_IN_BIT;
	size_t cur_chunk_index = start_chunk_index;

	size_t remaining = n;
	size_t chunk_scanned = 0;
	while (chunk_scanned < bs->m_nbc) {
		BITCHUNK cur_bchk = bs->m_bc[cur_chunk_index];
		if (!remaining) break;
		if (1 == mode) {
			size_t lzcnt = bs_lzcnt(cur_bchk);
			if (lzcnt < remaining) {
				if (CHK_IN_BIT == lzcnt) {
					remaining -= lzcnt;
					end_index += lzcnt;
					cur_chunk_index++;
					chunk_scanned++;
				}
				else {
					// not enough consecutive bits, restart scan
					// in this chunk
					if (one_chk_possible) mode = 0;
					else mode = 2;
					remaining = n;
				}
			}
			else {
				end_index += remaining;
				break;
			}
		}
		else if (2 == mode){
			// trailing mode could only happen when we just
			// started the crossing mode.
			size_t tzcnt = bs_tzcnt(cur_bchk);
			if (tzcnt) {
				start_index = (cur_chunk_index + 1) * CHK_IN_BIT - tzcnt;
				end_index = start_index + tzcnt - 1;
				remaining -= tzcnt;
				mode = 1;
			}
			else if (one_chk_possible) {
				// give up on this pass, start over from non-trailing mode
				// if it can be fit into one chunk.
				mode = 0;
			}
			cur_chunk_index++;
			chunk_scanned++;
		}
		else {// non crossing mode
			int index = bs_chk_scann(cur_bchk, n);
			if (index >= 0) {
				start_index = cur_chunk_index * CHK_IN_BIT + index;
				end_index = start_index + n - 1;
				break;
			}
			// no contiguous chunk found, starting trailing mode
			mode = 2;
		}
		if (cur_chunk_index == bs->m_nbc && start_chunk_index) {
			start_index = end_index = 0;
			cur_chunk_index = 0;
			remaining = n;
		}
	}

	if (cur_chunk_index >= bs->m_nbc) return BITSET_NON;
	bs_set_range(bs, start_index, end_index);
	return start_index;
}

/*
 * Sets the bits in [lo,hi] to 1. lo is the less significant bit.
 * lo and hi are bit indices and are *0-BASED*
 */
void bs_set_range(bitset *bs, size_t lo, size_t hi) {
	size_t lo_chk = lo / CHK_IN_BIT;
	size_t hi_chk = hi / CHK_IN_BIT;
	if (lo_chk == hi_chk) {
		bs->m_bc[lo_chk] |= ~(right_masks[(lo_chk + 1) * CHK_IN_BIT - lo] ^
				left_masks[hi + 1 - hi_chk * CHK_IN_BIT]);
	}
	else {
		size_t next_chk = lo_chk + 1;
		while (next_chk < hi_chk) bs->m_bc[next_chk++] |= BC_ONE;
		bs->m_bc[lo_chk] |= right_masks[(lo_chk + 1) * CHK_IN_BIT - lo];
		bs->m_bc[hi_chk] |= left_masks[hi + 1 - hi_chk * CHK_IN_BIT];
	}
}

/*
 * Sets the bits in [lo,hi] to 0. lo is the less significant bit
 * lo and hi are bit indices and are *0-BASED*
 */
void bs_clear_range(bitset *bs, size_t lo, size_t hi) {
	size_t lo_chk = lo / CHK_IN_BIT;
	size_t hi_chk = hi / CHK_IN_BIT;
	if (lo_chk == hi_chk) {
		bs->m_bc[lo_chk] &= (right_masks[(lo_chk + 1) * CHK_IN_BIT - lo] ^
				left_masks[hi + 1 - hi_chk * CHK_IN_BIT]);
	}
	else {
		size_t next_chk = lo_chk + 1;
		while (next_chk < hi_chk) bs->m_bc[next_chk++] &= 0ULL;
		bs->m_bc[lo_chk] &= ~right_masks[(lo_chk + 1) * CHK_IN_BIT - lo];
		bs->m_bc[hi_chk] &= ~left_masks[hi + 1 - hi_chk * CHK_IN_BIT];
	}
}

/*
 * Finds the start index of contiguous n 0 bits in a BITCHUNK.
 * returns -1 if the BITCHUNK doesn't meet the requirement.
 */
int bs_chk_scann(BITCHUNK bchk, size_t n) {
	int i = 0;
	while (bs_lzcnt(bchk) < n) {
		bchk = (bchk << 1) | (BITCHUNK) 1;
		if (bs_zerocnt(bchk) < n || (i == (CHK_IN_BIT - n))) return -1;
		i++;
	}
	return i;
}

static void init_lups() {
	for (int i = 0; i < LUP_ROW; ++i) {
		for (int j = 0; j < LUP_COL; ++j) {
			lup[i][j] = -1;
		}
	}

	for (int i = 0; i < LUP_ROW; ++i) {
		for (int j = 0; j < LUP_COL; ++j) {
			U1 mask = lz_mask[j];
			for (int k = 0; k < BIT_NUM - j; ++k) {
				// stops when we got j+1 consecutive 0s
				// or we reach the end.
				if (((U1)i | mask) == mask) {
					lup[i][j] = k;
					break;
				}
				mask = (U1) ((1 << 7) | (mask >> 1));
			}
		}
	}
}

static void init_masks() {
	right_masks[0] = 0LL;
	for (int i = 1; i < 65; ++i) {
		right_masks[i] = (right_masks[i-1] << 1) | 1LL;
	}

	left_masks[0] = 0LL;
	for (int i = 1; i < 65; ++i) {
		left_masks[i] = (left_masks[i-1] >> 1) | BC_LEFTMOST_MASK;
	}
}

// get the next zero index starts from index (inclusive)
size_t bs_next_zero(bitset *bs, size_t index) {
	for (size_t i = index; i < bs->m_nbc * CHK_IN_BIT; i = ALIGN_CHUNK_BOUNDARY(i + 1)) {
		BITCHUNK chk = bs->m_bc[i / CHK_IN_BIT] | left_masks[i % CHK_IN_BIT];
		if (bs_zerocnt(chk)) {
			size_t offset = 0;
			while ((chk << offset) & BC_LEFTMOST_MASK) ++offset;
			return i & CHK_ALIGN_MASK ? (i / CHK_IN_BIT) * CHK_IN_BIT + offset : i + offset;
		}
	}
	return BITSET_NON;
}

// get the next zero index starts from index (inclusive)
size_t bs_next_one(bitset *bs, size_t index) {
	for (size_t i = index; i < bs->m_nbc * CHK_IN_BIT; i = ALIGN_CHUNK_BOUNDARY(i + 1)) {
		BITCHUNK chk = bs->m_bc[i / CHK_IN_BIT] & ~left_masks[i % CHK_IN_BIT];
		if (bs_zerocnt(chk) < CHK_IN_BIT) {
			size_t offset = 0;
			while (!((chk << offset) & BC_LEFTMOST_MASK)) ++offset;
			return i & CHK_ALIGN_MASK ? (i / CHK_IN_BIT) * CHK_IN_BIT + offset : i + offset;
		}
	}
	return BITSET_NON;
}
