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

static Ref reref(
	const Ref N, const Array *const verbs,
	Array *const nodemap, const Array *const map)
{
	// Убедимся, что клиент наш
	assert(isnode(N) && N.external);

	if(!knownverb(N, verbs))
	{
		// Если нас не просили переписывать ссылки с такими verb-ами,
		// значит и не переписываем. Возвращаем как есть. Ссылка будет
		// выправлена на нужный узел на следующей стадии обработки (fix)

		return N;
	}

	const Ref val = refmap(map, N);
	
	if(val.code == FREE)
	{
		// Ничего не знаем про эту ссылку. Рассчитываем на то, что она
		// будет выправлена в процессе fix

		return N;
	}

	// Возвращаем копию значения из отображения. Если и появятся какие-то
	// внешние ссылки в этом месте, то они будут исправлены на fix-стадии

	return forkref(val, nodemap);
}

static Ref redef(
	const Ref N, const Array *const verbs,
	Array *const nodemap, const Array *const map)
{
	assert(isnode(N) && !N.external);

	// Имеем дело с определением узла. Надо его скопировать. Перед этим
	// выполнив подстановки в его атрибутах.

	const Ref n
		= newnode(
			nodeverb(N, NULL),
			rewrite(nodeattribute(N), map, verbs, nodemap),
			nodeline(N));
	
	tunerefmap(nodemap, N, n);
	
	return n;
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
	case DAG:
	{
		RState st =
		{
			.result = NULL,
			.map = map,
			.verbs = verbs,
			.nodemap = nodemap
		};

		forlist(r.u.list, rewriteone, &st, 0);

// 		return reflist(st.result);
		return (r.code == LIST ? reflist : refdag)(st.result);
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

static int fixone(List *const l, void *const ptr);

static void fix(Ref *const r, const Array *const nodemap)
{
	switch(r->code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		// Это всё нелокальные ссылки, их менять не нужно
		break;	

	case MAP:
		assert(r->external);
		break;

	case LIST:
	case DAG:
		forlist(r->u.list, fixone, (void *)nodemap, 0);
		break;

	case NODE:
		// В этом случае нам интересны: (1) локальные ссылки - это
		// (Ref.external == 1), о которых знает nodemap; (2) аттрибуты в
		// определениях узлов

		if(r->external)
		{
			const Ref n = refmap(nodemap, *r);
			if(n.code == NODE)
			{
				r->u.list = n.u.list;
			}
			else
			{
				assert(n.code == FREE);
			}
		}
		else
		{
			fix((Ref *)nodeattributecell(*r), nodemap);
		}

		break;
		
	default:
		assert(0);
	}
}

int fixone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	fix(&l->ref, ptr);
	return 0;
}

Ref exprewrite(const Ref r, const Array *const map, const Array *const verbs)
{
	Array *const nodemap = newkeymap();
	Ref q = rewrite(r, map, verbs, nodemap);
	fix(&q, nodemap);
	freekeymap(nodemap);
	return q;
}

static int simplerewriteone(List *const l, void *const ptr);

typedef struct
{
	List *L;
	const Array *const map;
} SRState;

Ref simplerewrite(const Ref r, const Array *const map)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
	case SYM:
	case ENV:
		return r;
	
	case NODE:
		assert(r.external);
		return refmap(map, r);
	
	case LIST:
	{
		SRState st =
		{
			.L = NULL,
			.map = map
		};

		if(forlist(r.u.list, simplerewriteone, &st, 0))
		{
			freelist(st.L);
			return reffree();
		}

		return reflist(st.L);
	}
	
	default:
		assert(0);
	}

	return reffree();
}

int simplerewriteone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	SRState *const S = ptr;

	const Ref r = simplerewrite(l->ref, S->map);
	if(r.code == FREE)
	{
		return !0;
	}

	if(l->ref.code == NODE && r.code == LIST)
	{
		// Если исходная ссылка была ссылкой на узел, а результат
		// является ссылкой на список, то приписываем этот список к
		// результату

		S->L = append(S->L, forklist(r.u.list));
		return 0;
	}

	S->L = append(S->L, RL(r));
	return 0;
}

