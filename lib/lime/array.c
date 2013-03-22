#include "array.h"

#include "util.h"

#define DBGER	1

// #define DBGFLAGS (DBGER)
#define DBGFLAGS 0

#include <assert.h>
#include <stdlib.h>

static unsigned clp2(unsigned n) {
	assert(sizeof(unsigned) == 4);

	n -= 1;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return n + 1;
}

void * exporesize(Array *const a, unsigned count) {
	const unsigned len = clp2(a->itemlength * count);
	assert(len);

	DBG(DBGER, "resizing for buf: %p; il: %u; count: %u; len: %u",
		a->buffer, a->itemlength, count, len);
	
	void *const p = realloc(a->buffer, len);
	assert(p);

	a->capacity = len;
	a->buffer = p;

	return p;
}

Array mkarray(const unsigned ilen) {
	return (Array) { .buffer = NULL, .capacity = 0, .itemlength = ilen };
}
