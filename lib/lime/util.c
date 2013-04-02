#include "util.h"

unsigned middle(const unsigned a, const unsigned b) {
	return a + (b - a) / 2;
}

int cmpui(const unsigned a, const unsigned b) {
	return 1 - (a == b) - ((a < b) << 1);
}

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

void * expogrow(const void *ptr, const unsigned ilen, const unsigned count) {
	const unsigned len = clp2(a->itemlength * count);
	assert(len && len < MAXNUM);

	if(a->capacity >= len) {
		return a->data;
	}

	DBG(DBGER, "resizing for buf: %p; il: %u; count: %u; len: %u",
		a->data, a->itemlength, count, len);
	
	void *const p = realloc(a->data, len);
	assert(p);

	a->capacity = len;
	a->data = p;

	return p;
}
