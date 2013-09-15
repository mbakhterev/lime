#include "construct.h"
#include "util.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

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

Node *newnode(
	const unsigned line,
	const unsigned verb, const List *const attributes, const unsigned dag)
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
	}

// Цена постоянных полей. Может быть, чрезмерная. Поле .dag инициируется именно
// так, потому что оно - битовое поле

	memcpy(n,
		&(Node) {
			.line = line,
			.verb = verb,
			.dag = dag != 0,
			.u.attributes = (List *)attributes }, sizeof(Node));

	return n;
}

void freenode(Node *const n)
{
	assert(n && n->verb != FREE);

// Платим свою цену:

	memcpy(n,
		&(Node) {
			.line = -1,
			.verb = FREE,
			.dag = 0,
			.u.attributes = NULL }, sizeof(Node));

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
	const Array *const nonroots;
// 	const Array *const map;
	const Array *const go;
	Array marks;
} GCState;

static unsigned inmap(const Array *const map, const unsigned i)
{
	return uireverse(map, i) != -1;
}

static int rootone(List *const l, void *const state)
{
	assert(l);
	DBG(DBGGC, "l.ref.u.(code node) = %u %p",
		l->ref.code, (void *)l->ref.u.node);

	assert(l->ref.code == NODE);
	assert(l->ref.u.node);

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
	Node *const n = l->ref.u.node;
	assert(n);

	// l - dag из одного узла n
	l->next = l;

	GCState *const st = state;
	assert(st);

	if(ptrreverse(&st->marks, n) != -1)
	{
		DBG(DBGGC,
// 			"keeping: %p:rmap(%u)=%u; map: %p",
			"keepind (%p:dag(%u)=%u)",
			(void *)n, n->verb,
//			uireverse(st->map, n->verb),
			n->dag);
//			(void *)st->map);

		// Приписываем отмеченный узел к результату
		st->L = append(st->L, l);

		// Если в этом узле подграф, который надо пройти, собираем мусор
		// в нём

// 		if(inmap(st->map, n->verb) && inmap(st->go, n->verb))
		if(n->dag && inmap(st->go, n->verb))
		{
			DBG(DBGGC, "gc dag on node (%u:%p)",
				n->verb, (void *)n);
// 			gcnodes(
// 				&n->u.attributes, st->map, st->go,
// 				st->nonroots);

			gcnodes(&n->u.attributes, st->go, st->nonroots);
		}
	}
	else
	{
		freedag(l);
		// , st->map);
	}

	return 0;
}

List *gcnodes(
	List **const dptr,
//	const Array *const map,
	const Array *const go,
	const Array *const nonroots)
{
	GCState st =
	{
		.nonroots = nonroots,
		.go = go,
// 		.map = map,
		.marks = makeptrmap(),
		.L = NULL
	};

	List *const l = *dptr;

	if(DBGFLAGS & DBGGC)
	{
		char *const c = strlist(NULL, *dptr);
		DBG(DBGGC, "going along list: %s", c);
		free(c);
	}

	forlist(l, rootone, &st, 0);

	while(st.L)
	{
		List *const k = tipoff(&st.L);

		// Список уже отфильтрован в rootone, поэтому знаем, что k -
		// это ссылка на узел. Расширять волну на под-графы не следует,
		// потому что под-графы изолированы по ссылкам

		const Node *const n = k->ref.u.node;

//		if(!isdag(dagmap, n->verb))
//		if(!inmap(map, n->verb))
		if(!n->dag)
		{
			forlist(n->u.attributes, expandone, &st, 0);
		}

		freelist(k);
	}

	forlist(l, rebuildone, &st, 0);

	freeptrmap(&st.marks);

	return (*dptr = st.L);
}

static int freeone(List *const l, void *const ptr)
{
	assert(ptr == NULL);
	assert(l);
	assert(l->ref.code == NODE);

	Node *const n = l->ref.u.node;

// 	if(!inmap(dagmap, n->verb))
	if(!n->dag)
	{
		freelist(n->u.attributes);
	}
	else
	{
		freedag(n->u.attributes);
		// , dagmap);
	}

	freenode(n);

	return 0;
}

void freedag(List *const dag)
// , const Array *const map)
{
	forlist(dag, freeone, NULL, 0);
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

List *forkdag(const List *const dag)
// , const Array *const map)
{
// 	assert(dag);

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
			n->line,
			n->verb,
//			inmap(map, n->verb) == 0 ?
//			n->dag,
			!n->dag ?
				  transforklist(n->u.attributes, &M, N, i)
				: forkdag(n->u.attributes), n->dag));
					// , map)));
	}

	return readrefs(N);
}
