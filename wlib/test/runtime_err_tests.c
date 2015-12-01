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
#include <cunit.h>
#include <wich.h>


static void setup()		{ }
static void teardown()	{ }

void test_op_null_vector() {
    PVector_ptr x = Vector_new((double []){1,2,3}, 3);
    PVector_ptr y = NIL_VECTOR;
    print_vector(Vector_add(x,y));
}

void test_vector_element_assign_err() {
    PVector_ptr x = Vector_new((double []){1,2,3}, 3);
    set_ith(x,5,5.0);
}

void test_div_zero_err() {
    PVector_ptr x = Vector_new((double []){1,2,3}, 3);
    PVector_ptr y = Vector_new((double []){1,0,3}, 3);
    Vector_div(x,y);
}

void test_vector_op_err() {
    PVector_ptr x = Vector_new((double []){1,2,3}, 3);
    PVector_ptr y = Vector_new((double []){1,2}, 2);
    Vector_add(x,y);

}

void test_vector_index_out_of_range_err() {
    PVector_ptr x = Vector_new((double []){1,2,3}, 3);
    printf("%1.2f\n",ith(x,5));
}

void test_len_null_string_err() {
    printf("%d\n",String_len(NIL_STRING));
}
int main(int argc, char *argv[]) {
    cunit_setup = setup;
    cunit_teardown = teardown;

    test(test_op_null_vector);
    test(test_vector_element_assign_err);
    test(test_div_zero_err);
    test(test_vector_op_err);
    test(test_vector_index_out_of_range_err);
    test(test_len_null_string_err);
    return 0;
}

