#include "construct.h"
#include "util.h"

#include <assert.h>

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

void doex(
	Core *const C, Marks *const M, const Ref N, const unsigned envnum)
{
	Array *const U = C->U;
	const Array *const E = C->E;
	const Array *const V = C->verbs.system;

	Array *const marks = M->marks;

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

	const Ref space = len > 0 ? searchspace(R[0], V, U) : reffree();
	const Ref name = len > 1 ? simplerewrite(R[1], marks, V) : reffree();

	if(len != 2 || space.code != ATOM || !isbasickey(name))
	{
		freeref(name);

		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	const List *const p 
		= tracepath(envkeymap(E, refenv(envnum)), U,
			readtoken(U, "ENV"), readtoken(U, "parent"));
	
	DL(key, RS(space, markext(name)));
	unsigned depth;
	const Binding *const b = pathlookup(p, key, &depth);
	freelist((List *)p);
	freeref(name);

	tunerefmap(marks, N, decodesearch(U, b, depth));
}

static Ref uniqatom(Array *const U, Array *const E, const Ref atom);

// void makeuniq(const Ref N, EState *const st)
// {
// 	const Ref r = nodeattribute(N);
// 	const unsigned line = nodeline(N);
// 	const unsigned char *const name = nodename(st->U, N);
// 
// 	if(r.code != LIST)
// 	{
// 		item = line;
// 		ERR("node \"%s\": expecting attribute list", name);
// 		return;
// 	}
// 
// 	const unsigned len = listlen(r.u.list);
// 	const Ref R[len];
// 	assert(writerefs(r.u.list, (Ref *)R, len) == len);
// 	if(len != 1 || R[0].code != ATOM)
// 	{
// 		item = line;
// 		ERR("node \"%s\": wrong attribute structure", name);
// 		return;
// 	}
// 
// 	Array *const E = envmap(st->envmarks, N);
// 	if(!E)
// 	{
// 		item = line;
// 		ERR("node \"%s\": no environment mark", name);
// 		return;
// 	}
// 	
// 	tunerefmap(st->marks, N, uniqatom(st->U, E, R[0]));
// }

void douniq(
	Core *const C, Marks *const M, const Ref N, const unsigned envnum)
{
	Array *const U = C->U;
	const Array *const E = C->E;
	Array *const marks = M->marks;

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

	if(len != 1 || R[0].code != ATOM)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	Array *const env = envkeymap(E, refenv(envnum));

	tunerefmap(marks, N, uniqatom(U, env, R[0]));
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
