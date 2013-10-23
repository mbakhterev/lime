#include "construct.h"
#include "util.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define DBGGC	1

// #define DBGFLAGS (DBGGC)
#define DBGFLAGS 0

Ref forkdag(const Ref dag)
{
	Array *const M = newkeymap();
	const Ref r = forkref(dag, M);
	freekeymap(M);

	return r;
}

typedef struct
{
	Array *const map;
	WalkOne *const wlk;
	void *const ptr;
} WState;

static int walkone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	WState *const st = ptr;

	if(l->ref.code != NODE || l->ref.external)
	{
		// Хотим жёсткой структуры графа. Если это не определение узла
		assert(0);
	}

	const Ref *const attr = nodeattributecell(l->ref);

	const unsigned go
		= st->wlk(l->ref, nodeverb(l->ref, st->map), attr, st->ptr);

	if(go)
	{
		walkdag(*attr, st->wlk, st->ptr, st->map);
	}

	return 0;
}

void walkdag(const Ref dag, WalkOne wlk, void *const ptr, Array *const vm)
{
	assert(dag.code == LIST);

	WState st =
	{
		.map = vm,
		.wlk = wlk,
		.ptr = ptr
	};

	forlist(dag.u.list, walkone, &st, 0);
}

typedef struct
{
	Array *const marks;
	Array *const nonroots;
} GCState;

static int collectone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	GCState *const st = ptr;

	collectmarks(st->marks, l->ref, st->nonroots);

	return 0;
}

void collectmarks(Array *const marks, const Ref r, Array *const nonroots)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		break;
	
	case NODE:
		assert(isnode(r));

		if(r.external)
		{
			// Если речь идёт о ссылке на узел, то помечаем этот
			// узел, если он был до сих пор не помечен

			if(!setmap(marks, r))
			{
				tunesetmap(marks, r);
			}
		}
		else
		{
			// Если речь идёт об определение узла, то интересуемся
			// его атрибутом только если это узел не не-корневой

			if(nodeverb(r, nonroots) == -1)
			{
				collectmarks(marks, nodeattribute(r), nonroots);
			}
		}

		break;

	case LIST:
	{
		GCState st = { .marks = marks };
		forlist(r.u.list, collectone, &st, 0);

		break;
	}
	
	default:
		assert(0);
	}
}

