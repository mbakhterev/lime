#include "atomtab.h"
#include "buffer.h"

#include <assert.h>

void resetatomtab(AtomTable *const t) {
	t->count = 0;
}

unsigned rdatom(AtomTable *const t, FILE *const f) {
	unsigned len;
	unsigned hint;
	assert(fscanf(f, "%u.%u", &hint, &len) == 2);

	return 0;
}
