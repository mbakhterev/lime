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

typedef struct
{
	FILE *file;
	const Array *universe;
	const Array *dagmap;
} DumpContext;

typedef struct
{
	const Array *nodes;
//	const List *first;
	const char *tabstr;
	unsigned tabs;
} DumpCurrent;

typedef struct
{
	FILE *f;
	const List *last;
	const Array *nodes;
	const Array *universe;
} DAState;

static const char *tabstr(const unsigned tabs)
{
	char *const s = malloc(tabs + 1);
	assert(s);

	memset(s, '\t', tabs);
	s[tabs] = '\0';

	return s;
}

static int mapone(List *const l, void *const ptr)
{
	assert(l && l->ref.code == NODE);
	assert(ptr);
	Array *const nodes = ptr;

	assert(nodes->count == ptrmap(nodes, l->ref.u.node));

	return 0;
}

static void dumpattrlist(
	List *const, const Array *const,
	FILE *const, const Array *const);

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
	{
		const Array *const U = st->universe;
		assert(U);
		const Atom a = atomat(U, l->ref.u.number);
		const unsigned len = atomlen(a);

//		assert(fprintf(f, "A:%u", l->ref.u.number) > 0);

		assert(fprintf(f, "'%02x.%u.\"", atomhint(a), len) > 0);
		assert(fwrite(atombytes(a), 1, len, f) == len);
		assert(fputc('"', f) == '"');
		break;
	}

	case TYPE:
		assert(fprintf(f, "T:%u", l->ref.u.number) > 0);
		break;

	case LIST:
		dumpattrlist(l->ref.u.list, st->universe, f, st->nodes);
		break;

	case NODE:
	{
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

static void dumpattrlist(
	List *const l, const Array *const U,
	FILE *const f, const Array *const nodes)
{
	assert(f && nodes);
	DBG(DBGATTR, "f: %p", (void *)f);

	const DAState st =
	{
		.f = f,
		.last = l,
		.nodes = nodes,
		.universe = U
	};

	assert('(' == fputc('(', st.f));
	forlist(l, dumpattrone, (DAState *)&st, 0);
	assert(')' == fputc(')', st.f));
}

static void dumpattr(
	const DumpContext *const ctx, const DumpCurrent *const dc,
	List *const attr)
{
	assert(ctx);

	FILE *const f = ctx->file;
	assert(f);

	const Array *const U = ctx->universe;
	assert(U);
	
	DBG(DBGSTD, "f: %p", (void *)f);

	dumpattrlist(attr, U, f, dc->nodes);
}

static void dumpsubdag(
	const DumpContext *const ctx, const DumpCurrent *const dc,
	List *const dag)
{
	const Array *const U = ctx->universe;
	FILE *const f = ctx->file;

	assert(U);
	assert(f);

	dumpdag(f, dc->tabs + 1, U, dag, ctx->dagmap);
}

void dumpdag(
	FILE *const f, const unsigned tabs, const Array *const U, 
	const List *const dag, const Array *const dagmap)
{
	assert(f);
	assert(U);

	const DumpContext ctx =
	{
		.file = f,
		.universe = U,
		.dagmap = dagmap
	};

	DBG(DBGDAG, "f: %p", (void *)ctx.file);
	Array nodes = makeptrmap();
	forlist((List *)dag, mapone, &nodes, 0);

	DumpCurrent dc =
	{
		.nodes = &nodes,
		.tabs = tabs,
		.tabstr = tabstr(tabs)
	};

	assert(fprintf(f, "\n%s(", dc.tabstr) > 0);

	for(unsigned i = 0; i < nodes.count; i += 1)
	{
		const Node *const n = ptrdirect(dc.nodes, i);
		assert(n);

// 		FIXME: ?		
// 		assert(0 < fprintf(f, "\n%s\t.%s\tn%u\t= ",
		assert(0 < fprintf(f, "\n%s\t.%s\tn%u\t",
			dc.tabstr,
			atombytes(atomat(U, n->verb)),
			i));

		(uireverse(dagmap, n->verb) == -1 ?
			dumpattr : dumpsubdag)(&ctx, &dc, n->u.attributes);

		if(i + 1 < nodes.count)
		{
			assert(fputc(';', f) == ';');
		}
	}

	assert(fprintf(f, "\n%s)", dc.tabstr) > 0);

	free((void *)dc.tabstr);

	freeptrmap(&nodes);
}
