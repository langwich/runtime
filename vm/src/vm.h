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

static const int MAX_FUNCTIONS	= 1000;
static const int MAX_LOCALS		= 10;	// max locals/args in activation record
static const int MAX_CALL_STACK = 1000;
static const int MAX_OPND_STACK = 1000;
static const int NUM_INSTRS		= 64;

static const int VM_NIL         = ((uintptr_t)0);

typedef unsigned char byte;
typedef uintptr_t word; // has to be big enough to hold a native machine pointer
typedef void *ptr;
typedef unsigned int addr32;
typedef unsigned int word32;
typedef unsigned short word16;

// predefined type numbers; needed by VM and any compilers that target the VM.
// for example, to define the metadata for a global variable of type int, we need to specify
// the type somehow. We use this INT_TYPE value in olava object files.

static const int VOID_TYPE = 0;
static const int INT_TYPE = 1;
static const int FLOAT_TYPE	= 2;
static const int BOOLEAN_TYPE = 3;
static const int STRING_TYPE = 4;
static const int VECTOR_TYPE = 5;

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
	I2S,                // int to str
	F2S,                // float to str
	V2S,                // vector to str
	F2V,                // float to vector

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
	SCONST,				// string const from from literal in vm via index

	ILOAD,              // load int from local context using arg or local index
	FLOAD,              // load float from local context
	VLOAD,				// load vector
	SLOAD,              // load string from local context
	STORE,              // store into local context

	VECTOR,             // create vector of size n with element type float
	LOAD_INDEX,         // array index a[i]
	STORE_INDEX,		// store into a[i]

	NIL,                // push null pointer onto stack
	POP,				// drop top of stack

	CALL,               // call a function using index
	RETV,               // return a value from function
	RET,                // return from function

	IPRINT,             // print stack top (mostly debugging)
	FPRINT,
	BPRINT,
	SPRINT,
	VPRINT,

	NOP                 // no-op, no operation
} BYTECODE;

typedef struct {
	char *name;
	BYTECODE opcode;
    int opnd_size; // size in bytes
} VM_INSTRUCTION;

typedef union {
	int i;
	double f;
	bool b;
	char *s;
	PVector_ptr vptr;
} element;

// meta-data

// to call a func, we use index into table of Function descriptors
typedef struct function {
	char *name;
	int return_type;
	addr32 address; // index into code array
	int nargs;
	int nlocals;
} Function_metadata;

typedef struct activation_record {
	Function_metadata *func;
	addr32 retaddr;
	element locals[MAX_LOCALS]; // args + locals go here per func def
} Activation_Record;

typedef struct {
	// registers
	addr32 ip;        	// instruction pointer register
    int sp;             // stack pointer register
    int fp;             // frame pointer register
	int callsp;			// call stack pointer register

	byte *code;   		// byte-addressable code memory.
	int code_size;
	element stack[MAX_OPND_STACK]; 	// operand stack, grows upwards; word addressable
	Activation_Record call_stack[MAX_CALL_STACK];

	int data_size;

	int num_strings;
	int num_functions;

	char **strings;

	Function_metadata functions[MAX_FUNCTIONS]; // array of function defs
} VM;

extern VM *vm_alloc();
extern void vm_init(VM *vm, byte *code, int code_size);

extern void vm_exec(VM *vm, bool trace);

extern byte *vm_malloc(VM *vm, int nbytes);
extern void vm_free(VM *vm, byte *p);

extern int def_function(VM *vm, char *name, int return_type, addr32 address, int nargs, int nlocals);

extern VM_INSTRUCTION vm_instructions[];

#endif
