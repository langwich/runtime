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
#include "byteset.h"

/*
 * Initialize a byset used to represent n words.
 * This byset will borrow the first n bytes from the heap.
 */
void byset_init(byset *pbys, size_t n, void *pheap) {
	pbys->bytes = (char *) pheap;
	pbys->num_words = n;
	// unlike bitmap, we need to initialize byte board to '0'
	memset(pheap, '0', n);
	// set first n / WORD_SIZE bytes to 1, since this is
	// occupied by the byte score board.
	byset_set1(pbys, 0, n / WORD_SIZE - 1);
}

/*
 * Since the pattern is really simple, a naive string matcher will suffice.
 * The match time is O(n) (n is the length of the search base.
 * This method traverses each byte, which is extremely slow.
 */
static size_t naive_string_matcher(const char *s, size_t s_len, const char p, size_t n) {
	size_t remain = n;
	size_t mark = NOT_FOUND;
	for (size_t i  = 0; i < s_len; ++i) {
		if (s[i] == p) {
			mark = (mark == NOT_FOUND) ? i : mark;
			if (--remain == 0) return mark;
		}
		else {
			remain = n;
			mark = NOT_FOUND;
		}
	}
	return NOT_FOUND;
}

/*
 * Get "n-run" of character '0' in the byte score board.
 * Return the word offset of the start address in the
 * heap.
 */
size_t byset_nrun(byset *pbys, size_t n) {
	size_t offset;
#ifdef NAIVE
	offset = naive_string_matcher(pbys->bytes, pbys->num_words, '0', n);
#endif
	if (offset != NOT_FOUND) byset_set1(pbys, offset, offset + n - 1);
	return offset;
}

int byset_contain_ones(byset *bys, size_t lo, size_t hi) {
	for (size_t i = lo; i <= hi; ++i) {
		if (bys->bytes[i] != '1') return 0;
	}
	return 1;
}
