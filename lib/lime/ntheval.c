#include "construct.h"
#include "util.h"

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

static int evalone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	eval(l->ref, ptr);
	return 0;
}

typedef struct
{
	List *L;
} DCState;

static unsigned reftoidx(
	const Ref r, const Array *const verbs, unsigned *const correct)
{
	assert(r.code <= ATOM);
	*correct = 0;

	switch(r.code)
	{
	case NUMBER:
		assert(r.u.number < MAXINT);
		*correct = 1;
		return r.u.number;
	
	case ATOM:
		// Нам здесь везёт, что конец списка обозначается так же, как и
		// узел T

		if(verbmap(verbs, r) == TNODE)
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
		const unsigned idx = reftoidx(r, verbs, &valid)
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

	return (Index) { .from = -1, .to = -1, valid = 0 };
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

	if(!isnode(R[0]) || !knowverb(N, S->verbs) || R[1].code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure",
			atombytes(atomat(S->U, nodeverb(N, NULL))));

		return NULL;
	}
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
				eval(nodeattribute(B), S);
			}
		}

		return;
	
	default:
		assert(0);
	}
}
