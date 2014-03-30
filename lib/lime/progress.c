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
// #define DBGFLAGS (DBGSYNTH | DBGACT | DBGPRGS)

#define DBGFLAGS 0

static Ref atomtype(Array *const U, Array *const T, const unsigned atom)
{
	const unsigned hintclass = atomhint(atomat(U, atom));

	DL(key, RS(readpack(U, strpack(hintclass, ""))));
	return reftype(typeenummap(T, key));
}

static Array *toparea(List *const stk)
{
	const Ref A = tip(stk)->ref;
	assert(isarea(A));
	return A.u.array;
}

static Ref getbody(
	const Core *const C, const unsigned op, const unsigned atom)
{
	DBG(DBGGF, "%s", "entry");

	Array *const U = C->U;
	Array *const T = C->types;

	// Берём окружение для текущей области вывода
	Array *const env = areaenv(U, toparea(C->areastack));

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
		= newverbmap(C->U, 0, ES(
			"L", "FIn", "Nth",
			"E",
			"R", "Go", "Done",
			"F", "FEnv", "FPut", "FOut"));

	if(C->dumpinfopath)
	{
		fprintf(stderr, "\nnext active form. Original body\n");
		dumpdag(0, stderr, 0, C->U, formdag(form));
		assert(fputc('\n', stderr) == '\n');
	}

	const Ref rawbody
		= ntheval(
			C->U, formdag(form), escape,
			C->typemarks, C->types, C->symmarks, C->symbols,
			inlist);
	
	const Ref body = leval(C->U, rawbody, escape);
	freeref(rawbody);

// 	if(DBGFLAGS & DBGACTSUBST)
// 	{
// 		DBG(DBGACTSUBST, "%s", "ntheval");
// 		dumpdag(0, stderr, 0, C->U, body);
// 		assert(fputc('\n', stderr) == '\n');
// 	}

	if(C->dumpinfopath)
	{
		fprintf(stderr, "\nexpanded\n");
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

	formeval(C->U, area, C->activity,
		body, escape, envmarks, areamarks, C->typemarks);

	const Array *const etg
		= goeval(C->U, area, body, escape, envmarks, C->envtogo);
	
	// Убеждаемся, что: !envtogo. Если так, то etg может быть произвольным.
	// Иначе только NULL (случай ошибки) или совпадать с envtogo (Go не
	// встретили)

	assert(!C->envtogo || !etg || etg == C->envtogo);

	if(!C->envtogo)
	{
		C->envtogo = etg;
	}

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

	if(formcounter(l->ref))
	{
		// Эту форму рано ещё активировать, перекладываем её в
		// inactive-список. В исходном списке надо пометить эту форму
		// external-битом, чтобы она не была удалена при освобождении
		// исходного списка

		st->inactive = append(st->inactive, RL(l->ref));
		l->ref = markext(l->ref);
	
		return 0;
	}

	st->alive = 1;
	activate(l->ref, st->reactor, st->area, st->core);

// 	// В activate форма была аккуратно разобрана на запчасти, поэтому
// 	// звено списка вместе с ней можно просто удалить
// 	freelist(l);

	// Область может утратить активность, в этом случае цикл обработки нужно
	// прекращать
	return !isactive(st->core->U, st->area);
}

static unsigned synthesize(Core *const C, Array *const A, const unsigned rid)
{
	if(C->dumpinfopath)
	{
// 		const Array *const escape = stdareaupstreams(C->U);
		fprintf(stderr, "\nnext synthesis cycle at M:%p\n", (void *)A);
// 		dumpkeymap(0, stderr, 0, C->U, A, escape);
// 		freekeymap((Array *)escape);
// 		assert(fputc('\n', stderr) == '\n');
	}

	assert(isactive(C->U, A));

	SState st =
	{
		.inactive = NULL,
		.core = C,
		.area = A,
		.reactor = areareactor(C->U, A, rid),
		.alive = 0
	};

// 	if(DBGFLAGS & DBGSYNTH)
// 	{
// 		dumpkeymap(1, stderr, 0, C->U, A, NULL);
// 	}
	
	DBG(DBGSYNTH, "%s", "RF 1");
	{
		Ref *const RF = reactorforms(C->U, st.reactor);
		List *const l = RF->u.list;
		*RF = reflist(NULL);

		// После цикла прохода по l активированные формы можно стирать,
		// они разобраны на кусочки в activate, а неактивные формы
		// отмечены external в списке l и перенесены в st.inactive.
		// Поэтому l можно безопасно освободить

		const unsigned active = !forlist(l, synthone, &st, 0);
		freelist(l);

		if(!active)
		{
			// Область больше не активна (убедимся в этом), список
			// неактивных форм больше не нужен

			assert(!isactive(C->U, A));
			freelist(st.inactive);
			return 0;
		}
	}

	// Область активна и в дальнейшем интересны только неиспользованные
	// формы, поэтому (заменяем - ОШИБОЧНО) дополняем текущий список форм,
	// которые могли накопиться в процессе активации других форм при помощи
	// FPut

	assert(isactive(C->U, A));
	assert(areforms(reflist(st.inactive)));

	DBG(DBGSYNTH, "%s", "RF 2");
	{
		Ref *const RF = reactorforms(C->U, st.reactor);
		RF->u.list = append(RF->u.list, st.inactive);
	}

	return st.alive;
}

static unsigned syntaxmatch(
	Array *const U, const SyntaxNode op, const Array *const area)
{
	assert(op.op == EOP);

	const Ref R[2];
	assert(splitpair(areasyntax(U, area), (Ref *)R));

	return R[1].u.number == op.atom
		&& (R[0].u.number == UOP || R[0].u.number == LOP);
}

static Array *stackarea(Core *const C, const SyntaxNode op)
{
	const Ref key = reflist(RL(refatom(op.op), refatom(op.atom)));
	Array *const top = C->areastack ? toparea(C->areastack) : NULL;
	assert(!top || (isontop(C->U, top) && isonstack(C->U, top)));

	const Ref path = readtoken(C->U, "CTX");
	Array *const U = C->U;

	switch(op.op)
	{
	case AOP:
	case UOP:
	{
		// Нам понадобится новая область
		Array *const area = newarea(U, key, C->envtogo);
		assert(area);

		// Если на вершине стека была какая-то область, то надо с неё
		// снять метку ontop
		if(top)
		{
			markontop(U, top, 0);
		}

		// Для новой области надо поставить отметки о стеке и о вершине
		markonstack(U, area, 1);
		markontop(U, area, 1);

		C->areastack = append(RL(refkeymap(area)), C->areastack);
		return area;
	}

	case LOP:
	{
		// В этом случае нужно быть уверенными, что на вершине стека
		// что-то есть
		if(!top)
		{
			return NULL;
		}

		Array *const area = newarea(U, key, C->envtogo);
		assert(area);
		
		// Новая область будет на вершине стека. Отмечаем
		markonstack(U, area, 1);
		markontop(U, area, 1);

		{
			// В любом случае можно привязать top к новой области и
			// убрать top со стека. Теперь он будет называться LEFT
			// в area

			Array *const links = arealinks(U, area);
			assert(linkmap(U, links, path,
				readtoken(U, "LEFT"), refkeymap(top)) == top);

			markontop(U, top, 0);
			markonstack(U, top, 0);
		}

		if(isactive(U, top))
		{
			// Если top активная, то надо добавить UP-ссылку на area
			Array *const links = arealinks(U, top);
			assert(linkmap(U, links, path,
				readtoken(U, "UP"), refkeymap(area)) == area);
		}
		
		// Теперь надо заменить элемент на вершине стека на area
		freelist(tipoff(&C->areastack));
		C->areastack = append(RL(refkeymap(area)), C->areastack);
		return area;
	}

	case EOP:
	{
		// В этом случае на вершине стека должна быть конфигурация (...
		// area top). Нужно взять top и приписать справа к area. area
		// должна быть при этом активной, а её синтаксическая метка
		// должна соответствовать метке E. Сперва надо разобраться с
		// этими условиями

		const Ref S[2];
		const unsigned len = writerefs(C->areastack, (Ref *)S, 2);
		if(len != 2)
		{
			return NULL;
		}
		assert(S[0].u.array == top && iskeymap(S[1]));

		Array *const area = S[1].u.array;
		if(!isactive(U, area) || !syntaxmatch(U, op, area))
		{
			return NULL;
		}

		{
			// Теперь в любом случае нужно привязать top к area и
			// убрать top со стека. Новое имя top в area - это RIGHT

			Array *const links = arealinks(U, area);
			assert(linkmap(U, links, path,
				readtoken(U, "RIGHT"), refkeymap(top)) == top);
			
			markontop(U, top, 0);
			markonstack(U, top, 0);
		}

		if(isactive(U, top))
		{
			// Если top активная, то надо добавить UP-ссылку

			Array *const links = arealinks(U, top);
			assert(linkmap(U, links, path,
				readtoken(U, "UP"), refkeymap(area)) == area);
		}

		// Нужно поменять стек, убрав top (она теперь RIGHT) и поставив
		// TOP-отметку в area

		freelist(tipoff(&C->areastack));
		assert(iskeymap(tip(C->areastack)->ref)
			&& tip(C->areastack)->ref.u.array == area);

		markontop(U, area, 1);
		return area;
	}

	default:
		assert(0);
	}

	return NULL;
}

void ignite(Core *const C, const SyntaxNode op)
{
	const Ref key = reflist(RL(refatom(op.op), refatom(op.atom)));

	if(!C->envtogo)
	{
		char *const skey = strref(C->U, NULL, key);
		item = op.pos.line;
		ERR("STUCK state for input key: %s", skey);
		free(skey);
		return;
	}

	Array *const area = stackarea(C, op);
	if(!area)
	{
		char *const skey = strref(C->U, NULL, key);
		item = op.pos.line;
		ERR("can't get stack area for input key: %s", skey);
		free(skey);
		return;
	}

	const Ref body = getbody(C, op.op, op.atom);
	if(body.code == FREE)
	{
		char *const skey = strref(C->U, NULL, key);
		item = op.pos.line;
		ERR("can't get stack area for input key: %s", skey);
		free(skey);
		return;
	}

	const List *const out
		= RL(reflist(RL(forkref(key, NULL), refatom(op.atom))));
	
	if(intakeout(C->U, area, 0, out))
	{
		char *const skey = strref(C->U, NULL, key);
		item = op.pos.line;
		ERR("can't intake output list: %s", skey);
		free(skey);
		freelist((List *)out);
		return;
	}

	freelist((List *)out);

	intakeform(C->U, areareactor(C->U, area, 0), reflist(RL(key)), body);

	// Осталось добавить область в список активных областей
	if(!setmap(C->activity, refkeymap(area)))
	{
		tunesetmap(C->activity, refkeymap(area));
	}

	// И зарядить Core на ожидание нового Go
	C->envtogo = NULL;
}

// void progress(Core *const C, const SyntaxNode op)
void progress(Core *const C)
{
	const Array *active = C->activity;

	// Повторяем пока не встретилось Go (условие (!C->envtogo)) или пока
	// есть какая-то активность (условие (active->count > 0))
	
// 	while(!C->envtogo && active->count > 0)

	// Просто крутим активности до исчерпания. Всё-равно Go может быть
	// корректно исполнена только с вершины стека и один раз
	
	while(active->count > 0)
	{
		// Заряжаем новое отображение для описание накопленной
		// активности
		C->activity = newkeymap();

		// Идём по предыдущей таблице и обрабатываем все области в ней

		for(unsigned i = 0; i < active->count; i += 1)
		{
			const Binding *const b = bindingat(active, i);

			assert(iskeymap(b->key)
				&& b->ref.code == NUMBER && !b->ref.u.number);

			while(synthesize(C, b->key.u.array, 0))
			{
				DBG(DBGPRGS, "%s", "synthesized");
			}
		}

		// Теперь надо заменить предыдущую таблицу на новую

		freekeymap((Array *)active);
		active = C->activity;
	}
}
