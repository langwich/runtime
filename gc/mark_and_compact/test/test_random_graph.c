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
#include "bitset.h"

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

static const int NUM_NODES = 40;
static const int MAX_EDGES = 4;

typedef struct _Node {
	heap_object header;
	unsigned int ID;
	struct _Node *edges[MAX_EDGES];
} Node;

object_metadata Node_metadata = {
		"Node",
		MAX_EDGES,
		{__offsetof(Node,edges)+0*sizeof(void *),
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

set wack_random_roots(Node **nodes, int attempt_to_free);
Node *create_node(unsigned int ID);
void ensure_valid_node(heap_object *p);
void sniff_for_dead_nodes(heap_object *p);
char *Node_toString(Node *n);
void print_heap_object(heap_object *p);
void get_reachable(set *which, heap_object *p);
set get_reachable_objects(heap_object **roots);
void make_all_nodes_roots(Node **nodes);
void add_random_roots(Node **nodes);
void check_not_marked(heap_object *p);


// ------------------------ T E S T S ------------------------

void add_random_edges(Node *const *nodes);
Node **create_nodes();

void _many_connected_nodes_free_all_at_once(bool edges) {
	gc_begin_func();

	Node **nodes = create_nodes();
	if ( edges ) add_random_edges(nodes);
	make_all_nodes_roots(nodes);

	assert_equal(gc_num_live_objects(), NUM_NODES);

	set allnodes = get_reachable_objects((heap_object **) nodes);
	assert_equal(set_deg(allnodes), NUM_NODES);
//	printf("reachable = ");	set_print(allnodes);

	gc_end_func();

	gc(); // should kill all
	assert_equal(gc_num_live_objects(), 0);
	// after compaction, should be no dead nodes
	foreach_object(sniff_for_dead_nodes);

	foreach_live(check_not_marked);

	Heap_Info info = get_heap_info();
	assert_equal(info.busy_size, 0);
	assert_equal(info.free_size, info.heap_size);
}

void many_disconnected_nodes_free_all_at_once() {
	_many_connected_nodes_free_all_at_once(true);
}

void many_connected_nodes_free_all_at_once() {
	_many_connected_nodes_free_all_at_once(true);
}

void _many_connected_nodes_wack_random_roots(bool edges) {
	Node **nodes = create_nodes();
	if ( edges ) add_random_edges(nodes);
	make_all_nodes_roots(nodes);

	set allnodes = get_reachable_objects((heap_object **) nodes);
	assert_equal(set_deg(allnodes), NUM_NODES);
//	printf("reachable = ");	set_print(allnodes);

	gc(); // should do nothing

	set after_gc = get_reachable_objects((heap_object **) nodes);
	assert_true(set_equ(after_gc, allnodes));

	assert_equal(gc_num_live_objects(), NUM_NODES);

	set wacked = wack_random_roots(nodes, NUM_NODES/1.5);
//	printf("wacked = "); set_print(wacked);
	set after_wacking = get_reachable_objects((heap_object **) nodes);

	gc(); // should kill a *max* of num wacked; edges point amongst nodes

	after_gc = get_reachable_objects((heap_object **) nodes);
//	printf("reachable_after_gc = "); set_print(after_gc);
	assert_true(set_equ(after_gc, after_wacking));
	assert_false(set_equ(after_gc, allnodes));

	foreach_live(check_not_marked);

	// after compaction, should be no dead nodes
	foreach_object(sniff_for_dead_nodes);
}

void many_connected_nodes_wack_random_roots() {
	_many_connected_nodes_wack_random_roots(true);
}

void many_disconnected_nodes_wack_random_roots() {
	_many_connected_nodes_wack_random_roots(false);
}

// ------------------------ S U P P O R T ------------------------

void check_not_marked(heap_object *p) {
	printf("check_not_marked %s\n", Node_toString((Node *)p));
	if ( p->marked ) {
		printf("whoa");
	}
	assert_false(p->marked);
}

Node *create_node(unsigned int ID) {
	Node *n = (Node *)gc_alloc(&Node_metadata, sizeof(Node));
	if ( n!=NULL ) {
		n->ID = ID;
	}

	return n;
}

Node **create_nodes() {
	Node **nodes = calloc(NUM_NODES, sizeof(Node *));
	for (int i=0; i<NUM_NODES; i++) {
		nodes[i] = create_node(i);
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
//		printf("LIVE node %s\n", n->name->str);
	}
	else { // must be string
		String *s = (String *)p;
		assert_not_equal(s->length, 0);
		assert_str_not_equal(s->str, "wacked");
//		printf("LIVE string %s\n", s->str);
	}
}

set wack_random_roots(Node **nodes, int attempt_to_free) {
	set wacked = empty;
	for (int i=0; i<attempt_to_free; i++) { // kill 1/3 of nodes and then compact
		int node_index = rand() % NUM_NODES; // get number from 0..NUM_NODES-1
		Node *p = nodes[node_index];
		if ( p!=NULL ) {
//			printf("attempt to wack %s\n", p->name->str);
			set_orel(p->ID, &wacked);
			p->ID = (unsigned int)-1; // wack node
			nodes[node_index] = NULL; // kill root
		}
	}
	return wacked;
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
	sprintf(buf, "Node %d %s-> {", n->ID, n->header.marked ? "(marked) " : "");
	strcat(s, buf);
	for (int i=0; i<MAX_EDGES; i++) {
		sprintf(buf, " %d", n->edges[i]->ID);
		strcat(s, buf);
	}
	strcat(s, " }");
	return s; // a leak but just for debugging
}

set get_reachable_objects(heap_object **roots) {
	set which = empty;
	for (int i = 0; i < NUM_NODES; i++) {
		if ( roots[i]!=NULL ) {
			get_reachable(&which, roots[i]);
		}
	}
	return which;
}

/* Recursive method to compute reachability; used to compare with GC algorithm of liveness.
 * USES THE MARKED BIT. destructive.
 */
void get_reachable(set *which, heap_object *p) {
	if ( p->metadata == &Node_metadata ) {
		Node *n = (Node *)p;
		if ( set_el(n->ID, *which) ) return;
		set_orel(n->ID, which);
//		printf("REACH %s", Node_toString(p));
		for (int i=0; i<MAX_EDGES; i++) {
			if ( n->edges[i]!=NULL ) {
				get_reachable(which, (heap_object *) n->edges[i]);
			}
		}
//		printf("REACH %s count %d\n", Node_toString(p), count);
		return;
	}
	else { // must be string w/o edges
		fprintf(stderr, "Not a node\n");
	}
}

int main(int argc, char *argv[]) {
	srand((unsigned) time(NULL));

	cunit_setup = setup;
	cunit_teardown = teardown;

//	gc_debug(true);

	test(many_disconnected_nodes_free_all_at_once);
	test(many_disconnected_nodes_wack_random_roots);
//
	test(many_connected_nodes_free_all_at_once);
	test(many_connected_nodes_wack_random_roots);

	return 0;
}