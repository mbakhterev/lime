#include "construct.h"
#include "util.h"
#include "heap.h"
#include "util.h"

#include <string.h>

#include <assert.h>
#include <stdlib.h>

static Array *freearrays = NULL;

static Array *tipoffarray(Array **const lptr)
{
	Array *const l = *lptr;
	assert(l);

	Array *const a = l->u.nextfree;

	if(l != a)
	{
		l->u.nextfree = a->u.nextfree;
	}
	else
	{
		*lptr = NULL;
	}
	
	return a;
}

Array *newarray(
	const unsigned code, const unsigned ilen,
	const ItemCmp icmp, const KeyCmp kcmp)
{
	assert(ilen && icmp && kcmp);
	assert(code != FREE);

	Array *a = NULL;

	if(freearrays)
	{
		a = tipoffarray(&freearrays);
		assert(a->code == FREE);
	}
	else
	{
		a = malloc(sizeof(Array));
		assert(a);
	}

	memcpy(a,
		&(Array) {
			.keycmp = kcmp,
			.itemcmp = icmp,
			.u.data = NULL,
			.index = NULL, 
			.itemlength = ilen,
			.count = 0,
			.code = code }, sizeof(Array));
	
	return a;
}

void freearray(Array *const a)
{
	assert(a && a->code != FREE);

	if(a->u.data)
	{
		assert(a->index && a->count);
		free(a->u.data);
		free(a->index);
	}

	memcpy(a,
		&(Array) {
			.keycmp = NULL,
			.itemcmp = NULL,
			.index = NULL,
			.itemlength = 0,
			.count = 0,
			.code = FREE }, sizeof(Array));
	
	if(freearrays == NULL)
	{
		a->u.nextfree = a;
		freearrays = a;
		return;
	}

	freearrays->u.nextfree = a;
	a->u.nextfree = freearrays;
	freearrays = a;
}

void *itemat(const Array *const a, const unsigned i)
{
	unsigned char *ptr = a->u.data;
	return ptr + i * a->itemlength;
}

unsigned readin(Array *const a, const void *const p)
{
	const unsigned count = a->count;
	const unsigned ilen = a->itemlength;

	unsigned char (*const buf)[a->itemlength]
		= expogrow(a->u.data, ilen, count);
	
	unsigned *const idx = expogrow(a->index, sizeof(unsigned), count);
		
	memcpy(buf + count, p, sizeof(*buf));
	idx[count] = count;

	a->u.data = buf;
	a->index = idx;
	a->count += 1;

	heapsort(buf, idx, count + 1, a->itemcmp);

	return count;
}

unsigned lookup(const Array *const a, const void *const key)
{
	return heapsearch(a->u.data, a->index, a->count, key, a->keycmp);
}
