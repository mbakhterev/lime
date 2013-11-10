#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGEVE	1
#define DBGAL	2
#define DBGAE	4

// #define DBGFLAGS (DBGEVE | DBGAL)
// #define DBGFLAGS (DBGEVE)

#define DBGFLAGS (DBGAE)

typedef struct
{
	Array *const U;
	Array *const escape;
	Array *const markit;

	Array *const env;
	Array *const envmarks;

	Array *const defs;
	Array *const recode;
	Array *const envdefs;
} EState;

#define ENODE 0
#define ENV 1

static const char *const enverbs[] =
{
	[ENODE] = "E",
	[ENV] = "Env",
	NULL
};

static const List *attrlist(const Ref r, Array *const U)
{
	if(DBGFLAGS & DBGAL)
	{
		char *const c = strlist(U, r.u.list);
		DBG(DBGAL, "%s", c);
		free(c);
	}

	const Ref l = nodeattribute(r);

	if(l.code == LIST)
	{
		return l.u.list;
	}

	ERR("line: %u. node \"%s\": attribute should be a list",
		nodeline(r), atombytes(atomat(U, nodeverb(r, NULL))));

	return NULL;
}

static void mkenv(
	Array *const U, const Ref r, Array *const recode,
	Array *const env, Array *const envdefs)
{
	const List *const l = attrlist(r, U);
	const unsigned len = listlen(l);
	Ref R[len];
	assert(writerefs(l, R, len) == len);

	if(DBGFLAGS & DBGEVE)
	{
		char *const c = strlist(U, l);
		DBG(DBGEVE, "(len list) = (%u %s)", len, c);
		free(c);
	}

	// Проверим, что первые два аргумента нужного формата

	if(len < 2 || len > 3
		|| !isbasickey(R[0]) || R[1].code != LIST || !isbasickey(R[1]))
	{
		ERR("line: %u. node: \"%s\": wrong attribute format",
			nodeline(r),
			atombytes(atomat(U, nodeverb(r, NULL))));

		return;
	}

	Array *newenv = NULL;

	switch(len)
	{
	case 2:
	{
		Array *const t = newkeymap();
		newenv = makepath(env, U, R[0], R[1].u.list, refkeymap(t));
		assert(t == newenv);

		break;
	}

	case 3:
	{
		// Третьим атрибутом должна быть ссылка на уже оценённый
		// .E-узел. Проверяем

		if(!isnode(R[2])
			|| nodeverb(r, recode) != ENODE
			|| !(newenv = ptrmap(envdefs, R[2])))
		{
			ERR("line: %u. node: \"%s\": 3rd ref isn't known .E",
				nodeline(r),
				atombytes(atomat(U, nodeverb(r, NULL))));
		}

		const Array *const t
			= makepath(env, U, R[0], R[1].u.list,
				markext(refkeymap(newenv)));
		
		assert(t == newenv);

		break;
	}

	default:
		assert(0);
	}

	// Тут надо запомнить, чему равно текущее выражение с .E

	tuneptrmap(envdefs, r, newenv);
}

static void assignenv(
	Array *const U,
	const Ref r, Array *const recode,
	Array *const defs, Array *const envdefs)
{
	const List *const l = attrlist(r, U);
	const unsigned len = listlen(l);
	Ref R[len];
	assert(writerefs(l, R, len) == len);

	DBG(DBGAE, "(len ((R 0).verb local) env) = (%u (\"%s\" %u) %p)",
		len,
		atombytes(atomat(U, nodeverb(R[0], NULL))), nodeverb(R[0], recode),
		ptrmap(envdefs, R[0]));
	
	DBG(DBGAE, "((R 1).verb def (R 1).isnode) = (\"%s\" %u %u)",
		atombytes(atomat(U, nodeverb(R[1], NULL))),
		setmap(defs, R[1]),
		isnode(R[1]));

	// Проверим структуру аргументов и вычленим в случае успеха требуемое
	// окружение

	Array *env = NULL;

	if(len != 2
		|| nodeverb(R[0], recode) != ENODE || !(env = ptrmap(envdefs, R[0]))
		|| nodeverb(R[1], recode) != -1 || !setmap(defs, R[1]))
	{
		ERR("line: %u. node: \"%s\": wrong attribute structure",
			nodeline(r),
			atombytes(atomat(U, nodeverb(r, NULL))));

		return;
	}

	// Если проверки пройдены, то регистрируем (напоминание: tuneptrmap
	// сведётся к refmap, которая для своих аргументов делает markext)

	tuneptrmap(envdefs, R[1], env);
}

