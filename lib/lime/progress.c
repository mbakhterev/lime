#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGPGRS 1
#define DBGPGRSEX 2
#define DBGAFERR 4 

#define DBGFLAGS (DBGPGRSEX | DBGAFERR) 

static Form *getform(
	Array *const U, const unsigned code, const unsigned atom,
	const List *const env)
{
// 	// Первый шаг: ищем форму по ключу (0."A"; atom). Проверять то, что
// 	// номер 0."A" совпадает с AOP явно не будем, если совпадает, поиск
// 	// должен окончится успехом. Второй шаг: форма по ключу (0."A"; тип для
// 	// класса атома) 
// 
// 	DL(key, RS(refatom(AOP), refatom(atom)));

	// Первый шаг: ищем форму по ключу (code; atom). Второй шаг: ищем по
	// ключу (code; тип для класса атом)

	DL(key, RS(refatom(code), refatom(atom)));
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

// static void proatom(
// 	Array *const U, const unsigned atom,
// 	const List *const env, const List *const ctx)
// {
// 	// Найти соответствующую форму
// 
// 	// const Form *const f =
// 	getatomform(U, atom, env);
// }

void progress(
	Array *const U,
	const List *const env, const List *const ctx,
	const SyntaxNode cmd)
{
	assert(U);

	DBG(DBGPGRSEX, "%s %s",
		atombytes(atomat(U, cmd.op)), atombytes(atomat(U, cmd.atom)));
	
// 	// Первым делом здесь надо разобраться с типом конструкции: либо это
// 	// новая информация для нового контекста (A, B, U); либо дополнительная
// 	// информация в текущий контекст (L, два L подряд быть не могут, вроде
// 	// как), либо это слияние двух контексто, которое E.
// 
// 	switch(cmd.op)
// 	{
// 	case AOP:
// 		proatom(U, cmd.atom, env, ctx);
// 		break;
// 
// 	case UOP:
// 		break;
// 	}


	// Интуиция подсказывает, что  более простая структура у кода будет,
	// если пройдём несколько стадий: поиск формы, если нужно; размещение
	// нового контекста в стеке, если нужно; засевание контекста на вершине
	// стека новой формой или слияние двух верхних контекстов, в зависимости
	// от кода операции; осуществлять операцию вывода в контексте на
	// вершине, пока выводится

	const Form *f = NULL;

	switch(cmd.op)
	{
	case AOP:
	case UOP:
	case LOP:
		f = getform(U, cmd.op, cmd.atom, env);
		break;
	
	case EOP:
		break;	

	default:
		ERR("unknown syntax op: %s",
			cmd.op < U->count ? (char *)f->u.dag->ref.u.pointer: "null");

	}
}
