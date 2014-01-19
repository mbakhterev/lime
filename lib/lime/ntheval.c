#include <construct.h>
#include <util.h>

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
} Index;

static int evalone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);

	NState *const st = ptr;
}

static List *deconstruct(const Ref N, NState *const S)
{
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
