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
#include "cunit.h"
#include "byteset.h"

#define HEAP_SIZE           4096
#define NUM_WORDS           HEAP_SIZE/WORD_SIZE_IN_BYTE

static char g_heap[HEAP_SIZE];

static void setup() {
	memset(g_heap, '0', HEAP_SIZE);
}

static void teardown() { }

void test_bys_init() {
	byset bs;
	byset_init(&bs, NUM_WORDS, g_heap);
	char expected[65] = {0};
	memset(expected, '1', 64);
	assert_strn_equal(expected, bs.bytes, 64);
}

void test_bys_set1() {
	byset bs;
	byset_init(&bs, NUM_WORDS, g_heap);
	byset_set1(&bs, 66, 193);// 128 bytes
	char expected[129] = {0};
	memset(expected , '0', 2);
	memset(expected + 2, '1', 126);
	assert_strn_equal(expected, bs.bytes + 64, 128);
}

void test_bys_set0() {
	byset bs;
	byset_init(&bs, NUM_WORDS, g_heap);
	byset_set1(&bs, 66, 193);// 128 bytes
	byset_set0(&bs, 100, 121);// 22 bytes
	char expected[129] = {0};
	memset(expected, '0', 2);
	memset(expected + 2, '1', 126);
	memset(expected + 36, '0', 22);
	assert_strn_equal(expected, bs.bytes + 64, 128);
}

void test_bys_nrun() {
	byset bs;
	byset_init(&bs, NUM_WORDS, g_heap);

	size_t index_not_found = byset_nrun(&bs, NUM_WORDS + 1);
	assert_equal(NOT_FOUND, index_not_found);
	size_t index256 = byset_nrun(&bs, 256);
	assert_equal(64, index256);

	byset_set0(&bs, 128, 255);
	char expected[320];
	memset(expected, '1', sizeof(expected));
	memset(expected + 128, '0', 128);
	assert_strn_equal(expected, bs.bytes, sizeof(expected));

	bs.bytes[255] = '1';
	size_t index128 = byset_nrun(&bs, 128);
	assert_equal(320, index128);
	size_t index127 = byset_nrun(&bs, 127);
	assert_equal(128, index127);
	memset(expected, '1', sizeof(expected));
	assert_strn_equal(expected, bs.bytes, sizeof(expected));
}

int main(int argc, char *argv[]) {
	cunit_setup = setup;
	cunit_teardown = teardown;

	test(test_bys_init);
	test(test_bys_set1);
	test(test_bys_set0);
	test(test_bys_nrun);

	return 0;
}