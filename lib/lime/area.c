#include "construct.h"
#include "util.h"

#include <assert.h>

unsigned isarea(const Ref r)
{
	return iskeymap(r);
}

// static void initreactor(Array *const U, const unsigned id, Array *const area)
// {
// 	const Ref rkey = decorate(refnum(id), U, DAREA);
// 
// 	// Чистим external-бит для уверенности
// 	Binding *const b
// 		= (Binding *)bindingat(area, mapreadin(area, cleanext(rkey)));
// 
// 	if(!b)
// 	{
// 		// mapreadin не должен вернуть NULL, говорящий о том, что
// 		// mapreadin нашёл существующую в area запись с rkey
// 
// 		freeref(rkey);
// 		assert(0);
// 		return;
// 	}
// 
// 	Array *const r = newkeymap();
// 	Binding *const fb
// 		= (Binding *)bindingat(r, mapreadin(r, readtoken(U, "FORMS")));
// 
// 	if(!fb)
// 	{
// 		freekeymap(r);
// 		assert(0);
// 		return;
// 	}
// 
// 	fb->ref = reflist(NULL);
// 	b->ref = cleanext(refkeymap(r));
// 	return;
// }

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
// 	const Binding *const b
// 		= bindingat(reactor, maplookup(reactor, readtoken(U, "FORMS")));

	DL(fkey, RS(decoatom(U, DAREA), readtoken(U, "FORMS")));

	const Binding *const b = bindingat(reactor, maplookup(reactor, fkey));
	assert(b);
	assert(areforms(b->ref));
	
	return (Ref *)&b->ref;
}

// Array *areareactor(Array *const U, const Array *const area, const unsigned id)
// {
// 	DL(rkey, RS(decoatom(U, DAREA), refnum(id)));
// 	const Binding *const b = bindingat(area, maplookup(area, rkey));
// 	
// 	// Можно было бы создавать реакторы по запросу, но для лучшего контроля
// 	// за ошибками требуем, чтобы это вхождение уже было и было корректным
// 
// 	assert(b);
// 	assert(iskeymap(b->ref));
// 
// 	return b->ref.u.array;
// }

Array *areareactor(Array *const U, const Array *const area, const unsigned id)
{
	DL(rkey, RS(readtoken(U, "R"), refnum(id)));
// 	DL(key, RS(readtoken(U, "CTX"), rkey));

	// Можно было бы создавать реакторы по запросу, но для лучшего контроля
	// за ошибками требуем, чтобы это вхождение уже было и было корректным

// 	const Binding *const b = bindingat(area, maplookup(area, key));
// 	assert(b && iskeymap(b->ref));
// 	return b->ref.u.array;
	
	Array *const R
		= linkmap(U, (Array *)area, 
			readtoken(U, "CTX"), rkey, reffree());
	assert(R);
	return R;
}

// static void initdag(Array *const U, Array *const area)
// {
// 	Binding *const b
// 		= (Binding *)bindingat(
// 			area, mapreadin(area, readtoken(U, "DAG")));
// 	assert(b);
// // 	b->ref = cleanext(reflist(NULL));
// 	b->ref = cleanext(refdag(NULL));
// }

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

// 	Binding *const b
// 		= (Binding *)bindingat(area,
// 			maplookup(area, readtoken(U, "DAG")));

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

Array *newarea(Array *const U)
{
	Array *const area = newkeymap();
	markactive(U, area, 1);
	initlinks(U, area);

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
// 	return linkmap(U, (Array *)area,
// 		readtoken(U, "CTX"), readtoken(U, "STACK"), reffree()) != NULL;

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


// void areaonstack(Array *const U, Array *const A, const unsigned on)
// {
// 	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "STACK")));
// 	Binding *const b = (Binding *)bindingat(A, bindkey(A, key));
// 	assert(b);
// 	if(b->ref.code == FREE)
// 	{
// 		b->ref = refnum(0);
// 	}
// 	assert(b->ref.code == NUMBER);
// 
// 	b->ref.u.number = on != 0;
// }
// 
// unsigned isareaonstack(Array *const U, const Array *const A)
// {
// 	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "STACK")));
// 	const Binding *const b = bindingat(A, maplookup(A, key));
// 	if(b)
// 	{
// 		assert(b->ref.code == NUMBER);
// 		return b->ref.u.number != 0;
// 	}
// 
// 	return 0;
// }

// void areadone(Array *const U, Array *const A)
// {
// 	DL(key, RS(decoatom(U, DUTIL, readtoken(U, "DONE"))));
// 	Binding *const b = (Binding *)bindingat(A, bindkey(A, key));
// 	assert(b && b->ref.code == FREE);
// 	b->ref = refnum(1);
// }

// void isareadone(Array *const U, const Array *const A)
// {
// 	DL(key, RS(decoatom(U, DUTIL, readtoken(U, "DONE"))));
// 	const Binding *const b = bindingat(A, maplookup(A, key));
// 	if(b)
// 	{
// 		assert(b->ref.code == NUMBER && b->ref.u.number == 1);
// 		return 1;
// 	}
// 
// 	return 0;
// }

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
