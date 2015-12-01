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
	vm_exec(vm,false);
}

/*
 * print ("hello")
 */
void hello() {
	char *code =
		"1 strings\n"
        "0: 5/hello\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=0 type=0 4/main\n"
        "5 instr, 7 bytes\n"
        "GC_START\n"
        "SCONST 0\n"
        "SPRINT\n"
        "GC_END\n"
        "HALT\n";
	run(code);
}

/*
 * print ("hello"+"world")
 */
void string_add() {
    char *code =
        "2 strings\n"
        "0: 5/hello\n"
        "1: 5/world\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=0 type=0 4/main\n"
        "7 instr, 11 bytes\n"
        "GC_START\n"
        "SCONST 0\n"
        "SCONST 1\n"
        "SADD\n"
        "SPRINT\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}

/*
 * var x = 1
 */
void int_var_def() {
    char *code =
        "0 strings\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=1 type=0 4/main\n"
        "5 instr, 11 bytes\n"
        "GC_START\n"
        "ICONST 1\n"
        "STORE 0\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}

/*
 * var x = "hello"
 */
void string_var_def() {
    char *code =
        "1 strings\n"
        "0: 5/hello\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=1 type=0 4/main\n"
        "6 instr, 10 bytes\n"
        "GC_START\n"
        "SCONST 0\n"
        "STORE 0\n"
        "SROOT\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}

/*
 * var x = [1.0,2.0,3.0]
 * print (x)
 */
void vector() {
    char *code =
        "0 strings\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=1 type=0 4/main\n"
        "12 instr, 32 bytes\n"
        "GC_START\n"
        "FCONST 1.0\n"
        "FCONST 2.0\n"
        "FCONST 3.0\n"
        "ICONST 3\n"
        "VECTOR\n"
        "STORE 0\n"
        "VROOT\n"
        "VLOAD 0\n"
        "VPRINT\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}

/*
 * func f(){ g(3) }
 * func g(x:int):int { return x }
 * f()
 */
void func_call() {
    char *code =
        "0 strings\n"
        "3 functions\n"
        "0: addr=0 args=0 locals=0 type=0 1/f\n"
        "1: addr=12 args=1 locals=0 type=1 1/g\n"
        "2: addr=21 args=0 locals=0 type=0 4/main\n"
        "17 instr, 27 bytes\n"
        "GC_START\n"
        "ICONST 3\n"
        "CALL 1\n"
        "POP\n"
        "RET\n"
        "GC_END\n"
        "GC_START\n"
        "ILOAD 0\n"
        "GC_END\n"
        "RET\n"
        "PUSH_DFLT_RETV\n"
        "RET\n"
        "GC_END\n"
        "GC_START\n"
        "CALL 0\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}

/*
 * func f(){var y = g(3,1) print (y)}
 * func g(z:int,y:int):boolean { if(z>y){return true} else {return false} }
 * f()
 */
void func_call_two_args() {
    char *code =
        "0 strings\n"
        "3 functions\n"
        "0: addr=0 args=0 locals=1 type=0 1/f\n"
        "1: addr=23 args=2 locals=0 type=3 1/g\n"
        "2: addr=54 args=0 locals=0 type=0 4/main\n"
        "28 instr, 60 bytes\n"
        "GC_START\n"
        "ICONST 3\n"
        "ICONST 1\n"
        "CALL 1\n"
        "STORE 0\n"
        "ILOAD 0\n"
        "BPRINT\n"
        "RET\n"
        "GC_END\n"
        "GC_START\n"
        "ILOAD 0\n"
        "ILOAD 1\n"
        "IGT\n"
        "BRF 13\n"
        "ICONST 1\n"
        "GC_END\n"
        "RET\n"
        "BR 10\n"
        "ICONST 0\n"
        "GC_END\n"
        "RET\n"
        "PUSH_DFLT_RETV\n"
        "RET\n"
        "GC_END\n"
        "GC_START\n"
        "CALL 0\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}

/*
 * var i = 3
 * if ( i>0 ) print (i)
 */
void if_stat() {
    char *code =
        "0 strings\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=1 type=0 4/main\n"
        "11 instr, 27 bytes\n"
        "GC_START\n"
        "ICONST 3\n"
        "STORE 0\n"
        "ILOAD 0\n"
        "ICONST 0\n"
        "IGT\n"
        "BRF 7\n"
        "ILOAD 0\n"
        "IPRINT\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}

/*
 * var i = 3
 * if ( i>0 ) print (i)
 * else print (1)
 */
void if_else() {
    char *code =
        "0 strings\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=1 type=0 4/main\n"
        "14 instr, 36 bytes\n"
        "GC_START\n"
        "ICONST 3\n"
        "STORE 0\n"
        "ILOAD 0\n"
        "ICONST 0\n"
        "IGT\n"
        "BRF 10\n"
        "ILOAD 0\n"
        "IPRINT\n"
        "BR 9\n"
        "ICONST 1\n"
        "IPRINT\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}
/*
 * var i = 0
 * while ( i<10 ) {i = i + 1 }
 * print(i)
 */
void while_stat() {
    char *code =
        "0 strings\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=1 type=0 4/main\n"
        "16 instr, 42 bytes\n"
        "GC_START\n"
        "ICONST 0\n"
        "STORE 0\n"
        "ILOAD 0\n"
        "ICONST 10\n"
        "ILT\n"
        "BRF 18\n"
        "ILOAD 0\n"
        "ICONST 1\n"
        "IADD\n"
        "STORE 0\n"
        "BR -24\n"
        "ILOAD 0\n"
        "IPRINT\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}

/*
 * var x = [1.0,2.0,3.0]
 * x[1] = 4.0
 * print (x)
 */
void vector_element_assign() {
    char *code =
        "0 strings\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=1 type=0 4/main\n"
        "16 instr, 46 bytes\n"
        "GC_START\n"
        "FCONST 1.0\n"
        "FCONST 2.0\n"
        "FCONST 3.0\n"
        "ICONST 3\n"
        "VECTOR\n"
        "STORE 0\n"
        "VROOT\n"
        "VLOAD 0\n"
        "ICONST 1\n"
        "FCONST 4.0\n"
        "STORE_INDEX\n"
        "VLOAD 0\n"
        "VPRINT\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}

void test_int_to_string() {
    char *code =
        "1 strings\n"
        "\t0:5/hello\n"
        "1 functions\n"
        "\t0: addr=0 args=0 locals=1 type=0 4/main\n"
        "6 instr, 12 bytes\n"
        "\tSCONST 0\n"
        "\tICONST 101\n"
        "\tI2S\n"
        "\tSADD\n"
        "\tSPRINT\n"
        "\tHALT\n";
    run(code);
}

void vector_add_int() {
    char *code =
        "0 strings\n"
        "1 functions\n"
        "\t0: addr=0 args=0 locals=2 type=0 4/main\n"
        "13 instr, 41 bytes\n"
        "\tFCONST 1.0\n"
        "\tFCONST 2.0\n"
        "\tFCONST 3.0\n"
        "\tICONST 3\n"
        "\tVECTOR\n"
        "\tSTORE 0\n"
        "\tVLOAD 0\n"
        "\tICONST 1\n"
        "\tVADDI\n"
        "\tSTORE 1\n"
        "\tVLOAD 1\n"
        "\tVPRINT\n"
        "\tHALT\n";
    run(code);
}

/*
 * var x = "car"
 * var y = "cat"
 * var z = x[1] + y[2]
 * print (z)
 *
 */
void string_index() {
    char *code =
        "2 strings\n"
        "0: 3/car\n"
        "1: 3/cat\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=3 type=0 4/main\n"
        "20 instr, 44 bytes\n"
        "GC_START\n"
        "SCONST 0\n"
        "STORE 0\n"
        "SROOT\n"
        "SCONST 1\n"
        "STORE 1\n"
        "SROOT\n"
        "SLOAD 0\n"
        "ICONST 1\n"
        "SLOAD_INDEX\n"
        "SLOAD 1\n"
        "ICONST 2\n"
        "SLOAD_INDEX\n"
        "SADD\n"
        "STORE 2\n"
        "SROOT\n"
        "SLOAD 2\n"
        "SPRINT\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}

/*
 * func f(x:[]):[]{return x*2}
 * var v = [1,2,3]
 * var v2 = f(v)
 * print ((v2/v)+1)
*/
void vector_op() {
    char *code =
        "0 strings\n"
        "2 functions\n"
        "0: addr=0 args=1 locals=0 type=5 1/f\n"
        "1: addr=15 args=0 locals=2 type=0 4/main\n"
        "34 instr, 72 bytes\n"
        "GC_START\n"
        "VLOAD 0\n"
        "ICONST 2\n"
        "VMULI\n"
        "GC_END\n"
        "RET\n"
        "PUSH_DFLT_RETV\n"
        "RET\n"
        "GC_END\n"
        "GC_START\n"
        "ICONST 1\n"
        "I2F\n"
        "ICONST 2\n"
        "I2F\n"
        "ICONST 3\n"
        "I2F\n"
        "ICONST 3\n"
        "VECTOR\n"
        "STORE 0\n"
        "VROOT\n"
        "VLOAD 0\n"
        "COPY_VECTOR\n"
        "CALL 0\n"
        "COPY_VECTOR\n"
        "STORE 1\n"
        "VROOT\n"
        "VLOAD 1\n"
        "VLOAD 0\n"
        "VDIV\n"
        "ICONST 1\n"
        "VADDI\n"
        "VPRINT\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}

void testNop() {
    char *code =
        "1 strings\n"
        "0: 2/hi\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=1 type=0 4/main\n"
        "13 instr, 31 bytes\n"
        "GC_START\n"
        "ICONST 3\n"
        "STORE 0\n"
        "ILOAD 0\n"
        "ICONST 0\n"
        "IGT\n"
        "BRF 7\n"
        "NOP\n"
        "BR 7\n"
        "SCONST 0\n"
        "SPRINT\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}

void testFuncMissReturnValue() {
    char *code =
        "0 strings\n"
        "2 functions\n"
        "0: addr=0 args=1 locals=0 type=5 1/f\n"
        "1: addr=57 args=0 locals=0 type=0 4/main\n"
        "30 instr, 68 bytes\n"
        "GC_START\n"
        "ILOAD 0\n"
        "ICONST 0\n"
        "ILT\n"
        "BRF 23\n"
        "ICONST 0\n"
        "I2F\n"
        "ICONST 1\n"
        "VECTOR\n"
        "ILOAD 0\n"
        "VADDI\n"
        "GC_END\n"
        "RET\n"
        "BR 22\n"
        "ICONST 1\n"
        "I2F\n"
        "ICONST 1\n"
        "VECTOR\n"
        "ILOAD 0\n"
        "VADDI\n"
        "STORE 0\n"
        "PUSH_DFLT_RETV\n"
        "RET\n"
        "GC_END\n"
        "GC_START\n"
        "ICONST 3\n"
        "CALL 0\n"
        "VPRINT\n"
        "GC_END\n"
        "HALT";
    run(code);
}

void test_len() {
    char *code =
        "1 strings\n"
        "0: 2/hi\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=4 type=0 4/main\n"
        "24 instr, 62 bytes\n"
        "ICONST 1\n"
        "I2F\n"
        "ICONST 4\n"
        "I2F\n"
        "ICONST 2\n"
        "I2F\n"
        "ICONST 3\n"
        "I2F\n"
        "ICONST 4\n"
        "VECTOR\n"
        "STORE 0\n"
        "SCONST 0\n"
        "STORE 1\n"
        "VLOAD 0\n"
        "VLEN\n"
        "STORE 2\n"
        "SLOAD 1\n"
        "SLEN\n"
        "STORE 3\n"
        "ILOAD 2\n"
        "IPRINT\n"
        "ILOAD 3\n"
        "IPRINT\n"
        "HALT\n";
    run(code);
}

void test_len2() {
    char *code =
        "2 strings\n"
        "0: 5/hello\n"
        "1: 5/world\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=2 type=0 4/main\n"
        "21 instr, 49 bytes\n"
        "SCONST 0\n"
        "STORE 0\n"
        "ICONST 1\n"
        "I2F\n"
        "ICONST 2\n"
        "I2F\n"
        "ICONST 3\n"
        "I2F\n"
        "ICONST 3\n"
        "VECTOR\n"
        "VLEN\n"
        "STORE 1\n"
        "SLOAD 0\n"
        "SLEN\n"
        "SCONST 1\n"
        "SLEN\n"
        "IADD\n"
        "ILOAD 1\n"
        "IADD\n"
        "IPRINT\n"
        "HALT\n";
    run(code);

}

void test_float_div() {
    char *code =
        "0 strings\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=2 type=0 4/main\n"
        "11 instr, 27 bytes\n"
        "GC_START\n"
        "FCONST 1.0\n"
        "STORE 0\n"
        "FCONST 2.0\n"
        "STORE 1\n"
        "FLOAD 1\n"
        "FLOAD 0\n"
        "FSUB\n"
        "FPRINT\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}

void test_div_error() {
    char *code =
    "0 strings\n"
    "1 functions\n"
    "0: addr=0 args=0 locals=0 type=0 4/main\n"
    "21 instr, 53 bytes\n"
    "GC_START\n"
    "ICONST 2\n"
    "I2F\n"
    "ICONST 4\n"
    "I2F\n"
    "ICONST 6\n"
    "I2F\n"
    "ICONST 3\n"
    "VECTOR\n"
    "ICONST 1\n"
    "I2F\n"
    "ICONST 0\n"
    "I2F\n"
    "ICONST 3\n"
    "I2F\n"
    "ICONST 3\n"
    "VECTOR\n"
    "VDIV\n"
    "VPRINT\n"
    "GC_END\n"
    "HALT\n";
    run(code);
}

/*var x = [1,2,3]
 * x[4] = 4
 * print(x)
*/

void test_index_out_of_range() {
    char *code =
        "0 strings\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=1 type=0 4/main\n"
        "20 instr, 50 bytes\n"
        "GC_START\n"
        "ICONST 1\n"
        "I2F\n"
        "ICONST 2\n"
        "I2F\n"
        "ICONST 3\n"
        "I2F\n"
        "ICONST 3\n"
        "VECTOR\n"
        "STORE 0\n"
        "VROOT\n"
        "VLOAD 0\n"
        "ICONST 4\n"
        "ICONST 4\n"
        "I2F\n"
        "STORE_INDEX\n"
        "VLOAD 0\n"
        "VPRINT\n"
        "GC_END\n"
        "HALT\n";
    run(code);
}

void test_need_default_return(){
    char *code = "0 strings\n"
    "2 functions\n"
    "0: addr=0 args=0 locals=0 type=1 1/f\n"
    "1: addr=5 args=0 locals=0 type=0 4/main\n"
    "10 instr, 12 bytes\n"
    "GC_START\n"
    "NOP\n"
    "PUSH_DFLT_RETV\n"
    "RET\n"
    "GC_END\n"
    "GC_START\n"
    "CALL 0\n"
    "IPRINT\n"
    "GC_END\n"
    "HALT\n";
    run(code);
}

int main(int argc, char *argv[]) {
    cunit_setup = setup;
    cunit_teardown = teardown;

    test(string_add);
    test(hello);
    test(int_var_def);
    test(string_var_def);
    test(vector);
    test(func_call);
    test(func_call_two_args);
    test(if_stat);
    test(if_else);
    test(while_stat);
    test(vector_element_assign);
    test(test_int_to_string);
    test(vector_add_int);
    test(string_index);
    test(vector_op);
    test(testNop);
    test(testFuncMissReturnValue);
    test(test_len);
    test(test_len2);
    test(test_float_div);
    test(test_div_error);
    test(test_index_out_of_range);
    test(test_need_default_return);
    return 0;
}

