#include "construct.h"
#include "util.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define DBGDAG 1
#define DBGSTD 2
#define DBGATTR 4

// #define DBGFLAGS (DBGDAG | DBGSTD | DBGATTR)
// #define DBGFLAGS (DBGATTR)

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
		assert(fputs("; ", f) >= 0);
	}

	return 0;
}

static void dumpattrlist(
	List *const l, const Array *const U,
	FILE *const f, const Array *const nodes)
{
//	assert(f && nodes);

	assert(f);

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

// static int dumper(List *const l, void *const file);
// 
// typedef struct {
// 	FILE *const file;
// 	const List *const first;
// 	const Array *const universe;
// } DumpState;
// 
// static int dumper(List *const l, void *const state)
// {
// 	DumpState *const s = state;
// 	assert(s);
// 
// 	FILE *const f = s->file;
// 	assert(f);
// 
// 	const Array *const U = s->universe;
// 
// 	const unsigned isfinal = l->next == s->first;
// 
// 	switch(l->ref.code)
// 	{
// 	case NUMBER:
// 		assert(fprintf(f, "%u", l->ref.u.number) > 0);
// 		break;
// 	
// 	case ATOM:
// 		if(U)
// 		{
// 			const Atom a = atomat(U, l->ref.u.number);
// 			assert(0 < 
// 				fprintf(f, "%02x.\"%s\"",
// 					atomhint(a), atombytes(a)));
// 		}
// 		else
// 		{
// 			assert(fprintf(f, "A:%u", l->ref.u.number) > 0);
// 		}
// 
// 		break;
// 	
// 	case TYPE:
// 		assert(fprintf(f, "T:%u", l->ref.u.number) > 0);
// 		break;
// 	
// 	case NODE:
// 	{
// 		const Node *const n = l->ref.u.node;
// 		assert(n);
// 		
// 		if(U)
// 		{
// 			const char *const verb
// 				= (char *)atombytes(atomat(U, n->verb));
// 
// 			assert(fprintf(f, "N:%p.%s", (void *)n, verb) > 0);
// 		}
// 		else
// 		{
// 			assert(fprintf(f, "N:%p.%u", (void *)n, n->verb) > 0);
// 		}
// 
// 		break;
// 	}
// 	
// 	case LIST:
// 		dumplist(f, U, l->ref.u.list);
// 		break;
// 
// 	default:
// 		assert(0);
// 	}
// 
// 	if(!isfinal)
// 	{
// 		assert(fputc(' ', f) != EOF);
// 	}
// 
// 	return 0;
// }
// 
// void dumplist(
// 	FILE *const f, const Array *const U, const List *const list)
// {
// 	assert(fputc('(', f) != EOF);
// 
// 	DumpState s =
// 	{
// 		.file = f,
// 		.first = list != NULL ? list->next : NULL,
// 		.universe = U
// 	};
// 
// 	forlist((List *)list, dumper, &s, 0);
// 
// 	assert(fputc(')', f) != EOF);
// }

void dumplist(FILE *const f, const Array *const U, const List *const l)
{
	dumpattrlist((List *)l, U, f, NULL);
}

char *strlist(const Array *const U, const List *const l)
{
	char *buf = NULL;
	size_t length = 0;
	FILE *f = newmemstream(&buf, &length);
	assert(f);

	dumplist(f, NULL, l);

	assert(fputc(0, f) != EOF);
	fclose(f);

	return buf;
}

typedef struct
{
	FILE *const file;
	const Array *const universe;
	unsigned depth;
} DEnvState;

static int dumpenvone(List *const l, void *const ptr)
{
	assert(ptr);
	assert(l && l->ref.code == ENV && l->ref.u.environment);

	DEnvState *const st = ptr;
	FILE *const f = st->file;
	const Array *const U = st->universe;
	assert(f);
	assert(U);

	const Array *const E = l->ref.u.environment;
	const Binding *const B = E->data;

	assert(fprintf(f, "ENV{%u}:", st->depth) > 0);

	for(unsigned i = 0; i < E->count; i += 1)
	{
		switch(B[i].ref.code)
		{
		case FORM:
			assert(fprintf(f, "\n\tform-key: ") > 0);
			dumplist(f, U, B[i].key);

			assert(fprintf(f, "\n\tform-signature: ") > 0);
			dumplist(f, U, B[i].ref.u.form->signature);

			assert(fprintf(f, "\n\tform-dag: ") > 0);
			dumpdag(f, 1, U,
				B[i].ref.u.form->u.dag,
				B[i].ref.u.form->map);

			assert(fputc('\n', f) == '\n');

			break;
		}
	}

	st->depth += 1;

	return 0;
}

void dumpenvironment(
	FILE *const f, const Array *const U, const List *const env)
{
	const DEnvState st =
	{
		.file = f,
		.universe = U,
		.depth = 0
	};

	forlist((List *)env, dumpenvone, (void *)&st, 0);
}