static unsigned knownverb(const Ref n, Array *const map)
{
	return map != NULL && nodeverb(n, map) != -1;
}

static int makeassignone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);

	const Ref r = l->ref;
	EState *const st = ptr;

	if(r.code != NODE || r.external)
	{
		// Если это не определение узла, то нам не особо интересно.
		// Переходим дальше

		return 0;
	}

	// Узел определён. Отмечаем это
	tunesetmap(st->defs, r);

	switch(nodeverb(r, st->recode))
	{
	case ENODE:
		mkenv(st->U, r, st->recode, st->env, st->envdefs);
		break;

	case ENV:
		assignenv(st->U, r, st->recode, st->defs, st->envdefs);
		break;

	default:
		// Здесь мы имеем дело с неким другим узлом. Для некоторых из
		// них мы запоминаем окружение

		if(knownverb(r, st->markit))
		{
			tuneptrmap(st->envmarks, r, st->env);
		}

		break;
	}

	return 0;
}

static int diveone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	EState *const st = ptr;
	const Ref r = l->ref;

	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		// Не интересные варианты
		break;
	
	case LIST:
		// Погружаемся этой же процедурой глубже
		forlist(r.u.list, diveone, st, 0);
		break;
	
	case NODE:
		// Здесь нам интересны только определения узлов, в которые нам
		// можно входить, и которые отличаются от .E и .Env, которые уже
		// обработаны

		if(!r.external
			&& !knownverb(r, st->escape)
			&& nodeverb(r, st->recode) == -1)
		{
			// Сначала надо выяснить, отнесён ли узел при помощи
			// .Env к какому-нибудь окружению, отличному от
			// текущего. Ну и вызвать enveval рекурсивно

			Array *const t = ptrmap(st->envdefs, r);

			enveval(st->U,
				t ? t : st->env, st->envmarks,
				nodeattribute(r), st->escape, st->markit);
		}

		break;
	
	default:
		assert(0);
	}


	return 0;
}

static void envevalcore(
	Array *const U,
	Array *const env,
	Array *const envmarks,
	List *const l, Array *const escape, Array *const markit)
{
	assert(U);
	assert(env && env->code == MAP);
	assert(envmarks && env->code == MAP);

	EState st =
	{
		.U = U,
		.env = env,
		.envmarks = envmarks,
		.escape = escape,
		.markit = markit,
		.defs = newkeymap(),
		.recode = newverbmap(U, 0, enverbs),
		.envdefs = newkeymap()
	};

	// Первая стадия оценки окружений. Проходим по текущим атрибутам,
	// формируем окружения и размечаем интересные узлы

	forlist(l, makeassignone, &st, 0);

	// Вторая стадия оценки окружений. Проходим по текущим атрибутам и
	// рекурсивно вызываем enveval с учётом накопленной об окружениях
	// информации

	forlist(l, diveone, &st, 0);

	freekeymap(st.envdefs);
	freekeymap(st.recode);
	freekeymap(st.defs);
}

void enveval(
	Array *const U,
	Array *const env,
	Array *const envmarks,
	const Ref r, Array *const escape, Array *const markit)
{
	switch(r.code)
	{
	// Выражение может быть скучным и не давать никакой информации (в случае
	// с TYPE интереса больше, но раз там уже номер типа, то всё уже
	// рассчитано)

	case NUMBER:
	case ATOM:
	case TYPE:
		break;

	// Интересные варианты развития событий

	case LIST:
		// FIXME: возможно, имеет смысл написать assert(!r.external)

		envevalcore(U, env, envmarks, r.u.list, escape, markit);	
		return;

	case NODE:
	{
		DL(nl, RS(r));
		envevalcore(U, env, envmarks, nl.u.list, escape, markit);	
		return;
	}
	
	default:
		assert(0);
	}
}
