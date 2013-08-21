#include "construct.h"
#include "util.h"

#include <assert.h>

// static Context makecontext(const DagMap *const map)
static Context makecontext(void)
{
	return (Context)
	{
		.dag = NULL,
//		.dagmap = map,
		.outs = pushenvironment(NULL),
		.forms = NULL,
		.ins = pushenvironment(NULL),
	};
}

List *pushcontext(List *const ctx)
// , const DagMap *const map)
{
// 	assert(map);
	assert(!ctx || ctx->ref.code == CTX);

	Context *const new = malloc(sizeof(Context));
	assert(new);
// 	*new = makecontext(map);
	*new = makecontext();

	return append(RL(refctx(new)), ctx);
}

static void freectx(Context *const ctx, const Array *const map)
{
	assert(ctx);
	assert(popenvironment(ctx->outs) == NULL);
	assert(popenvironment(ctx->ins) == NULL);
	freelist(ctx->forms);

//	freedag(ctx->dag, ctx->dagmap);
	freedag(ctx->dag, map);

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
