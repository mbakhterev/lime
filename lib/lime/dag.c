#include "construct.h"
#include "util.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define DBGGC	1
#define DBGMRK	2
#define DBGRB	3

// #define DBGFLAGS (DBGMRK)

#define DBGFLAGS 0

unsigned isdaglist(const List *const l)
{
	return l == NULL || (l->ref.code == NODE && !l->ref.external);
}

unsigned isdag(const Ref dag)
{
	return dag.code == DAG && isdaglist(dag.u.list);
}

Ref forkdag(const Ref D)
{
	assert(isdag(D));

	if(D.external)
	{
		return D;
	}

	Array *const M = newkeymap();

// 	const Ref r = forkref(dag, M);
// 	const Ref r = forkref(reflist
// 	freekeymap(M);

	const Ref r = refdag(transforklist(D.u.list, M));
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
	assert(dag.code == LIST || dag.code == DAG);

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
	const Array *const nonroots;
	const Array *const map;

	Array *const defs;
} GCState;

static void mark(const Ref dag, GCState *const st);

static int markone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	mark(l->ref, ptr);

	return 0;
}

static void mark(const Ref r, GCState *const st)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		break;
	
	// У нас все текущие алгоритмы построены так, что им без разницы DAG или
	// LIST

	case LIST:
	case DAG:
	{
		forlist(r.u.list, markone, st, 0);
		break;
	}

	case NODE:
	{
		Array *const D = st->defs;
		Array *const M = st->marks;
		const Array *const NR = st->nonroots;
		const Array *const map = st->map;

		// Тут возможны варианты

		if(!r.external)
		{
			// Если речь идёт об определении узла, то в любом случае
			// узел надо отметить, как определённый локально. Если
			// узел уже есть в defs, то это нарушение структуры
			// графа tunesetmap вылетит

			DBG(DBGMRK, "def N:%p", (void *)r.u.list);
			tunesetmap(D, r);

			// Выясняем является ли узел nonroots и отмечен ли он (в
			// этом месте он может быть отмечен только внешним
			// образом: метка из gcnodes может быть поставлена,
			// только если (1) узел не был в defs и мы его поместили
			// в defs и это nonroot-узел, или если (2) был в defs, и
			// мы смотрим на него через ссылку; см. ниже)

			const unsigned isnr = nodeverb(r, NR) != -1;
			const unsigned mrk = setmap(M, r);

			if(!isnr)
			{
				// Если это не-nonroot узел, и он не отмечен
				// внешне, то надо его отметить

				if(!mrk)
				{
					DBG(DBGMRK,
						"mark root N:%p",
						(void *)r.u.list);

					tunesetmap(M, r);
				}

				// И пройтись по его атрибутам, если нас не
				// просят обойти этот узел стороной. То, что
				// проход не повторный гарантируется
				// уникальностью узла в defs

				if(!setmap(map, r))
				{
					mark(nodeattribute(r), st);
				}
			}
			else if(mrk)
			{
				// Если этот nonroot-узел, и он помечен (внешним
				// образом, изнутри мы до него не могли
				// добраться; см. ниже), то он нам тоже нужен.
				// Надо пройтись по его атрибутам. Если нас не
				// просят обойти узел стороной

				if(nodeverb(r, map) == -1)
				{
					mark(nodeattribute(r), st);
				}
			}
		}
		else
		{
			// Здесь мы наблюдаем ссылку на узел. Нам интересны
			// определённые узлы. Если такой узел не помечен, то это
			// обязан быть nonroot-узел. Который мы с удовольствием
			// отметим и пройдёмся по его атрибутам. Если узел
			// помечен, то этот проход уже был сделан и ничего
			// делать не нужно. Проходим, если узел особо не отмечен
			// в map

			if(setmap(D, r) && !setmap(M, r))
			{
				assert(nodeverb(r, NR) != -1);

				DBG(DBGMRK,
					"mark referred N:%p", (void *)r.u.list);

				tunesetmap(M, r);

				if(nodeverb(r, map) == -1)
				{
					mark(nodeattribute(r), st);
				}
			}
		}

		break;
	}

	default:
		assert(0);
	}
}

static void rebuild(Ref *const r, GCState *const st)
{
	assert(st);
	assert(r);

	// Сперва разберёмся со скучными тривиальными случаями, в которых ничего
	// делать не надо

	switch(r->code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		return;
	
	case LIST:
	case DAG:
		// Продолжим ниже
		break;
	
	default:
		// По идее rebuild не должна вызываться для всех других случаев.
		// Даже для NODE

		assert(0);
	}

	// Приступаем к реконструкции списка

	List *l = NULL;

	Array *const M = st->marks;
	const Array *const map = st->map;

	while(r->u.list)
	{
		List *const k = tipoff(&r->u.list);

		if(k->ref.code != NODE || k->ref.external)
		{
			// Если речь идёт не о определении узла, то оставляем
			// эту Ref в списке

			l = append(l, k);

			continue;
		}	

		// Здесь речь идёт об определение узла.  Чтобы быть оставленным
		// в списке он должен быть помечен. Если он не помечен, то
		// просто освобождаем список k

		if(!setmap(M, k->ref))
		{
			freelist(k);
			continue;
		}

		// Здесь мы знаем, что узел помечен. Его надо оставить. Но
		// сперва в том случае, если про него не сказано при помощи map:
		// не трогать! - мы должны перестроить его атрибут

		if(nodeverb(k->ref, map) == -1)
		{
			rebuild((Ref *)nodeattributecell(k->ref), st);
		}

		l = append(l, k);
	}

	r->u.list = l;
}

void gcnodes(
	Ref *const dag, const Array *const map,
	const Array *const nonroots, Array *const marks)
{
	assert(isdag(*dag));

	GCState st =
	{
		.map = map,
		.nonroots = nonroots,
		.marks = marks != NULL ? marks : newkeymap(),
		.defs = newkeymap()
	};

	mark(*dag, &st);
	rebuild(dag, &st);
	
	freekeymap(st.defs);

	if(!marks)
	{
		freekeymap(st.marks);
	}
}
