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

#if defined(MARK_AND_SWEEP)
#include <mark_and_sweep.h>
#elif defined(MARK_AND_COMPACT)
#include <mark_and_compact.h>
#endif

#include "gc.h"
#include "wich.h"

object_metadata PVector_metadata = {
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
	PVector *p = (PVector *)gc_alloc(&PVector_metadata, sizeof(PVector) + length * sizeof(PVectorFatNode));
	p->length = length;
	return p;
}

PVectorFatNodeElem *PVectorFatNodeElem_alloc() {
	PVectorFatNodeElem *p = (PVectorFatNodeElem *)gc_alloc(&PVectorFatNodeElem_metadata, sizeof(PVectorFatNodeElem));
	return p;
}

String *String_alloc(size_t length) {
	String *p = (String *)gc_alloc(&String_metadata, sizeof(String) + (length+1) * sizeof(char));
	p->length = length;
	return p;
}
