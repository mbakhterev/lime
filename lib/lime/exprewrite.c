#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGRONE 1

// #define DBGFLAGS (DBGRONE)

#define DBGFLAGS 0

static Ref rewrite(
	const Ref, const Array *const map, const Array *const verbs, Array *const nodemap);

typedef struct
{
	List *result;
	Array *const nodemap;
	const Array *const map;
	const Array *const verbs;
} RState;

static int rewriteone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);

	RState *const st = ptr;

	const Ref r = rewrite(l->ref, st->map, st->verbs, st->nodemap);

	DBG(DBGRONE, "%s", "rewriting");

	if(r.code != LIST || l->ref.code == LIST)
	{
		// Если получился не список, или если изначально переписывался
		// список, то добавляем узел, как он есть

		st->result = append(st->result, RL(r));
		return 0;
	}

	st->result = append(st->result, r.u.list);
	return 0;
}

static Ref rewrite(
	const Ref r, const Array *const map, const Array *const verbs,
	Array *const nodemap)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		return r;
	
	case MAP:
		assert(r.external);
		return r;
	
	case LIST:
	{
		RState st =
		{
			.result = NULL,
			.map = map,
			.verbs = verbs,
			.nodemap = nodemap
		};

		forlist(r.u.list, rewriteone, &st, 0);

		return reflist(st.result);
	}

	case NODE:
	{
		// Переписываем мы только ссылки. И такова должна быть структура
		// выражения

		assert(isnode(r) && r.external);

		if(!knownverb(r, verbs))
		{
			// Если нас не попросили переписывать ссылки с такими
			// verb-ами

			return r;
		}

		const Ref val = refmap(map, r);
		
		if(val.code == FREE)
		{
			// Ничего не знаем про эту ссылку
			return r;
		}

		return forkref(val, nodemap);
	}

	default:
		assert(0);
	}

	return reffree();
}

Ref exprewrite(const Ref r, const Array *const map, const Array *const verbs)
{
	Array *const nodemap = newkeymap();
	const Ref rnew = rewrite(r, map, verbs, nodemap);
	freekeymap(nodemap);
	return rnew;
}
