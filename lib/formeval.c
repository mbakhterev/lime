#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGFENV 1
#define DBGFOUT 2
#define DBGFPUT 3

// #define DBGFLAGS (DBGFENV)
// #define DBGFLAGS (DBGFOUT | DBGFPUT)
#define DBGFLAGS (DBGFENV)

// #define DBGFLAGS 0

// Реализовано в соответствии с
// 	2014-02-05 13:03:53
// 	2014-02-13 14:40:18
// 	2014-02-28 09:07:52
//	2014-03-29 23:03:44

typedef struct
{
	Array *const U;
	Array *const reactor;
	Array *const selfcheck;
	const Ref form;
} RState;

static int registerone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	RState *const st = ptr;

	// Надо поискать, существует ли соответствующий output

	DL(outkey, RS(decoatom(st->U, DOUT), l->ref));
	const Binding *const out
		= bindingat(st->reactor, maplookup(st->reactor, outkey));
	if(out)
	{
		// Если существует, то надо уменьшить счётчик активации у формы
		countdown(st->form);

		// Компонента исходной сигнатуры в l->ref нам больше не нужна
		freeref(l->ref);

		// Заменяем её ссылкой на ключ, хранящийся в out
		{
			const Ref R[2];
			assert(splitpair(out->key, (Ref *)R));
			l->ref = markext(R[1]);
		}

		return 0;
	}

	// Если не существует, то надо записать форму в соответствующую input-у 
	// очередь форм. input-ы могут жить дольше форм, поэтому мы позволяем
	// скопировать bindkey ключ из l->ref внутрь, в случае необходимости.
	// После привязывания исходный l->ref нам уже не нужен

	DL(inkey, RS(decoatom(st->U, DIN), l->ref));
	Binding *const in
		= (Binding *)bindingat(st->reactor,
			bindkey(st->reactor, inkey));
	freeref(l->ref);

	// Небольшая поправка содержимого и проверка корректности

	if(in->ref.code == FREE)
	{
		in->ref = reflist(NULL);
	}

	assert(areforms(in->ref));

	// В l->ref сохраняем теперь ссылку на содержимое ключа в in
	{
		const Ref R[2];
		assert(splitpair(in->key, (Ref *)R));
		l->ref = markext(R[1]);
	}

	// Сохраняем форму в списке ожидающих входа форм
	in->ref.u.list = append(in->ref.u.list, RL(markext(st->form)));

	return 0;
}

void intakeform(Array *const U, Array *const R, const Ref f)
{
	assert(R);
	assert(isform(f));

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
		.form = f,
		.selfcheck = NULL
	};

	forlist(formkeys(f).u.list, registerone, &st, 0);

	// Кажется, всё
}

enum { KEY = 0, VALUE };

static int checkone(List *const l, void *const ptr)
{
	assert(l);
	const Ref R[2];
	assert(splitpair(l->ref, (Ref *)R));

	assert(ptr);
	RState *const st = ptr;

	// Сначала проверка на самопересечения

	if(setmap(st->selfcheck, R[KEY]))
	{
		return !0;
	}

	tunesetmap(st->selfcheck, R[KEY]);
	
	DL(key, RS(decoatom(st->U, DOUT), R[KEY]));
	const Binding *const b
		= bindingat(st->reactor, maplookup(st->reactor, key));

	// Если (b == NULL), то всё хорошо, и надо вернуть 0. Поэтому
	return b != NULL;
}

static int countdownone(List *const l, void *const ptr)
{
	assert(l);
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
			= (Binding *)bindingat(st->reactor,
				mapreadin(st->reactor,
					decorate(forkref(R[KEY], NULL),
						st->U, DOUT)));

		assert(b);
		b->ref = forkref(R[VALUE], NULL);
	}

	// Теперь надо уменьшить счётчики у тех форм, которым нужен этот вход

	DL(key, RS(decoatom(st->U, DIN), R[KEY]));
	Binding *const b
		= (Binding *)bindingat(st->reactor,
			maplookup(st->reactor, key));
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
	assert(isactive(U, area));

	// Сперва надо проверить, не конфликтуют ли сигнатуры

	RState st = 
	{
		.U = U,
		.reactor = areareactor(U, area, rid),
		.form = reffree(),
		.selfcheck = newkeymap()
	};

	const unsigned fail = forlist((List *)outs, checkone, &st, 0);
	freekeymap(st.selfcheck);

	if(fail)
	{
		return !0;
	}

	// Если всё хорошо, то надо регистрировать выходы

	forlist((List *)outs, outone, &st, 0);
	return 0;
}

