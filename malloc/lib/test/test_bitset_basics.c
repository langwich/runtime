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
#include <bitset.h>
#include "cunit.h"

#define HEAP_SIZE           4096

static char g_heap[HEAP_SIZE];

static void setup() {
	memset(g_heap, 0, HEAP_SIZE);
}

static void teardown() { }

void test_bs_init() {
	bitset bs;
	bs_init(&bs, 2, g_heap);
	assert_equal(bs.m_bc[0], 0xC000000000000000);
	assert_equal(bs.m_bc[1], 0x0);
}

void test_bs_set1() {
	bitset bs;
	bs_init(&bs, 2, g_heap);
	bs_set_range(&bs, 23, 80);
	assert_equal(bs.m_bc[0], 0xC00001FFFFFFFFFF);
	assert_equal(bs.m_bc[1], 0xFFFF800000000000);
}

void test_bs_set1_left_boundary() {
	bitset bs;
	bs_init(&bs, 2, g_heap);
	bs_set_range(&bs, 64, 80);
	assert_equal(bs.m_bc[0], 0xC000000000000000);
	assert_equal(bs.m_bc[1], 0xFFFF800000000000);
}

void test_bs_set1_right_boundary_hi() {
	bitset bs;
	bs_init(&bs, 2, g_heap);
	bs_set_range(&bs, 23, 63);
	assert_equal(bs.m_bc[0], 0xC00001FFFFFFFFFF);
	assert_equal(bs.m_bc[1], 0x0);
}

void test_bs_set1_right_boundary_lo() {
	bitset bs;
	bs_init(&bs, 2, g_heap);
	bs_set_range(&bs, 63, 77);
	assert_equal(bs.m_bc[0], 0xC000000000000001);
	assert_equal(bs.m_bc[1], 0xFFFC000000000000);
}

void test_bs_set1_same_chk() {
	bitset bs;
	bs_init(&bs, 2, g_heap);
	assert_equal(bs.m_bc[0], 0xC000000000000000);
	bs_set_range(&bs, 2, 3);
	assert_equal(bs.m_bc[0], 0xF000000000000000);
}

void test_bs_set1_same_chk_middle() {
	bitset bs;
	bs_init(&bs, 2, g_heap);
	assert_equal(bs.m_bc[0], 0xC000000000000000);
	bs_set_range(&bs, 23, 33);
	assert_equal(bs.m_bc[0], 0xC00001FFC0000000);
	assert_equal(bs.m_bc[1], 0x0);
}

void test_bs_set0() {
	bitset bs;
	bs_init(&bs, 2, g_heap);
	bs_set_range(&bs, 23, 80);
	bs_clear_range(&bs, 55, 77);
	assert_equal(bs.m_bc[0], 0xC00001FFFFFFFE00);
	assert_equal(bs.m_bc[1], 0x0003800000000000);
}

void test_bs_set0_left_boundary() {
	bitset bs;
	bs_init(&bs, 2, g_heap);
	bs_set_range(&bs, 23, 80);
	bs_clear_range(&bs, 64, 77);
	assert_equal(bs.m_bc[0], 0xC00001FFFFFFFFFF);
	assert_equal(bs.m_bc[1], 0x0003800000000000);
}

void test_bs_set0_right_boundary_hi() {
	bitset bs;
	bs_init(&bs, 2, g_heap);
	bs_set_range(&bs, 23, 80);
	bs_clear_range(&bs, 44, 63);
	assert_equal(bs.m_bc[0], 0xC00001FFFFF00000);
	assert_equal(bs.m_bc[1], 0xFFFF800000000000);
}

void test_bs_set0_right_boundary_lo() {
	bitset bs;
	bs_init(&bs, 2, g_heap);
	bs_set_range(&bs, 23, 80);
	bs_clear_range(&bs, 63, 77);
	assert_equal(bs.m_bc[0], 0xC00001FFFFFFFFFE);
	assert_equal(bs.m_bc[1], 0x0003800000000000);
}

void test_bs_set0_same_chk() {
	bitset bs;
	bs_init(&bs, 2, g_heap);
	bs_set_range(&bs, 23, 63);//  0xC00001FFFFFFFFFF
	bs_clear_range(&bs, 24, 33);
	assert_equal(bs.m_bc[0], 0xC00001003FFFFFFF);
	assert_equal(bs.m_bc[1], 0x0);
}

void test_bs_chk_scann() {
	WORD bchk = 0xFFFFF1FFFFFFF011;
	int index7 = bs_chk_scann(bchk, 7);
	int index4 = bs_chk_scann(bchk, 4);
	int index3 = bs_chk_scann(bchk, 3);
	int index10 = bs_chk_scann(bchk, 10);
	assert_equal(52, index7);
	assert_equal(52, index4);
	assert_equal(20, index3);
	assert_equal(-1, index10);
}

void test_bs_chk_scann_left_bdry() {
	WORD bchk = 0x0100000000000000;
	int index7 = bs_chk_scann(bchk, 7);
	assert_equal(0, index7);
}

