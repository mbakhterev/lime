#include "construct.h"
#include "util.h"

#include <assert.h>

unsigned isarea(const Ref r)
{
	return r.code == AREA && r.u.array && r.u.array->code == MAP;
}

static void initreactor(Array *const U, const unsigned id, Array *const area)
{
	const Ref rkey = decorate(refnum(id), U, DAREA);

	// Чистим external-бит для уверенности
	Binding *const b
		= (Binding *)bindingat(area, mapreadin(area, cleanext(rkey)));

	if(!b)
	{
		// mapreadin не должен вернуть NULL, говорящий о том, что
		// mapreadin нашёл существующую в area запись с rkey

		freeref(rkey);
		assert(0);
		return;
	}

	Array *const r = newkeymap();
	Binding *const fb
		= (Binding *)bindingat(r, mapreadin(r, readtoken(U, "FORMS")));

	if(!fb)
	{
		freekeymap(r);
		assert(0);
		return;
	}

	fb->ref = reflist(NULL);
	b->ref = cleanext(refkeymap(r));
	return;
}

static void initdag(Array *const U, Array *const area)
{
	Binding *const b
		= (Binding *)bindingat(
			area, mapreadin(area, readtoken(U, "DAG")));
	assert(b);
// 	b->ref = cleanext(reflist(NULL));
	b->ref = cleanext(refdag(NULL));
}

Array *newarea(Array *const U)
{
	Array *const area = newkeymap();

	for(unsigned i = 0; i < 2; i += 1)
	{
		initreactor(U, i, area);
	}

	initdag(U, area);

	return area;
}

Array *areareactor(Array *const U, const Array *const area, const unsigned id)
{
	DL(rkey, RS(decoatom(U, DAREA), refnum(id)));
	const Binding *const b = bindingat(area, maplookup(area, rkey));
	
	// Можно было бы создавать реакторы по запросу, но для лучшего контроля
	// за ошибками требуем, чтобы это вхождение уже было и было корректным

	assert(b);
	assert(iskeymap(b->ref));

	return b->ref.u.array;
}

unsigned areforms(const Ref r)
{
	return r.code == LIST && (r.u.list == NULL || isform(r.u.list->ref));
}

Ref *areadag(Array *const U, const Array *const area)
{
	Binding *const b
		= (Binding *)bindingat(area,
			maplookup(area, readtoken(U, "DAG")));
	assert(b);
// 	assert(b->ref.code == LIST);
	assert(isdag(b->ref));
	return &b->ref;
}

Ref *reactorforms(Array *const U, const Array *const reactor)
{
	const Binding *const b
		= bindingat(reactor, maplookup(reactor, readtoken(U, "FORMS")));
	assert(b);

	assert(areforms(b->ref));
	
	return (Ref *)&b->ref;
}

void areaonstack(Array *const U, Array *const A, const unsigned on)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "STACK")));
	Binding *const b = (Binding *)bindingat(A, bindkey(A, key));
	assert(b);
	if(b->ref.code == FREE)
	{
		b->ref = refnum(0);
	}
	assert(b->ref.code == NUMBER);

	b->ref.u.number = on != 0;
}

unsigned isareaonstack(Array *const U, const Array *const A)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "STACK")));
	const Binding *const b = bindingat(A, maplookup(A, key));
	if(b)
	{
		assert(b->ref.code == NUMBER);
		return b->ref.u.number != 0;
	}

	return 0;
}

void areadone(Array *const U, Array *const A)
{
	DL(key, RS(decoatom(U, DUTIL, readtoken(U, "DONE"))));
	Binding *const b = (Binding *)bindingat(A, bindkey(A, key));
	assert(b && b->ref.code == FREE);
	b->ref = refnum(1);
}

void isareadone(Array *const U, const Array *const A)
{
	DL(key, RS(decoatom(U, DUTIL, readtoken(U, "DONE"))));
	const Binding *const b = bindingat(A, maplookup(A, key));
	if(b)
	{
		assert(b->ref.code == NUMBER && b->ref.u.number == 1);
		return 1;
	}

	return 0;
}
