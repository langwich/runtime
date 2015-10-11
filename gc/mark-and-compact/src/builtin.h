#include "gc.h"

// Define Vector and String builtin types for Wich
// This is here for testing but should likely go somewhere else as it is generic and
// useful for refcounting and any GC implementation. Only requires heap_object definition.
typedef struct {
	heap_object header; // all bookkeeping information
	int length;         // number of doubles
	double data[];      // a label to the start of the data part of vector
} Vector;

typedef struct string {
	heap_object header;
	int length;      // number of chars
	char str[];
	/* the string starts at the end of fixed fields; this field
	 * does not take any room in the structure; it's really just a
	 * label for the element beyond the length field. So, there is no
	 * need set this field. You must, however, copy strings into it.
	 * You cannot set p->str = "foo";
	 * Must terminate with '\0';
	 */
} String;

object_metadata Vector_metadata = {
		"Vector",
		0
};

object_metadata String_metadata = {
		"String",
		0
};

static inline Vector *Vector_alloc(int length) {
	Vector *p = (Vector *)gc_alloc(&Vector_metadata, sizeof(Vector) + length * sizeof(double));
	p->length = length;
	return p;
}

static inline String *String_alloc(int length) {
	String *p = (String *)gc_alloc(&String_metadata, sizeof(String) + length * sizeof(char));
	p->length = length;
	return p;
}
