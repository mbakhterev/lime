#include "construct.h"
#include "util.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define DBGDAG 1
#define DBGSTD 2
#define DBGATTR 4
#define DBGMAP 8

// #define DBGFLAGS (DBGDAG | DBGSTD | DBGATTR)
// #define DBGFLAGS (DBGATTR)
// #define DBGFLAGS (DBGDAG)
#define DBGFLAGS (DBGMAP)

// #define DBGFLAGS 0

typedef struct
{
	FILE *const f;
	const Array *const U;

	List *L;
	List *F;

	Array *const map;
	Array *const nodes;

	const char *const tabstr;
	const unsigned tabs;

	const unsigned dbg;

	unsigned count;
} DState;

static void dumpreflist(
	FILE *const, const Array *const, Array *const nodes, const List *const);

void dumpref(
	FILE *const f, const Array *const U, Array *const nodes, const Ref r)
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

	case LIST:
		dumpreflist(f, U, nodes, r.u.list);
		break;

	case NODE:
	{
		assert(isnode(r));
		const Ref n = refmap(nodes, r);
		
		// Когда печатаем узел в списке, то не важно, в каком режиме
		// печатаем, в отладочном или нет. Адреса узлов надо указать для
		// узлов, а не для ссылок на них, которые могут быть текущими
		// N-номерами. Если же узел находится вне nodes (например, когда
		// nodes == NULL во время печати просто списка), то будет
		// выведен адрес узла, которой поможет его идентифицировать

		if(n.code != FREE)
		{
			assert(fprintf(f, "n%u", n.u.number) > 0);
		}
		else
		{
			assert(fprintf(f, "N:%p", (void *)r.u.list) > 0);
		}

		break;

	case PTR:
		assert(fprintf(f, "P:%p", (void *)r.u.pointer) > 0);
		break;

	case FORM:
		assert(r.u.form);
		assert(fprintf(f, "F:%p (D:%p S:%p)",
			(void *)r.u.form,
			(void *)r.u.form->u.dag.u.list,
			(void *)r.u.form->signature) > 0);
		break;

	case MAP:
		assert(r.u.array && r.u.array->code == MAP);
		assert(fprintf(f, "M:%p", (void *)r.u.array) > 0);
		break;
	
	case FREE:
		assert(fprintf(f, "-1") > 0);
		break;
	}

	default:
		assert(0);
	}

}

static int dumprefone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);

	const DState *const st = ptr;
	FILE *const f = st->f;
	assert(f);

	DBG(DBGATTR, "f: %p; code: %u", (void *)f, l->ref.code);

	dumpref(f, st->U, st->nodes, l->ref);

	if(l != st->L)
	{
		assert(fputs("; ", f) >= 0);
	}

	return 0;
}

static void dumpreflist(
	FILE *const f, const Array *const U, Array *const nodes,
	const List *const l)
{
	assert(f);

	DBG(DBGATTR, "f: %p", (void *)f);

	DState st =
	{
		.f = f,
		.L = (List *)l,
		.nodes = nodes,
		.U = U
	};

	assert('(' == fputc('(', st.f));
	forlist((List *)l, dumprefone, &st, 0);
	assert(')' == fputc(')', st.f));
}

void dumplist(FILE *const f, const Array *const U, const List *const l)
{
	dumpreflist(f, U, NULL, l);
}

char *strlist(const Array *const U, const List *const l)
{
	char *buf = NULL;
	size_t length = 0;
	FILE *f = newmemstream(&buf, &length);
	assert(f);

	dumplist(f, U, l);

	assert(fputc(0, f) != EOF);
	fclose(f);

	return buf;
}

char *strref(const Array *const U, Array *const nodemap, const Ref r)
{
	char *buf = NULL;
	size_t length = 0;
	FILE *f = newmemstream(&buf, &length);
	assert(f);

	dumpref(f, U, nodemap, r);

	assert(fputc(0, f) == 0);
	fclose(f);

	return buf;
}

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

	Array *const nodes = ptr;
	assert(nodes);

	tunerefmap(nodes, l->ref, refnum(nodes->count));

	return 0;
}

static void dumpattr(const DState *const st, const Ref attr)
{
	assert(st);

	FILE *const f = st->f;
	assert(f);
	
	DBG(DBGSTD, "f: %p", (void *)f);

	assert(fputc('\t', f) == '\t');
	dumpreflist(f, st->U, st->nodes, attr.u.list);
}

static void dumpsubdag(const DState *const st, const Ref dag)
{
	FILE *const f = st->f;
	assert(f);

	assert(fputc('\n', f) == '\n');
	dumpdag(st->dbg, f, st->tabs + 1, st->U, dag, st->map);
}

