#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGFENV 1
#define DBGFOUT 2

// #define DBGFLAGS (DBGFENV)
#define DBGFLAGS (DBGFOUT)

// #define DBGFLAGS 0

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

// 		countdown(&st->form);
		countdown(st->form);
		return 0;
	}

	// Если не существует, то надо записать форму в соответствующую input-у 
	// очередь форм. Ключ нужно сделать external-ом, потому что он хранится
	// где-то во внешнем контексте (в списке форм реактора, например).
	// НАПОМИНАНИЕ: bindkey копирует ключ при необходимости

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

// Реализовано в соответствии с txt/log-2014.txt:2014-02-05 13:03:53. С
// поправкой на управление памятью по мотивам 2014-02-13 14:40:18

// static Ref reform(const Ref f)
// {
// 	if(f.external)
// 	{
// 		// Имеем дело со ссылкой на форму. Для корректной работы надо
// 		// создать новую форму со своим счётчиком. Но сигнатуру и граф
// 		// можно использовать внешние
// 
// 		return newform(markext(formdag(f)), markext(formkeys(f)));
// 	}
// 
// 	// Здесь мы имеем дело с определением формы. Пока логика такая, что это
// 	// некая форма уже скопированная из графа. Поэтому, её можно
// 	// использовать (см. extractform)
// 
// 	assert(isform(f));
// 	return f;
// }

static Ref reform(const Ref keys, const Ref body)
{
	// Считаем, что ключи сформированы в decorate или exprewrite. И мы
	// забираем ключи без fork. Само тело формы может быть из окружения или
	// из текущего графа. В обоих случаях ожидаем, что всё уже сформировано
	// (см. extractbody)

	assert(!keys.external);
	return newform(body, keys);
}

extern void intakeform(
	Array *const U, Array *const R, const Ref keys, const Ref body)
// 	Array *const U, Array *const area, const unsigned rid, const Ref form)
{
	// Дальше нам надо создать форму со своим счётчиком, которая будет
	// добавлена во всевозможные списки. Для этого нужно скопировать
	// исходные граф и ключи. newform копирует выражения, переданные через
	// параметры. Клиент intakeform может регулировать объём копирования
	// через external-биты

// 	const Ref f = reform(form);
	const Ref f = reform(keys, body);
// 	Array *const R = areareactor(U, area, rid);

	// Форму надо засунуть в список реактора. Корректность RF проверяется в
	// самой reactorforms. Забываем о RF сразу после использования, чтобы
	// соответствующая ячейка в памяти от нас не утекла при перестроении
	// таблиц

	{
		Ref *const RF = reactorforms(U, R);
		RF->u.list = append(RF->u.list, RL(f));
	}

	// Теперь надо расставить ссылки на форму

	RState st = 
	{
		.U = U,
		.reactor = R,
		.form = f
	};

	forlist(formkeys(f).u.list, registerone, &st, 0);

	// Кажется, всё
}

// static unsigned splitpair(const Ref p, Ref R[])
// {
// 	if(p.code != LIST)
// 	{
// 		return 0;
// 	}
// 
// 	const unsigned len = listlen(p.u.list);
// 
// 	if(len != 2)
// 	{
// 		return 0;
// 	}
// 
// 	return writerefs(p.u.list, R, len) == len;
// }

enum { KEY = 0, VALUE };

static int checkone(List *const l, void *const ptr)
{
	assert(l);
	const Ref R[2];
	assert(splitpair(l->ref, (Ref *)R));

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
// 	countdown(&l->ref);
	countdown(l->ref);
	return 0;
}

static int outone(List *const l, void *const ptr)
{
	assert(l);
	const Ref R[2];
	assert(splitpair(l->ref, (Ref *)R));

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
		.form = reffree()
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

	const Array *const sysverbs;

	Array *const formmarks;
} FEState;

static void eval(const Ref r, FEState *const st);

static int evalone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	eval(l->ref, ptr);
	return 0;
}

static const unsigned char *nodename(const Array *const U, const Ref N)
{
	return atombytes(atomat(U, nodeverb(N, NULL)));
}

static Ref getexisting(const Array *const env, Array *const U, const Ref key)
{
	// WARNING: освобождаем переданный key локально

	List *const l
		= tracepath(env, U,
			readtoken(U, "ENV"), readtoken(U, "parent"));
	
	// markext для перестраховки

	DL(K, RS(decoatom(U, DFORM), markext(key)));
	const Binding *const b = pathlookup(l, K, NULL);

	freelist(l);
	freeref(key);

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
// 	Array *const env, Array *const U, const Ref key, const Ref form)
	Array *const env, Array *const U, const Ref key, const Ref body)

