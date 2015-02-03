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

static unsigned hit(const Ref src, const Ref key);

void doeq(Marks *const M, const Ref N, const Core *const C)
{
	Array *const marks = M->marks;
	const Array *const U = C->U;
	const Array *const V = C->verbs.system;

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

	// Сперва вычисляем список значений (src) и ключ (key), который будем
	// искать среди значений src

	const Ref src = len > 0 ? simplerewrite(R[0], marks, V) : reffree();
	const Ref key = len > 1 ? simplerewrite(R[1], marks, V) : reffree();

	if(len != 4 || !issignaturekey(key)
		|| src.code != LIST || !issignaturekey(src))
	{
		freeref(key);
		freeref(src);

		item = nodeline(N);
		ERR("node \"%s\": expecting signature list and key",
			nodename(U, N));

		return;
	}

	// Выбираем один из оставшихся атрибутов в зависимости от того, нашёлся
	// ли ключ среди списка

	const unsigned id = 3 - hit(src, key);

	// Сами значения больше не нужны
	freeref(key);
	freeref(src);

	// Оцениваем соответствующий результат и привязываем его к N
	tunerefmap(marks, N, simplerewrite(R[id], marks, V));
}


static int checkone(List *const, void *const);

unsigned hit(const Ref src, const Ref key)
{
	assert(src.code == LIST);
	return forlist(src.u.list, checkone, (void *)&key, 0);
}

int checkone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);

	const Ref *const r = ptr;

	return keymatch(l->ref, r, NULL, 0, NULL);
}
