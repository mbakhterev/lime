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
//	const LDContext *ctx;
	FILE *f;
	const char *tabstr;
	const List *first;
	unsigned tabs;
	Array nodes;
} DState;

static int mapone(List *const l, void *const ptr)
{
	assert(l && l->ref.code == NODE);
	assert(ptr);
	DState *const st = ((LDContext *)ptr)->state;
	assert(st);

	assert(st->nodes.count == ptrmap(&st->nodes, l->ref.u.node));

	return 0;
}

static void onstddump(const LDContext *const ctx, List *const dag)
{
}

// static int dumpone(List *const l, void *const ptr)
// {
// 	const DState *const st = ptr;
// 	assert(st && st->f && st->ctx && st->ctx->universe);
// 	const Array *const U = st->ctx->universe;	
// 	
// 	assert(l && l->ref.code == NODE);
// 	const Node *const n = l->ref.u.node;
// 	assert(n);	
// 
// 	assert(0 < fprintf(st->f,
// 		"\n%s\t'%s\tn%u\t= ",
// 		st->tabstr,
// 		atombytes(atomat(U, n->verb)),
// 		ptrreverse(&st->nodes, n)));
// 
// 	const unsigned key = uireverse(st->ctx->keymap, n->verb);
// 
// 	(key == -1 ? onstddump : st->ctx->ondump[key])(
// 		st->ctx, st->f, n->u.attributes, st->tabs);
// 
// 	if(l != st->first)
// 	{
// 		assert(fputc(';', st->f) == ';');
// 	}
// 
// 	return 0;
// }

const char *dumpdag(
	const LDContext *const ctx, List *const dag, const unsigned tabs)
{
	char *buf = NULL;
	size_t sz = 0;

	DState st =
	{
		.nodes = makeptrmap(),
		.f = newmemstream(&buf, &sz),
//		.ctx = ctx,
		.tabs = tabs,
		.tabstr = tabstr(tabs),
		.first = dag
	};	

	assert(ctx);
	void *const tmp = ctx->state;
	((LDContext *)ctx)->state = &st;

	const Array *const U = ctx->universe;
	assert(U);

	forlist(dag, mapone, (void *)ctx, 0);

	assert(fprintf(st.f, "\n%s(", st.tabstr) > 0);

	for(unsigned i = 0; i < st.nodes.count; i += 1)
	{
		const Node *const n = ptrdirect(&st.nodes, i);
		assert(n);

		assert(0 < fprintf(st.f, "\n%s\t'%s\tn%u\t= ",
			st.tabstr,
			atombytes(atomat(U, n->verb)),
			i));

		const unsigned key = uireverse(ctx->keymap, n->verb);

		(key == -1 ?
			onstddump : ctx->ondump[key])(ctx, n->u.attributes);

		if(i + 1 < st.nodes.count)
		{
			assert(fputc(';', st.f) == ';');
		}
	}

	assert(fprintf(st.f, "\n%s)", st.tabstr) > 0);

	free((void *)st.tabstr);
	fclose(st.f);
	freeptrmap(&st.nodes);

	((LDContext *)ctx)->state = tmp;

	return buf;
}
