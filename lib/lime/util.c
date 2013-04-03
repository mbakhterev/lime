#include "util.h"

#include <stdlib.h>
#include <assert.h>

#define DBGEG 1

#define DBGFLAGS (DBGEG)

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

void *expogrow(void *const buf, const unsigned ilen, const unsigned cnt) {
	const unsigned curlen = clp2(ilen * cnt);
	assert(curlen < MAXNUM);
	const unsigned len = clp2(ilen * (cnt + 1));
	assert(len && len < MAXNUM);

	if(len <= curlen) {
		return buf;
	}

	DBG(DBGEG, "resizing for buf: %p; il: %u; count: %u; len: %u -> %u",
		buf, ilen, cnt, curlen, len);
	
	void *const p = realloc(buf, len);
	assert(p);

// 	a->capacity = len;
// 	a->data = p;

	return p;
}
