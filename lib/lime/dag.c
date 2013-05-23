#include "construct.h"

#include <stdlib.h>
#include <assert.h>

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

	if(freenodes) { } else
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
	return map && uireverse(map, i) != -1;
}

static int rootone(List *const l, void *const state)
{
	assert(l && l->ref.code == NODE && l->ref.u.node);

	GCState *const st = state;
	assert(st);

	const Node *const n = l->ref.u.node;
	assert(n);

// 	if(st->nonroots == NULL || uireverse(st->nonroots, n->verb) == -1)
	
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
	// список новый, либо удалено

	assert(l && l->ref.code == NODE);
	l->next = l;

	GCState *const st = state;
	assert(st);

	Node *const n = l->ref.u.node;
	assert(n);

	if(ptrreverse(&st->marks, n) != -1)
	{
		st->L = append(st->L, l);
		if(inmap(st->dagmap, n->verb))
		{
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
		.marks = makeptrmap(),
		.L = NULL
	};

	List *const l = *dptr;

	forlist(l, rootone, &st, 0);

	while(st.L)
	{
		List *const k = tipoff(&st.L);

		// Список уже отфильтрован в rootone, поэтому знаем, что k -
		// это ссылка на узел

		forlist(k->ref.u.node->u.attributes, expandone, &st, 0);

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

// 	if(dagmap == NULL || uimap(dagmap, n->verb) == -1)

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
