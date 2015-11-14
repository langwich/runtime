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
#include <wich.h>

#include "vm.h"

#include "wloader.h"

VM_INSTRUCTION vm_instructions[] = {
		{"HALT", HALT, 0},

		{"IADD",  IADD,  0},
		{"ISUB",  ISUB,  0},
		{"IMUL",  IMUL,  0},
		{"IDIV",  IDIV,  0},
		{"FADD",  FADD,  0},
		{"FSUB",  FSUB,  0},
		{"FMUL",  FMUL,  0},
		{"FDIV",  FDIV,  0},
		{"VADD",  VADD,  0},
		{"VADDI", VADDI, 0},
		{"VADDF", VADDF, 0},
		{"VSUB",  VSUB,  0},
		{"VSUBI", VSUBI, 0},
		{"VSUBF", VSUBF, 0},
		{"VMUL",  VMUL,  0},
		{"VMULI", VMULI, 0},
		{"VMULF", VMULF, 0},
		{"VDIV",  VDIV,  0},
		{"VDIVI", VDIVI, 0},
		{"VDIVF", VDIVF, 0},
		{"SADD",  SADD,  0},

		{"OR",    OR,    0},
		{"AND",   AND,   0},
		{"INEG",  INEG,  0},
		{"FNEG",  FNEG,  0},
		{"NOT",   NOT,   0},

		{"I2F",   I2F,   0},
		{"F2I",   F2I,   0},
		{"I2S",   I2S,   0},
		{"F2S",   F2S,   0},
		{"V2S",   V2S,   0},

		{"IEQ",   IEQ,   0},
		{"INEQ",  INEQ,  0},
		{"ILT",   ILT,   0},
		{"ILE",   ILE,   0},
		{"IGT",   IGT,   0},
		{"IGE",   IGE,   0},
		{"FEQ",   FEQ,   0},
		{"FNEQ",  FNEQ,  0},
		{"FLT",   FLT,   0},
		{"FLE",   FLE,   0},
		{"FGT",   FGT,   0},
		{"FGE",   FGE,   0},
		{"SEQ",   SEQ,   0},
		{"SNEQ",  SNEQ,  0},
		{"SGT",   SGT,   0},
		{"SGE",   SGE,   0},
		{"SLT",   SGT,   0},
		{"SLE",   SGE,   0},
		{"VEQ",   VEQ,   0},
		{"VNEQ",  VNEQ,  0},

		{"BR",    BR,    2},
		{"BRF",   BRF,   2},
		{"ICONST",ICONST,4},
		{"FCONST",FCONST,4},
		{"SCONST",SCONST,2},
		{"ILOAD", ILOAD, 2},
		{"FLOAD", FLOAD, 2},
		{"VLOAD", VLOAD, 2},
		{"SLOAD", SLOAD, 2},
		{"STORE", STORE, 2},
		{"VECTOR",VECTOR, 0},
		{"LOAD_INDEX",  VLOAD_INDEX, 0},
		{"STORE_INDEX", STORE_INDEX, 0},
		{"SLOAD_INDEX", SLOAD_INDEX, 0},
		{"PUSH",  PUSH,  2},
		{"POP",  POP,  0},
		{"CALL", CALL, 2},
		{"RETV", RETV, 0},
		{"RET",  RET,  0},
		{"IPRINT", IPRINT, 0},
		{"FPRINT", FPRINT, 0},
		{"BPRINT", BPRINT, 0},
		{"SPRINT", SPRINT, 0},
		{"VPRINT", VPRINT, 0},
		{"NOP", NOP, 0}
};

static void vm_print_instr(VM *vm, addr32 ip);
static void vm_print_stack(VM *vm);
static inline int int32(const byte *data, addr32 ip);
static inline int int16(const byte *data, addr32 ip);
static inline float float32(const byte *data, addr32 ip);
static void vm_call(VM *vm, Function_metadata *func);
static void vm_print_stack_value(VM *vm, word p);