// #define FNODE	0
// #define FPUT	1
// #define FENV	2
// #define FOUT	3
// #define RNODE	4
// 
// static const char *const verbs[] =
// {
// 	[FNODE]	= "F",
// 	[FPUT]	= "FPut",
// 	[FENV]	= "FEnv",
// 	[FOUT]	= "FOut",
// 	[RNODE]	= "R",
// 	NULL
// };
// 
// typedef struct
// {
// 	Array *const U;
// 	Array *const area;
// 	Array *const activity;
// 
// 	const Array *const escape;
// 
// 	const Array *const typemarks;
// 	const Array *const envmarks;
// 	const Array *const areamarks;
// 
// 	const Array *const verbs;
// 	const Array *const typeverbs;
// 	const Array *const sysverbs;
// 
// 	Array *const formmarks;
// } FEState;
// 
// static void eval(const Ref r, FEState *const st);
// 
// static int evalone(List *const l, void *const ptr)
// {
// 	assert(l);
// 	assert(ptr);
// 	eval(l->ref, ptr);
// 	return 0;
// }

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
		assert(b->ref.code == DAG);

		// Здесь нам нужна только ссылка на форму, которая уже сохранена
		// неким образом в окружении. Поэтому markext

		return markext(b->ref);
	}

	return reffree();
}

static Ref setnew(
	Array *const env, Array *const U, const Ref key, const Ref body)

{
	// WARNING: освобождаем в случае неудачи переданные ресурсы здесь,
	// локально. Чтобы избежать дополнительных копирований

	const Ref K = decorate(key, U, DFORM);
	Binding *const b = (Binding *)bindingat(env, mapreadin(env, K));
	if(!b)
	{
		freeref(body);
		freeref(K);
		return reffree();
	}

	assert(b->ref.code == FREE);

	// Рассчитываем на то, что форма уже сформирована нужным образом. Но
	// вернуть в любом случае нужно ссылку

	b->ref = body;
	return markext(b->ref);
}

// static Ref extractform(
// 	const Ref A, const Ref keys, const unsigned copy, FEState *const E)

static Ref extractform(
	Array *const U,
	const Ref A, const Ref keys, const unsigned copy,
	const Array *const formmarks, const Array *const verbs)
{
	if(A.code != NODE)
	{
		return reffree();
	}

	switch(nodeverb(A, verbs))
	{
	case FENV:
	{
		// Необходимо извлечь тело формы из окружения. Трассы в
		// окружении нет, поэтому на её месте пустой граф. Следует или
		// нет копировать тело формы зависит от флага. Ключ уже должен
		// быть готов к использованию в форме

		const Ref ref = refmap(formmarks, A);
		assert(isdag(ref) && ref.external);
		const Ref body = !copy ? ref : forkdag(cleanext(ref));

		return newform(body, refdag(NULL), keys);
	}

	case FNODE:
		// Здесь копирование графа в атрибуте узла .F должно быть
		// безусловным, так как исходная форма подвергнется чистке

		return newform(forkdag(nodeattribute(A)), refdag(NULL), keys);
	
	case RNODE:
	{
		Array *const R = envmap(formmarks, A);
		if(!R || isactive(U, R) || isareaconsumed(U, R))
		{
			return reffree();
		}

		const Ref body;
		const Ref trace;	
		riparea(U, R, (Ref *)&body, (Ref *)&trace);

		return newform(body, trace, keys);
	}
	}

	return reffree();
}

