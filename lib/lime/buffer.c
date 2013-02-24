#include "buffer.h"

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

void * exporesize(void *const buff, unsigned * plen, unsigned size) {
	unsigned len = clp2(*plen * size);
	assert(len);

	void *const p = realloc(buff, len);
	assert(p);

	// Первый assert гарантирует, что size != 0
	*plen = len / size;

	return p;
}
