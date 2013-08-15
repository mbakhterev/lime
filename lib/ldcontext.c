#include "construct.h"
#include "util.h"

#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

static LoadCurrent LC(List *const nodes, List *const refs)
{
	return (LoadCurrent) { .nodes = nodes, .refs = refs };
}

static LoadCurrent onloadatom(
	const LoadContext *const ctx, List *const env, List *const nodes);

static void ondumpatom(
	const DumpContext *const, const DumpCurrent *const, List *const);

static const char *const genverbs[] =
{
	"A",
	NULL
};

static const LoadAction genloadactions[] =
{
	onloadatom
};

static const DumpAction gendumpactions[] =
{
	ondumpatom
};

LoadCurrent onloadatom(
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

	return LC(nodes, RL(refatom(loadatom(U, f))));
}

void ondumpatom(
	const DumpContext *const ctx, const DumpCurrent *const dc,
	List *const attr)
{
}

LoadContext genloadcontext(FILE *const f, Array *const U)
{
//	Array *const km = malloc(sizeof(Array));
//	assert(km);

// 	*km = keymap(U, 0, genverbs);

	return (LoadContext)
	{
		.file = f,
		
		.universe = U,
		.keymap = keymap(U, 0, genverbs),
		.onload = genloadactions,

		.keyonly = 0
	};
}

void freeloadcontext(LoadContext *const ctx)
{
	freeuimap(&ctx->keymap);
	memset(ctx, 0, sizeof(LoadContext));
}

DumpContext gendumpcontext(FILE *const f, Array *const U)
{
	return (DumpContext)
	{
		.file = f,
		.universe = U,
		.keymap = keymap(U, 0, genverbs),
		.ondump = gendumpactions
	};
}

void freedumpcontext(DumpContext *const ctx)
{
	freeuimap(&ctx->keymap);
	memset(ctx, 0, sizeof(DumpContext));
}
