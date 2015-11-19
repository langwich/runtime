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
	vm_exec(vm, false);
}

/*
 * print ("hello")
 */
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

/*
 * print ("hello"+"world")
 */
void string_add() {
    char *code =
        "2 strings\n"
        "\t0: 5/hello\n"
        "\t1: 5/world\n"
        "1 functions\n"
        "\t0: addr=0 args=0 locals=0 type=1 4/main\n"
        "5 instr, 9 bytes\n"
        "\tSCONST 0\n"
        "\tSCONST 1\n"
        "\tSADD\n"
        "\tSPRINT\n"
        "\tHALT";
    run(code);
}

/*
 * var x = 1
 */
void int_var_def() {
    char *code =
        "0 strings\n"
        "1 functions\n"
        "\t0: addr=0 args=0 locals=1 type=0 4/main\n"
        "3 instr, 9 bytes\n"
        "\tICONST 1\n"
        "\tSTORE 0\n"
        "\tHALT\n";
    run(code);
}

/*
 * var x = "hello"
 */
void string_var_def() {
    char *code =
        "1 strings\n"
        "\t0: 5/hello\n"
        "1 functions\n"
        "\t0: addr=0 args=0 locals=1 type=0 4/main\n"
        "3 instr, 7 bytes\n"
        "\tSCONST 0\n"
        "\tSTORE 0\n"
        "\tHALT\n";
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
        "9 instr, 28 bytes\n"
        "\tFCONST 1.0\n"
        "\tFCONST 2.0\n"
        "\tFCONST 3.0\n"
        "\tICONST 3\n"
        "\tVECTOR\n"
        "\tSTORE 0\n"
        "\tVLOAD 0\n"
        "\tVPRINT\n"
        "\tHALT\n";
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
        "\t0: addr=0 args=0 locals=0 type=0 1/f\n"
        "\t1: addr=10 args=1 locals=0 type=1 1/g\n"
        "\t2: addr=16 args=0 locals=0 type=0 4/main\n"
        "10 instr, 20 bytes\n"
        "\tICONST 3\n"
        "\tCALL 1\n"
        "\tPOP\n"
        "\tRET\n"
        "\tILOAD 0\n"
        "\tRETV\n"
        "\tPUSH\n"
        "\tRETV\n"
        "\tCALL 0\n"
        "\tHALT";
    run(code);
}

/*
 * func f(q:int){var i = g(q,true) print (i)}
 * func g(z:int,b:boolean):int{ print(b) return z }
 * f(1)
 */
void func_call_with_args() {
    char *code =
        "0 strings\n"
        "3 functions\n"
        "0: addr=0 args=1 locals=1 type=0 1/f\n"
        "1: addr=19 args=2 locals=0 type=1 1/g\n"
        "2: addr=29 args=0 locals=0 type=0 4/main\n"
        "16 instr, 38 bytes\n"
        "ILOAD 0\n"
        "ICONST 1\n"
        "CALL 1\n"
        "STORE 1\n"
        "ILOAD 1\n"
        "IPRINT\n"
        "RET\n"
        "ILOAD 1\n"
        "BPRINT\n"
        "ILOAD 0\n"
        "RETV\n"
        "PUSH\n"
        "RETV\n"
        "ICONST 1\n"
        "CALL 0\n"
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
        "1: addr=21 args=2 locals=0 type=3 1/g\n"
        "2: addr=48 args=0 locals=0 type=0 4/main\n"
        "20 instr, 52 bytes\n"
        "ICONST 3\n"
        "ICONST 1\n"
        "CALL 1\n"
        "STORE 0\n"
        "ILOAD 0\n"
        "BPRINT\n"
        "RET\n"
        "ILOAD 0\n"
        "ILOAD 1\n"
        "IGT\n"
        "BRF 12\n"
        "ICONST 1\n"
        "RETV\n"
        "BR 9\n"
        "ICONST 0\n"
        "RETV\n"
        "PUSH\n"
        "RETV\n"
        "CALL 0\n"
        "HALT";
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
        "\t0: addr=0 args=0 locals=1 type=0 4/main\n"
        "9 instr, 25 bytes\n"
        "\tICONST 3\n"
        "\tSTORE 0\n"
        "\tILOAD 0\n"
        "\tICONST 0\n"
        "\tIGT\n"
        "\tBRF 7\n"
        "\tILOAD 0\n"
        "\tIPRINT\n"
        "\tHALT\n";
    run(code);
}

