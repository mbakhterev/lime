#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGEVE	1
#define DBGAL	2
#define DBGAE	4
#define DBGDED	8

// #define DBGFLAGS (DBGEVE | DBGAL)
// #define DBGFLAGS (DBGEVE)
// #define DBGFLAGS (DBGAE)
// #define DBGFLAGS (DBGDED)

#define DBGFLAGS 0

static unsigned isenode(const Ref N, const Array *const limeverbs)
{
	return N.code == NODE && nodeverb(N, limeverbs) == ENODE;
}

static unsigned isvalidtarget(const Ref N, const Array *const limeverbs)
{
	if(N.code != NODE)
	{
		return 0;
	}

	switch(nodeverb(N, limeverbs))
	{
	case -1:
		return !0;

	case SNODE:
	case TENV:
	case FENV:
	case UNIQ:
		return !0;
	
	default:
		return 0;
	}

	return 0;
}

void doedef(
	Array *const envdefs, Array *const envkeep,
	const Ref N, const Core *const C)
{
	const Array *const U = C->U;

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

	const unsigned eok = len > 0 && isenode(R[0], C->verbs.system);
	const unsigned tok = len > 1 && isvalidtarget(R[1], C->verbs.system);

	DBG(DBGDED, "len eok tok = %u (%u: %s) %u",
		len, eok, nodename(U, R[0]), tok);

	if(len != 2 || !eok || !tok)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	// Запоминаем, что для узла R[1] окружение переопределяется узлом R[0]
	tunerefmap(envdefs, R[1], R[0]);

	// Если Env помечал не системный узел, то его надо оставить в графе.
	// Если системный, то нужно стереть, так как и системный узел будет
	// стёрт

	if(!knownverb(R[1], C->verbs.system))
	{
		tunesetmap(envkeep, N);
	}
}

static unsigned maypass(Array *const U, const Array *const map);
static Array *nextpoint(Array *const U, const Array *const map);

static Array *newtarget(
	Array *const U, const Array *const map, const Ref id, void *const E);

void doenode(Core *const C, Marks *const M, const Ref N, const unsigned env)
{
	Array *const U = C->U;
	Array *const E = C->E;
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

	const Ref t = len == 2 ? refmap(marks, R[1]): reffree();

	if(len < 1 || len > 2 || (len == 2 && t.code != ENV))
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	const Ref K = simplerewrite(R[0], marks, V);
	if(K.code != LIST || !isbasickey(K))
	{
		freeref(K);

		item = nodeline(N);
		ERR("node \"%s\": can't rewrite 1st argument to basic key list",
			nodename(U, N));
		return;
	}

	const Ref tmap
		= t.code == ENV ? refkeymap(envkeymap(E, t)) : reffree();

	if(len == 2 && tmap.code == FREE)
	{
		freeref(K);

		item = nodeline(N);
		ERR("node \"%s\": environment isn't evaluated", nodename(U, N));
		return;
	}

	// Здесь у нас есть путь к окружению. Ref-а нового окружения (или
	// (Ref.code == FREE) в случае его отсутсвия). Нужно раздобыть ещё
	// текущий root. Все проверки внутри envkeymap

	Array *const root = envkeymap(E, refenv(env));

	// Остаётся запустить makepath

	const Array *const target
		= makepath(
			root, U, readtoken(U, "ENV"), K.u.list, tmap,
			maypass, newtarget, nextpoint, E);

	if(!target)
	{
		char *const kstr = strref(U, NULL, K);
		freeref(K);

		ERR("node \"%s\": can't %s environment with key: %s",
			nodename(U, N),
			len == 1 ? "trace" : "bind",
			kstr);

		free(kstr);
		return;
	}

	assert(tmap.code == FREE || target == tmap.u.array);

	freeref(K);

	tunerefmap(marks, N, envid(U, target));
}

static Array *nextpoint(Array *const U, const Array *const map)
{
	return (Array *)map;
}

static unsigned maypass(Array *const U, const Array *const map)
{
	return !0;
}

static Array *newtarget(
	Array *const U, const Array *const map, const Ref id, void *const state)
{
	Array *const T = newkeymap();
	Array *const E = state;

	const Ref mappath
		= forkref(cleanext(envrootpath(E, envid(U, map))), NULL);
	assert(mappath.code == LIST);

	const Ref path = reflist(append(mappath.u.list, RL(forkref(id, NULL))));

	const unsigned tid = mapreadin(E, path);
	assert(tid != -1);
	((Binding *)bindingat(E, tid))->ref = refkeymap(T);

	setenvid(U, T, tid);

	// FIXME: небольшой хак, чтобы не менять интерфейс makepath, но иметь
	// возможность обрабатывать специальное имя "/", указывающее на корневое
	// окружение

	Array *const R = envkeymap(E, refenv(0));
	assert(linkmap(U, T,
		readtoken(U, "ENV"), readtoken(U, "/"), refkeymap(R)) == R);
	
	// FIXME: автоматические this и parent. Кажется, разумно их создавать
	// именно так

	assert(linkmap(U, T,
		readtoken(U, "ENV"), readtoken(U, "this"), refkeymap(T)) == T);
	
	assert(linkmap(U, T,
		readtoken(U, "ENV"),
		readtoken(U, "parent"), refkeymap((Array *)map)) == map);

	return T;
}

void setenvid(Array *const U, Array *const env, const unsigned id)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "ENVID")));
	Binding *const b = (Binding *)bindingat(env, bindkey(env, key));
	assert(b && b->ref.code == FREE);

	b->ref = refenv(id);
}

Ref envid(Array *const U, const Array *const env)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "ENVID")));
	const Binding *const b = bindingat(env, maplookup(env, key));
	assert(b && b->ref.code == ENV);

	return b->ref;
}

static const Binding *idtobind(const Array *const E, const Ref id)
{
	assert(id.code == ENV && id.u.number < E->count);
	const Binding *const b = bindingat(E, id.u.number);
	assert(b);
	return b;
}

Array *envkeymap(const Array *const E, const Ref id)
{
	const Binding *const B = idtobind(E, id);
	assert(iskeymap(B->ref));

	return B->ref.u.array;
}

Ref envrootpath(const Array *const E, const Ref id)
{
	const Binding *const B = idtobind(E, id);
	assert(B->key.code == LIST);

	return markext(B->key);
}
