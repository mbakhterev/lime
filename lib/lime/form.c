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


// void freeform(Form *const f)

void freeform(const Ref r)
{
	assert(r.code == FORM);

	Form *const f = r.u.form;
	assert(f);

	if(!r.external)
	{
		freelist((List *)f->signature);
		freedag((List *)f->u.dag, f->map);
	}

	// Плата за наличие const полей в структуре

	memcpy(f,
		&(Form) {
			.signature = NULL,
			.map = NULL,
			.goal = 0,
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
// 	assert(f && f->ref.code == FORM);
// 	freeform(f->ref.u.form);

	assert(f);
	freeform(f->ref);

	return 0;
}

void freeformlist(List *const forms)
{
	forlist(forms, freeone, NULL, 0);
}

// Form *newform(

Ref newform(
	const List *const dag, const Array *const map,
	const List *const signature)
{
	Form *f = NULL;

	if(freeforms)
	{
		f = tipoffform(&freeforms);
		assert(f->count == FREE
			&& f->goal == 0
			&& f->signature == NULL
			&& f->map == NULL);
	}
	else
	{
		f = malloc(sizeof(Form));
		assert(f);
	}

	// Плата за const поля в структуре

	memcpy(f,
		&(Form) {
		.u.dag = forkdag(dag, map),
		.signature = forklist(signature),
		.map = map,
		.count = 0,
		.goal = 0 }, sizeof(Form));

	return refform(f);
}