/*
 * var i = 3
 * if ( i>0 ) print (i)
 * else print (1);
 */
void if_else() {
    char *code =
        "0 strings\n"
        "1 functions\n"
        "\t0: addr=0 args=0 locals=1 type=0 4/main\n"
        "12 instr, 32 bytes\n"
        "\tICONST 3\n"
        "\tSTORE 0\n"
        "\tILOAD 0\n"
        "\tICONST 0\n"
        "\tIGT\n"
        "\tBRF 14\n"
        "\tILOAD 0\n"
        "\tIPRINT\n"
        "\tBR 7\n"
        "\tICONST 1\n"
        "\tIPRINT\n"
        "\tHALT\n";
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
        "\t14 instr, 40 bytes\n"
        "\tICONST 0\n"
        "\tSTORE 0\n"
        "\tILOAD 0\n"
        "\tICONST 10\n"
        "\tILT\n"
        "\tBRF 18\n"
        "\tILOAD 0\n"
        "\tICONST 1\n"
        "\tIADD\n"
        "\tSTORE 0\n"
        "\tBR -24\n"
        "\tILOAD 0\n"
        "\tIPRINT\n"
        "\tHALT\n";
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
        "\t0: addr=0 args=0 locals=1 type=0 4/main\n"
        "13 instr, 42 bytes\n"
        "\tFCONST 1.0\n"
        "\tFCONST 2.0\n"
        "\tFCONST 3.0\n"
        "\tICONST 3\n"
        "\tVECTOR\n"
        "\tSTORE 0\n"
        "\tVLOAD 0\n"
        "\tICONST 1\n"
        "\tFCONST 4.0\n"
        "\tSTORE_INDEX\n"
        "\tVLOAD 0\n"
        "\tVPRINT\n"
        "\tHALT\n";
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
        "\t0: 3/car\n"
        "\t1: 3/cat\n"
        "1 functions\n"
        "\t0: addr=0 args=0 locals=3 type=0 4/main\n"
        "15 instr, 39 bytes\n"
        "\tSCONST 0\n"
        "\tSTORE 0\n"
        "\tSCONST 1\n"
        "\tSTORE 1\n"
        "\tSLOAD 0\n"
        "\tICONST 1\n"
        "\tSLOAD_INDEX \n"
        "\tSLOAD 1\n"
        "\tICONST 2\n"
        "\tSLOAD_INDEX\n"
        "\tSADD\n"
        "\tSTORE 2\n"
        "\tSLOAD 2\n"
        "\tSPRINT\n"
        "\tHALT\n";
    run(code);
}
/*
 * func fib(x:int) : int {
 *      if (x <= 1) { return 1 }
 *      return fib(x-1) + fib(x-2)
 *  }
 * print(fib(5))
 */

void fib() {
    char *code =
        "0 strings\n"
        "2 functions\n"
        "0: addr=0 args=1 locals=0 type=1 3/fib\n"
        "1: addr=46 args=0 locals=0 type=0 4/main\n"
        "22 instr, 56 bytes\n"
        "ILOAD 0\n"
        "ICONST 1\n"
        "ILE\n"
        "BRF 9\n"
        "ICONST 1\n"
        "RETV\n"
        "ILOAD 0\n"
        "ICONST 1\n"
        "ISUB\n"
        "CALL 0\n"
        "ILOAD 0\n"
        "ICONST 2\n"
        "ISUB\n"
        "CALL 0\n"
        "IADD\n"
        "RETV\n"
        "PUSH\n"
        "RETV\n"
        "ICONST 5\n"
        "CALL 0\n"
        "IPRINT\n"
        "HALT";
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
        "1: addr=12 args=0 locals=2 type=0 4/main\n"
        "25 instr, 63 bytes\n"
        "VLOAD 0\n"
        "ICONST 2\n"
        "VMULI\n"
        "RETV\n"
        "PUSH\n"
        "RETV\n"
        "ICONST 1\n"
        "I2F\n"
        "ICONST 2\n"
        "I2F\n"
        "ICONST 3\n"
        "I2F\n"
        "ICONST 3\n"
        "VECTOR\n"
        "STORE 0\n"
        "VLOAD 0\n"
        "CALL 0\n"
        "STORE 1\n"
        "VLOAD 1\n"
        "VLOAD 0\n"
        "VDIV\n"
        "ICONST 1\n"
        "VADDI\n"
        "VPRINT\n"
        "HALT";
    run(code);
}

