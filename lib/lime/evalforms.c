#include "construct.h"
#include "util.h"

#include <assert.h>

#define FNODE	0
#define FPUT	1
#define FGPUT	2
#define FEPUT	3

static const char *const formverbs[] =
{
	[FNODE] = "F",
	[FPUT] = "FPut",
	[FGPUT] = "FGPut",
	[FEPUT] = "FEPut",
	NULL
};

// EState - Evaluation State

typedef struct 
{
	const List *const env;
	const List *const ctx;
	const DagMap *const map;
	const Array *const verbs;
	Array *const universe;
} EState;

#define AKEY 0
#define ASIGNATURE 1
#define ADAG 2

static unsigned isfeputrefs(const Ref R[], const unsigned fverb)
{
	return R[AKEY].code == LIST 
		&& R[ASIGNATURE].code == LIST
		&& (R[ADAG].code == NODE
			&& R[ADAG].u.node
			&& R[ADAG].u.node->verb == fverb);
}

Ref *formkeytoref(
	Array *const U,
	const List *const env, const List *const key, const unsigned depth)
{
	DL(fkey, RS(
		refatom(readpack(U, strpack(0, "#"))),
		reflist((List *)key)));
	
	return keytoref(env, fkey, depth);
}

static void feputeval(
	const List *const attr, const List *const env, const EState *const st)
{
	assert(env && env->ref.code == ENV);
	
	// Получение и проверка структуры списка аттрибутов

	// Ожидаемое количество
	const unsigned len = 3;
	const Ref R[len + 1];

	// Загрузка в массив и проверка длины
	const unsigned refcnt = writerefs(attr, (Ref *)R, len + 1);
	assert(refcnt == len + 1 && R[len].code == FREE);

	if(!isfeputrefs(R, uidirect(st->verbs, FNODE)))
	{
		ERR("%s", ".FEPut node structure broken");
	}

	Ref *const r = formkeytoref(st->universe, env, R[AKEY].u.list, -1);

	if(r->code != FREE)
	{
		ERR("%s", "Form with key exists");
	}

	*r = (Ref)
	{
		.code = FORM,

		.u.form
			= newform(
				R[ADAG].u.node->u.attributes,
				st->map,
				R[ASIGNATURE].u.list)
	};
}

static void evalone(List *const l, void *const ptr)
{
	assert(ptr);
	assert(l && l->ref.code == NODE && l->ref.u.node);

	const EState *const st = ptr;
	const Array *const V = st->verbs;

	const Node *const n = l->ref.u.node;

	const unsigned key = uireverse(V, n->verb);

	switch(key)
	{
	case FEPUT:
		feputeval(n->u.attributes, st->env, st);
	}
}

// List *evalforms(
void evalforms(
	Array *const U,
	const List *const dag, const DagMap *const M,
	const List *const env, const List *const ctx)
{
	assert(env && env->ref.code == ENV);
//	assert(ctx && ctx->ref.code == CTX);

	const Array verbs = keymap(U, 0, formverbs);

	const EState st =
	{
		.env = env,
		.ctx = ctx,
		.map = M,
		.verbs = &verbs,
		.universe = U
	};
	
	walkdag(dag, M, evalone, (void *)&st);

	freeuimap((Array *)&verbs);
}
