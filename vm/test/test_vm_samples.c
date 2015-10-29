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
#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>
#include "vm.h"

#include <cunit.h>
#include <wloader.h>

static void setup()		{ }
static void teardown()	{ }

int main(int argc, char *argv[]) {
	cunit_setup = setup;
	cunit_teardown = teardown;
	char samplesdir[2000];
	char samplesfile[2000];

	char* wichruntime = getenv("WICHRUNTIME"); // set in intellij "test_vm_samples" environment variable config area
	if ( wichruntime==NULL ) {
		fprintf(stderr, "environment variable WICHRUNTIME not set to root of runtime area\n");
		return -1;
	}
	strcpy(samplesdir, wichruntime);
	strcat(samplesdir, "/vm/test/samples");

	struct dirent *dp;
	DIR *dir = opendir(samplesdir);
	if ( dir!=NULL ) {
		dp = readdir(dir);
		while (dp != NULL) {
			char *filename = dp->d_name;
			if ( strstr(filename, ".wasm")!=NULL ) {
				printf("loading %s\n", filename);
				strcpy(samplesfile, samplesdir);
				strcat(samplesfile, "/");
				strcat(samplesfile, filename);
				FILE *f = fopen(samplesfile, "r");
				VM *vm = vm_load(f);
				vm_exec(vm, true);
				fclose(f);
			}
			dp = readdir(dir);
		}
		closedir(dir);
	}
	else {
		fprintf(stderr, "can't find samples dir\n");
	}
}
