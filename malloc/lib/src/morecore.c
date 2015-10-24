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
#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

/*
Useful resource has been Doug Lea's malloc(): http://g.oswego.edu/pub/misc/malloc-2.6.6.c

This uses UNIX mmap() to get more core from the OS. It's not portable. E.g., A comment for Windows,
support says to use CreateFile():

	https://msdn.microsoft.com/en-us/library/windows/desktop/aa366556(v=vs.85).aspx

Doug Lea's allocator has a wsbrk() function to emulate UNIX sbrk(), which could be a better
way.
*/

/* Allocate size_in_bytes memory (not mapping a file to this memory...MAP_ANON).
 * This routine is independent of any headers needed by malloc() implementations.
 * Add any extra memory needed.  This is intended for educational purposes to
 * ask the OS for a heap of memory for malloc() and GC implementations.
 */
void *morecore(size_t size_in_bytes) {
	void *addr = 0;
	const int filedescriptor = -1; // not mapping a file to memory
	const int offset = 0;
	void *p = mmap(addr, size_in_bytes, PROT_READ|PROT_WRITE,
				   MAP_PRIVATE|MAP_ANON, filedescriptor, offset);
	if ( p == MAP_FAILED ) return NULL;
	return p;
}

/* Note: man page says size_in_bytes must be multiple of PAGESIZE but it seems to work
 * with non page sizes.
 */
void dropcore(void *p, size_t size_in_bytes) {
	if ( p!=NULL ) {
		int ret = munmap(p, size_in_bytes);
		assert(ret == 0); // munmap returns non-zero on failure
	}
}

void *calloc(size_t num, size_t size) { // calls our malloc if we link properly
	size_t n = num * size * sizeof(char);
	void *p = malloc(n);
	memset(p, 0, n);
	return p;
}
