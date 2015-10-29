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
#include "wloader.h"
#include "vm.h"

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
3 globals
	0: type=1 1/z
	1: type=1 1/r
	2: type=5 1/a
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
    vm->strings = (char **)calloc(nstrings, sizeof(char *));
    for (int i=0; i<nstrings; i++) {
        int index, name_size;
        fscanf(f, "%d: %d/", &index, &name_size);
        char *str = calloc(name_size+1, sizeof(char));
        fgets(str, name_size+1, f);
        printf("str %d %s\n", index, str);
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
        printf("func %d %d %d %d %d %s\n", index, addr, args, locals, type, name);
        def_function(vm, name, type, addr, args, locals);
    }

    int nglobals;
    fscanf(f, "%d globals\n", &nglobals);
    for (int i=1; i<=nglobals; i++) {
        int index, type, name_size;
        fscanf(f, "%d: type=%d %d/", &index, &type, &name_size);
        char name[name_size+1];
        fgets(name, name_size+1, f);
        printf("global %d %d %s\n", index, type, name);
        addr32 addr = (unsigned)index;
        def_global(vm, name, type, addr);
    }
    word *global_data = calloc(nglobals, sizeof(word));

    int ninstr, nbytes;
    fscanf(f, "%d instr, %d bytes\n", &ninstr, &nbytes);
    printf("%d instr, %d bytes\n", ninstr, nbytes);
    byte *code = calloc(nbytes, sizeof(byte));
    addr32 ip = 0; // start loading bytecode at address 0
    for (int i=1; i<=ninstr; i++) {
        char instr[80+1];
        fgets(instr, 80+1, f); // get instruction
        float fvalue;
        int n = sscanf(instr, "\tFCONST %f", &fvalue);
        if ( n==1 ) {
            printf("FCONST %f\n", fvalue);
            unsigned int as_int = *((unsigned int *)&fvalue);
            vm_write32(&code[ip], as_int);
            ip += 4;
            continue;
        }

        char name[80];
        int ivalue;
        n = sscanf(instr, "%s %d", name, &ivalue);
        VM_INSTRUCTION *I = vm_instr(name);
        code[ip] = I->opcode;
//        printf("%d: ", ip);
        ip++;
        if ( n==2 ) {
//            printf("%s %d\n", name, ivalue);
            if ( I->opnd_size==2 ) {
//                fflush(stdout);
                vm_write16(&code[ip], *((unsigned int *)&ivalue));
            }
            else { // must be 4 bytes
                vm_write32(&code[ip], *((unsigned int *)&ivalue));
            }
            ip += I->opnd_size;
        }
        else if ( n==1 ) {
//            printf("%s\n", name);
        }
    }

    fclose(f);

    vm_init(vm, code, nbytes, global_data, nglobals);
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

BYTECODE vm_opcode(char *name) {
    for (int i = 0; i < NUM_INSTRS; ++i) {
        if ( strcmp(name, vm_instructions[i].name)==0 ) {
            return vm_instructions[i].opcode;
        }
    }
    return -1;
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