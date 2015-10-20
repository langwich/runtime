#if defined(MARK_AND_SWEEP)
#include <mark_and_sweep.h>
#elif defined(MARK_AND_COMPACT)
#include <mark_and_compact.h>
#include <persistent_vector.h>

#endif

#include "gc.h"
#include "wich.h"

object_metadata Vector_metadata = {
		"PVector",
		0
};

object_metadata PVectorFatNodeElem_metadata = {
		"PVectorFatNodeElem",
		1,
		{__offsetof(PVectorFatNodeElem,next)}
};

object_metadata String_metadata = {
		"String",
		0
};

PVector *PVector_alloc(size_t length) {
	PVector *p = (PVector *)gc_alloc(&Vector_metadata, sizeof(PVector) + length * sizeof(PVectorFatNodeElem));
	p->length = length;
	return p;
}

PVectorFatNodeElem *PVectorFatNodeElem_alloc() {
	PVectorFatNodeElem *p = (PVectorFatNodeElem *)gc_alloc(&PVectorFatNodeElem_metadata, sizeof(PVectorFatNodeElem));
	return p;
}

String *String_alloc(size_t length) {
	String *p = (String *)gc_alloc(&String_metadata, sizeof(String) + length * sizeof(char));
	p->length = length;
	return p;
}
