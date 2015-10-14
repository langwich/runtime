#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cunit.h"
#include <persistent_vector.h>

int main(int argc, char *argv[])
{
	// var x = [1,2,3]
	PVector_ptr x = PVector_new((double[]){1,2,3}, 3);
	assert_equal(x.version, 0);
	assert_str_equal(PVector_as_string(x), "[1.00, 2.00, 3.00]");

	// check default values
	assert_float_equal(ith(x,0), 1.0);
	assert_float_equal(ith(x,1), 2.0);
	assert_float_equal(ith(x,2), 3.0);

	PVector_ptr y = PVector_copy(x);
	assert_equal(y.version, 1);

	// check default values via new pointer
	assert_float_equal(ith(y,0), 1.0);
	assert_float_equal(ith(y,1), 2.0);
	assert_float_equal(ith(y,2), 3.0);

	set_ith(x, 1, 102.00);
	assert_float_equal(ith(x,1), 102.0);
	assert_float_equal(ith(y,1), 2.0);      // our copy doesn't change.

	set_ith(y, 1, 302.00);                  // change copy now
	assert_float_equal(ith(x,1), 102.0);
	assert_float_equal(ith(y,1), 302.0);

	set_ith(y, 1, 902.00);                  // change copy now
	assert_float_equal(ith(x,1), 102.0);
	assert_float_equal(ith(y,1), 902.0);

	printf("done\n");

	return 0;
}