// static void fenv(const Ref N, FEState *const E)
// {
// 	DBG(DBGFENV, "%s", "entry");
// 
// 	const Ref r = nodeattribute(N);
// 	if(r.code != LIST)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": expecting attribute list",
// 			nodename(E->U, N));
// 
// 		return;
// 	}
// 
// 	const unsigned len = listlen(r.u.list);
// 	if(len < 1 || 2 < len)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": expecting 1 or 2 attributes",
// 			nodename(E->U, N));
// 
// 		return;
// 	}
// 
// 	const Ref R[len];
// 	assert(writerefs(r.u.list, (Ref *)R, len) == len);
// 
// 	// В ключе могут быть ссылки на типы. Для форм мы это разрешаем. Поэтому
// 	// надо преобразовать выражение
// 
// 	const Ref key = exprewrite(R[0], E->typemarks, E->typeverbs);
// 
// 	if(!issignaturekey(key))
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": expecting 1st attribute to be basic key",
// 			nodename(E->U, N));
// 
// 		return;
// 	}
// 	
// 	// В любом случае надо узнать, к какому окружению относится этот .FEnv
// 
// 	Array *const env = envmap(E->envmarks, N);
// 	if(!env)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": no environment definition for node",
// 			nodename(E->U, N));
// 
// 		return;
// 	}
// 
// // 	const Ref body = len == 2 ? extractbody(R[1], 0, E) : reffree();
// 	const Ref form
// 		= len == 2 ? extractform(R[1], reflist(NULL), 0, E) : reffree();
// 
// 	if(len == 2 && form.code == FREE)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": can't reconstruct form with 2nd attribute",
// 			nodename(E->U, N));
// 	}
// 
// 	assert(len != 2 || formkeys(form).u.list == NULL);
// 
// 	if(len == 2 && formtrace(form).u.list != NULL)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": environment forms should be without trace",
// 			nodename(E->U, N));
// 	}
// 
// 	// Забираем из формы body, а саму форму освобождаем, почистив поле DAG.
// 	// Это всё по условию, что у нас есть form
// 
// 	const Ref body = form.code != FREE ? formdag(form) : reffree();
// 
// 	if(form.code != FREE)
// 	{
// 		Ref *const R[FORMLEN];
// 		splitlist(form.u.list, (const Ref **const)R, FORMLEN);
// 		*R[BODY] = refdag(NULL);
// 
// 		freeform(form);
// 	}
// 
// 	// Теперь надо понять, с каким видом .FEnv мы имеем дело мы имеем дело.
// 	// Если у нас один параметр, то мы должны поискать форму в окружении.
// 	// Если два, то это запрос на регистрацию формы. Форму надо при этом
// 	// извлечь из второго параметра
// 
// 	const Ref bref
// 		= (len == 1) ? getexisting(env, E->U, key)
// 		: (len == 2) ? setnew(env, E->U, key, body)
// 		: reffree();
// 	
// 	// WARNING: key и body будут освобождены в getexisting или setnew по
// 	// необходимости
// 
// 	if(bref.code == FREE)
// 	{
// 		const Ref tk = exprewrite(R[0], E->typemarks, E->typeverbs);
// 		char *const strkey = strref(E->U, NULL, tk);
// 		freeref(tk);
// 
// 		item = nodeline(N);
// 		ERR("node \"%s\": can't %s type for key: %s",
// 			nodename(E->U, N), len == 1 ? "locate" : "allocate",
// 			strkey);
// 
// 		free(strkey);
// 		return;
// 	}
// 
// 	// Назначаем значение узлу. В form должен быть установлен external-бит
// 
// 	assert(isdag(bref) && bref.external);
// 	tunerefmap(E->formmarks, N, bref);
// }

void dofenv(
	Core *const C, Array *const marks, Array *const formmarks,
	const Ref N, const unsigned envnum)
{
	Array *const U = C->U;
	Array *const E = C->E;
	const Array *const V = C->verbs.system;

	const Ref r = nodeattribute(N);
	if(r.code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting attribute list", nodename(U, N));
		return;
	}

	const unsigned len = listlen(r.u.list);
	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len);

	const Ref key = len > 0 ? simplerewrite(R[0], marks) : reffree();
	const Ref form = len > 1 ?
		  extractform(U, R[1], reflist(NULL), 0, formmarks, V)
		: reffree();
	
	const unsigned kok = issignaturekey(key);
	const unsigned fok = len != 2 || form.code == FORM;

	DBG(DBGFENV, "len kok fok: %u %u %u", len, kok, fok);
	
