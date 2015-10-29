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
#include "vm.h"

#include <cunit.h>

static void setup()		{ }
static void teardown()	{ }

void hello() {
	printf("hello ----------------------\n");
	// code memory is little-endian
	byte hello[] = {
	//#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		ICONST, 34, 0, 0, 0,
	    IPRINT,
		SCONST, 0, 0,
		PPRINT,
	    HALT
	};
	int heap_size = 100;
	int stack_size = 100;

	int data_size = 0;
	word *data = NULL;

    VM *vm = vm_alloc();
    vm_init(vm, hello, sizeof(hello), data, data_size, heap_size, stack_size);
	vm->strings = (char **)calloc(1, sizeof(char *));
	vm->num_strings = 1;
	vm->strings[0] = "hello";
    vm_exec(vm, 0, true);
}

void callme() {
    printf("callme ----------------------\n");
    byte code[] = {
            CALL, 0, 0,				// 0
            HALT,					// 3
            SCONST, 0, 0,			// 4
            PPRINT,
            RET
    };

    int heap_size = 100;
    int stack_size = 100;

    int data_size = 0;
    word *data = NULL;

    VM *vm = vm_alloc();
    vm_init(vm, code, sizeof(code), data, data_size, heap_size, stack_size);
    vm->strings = (char **)calloc(1, sizeof(char *));
    vm->num_strings = 1;
    vm->strings[0] = "hello";
    def_function(vm, "foo", vm->void_type, 4, 0, 0);
    vm_exec(vm, 0, true);
}

void callarg1() {
    printf("callarg1 ----------------------\n");
    byte code[] = {
            // print f(34)
            ICONST, 34, 0, 0, 0,		// 0
            CALL, 0, 0, 				// 5
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
    int heap_size = 100;
    int stack_size = 100;

    word *data = NULL;
    int data_size = 0;

    VM *vm = vm_alloc();
    vm_init(vm, code, sizeof(code), data, data_size, heap_size, stack_size);
    def_function(vm, "f", vm->void_type, 10, 1, 0);
    vm_exec(vm, 0, true);
}

void array() {
	printf("array ----------------------\n");
	byte code[] = {
	// a = new int[10]
		ICONST, 10, 0, 0, 0,
		IARRAY,
		STORE_GLOBAL, 0, 0,
	// a[2] = 99
		LOAD_GLOBAL,  0, 0,
		ICONST, 02, 0, 0, 0,
		ICONST, 99, 0, 0, 0,
		STORE_INDEX,
	// print a[2]
		LOAD_GLOBAL,  0, 0,
		ICONST, 02, 0, 0, 0,
		LOAD_INDEX,
	    IPRINT,
	    HALT
	};
	int heap_size = 100;
	int stack_size = 100;
    int data_size = 1 * sizeof(word); // save array ptr
	word *data = (word *)malloc(data_size);

    VM *vm = vm_alloc();
	vm_init(vm, code, sizeof(code), data, data_size, heap_size, stack_size);
    def_global(vm, "a", vm->array_type, 0);
    vm_exec(vm, 0, true);
}

void locals() {
    printf("locals ----------------------\n");
    byte code[] = {
            // leave a 33 on stack before call
            ICONST, 33, 0, 0, 0,        // 0
            CALL, 0, 0, 				// 5
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
    int heap_size = 100;
    int stack_size = 100;
    int data_size = 0;
    word *data = NULL;

    VM *vm = vm_alloc();
    vm_init(vm, code, sizeof(code), data, data_size, heap_size, stack_size);
    def_global(vm, "a", vm->array_type, 0);
    def_function(vm, "foo", vm->void_type, 9, 0, 2);
    vm_exec(vm, 0, true);
}

void locals_and_args() {
    printf("locals_and_args ----------------------\n");
    byte code[] = {
            // call foo(33,34)
            ICONST, 33, 0, 0, 0,        // 0
            ICONST, 34, 0, 0, 0,        // 5
            CALL, 0, 0, 				// 10
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
    int heap_size = 100;
    int stack_size = 100;
    int data_size = 0;
    word *data = NULL;

    VM *vm = vm_alloc();
    vm_init(vm, code, sizeof(code), data, data_size, heap_size, stack_size);
    def_function(vm, "foo", vm->void_type, 14, 2, 2);
    vm_exec(vm, 0, true);
}

int main(int argc, char *argv[]) {
    cunit_setup = setup;
    cunit_teardown = teardown;

    test(hello);
	test(locals_and_args);
	test(locals);
	test(callme);
	test(callarg1);
	test(array);

    return 0;
}

