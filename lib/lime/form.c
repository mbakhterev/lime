#include "construct.h"
#include "util.h"

#include <assert.h>
#include <string.h>

static Form *freeforms = NULL;

static Form *tipoffform(Form **const fptr)
{
	Form *const f = *fptr;
	assert(f);

	Form *const g = f->u.nextfree;

	if(f != g)
	{
		f->u.nextfree = g->u.nextfree;
	}
	else
	{
		*fptr = NULL;
	}

	return g;
}

void freeform(const Ref r)
{
	assert(r.code == FORM);

	Form *const f = r.u.form;
	assert(f);

	if(!r.external)
	{
		freelist((List *)f->signature);
		freedag((List *)f->u.dag);
	}

	// Плата за наличие const полей в структуре

	memcpy(f,
		&(Form) {
			.signature = NULL,
			.count = FREE }, sizeof(Form));

	if(freeforms == NULL)
	{
		f->u.nextfree = f;
		freeforms = f;
		return;
	}

	freeforms->u.nextfree = f;
	f->u.nextfree = freeforms;
	freeforms = f;
}

static int freeone(List *const f, void *const ptr)
{
	assert(f);
	freeform(f->ref);

	return 0;
}

void freeformlist(List *const forms)
{
	forlist(forms, freeone, NULL, 0);
}


Ref newform(const List *const dag, const List *const signature)
{
	Form *f = NULL;

	if(freeforms)
	{
		f = tipoffform(&freeforms);
		assert(f->count == FREE
			&& f->signature == NULL);
	}
	else
	{
		f = malloc(sizeof(Form));
		assert(f);
	}

	// Плата за const поля в структуре

	memcpy(f,
		&(Form)
		{
			.u.dag = dag,
			.signature = signature,
			.count = 0 }, sizeof(Form));

	return refform(f);
}
