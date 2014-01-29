#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGRONE 1

// #define DBGFLAGS (DBGRONE)

#define DBGFLAGS 0

static Ref rewrite(
	const Ref, const Array *const map,
	const Array *const verbs, Array *const nodemap);

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

	if(r.code == FREE)
	{
		// Ничего не добавляем в этом случае. Предусмотрено для
		// ситуации, когда мы должны пропустить узел исходного графа

		return 0;
	}

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

static Ref noderemap(const Ref N, const Array *const nodemap)
{
	const Ref n = refmap(nodemap, N);
	if(n.code == FREE)
	{
		// Ссылка ведёт во вне. Её и возвращаем тогда - не наша
		// ответственность

		return N;
	}

	// В противном случае это должен быть узел

	assert(isnode(n));
	return markext(n);
}

static Ref reref(
	const Ref N, const Array *const verbs,
	Array *const nodemap, const Array *const map)
{
	// Убедимся, что клиент наш
	assert(isnode(N) && N.external);

	if(!knownverb(N, verbs))
	{
		// Если нас не просили переписывать ссылки с такими verb-ами,
		// значит и не переписываем. Но ссылка может вести в
		// скопированный узел, который теперь на новом месте. Надо это
		// учесть 

		return noderemap(N, nodemap);
	}

	const Ref val = refmap(map, N);
	
	if(val.code == FREE)
	{
		// Ничего не знаем про эту ссылку, но она может ввести на
		// скопированное определение. Учитываем

		return noderemap(N, nodemap);
	}

	return forkref(val, nodemap);
}

static Ref redef(
	const Ref N, const Array *const verbs,
	Array *const nodemap, const Array *const map)
{
	assert(isnode(N) && !N.external);

	const Ref n = refmap(map, N);

	if(knownverb(N, verbs) && n.code != FREE)
	{
		// Простой случай, когда узел будет заменён на нечто, отличное
		// от самого себя
	}

	return reffree();
}

static Ref rewrite(
	const Ref r, const Array *const map,
	const Array *const verbs, Array *const nodemap)
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
		// Возможны два варианта: (1) мы видим определение узла; (2)
		// видим ссылку

		return (r.external ? reref : redef)(r, verbs, nodemap, map);

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
