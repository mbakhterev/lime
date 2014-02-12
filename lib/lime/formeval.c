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

// 	DL(inkey, RS(decoatom(st->U, DIN), markext(l->ref)));

	DL(inkey, RS(decoatom(st->U, DIN), dynamark(l->ref)));
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

unsigned intakeout(
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

#define FNODE 0
#define FPUT 1
#define FENV 2
#define FOUT 3

static const char *const verbs[] =
{
	[FNODE] = "F",
	[FPUT] = "FPut",
	[FENV] = "FEnv",
	[FOUT] = "FOut",
	NULL
};

typedef struct
{
	Array *const U;
	Array *const area;

	const Array *const escape;

	const Array *const typemarks;
	const Array *const envmarks;

	const Array *const verbs;
	const Array *const typeverbs;

	Array *const valmap;
} FEState;

static void eval(const Ref r, FEState *const st);

static int evalone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	eval(l->ref, ptr);
	return 0;
}

static const char *nodename(const Array *const U, const Ref N)
{
	return atombytes(atomat(U, nodeverb(N, NULL)));
}

static Ref readtoken(Array *const U, const char *const str)
{
	return readpack(U, strpack(0, str));
}

static Ref getexisting(const Array *const env, Array *const U, const Ref key)
{
	List *const l
		= tracepath(env, U,
			readtoken(U, "ENV"), readtoken(U, "parent"));
	
	// markext для перестраховки

	DL(K, RS(decoatom(U, DFORM), markext(key)));
	const Binding *const b = pathlookup(l, K, NULL);

	freelist(l);

	if(b)
	{
		assert(b->ref.code == FORM);

		// Здесь нам нужна только ссылка на форму, которая уже сохранена
		// неким образом в окружении. Поэтому markext

		return markext(b->ref);
	}

	return reffree();
}

static Ref setnew(
	Array *const env, Array *const U, const Ref key, const Ref form)
{
	if(form.code != FORM)
	{
		return reffree();
	}

	const Ref K = decorate(forkref(key, NULL), U, DFORM);
	Binding *const b = mapreadin(env, K);
	if(!b)
	{
		freeref(K);
		return reffree();
	}

	assert(b->ref.code == FREE);

	// Рассчитываем на то, что форма уже сформирована нужным образом. Но
	// вернуть в любом случае нужно ссылку

	b->ref = form;
	return markext(b->ref);
}

static void fenv(const Ref N, FEState *const E)
{
	const Ref r = nodeattribute(r);
	if(r.code != LIST)
	{
		item = nodeline(r);
		ERR("node \"%s\": expecting attribute list",
			nodename(E->U, N));

		return;
	}

	const unsigned len = listlen(r.u.list);
	if(len < 1 || 2 < len)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting 1 or 2 attributes",
			nodename(E->U, N));

		return;
	}

	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len);

	// В ключе могут быть ссылки на типы. Для форм мы это разрешаем. Поэтому
	// надо преобразовать выражение

	const Ref key = exprewrite(R[0], st->typemarks, st->typeverbs);

	if(!issignaturekey(key))
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting 1st attribute to be basic key",
			nodename(E->U, N));

		return;
	}
	
	// В любом случае надо узнать, к какому окружению относится этот .FEnv

	Array *const env = envmap(E->envmarks, N);
	if(!env)
	{
		item = nodeline(N);
		ERR("node \"%s\": no environment definition for node",
			nodename(E->U, N));

		return;
	}

	// Теперь надо понять, с какой формой мы имеем дело. Если у нас один
	// параметр, то мы должны поискать форму в окружении. Если два, то это
	// запрос на регистрацию формы. Форму надо при этом извлечь из второго
	// параметра

	const Ref form
		= (len == 1) ? getexisting(env, E->U, key)
		: (len == 2) ? setnew(env, E->U, key, extractform(R[1], E))
		: reffree();
	if(form.code == FREE)
	{
		char *const strkey = strref(E->U, NULL, key);
		freeref(key);

		item = nodeline(N);
		ERR("node \"%s\": can't %s type for key: %s",
			nodename(N), len == 1 ? "locate" : "allocate", strkey);

		free(strkey);
		return;
	}
	freeref(key);

	// Назначаем значение узлу. В form должен быть установлен external-бит

	assert(form.code == FORM && form.external);
	tunerefmap(E->valmap, N, form);
}

static void eval(const Ref r, FEState *const st)
{
	switch(r.code)
	{
	case ATOM:
	case NUMBER:
	case TYPE:
		return;

	case LIST:
		forlist(r.u.list, evalone, st, 0);
		return;

	case NODE:
		if(!r.external)
		{
			// Интересуемся только определениями
			return;
		}

		switch(nodeverb(r, st->verbs))
		{
		case FENV:
			fenv(r, st);
			return;

		case FPUT:
		case FOUT:
			return;

		default:
			if(!knownverb(r, st->escape))
			{
				// Если не запрещено пройти внутрь узла
				eval(nodeattribute(r), st);
			}

			return;
		}

	default:
		assert(0);
	}
}

void formeval(
	Array *const U,
	Array *const area,
	const Ref dag, const Array *const escape,
	const Array *const typemarks, const Array *const envmarks)
{
	FEState st =
	{
		.U = U,
		.area = area,
		.escape = escape,
		.verbs = newverbmap(U, 0, verbs),
		.typeverbs = newverbmap(U, 0, ES("T", "TEnv")),
		.valmap = newkeymap(),
		.typemarks = typemarks,
		.envmarks = envmarks
	};

	eval(dag, &st);

	freekeymap(st.valmap);
	freekeymap((Array *)st.verbs);
}