static int dumpone(List *const l, void *const ptr)
{
	const DState *const st = ptr;
	assert(st);

	assert(l);
	const Ref n = l->ref;
	assert(isnode(n));

	FILE *const f = st->f;
	const Array *const U = st->U;
	assert(f);
	assert(U);

	Array *const map = st->map;

	const Ref i = refmap(st->nodes, n);

	if(st->dbg)
	{
		assert(0 < fprintf(f, "\n%s\t%p\t.%s\tn%u",
			st->tabstr,
			(void *)n.u.list,
			atombytes(atomat(U, nodeverb(n, NULL))), i.u.number));
	}
	else
	{
		assert(0 < fprintf(f, "\n%s\t.%s\tn%u",
			st->tabstr,
			atombytes(atomat(U, nodeverb(n, NULL))), i.u.number));
	}

	const unsigned k = knownverb(n, map);

	DBG(DBGDAG, "map = %p; k = %u", (void *)map, k);

	(!k ?  dumpattr : dumpsubdag)(st, nodeattribute(n));

	if(l != st->L)
	{
		assert(fputc(';', f) == ';');
	}

	return 0;
}

void dumpdag(
	const unsigned dbg, FILE *const f, const unsigned tabs,
	const Array *const U, const Ref dag, Array *const map)
{
	assert(f);

	// FIXME: требовать ли Universe для работы dumpdag?
	assert(U);

	DBG(DBGDAG, "f: %p", (void *)f);

	Array *const nodes = newkeymap();
	forlist(dag.u.list, mapone, nodes, 0);

	DState st =
	{
		.f = f,
		.U = U,
		.map = map,
		.dbg = dbg,
		.nodes = nodes,
		.tabs = tabs,
		.tabstr = tabstr(tabs),
		.L = dag.u.list
	};

	assert(fprintf(f, "%s(", st.tabstr) > 0);
	forlist(dag.u.list, dumpone, &st, 0);
	assert(fprintf(f, "\n%s)", st.tabstr) > 0);

	free((void *)st.tabstr);
	freekeymap(nodes);
}

static int dumpbindingone(Binding *const b, void *const ptr)
{
	assert(b);
	assert(ptr);
	DState *const st = ptr;

	FILE *const f = st->f;
	assert(f);

	assert(fprintf(f, "\n%s\tkey(%u): ", st->tabstr, b->key.external) > 0);
	dumpref(st->f, st->U, NULL, b->key);

	assert(fprintf(f, "\n%s\tval(%u): ", st->tabstr, b->ref.external) > 0);
	dumpref(st->f, st->U, NULL, b->ref);

	assert(fputc('\n', st->f) == '\n');

	switch(b->ref.code)
	{
	case FORM:
		if(!b->ref.external)
		{
			st->F = append(st->F, RL(markext(b->ref)));
		}
		break;

	case MAP:
		if(!b->ref.external)
		{
			st->L = append(st->L, RL(markext(b->ref)));
		}
		break;
	}

	return 0;
}

static int dumpkeymapone(List *const l, void *const ptr)
{
	assert(l && iskeymap(l->ref));
	assert(ptr);
	const DState *const st = ptr;

	dumpkeymap(st->f, st->tabs + 1, st->U, l->ref.u.array);

	return 0;
}

void dumpkeymap(
	FILE *const f, const unsigned tabs, const Array *const U,
	const Array *const map)
{
	assert(f);
	assert(map && map->code == MAP);

	DState st =
	{
		.f = f,
		.U = U,
		.L = NULL,
		.F = NULL,
		.tabs = tabs,
		.tabstr = tabstr(tabs)
	};

	assert(fprintf(f, "\n%smap: %p", st.tabstr, (void *)map) > 0);

	if(map->count)
	{
		walkbindings((Array *)map, dumpbindingone, &st);
		forlist(st.L, dumpkeymapone, &st, 0);
	}
	else
	{
		assert(fputc('\n', f) == '\n');
	}

	free((void *)st.tabstr);
	freelist(st.L);
	freelist(st.F);
}

