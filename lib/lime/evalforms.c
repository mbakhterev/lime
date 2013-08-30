#include "construct.h"
#include "util.h"

#include <assert.h>

#define FNODE	0
#define FPUT	1
#define FGPUT	2
#define FEPUT	3
#define FLOOK	4

static const char *const formverbs[] =
{
	[FNODE] = "F",
	[FPUT] = "FPut",
	[FGPUT] = "FGPut",
	[FEPUT] = "FEPut",
	[FLOOK] = "FLook",
	NULL
};

// EState - Evaluation State

#define AKEY 0
#define AFORMREF 1
#define ASIGNATURE 0
#define ADAG 1

typedef struct
{
	const List *const dag;
	const List *const signature;
} FEssence;

static unsigned isvoidessence(const FEssence e)
{
	if(e.dag == NULL)
	{
		assert(e.signature == NULL);
		return 1;
	}

	return 0;
}

static FEssence voidessence(void)
{
	return (FEssence) { .dag = NULL, .signature = NULL };
}

static FEssence extractfromnode(const Node *const n, const Array *const V)
{
	assert(n);

	switch(uireverse(V, n->verb))
	{
	case FLOOK:
	{
		const List *const attr = n->u.attributes;
		assert(attr && attr->ref.code == FORM);

		const Form *const f = attr->ref.u.form;
		assert(f);

		return (FEssence)
		{
			.dag = f->u.dag,
			.signature = f->signature
		};
	}
	}

	return voidessence();
}

static int isvalidkey(const Ref ref)
{
	return ref.code == LIST && ref.u.list && iscomparable(ref.u.list);
}

static int isvalidone(List *const l, void *const ptr)
{
	assert(l);
	return isvalidkey(l->ref);
}

static unsigned isvalidsignature(const Ref r)
{
	return r.code == LIST
		&& r.u.list
		&& forlist((List *)r.u.list, isvalidone, NULL, !0) == !0;
}

static FEssence extractfromlist(const List *const l, const Array *const V)
{
	assert(l);

	const unsigned len = 2;
	const Ref R[len + 1];
	const unsigned refcnt = writerefs(l, (Ref *)R, len + 1);

	if(!(refcnt == len + 1 && R[len].code == FREE))
	{
		return voidessence();
	}

	if(!isvalidsignature(R[ASIGNATURE]))
	{
		return voidessence();
	}

	const Ref d = R[ADAG];

	if(!(d.code == NODE
		&& d.u.node && uireverse(V, d.u.node->verb) == FNODE
		&& d.u.node->u.attributes))
	{
		return voidessence();
	}

	return (FEssence)
	{
		.dag = d.u.node->u.attributes,
		.signature = R[ASIGNATURE].u.list
	};
}

// extractform должна получить описание формы: граф и сигнатуру - из списка
// аргументов узла и убедиться, в процессе убеждаясь, что всё корректно. Чтобы
// вывод информации об ошибках оставался на одном уровне графа вызовов, функция
// возвращает voidessence, когда обнаруживает нарушение в структуре аргумента.
// Этой же политике следуют используемые функции: extractfromnode и
// extractfromlist

static FEssence extractform(const Ref ref, const Array *const V)
{
	assert(V);

	// Бывает всего два варианта аттрибутов, описывающих форму: ссылка на
	// узел .Flook или пара (signature list; .F (...))

	switch(ref.code)
	{
	case NODE:
		return extractfromnode(ref.u.node, V);

	case LIST:
		return extractfromlist(ref.u.list, V);
	}

	return voidessence();
}

typedef struct 
{
	const List *const env;
	const List *const ctx;
//	const DagMap *const map;
	const Array *const map;
	const Array *const verbs;
	Array *const universe;
} EState;


static void feputeval(const Node *const n, const EState *const st)
{
	assert(n);

	assert(st);
	assert(st->env && st->env->ref.code == ENV);
	assert(st->verbs);
	assert(st->universe);

	const List *const attr = n->u.attributes;
	assert(attr);

	// Ожидаемое количество
	const unsigned len = 2;
	const Ref R[len + 1];

	// Загрузка в массив и проверка длины
	const unsigned refcnt = writerefs(attr, (Ref *)R, len + 1);

	if(!(refcnt == len + 1 && R[len].code == FREE))
	{
		item = 10;
		ERR("%s", ".FEPut attribute structure is broken");
	}

	if(!isvalidkey(R[AKEY]))
	{
		item = n->line;
		ERR("%s", ".FEPut keyspec attribute structure is broken");
	}

	const FEssence fe = extractform(R[AFORMREF], st->verbs);

	if(isvoidessence(fe))
	{
		item = n->line;
		ERR("%s", ".FEPut formspec attribute structure is broken");
	}

	Ref *const r = formkeytoref(st->universe, st->env, R[AKEY].u.list, -1);

	if(r->code != FREE)
	{
		item = n->line;
		ERR("%s", "Form with key exists");
	}

	*r = (Ref)
	{
		.code = FORM,
		.u.form = newform(fe.dag, st->map, fe.signature)
	};
}

static void evalone(List *const l, void *const ptr)
{
	const EState *const st = ptr;
	assert(st);
	const Array *const V = st->verbs;
	assert(V);

	assert(l);
	const Node *const n = l->ref.u.node;
	assert(n);

	assert(l && l->ref.code == NODE && l->ref.u.node);
	
	const unsigned key = uireverse(V, n->verb);

	switch(key)
	{
	case FEPUT:
		feputeval(n, st);
	}
}

// void evalforms(
// 	Array *const U,
// 	const List *const dag, const DagMap *const M,
// 	const List *const env, const List *const ctx)

void evalforms(
	Array *const U,
	const List *const dag, const Array *const map, const Array *const go,
	const List *const env, const List *const ctx)
{
// Это надо проверять по ходу дела. В ситуации больше динамики, чем
// предполагают эти проверки. Откладываем их до конкретных функций
// 
// 	assert(env && env->ref.code == ENV);
// 	assert(ctx && ctx->ref.code == CTX);

	const Array verbs = keymap(U, 0, formverbs);

	const EState st =
	{
		.env = env,
		.ctx = ctx,
// 		.map = M,
		.map = map,
		.verbs = &verbs,
		.universe = U
	};
	
// 	walkdag(dag, M, evalone, (void *)&st);
	walkdag(dag, map, go, evalone, (void *)&st);

	freeuimap((Array *)&verbs);
}
