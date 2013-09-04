#include "construct.h"
#include "util.h"

#include <assert.h>

static struct reactor emptyreactor(void)
{
	return (struct reactor)
	{ 
		.forms = NULL,
		.outs = pushenvironment(NULL),
		.ins = pushenvironment(NULL)
	};
}

static void purgereactor(struct reactor *const r)
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

static void freectx(Context *const ctx, const Array *const map)
{
	assert(ctx);

// 	assert(popenvironment(ctx->outs) == NULL);
// 	assert(popenvironment(ctx->ins) == NULL);
// 	freelist(ctx->forms);
// 
// 	freedag(ctx->dag, map);

	purgereactor(ctx->R + 0);
	purgereactor(ctx->R + 1);

	free(ctx);
}

extern List *popcontext(List *const ctx, const Array *const map)
{
	assert(ctx && ctx->ref.code == CTX);

	List *c = ctx;
	List *t = tipoff(&c);

	freectx(t->ref.u.context, map);
	freelist(t);

	return c;
}
