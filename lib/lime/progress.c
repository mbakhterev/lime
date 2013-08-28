#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGPGRS	1

#define DBGFLAGS (DBGPGRS)

void progress(
	Array *const U,
	const List *const env, const List *const ctx,
	const SyntaxNode cmd)
{
	assert(U);

	DBG(DBGPGRS, "SN. %s %s",
		atombytes(atomat(U, cmd.op)), atombytes(atomat(U, cmd.atom)));
}
