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
	DBG(DBGAFERR, "search failed for form-key: %s", c);
	free(c);
	ERR("%s", "no form for key");

	return NULL;
}

static const char *atomizeop(const Array *const U, const unsigned op)
{
	assert(U && U->code == ATOM);

	if(op < U->count)
	{
		return (char *)atombytes(atomat(U, op));
	}

	return "Not-An-Atom";
}

void progress(
	Array *const U,
	const List **const penv, const List **const pctx,
	const SyntaxNode cmd)
{
	assert(pctx);
	assert(penv);
	assert(U);

	const List *ctx = *pctx;
	const List *env = *penv;

	DBG(DBGPGRSEX, "%s %s",
		atombytes(atomat(U, cmd.op)), atombytes(atomat(U, cmd.atom)));

// 	// Интуиция подсказывает, что  более простая структура у кода будет,
// 	// если пройдём несколько стадий: поиск формы, если нужно; размещение
// 	// нового контекста в стеке, если нужно; засевание контекста на вершине
// 	// стека новой формой или слияние двух верхних контекстов, в зависимости
// 	// от кода операции; осуществлять операцию вывода в контексте на
// 	// вершине, пока выводится
// 
// 	const Form *f = NULL;
// 
// 	switch(cmd.op)
// 	{
// 	case AOP:
// 	case UOP:
// 	case LOP:
// 		f = getform(U, cmd.op, cmd.atom, env);
// 		break;
// 	
// 	case EOP:
// 		break;	
// 
// 	default:
// 		ERR("unknown syntax op: %s",
// 			cmd.op < U->count ?
// 				(char *)f->u.dag->ref.u.pointer
// 				: "null");
// 
// 	}

	// В соответствии с txt/worklog.txt:2331 2013-09-01 22:31:36

	// Поиск новой для контекста формы в окружении
	const Form *f = getform(U, cmd.op, cmd.atom, env);
	assert(f);

	// При необходимости добавляем новый контекст на вершину стека
	switch(cmd.op)
	{
	case AOP:
	case UOP:
	case BOP:
		ctx = pushcontext((List *)ctx, EMPTY, cmd.atom);
		break;
	
	case LOP:
	case EOP:
		break;

	default:
		ERR("unknown cmd op: %u; atom: %s",
			cmd.op, atomizeop(U, cmd.op));
	}

	// Размещаем в текущем контексте форму. Для проверки корректности
	// происходящего убеждаемся, что forward-реактор на вершине стека пустой

	assert(isforwardempty(ctx));

	// Забираем форму в текущий реактор (то есть, ctx->ref.u.context->R[0]).
	// Забираем, как external

	intakeform(
		U, tip(ctx)->ref.u.context, 0,
		f->u.dag, f->map, f->signature, 1);

	*pctx = ctx;
	*penv = env;
}