// 	if(len < 1 || 2 < len
// 		|| !issignaturekey(key)
// 		|| (len == 2 && form.code == FREE))

	if(len < 1 || 2 < len || !kok || !fok)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	if(form.code == FORM && formkeys(form).u.list != NULL)
	{
		item = nodeline(N);
		ERR("node \"%s\": can't take form with trace into environment",
			nodename(U, N));
		return;
	}

	const Ref body = form.code != FREE ? formdag(form) : reffree();

	if(form.code != FREE)
	{
		Ref *const R[FORMLEN];
		assert(splitlist(form.u.list, (const Ref **const)R, FORMLEN));
		*R[BODY] = refdag(NULL);

		freeform(form);
	}

	Array *const env = envkeymap(E, refenv(envnum));

	// Об освобождении key и body позаботятся getexisting и setnew

	const Ref bref
		= len == 1 ? getexisting(env, U, key)
		: len == 2 ? setnew(env, U, key, body)
		: reffree();

	if(bref.code == FREE)
	{
		const Ref tk = simplerewrite(R[0], marks);
		char *const kstr = strref(U, NULL, tk);
		freeref(tk);

		item = nodeline(N);
		ERR("node \"%s\": can't %s type for key: %s",
			nodename(U, N),
			len == 1 ? "locate" : "allocate",
			kstr);

		free(kstr);
		return;
	}

	assert(isdag(bref) && bref.external);
	tunerefmap(formmarks, N, bref);
}

typedef struct
{
	List *out;
	const Array *const typemarks;
	const Array *const typeverbs;
	const Array *const sysverbs;
	const unsigned allownodes;
} TState;

// static unsigned isvalidlink(const Ref, const Array *const);
static unsigned isvalidlink(const Ref, const TState *const ptr);

static int isvalidone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	return isvalidlink(l->ref, ptr);
}

// static unsigned isvalidlink(const Ref r, const Array *const forbidden)
static unsigned isvalidlink(const Ref r, const TState *const st)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		// Допустимые Ref-ы
		return !0;
	
	case NODE:
		if(!st->allownodes)
		{
			// Узлы нельзя передавать
			return 0;
		}

		if(!r.external)
		{
			// В описании связи между кусочками графов не может быть
			// определения узла
			return 0;
		}

// 		if(knownverb(r, st->forbidden))
		if(knownverb(r, st->sysverbs))
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
// 		return forlist(r.u.list, isvalidone, (Array *)forbidden, !0);
		return forlist(r.u.list, isvalidone, (TState *)st, !0);
	
	default:
		// Всё остальное недопустимо
		return 0;
	}
}

