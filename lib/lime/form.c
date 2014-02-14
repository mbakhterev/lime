#include "construct.h"
#include "util.h"

#include <assert.h>

enum { DAG = 0, KEYS, COUNT };

// Было бы круто это писать так:
//	(return (len > COUNT && (R (DAG KEYS COUNT)).code == LIST LIST NUMBER))

static unsigned isformrefs(const Ref R[], const unsigned len)
{
	return len > COUNT
		&& R[DAG].code == LIST
		&& R[KEYS].code == LIST
		&& R[COUNT].code == NUMBER;
}

// Заряжаем счётчик сразу. Иначе придётся усложнять интерфейс. А не хочется

Ref newform(const Ref dag, const Ref keys)
{
// 	return refform(RL(
// 		forkdag(dag),
// 		forkref(keys, NULL),
// 		refnum(listlen(keys.u.list))));

	// Небольшая проверка структуры
	assert(dag.code == LIST && keys.code == LIST);

	// Видимо, логичнее не делать здесь fork-ов, чтобы дать возможность
	// регулировать состав формы внешнему коду
	return refform(RL(dag, keys, refnum(listlen(keys.u.list))));
}

void freeform(const Ref f)
{
	assert(f.code == FORM);

	if(!f.external)
	{
		freelist(f.u.list);
	}
}

Ref forkform(const Ref f)
{
	if(!f.external)
	{
		return newform(forkdag(formdag(f)), forkref(formkeys(f), NULL));
	}

	return f;
}

unsigned isformlist(const List *const l)
{
	const unsigned len = listlen(l);
	const Ref R[len];
	assert(writerefs(l, (Ref *)R, len) == len);

	return isformrefs(R, len);
}

unsigned isform(const Ref r)
{
	return r.code == FORM && isformlist(r.u.list);
}

Ref formdag(const Ref r)
{
	assert(r.code == FORM);

	const unsigned len = listlen(r.u.list);
	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len && isformrefs(R, len));

	return R[DAG];
}

Ref formkeys(const Ref r)
{
	assert(r.code == FORM);

	const unsigned len = listlen(r.u.list);
	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len && isformrefs(R, len));

	return R[KEYS];
}

unsigned formcounter(const Ref r)
{
	assert(r.code == FORM);

	const unsigned len = listlen(r.u.list);
	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len && isformrefs(R, len));

	return R[COUNT].u.number;
}

unsigned countdown(const Ref *const r)
{
	assert(isform(*r));

	const Ref *R[3];

	{
		DL(list, RS(reffree(), reffree(), reffree()));
		const Ref pattern = { .code = FORM, .u.list = list.u.list };
		unsigned matches = 0;
		assert(keymatch(pattern, r, R, 3, &matches) && matches == 3);
	}

	Ref *const N = (Ref *)R[COUNT];
	assert(N->code == NUMBER && N->u.number > 0);

	return !(N->u.number -= 1);
}
