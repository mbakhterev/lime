#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGPGRS 1
#define DBGPGRSEX 2	

#define DBGFLAGS (DBGPGRSEX) 

unsigned progress(
	Array *const U,
	const List *const env, const List *const ctx,
	const SyntaxNode cmd)
{
	assert(U);

	DBG(DBGPGRSEX, "%s %s",
		atombytes(atomat(U, cmd.op)), atombytes(atomat(U, cmd.atom)));
	
	// Первым делом здесь надо разобраться с типом конструкции: либо это
	// новая информация для нового контекста (A, B, U); либо дополнительная
	// информация в текущий контекст (L, два L подряд быть не могут, вроде
	// как), либо это слияние двух контексто, которое E.

	switch(cmd.op)
	{
	case AOP:
	case UOP:
		break;
	}

	return 0;
}
