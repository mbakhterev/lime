#include "construct.h"
#include "util.h"

#include <assert.h>

static unsigned areforms(const Ref r)
{
	return r.code == LIST && (!r.u.list == NULL || isform(r.u.list->ref));
}

extern unsigned intakeform(
	Array *const U, Array *const area, const unsigned rid, const Ref form)
{
}
