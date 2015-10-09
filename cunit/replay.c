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
#include <stdlib.h>
#include <string.h>

/* Read in stuff like:
 *
	malloc 272 -> 437
	malloc 64 -> 438
	malloc 64 -> 439
	free 438

 * and make calls to malloc/free per the "instructions" to simulate
 * a program trace using a memory allocator.
 *
 * Link with your malloc/free and it should call them not the system lib.
 */

static const int MAX_INDEXES = 200000; // assume a large number of unique addresses for simplicity
static void *index2addr[MAX_INDEXES];
static int i = 0; // track which simulation instruction we're doing

/* Replay malloc/free instructions; return 0 if no problem, 1 otherwise */
int replay_malloc(char *filename) {
	FILE *f = fopen(filename, "r");
	if ( f==NULL ) {
		fprintf(stderr, "replay cannot find/open %s\n", filename);
		return 1;
	}
	char line[100];
	while ( fgets(line, sizeof(line), f) ) {
		line[strlen(line)-1] = '\0';
		if ( line[0]=='#' ) continue; // ignore comment lines starting with '#'
		printf("%d: %s", i, line);
		i++;
		if ( line[0]=='m' ) { // malloc size -> addrindex
			int size;
			int index;
			sscanf(line, "malloc %d -> %d\n", &size, &index);
			if ( index>=MAX_INDEXES ) {
				fprintf(stderr, "index size overrun (%d)\n", MAX_INDEXES);
				return 1;
			}
			void *p = malloc((size_t)size);
			printf(" (%p)\n", p);
			index2addr[index] = p;
		}
		else { // must be a free
			int index;
			sscanf(line, "free %d\n", &index);
			void *p = index2addr[index];
			printf(" (%p)\n", p);
			free(p);
			// I'm commenting out next line to allow simulation of erroneous streams such as:
			// free 99
			// free 99
			// free 99
			// i.e., free() should be able to detect free() of stale data
//			index2addr[index] = NULL;  // prevents extra free of this address from messing us up.
		}
	}
	fclose(f);
	return 0;
}
