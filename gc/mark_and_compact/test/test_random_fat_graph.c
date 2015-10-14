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
#include <mark_and_compact.h>
#include <wich.h>
#include <cunit.h>
#include <string.h>
#include <time.h>
#include <gc.h>

const size_t HEAP_SIZE = 20000000;

Heap_Info verify_heap() {
	Heap_Info info = get_heap_info();
	assert_equal(info.heap_size, HEAP_SIZE);
	assert_equal(info.busy_size, info.computed_busy_size);
	assert_equal(info.heap_size, info.busy_size+info.free_size);
	return info;
}

static int __save_root_count = 0;
static void setup()		{ __save_root_count = gc_num_roots(); gc_init(HEAP_SIZE); }
static void teardown()	{ gc_set_num_roots(__save_root_count); verify_heap(); gc_shutdown(); }

static const int NUM_NODES = 100;
static const int MAX_EDGES = 1;

typedef struct _Node {
	heap_object header;
	int ID;
	String *name;
	struct _Node *edges[MAX_EDGES];
} Node;

object_metadata Node_metadata = {
		"Node",
		1+MAX_EDGES,
		{__offsetof(Node,name),
		 __offsetof(Node,edges)+0*sizeof(void *),
		 __offsetof(Node,edges)+1*sizeof(void *),
		 __offsetof(Node,edges)+2*sizeof(void *),
		 __offsetof(Node,edges)+3*sizeof(void *),
//	 __offsetof(Node,edges)+4*sizeof(void *),
//	 __offsetof(Node,edges)+5*sizeof(void *),
//	 __offsetof(Node,edges)+6*sizeof(void *),
//	 __offsetof(Node,edges)+7*sizeof(void *),
//	 __offsetof(Node,edges)+8*sizeof(void *),
//	 __offsetof(Node,edges)+9*sizeof(void *),
		}
};

static bool unreachable[NUM_NODES]; // freed[i] true if free_random_nodes() wacked root to node->ID == i

int wack_random_roots_and_targeted_nodes(Node **nodes, int attempt_to_free);
int wack_random_roots(Node **nodes, int attempt_to_free);
Node *create_node(int ID, char *name);
Node *create_node0(int ID);
void ensure_valid_node(heap_object *p);
void sniff_for_dead_nodes(heap_object *p);
char *Node_toString(Node *n);
void print_heap_object(heap_object *p);
int get_num_reachable(heap_object *p);
int get_num_reachable_objects(heap_object **roots);
void make_all_nodes_roots(Node **nodes);
void add_random_roots(Node **nodes);


// ------------------------ T E S T S ------------------------

void add_random_edges(Node *const *nodes);
Node **create_nodes();

void many_disconnected_nodes_free_all_at_once() {
	gc_begin_func();

	create_nodes();

	gc(); // should do nothing
	assert_equal(gc_num_live_objects(), 2*NUM_NODES); // 1 for node, 1 for name String

	gc_end_func();

	gc(); // should kill all
	assert_equal(gc_num_live_objects(), 0);

	Heap_Info info = get_heap_info();
	assert_equal(info.busy_size, 0);
	assert_equal(info.free_size, info.heap_size);
}

void many_disconnected_nodes_wack_random_roots() {
	Node **nodes = create_nodes();

	gc(); // should do nothing
	assert_equal(gc_num_live_objects(), 2*NUM_NODES); // 1 for node, 1 for name String

	int num_wacked = wack_random_roots_and_targeted_nodes(nodes, NUM_NODES/3);
	//	printf("wacked %d nodes\n", num_wacked);

	gc_mark();
	foreach_live(ensure_valid_node);
	gc_unmark();

	gc(); // should kill num_wacked
	assert_equal(gc_num_live_objects(), 2*(NUM_NODES - num_wacked));

	// after compaction, should be no dead nodes
	foreach_object(sniff_for_dead_nodes);
}

void many_connected_nodes_free_all_at_once() {
	gc_begin_func();

	Node **nodes = create_nodes();

	add_random_edges(nodes);

	gc(); // should do nothing
	assert_equal(gc_num_live_objects(), 2*NUM_NODES); // 1 for node, 1 for name String

	gc_end_func();

	gc(); // should kill all
	assert_equal(gc_num_live_objects(), 0);

	Heap_Info info = get_heap_info();
	assert_equal(info.busy_size, 0);
	assert_equal(info.free_size, info.heap_size);
}

