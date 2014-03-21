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
// #define DBGFLAGS (DBGMAP)

#define DBGFLAGS 0

typedef struct
{
	FILE *const f;
	const Array *const U;

	List *L;
	List *F;
	List *D;

	const Array *const map;
	Array *const nodes;

	const Array *const escape;
	Array *const visited;

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
	
	case DAG:
		assert(isdaglist(r.u.list));
		assert(fprintf(f, "D:%p", (void *)r.u.list));
		break;

	case FORM:
		assert(isformlist(r.u.list));
		assert(fprintf(f, "F:%p (D:%p S:%p C:%u)",
			(void *)r.u.list,
			(void *)formdag(r).u.list,
			(void *)formkeys(r).u.list,
			formcounter(r)) > 0);
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
	dumpdag(st->dbg, f, st->tabs + 1, st->U, dag);
// 	, st->map);
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

// 	const Array *const map = st->map;

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

// 	const unsigned k = knownverb(n, map);
// 
// 	DBG(DBGDAG, "map = %p; k = %u", (void *)map, k);
// 
// 	(!k ?  dumpattr : dumpsubdag)(st, nodeattribute(n));

	const Ref attr = nodeattribute(n);
	(attr.code != DAG ? dumpattr : dumpsubdag)(st, attr);

	if(l != st->L)
	{
		assert(fputc(';', f) == ';');
	}

	return 0;
}

void dumpdag(
	const unsigned dbg, FILE *const f, const unsigned tabs,
	const Array *const U, const Ref dag)
