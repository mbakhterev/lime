#include "construct.h"
#include "util.h"

#include <assert.h>

#define FIN 0U
#define NTH 1U
#define SNODE 2U
#define TNODE 3U
#define TENV 4U

static const char *const verbs[] =
{
	[FIN] = "FIn",
	[NTH] = "Nth",
	[SNODE] = "S",
	[TNODE] = "T",
	[TENV] = "TEnv"
};

typedef struct
{
	const Array *const U;
	const Array *const escape;
	const Array *const verbs;

	const List *const inlist;

	// Исходные данные для разбора. Символы и типы, из которых можно Nth-ами
	// извлечь компоненты

	const Array *const typemarks;
	const Array *const symmarks;
	
	// Соответствия узлов Nth и FIn выражениям

	Array *const evalmarks;

} NState;

typedef struct
{
	const unsigned from;
	const unsigned to;
	const unsigned valid;
} Index;

static void eval(const Ref, NState *const);

static int evalone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	eval(l->ref, ptr);
	return 0;
}

typedef struct
{
	Ref L;
	const Array *const verbs;
} DCState;

static unsigned reftoidx(
	const Ref r, const Array *const verbs, unsigned *const correct)
{
	assert(r.code <= ATOM);
	*correct = 0;

	switch(r.code)
	{
	case NUMBER:
		assert(r.u.number < MAXNUM);
		*correct = 1;
		return r.u.number;
	
	case ATOM:
		// Нам здесь везёт, что конец списка обозначается так же, как и
		// узел T

		if(verbmap(verbs, r.u.number) == TNODE)
		{
			*correct = 1;
			return -1;
		}

		break;
	}

	assert(0);
	return -1;
}

static Index index(const Ref r, const Array *const verbs)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	{
		unsigned valid = 0;
		const unsigned idx = reftoidx(r, verbs, &valid);
		return (Index) { .from = idx, .to = idx, .valid = valid };
	}

	case LIST:
	{
		const unsigned len = listlen(r.u.list);
		const Ref R[len];
		writerefs(r.u.list, (Ref *)R, len);

		if(len != 2)
		{
			break;
		}

		unsigned valid[2];
		const unsigned from = reftoidx(R[0], verbs, valid);
		const unsigned to = reftoidx(R[1], verbs, valid + 1);

		return (Index)
		{
			.from = from,
			.to = to,
			.valid = valid[0] && valid[1]
		};
	}
	}

	return (Index) { .from = -1, .to = -1, .valid = 0 };
}

static Ref reexpress(List *const l)
{
	if(listlen(l) == 1)
	{
		return l->ref;
	}

	return reflist(l);
}

static int indexone(List *const i, void *const ptr)
{
	assert(i);
	assert(ptr);
	DCState *const st = ptr;
	const Index idx = index(i->ref, st->verbs);

	if(!idx.valid)
	{
		// Прекращаем этот проход, если получили не пару индексов
		return 1;
	}

	if(st->L.code != LIST)
	{
		// Прекращаем этот проход, если нас просят разобрать не список
		return 1;
	}

	unsigned valid = 0;
	List *const l = forklistcut(st->L.u.list, idx.from, idx.to, &valid);

	if(!idx.valid)
	{
		freelist(l);
		return 1;
	}

	freeref(st->L);

	// Здесь нужен небольшой грязный хак отождествления списков из одного
	// элемента с самим этим элементом

	st->L = reexpress(l);

	return 0;
}


static List *deconstruct(const Ref N, NState *const S)
{
	const Ref r = nodeattribute(N);
	if(r.code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting attribute list",
			atombytes(atomat(S->U, nodeverb(N, NULL))));

		return NULL;
	}

	const unsigned len = listlen(r.u.list);

	if(len != 2)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure",
			atombytes(atomat(S->U, nodeverb(N, NULL))));

		return NULL;
	}

	const Ref R[len];
	writerefs(r.u.list, (Ref *)R, len);

	if(!isnode(R[0]) || !knownverb(N, S->verbs) || R[1].code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure",
			atombytes(atomat(S->U, nodeverb(N, NULL))));

		return NULL;
	}

	// Параметры разобрали, теперь нужно понять, какой список нужно
	// разобрать на кусочки

	forlist(NULL, indexone, NULL, 0);
	return NULL;
}

static void eval(const Ref N, NState *const S)
{
	switch(N.code)
	{
	case NUMBER:
	case ATOM:
		return;

	case LIST:
		forlist(N.u.list, evalone, S, 0);
		return;
	
	case NODE:
		if(N.external)
		{
			// Традиционно, нам интересны только определения узлов
			return;
		}

		switch(nodeverb(N, S->verbs))
		{
		case FIN:
			tunerefmap(S->evalmarks, N, reflist((List *)S->inlist));
			return;

		case NTH:
			tunerefmap(S->evalmarks, N, reflist(deconstruct(N, S)));
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

Ref ntheval(
	const Array *const U,
	const Ref dag, const Array *const escape,
	const Array *const symmarks, const Array *const typemarks,
	const List *const inlist)
{
	NState st =
	{
		.U = U,
		.escape = escape,
		.symmarks = symmarks,
		.typemarks = typemarks,
		.inlist = inlist,
		.evalmarks = newkeymap()
	};

	eval(dag, &st);

	freekeymap(st.evalmarks);

	return reffree();
}
