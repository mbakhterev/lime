#include "construct.h"
#include "util.h"

List *pushcontext(List *const ctx)
{
	return append(RL(refctx(NULL)), ctx);
}
