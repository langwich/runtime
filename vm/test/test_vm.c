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
#include <stdbool.h>
#include "vm.h"

#include <cunit.h>
#include <vm.h>

int hello[] = {
	ICONST, 1234,
	PRINT,
	HALT
};

int loop[] = {
// .GLOBALS 2; N, I
// N = 10                 ADDRESS
	ICONST, 10,            // 0
	GSTORE, 0,             // 2
// I = 0
	ICONST, 0,             // 4
	GSTORE, 1,             // 6
// WHILE I<N:
// START (8):
	GLOAD, 1,              // 8
	GLOAD, 0,              // 10
	ILT,                   // 12
	BRF, 24,               // 13
//     I = I + 1
	GLOAD, 1,              // 15
	ICONST, 1,             // 17
	IADD,                  // 19
	GSTORE, 1,             // 20
	BR, 8,                 // 22
// DONE (24):
// PRINT "LOOPED "+N+" TIMES."
	HALT                   // 24
};

const int FACTORIAL_ADDRESS = 0;
const int FACTORIAL_MAIN_ADDRESS = 23;
int factorial[] = {
	//						ADDRESS
//.def factorial: ARGS=1, LOCALS=0
//	IF N < 2 RETURN 1
	LOAD, 0,                // 0
	ICONST, 2,              // 2
	ILT,                    // 4
	BRF, 10,                // 5
	ICONST, 1,              // 7
	RET,                    // 9
//CONT:
//	RETURN N * FACT(N-1)
	LOAD, 0,                // 10
	LOAD, 0,                // 12
	ICONST, 1,              // 14
	ISUB,                   // 16
	CALL, FACTORIAL_ADDRESS, 1, 0,    // 17
	IMUL,                   // 21
	RET,                    // 22
//.DEF MAIN: ARGS=0, LOCALS=0
// PRINT FACT(1)
	ICONST, 5,              // 23    <-- MAIN METHOD!
	CALL, FACTORIAL_ADDRESS, 1, 0,    // 25
	GSTORE, 0,              // 29
	HALT                    // 31
};

static int dub[] = {
	//						ADDRESS
	//.def main() { print f(10); }
	ICONST, 10,             // 0
	CALL, 9, 1, 1,          // 2
	GSTORE, 0,              // 6
	HALT,                   // 8
	//.def f(x): ARGS=1, LOCALS=1
	//  a = x;
	LOAD, 0,                // 9	<-- start of f
	STORE, 1,
	// return 2*a
	LOAD, 1,
	ICONST, 2,
	IMUL,
	RET
};

static void setup()		{ }
static void teardown()	{ }

void helloworld() {
	VM *vm = vm_create(hello, sizeof(hello), 0);
	vm_exec(vm, 0, false);
	vm_free(vm);
}

void simple_loop() {
	VM *vm = vm_create(loop, sizeof(loop), 2);
	vm_exec(vm, 0, false);
	assert_equal(vm->globals[0], 10);
	assert_equal(vm->globals[1], 10);
	vm_free(vm);
}

void factorial_func() {
	VM *vm = vm_create(factorial, sizeof(factorial), 0);
	vm_exec(vm, FACTORIAL_MAIN_ADDRESS, false);
	assert_equal(vm->globals[0], 120);
	vm_free(vm);
}

void dubme() {
	VM *vm = vm_create(dub, sizeof(dub), 0);
	vm_exec(vm, 0, false);
	assert_equal(vm->globals[0], 20);
	vm_free(vm);
}

int main(int argc, char *argv[]) {
	cunit_setup = setup;
	cunit_teardown = teardown;

	test(helloworld);
	test(simple_loop);
	test(factorial_func);
	test(dubme);

	return 0;
}
