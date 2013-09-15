#include "construct.h"
#include "util.h"

#include <assert.h>
#include <string.h>

static Reactor emptyreactor(void)
{
	return (Reactor)
	{ 
		.forms = NULL,
		.outs = pushenvironment(NULL),
		.ins = pushenvironment(NULL)
	};
}

static void purgereactor(Reactor *const r)
{
	assert(popenvironment(r->outs) == NULL);
	assert(popenvironment(r->ins) == NULL);

	// freeformlist работает через freeform, которая отличает различает 
	// external-формы. Формы из окружений не будут затронуты

	freeformlist(r->forms);

	freelist(r->forms);
}

static Context makecontext(const unsigned state, const unsigned marker)
{
// 	return (Context)
// 	{
// 		.dag = NULL,
// 		.state = state,
// 		.marker = marker,
// 		.outs = pushenvironment(NULL),
// 		.forms = NULL,
// 		.ins = pushenvironment(NULL),
// 	};

	return (Context)
	{
		.dag = NULL,
		.state = state,
		.marker = marker,
		.R = { emptyreactor(), emptyreactor() }
	};
}

List *pushcontext(
	List *const ctx, const unsigned state, const unsigned marker)
{
	assert(!ctx || ctx->ref.code == CTX);

	Context *const new = malloc(sizeof(Context));
	assert(new);

	// Потому что в структуре есть const поля
	{
		const Context C = makecontext(state, marker);
		memcpy(new, &C, sizeof(Context));
	}

	return append(RL(refctx(new)), ctx);
}

static void freectx(Context *const ctx)
// const Array *const map)
{
	assert(ctx);

	purgereactor(ctx->R + 0);
	purgereactor(ctx->R + 1);

	free(ctx);
}

extern List *popcontext(List *const ctx)
// , const Array *const map)
{
	assert(ctx && ctx->ref.code == CTX);

	List *c = ctx;
	List *t = tipoff(&c);

	freectx(t->ref.u.context);
	// , map);

	freelist(t);

	return c;
}

static unsigned isemptyenv(const List *const env)
{
	return env && env->next == env
		&& env->ref.code == ENV && env->ref.u.environment
		&& env->ref.u.environment->count == 0;
}

unsigned isforwardempty(const List *const ctx)
{
	if(ctx && ctx->ref.code == CTX && ctx->ref.u.context)
	{
	}
	else
	{
		return 0;
	}

	const Reactor *const r = ctx->ref.u.context->R + 1;

	return r->forms == NULL && isemptyenv(r->ins) && isemptyenv(r->outs);
}
