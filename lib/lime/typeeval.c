#include "construct.h"
#include "util.h"

#include <assert.h>

typedef struct
{
	Array *const U;
	Array *const types;
	Array *const typemap;

	Array *const escape;
	Array *const envmap;

	Array *const verbs;
	Array *const varmap;
} EState;

static void enumtype(const Ref, EState *const);

static int enumone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);

	enumtype(l->ref, ptr);

	return 0;
}

#define TNODE 0
#define TENV 1

static const char *const verbs[] =
{
	[TNODE] = "T",
	[TENV] = "TEnv",
	[TDEF] = "TDef",
	NULL
};

static void enumnode(
	Array *const U,
	Array *const types, Array *const typemap, Array *const varmap,
	const Ref r, Array *const verbs)
{
	if(r.external)
	{
		// Ссылками не интересуемся. Нам важны только определения
		return;
	}

	switch(nodeverb(r, verbs))
	{
	case TNODE:
	{
		// Смотрим, во что превращается атрибут узла, в текущем рабочем
		// контексте

		const Ref key = expeval(nodeattribute(r), varmap);

		// Полученное выражение должно быть описанием типа

		if(!istypekey(key))
		{
			return;
		}

		// Присваиваем .T-выражению номер типа, которое оно задаёт. При
		// чём, и в результирующей typemap, и в отображении узлов на
		// (TYPE + TYPEVAR). Два отображения нам нужны, потому что,
		// .TEnv-ы при разборе .T и других узлов должны отображаться на
		// разные объекты. В первом случае это должны быть TVAR-ы, а
		// во втором - TYPE-ы

		const unsigned typeid = typeenummap(types, key);
		freeref(key);

		tunerefmap(workmap, r, reftype(typeid));
		tunerefmap(typemap, r, reftype(typeid));

		break;
	}
		
	case TENV:
	{
		// Нас интересуют те .TEnv-ы, которые задают TVAR-ы, то есть
		// .TEnv-ы со списками атрибутов единичной длины. Остальные
		// .TEnv описывают другие сущности и мы их здесь не трогаем

		const Ref attr = nodeattribute(r);
		if(attr.code != LIST)
		{
			return;
		}

		const unsigned len = listlen(attr.u.list);
		if(len != 1)
		{
			return;
		}

		// Добываем этот единственный элемент из списка. Он будет
		// служить нам основным ключом. Нужно проверить, что он подходит
		// под критерии basic

		const Ref key = attr.u.list->ref;

		if(!isbasickey(key))
		{
			return;
		}
		
		// Если ключ подходящий, дополнительно его декорируем и
		// добавляем в него ссылку на окружение, в котором видим текущий
		// .TEnv

		const Ref augkey 
			= reflist(
				append(
					RL(refkeymap(envmap(envmarks, r))),
					decorate(key, U, TYPE)));

		// Теперь надо найти этому выражению место в таблице типов. И
		// соответствующим образом расширить varmap

		const unsigned id = typeenummap(types, augkey);
		freeref(augkey);

		tunerefmap(varmap, r, reftvar(id));
	}
	}
}

static void enumtype(const Ref r, EState *const E)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
		// В этом нет никакой информации о типах
		return;

	case LIST:
		// В списке могут встретится определения узлов .T и .TEnv
		// проходим его

		forlist(r.u.list, enumone, E, 0);
		return;

	
	case NODE:
		enumnode(E->U, E->types, E->typemap, E->varmap, r, E->verbs);
		return;
	
	default:
		assert(0);
	}
}

void typeeval(
	Array *const U,
	Array *const types,
	Array *const typemarks,
	const Ref dag, Array *const escape, Array *const envmarks)
{
}
