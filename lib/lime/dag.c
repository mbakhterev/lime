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
	Array *const defs;
	Array *const map;
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
	
	case LIST:
	{
		forlist(r.u.list, markone, st, 0);
		break;
	}

	case NODE:
	{
		Array *const D = st->defs;
		Array *const NR = st->nonroots;
		Array *const M = st->marks;
		Array *const map = st->map;


		// Тут возможны варианты

		if(!r.external)
		{
			// Если речь идёт об определении узла, то в любом случае
			// узел надо отметить, как определённый локально. Если
			// узел уже есть в defs, то это нарушение структуры
			// графа tunesetmap вылетит

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

	// Сперва разберёмся со скучными тривиальными случаями, в которых ничего
	// делать не надо

	switch(r->code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		return;
	
	case LIST:
		break;
	
	default:
		// По идее rebuild не должна вызываться для всех других случаев.
		// Даже для NODE

		assert(0);
	}

	// Приступаем к реконструкции списка

	List *l = NULL;

	while(r->u.list)
	{
		List *const k = tipoff(&r->u.list);

		if(k->code != NODE)
		{
		}	
	}
}

void gcnodes(
	Ref *const dag, Array *const map,
	Array *const nonroots, Array *const marks)
{
	GCState st =
	{
		.map = map,
		.nonroots = nonroots,
		.marks = marks,
		.defs = newkeymap()
	};

	mark(*dag, &st);
	
	freekeymap(st.defs);
}
