#include "construct.h"
#include "util.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define DBGDAG 1
#define DBGSTD 2
#define DBGATTR 4

// #define DBGFLAGS (DBGDAG | DBGSTD | DBGATTR)

#define DBGFLAGS 0

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

typedef struct
{
	FILE *f;
	const List *last;
	const Array *nodes;
} DAState;

static void dumpattrlist(List *const, FILE *const, const Array *const);

static int dumpattrone(List *const l, void *const ptr)
{
	assert(l);
	const DAState *const st = ptr;
	assert(st);
	FILE *const f = st->f;
	assert(f);

	DBG(DBGATTR, "f: %p; code: %u", (void *)f, l->ref.code);

	switch(l->ref.code)
	{
	case NUMBER:
		assert(fprintf(f, "%u", l->ref.u.number) > 0);
		break;

	case ATOM:
		assert(fprintf(f, "A:%u", l->ref.u.number) > 0);
		break;

	case TYPE:
		assert(fprintf(f, "T:%u", l->ref.u.number) > 0);
		break;

	case LIST:
		dumpattrlist(l->ref.u.list, f, st->nodes);
		break;

	case NODE: {
		const Node *const n = l->ref.u.node;
		assert(n);
		const Array *const map = st->nodes;
		assert(map);
		const unsigned k = ptrreverse(map, n);

		if(k != -1)
		{
			assert(fprintf(f, "n%u", k) > 0);
		}

		break;
	}

	default:
		assert(0);
	}

	if(l != st->last)
	{
		assert(fputs("; ", f) > 0);
	}

	return 0;
}

static void dumpattrlist(List *const l, FILE *const f, const Array *const nodes)
{
	assert(f && nodes);
	DBG(DBGATTR, "f: %p", (void *)f);

	const DAState st =
	{
		.f = f,
		.last = l,
		.nodes = nodes,
	};

	assert('(' == fputc('(', st.f));
	forlist(l, dumpattrone, (DAState *)&st, 0);
	assert(')' == fputc(')', st.f));
}

static void onstddump(const LDContext *const ctx, List *const attr)
{
	assert(ctx);
	assert(ctx->state);
	
	const DState *const st = ctx->state;
	
	DBG(DBGSTD, "f: %p", st->f);

	dumpattrlist(attr, st->f, &st->nodes);
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
//		.ctx = ctx,
		.tabs = tabs,
		.tabstr = tabstr(tabs),
		.first = dag
	};	

	DBG(DBGDAG, "f: %p", (void *)st.f);

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
