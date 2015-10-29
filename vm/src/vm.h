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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef VM_H_
#define VM_H_

#define MAX_FUNCTIONS		1000
#define MAX_CLASSES			1000
#define MAX_GLOBALS			1000

#define VM_FALSE            0
#define VM_TRUE             1

#define VM_NIL              ((uintptr_t)0)

typedef unsigned char byte;
typedef uintptr_t word; // has to be big enough to hold a native machine pointer
typedef void *ptr;
typedef unsigned int addr32;
typedef unsigned int word32;
typedef unsigned short word16;

// predefined type numbers; needed by VM and any compilers that target the VM.
// for example, to define the metadata for a global variable of type int, we need to specify
// the type somehow. We use this INT_TYPE value in olava object files.

#define VOID_TYPE	0
#define INT_TYPE	1
#define FLOAT_TYPE	2
#define CHAR_TYPE	3
#define STRING_TYPE	4
#define ARRAY_TYPE	5
#define IARRAY_TYPE	6
#define FARRAY_TYPE	7

// Bytecodes are all 8 bits but operand size can vary
// Explicitly type the operations, even the loads/stores
// for both safety, efficiency, and possible JIT from bytecodes later.
typedef enum {
	HALT=0,             // stop the program

	IADD,               // int add
	ISUB,
	IMUL,
	IDIV,
	FADD,               // int add
	FSUB,
	FMUL,
	FDIV,

    OR,
    AND,

    INEG,               // negate integer
	FNEG,               // negate float
	NOT,                // boolean not 0->1, 1->0

	I2F,                // int to float
	F2I,                // float to int
	I2C,                // int to char
	C2I,                // chart to int

	IEQ,                // int equal
	INEQ,
	ILT,                // int less than
	ILE,
	IGT,
	IGE,
	FEQ,                // float equal
	FNEQ,
	FLT,                // float less than
	FLE,
	FGT,
	FGE,
	ISNIL,

	BR,                 // branch 16-bit relative in code memory; relative to addr of BR
	BRT,                // branch if true
	BRF,                // branch if false

	ICONST,             // push 32-bit constant integer
	FCONST,             // floating point constant
	CCONST,             // char const
	SCONST,				// string const from from literal in vm via index

	ILOAD,              // load int from local context using arg or local index
	FLOAD,              // load float from local context
	PLOAD,				// load ptr
	CLOAD,              // load char from local context
	STORE,              // store int in local context

	LOAD_GLOBAL,        // load ith global
	STORE_GLOBAL,       // store value to ith global

	NEW,             	// create new obj/struct in heap using struct index, push hardware pointer
	FREE,               // release hardware ptr into heap memory
	LOAD_FIELD,         // load ith field
	STORE_FIELD,        // field store

	IARRAY,             // create array of size n from stack top with element type int in heap, push pointer
	FARRAY,             // create array of size n with element type float
	PARRAY,             // create array of size n with element type object ptr
	LOAD_INDEX,         // array index a[i]
	STORE_INDEX,		// store into a[i]

	NIL,                // push null pointer onto stack
	POP,				// drop top of stack

	CALL,               // call a function using index
	RETV,               // return a value from function
	RET,                // return from function

	IPRINT,             // print stack top (mostly debugging)
	FPRINT,
	PPRINT,             // pointer print (must know type of the object)
	CPRINT,

	NOP                 // no-op, no operation
} BYTECODE;

typedef struct {
//	struct hash_elem hash_elem; /* Hash table element. */
	char *name;
	BYTECODE opcode;
    int opnd_size; // size in bytes
} VM_INSTRUCTION;

// meta-data

// to call a func, we use index into table of Function descriptors
typedef struct function {
	char *name;
	int return_type;
	addr32 address; // index into code array
	int nargs;
	int nlocals;
} Function_metadata;

typedef struct global_variable {
	char *name;
	int type;
	addr32 address;
} Global_metadata;

typedef struct activation_record {
	Function_metadata *func;
	addr32 retaddr;
	word locals[]; // args + locals go here per func def
} Activation_Record;

// A generic object definition has a type ptr and slots for fields.
// All of this metadata has to be defined as global data, not in the
// VM heap.
// The number of fields is known by the metadata via type ptr.
typedef struct object {
	int type;
	word fields[];
} Object;

typedef struct {
	// registers
	addr32 ip;        	// instruction pointer register
    int sp;             // stack pointer register
    int fp;             // frame pointer register
	int callsp;			// call stack pointer register

	byte *code;   		// byte-addressable code memory.
	int code_size;
	word *stack;		// operand stack, grows upwards; word addressable
	int stack_size;
	Activation_Record **call_stack;
	int call_stack_size;

	word *data;  		// global variable space; word addressable
	byte *heap;			// heap space
	int data_size;
	int heap_size;
	byte *end_of_heap;
	byte *next_free;

	// define predefined types
	int void_type;
	int int_type;
	int float_type;
	int char_type;
	int string_type;	// generic metadata for a string
	int array_type;		// generic metadata for all arrays
	int iarray_type;	// int array
	int farray_type;	// float array
	int object_type;	// generic metadata for all objects
						// used when creating an array of any ptr type, for example.

	int num_strings;
	int num_globals;
	int num_functions;
	int num_classes;

	char **strings;
	// hardcode sizes since C is so primitive
	Global_metadata globals[MAX_GLOBALS];		// array of global variables
	Function_metadata functions[MAX_FUNCTIONS]; // array of function defs
} VM;

extern VM *vm_alloc();
extern void vm_init(
		VM *vm,
		byte *code, int code_size,
		word *globals, int num_globals,
		int heap_size_in_bytes, int stack_size_in_words);

extern void vm_exec(VM *vm, addr32 main_func_ip, bool trace);

extern byte *vm_malloc(VM *vm, int nbytes);
extern void vm_free(VM *vm, byte *p);

extern int def_global(VM *vm, char *name, int type, addr32 address);
extern int def_function(VM *vm, char *name, int return_type, addr32 address, int nargs, int nlocals);

static const int NUM_INSTRUCTIONS=64;
extern VM_INSTRUCTION vm_instructions[];

#endif
