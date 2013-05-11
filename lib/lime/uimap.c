#include "construct.h"
#include "util.h"

#include <assert.h>

static int kcmp(const void *const D, const unsigned i, const void *const key)
{
	const unsigned *const U = D;
	return cmpui(U[i], *(const unsigned *)key);
}

static int icmp(const void *const D, const unsigned i, const unsigned j)
{
	return kcmp(D, i, (const unsigned *)D + j);
}

Array makeuimap(void)
{
	return makearray(MAP, sizeof(unsigned), icmp, kcmp);
}

void freeuimap(Array *const m)
{
	assert(m->code == MAP);
	freearray(m);
}

unsigned uimap(Array *const m, const unsigned y)
{
	const unsigned k = uireverse(m, y);
	if(k != -1) { return k; }

	return readin(m, &y);
}

unsigned uidirect(const Array *const m, const unsigned x)
{
	assert(m->code == MAP);

	if(x < m->count)
	{
		const unsigned *const U = m->data;
		return U[x];
	}

	return -1;
}

unsigned uireverse(const Array *const m, const unsigned y)
{
	assert(m->code == MAP);
	const unsigned x = lookup(m, &y);
	return x;
}
