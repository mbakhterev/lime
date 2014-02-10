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

	// Ну всё, сохраняем ссылку на форму, убедившись, что всё пока верно

	assert(areforms(in->ref));
	in->ref.u.list = append(in->ref.u.list, RL(markext(st->form)));

	return 0;
}

// Реализовано в соответствии с txt/log-2014.txt:2014-02-05 13:03:53

extern void intakeform(
	Array *const U, Array *const area, const unsigned rid, const Ref form)
{
	// Здесь должен быть список форм

	Ref *const F = reactorforms(U, area, rid);
	assert(areforms(*F));

	// Дальше нам надо создать форму со своим счётчиком, которая будет
	// добавлена во всевозможные списки. Для этого нужно скопировать граф и
	// ключи из исходной формы. newform копирует выражения, переданные через
	// параметры. Клиент intakeform может регулировать объём копирования
	// через external-биты

	const Ref keys = formkeys(form);
	assert(keys.code == LIST);
	const Ref f = newform(keys, formdag(form));

	// Форму надо засунуть в список реактора. Корректность F проверяется в
	// самой reactorforms. Но для надёжности

	assert(areforms(*F));
	F->u.list = append(F->u.list, RL(f));

	// Теперь надо расставить ссылки на форму

	RState st = 
	{
		.U = U,
		.reactor = areareactor(U, area, rid),
		.form = f
	};

	forlist(keys.u.list, registerone, &st, 0);

	// Кажется, всё
}
