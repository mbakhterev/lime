#include "construct.h"
#include "util.h"

#include <assert.h>

static unsigned areforms(const Ref r)
{
	return r.code == LIST && (r.u.list == NULL || isform(r.u.list->ref));
}

typedef struct
{
	Array *const U;
	Array *const reactor;
	const Ref form;
	Ref *const reactorforms;
} RState;

static int registerone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	RState *const st = ptr;

	// Надо поискать, существует ли соответствующий output

	DL(outkey, RS(decoatom(st->U, DOUT), l->ref));
	const Binding *const out = maplookup(st->reactor, outkey);
	if(out)
	{
		// Если существует, то надо уменьшить счётчик активации у формы
		// и больше ничего не делать

		countdown(&st->form);
		return 0;
	}

	// Если не существует, то надо записать форму в соответствующую input-у 
	// очередь форм. Ключ нужно сделать external-ом, потому что он хранится
	// где-то во внешнем контексте (в списке форм реактора, например).
	// НАПОМИНАНИЕ: bindkey копирует ключ при необходимости

	DL(inkey, RS(decoatom(st->U, DIN), markext(l->ref)));
	Binding *const in = bindkey(st->reactor, inkey);

	if(in->ref.code == FREE)
	{
		in->ref = reflist(NULL);
	}

	// Ну всё, сохраняем ссылку на форму, убедившись, что всё пока верно

	assert(areforms(in->ref));
	in->ref.u.list = append(in->ref.u.list, RL(markext(st->form)));

	return 0;
}

// Реализовано в соответствии с txt/log-2014.txt:2014-02-05 13:03:53

extern void intakeform(
	Array *const U, Array *const area, const unsigned rid,
	const Ref dag, const Ref keys)
{
	// Здесь должен быть список форм

	Ref *const RF = reactorforms(U, area, rid);
// 	assert(areforms(*RF));

	// Дальше нам надо создать форму со своим счётчиком, которая будет
	// добавлена во всевозможные списки. Для этого нужно скопировать
	// исходные граф и ключи. newform копирует выражения, переданные через
	// параметры. Клиент intakeform может регулировать объём копирования
	// через external-биты

	assert(dag.code == LIST && keys.code == LIST);
	const Ref f = newform(dag, keys);

	// Форму надо засунуть в список реактора. Корректность F проверяется в
	// самой reactorforms. Но для надёжности

	assert(areforms(*RF));
	RF->u.list = append(RF->u.list, RL(f));

	// Теперь надо расставить ссылки на форму

	RState st = 
	{
		.U = U,
		.reactor = areareactor(U, area, rid),
		.form = f,
		.reactorforms = NULL
	};

	forlist(keys.u.list, registerone, &st, 0);

	// Кажется, всё
}

static void splitpair(const Ref p, Ref R[])
{
	assert(p.code == LIST);
	const unsigned len = listlen(p.u.list);
	assert(len == 2 && writerefs(p.u.list, R, len) == len);
}

enum { KEY = 0, VALUE };

static int checkone(List *const l, void *const ptr)
{
	assert(l);
	const Ref R[2];
	splitpair(l->ref, (Ref *)R);

	assert(ptr);
	RState *const st = ptr;

	DL(key, RS(decoatom(st->U, DOUT), R[KEY]));
	const Binding *const b = maplookup(st->reactor, key);

	// Если (b == NULL), то всё хорошо, и надо вернуть 0. Поэтому
	return b != NULL;
}

static int countdownone(List *const l, void *const ptr)
{
	assert(l);
	countdown(&l->ref);
	return 0;
}

static int outone(List *const l, void *const ptr)
{
	assert(l);
	const Ref R[2];
	splitpair(l->ref, (Ref *)R);

	assert(ptr);
	RState *const st = ptr;

	// В любом случае надо зарегистрировать выход. Ключ будет сохранён

	{
		Binding *const b
			= mapreadin(st->reactor,
				decorate(forkref(R[KEY], NULL), st->U, DOUT));

		assert(b);
		b->ref = forkref(R[VALUE], NULL);
	}

	// Теперь надо уменьшить счётчики у тех форм, которым нужен этот вход

	DL(key, RS(decoatom(st->U, DIN), R[KEY]));
	Binding *const b = (Binding *)maplookup(st->reactor, key);
	if(!b)
	{
		// Ничего не было найдено, продолжаем
		return 0;
	}

	// Если есть ожидающие формы, надо приблизить их активацию, после чего
	// сбросить в NULL этот список

	assert(areforms(b->ref));
	forlist(b->ref.u.list, countdownone, NULL, 0);

	freeref(b->ref);
	b->ref = reflist(NULL);

	return 0;
}

unsigned intakeouts(
	Array *const U,
	Array *const area, const unsigned rid, const List *const outs)
{
	// Сперва надо проверить, не конфликтуют ли сигнатуры

	RState st = 
	{
		.U = U,
		.reactor = areareactor(U, area, rid),
		.form = reffree(),
		.reactorforms = reactorforms(U, area, rid)
	};

	if(forlist((List *)outs, checkone, &st, 0))
	{
		return !0;
	}

	// Если всё хорошо, то надо регистрировать выходы

	forlist((List *)outs, outone, &st, 0);
	return 0;
}
