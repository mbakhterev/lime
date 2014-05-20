#include "construct.h"
#include "util.h"

#include <assert.h>

static Ref reconstructcore(
	Array *const U, Array *const symdefs, Array *const typedefs,
	const Ref, const Array *const V,
	const Array *const E, const Array *const T, const Array *const S);

Ref reconstruct(
	Array *const U,
	const Ref dag, const Array *const V,
	const Array *const E, const Array *const T, const Array *const S)
{
	Array *const symdefs = newkeymap();
	Array *const typedefs = newkeymap();

	const Ref r = reconstructcore(U, symdefs, typedefs, dag, V, E, T, S);

	freekeymap(typedefs);
	freekeymap(symdefs);

	return r;
}

typedef struct
{
	Array *const U;
	Array *const marks;
	Array *const envdefs;

	Array *const symdefs;
	Array *const typedefs;

	List *L;

	const Array *const E;
	const Array *const T;
	const Array *const S;

	const Array *const verbs;
} RState;

static int stageone(List *const, void *const);
static int stagetwo(List *const, void *const);

Ref reconstructcore(
	Array *const U, Array *const symdefs, Array *const typedefs,
	const Ref dag, const Array *const verbs,
	const Array *const E, const Array *const T, const Array *const S)
{
	assert(isdag(dag));

	RState st =
	{
		.U = U,
		.marks = newkeymap(),
		.envdefs = newkeymap(),
		.symdefs = symdefs,
		.typedefs = typedefs,
		.L = NULL,
		.E = E,
		.T = T,
		.S = S,
		.verbs = verbs
	};

	const unsigned ok
		= forlist(dag.u.list, stageone, &st, 0) == 0
			&& forlist(dag.u.list, stagetwo, &st, 0) == 0;

	freekeymap(st.envdefs);
	freekeymap(st.marks);

	if(ok)
	{
		return refdag(st.L);
	}

	freelist(st.L);
	return reffree();
}

static unsigned defenv(Array *const envdefs, const Ref N, const Array *const U)
{
	const Ref r = nodeattribute(N);
	assert(r.code == LIST);

	const unsigned len = listlen(r.u.list);
	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len);

	assert(len == 2 && R[0].code == ENV && R[1].code == NODE);

	tunerefmap(envdefs, R[1], R[0]);
	return 0;
	
}

int stageone(List *const l, void *const ptr)
{
	assert(l && isnode(l->ref) && !l->ref.external);

	assert(ptr);
	RState *const st = ptr;

	switch(nodeverb(l->ref, st->verbs))
	{
	case EDEF:
		return defenv(st->envdefs, l->ref, st->U);
	}

	return 0;
}

// static int gatherone(List *const, void *const);
static void gather(RState *const, const Ref);

static Ref totalrewrite(const Ref, const Array *const);

int stagetwo(List *const l, void *const ptr)
{
	assert(l && isnode(l->ref) && !l->ref.external);
	const Ref N = l->ref;

	assert(ptr);
	RState *const st = ptr;

	Array *const U = st->U;
	Array *const sdefs = st->symdefs;
	Array *const tdefs = st->typedefs;
	Array *const marks = st->marks;
	const Array *const E = st->E;
	const Array *const T = st->T;
	const Array *const S = st->S;
	const Array *const V = st->verbs;

	const Ref r = nodeattribute(N);

	if(r.code != DAG)
	{
		gather(st, r);
	}

	const Ref attr
		= r.code == DAG ?
			  reconstructcore(U, sdefs, tdefs, r, V, E, T, S)
			: totalrewrite(r, st->marks);
	
	assert(attr.code != FREE);

	const Ref n = newnode(nodeverb(N, NULL), attr, nodeline(N));
	tunerefmap(marks, N, n);

	st->L = append(st->L, RL(n));

	return 0;
}

typedef struct
{
	List *L;
	const Array *const map;
} TRState;

static int totalrewriteone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	TRState *const S = ptr;

	const Ref r = totalrewrite(l->ref, S->map);
	if(r.code == FREE)
	{
		return !0;
	}

	S->L = append(S->L, RL(r));

	return 0;
}

Ref totalrewrite(const Ref R, const Array *const map)
{
	switch(R.code)
	{
	case NUMBER:
	case ATOM:
		return R;
	
	case TYPE:
	case SYM:
	case ENV:
	case NODE:
	{
		const Ref r = refmap(map, R);
		return r;
	}
	
	case LIST:
	{
		TRState st =
		{
			.L = NULL,
			.map = map
		};

		if(forlist(R.u.list, totalrewriteone, &st, 0))
		{
			freelist(st.L);
			return reffree();
		}

		return reflist(st.L);
	}

	default:
		assert(0);
	}

	return reffree();
}

static int gatherone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	gather(ptr, l->ref);
	return 0;
}

void gather(RState *const st, const Ref r)
{
	Array *const marks = st->marks;

	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case NODE:
		return;

	case TYPE:
		if(refmap(marks, r).code == FREE)
		{
			tunerefmap(marks, r, r);
		}

		return;

	case SYM:
		if(refmap(marks, r).code == FREE)
		{
			tunerefmap(marks, r, r);
		}

		return;
	
	case ENV:
		if(refmap(marks, r).code == FREE)
		{
			tunerefmap(marks, r, r);
		}

		return;
	
	case LIST:
		assert(forlist(r.u.list, gatherone, st, 0) == 0);
		return;
	
	default:
		assert(0);
	}
}
