#include "construct.h"
#include "util.h"

#include <assert.h>
#include <stdio.h>
#include <ctype.h>

static LoadCurrent LC(List *const nodes, List *const refs)
{
	return (LoadCurrent) { .nodes = nodes, .refs = refs };
}

static LoadCurrent onatom(
	const LoadContext *const ctx, List *const env, List *const nodes);

static const char *const genverbs[] =
{
	"A",
	NULL
};

static const LoadAction genactions[] =
{
	onatom
};

LoadCurrent onatom(
	const LoadContext *const ctx, List *const env, List *const nodes)
{
	FILE *const f = ctx->file;
	Array *const U = ctx->universe;

	assert(f);
	assert(U && U->code == ATOM);

	const int c = skipspaces(f);
	if(isxdigit(c))
	{
		assert(ungetc(c, f) == c);
	}
	else
	{
		errexpect(c, ES("[0-9A-fa-f]"));
	}

	return LC(nodes, RL(refnum(ATOM, loadatom(U, f))));
}

LoadContext gencontext(FILE *const f, Array *const U)
{
	Array *const km = malloc(sizeof(Array));
	assert(km);
	*km = keymap(U, 0, genverbs);

	return (LoadContext)
	{
		.file = f,
		.state = NULL,
		
		.universe = U,
		.keymap = km,
		.actions = genactions,

		.keyonly = 0
	};
}
