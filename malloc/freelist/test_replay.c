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
#include <time.h>
#include "freelist.h"
#include "cunit.h"
#include "replay.h"

const size_t HEAP_SIZE = 1000000000; // try 1G

extern void freelist_init(uint32_t max_heap_size);
extern void freelist_shutdown();

Heap_Info verify_heap() {
	Heap_Info info = get_heap_info();
	assert_equal(info.heap_size, HEAP_SIZE);
	assert_equal(info.heap_size, info.busy_size+info.free_size);
	return info;
}

static void setup()		{ freelist_init(HEAP_SIZE); }
static void teardown()	{ verify_heap(); freelist_shutdown(); }

void replay_ansic_grammar_with_dparser() {
	int result = replay_malloc("/tmp/trace.txt");
	assert_equal(0, result);
}

int main(int argc, char *argv[]) {
	// TODO: currently must run from cmd-line: no way to set working dir in cmake?
	printf("converting addresses to indexes\n");
	system("python ../cunit/addr2index.py < ../cunit/ANSIC_MALLOC_FREE_TRACE.txt > /tmp/trace.txt");
	printf("simulating...\n");

	cunit_setup = setup;
	cunit_teardown = teardown;

	freelist_init(HEAP_SIZE);

	{
		time_t start = time(NULL);
		test(replay_ansic_grammar_with_dparser);
		time_t stop = time(NULL);

		fprintf(stderr, "simulation took %ldms\n", (stop-start));
	}

	return 0;
}