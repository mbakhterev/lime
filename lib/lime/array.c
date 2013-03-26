#include "array.h"
#include "util.h"

#include <string.h>

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

	if(a->capacity >= len) {
		return a->buffer;
	}

	DBG(DBGER, "resizing for buf: %p; il: %u; count: %u; len: %u",
		a->buffer, a->itemlength, count, len);
	
	void *const p = realloc(a->buffer, len);
	assert(p);

	a->capacity = len;
	a->buffer = p;

	return p;
}

Array mkarray(const unsigned ilen) {
	return (Array) {
		.buffer = NULL,
		.capacity = 0,
		.itemlength = ilen,
		.count = 0
	};
}

void * append(Array *const a, const void *const p) {
	const unsigned count = a->count;
	unsigned char (*const buf)[a->itemlength] = exporesize(a, count + 1);
	a->count += 1;
	memcpy(buf + count, p, sizeof(*buf));
	return buf + count;
}

void freearray(Array *const a) {
	assert(a->buffer 
		&& a->capacity
		&& a->itemlength * a->count < a->capacity);
	
	free(a->buffer);
	a->buffer = 0;
	a->capacity = 0;
	a->count = 0;
	a->itemlength = 0;
}
