#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGPGRS 1
#define DBGPGRSEX 2
#define DBGAFERR 4 

#define DBGFLAGS (DBGPGRSEX | DBGAFERR) 

static Form *getform(
	Array *const U, const unsigned code, const unsigned atom,
	const List *const env,
	const List **const pkey)
{
	assert(pkey);

	// Первый шаг: ищем форму по ключу (code; atom). Второй шаг: ищем по
	// ключу (code; тип для класса атом)

	DL(key, RS(refatom(code), refatom(atom)));
	const Ref *const r = formkeytoref(U, env, key, -1);

	if(r->code == FORM)
	{
		assert(r->u.form);

		*pkey = forklist(key);
		return r->u.form;
	}

	assert(r->code == FREE);

	// FIXME: второй шаг

	char *const c = strlist(U, key);
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

	// В соответствии с txt/worklog.txt:2331 2013-09-01 22:31:36

	// Поиск новой для контекста формы в окружении. getform скажет, с каким
	// ключом она нагла форму

	const List *key = NULL;
	const Form *f = getform(U, cmd.op, cmd.atom, env, &key);
	assert(f);
	assert(key);

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
		f->u.dag, f->map, f->signature, ITEXTERNAL);

	// Забираем ((key); cmd.atom), в outs текущего реактора. Сначала
	// формируем собственно пару из ((key) atom); а затем outs, как список
	// из этой пары

	DL(pair, RS(reflist((List *)key), refatom(cmd.atom)));
	DL(outs, RS(reflist((List *)pair)));

	intakeout(U, tip(ctx)->ref.u.context, 0, outs);

	// Ключ больше не нужен
	freelist((List *)key);

	*pctx = ctx;
	*penv = env;
}