static inline void check_not_marked(heap_object *p) {
	if ( p->marked ) {
		printf("whoa");
	}
	assert_false(p->marked);
}

void many_connected_nodes_wack_random_roots() {
	Node **nodes = create_nodes();
	make_all_nodes_roots(nodes);

	add_random_edges(nodes);

	int recursive_method_count = get_num_reachable_objects(nodes);
	assert_equal(recursive_method_count, NUM_NODES * 2);

//	foreach_object(print_heap_object);

//	gc(); // should do nothing
//	assert_equal(gc_num_live_objects(), 2*NUM_NODES); // 1 for node, 1 for name String

	int num_wacked = wack_random_roots(nodes, NUM_NODES*2);
	printf("wacked %d roots\n", num_wacked);

	int num_live_before_gc = gc_num_live_objects();
	assert_true(num_live_before_gc < NUM_NODES*2);

	int recursive_method_count_before_gc = get_num_reachable_objects(nodes);
	assert_equal(num_live_before_gc, recursive_method_count_before_gc);

	int num_valid_roots = NUM_NODES - num_wacked;
	printf("num_valid_roots %d\n", num_valid_roots);

	gc_mark();
	foreach_live(ensure_valid_node);
	gc_unmark();

	gc(); // should kill a *max* of num_wacked; edges point amongst nodes

	foreach_live(check_not_marked);

	int num_live_after_gc = gc_num_live_objects();

	assert_equal(num_live_after_gc, num_live_before_gc);

	assert_not_equal(num_live_after_gc, NUM_NODES); // must have killed at least some nodes

	// after compaction, should be no dead nodes
	foreach_object(sniff_for_dead_nodes);
}

// ------------------------ S U P P O R T ------------------------

Node *create_node(int ID, char *name) {
	Node *n = (Node *)gc_alloc(&Node_metadata, sizeof(Node));
	if ( n!=NULL ) {
		n->name = String_alloc(20);
		n->ID = ID;
		strcpy(n->name->str, name);
	}

	return n;
}

Node **create_nodes() {
	Node **nodes = calloc(NUM_NODES, sizeof(Node *));
	char buf[255];
	memset(buf, '\0', 255);
	for (int i=0; i<NUM_NODES; i++) {
		sprintf(buf, "node%d", i);
		nodes[i] = create_node(i, buf);
	}
	return nodes;
}

void make_all_nodes_roots(Node **nodes) {
	for (int i=0; i<NUM_NODES; i++) {
		if (nodes[i] != NULL) {
			gc_add_root((void **) &nodes[i]);
		}
	}
}

void add_random_roots(Node **nodes) {
	for (int i=0; i<NUM_NODES; i++) {
		if (nodes[i] != NULL) {
			gc_add_root((void **) &nodes[i]);
		}
	}
}

void add_random_edges(Node *const *nodes) {// connect each node to MAX_EDGES random nodes
	for (int i=0; i<NUM_NODES; i++) {
		Node *n = nodes[i];
		for (int e=0; e<MAX_EDGES; e++) { // duplicate edges are possible!
			int target = rand() % NUM_NODES;
			n->edges[e] = nodes[target];
		}
	}
}

void ensure_valid_node(heap_object *p) {
	if ( p->metadata == &Node_metadata ) {
		Node *n = (Node *) p;
		assert_true(ptr_is_in_heap(p));
		assert_not_equal(n->ID, -1);
		assert_false(unreachable[n->ID]); // is this a node we made unreachable?
		assert_not_equal(n->name, NULL);
		assert_str_not_equal(n->name->str, "wacked");
//		printf("LIVE node %s\n", n->name->str);
	}
	else { // must be string
		String *s = (String *)p;
		assert_not_equal(s->length, 0);
		assert_not_equal(strlen(s->str), 0);
//		assert_equal(strlen(s->str), s->length);
//		printf("LIVE string %s\n", s->str);
	}
}

