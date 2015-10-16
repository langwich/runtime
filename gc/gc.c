#if defined(MARK_AND_SWEEP)
#include <mark_and_sweep.h>
#elif defined(MARK_AND_COMPACT)
#include <mark_and_compact.h>
#endif

#include "gc.h"
#include "wich.h"

object_metadata Vector_metadata = {
		"Vector",
		0
};

object_metadata String_metadata = {
		"String",
		0
};

Vector *Vector_alloc(size_t length) {
	Vector *p = (Vector *)gc_alloc(&Vector_metadata, sizeof(Vector) + length * sizeof(double));
	p->length = length;
	return p;
}

String *String_alloc(size_t length) {
	String *p = (String *)gc_alloc(&String_metadata, sizeof(String) + length * sizeof(char));
	p->length = length;
	return p;
}
