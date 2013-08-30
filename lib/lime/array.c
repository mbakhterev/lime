#include "construct.h"
#include "util.h"
#include "heap.h"
#include "util.h"

#include <string.h>

#include <assert.h>
#include <stdlib.h>

Array makearray(const int code, const unsigned ilen,
	const ItemCmp icmp, const KeyCmp kcmp) {
	return (Array) {
		.keycmp = kcmp,
		.itemcmp = icmp,
		.data = NULL,
		.index = NULL, 
		.itemlength = ilen,
		.count = 0,
		.code = code
	};
}

void freearray(Array *const a)
{
	assert(a);

	if(a->data)
	{
		assert(a->index && a->count);
		free(a->data);
		free(a->index);

// 		*a = makearray(a->code, a->itemlength, a->itemcmp, a->keycmp);
		
		const Array fresh
			= makearray(a->code,
				a->itemlength, a->itemcmp, a->keycmp);

		memcpy(a, &fresh, sizeof(Array));
	}
}

void *itemat(const Array *const a, const unsigned i) {
	unsigned char *ptr = a->data;
	return ptr + i * a->itemlength;
}

unsigned readin(Array *const a, const void *const p) {
	const unsigned count = a->count;
	const unsigned ilen = a->itemlength;

	unsigned char (*const buf)[a->itemlength]
		= expogrow(a->data, ilen, count);
	
	unsigned *const idx = expogrow(a->index, sizeof(unsigned), count);
		
	memcpy(buf + count, p, sizeof(*buf));
	idx[count] = count;

	a->data = buf;
	a->index = idx;
	a->count += 1;

	heapsort(buf, idx, count + 1, a->itemcmp);

	return count;
}

unsigned lookup(const Array *const a, const void *const key)
{
	return heapsearch(a->data, a->index, a->count, key, a->keycmp);
}