{

	// WARNING: освобождаем в случае неудачи переданные ресурсы здесь,
	// локально. Чтобы избежать дополнительных копирований

	const Ref K = decorate(key, U, DFORM);
	Binding *const b = mapreadin(env, K);
	if(!b)
	{
// 		freeform(form);
		freeref(body);
		freeref(K);
		return reffree();
	}

	assert(b->ref.code == FREE);

	// Рассчитываем на то, что форма уже сформирована нужным образом. Но
	// вернуть в любом случае нужно ссылку

// 	b->ref = form;
	b->ref = body;
	return markext(b->ref);
}

// static Ref extractform(const Ref A, FEState *const E)
// {
// 	if(A.code == NODE)
// 	{
// 		// Имеем дело с узлом. Пока это может быть только FEnv. Который
// 		// оценивается в форму из окружения. Её и возвращаем в виде
// 		// ссылки
// 
// 		switch(nodeverb(A, E->verbs))
// 		{
// 		case FENV:
// 			return markext(refmap(E->formmarks, A));
// 
// 		default:
// 			return reffree();
// 		}
// 	}
// 
// 	// В противном случае мы имеем дело с парой (ключи (тело формы)). Надо
// 	// её реконструировать
// 
// 	const Ref R[2];
// 	if(!splitpair(A, (Ref *)R))
// 	{
// 		return reffree();
// 	}
// 
// 	// Проверяем, что пара имеет вид (ключи; .F (...))
// 
// 	if(!isnode(R[1]) || nodeverb(R[1], E->verbs) != FNODE)
// 	{
// 		return reffree();
// 	}
// 
// 	// Проверяем, что ключи подходят под определение списка сигнатур формы.
// 	// Но сначала в них надо пересчитать типы
// 
// 	const Ref key = exprewrite(R[0], E->typemarks, E->typeverbs);
// 
// 	if(key.code != LIST || !issignaturekey(key))
// 	{
// 		freeref(key);
// 		return reffree();
// 	}
// 
// 	// Если всё хорошо, создаём новую форму. Граф при этом копируем. Ключ и
// 	// без того уже является преобразованной копией
// 
// 	return newform(forkdag(nodeattribute(R[1])), key);
// }

static Ref extractbody(const Ref A, FEState *const E)
{
	if(A.code == NODE)
	{
		// Имеем дело с узлом. Это может быть либо FEnv, который
		// оценивается в тело формы из окружения. Её и возвращаем в виде
		// ссылки. Либо F, который защищает тело формы. Её надо
		// скопировать в этом случае, потому что граф с этим телом будет
		// трансформироваться

		switch(nodeverb(A, E->verbs))
		{
		case FENV:
			return markext(refmap(E->formmarks, A));

		case FNODE:
			return forkdag(nodeattribute(A));

		default:
			return reffree();
		}
	}

// 	// В противном случае имеем дело с формой, заданной F-выражением
// 
// 	if(!isnode(A) || nodeverb(A, E->verbs) != FNODE)
// 	{
// 		return reffree();
// 	}
// 
// 	return forkdag(nodeattribute(A));

	return reffree();
}

static void fenv(const Ref N, FEState *const E)
{
	DBG(DBGFENV, "%s", "entry");

	const Ref r = nodeattribute(N);
	if(r.code != LIST)
	{
		item = nodeline(N);
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

	const Ref key = exprewrite(R[0], E->typemarks, E->typeverbs);

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

// Здесь нужно только тело формы
// 	const Ref form = extractform(R[1], E);

	const Ref body = len == 2 ? extractbody(R[1], E) : reffree();

	if(len == 2 && body.code == FREE)
	{
		item = nodeline(N);
		ERR("node \"%s\": can't read form from 2nd attribute structure",
			nodename(E->U, N));
	}

	// Теперь надо понять, с каким видом .FEnv мы имеем дело мы имеем дело.
	// Если у нас один параметр, то мы должны поискать форму в окружении.
	// Если два, то это запрос на регистрацию формы. Форму надо при этом
	// извлечь из второго параметра

// 	const Ref fref
	const Ref bref
		= (len == 1) ? getexisting(env, E->U, key)
// 		: (len == 2) ? setnew(env, E->U, key, form)
		: (len == 2) ? setnew(env, E->U, key, body)
		: reffree();
	
	// WARNING: key и form будут освобождены в getexisting или setnew по
	// необходимости

	if(bref.code == FREE)
	{
		const Ref tk = exprewrite(R[0], E->typemarks, E->typeverbs);
		char *const strkey = strref(E->U, NULL, tk);
		freeref(tk);

		item = nodeline(N);
		ERR("node \"%s\": can't %s type for key: %s",
			nodename(E->U, N), len == 1 ? "locate" : "allocate",
			strkey);

		free(strkey);
		return;
	}

	// Назначаем значение узлу. В form должен быть установлен external-бит

// 	assert(isform(fref) && fref.external);
// 	tunerefmap(E->formmarks, N, fref);

	assert(isdag(bref) && bref.external);
	tunerefmap(E->formmarks, N, bref);
}

typedef struct
{
	List *out;
	const Array *const typemarks;
	const Array *const typeverbs;
	const Array *const sysverbs;
} TState;

static unsigned isvalidlink(const Ref, const Array *const);

static int isvalidone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	return isvalidlink(l->ref, ptr);
}

