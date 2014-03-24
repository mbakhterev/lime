#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGPRGS 1
#define DBGGF 2
#define DBGSYNTH 4
#define DBGCLLT 8
#define DBGACT 16
#define DBGACTSUBST 32

// #define DBGFLAGS (DBGPRGS | DBGGF)
// #define DBGFLAGS (DBGPRGS | DBGSYNTH | DBGCLLT)

#define DBGFLAGS (DBGSYNTH | DBGACT)

// #define DBGFLAGS 0

static Ref atomtype(Array *const U, Array *const T, const unsigned atom)
{
// 	const unsigned hintclass = atomhint(atomat(U, atom)) & 0xf0;
	const unsigned hintclass = atomhint(atomat(U, atom));

	// typeenummap скопирует ключ при необходимости
	DL(key, RS(readpack(U, strpack(hintclass, ""))));
	return reftype(typeenummap(T, key));
}

static Ref getbody(
	const Core *const C, const unsigned op, const unsigned atom)
{
	DBG(DBGGF, "%s", "entry");

	Array *const U = C->U;
	Array *const T = C->types;

	// Работать будем в этом окружении, поэтому
	Array *const env = C->envtogo;

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

			return markext(b->ref);
		}

		freeref(key);
	}

	freelist(p);

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

	DL(key, RS(decoatom(st->U, DOUT), markext(l->ref)));
	const Binding *const b = bindingat(st->R, maplookup(st->R, key));

	if(DBGFLAGS & DBGCLLT)
	{
		char *const kstr = strref(st->U, NULL, key);
		DBG(DBGCLLT, "key: %s", kstr);
		free(kstr);

		dumpkeymap(1, stderr, 0, st->U, st->R, NULL);
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
	Array *const area, Core *const C, const Array *const envtogo)
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
		= newverbmap(C->U, 0, ES(
			"L", "FIn", "Nth",
			"R",
			"F", "FEnv", "FPut", "FOut"));

	const Ref rawbody
		= ntheval(
			C->U, formdag(form), escape,
			C->typemarks, C->types, C->symmarks, C->symbols,
			inlist);
	
	const Ref body = leval(C->U, rawbody, escape);
	freeref(rawbody);

	if(DBGFLAGS & DBGACTSUBST)
	{
		DBG(DBGACTSUBST, "%s", "ntheval");
		dumpdag(0, stderr, 0, C->U, body);
		assert(fputc('\n', stderr) == '\n');
	}

	Array *const envmarks = newkeymap();
	const Array *const tomark
		= newverbmap(C->U, 0, ES("FEnv", "TEnv", "S"));
	
	Array *const env = areaenv(C->U, area);

	enveval(C->U, env, envmarks, body, escape, tomark);
	freekeymap((Array *)tomark);

	typeeval(C->U, C->types, C->typemarks, body, escape, envmarks);

	Array *const areamarks = newkeymap();
	reval(C->U, area, areamarks, body, escape);

	symeval(C->U, C->symbols, C->symmarks,
		body, escape, envmarks, C->typemarks);

	formeval(C->U, area, body, escape, envmarks, NULL, C->typemarks);

	const Array *const etg
		= goeval(C->U, area, body, escape, envmarks, envtogo);

	assert(!envtogo || etg == envtogo || etg == NULL);

	freekeymap(areamarks);
	freekeymap(envmarks);

	// FIXME: ещё несколько стадий

	// Убираем использованные синтаксические команды
	gcnodes((Ref *)&body, escape, nonroots, NULL);

	freekeymap((Array *)nonroots);
	freekeymap((Array *)escape);

	// Будут удалены все части списка:
	freelist((List *)inlist);

	// Наращиваем тело графа
	Ref *const AD = areadag(C->U, area);
	assert(isdag(body) && isdag(*AD));
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
	activate(l->ref, st->reactor, st->area, st->core, NULL);

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
		dumpkeymap(1, stderr, 0, C->U, A, NULL);
	}
	
	DBG(DBGSYNTH, "%s", "RF 1");
	{
		Ref *const RF = reactorforms(C->U, st.reactor);
		List *const l = RF->u.list;
		*RF = reflist(NULL);
		forlist(l, synthone, &st, 0);
	}


	// Нам в дальнейшем интересны только неиспользованные формы, поэтому
	// (заменяем - ОШИБОЧНО) дополняем текущий список форм, которые могли
	// накопиться в процессе активации других форм при помощи FPut

	assert(areforms(reflist(st.inactive)));
	DBG(DBGSYNTH, "%s", "RF 2");
	{
		Ref *const RF = reactorforms(C->U, st.reactor);
		RF->u.list = append(RF->u.list, st.inactive);
	}
// 	*reactorforms(C->U, st.reactor) = reflist(st.inactive);

	return st.alive;
}

void progress(Core *const C, const SyntaxNode op)
{
// 	const Ref key;
// 	const Ref form = getform((Ref *)&key, C, op.op, op.atom);

// 	DL(key, RS(refatom(op.op), refatom(op.atom)));
	const Ref key = reflist(RL(refatom(op.op), refatom(op.atom)));
	const Ref body = getbody(C, op.op, op.atom);

	if(body.code == FREE)
	{		
// 		DL(key, RS(refatom(op.op), refatom(op.atom)));
		char *const strkey = strref(C->U, NULL, key);

		item = op.pos.line;
		ERR("can't find form for input key: %s", strkey);
		
		free(strkey);
	}

	assert(isdag(body));

	DBG(DBGPRGS, "found body: %p", (void *)body.u.list);

	// Форма найдена, нужно теперь её вместе с подходящим out-ом
	// зарегистрировать в области вывода

	// FIXME:
	const Ref A = tip(C->areastack)->ref;
	assert(isarea(A));

	// Создаём спсиок вида ((keys op.atom)) - список из одной пары (ключ
	// значение)

	const List *const out
		= RL(reflist(RL(forkref(key, NULL), refatom(op.atom))));

	// Забираем его в область вывода. intakeout скопирует компоненты ключа
	// при необходимости

	if(intakeout(C->U, A.u.array, 0, out))
	{
		char *const ostr = strlist(C->U, out);
		ERR("can't intake output list: %s", ostr);
		free(ostr);
		freelist((List *)out);
		return;
	}

	// WARNING: Освободит и forkref(key) среди прочего
	freelist((List *)out);

	// form у нас здесь помечена, как external. intakeform умеет сама
	// разобраться с такой ситуацией

// 	intakeform(C->U, A.u.array, 0, key, body);
	intakeform(
		C->U, areareactor(C->U, A.u.array, 0), reflist(RL(key)), body);

	// Информация принята в реактор, синтезируем на её основе продолжение
	// графа

	while(synthesize(C, A.u.array, 0))
	{
	}

	if(DBGFLAGS & DBGPRGS)
	{
		dumpkeymap(1, stderr, 0, C->U, A.u.array, NULL);
	}
}
