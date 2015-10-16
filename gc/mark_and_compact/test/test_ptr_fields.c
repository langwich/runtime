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

#include <cunit.h>
#include <wich.h>

const size_t HEAP_SIZE = 2000;

typedef struct {
	heap_object header;

	int userid;
	int parking_spot;
	float salary;
	String *name;
} User;

typedef struct _Employee {
	heap_object header;

	int ID;
	String *name;
	struct _Employee *mgr;
} Employee;

object_metadata User_metadata = {
	"User",
	1,
	{__offsetof(User,name)}
};

object_metadata Employee_metadata = {
	"Employee",
	2,
	{__offsetof(Employee,name),
	 __offsetof(Employee,mgr)}
};

User *create_one_user(char *name);
Employee *create_one_employee(char *name);

Heap_Info verify_heap() {
	Heap_Info info = get_heap_info();
	assert_equal(info.heap_size, HEAP_SIZE);
	assert_equal(info.busy_size, info.computed_busy_size);
	assert_equal(info.heap_size, info.busy_size+info.free_size);
	return info;
}

static void setup()		{ gc_init(HEAP_SIZE); }
static void teardown()	{ verify_heap(); gc_shutdown(); }

void one_user_set_ptr_null() {
	User *u = create_one_user("parrt");
	gc_add_root((void **)&u);

	Heap_Info info = get_heap_info();
	size_t u_expected_size = align_to_word_boundary(sizeof(User));
	size_t uname_expected_size = align_to_word_boundary(sizeof(String) + u->name->length * sizeof(char));
	assert_addr_equal(u, info.start_of_heap);
	assert_addr_equal(info.next_free, ((void *)u->name) + uname_expected_size);
	assert_equal(info.busy_size, u_expected_size + uname_expected_size);
	assert_equal(info.free_size, info.heap_size - (u_expected_size + uname_expected_size));

	assert_equal(gc_num_live_objects(), 2);
	gc();
	assert_equal(gc_num_live_objects(), 2);

	u = NULL; // should free user and string
	gc();
	assert_equal(gc_num_live_objects(), 0);

	info = get_heap_info();
	assert_equal(info.busy_size, 0);
	assert_equal(info.free_size, info.heap_size);
}

void one_user_ptr_out_of_scope() {
	gc_begin_func();

	User *u = create_one_user("parrt");
	gc_add_root((void **)&u);

	Heap_Info info = get_heap_info();
	size_t u_expected_size = align_to_word_boundary(sizeof(User));
	size_t uname_expected_size = align_to_word_boundary(sizeof(String) + u->name->length * sizeof(char));
	assert_addr_equal(u, info.start_of_heap);
	assert_addr_equal(info.next_free, ((void *)u->name) + uname_expected_size);
	assert_equal(info.busy_size, u_expected_size + uname_expected_size);
	assert_equal(info.free_size, info.heap_size - (u_expected_size + uname_expected_size));

	assert_equal(gc_num_live_objects(), 2);
	gc();
	assert_equal(gc_num_live_objects(), 2);

	gc_end_func(); // u is out of scope as far as gc is concerned

	gc();
	assert_equal(gc_num_live_objects(), 0);

	info = get_heap_info();
	assert_equal(info.busy_size, 0);
	assert_equal(info.free_size, info.heap_size);
}

void two_employees_set_ptr_null() { // just one root pointing to parrt that points at tombu
	Employee *e = create_one_employee("parrt");
	gc_add_root((void **)&e);
	assert_equal(gc_num_live_objects(), 2);
	String *last_object = e->name;

	Heap_Info info = get_heap_info();
	size_t e_expected_size = align_to_word_boundary(sizeof(Employee));
	size_t ename_expected_size = align_to_word_boundary(sizeof(String) + last_object->length * sizeof(char));
	assert_addr_equal(e, info.start_of_heap);
	assert_addr_equal(info.next_free, ((void *) last_object) + ename_expected_size);
	assert_equal(info.busy_size, e_expected_size + ename_expected_size);
	assert_equal(info.free_size, info.heap_size - (e_expected_size + ename_expected_size));

	e->mgr = create_one_employee("tombu");
	last_object = e->mgr->name;
	assert_equal(gc_num_live_objects(), 4);

	info = get_heap_info();
	assert_addr_equal(e->mgr, ((void *)e->name) + ename_expected_size);
	assert_addr_equal(info.next_free, ((void *) last_object) + ename_expected_size);
	assert_equal(info.busy_size, 2 * (e_expected_size + ename_expected_size));
	assert_equal(info.free_size, info.heap_size - 2 * (e_expected_size + ename_expected_size));

	e = NULL; // should free employee and string
	gc();
	assert_equal(gc_num_live_objects(), 0);

	info = get_heap_info();
	assert_equal(info.busy_size, 0);
	assert_equal(info.free_size, info.heap_size);
}

void one_employee_in_cycle_set_ptr_null() { // just one root pointing to parrt that points at itself
	Employee *e = create_one_employee("parrt");
	gc_add_root((void **)&e);
	assert_equal(gc_num_live_objects(), 2);
	String *last_object = e->name;

	Heap_Info info = get_heap_info();
	size_t e_expected_size = align_to_word_boundary(sizeof(Employee));
	size_t ename_expected_size = align_to_word_boundary(sizeof(String) + last_object->length * sizeof(char));
	assert_addr_equal(e, info.start_of_heap);
	assert_addr_equal(info.next_free, ((void *) last_object) + ename_expected_size);
	assert_equal(info.busy_size, e_expected_size + ename_expected_size);
	assert_equal(info.free_size, info.heap_size - (e_expected_size + ename_expected_size));

	e->mgr = e; // parrt is mgr of himself
	last_object = e->mgr->name;
	assert_equal(gc_num_live_objects(), 2);

	info = get_heap_info();
	assert_addr_equal(e, info.start_of_heap);
	assert_addr_equal(info.next_free, ((void *) last_object) + ename_expected_size);
	assert_equal(info.busy_size, e_expected_size + ename_expected_size);
	assert_equal(info.free_size, info.heap_size - (e_expected_size + ename_expected_size));

	e = NULL; // should free employee and string
	gc();	  // should not get into infinite loop during trace
	assert_equal(gc_num_live_objects(), 0);

	info = get_heap_info();
	assert_equal(info.busy_size, 0);
	assert_equal(info.free_size, info.heap_size);
}

User *create_one_user(char *name) {
	User *u = (User *)gc_alloc(&User_metadata, sizeof(User));
	u->userid = 99;
	u->parking_spot = 0xAAA;
	u->salary = 0.01;
	u->name = String_alloc(20);
	strcpy(u->name->str, name);

	return u;
}

Employee *create_one_employee(char *name) {
	Employee *e = (Employee *)gc_alloc(&Employee_metadata, sizeof(User));
	e->name = String_alloc(20);
	strcpy(e->name->str, name);

	return e;
}

int main(int argc, char *argv[]) {
	cunit_setup = setup;
	cunit_teardown = teardown;

	test(one_user_set_ptr_null);
	test(one_user_ptr_out_of_scope);
	test(two_employees_set_ptr_null);
	test(one_employee_in_cycle_set_ptr_null);

	return 0;
}