#include "construct.h"
#include "util.h"

#include <assert.h>

// static Ref readtoken(Array *const U, const char *const str)
// {
// 	return readpack(U, strpack(0, str));
// }

// FIXME: не нравится мне код для initreactor

unsigned isarea(const Ref r)
{
	return r.code == AREA && r.u.array && r.u.array->code == MAP;
}

static void initreactor(Array *const U, const unsigned id, Array *const area)
{
	const Ref rkey = decorate(refnum(id), U, DREACTOR);

	// Чистим external-бит для уверенности
	Binding *const b = mapreadin(area, cleanext(rkey));

	if(!b)
	{
		// mapreadin не должен вернуть NULL, говорящий о том, что
		// mapreadin нашёл существующую в area запись с rkey

		freeref(rkey);
		assert(0);
		return;
	}

	Array *const r = newkeymap();
	Binding *const fb = mapreadin(r, readtoken(U, "FORMS"));

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
	Binding *const b = mapreadin(area, readtoken(U, "DAG"));
	assert(b);
	b->ref = cleanext(reflist(NULL));
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
	DL(rkey, RS(decoatom(U, DREACTOR), refnum(id)));
	const Binding *const b = maplookup(area, rkey);
	
	// Можно было бы создавать реакторы по запросу, но для лучшего контроля
	// за ошибками требуем, чтобы это вхождение уже было и было корректным

	assert(b);
	assert(iskeymap(b->ref));

	return b->ref.u.array;
}

Ref *areadag(Array *const U, const Array *const area)
{
// 	const
	Binding *const b = (Binding *)maplookup(area, readtoken(U, "DAG"));
	assert(b);
	assert(b->ref.code == LIST);
// 	return b->ref;
	return &b->ref;
}

Ref *reactorforms(Array *const U, const Array *const area, const unsigned id)
{
	const Binding *const b
		= maplookup(areareactor(U, area, id), readtoken(U, "FORMS"));
	
	assert(b);
	assert(b->ref.code == LIST
		&& (b->ref.u.list == NULL || isform(b->ref.u.list->ref)));
	
	return (Ref *)&b->ref;
}
