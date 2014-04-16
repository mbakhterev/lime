#include "construct.h"
#include "util.h"

#include <assert.h>

#define EX	0
#define EQ	1
#define UNIQ	2
#define TENV	3
#define FENV	4
#define SNODE	5

static const char *const verbs[] =
{
	[EX] = "Ex",
	[EQ] = "Eq",
	[UNIQ] = "Uniq",
	[TENV] = "TEnv",
	[FENV] = "FEnv",
	[SNODE] = "S",
	NULL
};

typedef struct
{
	Array *const U;
	const Array *const envmarks;
	const Array *const escape;

	Array *const marks;
	const Array *const verbs;
} EState;

static void eval(const Ref, EState *const);

static int evalone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	eval(l->ref, ptr);
	return 0;
}

static void exists(const Ref, EState *const);
static void compare(const Ref, EState *const);
static void makeuniq(const Ref, EState *const);

static void eval(const Ref N, EState *const E)
{
	switch(N.code)
	{
	// Попробуем воспользоваться разделением на LIST и DAG
	case NUMBER:
	case ATOM:
	case TYPE:
	case LIST:
		return;
	
	case DAG:
		forlist(N.u.list, evalone, E, 0);
		return;

	case NODE:
		assert(!N.external);

		switch(nodeverb(N, E->verbs))
		{
		case EX:
			exists(N, E);
			return;

		case EQ:
			compare(N, E);
			return;

		case UNIQ:
			makeuniq(N, E);
			return;

		default:
			if(!knownverb(N, E->escape))
			{
				eval(nodeattribute(N), E);
			}
		}
	}
}

static Ref searchspace(const Ref r, const Array *const V, Array *const U)
{
	if(r.code != ATOM)
	{
		return reffree();
	}

	switch(verbmap(V, r.u.number))
	{
	case TENV:
		return decoatom(U, DTYPE);

	case FENV:
		return decoatom(U, DFORM);

	case SNODE:
		return decoatom(U, DSYM);
	
	default:
		return reffree();
	}
}

static Ref decodesearch(
	Array *const U, const Binding *const b, const unsigned depth)
{
	if(!b)
	{
		return readtoken(U, "FREE");
	}	

	if(!depth)
	{
		return readtoken(U, "BOUND");
	}

	return readtoken(U, "SHADOW");
}

void exists(const Ref N, EState *const st)
{
	Array *const U = st->U;
	const Array *const V = st->verbs;
// 	const Array *const E = st->env;
	Array *const marks = st->marks;

	const Ref r = nodeattribute(N);
	if(r.code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting attribute list", nodename(U, N));
		return;
	}

	const unsigned len = listlen(r.u.list);
	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len);

	const Ref space = len >= 1 ? searchspace(R[0], V, U) : reffree();

	if(len != 2 || space.code == FREE || !isbasickey(R[1]))
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	const Array *const E = envmap(st->envmarks, N);
	if(!E)
	{
		item = nodeline(N);
		ERR("node \"%s\": no environment evaluation", nodename(U, N));
		return;
	}

	const List *const p
		= tracepath(E, U, readtoken(U, "ENV"), readtoken(U, "parent"));
	
	DL(key, RS(space, markext(R[1])));
	unsigned depth;
	const Binding *const b = pathlookup(p, key, &depth);
	freelist((List *)p);

	tunerefmap(marks, N, decodesearch(U, b, depth));
}

void compare(const Ref N, EState *const st)
{
}

static Ref uniqatom(Array *const U, Array *const E, const Ref atom);

void makeuniq(const Ref N, EState *const st)
{
	const Ref r = nodeattribute(N);
	const unsigned line = nodeline(N);
	const unsigned char *const name = nodename(st->U, N);

	if(r.code != LIST)
	{
		item = line;
		ERR("node \"%s\": expecting attribute list", name);
		return;
	}

	const unsigned len = listlen(r.u.list);
	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len);
	if(len != 1 || R[0].code != ATOM)
	{
		item = line;
		ERR("node \"%s\": wrong attribute structure", name);
		return;
	}

	Array *const E = envmap(st->envmarks, N);
	if(!E)
	{
		item = line;
		ERR("node \"%s\": no environment mark", name);
		return;
	}
	
	tunerefmap(st->marks, N, uniqatom(st->U, E, R[0]));
}

Ref uniqatom(Array *const U, Array *const E, const Ref r)
{
	assert(r.code == ATOM);
	const Atom atom = atomat(U, r.u.number);
	const unsigned len = atomlen(atom);
	const unsigned hint = atomhint(atom);
	const unsigned char *const bytes = atombytes(atom);

	DL(key, RS(decoatom(U, DUNIQ), r));
	Binding *const b = (Binding *)bindingat(E, bindkey(E, key));
	assert(b);
	if(b->ref.code == FREE)
	{
		b->ref = refnum(0);
	}
	assert(b->ref.code == NUMBER && b->ref.u.number < MAXLEN);

	// 8 байтов должно хватить для представления счётчика в hex-виде
	char buf[len + 1 + 8 + 1];
	assert(sprintf(buf, "%sx%x", bytes, b->ref.u.number) < sizeof(buf));
	b->ref.u.number += 1;

	return readpack(U, strpack(hint, buf));
}

void exeqeval(
	Array *const U,
	Array *const marks,
	const Ref dag, const Array *const escape, const Array *const envmarks)
{
	EState st =
	{
		.U = U,
		.marks = marks,
		.escape = escape,
		.envmarks = envmarks,
		.verbs = newverbmap(U, 0, verbs)
	};

	eval(dag, &st);

	freekeymap((Array *)st.verbs);
}
