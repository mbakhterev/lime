#include "construct.h"
#include "util.h"

#include <string.h>

#define DBGER	1

// #define DBGFLAGS (DBGER)
#define DBGFLAGS 0

#include <assert.h>
#include <stdlib.h>

Array mkarray(const int code, const unsigned ilen,
	const ItemCmp icmp, const KeyCmp kcmp) {
	return (Array) {
		.keycmp = kcmp,
		.itemcmp = icmp,
		.data = NULL,
		.index = NULL, 
		.capacity = 0,
		.itemlength = ilen,
		.count = 0,
		.code = code
	};
}

void *attach(Array *const a, const void *const p) {
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