VM *vm_alloc()
{
	VM *vm = calloc(1, sizeof(VM));
	return vm;
}

void vm_init(VM *vm, byte *code, int code_size)
{
	// we are linking in mark-and-compact collector so allocations all occur outside of the VM
	vm->code = code;
	vm->code_size = code_size;
	vm->sp = -1; // grow upwards, stack[sp] is top of stack and valid
	vm->fp = -1; // frame pointer is invalid initially
	vm->callsp = -1;
}

int def_function(VM *vm, char *name, int return_type, addr32 address, int nargs, int nlocals)
{
	if ( vm->num_functions>=MAX_FUNCTIONS ) {
		fprintf(stderr, "Exceeded max functions %d\n", MAX_FUNCTIONS);
		return -1;
	}
	int i = vm->num_functions++;
	Function_metadata *f = &vm->functions[i];
	f->name = strdup(name);
	f->return_type = return_type;
	f->address = address;
	f->nargs = nargs;
	f->nlocals = nlocals;
	return i;
}

#define WRITE_BACK_REGISTERS(vm) vm->ip = ip; vm->sp = sp; vm->fp = fp;
#define LOAD_REGISTERS(vm) ip = vm->ip; sp = vm->sp; fp = vm->fp;

static void inline validate_stack_address(int a) {
	if ((a) < 0 || (a) >= MAX_OPND_STACK) {
		fprintf(stderr, "%d stack ptr out of range 0..%d\n", a, MAX_OPND_STACK - 1);
	}
}

