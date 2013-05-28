#include "construct.h"
#include "util.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char *tabstr(const unsigned tabs)
{
	char *const s = malloc(tabs + 1);
	assert(s);

	memset(s, '\t', tabs);
	s[tabs] = '\0';

	return s;
}

typedef struct
{
	const LDContext *ctx;
	FILE *f;
	const char *tabstr;
	const List *first;
	unsigned tabs;
	Array nodes;
} DState;

static int mapone(List *const l, void *const ptr)
{
	assert(l && l->ref.code == NODE);
	DState *const st = ptr;
	assert(st);

	assert(st->nodes.count == ptrmap(&st->nodes, l->ref.u.node));

	return 0;
}

static int dumpone(List *const l, void *const ptr)
{
	const DState *const st = ptr;
	assert(st && st->f && st->ctx && st->ctx->universe);
	const Array *const U = st->ctx->universe;	
	
	assert(l && l->ref.code == NODE);
	const Node *const n = l->ref.u.node;
	assert(n);	

	assert(0 < fprintf(st->f,
		"\n%s\t'%s\tn%u\t= ",
		st->tabstr,
		atombytes(atomat(U, n->verb)),
		ptrreverse(&st->nodes, n)));

	if(uireverse(st->ctx->keymap, n->verb) != -1)
	{
	}
	else
	{
	}

	if(l != st->first)
	{
		assert(fputc(';', st->f) == ';');
	}

	return 0;
}

const char *dumpdag(
	const LDContext *const ctx, List *const dag, const unsigned tabs)
{
	char *buf = NULL;
	size_t sz = 0;

	DState st =
	{
		.nodes = makeptrmap(),
		.f = newmemstream(&buf, &sz),
		.ctx = ctx,
		.tabs = tabs,
		.tabstr = tabstr(tabs),
		.first = dag
	};	

	forlist(dag, mapone, &st, 0);

	assert(fprintf(st.f, "\n%s(", st.tabstr) > 0);
	forlist(dag, dumpone, &st, 0);
	assert(fprintf(st.f, "\n%s)", st.tabstr) > 0);

	free((void *)st.tabstr);
	fclose(st.f);
	freeptrmap(&st.nodes);

	return buf;
}
