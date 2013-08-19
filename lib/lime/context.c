#include "construct.h"
#include "util.h"

#include <assert.h>

static Context makecontext(const DagMap *const map)
{
	return (Context)
	{
		.dag = NULL,
		.dagmap = map,
		.outs = pushenvironment(NULL),
		.forms = NULL,
		.ins = pushenvironment(NULL),
	};
}

List *pushcontext(List *const ctx, const DagMap *const map)
{
	assert(map);
	assert(!ctx || ctx->ref.code == CTX);

	Context *const new = malloc(sizeof(Context));
	assert(new);
	*new = makecontext(map);

	return append(RL(refctx(new)), ctx);
}

static void freectx(Context *const ctx)
{
	assert(ctx);
	assert(popenvironment(ctx->outs) == NULL);
	assert(popenvironment(ctx->ins) == NULL);
	freelist(ctx->forms);
	freedag(ctx->dag, ctx->dagmap);
	free(ctx);
}

extern List *popcontext(List *const ctx)
{
	assert(ctx && ctx->ref.code == CTX);

	List *c = ctx;
	List *t = tipoff(&c);

	freectx(t->ref.u.context);
	freelist(t);

	return c;
}