void vm_exec(VM *vm, bool trace) {
	int a = 0;
	int i = 0;
	bool b1, b2;
	double f,g;
	char* c;
	PVector_ptr vptr,r,l;
	int x, y;
	Activation_Record *frame;

	// simulate a call to main()
	Function_metadata *const main = vm_function(vm, "main");
	vm_call(vm, main);

	// Define VM registers (C compiler probably ignores 'register' nowadays
	// but it's good documentation in this case. Keep as locals for
	// convenience but write them back to the vm object after each decode/execute.
	register addr32 ip = vm->ip;
	register int sp = vm->sp;
	register int fp = vm->fp;
	const byte *code = vm->code;
	element *stack = vm->stack;

	int opcode = code[ip];

	while (opcode != HALT && ip < vm->code_size ) {
		if (trace) vm_print_instr(vm, ip);
		ip++; //jump to next instruction or to operand
		switch (opcode) {
			case IADD:
				validate_stack_address(sp-1);
				y = stack[sp--].i;
				x = stack[sp].i;
				stack[sp].i = x + y;
				break;
			case ISUB:
				validate_stack_address(sp-1);
				y = stack[sp--].i;
				x = stack[sp].i;
				stack[sp].i = x - y;
				break;
			case IMUL:
				validate_stack_address(sp-1);
				y = stack[sp--].i;
				x = stack[sp].i;
				stack[sp].i = x * y;
				break;
			case IDIV:
				validate_stack_address(sp-1);
				y = stack[sp--].i;
				x = stack[sp].i;
				stack[sp].i = x / y;
				break;
			case FADD:
				validate_stack_address(sp-1);
				f = stack[sp--].f;
				g = stack[sp].f;
				stack[sp].f = f + g;
				break;
			case FSUB:
				validate_stack_address(sp-1);
				f = stack[sp--].f;
				g = stack[sp].f;
				stack[sp].f = f - g;
				break;
			case FMUL:
				validate_stack_address(sp-1);
				f = stack[sp--].f;
				g = stack[sp].f;
				stack[sp].f = f * g;
				break;
			case FDIV:
				validate_stack_address(sp-1);
				f = stack[sp--].f;
				g = stack[sp].f;
				stack[sp].f = f / g;
				break;
            case VADD:
				validate_stack_address(sp-1);
				r = stack[sp--].vptr;
				l = stack[sp].vptr;
				vptr = Vector_add(l,r);
				stack[sp].vptr = vptr;
                break;
			case VADDI:
				validate_stack_address(sp-1);
				i = stack[sp--].i;
				vptr = stack[sp].vptr;
				vptr = Vector_add(vptr,Vector_from_int(i,vptr.vector->length));
				stack[sp].vptr = vptr;
				break;
			case VADDF:
				validate_stack_address(sp-1);
				f = stack[sp--].f;
				vptr = stack[sp].vptr;
				vptr = Vector_add(vptr,Vector_from_float(f,vptr.vector->length));
				stack[sp].vptr = vptr;
				break;
            case VSUB:
				validate_stack_address(sp-1);
				r = stack[sp--].vptr;
				l = stack[sp].vptr;
				vptr = Vector_sub(l,r);
				stack[sp].vptr = vptr;
                break;
			case VSUBI:
				validate_stack_address(sp-1);
				i = stack[sp--].i;
				vptr = stack[sp].vptr;
				vptr = Vector_sub(vptr,Vector_from_int(i,vptr.vector->length));
				stack[sp].vptr = vptr;
				break;
			case VSUBF:
				validate_stack_address(sp-1);
				f = stack[sp--].f;
				vptr = stack[sp].vptr;
				vptr = Vector_sub(vptr,Vector_from_float(f,vptr.vector->length));
				stack[sp].vptr = vptr;
				break;
            case VMUL:
				validate_stack_address(sp-1);
				r = stack[sp--].vptr;
				l = stack[sp].vptr;
				vptr = Vector_mul(l,r);
				stack[sp].vptr = vptr;
                break;
			case VMULI:
				validate_stack_address(sp-1);
				i = stack[sp--].i;
				vptr = stack[sp].vptr;
				vptr = Vector_mul(vptr,Vector_from_int(i,vptr.vector->length));
				stack[sp].vptr = vptr;
				break;
			case VMULF:
				validate_stack_address(sp-1);
				f = stack[sp--].f;
				vptr = stack[sp].vptr;
				vptr = Vector_mul(vptr,Vector_from_float(f,vptr.vector->length));
				stack[sp].vptr = vptr;
				break;
            case VDIV:
                validate_stack_address(sp-1);
				r = stack[sp--].vptr;
				l = stack[sp].vptr;
                vptr = Vector_div(l,r);
                stack[sp].vptr = vptr;
                break;
			case VDIVI:
				validate_stack_address(sp-1);
				i = stack[sp--].i;
				vptr = stack[sp].vptr;
				vptr = Vector_div(vptr,Vector_from_int(i,vptr.vector->length));
				stack[sp].vptr = vptr;
				print_pvector(vptr);
				break;
			case VDIVF:
				validate_stack_address(sp-1);
				f = stack[sp--].f;
				vptr = stack[sp].vptr;
				vptr = Vector_div(vptr,Vector_from_float(f,vptr.vector->length));
				stack[sp].vptr = vptr;
				break;
            case SADD:
				validate_stack_address(sp-1);
				char * right = stack[sp--].s;
				stack[sp].s = String_add(String_new(stack[sp].s),String_new(right))->str;
                break;
			case OR :
				validate_stack_address(sp-1);
				b2 = stack[sp--].b;
				b1 = stack[sp].b;
				stack[sp].b = b1 || b2;
				break;
			case AND :
				validate_stack_address(sp-1);
				b2 = stack[sp--].b;
				b1 = stack[sp].b;
				stack[sp].b = b1 && b2;
				break;
			case INEG:
				validate_stack_address(sp);
				stack[sp].i = -stack[sp].i;
				break;
			case FNEG:
				validate_stack_address(sp);
				stack[sp].f = -stack[sp].f;
				break;
			case NOT:
				validate_stack_address(sp);
				stack[sp].b = !stack[sp].b;
				break;
			case I2F:
				validate_stack_address(sp);
				stack[sp].f = stack[sp].i;
				break;
			case I2S:
				validate_stack_address(sp);
				stack[sp].s = String_from_int(stack[sp].i)->str;
				break;
			case F2I:
				validate_stack_address(sp);
				stack[sp].i = (int)stack[sp].f;
				break;
            case F2S:
				validate_stack_address(sp);
				stack[sp].s = String_from_float((float)stack[sp].f)->str;
                break;
            case V2S:
				validate_stack_address(sp);
				vptr = stack[sp].vptr;
				stack[sp].s = String_from_vector(vptr)->str;
                break;
			case IEQ:
				validate_stack_address(sp-1);
				y = stack[sp--].i;
				x = stack[sp].i;
				stack[sp].b = x == y;
				break;
			case INEQ:
				validate_stack_address(sp-1);
				y = stack[sp--].i;
				x = stack[sp].i;
				stack[sp].b = x != y;
				break;
			case ILT:
				validate_stack_address(sp-1);
				y = stack[sp--].i;
				x = stack[sp].i;
				stack[sp].b = x < y;
				break;
			case ILE:
				validate_stack_address(sp-1);
				y = stack[sp--].i;
				x = stack[sp].i;
				stack[sp].b = x <= y;
				break;
			case IGT:
				validate_stack_address(sp-1);
				y = stack[sp--].i;
				x = stack[sp].i;
				stack[sp].b = x > y;
				break;
			case IGE:
				validate_stack_address(sp-1);
				y = stack[sp--].i;
				x = stack[sp].i;
				stack[sp].b = x >= y;
				break;
			case FEQ:
				validate_stack_address(sp-1);
				g = stack[sp--].f;
				f = stack[sp].f;
				stack[sp].b = f == g;
				break;
			case FNEQ:
				validate_stack_address(sp-1);
				g = stack[sp--].f;
				f = stack[sp].f;
				stack[sp].b = f != g;
				break;
			case FLT:
				validate_stack_address(sp-1);
				g = stack[sp--].f;
				f = stack[sp].f;
				stack[sp].b = f < g;
				break;
			case FLE:
				validate_stack_address(sp-1);
				g = stack[sp--].f;
				f = stack[sp].f;
				stack[sp].b = f <= g;
				break;
			case FGT:
				validate_stack_address(sp-1);
				g = stack[sp--].f;
				f = stack[sp].f;
				stack[sp].b = f > g;
				break;
			case FGE:
				validate_stack_address(sp-1);
				g = stack[sp--].f;
				f = stack[sp].f;
				stack[sp].b = f >= g;
				break;
            case SEQ:
				validate_stack_address(sp-1);
				c = stack[sp--].s;
				b1 = String_eq(String_new(stack[sp--].s),String_new(c));
				stack[++sp].b = b1;
                break;
            case SNEQ:
				validate_stack_address(sp-1);
				c = stack[sp--].s;
				b1 = String_neq(String_new(stack[sp--].s),String_new(c));
				stack[++sp].b = b1;
                break;
            case SGT:
				validate_stack_address(sp-1);
				c = stack[sp--].s;
				b1 = String_gt(String_new(stack[sp--].s),String_new(c));
				stack[++sp].b = b1;
                break;
            case SGE:
				validate_stack_address(sp-1);
				c = stack[sp--].s;
				b1 = String_ge(String_new(stack[sp--].s),String_new(c));
				stack[++sp].b = b1;
                break;
            case SLT:
				validate_stack_address(sp-1);
				c = stack[sp--].s;
				b1 = String_lt(String_new(stack[sp--].s),String_new(c));
				stack[++sp].b = b1;
                break;
            case SLE:
				validate_stack_address(sp-1);
				c = stack[sp--].s;
				b1 = String_le(String_new(stack[sp--].s),String_new(c));
				stack[++sp].b = b1;
                break;
			case VEQ:
				validate_stack_address(sp-1);
				b1 = Vector_eq(stack[sp--].vptr,stack[sp--].vptr);
				stack[++sp].b = b1;
				break;
			case VNEQ:
				validate_stack_address(sp-1);
				b1 = Vector_neq(stack[sp--].vptr,stack[sp--].vptr);
				stack[++sp].b = b1;
				break;
			case BR:
				ip += int16(code,ip) - 1;
				break;
			case BRF:
				validate_stack_address(sp);
				if ( !stack[sp--].b ) {
					int offset = int16(code,ip);
					ip += offset - 1;
				}
				else {
					ip += 2;
				}
				break;
			case ICONST: // code4[ip]
				stack[++sp].i = int32(code,ip);
				ip += 4;
				break;
			case FCONST:
				stack[++sp].f = float32(code,ip);
				ip += 4;
				break;
			case SCONST :
				i = int16(code,ip);
				ip += 2;
				stack[++sp].s = vm->strings[i];
				break;
			case ILOAD:
				i = int16(code,ip); // get index into locals
				ip += 2;
				stack[++sp].i = vm->call_stack[vm->callsp].locals[i].i;
				break;
			case FLOAD:
				i = int16(code,ip); // get index into locals
				ip += 2;
				stack[++sp].f = vm->call_stack[vm->callsp].locals[i].f;
				break;
            case VLOAD:
                i = int16(code,ip);
                ip += 2;
                stack[++sp].vptr = vm->call_stack[vm->callsp].locals[i].vptr;
                break;
            case SLOAD:
                i = int16(code,ip);
                ip += 2;
                stack[++sp].s = vm->call_stack[vm->callsp].locals[i].s;
				break;
			case STORE:
				i = int16(code,ip);
				ip += 2;
				vm->call_stack[vm->callsp].locals[i] = stack[sp--]; // untyped store; it'll just copy all bits
				break;
			case VECTOR:
				i = stack[sp--].i;
				validate_stack_address(sp-i+1);
				PVector_ptr pvec = PVector_init(0, i);
				for (int j = i-1; j >= 0;j--) {
					set_ith(pvec,j,stack[sp--].f);
				}
				stack[++sp].vptr = pvec;
				break;
			case VLOAD_INDEX:
				i = stack[sp--].i;
				vptr = stack[sp--].vptr;
				vm->stack[++sp].f = ith(vptr, i);
				break;
			case STORE_INDEX:
				f = stack[sp--].f;
				i = stack[sp--].i;
				vptr = stack[sp--].vptr;
				set_ith(vptr, i, f);
				print_vector(vptr);
				break;
			case SLOAD_INDEX:
				i = stack[sp--].i;
				char* c = String_from_char(stack[sp--].s[i-1])->str;
				stack[++sp].s = c;
				break;
			case PUSH:
				i = int16(code,ip); // index of function's return type
				ip += 2;
				switch (i) {
					case INT_TYPE:
						stack[++sp].i = 0;
						break;
					case FLOAT_TYPE:
						stack[++sp].f = 0.0;
						break;
					case BOOLEAN_TYPE:
						stack[++sp].b = true;
						break;
					case STRING_TYPE:
						stack[++sp].s = "";
						break;
					case VECTOR_TYPE:
						stack[++sp].vptr = PVector_init(0, 0);
						break;
					default:
						break;
				}
				break;
			case POP:
				sp--;
				break;
			case CALL:
				a = int16(code,ip); // load index of function from code memory
				WRITE_BACK_REGISTERS(vm); // (ip has been updated)
				vm_call(vm, &vm->functions[a]);
				LOAD_REGISTERS(vm);
				break;
			case RETV:
				frame = &vm->call_stack[vm->callsp--];
				ip = frame->retaddr;
				break;
			case RET:
				frame = &vm->call_stack[vm->callsp--];
				ip = frame->retaddr;
				break;
			case IPRINT:
				validate_stack_address(sp);
				printf("%d\n", stack[sp--].i);
				break;
			case FPRINT:
				validate_stack_address(sp);
				printf("%f\n", stack[sp--].f);
				break;
			case BPRINT:
				validate_stack_address(sp);
				printf("%s\n", stack[sp--].b ? "true" : "false");
				break;
			case SPRINT:
				validate_stack_address(sp);
				printf("%s\n", stack[sp--].s);
				break;
			case VPRINT:
				validate_stack_address(sp);
				print_vector(stack[sp--].vptr);
				break;
			case NOP : break;
			default:
				printf("invalid opcode: %d at ip=%d\n", opcode, (ip - 1));
				exit(1);
		}
		WRITE_BACK_REGISTERS(vm);
		if (trace) vm_print_stack(vm);
		opcode = code[ip];
	}
	if (trace) vm_print_instr(vm, ip);
	if (trace) vm_print_stack(vm);
}