void op_boolean_vars() {
    char *code = "0 strings\n"
        "3 functions\n"
        "0: addr=0 args=1 locals=0 type=3 3/foo\n"
        "1: addr=12 args=1 locals=0 type=3 3/bar\n"
        "2: addr=41 args=0 locals=2 type=0 4/main\n"
        "28 instr, 72 bytes\n"
        "ILOAD 0\n"
        "ICONST 10\n"
        "ILT\n"
        "RETV\n"
        "PUSH\n"
        "RETV\n"
        "ILOAD 0\n"
        "ICONST 1\n"
        "ILT\n"
        "BRF 12\n"
        "ICONST 1\n"
        "RETV\n"
        "BR 9\n"
        "ICONST 0\n"
        "RETV\n"
        "PUSH\n"
        "RETV\n"
        "ICONST 5\n"
        "CALL 1\n"
        "STORE 0\n"
        "ICONST 1\n"
        "CALL 0\n"
        "STORE 1\n"
        "ILOAD 0\n"
        "ILOAD 1\n"
        "OR\n"
        "BPRINT\n"
        "HALT\n";
    run(code);
}

void testNestedBlock() {
    char *code =
        "4 strings\n"
        "0: 3/cat\n"
        "1: 3/dog\n"
        "2: 3/moo\n"
        "3: 3/boo\n"
        "2 functions\n"
        "0: addr=0 args=1 locals=5 type=1 1/f\n"
        "1: addr=53 args=0 locals=0 type=0 4/main\n"
        "26 instr, 70 bytes\n"
        "ICONST 32\n"
        "STORE 1\n"
        "SCONST 0\n"
        "STORE 2\n"
        "SCONST 1\n"
        "STORE 4\n"
        "SCONST 2\n"
        "STORE 5\n"
        "ILOAD 1\n"
        "RETV\n"
        "SCONST 3\n"
        "STORE 4\n"
        "ICONST 7\n"
        "I2F\n"
        "ICONST 1\n"
        "VECTOR\n"
        "STORE 3\n"
        "PUSH\n"
        "RETV\n"
        "ICONST 1\n"
        "I2F\n"
        "ICONST 1\n"
        "VECTOR\n"
        "CALL 0\n"
        "IPRINT\n"
        "HALT\n";
    run(code);
}

void testNop() {
    char *code =
        "1 strings\n"
        "0: 2/hi\n"
        "1 functions\n"
        "0: addr=0 args=0 locals=1 type=0 4/main\n"
        "11 instr, 29 bytes\n"
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
        "HALT\n";
    run(code);
}

void testFuncMissReturnValue() {
    char *code =
        "0 strings\n"
        "2 functions\n"
        "0: addr=0 args=1 locals=0 type=5 1/f\n"
        "1: addr=53 args=0 locals=0 type=0 4/main\n"
        "25 instr, 63 bytes\n"
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
        "RETV\n"
        "BR 22\n"
        "ICONST 1\n"
        "I2F\n"
        "ICONST 1\n"
        "VECTOR\n"
        "ILOAD 0\n"
        "VADDI\n"
        "STORE 0\n"
        "PUSH\n"
        "RETV\n"
        "ICONST 3\n"
        "CALL 0\n"
        "VPRINT\n"
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
        "GC_S\n"
        "FCONST 1.0\n"
        "STORE 0\n"
        "FCONST 2.0\n"
        "STORE 1\n"
        "FLOAD 1\n"
        "FLOAD 0\n"
        "FSUB\n"
        "FPRINT\n"
        "GC_E\n"
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
    test(func_call_with_args);
    test(func_call_two_args);
    test(if_stat);
    test(if_else);
    test(while_stat);
    test(vector_element_assign);
    test(test_int_to_string);
    test(vector_add_int);
    test(string_index);
    test(fib);
    test(vector_op);
    test(op_boolean_vars);
    test(testNestedBlock);
    test(testNop);
    test(testFuncMissReturnValue);
    test(test_len);
    test(test_len2);
    test(test_float_div);
    return 0;
}