// static int translateone(List *const l, void *const ptr)
// {
// 	assert(l);
// 	assert(ptr);
// 	TState *const st = ptr;
// 
// 	const Ref R[2];
// 	if(!splitpair(l->ref, (Ref *)R))
// 	{
// 		return !0;
// 	}
// 
// 	const Ref key = exprewrite(R[0], st->typemarks, st->typeverbs);
// 	if(!issignaturekey(key))
// 	{
// 		freeref(key);
// 		return !0;
// 	}
// 
// 	const Ref links = exprewrite(R[1], st->typemarks, st->typeverbs);
// 
// // 	if(!isvalidlink(R[1], st->sysverbs))
// // 	if(!isvalidlink(R[1], st))
// 	if(!isvalidlink(links, st))
// 	{
// 		freeref(links);
// 		freeref(key);
// 		return !0;
// 	}
// 
// // 	st->out = append(st->out, RL(reflist(RL(key, forkref(R[1], NULL)))));
// 	st->out = append(st->out, RL(reflist(RL(key, links))));
// 
// 	return 0;
// }
// 
// typedef struct
// {
// 	Array *const area;
// 	const unsigned rid;
// } Target;
// 
// static Target notarget()
// {
// 	return (Target) { .area = NULL, .rid = -1 };
// }
// 
// static Target aim(const Ref R, FEState *const E)
// {
// 	switch(R.code)
// 	{
// 	case NUMBER:
// 		if(R.u.number > 1)
// 		{
// 			return notarget();
// 		}
// 
// 		return (Target) { .area = E->area, .rid = R.u.number };
// 	
// 	case NODE:
// 	{
// 		Array *const area = envmap(E->areamarks, R);
// 		if(!area)
// 		{
// 			return notarget();
// 		}
// 
// 		return (Target) { .area = area, .rid = 0 };
// 	}
// 	}
// 
// 	return notarget();
// }
// 
// static void fout(const Ref N, FEState *const E)
// {
// 	if(!E->activity || !E->area)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": can't evaluate in init mode",
// 			nodename(E->U, N));
// 		return;
// 	}
// 
// 	const Ref r = nodeattribute(N);
// 	if(r.code != LIST)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": expecting attribute list", nodename(E->U, N));
// 		return;
// 	}
// 
// 	const unsigned len = listlen(r.u.list);
// 	const Ref R[len];
// 	assert(writerefs(r.u.list, (Ref *)R, len) == len);
// 	if(len != 2
// 		|| (R[0].code != NUMBER && R[0].code != NODE)
// 		|| R[1].code != LIST)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": wrong attribute structure",
// 			nodename(E->U, N));
// 		return;
// 	}
// 
// 	const Target T = aim(R[0], E);
// 	if(!T.area)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": can't detect reactor with 1st attribute",
// 			nodename(E->U, N));
// 
// 		return;
// 	}
// 
// 	if(!isactive(E->U, T.area))
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": inactive area is specified",
// 			nodename(E->U, N));
// 
// 		return;
// 	}
// 
// // 	const unsigned rid = R[0].u.number;
// 
// 	// Теперь нужно превратить исходный список пар в новый список пар с
// 	// преобразованными с учётом типов и проверенными на корректность
// 	// ключами. Эта операция может не завершиться успехом. Учитываем это
// 
// 	TState st =
// 	{
// 		.out = NULL,
// 		.typeverbs = E->typeverbs,
// 		.typemarks = E->typemarks,
// 		.sysverbs = E->sysverbs,
// 		.allownodes = T.area == E->area
// 	};
// 
// 	if(forlist(R[1].u.list, translateone, &st, 0))
// 	{
// 		freelist(st.out);
// 
// 		item = nodeline(N);
// 		ERR("node \"%s\": wrong output list structure",
// 			nodename(E->U, N));
// 		return;
// 	}
// 
// 	if(DBGFLAGS & DBGFOUT)
// 	{
// 		char *const ostr = strlist(E->U, st.out);
// 		DBG(DBGFOUT, "intaking: %s", ostr);
// 		free(ostr);
// 	}
// 
// // 	if(intakeout(E->U, E->area, rid, st.out))
// 	if(intakeout(E->U, T.area, T.rid, st.out))
// 	{
// 		freelist(st.out);
// 
// 		item = nodeline(N);
// 		ERR("node \"%s\": can't intake output list",
// 			nodename(E->U, N));
// 		return;
// 	}
// 
// 	freelist(st.out);
// 
// 	// Добавляем информацию об активности в области, если она отличается от
// 	// текущей
// 
// 	if(T.area != E->area && !setmap(E->activity, refkeymap(T.area)))
// 	{
// 		tunesetmap(E->activity, refkeymap(T.area));
// 	}
// }
// 
// static void fput(const Ref N, FEState *const E)
// {
// 	DBG(DBGFPUT, "%s", "here");
// 
// 	if(!E->activity || !E->area)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": can't evaluate in init mode",
// 			nodename(E->U, N));
// 		return;
// 	}
// 
// 	const Ref r = nodeattribute(N);
// 	if(r.code != LIST)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": expecting attribute list", nodename(E->U, N));
// 		return;
// 	}
// 
// 	const unsigned len = listlen(r.u.list);
// 	const Ref R[len];
// 	assert(writerefs(r.u.list, (Ref *)R, len) == len);
// 	if(len != 3
// 		|| (R[0].code != NUMBER && R[0].code != NODE)
// 		|| R[1].code != LIST
// 		|| R[2].code != NODE)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": wrong attribute structure",
// 			nodename(E->U, N));
// 		return;
// 	}
// 
// 	const Target T = aim(R[0], E);
// 	if(!T.area)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": can't detect reactor with 1st attribute",
// 			nodename(E->U, N));
// 
// 		return;
// 	}
// 
// 	if(!isactive(E->U, T.area))
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": inactive area is specified",
// 			nodename(E->U, N));
// 
// 		return;
// 	}
// 
// // 	const unsigned rid = R[0].u.number;
// 
// 	const Ref K = exprewrite(R[1], E->typemarks, E->typeverbs);
// 	if(!issignaturekey(K))
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": can't get signature from 2nd attribute",
// 			nodename(E->U, N));
// 
// 		freeref(K);
// 		return;
// 	}
// 
// 	// Если отправляем форму не в локальный R.0, то надо её скопировать
// 	const Ref form
// 		= extractform(R[2], K, T.area != E->area || T.rid != 0, E);
// 
// 	if(form.code == FREE)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": can't extract form body from 3rd attribute",
// 			nodename(E->U, N));
// 
// 		freeref(K);
// 		return;
// 	}
// 
// 	assert(formkeys(form).u.list == K.u.list);
// 
// 	if(DBGFLAGS & DBGFPUT)
// 	{
// 		char *const kstr = strref(E->U, NULL, K);
// 		DBG(DBGFPUT, "intaking with key: %s", kstr);
// 		free(kstr);
// 	}
// 
// 	// Добавляем информацию об активности в области, если она отличается от
// 	// текущей
// 
// 	if(T.area != E->area && !setmap(E->activity, refkeymap(T.area)))
// 	{
// 		tunesetmap(E->activity, refkeymap(T.area));
// 	}
// }
// 
// static void eval(const Ref r, FEState *const st)
// {
// 	switch(r.code)
// 	{
// 	case ATOM:
// 	case NUMBER:
// 	case TYPE:
// 		return;
// 
// 	// Здесь различия между DAG и LIST неcущественны 
// 
// 	case LIST:
// 	case DAG:
// 		forlist(r.u.list, evalone, st, 0);
// 		return;
// 
// 	case NODE:
// 		if(r.external)
// 		{
// 			// Интересуемся только определениями
// 			return;
// 		}
// 
// 		switch(nodeverb(r, st->verbs))
// 		{
// 		case FENV:
// 			fenv(r, st);
// 			return;
// 
// 		case FPUT:
// 			fput(r, st);
// 			return;
// 
// 		case FOUT:
// 			fout(r, st);
// 			return;
// 
// 		default:
// 			if(!knownverb(r, st->escape))
// 			{
// 				// Если не запрещено пройти внутрь узла
// 				eval(nodeattribute(r), st);
// 			}
// 
// 			return;
// 		}
// 
// 	default:
// 		assert(0);
// 	}
// }
// 
// static const char *const sysverbs[] =
// {
// 	"FIn", "Nth",
// 	"F", "FEnv", "FOut",
// 	"R", "Rip", "Done", "Go",
// 	NULL
// };
// 
// void formeval(
// 	Array *const U,
// 	Array *const area,
// 	Array *const activity,
// 	const Ref dag, const Array *const escape,
// 	const Array *const envmarks,
// 	const Array *const areamarks,
// 	const Array *const typemarks)
// {
// 	FEState st =
// 	{
// 		.U = U,
// 		.area = area,
// 		.escape = escape,
// 		.verbs = newverbmap(U, 0, verbs),
// 		.typeverbs = newverbmap(U, 0, ES("T", "TEnv")),
// 		.formmarks = newkeymap(),
// 		.typemarks = typemarks,
// 		.envmarks = envmarks,
// 		.sysverbs = newverbmap(U, 0, sysverbs),
// 		.areamarks = areamarks,
// 		.activity = activity
// 	};
// 
// 	eval(dag, &st);
// 
// 	freekeymap((Array *)st.sysverbs);
// 	freekeymap(st.formmarks);
// 	freekeymap((Array *)st.typeverbs);
// 	freekeymap((Array *)st.verbs);
// }
