#include "construct.h"
#include "util.h"
#include "nodeutil.h"

#include <assert.h>

#define DBGS 1

// #define DBGFLAGS (DBGS)

#define DBGFLAGS (0)

static Ref getexisting(
	const Array *const env, Array *const U, const Ref key)
{
	List *const l
		= tracepath(
			env, U,
			readpack(U, strpack(0, "ENV")),
			readpack(U, strpack(0, "parent")));
	
	const Ref K = decorate(markext(key), U, DSYM);
	const Binding *const b = pathlookup(l, K, NULL);
	freeref(K);

	freelist(l);

	if(b && b->ref.code == SYM)
	{
// 		// Ключ больше не нужен, можно от него избавиться
// 		freeref(key);
		return b->ref;
	}

	return reffree();
}

static Ref setnew(
	Array *const env, Array *const U, Array *const symbols,
	const Ref key, const Ref typeref)
{
	if(typeref.code != TYPE)
	{
		return reffree();
	}

	const Ref name = forkref(key, NULL);
	const Ref id = reflist(RL(envid(U, env), name));

	const unsigned sid = mapreadin(symbols, id);
	Binding *const symb = (Binding *)bindingat(symbols, sid);

	if(!symb)
	{
		// name будет освобождена автоматически
		freeref(id);
		return reffree();
	}

	symb->ref = typeref;

// 	const unsigned symnum = symb - (Binding *)symbols->u.data;
// 	assert(symnum + 1 == symbols->count);

	const Ref K = decorate(markext(name), U, DSYM);
	Binding *const b = (Binding *)bindingat(env, mapreadin(env, K));

	if(!b)
	{
		// id и name освобождать не надо. Они сохренены в symbols.
		// Вместе с ней и будут освобождены

		freeref(K);
		return reffree();
	}

// 	b->ref = refnum(symnum);
// 	return b->ref;

	b->ref = refsym(sid);
	return b->ref;
}

void dosnode(
	Core *const C, Marks *const M, const Ref N, const unsigned envid)
{
	Array *const U = C->U;
	Array *const E = C->E;
	Array *const S = C->S;
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

	const Ref key = len > 0 ? simplerewrite(R[0], marks, V) : reffree();
	const Ref type = len > 1 ? refmap(marks, R[1]) : reffree();

	if(DBGFLAGS & DBGS)
	{
		char *const rstr = strref(U, NULL, R[0]);
		char *const kstr = strref(U, NULL, key);
		DBG(DBGS, "(R 0 = %s) (key = %s)", rstr, kstr);
		free(kstr);
		free(rstr);
	}

	if(len < 1 || 2 < len
		|| !isbasickey(key)
		|| (len == 2 && type.code != TYPE))
	{
		freeref(key);

		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	Array *const env = envkeymap(E, refenv(envid));
	const Ref id
		= len == 1 ? getexisting(env, U, key)
		: len == 2 ? setnew(env, U, S, key, type)
		: reffree();

	if(id.code == FREE)
	{
		char *const kstr = strref(U, NULL, key);
		freeref(key);

		item = nodeline(N);

// 		ERR("node \"%s\": can't %s symbol for key: %s",
// 			nodename(U, N),
// 			len == 1 ? "locate" : "allocate",
// 			kstr);

		ERRNODE(U, N, "can't %s symbol for key: %s",
			len == 1 ? "locate" : "allocate",
			kstr);

		free(kstr);
		return;
	}

	freeref(key);
	tunerefmap(marks, N, id);
}

Ref symname(const Array *const symbols, const Ref id)
{
	assert(symbols);
	assert(id.code == SYM && id.u.number < symbols->count);

	const Binding *const b = (Binding *)symbols->u.data + id.u.number;
	assert(b->key.code == LIST && b->ref.code == TYPE);

	const unsigned len = listlen(b->key.u.list);
	const Ref R[len];
	assert(len == 2
		&& writerefs(b->key.u.list, (Ref *)R, len) == len
		&& R[0].code == ENV);

	return markext(R[1]);
}

Ref symenv(const Array *const symbols, const Ref id)
{
	assert(symbols);
	assert(id.code == SYM && id.u.number < symbols->count);

	const Binding *const b = (Binding *)symbols->u.data + id.u.number;
	assert(b->key.code == LIST && b->ref.code == TYPE);

	const Ref R[2];
	assert(splitpair(b->key, (Ref *)R) && R[0].code == ENV);

	return R[0];
}

Ref symtype(const Array *const symbols, const Ref id)
{
	assert(symbols);
	assert(id.code == SYM && id.u.number < symbols->count);

	const Binding *const b = (Binding *)symbols->u.data + id.u.number;
	assert(b->ref.code == TYPE);
	return b->ref;
}
