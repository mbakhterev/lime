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
//	const Array *map;
} DumpContext;

typedef struct
{
	const Array *const nodes;
	const char *const tabstr;
	const unsigned tabs;
	const unsigned dbg;
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
	assert(l);
	assert(l->ref.code == NODE);
	assert(ptr);
	Array *const nodes = ptr;

	assert(nodes->count == ptrmap(nodes, l->ref.u.node));

	return 0;
}

static void dumpattrlist(
	List *const, const Array *const,
	FILE *const, const Array *const);

static void dumpref(
	FILE *const f, const Array *const U,
	const Array *const nodes, const Ref r)
{
	switch(r.code)
	{
	case NUMBER:
		assert(fprintf(f, "%u", r.u.number) > 0);
		break;

	case ATOM:
	{
		if(U)
		{
			const Atom a = atomat(U, r.u.number);
			const unsigned len = atomlen(a);

			assert(fprintf(f, "'%02x.%u.\"", atomhint(a), len) > 0);
			assert(fwrite(atombytes(a), 1, len, f) == len);
			assert(fputc('"', f) == '"');
		}
		else
		{
			assert(fprintf(f, "A:%u", r.u.number) > 0);
		}

		break;
	}

	case TYPE:
		assert(fprintf(f, "T:%u", r.u.number) > 0);
		break;

	case FORM:
		assert(r.u.form);
		assert(fprintf(f, "F:%p (D:%p S:%p)",
			(void *)r.u.form,
			(void *)r.u.form->u.dag,
			(void *)r.u.form->signature) > 0);
		break;

	case LIST:
		dumpattrlist(r.u.list, U, f, nodes);
		break;

	case NODE:
	{
		const Node *const n = r.u.node;
		assert(n);

		const unsigned k = ptrreverse(nodes, n);

		// Когда печатаем узел в списке, то не важно, в каком режиме
		// печатаем, в отладочном или нет. Адреса узлов надо указать для
		// узлов, а не для ссылок на них, которые могут быть текущими
		// N-номерами. Если же узел находится вне nodes (например, когда
		// nodes == NULL во время печати просто списка), то будет
		// выведен адрес узла, которой поможет его идентифицировать

		if(k != -1)
		{
			assert(fprintf(f, "n%u", k) > 0);
		}
		else
		{
			assert(fprintf(f, "N:%p", (void *)n) > 0);
		}

		break;
	}

	default:
		assert(0);
	}

}

static int dumpattrone(List *const l, void *const ptr)
{
	assert(l);
	const DAState *const st = ptr;
	assert(st);
	FILE *const f = st->f;
	assert(f);

	DBG(DBGATTR, "f: %p; code: %u", (void *)f, l->ref.code);

// 	switch(l->ref.code)
// 	{
// 	case NUMBER:
// 		assert(fprintf(f, "%u", l->ref.u.number) > 0);
// 		break;
// 
// 	case ATOM:
// 	{
// 		const Array *const U = st->universe;
// 
// // 		assert(U);
// 		if(U)
// 		{
// 			const Atom a = atomat(U, l->ref.u.number);
// 			const unsigned len = atomlen(a);
// 
// 			assert(fprintf(f, "'%02x.%u.\"", atomhint(a), len) > 0);
// 			assert(fwrite(atombytes(a), 1, len, f) == len);
// 			assert(fputc('"', f) == '"');
// 		}
// 		else
// 		{
// 			assert(fprintf(f, "A:%u", l->ref.u.number) > 0);
// 		}
// 
// 		break;
// 	}
// 
// 	case TYPE:
// 		assert(fprintf(f, "T:%u", l->ref.u.number) > 0);
// 		break;
// 
// 	case LIST:
// 		dumpattrlist(l->ref.u.list, st->universe, f, st->nodes);
// 		break;
// 
// 	case NODE:
// 	{
// 		const Node *const n = l->ref.u.node;
// 		assert(n);
// 
// 		const Array *const map = st->nodes;
// // 		assert(map);
// 		const unsigned k = ptrreverse(map, n);
// 
// 		// Когда печатаем узел в списке, то не важно, в каком режиме
// 		// печатаем, в отладочном или нет. Адреса узлов надо указать для
// 		// узлов, а не для ссылок на них, которые могут быть текущими
// 		// N-номерами. Если же узел находится вне nodes (например, когда
// 		// nodes == NULL во время печати просто списка), то будет
// 		// выведен адрес узла, которой поможет его идентифицировать
// 
// 		if(k != -1)
// 		{
// 			assert(fprintf(f, "n%u", k) > 0);
// 		}
// 		else
// 		{
// 			assert(fprintf(f, "N:%p", (void *)n) > 0);
// 		}
// 
// // Немного фантазий о высоком уровне LiME
// // 		(if (k != -1)
// // 			(assert (fprintf f "N%u" k > 0))
// // 			(assert (fprintf f "N:%p" (val void.ptr = n) > 0))
// 
// 		break;
// 	}
// 
// 	default:
// 		assert(0);
// 	}

	dumpref(f, st->universe, st->nodes, l->ref);

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

	assert(fputc('\t', f) == '\t');
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

	assert(fputc('\n', f) == '\n');
	dumpdag(dc->dbg, f, dc->tabs + 1, U, dag);
	// , ctx->map);
}

