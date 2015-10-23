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

#include <string.h>
#include <cunit.h>
#include "bitmap.h"

#define HEAP_SIZE           4096

static WORD *g_pbsm;
static WORD *g_pbsa;
static void *g_pheap;

static void setup() {
	g_pheap = bitmap_get_heap();
	g_pbsm = (WORD *) g_pheap;
	g_pbsa = &(g_pbsm[HEAP_SIZE / WORD_SIZE_IN_BIT / WORD_SIZE_IN_BYTE]);
}

static void teardown() {
//	assert_equal(verify_bit_score_board(), 1);
	bitmap_release();
}

void test_bitmap_init() {
	assert_equal(0xFFFF000000000000, *g_pbsm);
	assert_equal(0x8000000000000000, *g_pbsa);
}

void test_bitmap_malloc() {
	void *addr10 = malloc(10);
	// check bit board
	assert_equal(0xFFFFC00000000000, *g_pbsm);
	// check address
	assert_addr_equal(WORD(g_pheap) + 16, addr10);
	// check associated bit board
	assert_equal(0x8000800000000000, *g_pbsa);

	void *addr20 = malloc(20);
	assert_equal(0xFFFFF80000000000, *g_pbsm);
	assert_addr_equal(WORD(g_pheap) + 18, addr20);
	assert_equal(0x8000A00000000000, *g_pbsa);

	void *addr344 = malloc(344);
	assert_equal(0xFFFFFFFFFFFFFFFF, *g_pbsm);
	assert_addr_equal(WORD(g_pheap) + 21, addr344);
	assert_equal(0x8000A40000000000, *g_pbsa);

	// start a new chunk
	void *addr23 = malloc(23);
	assert_equal(0xE000000000000000, g_pbsm[1]);
	assert_addr_equal(WORD(g_pheap) + 64, addr23);
	assert_equal(0x8000000000000000, g_pbsa[1]);

	void *addr30 = malloc(30);
	assert_equal(0xFE00000000000000, g_pbsm[1]);
	assert_addr_equal(WORD(g_pheap) + 67, addr30);
	assert_equal(0x9000000000000000, g_pbsa[1]);
}

void test_bitmap_malloc_free() {
	//////////
	void *addr20 = malloc(20);
	// check bit board
	assert_equal(0xFFFFE00000000000, *g_pbsm);
	// check address
	assert_addr_equal(WORD(g_pheap) + 16, addr20);
	// check associated bit board
	assert_equal(0x8000800000000000, *g_pbsa);

	//////////
	void *addr100 = malloc(100);
	assert_equal(0xFFFFFFFF00000000, *g_pbsm);
	assert_addr_equal(WORD(g_pheap) + 19, addr100);
	assert_equal(0x8000900000000000, *g_pbsa);

	// test unsatisfiable request
	assert_equal(NULL, malloc(4096));
	///////////
	// freeing
	free(addr20);
	assert_equal(0xFFFF1FFF00000000, *g_pbsm);
	assert_equal(0x8000100000000000, *g_pbsa);

	free(addr100);
	assert_equal(0xFFFF000000000000, *g_pbsm);
	assert_equal(0x8000000000000000, *g_pbsa);


	// acquiring all memory
	void *addrAll = malloc(HEAP_SIZE - 16 * 8);
	assert_addr_equal(addrAll, WORD(g_pheap) + 16);
	// all bits should be turned on.
	for (int i = 0; i < 8; ++i) assert_equal(~0x0, g_pbsm[i]);
	// check boundary
	assert_equal(0x8000800000000000, *g_pbsa);
}

int main(int argc, char *argv[]) {
	cunit_setup = setup;
	cunit_teardown = teardown;

	test(test_bitmap_init);
	test(test_bitmap_malloc);
	test(test_bitmap_malloc_free);

	return 0;
}