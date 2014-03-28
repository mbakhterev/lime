#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGAR	1
#define DBGAE	(1 << 1)

// #define DBGFLAGS (DBGAR | DBGAE)

#define DBGFLAGS 0

unsigned isarea(const Ref r)
{
	return iskeymap(r);
}

static void initreactor(Array *const U, Array *const area, const unsigned id)
{
	Array *const R = newkeymap();
	DL(fkey, RS(decoatom(U, DAREA), readtoken(U, "FORMS")));

	Binding *const fb = (Binding *)bindingat(R, bindkey(R, fkey));
	if(!fb || fb->ref.code != FREE)
	{
		freekeymap(R);
		assert(0);
		return;
	}

	fb->ref = reflist(NULL);

	DL(rkey, RS(readtoken(U, "R"), refnum(id)));
	assert(linkmap(U, area, readtoken(U, "CTX"), rkey, refkeymap(R)) == R);
}

unsigned areforms(const Ref r)
{
	return r.code == LIST && (r.u.list == NULL || isform(r.u.list->ref));
}

Ref *reactorforms(Array *const U, const Array *const reactor)
{
	DL(fkey, RS(decoatom(U, DAREA), readtoken(U, "FORMS")));

	const Binding *const b = bindingat(reactor, maplookup(reactor, fkey));
	assert(b);
	assert(areforms(b->ref));
	
	return (Ref *)&b->ref;
}

Array *areareactor(Array *const U, const Array *const area, const unsigned id)
{
	DL(rkey, RS(readtoken(U, "R"), refnum(id)));

	DBG(DBGAR, "%p", (void *)area);
	if(DBGFLAGS & DBGAR)
	{
		dumpkeymap(1, stderr, 0, U, area, NULL);
	}

	// Можно было бы создавать реакторы по запросу, но для лучшего контроля
	// за ошибками требуем, чтобы это вхождение уже было и было корректным
	
	Array *const R
		= linkmap(U, (Array *)area, 
			readtoken(U, "CTX"), rkey, reffree());
	assert(R);
	return R;
}

void unlinkareareactor(Array *const U, Array *const area, const unsigned id)
{
	DL(rkey, RS(readtoken(U, "R"), refnum(id)));
	assert(unlinkmap(U, area, readtoken(U, "CTX"), rkey));
}

static void initdag(Array *const U, Array *const area)
{
	Array *const R = areareactor(U, area, 1);
	DL(dkey, RS(decoatom(U, DAREA), readtoken(U, "DAG")));

	Binding *const b = (Binding *)bindingat(R, bindkey(R, dkey));
	assert(b && b->ref.code == FREE);
	b->ref = cleanext(refdag(NULL));
}

Ref *areadag(Array *const U, const Array *const area)
{
	Array *const R = areareactor(U, area, 1);
	DL(dkey, RS(decoatom(U, DAREA), readtoken(U, "DAG")));

	Binding *const b = (Binding *)bindingat(R, maplookup(R, dkey));
	assert(b);
	assert(isdag(b->ref));

	return &b->ref;
}

static void initlinks(Array *const U, Array *const area)
{
	Array *const R = newkeymap();
	assert(linkmap(U, area,
		readtoken(U, "CTX"), readtoken(U, "LINKS"), refkeymap(R)) == R);
}

Array *arealinks(Array *const U, const Array *const area)
{
	Array *const L
		= linkmap(U, (Array *)area,
			readtoken(U, "CTX"), readtoken(U, "LINKS"), reffree());
	assert(L);
	return L;
}

unsigned unlinkarealinks(Array *const U, Array *const area)
{
	return unlinkmap(U, area, readtoken(U, "CTX"), readtoken(U, "LINKS"));
}

static void initsyntax(Array *const U, Array *const area, const Ref syntax)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "SYNTAX")));
	Binding *const b = (Binding *)bindingat(area, bindkey(area, key));
	assert(b && b->ref.code == FREE);

	b->ref = forkref(syntax, NULL);
}

Ref areasyntax(Array *const U, const Array *const area)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "SYNTAX")));
	const Binding *const b = bindingat(area, maplookup(area, key));
	if(b && b->ref.code != FREE)
	{
		return markext(b->ref);
	}

	return reffree();
}

static void initenv(Array *const U, Array *const area, const Array *const env)
{
	const Ref r = refkeymap((Array *)env);
	const Ref path = readtoken(U, "ENV");
	assert(linkmap(U, area, path, readtoken(U, "this"), r) == env);
}

Array *areaenv(Array *const U, const Array *const area)
{
	DBG(DBGAE, "%p", (void *)area);

	const Ref path = readtoken(U, "ENV");
	Array *const env
		= linkmap(U, (Array *)area,
			path, readtoken(U, "this"), reffree());

	assert(env);
	return env;
}

void unlinkareaenv(Array *const U, Array *const area)
{
	const Ref path = readtoken(U, "ENV");
	assert(unlinkmap(U, area, path, readtoken(U, "this")));
}

Array *newarea(Array *const U, const Ref syntax, const Array *const env)
{
	Array *const area = newkeymap();
	markactive(U, area, 1);
	initlinks(U, area);
	initenv(U, area, env);
	initsyntax(U, area, syntax);

	for(unsigned i = 0; i < 2; i += 1)
	{
		initreactor(U, area, i);
	}
	initdag(U, area);

	return area;
}

void markonstack(Array *const U, Array *const area, const unsigned on)
{
	if(on)
	{
		assert(linkmap(U, area,
			readtoken(U, "CTX"), readtoken(U, "STACK"),
				refkeymap(area)) == area);
		return;
	}

	assert(unlinkmap(U, area, readtoken(U, "CTX"), readtoken(U, "STACK")));
}

unsigned isonstack(Array *const U, const Array *const area)
{
	Array *const t
		= linkmap(U, (Array *)area,
			readtoken(U, "CTX"), readtoken(U, "STACK"), reffree());
	
	if(!t)
	{
		return 0;
	}

	assert(t == area);
	return !0;
}
void markontop(Array *const U, Array *const map, const unsigned on)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "TOP")));
	Binding *const b = (Binding *)bindingat(map, bindkey(map, key));
	assert(b);

	if(on)
	{
		// Нужно убедится, что установка флага не повторная
		assert(b->ref.code == FREE
			|| (b->ref.code == NUMBER && !b->ref.u.number));

		b->ref = refnum(!0);
		return;
	}

	// Сбросить флаг активности можно только с активной сущности
	assert(b->ref.code == NUMBER && b->ref.u.number != 0);
	b->ref = refnum(0);
}

unsigned isontop(Array *const U, const Array *const map)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "TOP")));
	const Binding *const b = bindingat(map, maplookup(map, key));
	assert(!b || b->ref.code == NUMBER);

	return b != NULL && b->ref.u.number != 0;
}
