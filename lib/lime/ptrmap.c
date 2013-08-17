#include "construct.h"
#include "util.h"

#include <assert.h>

static int kcmp(const void *const D, const unsigned i, const void *const key)
{
	const void *const *const P = D;
	return cmpptr(P[i], *(const void *const *)key);
}

static int icmp(const void *const D, const unsigned i, const unsigned j)
{
	return kcmp(D, i, (const void *const *const)D + j);
}

Array makeptrmap(void)
{
	return makearray(PTR, sizeof(const void *const), icmp, kcmp);
}

void freeptrmap(Array *const m)
{
	assert(m);
	assert(m->code == PTR);
	freearray(m);
}

unsigned ptrmap(Array *const m, const void *const y)
{
	assert(m);

	const unsigned k = ptrreverse(m, y);
	if(k != -1) { return k; }

	return readin(m, &y);
}

const void *const ptrdirect(const Array *const m, const unsigned x)
{
	if(m)
	{
		assert(m->code == PTR);

		if(x < m->count)
		{
			const void *const *const P = m->data;
			return P[x];
		}
	}

	return NULL;
}

unsigned ptrreverse(const Array *const m, const void *const y)
{
	if(m)
	{
		assert(m->code == PTR);
		return lookup(m, &y);
	}

	return -1;
}
