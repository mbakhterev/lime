#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGPRGS 1
#define DBGGF 2
#define DBGSYNTH 4
#define DBGCLLT 8
#define DBGACT 16

// #define DBGFLAGS (DBGPRGS | DBGGF)
// #define DBGFLAGS (DBGPRGS | DBGSYNTH | DBGCLLT)

#define DBGFLAGS (DBGSYNTH)

// #define DBGFLAGS 0

static Ref atomtype(Array *const U, Array *const T, const unsigned atom)
{
	const unsigned hintclass = atomhint(atomat(U, atom)) & 0xf0;

	// typeenummap скопирует ключ при необходимости
	DL(key, RS(readpack(U, strpack(hintclass, ""))));
	return reftype(typeenummap(T, key));
}

static Ref getform(
	Ref *const K,
	const Core *const C, const unsigned op, const unsigned atom)
{
	DBG(DBGGF, "%s", "entry");

	Array *const U = C->U;
	Array *const T = C->types;
	Array *const env = C->env;

	// Путь к корню дерева окружений
	List *const p
		= tracepath(env, U,
			readtoken(U, "ENV"), readtoken(U, "parent"));
	
	// Последний элемент в списке должен быть root-ом
	assert(p && iskeymap(p->ref) && p->ref.u.array == C->root);

	DBG(DBGGF, "%s", "1st");

	// Первый шаг поиска: ищем форму напрямую, без типов

	{
		const Ref key = reflist(RL(refatom(op), refatom(atom)));
		DL(lk, RS(decoatom(U, DFORM), key));

		const Binding *const b = pathlookup(p, lk, NULL);
		if(b)
		{
			freelist(p);

			*K = key;
			// Подчёркиваем, что форма из окружения
			return markext(b->ref);
		}

		freeref(key);
	}

	DBG(DBGGF, "%s", "2nd");

	// Второй шаг поиска: ищем форму по ключу с типом

	{
		const Ref key = reflist(RL(refatom(op), atomtype(U, T, atom)));
		DL(lk, RS(decoatom(U, DFORM), key));

		if(DBGFLAGS & DBGGF)
		{
			char *const kstr = strref(U, NULL, key);
			DBG(DBGGF, "pathlooking for: %s", kstr);
			free(kstr);
		}
		
		const Binding *const b = pathlookup(p, lk, NULL);
		DBG(DBGGF, "%p", (void *)b);

		if(b)
		{
			freelist(p);

			*K = key;
			return markext(b->ref);
		}

		freeref(key);
	}

	freelist(p);

	*K = reffree();
	return reffree();
}

typedef struct
{
	List *inkeys;
	List *invals;
	Array *const U;
	const Array *const R;
} AState;

static int collectone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	AState *const st = ptr;

	DL(key, RS(decoatom(st->U, DOUT), dynamark(l->ref)));
	const Binding *const b = maplookup(st->R, key);

	if(DBGFLAGS & DBGCLLT)
	{
		char *const kstr = strref(st->U, NULL, key);
		DBG(DBGCLLT, "key: %s", kstr);
		free(kstr);

		dumpkeymap(1, stderr, 0, st->U, st->R);
	}

	DBG(DBGCLLT, "b: %p", (void *)b);

	assert(b);

	// Как ключи, так и значения могут быть подставлены в тело формы и могут
	// пережить реактор, в котором они созданы. Поэтому и ключ, и значение
	// имеет смысл скопировать. И взять их лучше именно из реактора

	const Ref *R[1];

	{
		DL(pattern, RS(decoatom(st->U, DOUT), reffree()));
		unsigned nok = 0;
		assert(keymatch(pattern, &b->key, R, 2, &nok) && nok == 1);
	}

	st->inkeys = append(st->inkeys, RL(forkref(*R[0], NULL)));
	st->invals = append(st->invals, RL(forkref(b->ref, NULL)));

	return 0;
}

