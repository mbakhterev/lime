#include "construct.h"
#include "util.h"

#include <assert.h>

// #include <string.h>
// 
// static Form *freeforms = NULL;
// 
// static Form *tipoffform(Form **const fptr)
// {
// 	Form *const f = *fptr;
// 	assert(f);
// 
// 	Form *const g = f->u.nextfree;
// 
// 	if(f != g)
// 	{
// 		f->u.nextfree = g->u.nextfree;
// 	}
// 	else
// 	{
// 		*fptr = NULL;
// 	}
// 
// 	return g;
// }
// 
// void freeform(const Ref r)
// {
// 	assert(r.code == FORM);
// 
// 	Form *const f = r.u.form;
// 	assert(f);
// 
// 	if(!r.external)
// 	{
// 		freelist((List *)f->signature);
// 		freedag((List *)f->u.dag);
// 	}
// 
// 	// Плата за наличие const полей в структуре
// 
// 	memcpy(f,
// 		&(Form) {
// 			.signature = NULL,
// 			.count = FREE }, sizeof(Form));
// 
// 	if(freeforms == NULL)
// 	{
// 		f->u.nextfree = f;
// 		freeforms = f;
// 		return;
// 	}
// 
// 	freeforms->u.nextfree = f;
// 	f->u.nextfree = freeforms;
// 	freeforms = f;
// }
// 
// static int freeone(List *const f, void *const ptr)
// {
// 	assert(f);
// 	freeform(f->ref);
// 
// 	return 0;
// }
// 
// void freeformlist(List *const forms)
// {
// 	forlist(forms, freeone, NULL, 0);
// }
// 
// 
// Ref newform(const List *const dag, const List *const signature)
// {
// 	Form *f = NULL;
// 
// 	if(freeforms)
// 	{
// 		f = tipoffform(&freeforms);
// 		assert(f->count == FREE
// 			&& f->signature == NULL);
// 	}
// 	else
// 	{
// 		f = malloc(sizeof(Form));
// 		assert(f);
// 	}
// 
// 	// Плата за const поля в структуре
// 
// 	memcpy(f,
// 		&(Form)
// 		{
// 			.u.dag = dag,
// 			.signature = signature,
// 			.count = 0 }, sizeof(Form));
// 
// 	return refform(f);
// }

enum { DAG = 0, KEYS, COUNT };

Ref newform(const Ref keys, const Ref dag)
{
	return refform(RL(forkdag(dag), forkref(keys, NULL)));
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

static unsigned keydag(const Ref R[], const unsigned len)
{
	return len >= 2 && R[DAG].code == LIST && R[KEYS].code == LIST;
}

unsigned isformlist(const List *const l)
{
	const unsigned len = listlen(l);
	const Ref R[len];
	assert(writerefs(l, (Ref *)R, len) == len);

	const unsigned form = keydag(R, len);

	switch(len)
	{
	case 2:
		return form;

	case 3:
		return form && R[COUNT].code == NUMBER;
	}

	return 0;
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
	assert(writerefs(r.u.list, (Ref *)R, len) == len && keydag(R, len));

	return R[DAG];
}

Ref formkeys(const Ref r)
{
	assert(r.code == FORM);

	const unsigned len = listlen(r.u.list);
	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len && keydag(R, len));

	return R[KEYS];
}

Ref setcounter(const Ref r)
{
	assert(r.code == FORM);

	const unsigned len = listlen(r.u.list);
	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len && keydag(R, len));
	assert(len == 2);

	return reflist(append(r.u.list, RL(refnum(listlen(R[KEYS].u.list)))));	
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

	Ref *const N = (Ref *)R[NUMBER];
	assert(N->code == NUMBER && N->u.number > 0);

	return !(N->u.number -= 1);
}
