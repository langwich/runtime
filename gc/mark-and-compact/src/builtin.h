// TODO: move to template file
// here for testing gc
typedef struct {
	heap_object header; // all bookkeeping information
	size_t length;      // number of doubles
	double data[];      // a label to the start of the data part of vector
} Vector;

typedef struct string {
	heap_object header;
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
		sizeof(Vector),
		0
};

object_metadata String_metadata = {
		"String",
		sizeof(String),
		0
};
