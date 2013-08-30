#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGPGRS 1
#define DBGPGRSEX 2
#define DBGAFERR 4 

#define DBGFLAGS (DBGPGRSEX | DBGAFERR) 

static Form *getatomform(
	Array *const U, const unsigned atom, const List *const env)
{
	// Первый шаг: ищем форму по ключу (0."A"; atom). Проверять то, что
	// номер 0."A" совпадает с AOP явно не будем, если совпадает, поиск
	// должен окончится успехом. Второй шаг: форма по ключу (0."A"; тип для
	// класса атома) 

	DL(key, RS(refatom(AOP), refatom(atom)));
	const Ref *const r = formkeytoref(U, env, key, -1);

	if(r->code == FORM)
	{
		assert(r->u.form);
		return r->u.form;
	}

	assert(r->code == FREE);

	// FIXME: второй шаг

	char *const c = listtostr(U, key);
	DBG(DBGAFERR, "search failed for key: %s", c);
	free(c);
	ERR("%s", "no form for key");

	return NULL;
}

static void proatom(
	Array *const U, const unsigned atom,
	const List *const env, const List *const ctx)
{
	// Найти соответствующую форму

	// const Form *const f =
	getatomform(U, atom, env);
}

void progress(
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
		proatom(U, cmd.atom, env, ctx);
		break;

	case UOP:
		break;
	}
}