// typedef struct
// {
// 	FILE *const file;
// 	const Array *const universe;
// 	const char *const tabstr;
// 	const unsigned tabs;
// 	unsigned depth;
// } DEnvState;
// 
// static int dumpenvone(List *const l, void *const ptr)
// {
// 	assert(ptr);
// 	assert(l && l->ref.code == ENV && l->ref.u.environment);
// 
// 	DEnvState *const st = ptr;
// 	FILE *const f = st->file;
// 	const Array *const U = st->universe;
// 	const char *const tabs = st->tabstr;
// 	assert(f);
// 	assert(U);
// 
// 	const Array *const E = l->ref.u.environment;
// 	const Binding *const B = E->data;
// 
// 	assert(fprintf(f, "\n%sENV{%u}:", tabs, st->depth) > 0);
// 
// 	for(unsigned i = 0; i < E->count; i += 1)
// 	{
// 		assert(fprintf(f, "\n%s\tkey: ", tabs) > 0);
// 		dumplist(f, U, B[i].key);
// 
// 		assert(fprintf(f, "\n%s\tvalue: ", tabs) > 0);
// 		dumpref(f, U, NULL, B[i].ref);
// 
// 		assert(fputc('\n', f) == '\n');
// 	}
// 
// 	st->depth += 1;
// 
// 	return 0;
// }
// 
// void dumpenvironment(
// 	FILE *const f, const unsigned tabs,
// 	const Array *const U, const List *const env)
// {
// 	DEnvState st =
// 	{
// 		.tabs = tabs,
// 		.tabstr = tabstr(tabs),
// 		.file = f,
// 		.universe = U,
// 		.depth = 0
// 	};
// 
// 	forlist((List *)env, dumpenvone, &st, 0);
// 
// 	free((char *)st.tabstr);
// }
// 
// typedef struct
// {
// 	const Array *const U;
// 	FILE *const f;
// 	unsigned depth;
// } DCState;
// 
// static void dumpforms(
// 	FILE *const f, const unsigned tabs,
// 	const Array *const U, const List *const forms);
// 
// static int dumpctxone(List *const c, void *const ptr)
// {
// 	assert(c && c->ref.code == CTX);
// 	const Context *const ctx = c->ref.u.context;
// 
// 	assert(ptr);
// 	DCState *const st = ptr;
// 	const Array *const U = st->U;
// 	FILE *const f = st->f;
// 
// 	assert(U);
// 	assert(f);
// 
// 	assert(fprintf(f, "\nCTX{%u}:", st->depth) > 0);
// 
// 	for(unsigned i = 0; i < sizeof(ctx->R) / sizeof(Reactor); i += 1)
// 	{
// 		assert(fprintf(f, "\n\tR{%u}.outs:", i) > 0);
// 		dumpenvironment(f, 2, U, ctx->R[i].outs);
// 
// 		assert(fprintf(f, "\n\tR{%u}.ins:", i) > 0);
// 		dumpenvironment(f, 2, U, ctx->R[i].ins);
// 
// 		assert(fprintf(f, "\n\tR{%u}.forms: ", i) > 0);
// 		dumpforms(f, 2, U, ctx->R[i].forms);
// 
// 	}
// 
// 	assert(fprintf(f, "\n\tDAG:\n") > 0);
// 	dumpdag(1, f, 1, U, ctx->dag);
// 	assert(fputc('\n', f) == '\n');
// 
// 	st->depth += 1;
// 
// 	return 1;
// }
// 
// void dumpcontext(
// 	FILE *const f, const Array *const U, const List *const ctx)
// {
// 	DCState st =
// 	{
// 		.f = f, .U = U, .depth = 0
// 	};
// 
// 	forlist((List *)ctx, dumpctxone, &st, 0);
// }
// 
// typedef struct
// {
// 	const Array *const U;
// 	FILE *const f;
// 	const char *const tabstr;
// 	const unsigned tabs;
// } DFState;
// 
// static int dumpformone(List *const l, void *const ptr)
// {
// 	assert(l && l->ref.code == FORM && l->ref.u.form);
// 	assert(ptr);
// 
// 	const Form *frm = l->ref.u.form;
// 	const DFState *const st = ptr;
// 	FILE *const f = st->f;
// 	assert(st->U);
// 	assert(f);
// 
// 	assert(fprintf(f, "\n%sform: %p", st->tabstr, (void *)frm) > 0);
// 
// 	assert(fprintf(f, "\n%ssig %p: ",
// 		st->tabstr, (void *)frm->signature) > 0);
// 	dumplist(f, st->U, frm->signature);	
// 
// 	assert(fprintf(f, "\n%sdag %p:\n", st->tabstr, (void *)frm->u.dag) > 0);
// 	dumpdag(1, f, st->tabs, st->U, frm->u.dag);
// 
// 	assert(fputc('\n', f) == '\n');
// 
// 	return 0;
// }
// 
// void dumpforms(
// 	FILE *const f, const unsigned tabs,
// 	const Array *const U, const List *const forms)
// {
// 	DFState st =
// 	{
// 		.U = U, .f = f, .tabs = tabs, .tabstr = tabstr(tabs)
// 	};
// 
// 	forlist((List *)forms, dumpformone, &st, 0);
// }
