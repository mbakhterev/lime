#include "construct.h"
#include "util.h"

#include <assert.h>

static Ref readtoken(Array *const U, const char *const str)
{
	return readpack(U, strpack(0, str));
}

static void initreactor(Array *const U, const unsigned id, Array *const area)
{
	DL(rkey, RS(readtoken(U, "R"), refnum(id)));

	// Чистим external-бит для "уверенного" копирования ключа внутрь
	Binding *const b = mapreadin(area, cleanext(rkey));

	// Вхождения с таким ключом в area быть не должно
	assert(b);

	b->ref = cleanext(refkeymap(newkeymap()));
}

static void initdag(Array *const U, Array *const area)
{
	Binding *const b = mapreadin(area, readtoken(U, "DAG"));
	assert(b);
	b->ref = cleanext(refkeymap(newkeymap()));
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
	DL(rkey, RS(readtoken(U, "R"), refnum(id)));
	const Binding *const b = maplookup(area, rkey);
	
	// Можно было бы создавать реакторы по запросу, но для лучшего контроля
	// за ошибками требуем, чтобы это вхождение уже было и было корректным

	assert(b);
	assert(iskeymap(b->ref));

	return b->ref.u.array;
}

Ref areadag(Array *const U, const Array *const area)
{
	const Binding *const b = maplookup(area, readtoken(U, "DAG"));
	assert(b);
	assert(b->ref.code == LIST);
	return b->ref;
}