static void rebuild(

// typedef struct
// {
// 	List *L;
// 	const Array *const nonroots;
// 	const Array *const go;
// 	Array marks;
// } GCState;
// 
// static unsigned inmap(const Array *const map, const unsigned i)
// {
// 	return uireverse(map, i) != -1;
// }
// 
// static int rootone(List *const l, void *const state)
// {
// 	assert(l);
// 	DBG(DBGGC, "l.ref.u.(code node) = %u %p",
// 		l->ref.code, (void *)l->ref.u.node);
// 
// 	assert(l->ref.code == NODE);
// 	assert(l->ref.u.node);
// 
// 	GCState *const st = state;
// 	assert(st);
// 
// 	const Node *const n = l->ref.u.node;
// 	assert(n);
// 
// 	if(!inmap(st->nonroots, n->verb))
// 	{
// 		ptrmap(&st->marks, n);
// 		st->L = append(st->L, RL(refnode((Node *)n)));
// 	}
// 
// 	return 0;
// }
// 
// static int expandone(List *const l, void *const state)
// {
// 	GCState *const st = state;
// 	assert(l);
// 	assert(st);
// 
// 	if(l->ref.code != NODE)
// 	{
// 		return 0;
// 	}
// 
// 	const Node *const n = l->ref.u.node;
// 	assert(n);
// 
// 	// FIXME: это очень неэффективный поиск в ширину из-за ptrmap
// 
// 	if(ptrreverse(&st->marks, n) != -1)
// 	{
// 		return 0;
// 	}
// 
// 	ptrmap(&st->marks, l->ref.u.node);
// 	st->L = append(st->L, RL(refnode((Node *)n)));
// 
// 	return 0;
// }
// 
// static int rebuildone(List *const l, void *const state)
// {
// 	// Проверяем и отцепляем звено от списка. Оно будет либо добавлено в
// 	// новый список, либо удалено
// 
// 	assert(l && l->ref.code == NODE);
// 	Node *const n = l->ref.u.node;
// 	assert(n);
// 
// 	// l - dag из одного узла n
// 	l->next = l;
// 
// 	GCState *const st = state;
// 	assert(st);
// 
// 	if(ptrreverse(&st->marks, n) != -1)
// 	{
// 		DBG(DBGGC,
// 			"keepind (%p:dag(%u)=%u)",
// 			(void *)n, n->verb,
// 			n->dag);
// 
// 		// Приписываем отмеченный узел к результату
// 		st->L = append(st->L, l);
// 
// 		// Если в этом узле подграф, который надо пройти, собираем мусор
// 		// в нём
// 
// 		if(n->dag && inmap(st->go, n->verb))
// 		{
// 			DBG(DBGGC, "gc dag on node (%u:%p)",
// 				n->verb, (void *)n);
// 
// 			gcnodes(&n->u.attributes, st->go, st->nonroots);
// 		}
// 	}
// 	else
// 	{
// 		freedag(l);
// 	}
// 
// 	return 0;
// }
// 
// List *gcnodes(
// 	List **const dptr,
// 	const Array *const go, const Array *const nonroots)
// {
// 	GCState st =
// 	{
// 		.nonroots = nonroots,
// 		.go = go,
// 		.marks = makeptrmap(),
// 		.L = NULL
// 	};
// 
// 	List *const l = *dptr;
// 
// 	if(DBGFLAGS & DBGGC)
// 	{
// 		char *const c = strlist(NULL, *dptr);
// 		DBG(DBGGC, "going along list: %s", c);
// 		free(c);
// 	}
// 
// 	forlist(l, rootone, &st, 0);
// 
// 	while(st.L)
// 	{
// 		List *const k = tipoff(&st.L);
// 
// 		// Список уже отфильтрован в rootone, поэтому знаем, что k -
// 		// это ссылка на узел. Расширять волну на под-графы не следует,
// 		// потому что под-графы изолированы по ссылкам
// 
// 		const Node *const n = k->ref.u.node;
// 
// 		if(!n->dag)
// 		{
// 			forlist(n->u.attributes, expandone, &st, 0);
// 		}
// 
// 		freelist(k);
// 	}
// 
// 	forlist(l, rebuildone, &st, 0);
// 
// 	freeptrmap(&st.marks);
// 
// 	return (*dptr = st.L);
// }
// 
// static int freeone(List *const l, void *const ptr)
// {
// 	assert(ptr == NULL);
// 	assert(l);
// 	assert(l->ref.code == NODE);
// 
// 	Node *const n = l->ref.u.node;
// 
// 	if(!n->dag)
// 	{
// 		freelist(n->u.attributes);
// 	}
// 	else
// 	{
// 		freedag(n->u.attributes);
// 	}
// 
// 	freenode(n);
// 
// 	return 0;
// }
// 
// void freedag(List *const dag)
// {
// 	forlist(dag, freeone, NULL, 0);
// 	freelist(dag);
// }
// 
// // FIXME: может быть, имеет смысл вынести mapone и inmap в extern-функции
// // каких-нибудь dag-утилит. Возможно, они часто будут всплывать
// 
// static int mapone(List *const l, void *const ptr)
// {
// 	assert(l && l->ref.code == NODE);
// 	assert(ptr);
// 	Array *const nodes = ptr;
// 
// 	assert(nodes->count == ptrmap(nodes, l->ref.u.node));
// 
// 	return 0;
// }
// 
// typedef struct
// {
// 	const Array *nodemap;
// 	const Node *nodes;
// 	unsigned bound;
// } FState;
// 
// List *forkdag(const List *const dag)
// {
// 	Array M = makeptrmap();
// 	forlist((List *)dag, mapone, &M, 0);
// 
// 	Ref N[M.count + 1];
// 	N[M.count] = (Ref) { .code = FREE, .u.node = NULL };
// 
// 	// Исходные узлы
// 	const Node *const *const nsrc = M.data;
// 
// 	for(unsigned i = 0; i < M.count; i += 1)
// 	{
// 		const Node *const n = nsrc[i];
// 
// 		N[i] = refnode(newnode(		
// 			n->line, n->verb,
// 			!n->dag ?
// 				  transforklist(n->u.attributes, &M, N, i)
// 				: forkdag(n->u.attributes), n->dag));
// 	}
// 
// 	return readrefs(N);
// }
