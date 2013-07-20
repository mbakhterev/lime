#include "construct.h"
#include "util.h"

#include <assert.h>

// NWState - Node Walk State. Current используется в смысле "течение".

typedef struct
{
	const Array *dagmap;
	const Array *divemap;
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

	const Array *const M = st->dagmap;
	const Array *const dive = st->divemap;
	const WalkOne walk = st->walk;
	void *const current = st->current;

	if(uireverse(M, n->verb) == -1)
	{
		walk(l, current);
	}
	else
	{
		if(uireverse(dive, n->verb) != -1)
		{
			walkdag(n->u.attributes, M, dive, walk, current);
		}
	}

	return 0;
}

void walkdag(
	const List *const dag, const Array *const M, const Array *const dive,
	const WalkOne walk, void *const ptr)
{
	assertuimap(M);
	assertuimap(dive);
	assert(walk);

	NWState st =
	{
		.dagmap = M,
		.divemap = dive,
		.walk = walk,
		.current = ptr
	};

	forlist((List *)dag, pernode, &st, 0);
}

#define LNODE	0
#define LNTH	1

// Expand State - структура для сбора информации о процессе переписывания
// атрибутов

typedef struct
{
	List *expanded;
	const Array *verbs;
} EState;

static List *expandlistrefs(const List *const, const Array *const verbs);

// FIXME: По уму здесь надо бы накапливать куски списка аттриботов, в которах
// нет ссылок на L-узлы и целыми такими кусками цеплять всё в expanded.

static int rewriteref(List *const l, void *const ptr)
{
	EState *const st = ptr;

	// Что будет добавлено в результат
	List *e = l;

	if(l->ref.code != NODE)
	{
		// Добавляется один элемент
		e->next = e;
	}
	else
	{
		const Node *const n = l->ref.u.node;
		const unsigned key = uireverse(st->verbs, n->verb);

		switch(key)
		{
		case LNODE:
			e = forklist(n->u.attributes);
			break;

		default:
			e->next = e;
		}
	}

	append(st->expanded, e);
	return 0;
}

static List *expandlistrefs(const List *const l, const Array *const verbs)
{

	EState st = { .expanded = NULL, .verbs = verbs };
	forlist(l->ref.u.node->u.attributes, rewriteref, &st, 0);
	return st.expanded;
}

static void rewriteone(List *const l, void *const ptr)
{
	// В этой функции уже известно, что l содержит ссылку на узел
	Node *const n = l->ref.u.node;

	const Array *const verbs = ptr;
	const unsigned key = uireverse(verbs, n->verb);

	switch(key)
	{
	case -1:
	case LNODE:
// 		forlist(l->ref.u.node->u.attributes, rewriteref, ptr, 0);
		n->u.attributes = expandlistrefs(n->u.attributes, verbs);
		break;
	
	case LNTH:
		break;
	
	default:
		assert(0);
	}
}

static const char *listverbs[] =
{
	[LNODE] = "L",
	[LNTH] = "LNth",
	NULL
};

List *evallists(
	Array *const U,
	List **const dag, const Array *const M, const Array *const dive)
{
	const Array verbs = keymap(U, 0, listverbs);
	walkdag(*dag, M, dive, rewriteone, (void *)&verbs);
	freeuimap((Array *)&verbs);

// 	FIXME: Нужно прибрать L-узлы
// 	const Array nonroots = keymap(U, 0, ES("L"));
// 	gcnodes(dag, M, &nonroots);
// 	freeuimap((Array *)&nonroots);

	return *dag;
}
