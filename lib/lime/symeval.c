#include "construct.h"
#include "util.h"

#include <assert.h>

// В основном, списано из typeeval. S ведут себя похожим на TEnv образом

typedef struct
{
	Array *const U;
	Array *const symmarks;

	const Array *const escape;
	const Array *const envmarks;
	const Array *const typemarks;

	const Array *const verbs;
} EState;

static void eval(const Ref N, EState *const E);

static int evalone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);

	eval(l->ref, ptr);

	return 0;
}

#define SNODE 0

static const char *const verbs[] =
{
	[SNODE] = "S",
	NULL
};

static const Binding *getexisting(
	const Array *const env, Array *const U, const Ref key)
{
	List *const l
		= tracepath(
			env, U,
			readpack(U, strpack(0, "ENV")),
			readpack(U, strpack(0, "parent")));
	
	const Ref K = decorate(dynamark(key), U, ATOM);
	const Binding *const b = pathlookup(l, K, NULL);

	freeref(K);
	freelist(l);

	return b;
}

static const Binding *setnew(
	Array *const env, Array *const U, const Ref key, const Ref typeref)
{
	if(typeref.code != TYPE)
	{
		return NULL;
	}

	const Ref K = decorate(forkref(key, NULL), U, ATOM);
	Binding *const b = mapreadin(env, K);

	if(!b)
	{
		freeref(K);
	}

	b->ref = typeref;

	return b;
}

static void snode(const Ref N, EState *const E)
{
	const Ref r = nodeattribute(N);
	if(r.code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting attribute list",
			atombytes(atomat(E->U, nodeverb(N, NULL))));

		return;
	}

	const unsigned len = listlen(r.u.list);

	if(len < 1 || 2 < len)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting 1 or 2 attributes",
			atombytes(atomat(E->U, nodeverb(N, NULL))));

		// Это символично, конечно же. Вообще, надо возвращать именно
		// ошибку и делать что-то в стиле монад, но на Си это сложно.
		// Поэтому оставляем до следующей версии

		return;
	}

	Ref R[len];
	writerefs(r.u.list, R, len);

	if(!isbasickey(R[0]))
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting 1st attributes to be basic key",
			atombytes(atomat(E->U, nodeverb(N, NULL))));

		return;
	}

	const Ref typeref = len == 2 ? refmap(E->typemarks, R[1]) : reffree();

	Array *const env = envmap(E->envmarks, N);

	if(!env)
	{
		item = nodeline(N);
		ERR("node \"%s\": no environment definition",
			atombytes(atomat(E->U, nodeverb(N, NULL))));
		
		return;
	}

	const Binding *const b
		= (len == 1) ? getexisting(env, E->U, R[0])
		: (len == 2) ? setnew(env, E->U, R[0], typeref)
		: NULL;
	
	if(!b)
	{
		char *const strkey = strref(E->U, NULL, R[0]);
		
		item = nodeline(N);
		ERR("node \"%s\": can't %s symbol for key: %s",
			atombytes(atomat(E->U, nodeverb(N, NULL))),
			(len == 1) ? "locate" : "allocate",
			strkey);

		free(strkey);
		return;
	}

	tuneptrmap(E->symmarks, N, (Binding *)b);
}

static void evalnode(const Ref N, EState *const E)
{
// 	if(N.external)
// 	{
// 		return;
// 	}

	switch(nodeverb(N, E->verbs))
	{
	case SNODE:
		snode(N, E);
		break;
	}
}

static void eval(const Ref N, EState *const E)
{
	switch(N.code)
	{
	case NUMBER:
	case ATOM:
		return;
	
	case LIST:
		forlist(N.u.list, evalone, E, 0);
		return;
	
	case NODE:
		if(N.external)
		{
			return;
		}

		if(knownverb(N, E->verbs))
		{
			evalnode(N, E);
			return;
		}

		if(!knownverb(N, E->escape))
		{
			eval(nodeattribute(N), E);
		}

		return;
	
	default:
		assert(0);
		
	}
}

void symeval(
	Array *const U,
	Array *const symmarks,
	const Ref dag, const Array *const escape,
	const Array *const envmarks, const Array *typemarks)
{
	EState E =
	{
		.U = U,
		.symmarks = symmarks,

		.typemarks = typemarks,
		.envmarks = envmarks,
		.escape = escape,

		.verbs = newverbmap(U, 0, verbs)
	};

	eval(dag, &E);

	freekeymap((Array *)E.verbs);
}