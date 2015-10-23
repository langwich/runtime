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

/*
 * 60s - 64s with -mpopcnt -mlzcnt turned on similar result without
 * those intrinsics. (that's for the old boundary tag implementation)
 *
 * 270s - 300s for the two-bitmap implementation.
 *
 * 736s after using the last freed chunk heuristic.
 */

#include <stddef.h>
#include <morecore.h>

#include <bitset.h>

#include "bitmap.h"

static void *g_pheap;
static size_t g_heap_size;

static bitset g_bsm;// bit set used to keep track of heap usage.
static bitset g_bsa;// associated bit set to mark the boundaries.

static size_t last_free_index = 0;

/*
 * Current implementation is really straightforward. We don't
 * dynamically adjust the arena size.
 */
void heap_init(size_t size) {
	if ( g_pheap!=NULL ) bitmap_release();
	g_pheap = morecore(size);
	g_heap_size = size;

	size_t num_chk = size / (WORD_SIZE_IN_BIT * WORD_SIZE_IN_BYTE);
	bs_init(&g_bsm, num_chk, g_pheap);
	// set the first few bits to 1.
	// Those bits count for the space consumed by the bit score board.
	bs_set_range(&g_bsm, 0, num_chk - 1);

	// the associated bit board starts right after the main one
	// to improve locality.
	bs_set_range(&g_bsm, num_chk, 2 * num_chk - 1);
	bs_init(&g_bsa, num_chk, ((WORD *)g_pheap) + num_chk);
	bs_set(&g_bsa, 0);
}

void bitmap_release() {
	dropcore(g_pheap, g_heap_size);
}

/*
 * This function is used to return the start address of required
 * amount of heap/mapped memory.
 * The size is round up to the word boundary.
 * NULL is returned when there is not enough memory.
 */
void *malloc(size_t size)
{
	if ( g_pheap==NULL ) { heap_init(DEFAULT_MAX_HEAP_SIZE); }
	size_t n = ALIGN_WORD_BOUNDARY(size);

	size_t run_index = 0;
	if ((run_index = bs_nrun_from(&g_bsm, n / WORD_SIZE_IN_BYTE, last_free_index)) == BITSET_NON) return NULL;
	void *ptr = WORD(g_pheap) + run_index;
	// make sure the we set the correct boundary
	bs_set(&g_bsa, run_index);

	return ptr;
}

/*
 * This function returns the memory to arena and unmarks the
 * related bits in bitset.
 */
void free(void *ptr)
{
	if (ptr == NULL) return;

	size_t offset = WORD(ptr) - WORD(g_pheap);
	if (!bs_check_set(&g_bsa, offset)) {
#ifdef DEBUG
		fprintf(stderr, "boundary bit not set for address %p\n", ptr);
#endif
		// returning here to avoid corruption
		return;
	}

	// find the size of consecutive ones
	size_t next_set = bs_next_one(&g_bsa, offset + 1);
	size_t size;
	if (next_set == BITSET_NON) {
		// if nothing found in the associative bitmap
		// then there is only one chunk in the main bitmap.
		size_t next_unset = bs_next_zero(&g_bsm, offset + 1);
		// the true answer means the allocated memory goes to the end.
		size = (next_unset == BITSET_NON) ? g_bsa.m_nbc * WORD_SIZE_IN_BIT - offset : next_unset - offset;
	}
	else {
		size = next_set - offset;
	}

	// clear the bits in two bitmaps
	bs_clear_range(&g_bsm, offset, offset + size - 1);
	bs_clear(&g_bsa, offset);

	last_free_index = offset;
}

#ifdef DEBUG
void *bitmap_get_heap() {
	return g_pheap;
}
#endif

bool check_bitmap_consistency(){
	// 110000011111100
	// 100000010000000
	for (size_t i = bs_next_one(&g_bsa, 0); i != BITSET_NON; i = bs_next_one(&g_bsa, i + 1)) {
		if (!bs_check_set(&g_bsa, i)) {
#ifdef DEBUG
			fprintf(stderr, "The bitmap is in an inconsistent state.");
#endif
			return false;
		}
	}
	return true;
}
