#include "construct.h"
#include "util.h"

#include <assert.h>

// NWState - Node Walk State. Current используется в смысле "течение".

typedef struct
{
// 	const Array *dagmap;
// 	const Array *divemap;
	const DagMap *dagmap;
	WalkOne walk;
	void *current;
} NWState;

static void assertuimap(const Array *const M)
{
	assert(M == NULL || M->code == MAP);
}

static int pernode(List *const l, void *const ptr)
{
	assert(l && l->ref.code == NODE);
	const NWState *const st = ptr;
	assert(st);

	const Node *const n = l->ref.u.node;
	assert(n);

// 	const Array *const M = st->dagmap;
// 	const Array *const dive = st->divemap;

	const DagMap *const M = st->dagmap;
	const WalkOne walk = st->walk;
	void *const current = st->current;

// 	if(uireverse(M, n->verb) == -1)
	if(!isdag(M, n->verb))
	{
		walk(l, current);
	}
	else
	{
// 		if(uireverse(dive, n->verb) != -1)
		if(isgodag(M, n->verb))
		{
// 			walkdag(n->u.attributes, M, dive, walk, current);
			walkdag(n->u.attributes, M, walk, current);
		}
	}

	return 0;
}

void walkdag(
	const List *const dag,
// 	const Array *const M, const Array *const dive,
	const DagMap *const M, const WalkOne walk, void *const ptr)
{
	assert(M);
	assertuimap(&M->map);
	assertuimap(&M->go);

	assert(walk);

	NWState st =
	{
		.dagmap = M,
//		.divemap = dive,
		.walk = walk,
		.current = ptr
	};

	forlist((List *)dag, pernode, &st, 0);
}


