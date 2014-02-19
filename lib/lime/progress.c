#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGPRGS 1
#define DBGGF 2
#define DBGSYNTH 4

// #define DBGFLAGS (DBGPRGS | DBGGF)

#define DBGFLAGS (DBGPRGS | DBGSYNTH)

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

// 		const Ref key
// 			= decorate(
// 				reflist(RL(refatom(op), refatom(atom))),
// 				U, DFORM);

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

// 		const Ref key 
// 			= decorate(
// 				reflist(RL(refatom(op), atomtype(U, T, atom))),
// 				U, DFORM);

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

// static Ref activate(Array *const U, Array *const T, Array *const S, const Ref f)
// {
// 	return reffree();
// }

typedef struct
{
	List *inactive;
	Core *const C;
	Array *const R;
} SState;

static int synthone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	SState *const st = ptr;

	// Превращаем звено списка в список из одного элемента
	l->next = l;

	// FIXME
	if(formcounter(l->ref))
	{
		// Эту форму рано ещё активировать, перекладываем её в
		// inactive-список

		st->inactive = append(st->inactive, l);
		return 0;
	}

	freelist(l);

	return 0;
}

static unsigned areforms(const Ref r)
{
	return r.code == LIST && (r.u.list == NULL ||  isform(r.u.list->ref));
}

static void synthesize(Core *const C, Array *const A, const unsigned rid)
{
	SState st =
	{
		.inactive = NULL,
		.C = C,
		.R = areareactor(C->U, A, rid),
	};

// 	{
// 		Ref *const RF = reactorforms(C->U, A, rid);
// 		forlist(RF->u.list, synthone, &st, 0);
// 	}

	if(DBGFLAGS & DBGSYNTH)
	{
		dumpkeymap(0, stderr, 0, C->U, A);
	}
	forlist(reactorforms(C->U, A, rid)->u.list, synthone, &st, 0);

	// Нам в дальнейшем интересны только неиспользованные формы, поэтому
	// заменяем текущий список форм

	assert(areforms(reflist(st.inactive)));
	*reactorforms(C->U, A, rid) = reflist(st.inactive);
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

	synthesize(C, A.u.array, 0);

	if(DBGFLAGS & DBGPRGS)
	{
		dumpkeymap(0, stderr, 0, C->U, A.u.array);
	}
}
