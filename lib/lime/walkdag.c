#include "construct.h"
#include "util.h"

#include <assert.h>

// NWState - Node Walk State. Current используется в смысле "течение".

typedef struct
{
// 	const Array *const map;
	const Array *const go;
	const WalkOne walk;
	void *const current;
} NWState;

static void assertuimap(const Array *const M)
{
	assert(M == NULL || M->code == MAP);
}

static unsigned inmap(const Array *const M, const unsigned verb)
{
	return uireverse(M, verb) != -1;
}

static int pernode(List *const l, void *const ptr)
{
	assert(l && l->ref.code == NODE);
	const NWState *const st = ptr;
	assert(st);

	const Node *const n = l->ref.u.node;
	assert(n);

// 	const Array *const map = st->map;
	const Array *const go = st->go;

	const WalkOne walk = st->walk;
	void *const current = st->current;

//	if(!inmap(map, n->verb))
	if(!n->dag)
	{
		walk(l, current);
	}
	else
	{
// 		if(isgodag(M, n->verb))
		if(inmap(go, n->verb))
		{
// 			walkdag(n->u.attributes, map, go, walk, current);
			walkdag(n->u.attributes, go, walk, current);
		}
	}

	return 0;
}

void walkdag(
	const List *const dag,
// 	const Array *const map,
	const Array *const go,
	const WalkOne walk, void *const ptr)
{
// 	assert(M);
// 	assertuimap(&M->map);
// 	assertuimap(&M->go);

// 	assertuimap(map);
	assertuimap(go);

	assert(walk);

	NWState st =
	{
//		.dagmap = M,
		.go = go,
// 		.map = map,
		.walk = walk,
		.current = ptr
	};

	forlist((List *)dag, pernode, &st, 0);
}


