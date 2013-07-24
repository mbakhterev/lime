#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGRONE	1

#define DBGFLAGS (DBGRONE)

#define LNODE	0
#define LNTH	1

// Expand State - структура для сбора информации о процессе переписывания
// атрибутов

typedef struct
{
	List *rewritten;
	const Array *verbs;
} EState;

static List *rewritelistrefs(List *const, const Array *const verbs);

// FIXME: По уму здесь надо бы накапливать куски списка аттриботов, в которах
// нет ссылок на L-узлы и целыми такими кусками цеплять всё в expanded.

static int rewriteref(List *const l, void *const ptr)
{
	EState *const st = ptr;
	assert(st);

	// В результат будет дописан либо l - текущий элемент списка (возможно,
	// с переписанным содержимым), либо список атрибутов L-узла, на который
	// ссылается l->ref (и тогда l будет удалён). В любом случае l
	// необходимо закольцевать. Результат в списке r

	List *r = l;
	r->next = r;

	switch(l->ref.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		// Дописываем в результат сам элемент списка
		break;
	
	case LIST:
		// Дописываем в результат сам элемент списка, но с переписанным
		// содержимым

		r->ref.u.list = rewritelistrefs(r->ref.u.list, st->verbs);
		break;

	case NODE:
	{
		const Node *const n = l->ref.u.node;
		const unsigned key = uireverse(st->verbs, n->verb);
		switch(key)
		{
		case LNODE:
		case LNTH:
			freelist(l);
			r = forklist(n->u.attributes);
			break;
		}

		break;
	}

	default:
		assert(0);
	}

	st->rewritten = append(st->rewritten, r);
	return 0;
}

static List *rewritelistrefs(List *const l, const Array *const verbs)
{
	EState st = { .rewritten = NULL, .verbs = verbs };
	forlist(l, rewriteref, &st, 0);
	return st.rewritten;
}

static void rewriteone(List *const l, void *const ptr)
{
	assert(l && l->ref.code == NODE);

	// В этой функции уже известно, что l содержит ссылку на узел
	Node *const n = l->ref.u.node;

	const Array *const verbs = ptr;
	const unsigned key = uireverse(verbs, n->verb);

	DBG(DBGRONE, "%u", n->verb);

	switch(key)
	{
	case -1:
	case LNODE:
		n->u.attributes = rewritelistrefs(n->u.attributes, verbs);
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
	List **const dag,
// 	const Array *const M, const Array *const dive)
	const DagMap *const M)
{
	const Array verbs = keymap(U, 0, listverbs);
//	walkdag(*dag, M, dive, rewriteone, (void *)&verbs);
	walkdag(*dag, M, rewriteone, (void *)&verbs);
	freeuimap((Array *)&verbs);

// 	const Array nonroots = keymap(U, 0, ES("L"));
// 	gcnodes(dag, M, &nonroots);
// 	freeuimap((Array *)&nonroots);

	return *dag;
}