void dumpdag(
	const unsigned dbg, FILE *const f, const unsigned tabs,
	const Array *const U, const List *const dag)
// 	, const Array *const map)
{
	assert(f);

	// FIXME: требовать ли Universe для работы dumpdag?
	assert(U);

	const DumpContext ctx =
	{
		.file = f,
		.universe = U
// 		.map = map
	};

	DBG(DBGDAG, "f: %p", (void *)ctx.file);
	Array nodes = makeptrmap();
	forlist((List *)dag, mapone, &nodes, 0);

	DumpCurrent dc =
	{
		.dbg = dbg,
		.nodes = &nodes,
		.tabs = tabs,
		.tabstr = tabstr(tabs)
	};

	assert(fprintf(f, "%s(", dc.tabstr) > 0);

	for(unsigned i = 0; i < nodes.count; i += 1)
	{
		const Node *const n = ptrdirect(dc.nodes, i);
		assert(n);

		if(dbg)
		{
			assert(0 < fprintf(f, "\n%s\t%p\t.%s\tn%u",
				dc.tabstr,
				(void *)n, atombytes(atomat(U, n->verb)), i));
		}
		else
		{
			assert(0 < fprintf(f, "\n%s\t.%s\tn%u",
				dc.tabstr,
				atombytes(atomat(U, n->verb)), i));
		}

// 		(uireverse(map, n->verb) == -1 ?
		(!n->dag ?
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
	const char *const tabstr;
	const unsigned tabs;
	unsigned depth;
} DEnvState;

static int dumpenvone(List *const l, void *const ptr)
{
	assert(ptr);
	assert(l && l->ref.code == ENV && l->ref.u.environment);

	DEnvState *const st = ptr;
	FILE *const f = st->file;
	const Array *const U = st->universe;
	const char *const tabs = st->tabstr;
	assert(f);
	assert(U);

	const Array *const E = l->ref.u.environment;
	const Binding *const B = E->data;

	assert(fprintf(f, "\n%sENV{%u}:", tabs, st->depth) > 0);

	for(unsigned i = 0; i < E->count; i += 1)
	{
// 		switch(B[i].ref.code)
// 		{
// 		case FORM:
// 			assert(fprintf(f, "\n%s\tform-key: ", tabs) > 0);
// 			dumplist(f, U, B[i].key);
// 
// 			assert(fprintf(f, "\n%s\tform-signature: ", tabs) > 0);
// 			dumplist(f, U, B[i].ref.u.form->signature);
// 
// 			assert(fprintf(f, "\n%s\tform-dag:\n", tabs) > 0);
// 			dumpdag(1, f, st->tabs + 1, U,
// 				B[i].ref.u.form->u.dag,
// 				B[i].ref.u.form->map);
// 
// 			assert(fputc('\n', f) == '\n');
// 
// 			break;
// 		}

		assert(fprintf(f, "\n%s\tkey: ", tabs) > 0);
		dumplist(f, U, B[i].key);
		assert(fprintf(f, "\n%s\tvalue: ", tabs) > 0);
		dumpref(f, U, NULL, B[i].ref);
		assert(fputc('\n', f) == '\n');
	}

	st->depth += 1;

	return 0;
}

void dumpenvironment(
	FILE *const f, const unsigned tabs,
	const Array *const U, const List *const env)
{
	DEnvState st =
	{
		.tabs = tabs,
		.tabstr = tabstr(tabs),
		.file = f,
		.universe = U,
		.depth = 0
	};

	forlist((List *)env, dumpenvone, &st, 0);

	free((char *)st.tabstr);
}

typedef struct
{
	const Array *const U;
	FILE *const f;
	unsigned depth;
} DCState;

static void dumpforms(
	FILE *const f, const unsigned tabs,
	const Array *const U, const List *const forms);

static int dumpctxone(List *const c, void *const ptr)
{
	assert(c && c->ref.code == CTX);
	const Context *const ctx = c->ref.u.context;

	assert(ptr);
	DCState *const st = ptr;
	const Array *const U = st->U;
	FILE *const f = st->f;

	assert(U);
	assert(f);

	assert(fprintf(f, "\nCTX{%u}:", st->depth) > 0);

	for(unsigned i = 0; i < sizeof(ctx->R) / sizeof(Reactor); i += 1)
	{
		assert(fprintf(f, "\n\tR{%u}.outs:", i) > 0);
		dumpenvironment(f, 2, U, ctx->R[i].outs);

		assert(fprintf(f, "\n\tR{%u}.ins:", i) > 0);
		dumpenvironment(f, 2, U, ctx->R[i].ins);

		assert(fprintf(f, "\n\tR{%u}.forms: ", i) > 0);
		dumpforms(f, 2, U, ctx->R[i].forms);

	}

	assert(fprintf(f, "\n\tDAG:\n") > 0);
	dumpdag(1, f, 1, U, ctx->dag);
	assert(fputc('\n', f) == '\n');

	st->depth += 1;

	return 1;
}

void dumpcontext(
	FILE *const f, const Array *const U, const List *const ctx)
{
	DCState st =
	{
		.f = f, .U = U, .depth = 0
	};

	forlist((List *)ctx, dumpctxone, &st, 0);
}

typedef struct
{
	const Array *const U;
	FILE *const f;
	const char *const tabstr;
	const unsigned tabs;
} DFState;

static int dumpformone(List *const l, void *const ptr)
{
	assert(l && l->ref.code == FORM && l->ref.u.form);
	assert(ptr);

	const Form *frm = l->ref.u.form;
	const DFState *const st = ptr;
	FILE *const f = st->f;
	assert(st->U);
	assert(f);

	assert(fprintf(f, "\n%sform: %p", st->tabstr, (void *)frm) > 0);

	assert(fprintf(f, "\n%ssig %p: ",
		st->tabstr, (void *)frm->signature) > 0);
	dumplist(f, st->U, frm->signature);	

	assert(fprintf(f, "\n%sdag %p:\n", st->tabstr, (void *)frm->u.dag) > 0);
	dumpdag(1, f, st->tabs, st->U, frm->u.dag);
	// , frm->map);

	assert(fputc('\n', f) == '\n');

	return 0;
}

void dumpforms(
	FILE *const f, const unsigned tabs,
	const Array *const U, const List *const forms)
{
	DFState st =
	{
		.U = U, .f = f, .tabs = tabs, .tabstr = tabstr(tabs)
	};

	forlist((List *)forms, dumpformone, &st, 0);
}