void sniff_for_dead_nodes(heap_object *p) { // should NOT be any dead nodes after GC compacts
	if ( p->metadata == &Node_metadata ) {
		Node *n = (Node *) p;
		assert_true(ptr_is_in_heap(p));
		assert_not_equal(n->ID, -1);
		assert_not_equal(n->name, NULL);
		assert_str_not_equal(n->name->str, "wacked"); // also tests that n->name->str is a valid address
//		printf("LIVE node %s\n", n->name->str);
	}
	else { // must be string
		String *s = (String *)p;
		assert_not_equal(s->length, 0);
		assert_str_not_equal(s->str, "wacked");
//		printf("LIVE string %s\n", s->str);
	}
}

/* wack roots but also set nodes directly pointed to from roots to wacky values */
int wack_random_roots_and_targeted_nodes(Node **nodes, int attempt_to_free) {
	memset(unreachable, false, NUM_NODES);
	int num_wacked = 0; // random num could hit same index
	for (int i=0; i<attempt_to_free; i++) { // kill 1/3 of nodes and then compact
		int node_index = rand() % NUM_NODES; // get number from 0..NUM_NODES-1
		Node *p = nodes[node_index];
		if ( p!=NULL ) {
//			printf("attempt to wack %s\n", p->name->str);
			num_wacked++;
			// wack the node and the string it points to, which will also be garbage
			p->ID = -1;
			strcpy(p->name->str, "wacked"); // overwrite name for error checking
			unreachable[node_index] = true;
			nodes[node_index] = NULL; // kill root
		}
	}
	return num_wacked;
}

int wack_random_roots(Node **nodes, int attempt_to_free) {
	memset(unreachable, false, NUM_NODES);
	int num_wacked = 0; // random num could hit same index
	for (int i=0; i<attempt_to_free; i++) { // kill 1/3 of nodes and then compact
		int node_index = rand() % NUM_NODES; // get number from 0..NUM_NODES-1
		Node *p = nodes[node_index];
		if ( p!=NULL ) {
//			printf("attempt to wack %s\n", p->name->str);
			num_wacked++;
			nodes[node_index] = NULL; // kill root
		}
	}
	return num_wacked;
}

void print_heap_object(heap_object *p) {
	if ( p->metadata == &Node_metadata ) {
		Node *n = (Node *) p;
		printf("%s", Node_toString(n));
	}
	else { // must be string
		String *s = (String *)p;
		printf("String %s\n", s->str);
	}
}

char *Node_toString(Node *n) {
	char *s = malloc(1000);
	char buf[200];
	sprintf(buf, "Node %s\n", n->name->str);
	strcat(s, buf);
	for (int i=0; i<MAX_EDGES; i++) {
		sprintf(buf, "  -> %s\n", n->edges[i]->name->str);
		strcat(s, buf);
	}
	return s; // a leak but just for debugging
}

int get_num_reachable_objects(heap_object **roots) {
	gc_unmark();
	int count = 0;
	for (int i = 0; i < NUM_NODES; i++) {
		if ( roots[i]!=NULL ) {
			count += get_num_reachable(roots[i]);
		}
	}
	gc_unmark();
	return count;
}

/* Recursive method to compute reachability; used to compare with GC algorithm of liveness.
 * USES THE MARKED BIT. destructive.
 */
int get_num_reachable(heap_object *p) {
	if ( p->marked ) return 0;
	p->marked = true;
	if ( p->metadata == &Node_metadata ) {
		Node *n = (Node *)p;
//		printf("REACH %s", Node_toString(p));
		int count = 1; // we at least got here
		count += get_num_reachable(n->name);
		for (int i=0; i<MAX_EDGES; i++) {
			if ( n->edges[i]!=NULL ) {
				count += get_num_reachable((heap_object *) n->edges[i]);
			}
		}
//		printf("REACH %s count %d\n", Node_toString(p), count);
		return count;
	}
	else { // must be string w/o edges
		String *s = (String *)p;
//		printf("REACH %s", s->str);
		return 1;
	}
}

int main(int argc, char *argv[]) {
	srand((unsigned) time(NULL));

	cunit_setup = setup;
	cunit_teardown = teardown;

	gc_debug(true);

//	test(many_disconnected_nodes_free_all_at_once);
//	test(many_disconnected_nodes_wack_random_roots);
//
//	test(many_connected_nodes_free_all_at_once);
	test(many_connected_nodes_wack_random_roots);

	return 0;
}