#include "construct.h"
#include "util.h"

#include <assert.h>

#define LNODE 0

static const char *const verbs[] =
{
	[LNODE] = "L",
	NULL
};

typedef struct
{
	Array *const U;
	Array *const verbs;
	Array *const lmarks;

	const Array *const escape;
} LState;

static void eval(const Ref, LState *const);
static int evalone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	eval(l->ref, ptr);
	return 0;
}

static void eval(const Ref N, LState *const S)
{
	switch(N.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		return;
	
	case LIST:
	case DAG:
		forlist(N.u.list, evalone, S, 0);
		return;
	
	case NODE:
		if(N.external)
		{
			return;
		}

		switch(nodeverb(N, S->verbs))
		{
		case LNODE:
			tunerefmap(S->lmarks, N, nodeattribute(N));
			return;

		default:
			if(!knownverb(N, S->escape))
			{
				eval(nodeattribute(N), S);
			}
		}
		
		return;

	default:
		assert(0);
	}
}

Ref leval(Array *const U, const Ref dag, const Array *const escape)
{
	LState st =
	{
		.U = U,
		.verbs = newverbmap(U, 0, verbs),
		.lmarks = newkeymap(),
		.escape = escape
	};

	eval(dag, &st);

	const Array *const torewrite = newverbmap(U, 0, ES("L"));
	const Ref r = exprewrite(dag, st.lmarks, torewrite);

	freekeymap((Array *)torewrite);
	freekeymap(st.lmarks);
	freekeymap((Array *)st.verbs);

	return r;
}
