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

#include <morecore.h>
#include <byteset.h>
#include "bytemap.h"

static byset g_bys;
static void *g_pheap;
static size_t g_heap_size;

/*
 * The naive implementation took 3968s to finish
 * the simulation trace.
 */


/*
 * Allocate size bytes from the arena. Size will
 * be rounded up to word boundary.
 *
 * Algorithm: Find consecutive size / WORD_SIZE_IN_BYTE
 * bytes that are '0' from the byte score board.
 * Use the start address of this n-run of '0' as
 * the offset and return the start address of the
 * empty unallocated memory.
 */
void *malloc(size_t size) {
	if ( g_pheap==NULL ) { heap_init(DEFAULT_MAX_HEAP_SIZE); }

	// number of words to satisfy request
	size_t n = ALIGN_WORD_BOUNDARY(size);

	size_t offset;
	size_t num_bytes = n / WORD_SIZE_IN_BYTE + 1;
	// plus one here for the extra word used for boundary tag.
	if ((offset = byset_nrun(&g_bys, num_bytes)) == NOT_FOUND) return NULL;
	void *ptr = WORD(g_pheap) + offset;
	U32 *boundary = (U32 *)ptr;
	boundary[0] = BOUNDARY_TAG;
	boundary[1] = (U32) num_bytes;
	return WORD(ptr) + 1;
}

void free(void *ptr) {
	if (ptr == NULL) return;
	U32 *boundary = (U32 *) (WORD(ptr) - 1);
	if (BOUNDARY_TAG != boundary[0]) {
#ifdef DEBUG
		fprintf(stderr, "boundary tag corrupted for address %p, have you freed it before?\n", ptr);
#endif
		// returning here in case user are not freeing correctly.
		return;
	}
	size_t start_index = WORD(ptr) - 1 - WORD(g_pheap);
	byset_set0(&g_bys, start_index, start_index + boundary[1] - 1);
	// remove boundary tag
	boundary[0] = 0;
}

void heap_init(size_t size) {
	if ( g_pheap!=NULL ) bytemap_release();
	g_pheap = morecore(size);
	g_heap_size = size;
	byset_init(&g_bys, g_heap_size / WORD_SIZE_IN_BYTE, g_pheap);
}

void bytemap_release() {
	dropcore(g_pheap, g_heap_size);
}

#ifdef DEBUG
void *bytemap_get_heap() {
	return g_pheap;
}
#endif

#ifdef DEBUG
int verify_byte_score_board() {
	for (size_t byte_index = 0; byte_index < g_bys.num_words; ++byte_index) {
		// boundary tag
		U32 tag = ((U32 *)(&WORD(g_pheap)[byte_index]))[0];
		if (tag == BOUNDARY_TAG) {
			U32 len = ((U32 *)(&WORD(g_pheap)[byte_index]))[1];
			size_t end_index = byte_index + len - 1;
			if (!byset_contain_ones(&g_bys, byte_index, end_index)) {
				fprintf(stderr, "verification failed, bitmap is in wrong status.\n");
				return 0;
			}
		}
	}
	return 1;
}
#endif