// 	, const Array *const map)
{
	assert(f);
	// FIXME: требовать ли Universe для работы dumpdag?
	assert(U);
	assert(isdag(dag));

	DBG(DBGDAG, "f: %p", (void *)f);

	Array *const nodes = newkeymap();
	forlist(dag.u.list, mapone, nodes, 0);

	DState st =
	{
		.f = f,
		.U = U,
//		.map = map,
		.map = NULL,
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

static unsigned islink(Array *const U, const Ref r)
{
// 	DL(pattern, RS(decoatom(U, DMAP), reffree()));
// 	return keymatch(pattern, &r, NULL, 0, NULL);
	return decomatch(r, U, DMAP);
}

static int dumpbindingone(const Binding *const b, void *const ptr)
{
	assert(b);
	assert(ptr);
	DState *const st = ptr;

	FILE *const f = st->f;
	assert(f);

	char addr[64];
	addr[0] = '\0';

	if(st->dbg)
	{
		sprintf(addr, "%p:\t", (void *)b);
	}

	assert(
		fprintf(f, "\n\t%s%skey(%u): ",
			st->tabstr, addr, b->key.external) > 0);
	dumpref(st->f, st->U, NULL, b->key);

	if(st->dbg)
	{
		sprintf(addr, "\t\t");
	}

	assert(
		fprintf(f, "\n\t%s%sval(%u): ",
			st->tabstr, addr, b->key.external) > 0);
	dumpref(st->f, st->U, NULL, b->ref);

	assert(fputc('\n', st->f) == '\n');

	switch(b->ref.code)
	{
	case DAG:
		if(!b->ref.external)
		{
			st->D = append(st->D, RL(markext(b->ref)));
		}
		break;

	case FORM:
		if(!b->ref.external)
		{
			st->F = append(st->F, RL(markext(b->ref)));
		}
		break;

	case MAP:
// 		if(!b->ref.external)
// 		{
// 			st->L = append(st->L, RL(markext(b->ref)));
// 		}
// 		break;

		assert(b->ref.external);

// 		if(!setmap(st->visited, b->ref) && !setmap(st->escape, b->key))
// 		{
// 			st->L = append(st->L, RL(markext(b->ref)));
// 		}

		if(islink((Array *)st->U, b->key)
			&& !setmap(st->escape, b->key))
		{
			st->L = append(st->L, RL(markext(b->ref)));
		}
	}

	return 0;
}

static int dumpformone(List *const l, void *const ptr)
{
	assert(l && isform(l->ref));
	assert(ptr);
	const DState *const st = ptr;

	const Ref keys = formkeys(l->ref);
	assert(fprintf(
		st->f, "\n%s\tform-sig(%u) %p:\t", st->tabstr,
		keys.external, (void *)keys.u.list) > 0);
	dumpref(st->f, st->U, NULL, keys);

	const Ref dag = formdag(l->ref);
	assert(fprintf(
		st->f, "\n%s\tform-dag(%u) %p:\n", st->tabstr,
		dag.external, (void *)dag.u.list) > 0);
	dumpdag(st->dbg, st->f, st->tabs + 1, st->U, dag);
// 	, NULL);

	assert(fputc('\n', st->f) == '\n');

	return 0;
}

static int dumpdagone(List *const l, void *const ptr)
{
	assert(l && isdag(l->ref));
	const Ref D = l->ref;
	assert(ptr);
	const DState *const st = ptr;

	assert(fprintf(
		st->f, "\n%s\tdag: %p\n", st->tabstr, (void *)D.u.list) > 0);
	dumpdag(st->dbg, st->f, st->tabs + 1, st->U, D);
	assert(fputc('\n', st->f) == '\n');

	return 0;
}

static void dumpkeymapcore(
	const unsigned debug, FILE *const f, const unsigned tabs,
	const Array *const U,
	const Array *const map, const Array *const escape, Array *const V);

static int dumpkeymapone(List *const l, void *const ptr)
{
	assert(l && iskeymap(l->ref));
	assert(ptr);
	const DState *const st = ptr;

	if(!setmap(st->visited, l->ref))
	{
		assert(fputc('\n', st->f) == '\n');
		dumpkeymapcore(
			st->dbg, st->f, st->tabs + 1,
			st->U, l->ref.u.array, st->escape, st->visited);
	}

	return 0;
}

static void dumpkeymapcore(
	const unsigned debug, FILE *const f, const unsigned tabs,
	const Array *const U,
	const Array *const map, const Array *const escape, Array *const V)
{
	assert(f);
	assert(map && map->code == MAP);

	tunesetmap(V, refkeymap((Array *)map));

	DState st =
	{
		.f = f,
		.U = U,
		.L = NULL,
		.F = NULL,
		.D = NULL,
		.tabs = tabs,
		.tabstr = tabstr(tabs),
		.dbg = debug,
		.visited = V,
		.escape = escape
	};

	assert(fprintf(f, "%smap: %p", st.tabstr, (void *)map) > 0);

	if(map->count)
	{
		walkbindings((Array *)U,
			(Array *)map, escape, dumpbindingone, &st);

		// FIXME: тут нужен более разумный подход
		if(st.F && U)
		{
			forlist(st.F, dumpformone, &st, 0);
		}

		if(st.D && U)
		{
			forlist(st.D, dumpdagone, &st, 0);
		}

		if(st.L)
		{
			forlist(st.L, dumpkeymapone, &st, 0);
		}
	}
	else
	{
		assert(fputc('\n', f) == '\n');
	}

	free((void *)st.tabstr);
	freelist(st.L);
	freelist(st.D);
	freelist(st.F);
}

void dumpkeymap(
	const unsigned debug, FILE *const f, const unsigned tabs,
	const Array *const U,
	const Array *const map, const Array *const escape)
{
	Array *const V = newkeymap();
	dumpkeymapcore(debug, f, tabs, U, map, escape, V);
	freekeymap(V);
}

void dumptable(
	FILE *const f, const unsigned tabs, const Array *const U,
	const Array *const types)
{
	assert(f);
	assert(U);
	assert(types);

	const char *const pad = tabstr(tabs);

	assert(fprintf(f, "%stable: %p", pad, (void *)types) > 0);

	if(!types->count)
	{
		assert(fputc('\n', f) == '\n');
		return;
	}

	const Binding *const B = types->u.data;

	for(unsigned i = 0; i < types->count; i += 1)
	{
		assert(fprintf(f,
			"\n%s\t%u:\tkey(%u): ", pad, i, B[i].key.external) > 0);
		dumpref(f, U, NULL, B[i].key);

		assert(fprintf(f,
			"\n%s\t\tdef(%u): ", pad, B[i].ref.external) > 0);
		dumpref(f, U, NULL, B[i].ref);

		assert(fputc('\n', f) == '\n');
	}

	free((char *)pad);
}
