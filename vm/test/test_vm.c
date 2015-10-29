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
#include <wich.h>
#include "vm.h"

#include <cunit.h>
#include <wloader.h>

static void setup()		{ }
static void teardown()	{ }

static void run(char *code) {
	save_string("/tmp/t.wasm", code);
	FILE *f = fopen("/tmp/t.wasm", "r");
	VM *vm = vm_load(f);
	fclose(f);
	vm_exec(vm, true);
}


void hello() {
	char *code =
		"1 strings\n"
		"\t0: 5/hello\n"
		"1 functions\n"
		"\t0: addr=0 args=0 locals=0 type=1 4/main\n"
		"3 instr, 5 bytes\n"
		"\tSCONST 0\n"
		"\tSPRINT\n"
		"\tHALT";

	run(code);
}

void callme() {
    printf("callme ----------------------\n");
    byte code[] = {
            CALL, 1, 0,				// 0
            HALT,					// 3
            SCONST, 0, 0,			// 4
            SPRINT,
            RET
    };

    VM *vm = vm_alloc();
	vm_init(vm, code, sizeof(code));
    vm->strings = (char **)calloc(1, sizeof(char *));
    vm->num_strings = 1;
    vm->strings[0] = "hello";
	def_function(vm, "main", VOID_TYPE, 0, 0, 0);
	def_function(vm, "foo", VOID_TYPE, 4, 0, 0);
	vm_exec(vm, true);
}

void callarg1() {
    printf("callarg1 ----------------------\n");
    byte code[] = {
            // print f(34)
            ICONST, 34, 0, 0, 0,		// 0
            CALL, 1, 0, 				// 5
            IPRINT,						// 8
            HALT,						// 9
            // def f(x)
            // print x
            ILOAD, 0, 0,				// 10
            IPRINT,
            // return 99
            ICONST, 99, 0, 0, 0,
            RETV
    };

    VM *vm = vm_alloc();
	vm_init(vm, code, sizeof(code));
	def_function(vm, "main", VOID_TYPE, 0, 0, 0);
	def_function(vm, "f", VOID_TYPE, 10, 1, 0);
	vm_exec(vm, true);
}

void vec0() {
	char *code =
		"0 strings\n"
		"1 functions\n"
		"	0: addr=0 args=0 locals=0 type=1 4/main\n"
		"4 instr, 8 bytes\n"
		"	ICONST 5\n"
		"	VECTOR\n"
		"	VPRINT\n"
		"	HALT";

	run(code);
}

void locals() {
    printf("locals ----------------------\n");
    byte code[] = {
            // leave a 33 on stack before call
            ICONST, 33, 0, 0, 0,        // 0
            CALL, 1, 0, 				// 5
            HALT,                       // 8
            // a = 99
            ICONST, 99, 0, 0, 0,        // 9
            STORE, 0, 0,
            // b = 100
            ICONST, 100, 0, 0, 0,       // 4
            STORE, 1, 0,
            // print a
            ILOAD, 0, 0,
            IPRINT,
            // print b
            ILOAD, 1, 0,
            IPRINT,
            RET
    };

    VM *vm = vm_alloc();
	vm_init(vm, code, sizeof(code));
	def_function(vm, "main", VOID_TYPE, 0, 0, 0);
    def_function(vm, "foo", VOID_TYPE, 9, 0, 2);
	vm_exec(vm, true);
}

void locals_and_args() {
    printf("locals_and_args ----------------------\n");
    byte code[] = {
            // call foo(33,34)
            ICONST, 33, 0, 0, 0,        // 0
            ICONST, 34, 0, 0, 0,        // 5
            CALL, 1, 0, 				// 10		call foo at function index 1
            HALT,                       // 13
            // def foo(x,y)
            // a = x
            ILOAD, 0, 0,                // 14
            STORE, 2, 0,
            // b = y
            ILOAD, 1, 0,
            STORE, 3, 0,
            // print a
            ILOAD, 0, 0,
            IPRINT,
            // print b
            ILOAD, 1, 0,
            IPRINT,
            RET
    };

    VM *vm = vm_alloc();
	vm_init(vm, code, sizeof(code));
	def_function(vm, "main", VOID_TYPE, 0, 0, 0);
    def_function(vm, "foo", VOID_TYPE, 14, 2, 2);
	vm_exec(vm, true);
}

int main(int argc, char *argv[]) {
    cunit_setup = setup;
    cunit_teardown = teardown;

	test(vec0);
//    test(hello);
//	test(locals_and_args);
//	test(locals);
//	test(callme);
//	test(callarg1);

    return 0;
}

