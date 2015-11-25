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
#include <sys/stat.h>
#include <wich.h>
#include "vm.h"
#include "wloader.h"

static void vm_write16(byte *data, unsigned int n);
static void vm_write32(byte *data, unsigned int n);

/*
Create a VM from a Wich object/asm file, .wasm; files look like:

2 strings
	0: 2/hi
	1: 3/bye
3 functions
	0: addr=0 locals=4 type=0 1/f
	1: addr=25 locals=0 type=1 1/g
	2: addr=32 locals=1 type=1 4/main
40 instr, 112 bytes
	ICONST 1
	ICONST 0
	OR
    ...
 */
VM *vm_load(FILE *f)
{
    VM *vm = vm_alloc();

    int nstrings;
    fscanf(f, "%d strings\n", &nstrings);
    vm->strings = (char **)calloc((size_t)nstrings, sizeof(char *));
    for (int i=0; i<nstrings; i++) {
        int index, name_size;
        fscanf(f, "%d: %d/", &index, &name_size);
        char *str = calloc((size_t)name_size+1, sizeof(char));
        fgets(str, name_size+1, f);
        vm->strings[index] = str;
    }
    vm->num_strings = nstrings;

    int nfuncs;
    fscanf(f, "%d functions\n", &nfuncs);
    for (int i=1; i<=nfuncs; i++) {
        int index, args, locals, type, name_size;
        addr32 addr;
        fscanf(f, "%d: addr=%d args=%d locals=%d type=%d %d/",
                &index, &addr, &args, &locals, &type, &name_size);
        char name[name_size+1];
        fgets(name, name_size+1, f);
        def_function(vm, name, type, addr, args, locals);
    }

    int ninstr, nbytes;
    element e;
    fscanf(f, "%d instr, %d bytes\n", &ninstr, &nbytes);
    byte *code = calloc((size_t)nbytes, sizeof(byte));
    addr32 ip = 0;
    for (int i=1; i<=ninstr; i++) {
        char instr[80+1];
        fgets(instr, 80+1, f);
        float fvalue;
        int n = sscanf(instr, "\tFCONST %f", &fvalue);
        if ( n==1 ) {
            VM_INSTRUCTION *I = vm_instr("FCONST");
            code[ip] = I->opcode;
            ip++;
            e.f = fvalue;
            unsigned int as_int = (unsigned int)e.f;
            vm_write32(&code[ip], as_int);
            ip += 4;
            continue;
        }

        char name[80];
        int ivalue;
        n = sscanf(instr, "%s %d", name, &ivalue);
        VM_INSTRUCTION *I = vm_instr(name);
        code[ip] = I->opcode;
        ip++;
        if ( n==2 ) {
            if ( I->opnd_size==2 ) {
                vm_write16(&code[ip], *((unsigned int *)&ivalue));
            }
            else { // must be 4 bytes
                vm_write32(&code[ip], *((unsigned int *)&ivalue));
            }
            ip += I->opnd_size;
        }
        else if ( n==1 ) {
        }
    }
    fclose(f);
    vm_init(vm, code, nbytes);
    return vm;
}

static void vm_write32(byte *data, unsigned int n)
{
    // assume little-endian!
    data[3] = (byte)((n >> 24) & 0xFF);  // get high byte
    data[2] = (byte)((n >> 16) & 0xFF);
    data[1] = (byte)((n >> 8) & 0xFF);
    data[0] = (byte)(n & 0xFF);
}

static void vm_write16(byte *data, unsigned int n)
{
    // assume little-endian!
    data[1] = (byte)((n >> 8) & 0xFF);
    data[0] = (byte)(n & 0xFF);
}

VM_INSTRUCTION *vm_instr(char *name) {
    for (int i = 0; i < NUM_INSTRS; ++i) {
        if ( strcmp(name, vm_instructions[i].name)==0 ) {
            return &vm_instructions[i];
        }
    }
    return NULL;
}

Function_metadata *vm_function(VM *vm, char *name) {
    for (int i = 0; i < vm->num_functions; ++i) {
        if ( strcmp(name, vm->functions[i].name)==0 ) {
            return &vm->functions[i];
        }
    }
    return NULL;
}

void save_string(char *filename, char *s) {
	FILE *f = fopen(filename, "w");
	if ( f==NULL ) {
		fprintf(stderr, "can't open %s\n", filename);
		return;
	}
	fputs(s, f);
	fclose(f);
}