static unsigned isvalidlink(const Ref r, const Array *const forbidden)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		// Допустимые Ref-ы
		return !0;
	
	case NODE:
		if(!r.external)
		{
			// В описании связи между кусочками графов не может быть
			// определения узла

			return 0;
		}

		if(knownverb(r, forbidden))
		{
			// Ссылаться на эти узлы нельзя. Основная причина - они
			// системные и будут удалены из текущего кусочка графа
			// ходе его трансформаций

			return 0;
		}

		// Анализируем ссылки на узлы, поэтому атрибуты их нам не
		// интересны. Поэтому, остальное допустимо

		return !0;
	
	case LIST:
		return forlist(r.u.list, isvalidone, (Array *)forbidden, !0);
	
	default:
		// Всё остальное недопустимо
		return 0;
	}
}

static int translateone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	TState *const st = ptr;

	const Ref R[2];
	if(!splitpair(l->ref, (Ref *)R))
	{
		return !0;
	}

	const Ref key = exprewrite(R[0], st->typemarks, st->typeverbs);
	if(!issignaturekey(key))
	{
		freeref(key);
		return !0;
	}

	if(!isvalidlink(R[0], st->sysverbs))
	{
		freeref(key);
		return !0;
	}

	st->out = append(st->out, RL(reflist(RL(key, forkref(R[1], NULL)))));

	return 0;
}

static void fout(const Ref N, FEState *const E)
{
	const Ref r = nodeattribute(N);
	if(r.code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting attribute list", nodename(E->U, N));
		return;
	}

	const unsigned len = listlen(r.u.list);
	if(len != 2)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting 2 attributes", nodename(E->U, N));
		return;
	}

	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len);
	if(R[0].code != NUMBER || R[0].u.number > 1 || R[1].code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure",
			nodename(E->U, N));
		return;
	}

	const unsigned rid = R[0].u.number;

	// Теперь нужно превратить исходный список пар в новый список пар с
	// преобразованными с учётом типов и проверенными на корректность
	// ключами. Эта операция может не завершиться успехом. Учитываем это

	TState st =
	{
		.out = NULL,
		.typeverbs = E->typeverbs,
		.typemarks = E->typemarks,
		.sysverbs = E->sysverbs
	};

	if(forlist(R[1].u.list, translateone, &st, 0))
	{
		freelist(st.out);

		item = nodeline(N);
		ERR("node \"%s\": wrong output list structure",
			nodename(E->U, N));
		return;
	}

	if(DBGFLAGS & DBGFOUT)
	{
		char *const ostr = strlist(E->U, st.out);
		DBG(DBGFOUT, "intaking: %s", ostr);
		free(ostr);
	}

	if(intakeout(E->U, E->area, rid, st.out))
	{
		freelist(st.out);

		item = nodeline(N);
		ERR("node \"%s\": can't intake output list",
			nodename(E->U, N));
		return;
	}

	freelist(st.out);
}

static void eval(const Ref r, FEState *const st)
{
	switch(r.code)
	{
	case ATOM:
	case NUMBER:
	case TYPE:
		return;

	// Здесь различия между DAG и LIST неcущественны 

	case LIST:
	case DAG:
		forlist(r.u.list, evalone, st, 0);
		return;

	case NODE:
		if(r.external)
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
			return;

		case FOUT:
			fout(r, st);
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

static const char *const sysverbs[] =
{
	"FIn", "Nth",
	"F", "FEnv", "FOut",
	"R", "Rip",
	NULL
};

void formeval(
	Array *const U,
	Array *const area,
	const Ref dag, const Array *const escape,
	const Array *const envmarks, const Array *const typemarks)
{
	FEState st =
	{
		.U = U,
		.area = area,
		.escape = escape,
		.verbs = newverbmap(U, 0, verbs),
		.typeverbs = newverbmap(U, 0, ES("T", "TEnv")),
		.formmarks = newkeymap(),
		.typemarks = typemarks,
		.envmarks = envmarks,
		.sysverbs = newverbmap(U, 0, sysverbs)
	};

	eval(dag, &st);

	freekeymap((Array *)st.sysverbs);
	freekeymap(st.formmarks);
	freekeymap((Array *)st.typeverbs);
	freekeymap((Array *)st.verbs);
}
