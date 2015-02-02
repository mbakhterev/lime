#include "construct.h"
#include "util.h"

#include <assert.h>

static Ref reconstructcore(
	Array *const U,
	Array *const envdefs, Array *const symdefs, Array *const typedefs,
	const Ref, const Array *const V,
	const Array *const E, const Array *const T, const Array *const S,
	const unsigned envnum);

Ref reconstruct(
	Array *const U,
	const Ref dag, const Array *const V,
	const Array *const E, const Array *const T, const Array *const S)
{
	Array *const envdefs = newkeymap();
	Array *const symdefs = newkeymap();
	Array *const typedefs = newkeymap();

	const Ref r
		= reconstructcore(U,
			envdefs, symdefs, typedefs, dag, V, E, T, S, 0);

	freekeymap(typedefs);
	freekeymap(symdefs);
	freekeymap(envdefs);

	return r;
}

typedef struct
{
	Array *const U;
	Array *const marks;
	Array *const envmarks;

	Array *const envdefs;
	Array *const symdefs;
	Array *const typedefs;

	List *L;
	List *G;

	const Array *const E;
	const Array *const T;
	const Array *const S;

	const Array *const verbs;
	const unsigned envnum;
} RState;

static int stageone(List *const, void *const);
static int stagetwo(List *const, void *const);

Ref reconstructcore(
	Array *const U,
	Array *const envdefs, Array *const symdefs, Array *const typedefs,
	const Ref dag, const Array *const verbs,
	const Array *const E, const Array *const T, const Array *const S,
	const unsigned envnum)
{
	assert(isdag(dag));

	RState st =
	{
		.U = U,
		.marks = newkeymap(),
		.envmarks = newkeymap(),
		.envdefs = envdefs,
		.symdefs = symdefs,
		.typedefs = typedefs,
		.L = NULL,
		.G = NULL,
		.E = E,
		.T = T,
		.S = S,
		.verbs = verbs,
		.envnum = envnum
	};

	const unsigned ok
		= forlist(dag.u.list, stageone, &st, 0) == 0
			&& forlist(dag.u.list, stagetwo, &st, 0) == 0;

	freekeymap(st.envmarks);
	freekeymap(st.marks);

	if(ok)
	{
		return refdag(append(st.G, st.L));
	}

	freelist(st.L);
	freelist(st.G);

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
		return defenv(st->envmarks, l->ref, st->U);
	}

	return 0;
}

static void gather(RState *const, const Ref);
static Ref totalrewrite(const Ref, const Array *const);
static int selectenv(const unsigned, const Array *const envmarks, const Ref N);

int stagetwo(List *const l, void *const ptr)
{
	assert(l && isnode(l->ref) && !l->ref.external);
	const Ref N = l->ref;

	assert(ptr);
	RState *const st = ptr;

	Array *const U = st->U;
	Array *const edefs = st->envdefs;
	Array *const sdefs = st->symdefs;
	Array *const tdefs = st->typedefs;
	Array *const marks = st->marks;
	Array *const envmarks = st->envmarks;
	const Array *const E = st->E;
	const Array *const T = st->T;
	const Array *const S = st->S;
	const Array *const V = st->verbs;
	const unsigned envnum = st->envnum;

	const Ref r = nodeattribute(N);

	if(r.code != DAG)
	{
		gather(st, r);
	}

	const Ref attr
		= r.code != DAG ?
			  totalrewrite(r, st->marks)
			: reconstructcore(U, edefs, sdefs, tdefs,
				r, V, E, T, S, selectenv(envnum, envmarks, N));
	
	assert(attr.code != FREE);

	const Ref n = newnode(nodeverb(N, NULL), attr, nodeline(N));
	tunerefmap(marks, N, n);

	st->L = append(st->L, RL(n));

	return 0;
}

int selectenv(
	const unsigned envnum, const Array *const envmarks, const Ref N)
{
	const Ref t = refmap(envmarks, N);
	return t.code == ENV ? t.u.number : envnum;
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
	case NODE:
	{
		const Ref r = refmap(map, R);
		if(r.code != FREE)
		{
			assert(r.code == NODE);
			return r;
		}

		return reffree();
	}

	case SYM:
	case ENV:
		return refmap(map, R);
	
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

static void gathertype(RState *const, const Ref);
static void gatherenv(RState *const, const Ref);
static void gathersym(RState *const, const Ref);

void gather(RState *const st, const Ref r)
{
// 	Array *const marks = st->marks;

	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case NODE:
		return;

	case TYPE:
		gathertype(st, r);
		return;

	case SYM:
// 		if(refmap(marks, r).code == FREE)
// 		{
// 			tunerefmap(marks, r, r);
// 		}

		gathersym(st, r);
		return;
	
	case ENV:
		gatherenv(st, r);
		return;
	
	case LIST:
		assert(forlist(r.u.list, gatherone, st, 0) == 0);
		return;
	
	default:
		assert(0);
	}
}

