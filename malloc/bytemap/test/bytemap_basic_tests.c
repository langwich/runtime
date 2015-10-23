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
#include <byteset.h>
#include <stdio.h>
#include "bytemap.h"

#define HEAP_SIZE           4096


static char test_buf[HEAP_SIZE / WORD_SIZE_IN_BYTE];
static void *g_pheap;

static void setup() {
	g_pheap = bytemap_get_heap();
	memset(test_buf, '0', sizeof(test_buf));
}

static void teardown() {
	assert_equal(verify_byte_score_board(), 1);
	bytemap_release();
}

void test_bytemap_init() {
	// check byte score board after initialization
	memset(test_buf, '1', 64);
	assert_strn_equal(test_buf, g_pheap, sizeof(test_buf));
}

void test_bytemap_malloc() {
	// sync test_buf with byte score board after initialization
	memset(test_buf, '1', 64);

	// try assigning 20 bytes (24 + 8 bytes really)
	void *addr20 = malloc(20);
	assert_addr_equal(WORD(g_pheap) + 65, addr20);
	memset(test_buf + 64, '1', 4);
	assert_strn_equal(test_buf, g_pheap, sizeof(test_buf));

	// try assigning one byte more than the space available.
	void *addr_null = malloc(HEAP_SIZE - HEAP_SIZE / WORD_SIZE_IN_BYTE - 20 + 1);
	assert_equal(addr_null, NULL);
	assert_strn_equal(test_buf, g_pheap, sizeof(test_buf));

	// try assigning 1001 bytes (1008 + 8 bytes really)
	void *addr1001 = malloc(1001);
	memset(test_buf + 68, '1', 127);
	assert_strn_equal(test_buf, g_pheap, sizeof(test_buf));
	assert_addr_equal(WORD(g_pheap) + 69, addr1001);

	// try get all remaining bytes
	// 32, 1016, for the previous allocations.
	// 512 for the byte score board, and 8 for the boundary tag.
	void *add_remain = malloc(HEAP_SIZE - 32 - 1016 - 512 - 8);
	memset(test_buf, '1', sizeof(test_buf));
	assert_strn_equal(test_buf, g_pheap, sizeof(test_buf));
	assert_addr_equal(WORD(g_pheap) + 196, add_remain);
}

void test_bytemap_malloc_free() {
	// sync test_buf with byte score board after initialization
	memset(test_buf, '1', 64);

	// try assigning 20 bytes (24 + 8 bytes really)
	void *addr20 = malloc(20);
	assert_addr_equal(WORD(g_pheap) + 65, addr20);
	memset(test_buf + 64, '1', 4);
	assert_strn_equal(test_buf, g_pheap, sizeof(test_buf));

	// try assigning 1001 bytes (1008 + 8 bytes really)
	void *addr1001 = malloc(1001);
	memset(test_buf + 68, '1', 127);
	assert_strn_equal(test_buf, g_pheap, sizeof(test_buf));
	assert_addr_equal(WORD(g_pheap) + 69, addr1001);

	// freeing the first chunk
	free(addr20);
	memset(test_buf + 64, '0', 4);
	assert_strn_equal(test_buf, g_pheap, sizeof(test_buf));

	// try get 7 bytes (8 + 8 bytes really)
	void *addr7 = malloc(7);
	memset(test_buf + 64, '1', 2);
	assert_strn_equal(test_buf, g_pheap, sizeof(test_buf));
	assert_addr_equal(WORD(g_pheap) + 65, addr7);

	// try get all remaining bytes
	// 32, 1016, for the previous allocations.
	// 512 for the byte score board, and 8 for the boundary tag.
	void *addr_remain = malloc(HEAP_SIZE - 32 - 1016 - 512 - 8);
	memset(test_buf, '1', sizeof(test_buf));
	memset(test_buf + 66, '0', 2);
	assert_strn_equal(test_buf, g_pheap, sizeof(test_buf));
	assert_addr_equal(WORD(g_pheap) + 196, addr_remain);

	// free all left chunks
	free(addr7);
	free(addr_remain);
	free(addr1001);
	memset(test_buf, '0', sizeof(test_buf));
	memset(test_buf, '1', 64);
	assert_strn_equal(test_buf, g_pheap, sizeof(test_buf));
}

int main(int argc, char *argv[]) {
	cunit_setup = setup;
	cunit_teardown = teardown;

	test(test_bytemap_init);
	test(test_bytemap_malloc);
	test(test_bytemap_malloc_free);

	return 0;
}