void vm_call(VM *vm, Function_metadata *func)
{
	Activation_Record *r = &vm->call_stack[++vm->callsp];
	r->func = func;
	r->retaddr = vm->ip + 2; // save return address (assume ip is 1st byte of operand)
	// copy args to frame activation record
	for (int i = func->nargs-1; i>=0 ; --i) {
		r->locals[i] = vm->stack[vm->sp--];
	}
	for (int i = 0; i<func->nlocals; i++) {
		r->locals[func->nargs+i].i = 0; // init locals
	}
	vm->ip = func->address; // jump!
}

static inline int int32(const byte *data, addr32 ip)
{
	return *((word32 *)&data[ip]);
}

static inline float float32(const byte *data, addr32 ip)
{
	return *((int *)&data[ip]); // could be negative value
}

static inline int int16(const byte *data, addr32 ip)
{
	return *((short *)&data[ip]); // could be negative value
}

static void vm_print_instr(VM *vm, addr32 ip)
{
	int opcode = vm->code[ip];
	VM_INSTRUCTION *inst = &vm_instructions[opcode];
	switch (inst->opnd_size) {
		case 0:
			fprintf(stderr, "%04d:  %-25s", ip, inst->name);
			break;
		case 1:
			fprintf(stderr, "%04d:  %-15s%-10d", ip, inst->name, vm->code[ip+1]);
			break;
		case 2:
			fprintf(stderr, "%04d:  %-15s%-10d", ip, inst->name, int16(vm->code, ip + 1));
			break;
		case 4:
			fprintf(stderr, "%04d:  %-15s%-10d", ip, inst->name, int32(vm->code, ip + 1));
			break;
	}
}

static void vm_print_stack(VM *vm) {
	// stack grows upwards; stack[sp] is top of stack
	fprintf(stderr, "calls=[");
	for (int i = 0; i <= vm->callsp; i++) {
		Activation_Record *frame = &vm->call_stack[i];
		Function_metadata *func = frame->func;
		fprintf(stderr, " %s=[", func->name);
		for (int j = 0; j < func->nlocals+func->nargs; ++j) {
			vm_print_stack_value(vm, frame->locals[j].i);
		}
		fprintf(stderr, " ]");
	}
	fprintf(stderr, " ]  ");
	fprintf(stderr, "opnds=[");
	for (int i = 0; i <= vm->sp; i++) {
		word p = vm->stack[i].i;
		vm_print_stack_value(vm, p);
	}
	fprintf(stderr, " ] fp=%d sp=%d\n", vm->fp, vm->sp);
}

void vm_print_stack_value(VM *vm, word p) {
	if ( ((long)p) >= 0 ) {
		fprintf(stderr, " %lu", p);
	}
	else {
		fprintf(stderr, " %ld", p); // assume negative value is correct (and not a huge unsigned)
	}
}