void gathertype(RState *const st, const Ref r)
{
	Array *const marks = st->marks;
	Array *const U = st->U;
	Array *const tdefs = st->typedefs;

	{
		const Ref t = refmap(st->marks, r);
		if(t.code != FREE)
		{
			assert(t.code == NODE && t.external);
			return;
		}
	}

	const Binding *const b = typeat(st->T, r);

	// Сначала нужно добавить узлы, необходимые для описания структуры типа
	gather(st, b->key);

	// Затем добавить в список сам узел с описанием типа и отождествить с
	// ним r

	const Ref tnodeattr = totalrewrite(b->key, marks);
	assert(tnodeattr.code != FREE);

	const Ref tnode = newnode(readtoken(U, "T").u.number, tnodeattr, 0);
	tunerefmap(marks, r, tnode);

	st->G = append(st->G, RL(tnode));

	if(b->ref.code == FREE || setmap(tdefs, r))
	{
		return;
	}

	// Если у типа есть определение, и оно ещё не было сделано, то надо
	// добавить в список узел TDef. И пометить r, как определённый. Перед
	// этим нужно добавить в список всё необходимое

	gather(st, b->ref);

	const Ref tdefattr
		= reflist(RL(markext(tnode), totalrewrite(b->ref, marks)));
	assert(tdefattr.code != FREE);

	const Ref tdef = newnode(readtoken(U, "TDef").u.number, tdefattr, 0);
	tunesetmap(tdefs, r);

	st->G = append(st->G, RL(tdef));
}

// static Ref idbylink(Array *const U, const Ref, Array *const env);

void gatherenv(RState *const st, const Ref R)
{
	Array *const U = st->U;
	Array *const marks = st->marks;
// 	Array *const edefs = st->envdefs;
	const Array *const E = st->E;

	{
		const Ref t = refmap(marks, R);
		if(t.code != FREE)
		{
			assert(t.code == NODE && t.external);
			return;
		}
	}

	const Ref path = forkref(markext(envrootpath(E, R)), NULL);
	assert(path.code == LIST);

	// Сначала узел, описывающий само окружение. На него будут направлены
	// все соответствующие E:N ссылки

	const Ref aE = readtoken(U, "E");

	const Ref enode = newnode(aE.u.number, reflist(RL(path)), 0);
	tunerefmap(marks, R, enode);

// 	// Нам может потребоваться определить this и parent ссылки, поэтому
// 	// сразу в список узел не помещаем: трассировка parent может быть
// 	// длинной, а для отладки лучше соблюсти плотную группировку 
// 
// 	if(setmap(edefs, R))
// 	{
// 		// Окружение уже определено. Ничего больше ссылки по имени на
// 		// него не нужно
// 
// 		st->G = append(st->G, RL(enode));
// 		return;
// 	}
// 
// 	// Нужно определить окружение. Сначала узнаем идентификаторы this и
// 	// parent и соберём для них выражения
// 
// 	const Ref athis = readtoken(U, "this");
// 	const Ref aparent = readtoken(U, "parent");
// 
// 	const Ref thisid = idbylink(U, athis, envkeymap(E, R));
// 	const Ref parentid = idbylink(U, aparent, envkeymap(E, R));
// 
// 	// Если parent или this указывают на само окружение, то не надо его
// 	// выдавать. Будет сделано отдельно
// 
// 	if(parentid.code == ENV && parentid.u.number != R.u.number)
// 	{
// 		gather(st, parentid);
// 	}
// 
// 	if(thisid.code == ENV && thisid.u.number != R.u.number)
// 	{
// 		gather(st, thisid);
// 	}
// 
// 	// Тут можно выдавать уже узлы

	st->G = append(st->G, RL(enode));

// 	if(thisid.code == ENV)
// 	{
// 		const Ref thispath
// 			= reflist(append(forklist(path.u.list), RL(athis)));
// 
// 		const Ref thisnode
// 			= newnode(aE.u.number, reflist(RL(
// 				thispath, refmap(marks, thisid))), 0);
// 
// 		st->G = append(st->G, RL(thisnode));
// 	}
// 
// 	if(parentid.code == ENV)
// 	{
// 		const Ref parentpath
// 			= reflist(append( forklist(path.u.list), RL(aparent)));
// 		
// 		const Ref parentnode
// 			= newnode(aE.u.number, reflist(RL(
// 				parentpath, refmap(marks, parentid))), 0);
// 
// 		st->G = append(st->G, RL(parentnode));
// 	}
// 
// 	tunesetmap(edefs, R);
}

// Ref idbylink(Array *const U, const Ref name, Array *const env)
// {
// 	const Array *const t
// 		= linkmap(U, env, readtoken(U, "ENV"), name, reffree());
// 
// 	return t ? envid(U, t) : reffree();
// }

void gathersym(RState *const st, const Ref R)
{
	Array *const U = st->U;
	Array *const marks = st->marks;
	Array *const sdefs = st->symdefs;
	const Array *const S = st->S;
	const unsigned envnum = st->envnum;

	{
		const Ref t = refmap(marks, R);
		if(t.code != FREE)
		{
			assert(t.code == NODE && t.external);
			return;
		}
	}

	const Ref aS = readtoken(U, "S");
	const Ref name = forkref(markext(symname(S, R)), NULL);

	if(setmap(sdefs, R))
	{
		const Ref snode
			= newnode(aS.u.number, reflist(RL(name)), 0);

		st->G = append(st->G, RL(snode));
		tunerefmap(marks, R, snode);

		return;
	}

	const Ref stype = symtype(S, R);
	assert(stype.code == TYPE);
	gather(st, stype);

	const Ref senv = symenv(S, R);
	assert(senv.code == ENV);
	if(senv.u.number != envnum)
	{
		gather(st, senv);
	}

	const Ref snode
		= newnode(aS.u.number,
			reflist(RL(name, refmap(marks, stype))), 0);
	
	st->G = append(st->G, RL(snode));
	tunerefmap(marks, R, snode);

	if(senv.u.number != envnum)
	{
		const Ref aEDef = readtoken(U, "EDef");
		const Ref edef
			= newnode(
				aEDef.u.number,
				reflist(RL(
					refmap(marks, senv), markext(snode))),
				0);

		st->G = append(st->G, RL(edef));
	}

	tunesetmap(sdefs, R);
}