static void activate(
	const Ref form, const Array *const reactor,
	Array *const area, Core *const C)
{
	// Сначала надо составить список входов для формы

	AState st =
	{
		.inkeys = NULL,
		.invals = NULL,
		.U = C->U,
		.R = reactor
	};

	const Ref keys = formkeys(form);
	assert(keys.code == LIST);
	forlist(keys.u.list, collectone, &st, 0);

	const List *const inlist = RL(reflist(st.inkeys), reflist(st.invals));

	// Теперь нужно выполнить процедуры оценки

	const Array *const escape = newverbmap(C->U, 0, ES("F"));

	const Array *const nonroots
		= newverbmap(C->U, 0, ES("FIn", "Nth", "FOut"));

	const Ref body
		= ntheval(
			C->U, formdag(form), escape,
			C->symmarks, C->typemarks, C->types,
			inlist);

	if(DBGFLAGS & DBGACT)
	{
		DBG(DBGACT, "%s", "ntheval");
		dumpdag(0, stderr, 0, C->U, body, NULL);
	}

	Array *const envmarks = newkeymap();
	const Array *const tomark = newverbmap(C->U, 0, ES("FEnv", "TEnv"));

	enveval(C->U, C->env, envmarks, body, escape, tomark);
	freekeymap((Array *)tomark);

	typeeval(C->U, C->types, C->typemarks, body, escape, envmarks);
	formeval(C->U, area, body, escape, envmarks, C->typemarks);

	freekeymap(envmarks);

	// FIXME: ещё несколько стадий

	// Убираем использованные синтаксические команды

	gcnodes((Ref *)&body, escape, nonroots, NULL);

	freekeymap((Array *)nonroots);
	freekeymap((Array *)escape);

	// Будут удалены все части списка:
	freelist((List *)inlist);

	// Наращиваем тело графа:

	Ref *const AD = areadag(C->U, area);
	assert(body.code == LIST && AD->code == LIST);
	AD->u.list = append(AD->u.list, body.u.list);
}

typedef struct
{
	List *inactive;
	Core *const core;
	Array *const area;
	Array *const reactor;
	unsigned alive;
} SState;

static int synthone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	SState *const st = ptr;

	// Превращаем звено списка в список из одного элемента
	l->next = l;

	if(formcounter(l->ref))
	{
		// Эту форму рано ещё активировать, перекладываем её в
		// inactive-список

		st->inactive = append(st->inactive, l);
		return 0;
	}

	st->alive = 1;
	activate(l->ref, st->reactor, st->area, st->core);

	// В activate форма будет аккуратно разобрана на запчасти, поэтому
	// звено списка вместе с ней можно просто удалить

	freelist(l);

	return 0;
}

static unsigned synthesize(Core *const C, Array *const A, const unsigned rid)
{
	SState st =
	{
		.inactive = NULL,
		.core = C,
		.area = A,
		.reactor = areareactor(C->U, A, rid),
		.alive = 0
	};

	if(DBGFLAGS & DBGSYNTH)
	{
		dumpkeymap(0, stderr, 0, C->U, A);
	}
	
	DBG(DBGSYNTH, "%s", "RF 1");
	{
		Ref *const RF = reactorforms(C->U, st.reactor);
		List *const l = RF->u.list;
		*RF = reflist(NULL);
		forlist(l, synthone, &st, 0);
	}


	// Нам в дальнейшем интересны только неиспользованные формы, поэтому
	// заменяем текущий список форм

	assert(areforms(reflist(st.inactive)));
	DBG(DBGSYNTH, "%s", "RF 2");
	*reactorforms(C->U, st.reactor) = reflist(st.inactive);

	return st.alive;
}

void progress(Core *const C, const SyntaxNode op)
{
	const Ref key;
	const Ref form = getform((Ref *)&key, C, op.op, op.atom);

	if(form.code == FREE)
	{		
		DL(key, RS(refatom(op.op), refatom(op.atom)));
		char *const strkey = strref(C->U, NULL, key);

		item = op.pos.line;
		ERR("can't find form for input key: %s", strkey);
		
		free(strkey);
	}

	DBG(DBGPRGS, "found form: %p", (void *)form.u.list);

	// Форма найдена, нужно теперь её вместе с подходящим out-ом
	// зарегистрировать в области вывода

	// FIXME:
	const Ref A = tip(C->areastack)->ref;
	assert(isarea(A));

	// Создаём спсиок вида ((keys op.atom)) - список из одной пары (ключ
	// значение)

	const List *const out = RL(reflist(RL(key, refatom(op.atom))));

	// Забираем его в область вывода. intakeout скопирует компоненты ключа
	// при необходимости

	if(intakeout(C->U, A.u.array, 0, out))
	{
		char *const ostr = strlist(C->U, out);
		ERR("can't intake output list: %s", ostr);
		free(ostr);
	}

	// WARNING: Освободит и key среди прочего
	freelist((List *)out);

	// form у нас здесь помечена, как external. intakeform умеет сама
	// разобраться с такой ситуацией

	intakeform(C->U, A.u.array, 0, form);

	// Информация принята в реактор, синтезируем на её основе продолжение
	// графа

	while(synthesize(C, A.u.array, 0))
	{
	}

	if(DBGFLAGS & DBGPRGS)
	{
		dumpkeymap(1, stderr, 0, C->U, A.u.array);
	}
}
