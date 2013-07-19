#include "construct.h"
#include "util.h"

#include <stdlib.h>
#include <assert.h>

#define DBGGC	1

// #define DBGFLAGS (DBGGC)

#define DBGFLAGS 0

static Node *freenodes = NULL;

// Откусывание первого элемента из списка узлов, связанных u.nextfree.
// cf. lib/lime/list.c

static Node *tipoffnode(Node **const lptr) {
	Node *const l = *lptr;
	assert(l);

	Node *const n = l->u.nextfree;

	if(l != n) {
		l->u.nextfree = n->u.nextfree;
	}
	else {
		*lptr = NULL;
	}

	return n;

}

Node *newnode(const unsigned verb, const List *const attributes)
{
	assert(verb != FREE);

	Node *n = NULL;

	if(freenodes)
	{
		n = tipoffnode(&freenodes);
		assert(n->verb == FREE);
	}
	else
	{
		n = malloc(sizeof(Node));
		assert(n);
		n->verb = FREE;
	}

	n->u.nextfree = n;

	n->verb = verb;
	n->u.attributes = (List *)attributes;

	return n;
}

void freenode(Node *const n)
{
	assert(n && n->verb != FREE);

	n->verb = FREE;

	if(freenodes == NULL)
	{
		n->u.nextfree = n;
		freenodes = n;
		return;
	}

	freenodes->u.nextfree = n;
	n->u.nextfree = freenodes;
	freenodes = n;
}

typedef struct
{
	List *L;
	const Array *nonroots;
	const Array *dagmap;
	Array marks;
} GCState;

static unsigned inmap(const Array *const map, const unsigned i)
{
// 	return map && uireverse(map, i) != -1;
	return uireverse(map, i) != -1;
}

static int rootone(List *const l, void *const state)
{
	assert(l && l->ref.code == NODE && l->ref.u.node);

	GCState *const st = state;
	assert(st);

	const Node *const n = l->ref.u.node;
	assert(n);

	if(!inmap(st->nonroots, n->verb))
	{
		ptrmap(&st->marks, n);
		st->L = append(st->L, RL(refnode((Node *)n)));
	}

	return 0;
}

static int expandone(List *const l, void *const state)
{
	GCState *const st = state;
	assert(l);
	assert(st);

	if(l->ref.code != NODE)
	{
		return 0;
	}

	const Node *const n = l->ref.u.node;
	assert(n);

	// FIXME: это очень неэффективный поиск в ширину из-за ptrmap

	if(ptrreverse(&st->marks, n) != -1)
	{
		return 0;
	}

	ptrmap(&st->marks, l->ref.u.node);
	st->L = append(st->L, RL(refnode((Node *)n)));

	return 0;
}

static int rebuildone(List *const l, void *const state)
{
	// Проверяем и отцепляем звено от списка. Оно будет либо добавлено в
	// новый список, либо удалено

	assert(l && l->ref.code == NODE);
	l->next = l;

	GCState *const st = state;
	assert(st);

	Node *const n = l->ref.u.node;
	assert(n);

	if(ptrreverse(&st->marks, n) != -1)
	{
		DBG(DBGGC,
			"keeping: %p:rmap(%u)=%u; map: %p",
			(void *)n, n->verb, uireverse(st->dagmap, n->verb),
			(void *)st->dagmap);

		st->L = append(st->L, l);
		if(inmap(st->dagmap, n->verb))
		{
			DBG(DBGGC, "gc for node: %u", n->verb);

			gcnodes(&n->u.attributes, st->dagmap, st->nonroots);
		}
	}
	else
	{
		freedag(l, st->dagmap);
	}

	return 0;
}

List *gcnodes(
	List **const dptr,
	const Array *const dagmap, const Array *const nonroots)
{
	GCState st =
	{
		.nonroots = nonroots,
		.dagmap = dagmap,
		.marks = makeptrmap(),
		.L = NULL
	};

	List *const l = *dptr;

	forlist(l, rootone, &st, 0);

	while(st.L)
	{
		List *const k = tipoff(&st.L);

		// Список уже отфильтрован в rootone, поэтому знаем, что k -
		// это ссылка на узел. Расширять волну на под-графы не следует,
		// потому что под-графы изолированы по ссылкам

		const Node *const n = k->ref.u.node;

		if(!inmap(dagmap, n->verb))
		{
			forlist(n->u.attributes, expandone, &st, 0);
		}

		freelist(k);
	}

	forlist(l, rebuildone, &st, 0);

	freeptrmap(&st.marks);

	return (*dptr = st.L);
}

static int freeone(List *const l, void *const dagmap)
{
	assert(l);
	assert(l->ref.code == NODE);

	Node *const n = l->ref.u.node;

	if(!inmap(dagmap, n->verb))
	{
		freelist(n->u.attributes);
	}
	else
	{
		freedag(n->u.attributes, dagmap);
	}

	freenode(n);

	return 0;
}

void freedag(List *const dag, const Array *const dagmap)
{
	forlist(dag, freeone, (void *)dagmap, 0);
	freelist(dag);
}

// FIXME: может быть, имеет смысл вынести mapone и inmap в extern-функции
// каких-нибудь dag-утилит. Возможно, они часто будут всплывать

static int mapone(List *const l, void *const ptr)
{
	assert(l && l->ref.code == NODE);
	assert(ptr);
	Array *const nodes = ptr;

	assert(nodes->count == ptrmap(nodes, l->ref.u.node));

	return 0;
}

typedef struct
{
	const Array *nodemap;
	const Node *nodes;
	unsigned bound;
} FState;

List *forkdag(const List *const dag, const Array *const dm)
{
	assert(dag);

	Array M = makeptrmap();
	forlist((List *)dag, mapone, &M, 0);

	Ref N[M.count + 1];
	N[M.count] = (Ref) { .code = FREE, .u.node = NULL };

	// Исходные узлы
	const Node *const *const nsrc = M.data;

	for(unsigned i = 0; i < M.count; i += 1)
	{
		const Node *const n = nsrc[i];

		N[i] = refnode(newnode(		
			n->verb,
			uireverse(dm, n->verb) == -1 ?
				  transforklist(n->u.attributes, &M, N, i)
				: forkdag(n->u.attributes, dm)));
	}

	return readrefs(N);
}