void test_bs_chk_scann_right_bdry() {
	WORD bchk = 0xFFFFFFFFFFFFFF80;
	int index7 = bs_chk_scann(bchk, 7);
	assert_equal(57, index7);
}

void test_bs_nrun() {
	bitset bs;
	// initialization
	bs_init(&bs, 2, g_heap);
	assert_equal(bs.m_bc[0], 0xC000000000000000);
	// try acquiring the 3rd and 4th bits.
	size_t index2 = bs_nrun(&bs, 2);
	assert_equal(2, index2);
	assert_equal(bs.m_bc[0], 0xF000000000000000);
	// try acquiring next 12 bits.
	size_t index12 = bs_nrun(&bs, 12);
	assert_equal(4, index12);
	assert_equal(bs.m_bc[0], 0xFFFF000000000000);//16 1s
	// setting the last 33 bits of the first chunk
	// to 1 such that we can get the next trunk for allocation
	// of 16 bits. there are only 15 bits left for the first
	// chunk now.
	bs_set_range(&bs, 31, 63);
	size_t index16 = bs_nrun(&bs, 16);
	assert_equal(64, index16);
	assert_equal(bs.m_bc[1], 0xFFFF000000000000);
	// get the 15 bits left in the first chunk.
	size_t index15 = bs_nrun(&bs, 15);
	assert_equal(16, index15);
	assert_equal(bs.m_bc[0], 0xFFFFFFFFFFFFFFFF);
	assert_equal(bs.m_bc[1], 0xFFFF000000000000);
	// trying leading mode here
	bs_clear_range(&bs, 50, 70);// set a 0 "island" cross two chunks
	assert_equal(bs.m_bc[0], 0xFFFFFFFFFFFFC000);
	assert_equal(bs.m_bc[1], 0x01FF000000000000);
	size_t index19 = bs_nrun(&bs, 19);
	assert_equal(50, index19);
	assert_equal(bs.m_bc[0], 0xFFFFFFFFFFFFFFFF);
	assert_equal(bs.m_bc[1], 0xF9FF000000000000);
}

void test_bs_nrun_long() {
	bitset bs;
	// 8 chks right covers 4096 bytes (our heap size).
	bs_init(&bs, 8, g_heap);
	// we need one full byte to cover 8 chunks
	assert_equal(bs.m_bc[0], 0xFF00000000000000);

	size_t index375 = bs_nrun(&bs, 375);
	assert_equal(8, index375);
	for (int i = 0; i < 5; ++i) {
		// all chunks should be occupied right now.
		assert_equal(bs.m_bc[i], ~0x0);
	}
	assert_equal(bs.m_bc[5], 0xFFFFFFFFFFFFFFFE);
	// now the last two chunks with one extra bit should be 1s
	bs_set_range(&bs, 383, 511);
	bs_clear_range(&bs, 8, 382);
	for (int i = 1; i < 5; ++i) {
		assert_equal(bs.m_bc[i], 0x0);
	}
	assert_equal(bs.m_bc[0], 0xFF00000000000000);
	assert_equal(bs.m_bc[5], 0x0000000000000001);
	assert_equal(bs.m_bc[6], 0xFFFFFFFFFFFFFFFF);
	assert_equal(bs.m_bc[7], 0xFFFFFFFFFFFFFFFF);

	// trying to get a chunk that is longer than available
	size_t index376 = bs_nrun(&bs, 376);
	// nothing should change.
	for (int i = 1; i < 5; ++i) {
		assert_equal(bs.m_bc[i], 0x0);
	}
	assert_equal(bs.m_bc[0], 0xFF00000000000000);
	assert_equal(bs.m_bc[5], 0x0000000000000001);
	assert_equal(bs.m_bc[6], ~0x0);
	assert_equal(bs.m_bc[7], ~0x0);
	// returns BITSET NON since we only have 375 bits available
	assert_equal(BITSET_NON, index376);

	size_t index374 = bs_nrun(&bs, 374);
	// now we should only have one bit available.
	assert_equal(8, index374);
	for (int i = 0; i < 5; ++i) {
		assert_equal(bs.m_bc[i], ~0x0);
	}
	assert_equal(bs.m_bc[5], 0xFFFFFFFFFFFFFFFD);
	assert_equal(bs.m_bc[6], ~0x0);
	assert_equal(bs.m_bc[7], ~0x0);
}

int main(int argc, char *argv[]) {
	cunit_setup = setup;
	cunit_teardown = teardown;

	test(test_bs_init);
	test(test_bs_set1);
	test(test_bs_set1_left_boundary);
	test(test_bs_set1_right_boundary_hi);
	test(test_bs_set1_right_boundary_lo);
	test(test_bs_set1_same_chk);
	test(test_bs_set1_same_chk_middle);
	test(test_bs_set0);
	test(test_bs_set0_left_boundary);
	test(test_bs_set0_right_boundary_hi);
	test(test_bs_set0_right_boundary_lo);
	test(test_bs_set0_same_chk);

	test(test_bs_chk_scann);
	test(test_bs_chk_scann_left_bdry);
	test(test_bs_chk_scann_right_bdry);

	test(test_bs_nrun);
	test(test_bs_nrun_long);

	return 0;
}