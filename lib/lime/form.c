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

// static Ref setcounter(const Ref r)
// {
// 	assert(r.code == FORM);
// 
// 	const unsigned len = listlen(r.u.list);
// 	const Ref R[len];
// 	assert(writerefs(r.u.list, (Ref *)R, len) == len && keydag(R, len));
// 	assert(len == 2);
// 
// 	return refform(append(r.u.list, RL(refnum(listlen(R[KEYS].u.list)))));	
// }

// Заряжаем счётчик сразу. Иначе придётся усложнять интерфейс. А не хочется

Ref newform(const Ref keys, const Ref dag)
{
	return refform(RL(
		forkdag(dag),
		forkref(keys, NULL),
		refnum(listlen(keys.u.list))));
}

void freeform(const Ref f)
{
	if(!f.external)
	{
		freelist(f.u.list);
	}
}

Ref forkform(const Ref f)
{
	if(!f.external)
	{
		return newform(formkeys(f), formdag(f));
	}

	return f;
}

unsigned isformlist(const List *const l)
{
	const unsigned len = listlen(l);
	const Ref R[len];
	assert(writerefs(l, (Ref *)R, len) == len);

// 	const unsigned form = keydag(R, len);
// 
// 	switch(len)
// 	{
// 	case 2:
// 		return form;
// 
// 	case 3:
// 		return form && R[COUNT].code == NUMBER;
// 	}
// 
// 	return 0;

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
		DL(pattern, RS(reffree(), reffree(), reffree()));
		unsigned matches = 0;
		assert(keymatch(pattern, r, R, 3, &matches) && matches == 3);
	}

	Ref *const N = (Ref *)R[COUNT];
	assert(N->code == NUMBER && N->u.number > 0);

	return !(N->u.number -= 1);
}
