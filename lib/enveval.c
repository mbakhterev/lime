#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGEVE	1
#define DBGAL	2
#define DBGAE	4
#define DBGDED	8

// #define DBGFLAGS (DBGEVE | DBGAL)
// #define DBGFLAGS (DBGEVE)
// #define DBGFLAGS (DBGAE)
// #define DBGFLAGS (DBGDED)

#define DBGFLAGS 0

// typedef struct
// {
// 	Array *const U;
// 
// 	// В атрибуты каких узлов ходить не следует и какие из узлов следует
// 	// помечать
// 
// 	const Array *const escape;
// 	const Array *const markit;
// 
// 	// Результат работы: дерево новых окружений вписанное в текущий env и
// 	// метки о том, в каких окружениях находятся те или иные узлы
// 
// 	Array *const env;
// 	Array *const envmarks;
// 
// 	// Вспомогательные отображения
// 
// 	Array *const defs;
// 	Array *const envdefs;
// 
// 	// Отображение для узнавания E и Env
// 	const Array *const recode;
// } EState;
// 
// #define ENODE 0
// #define ENV 1
// 
// static const char *const enverbs[] =
// {
// 	[ENODE] = "E",
// 	[ENV] = "Env",
// 	NULL
// };
// 
// static const List *attrlist(const Ref r, Array *const U)
// {
// 	if(DBGFLAGS & DBGAL)
// 	{
// 		char *const c = strlist(U, r.u.list);
// 		DBG(DBGAL, "%s", c);
// 		free(c);
// 	}
// 
// 	const Ref l = nodeattribute(r);
// 
// 	if(l.code == LIST)
// 	{
// 		return l.u.list;
// 	}
// 
// 	ERR("line: %u. node \"%s\": attribute should be a list",
// 		nodeline(r), atombytes(atomat(U, nodeverb(r, NULL))));
// 
// 	return NULL;
// }
// 
// // static Ref newitem(Array *const U)
// // {
// // 	return refkeymap(newkeymap());
// // }
// 
// static Array *newtarget(Array *const U, const Array *const map)
// {
// 	return newkeymap();
// }
// 
// static Array *nextpoint(Array *const U, const Array *const map)
// {
// 	return (Array *)map;
// }
// 
// static unsigned maypass(Array *const U, const Array *const map)
// {
// 	return !0;
// }
// 
// static void mkenv(
// 	Array *const U, const Ref r, const Array *const recode,
// 	Array *const env, Array *const envdefs, Array *const envmarks)
// {
// 	const List *const l = attrlist(r, U);
// 	const unsigned len = listlen(l);
// 	Ref R[len];
// 	assert(writerefs(l, R, len) == len);
// 
// 	if(DBGFLAGS & DBGEVE)
// 	{
// 		char *const c = strlist(U, l);
// 		DBG(DBGEVE, "(len list) = (%u %s)", len, c);
// 		free(c);
// 	}
// 
// 	// Проверим, что первые два аргумента нужного формата
// 
// 	if(len < 2 || len > 3
// 		|| !isbasickey(R[0]) || R[1].code != LIST || !isbasickey(R[1]))
// 	{
// 		ERR("line: %u. node: \"%s\": wrong attribute format",
// 			nodeline(r),
// 			atombytes(atomat(U, nodeverb(r, NULL))));
// 
// 		return;
// 	}
// 
// 	const Ref path = readtoken(U, "ENV");
// 	Array *newenv = NULL;
// 
// 	switch(len)
// 	{
// 	case 2:
// 	{
// // 		newenv = makepath(env, U, R[0], R[1].u.list, reffree());
// 		newenv
// 			= makepath(env, U,
// 				path, R[1].u.list, reffree(),
// 				maypass, newtarget, nextpoint);
// 					
// 		assert(newenv && newenv->code == MAP);
// 
// 		break;
// 	}
// 
// 	case 3:
// 	{
// 		// Третьим атрибутом должна быть ссылка на уже оценённый
// 		// .E-узел. Проверяем
// 
// 		if(!isnode(R[2])
// 			|| nodeverb(r, recode) != ENODE
// 			|| !(newenv = envmap(envdefs, R[2])))
// 		{
// 			ERR("line: %u. node: \"%s\": 3rd ref isn't known .E",
// 				nodeline(r),
// 				atombytes(atomat(U, nodeverb(r, NULL))));
// 		}
// 
// // 		const Array *const t
// // 			= makepath(env, U, R[0], R[1].u.list,
// // 				markext(refkeymap(newenv)));
// 
// 		const Array *const t
// 			= makepath(env, U,
// 				path, R[1].u.list, refkeymap(newenv),
// 				maypass, newtarget, nextpoint);
// 		
// 		if(!t)
// 		{
// 			item = nodeline(r);
// 			ERR("node \"%s\": can't link environments",
// 				nodename(U, r));
// 		}
// 
// 		assert(t == newenv);
// 
// 		break;
// 	}
// 
// 	default:
// 		assert(0);
// 	}
// 
// 	// Тут надо запомнить, чему равно текущее выражение с .E
// 
// 	tuneenvmap(envdefs, r, newenv);
// 	tuneenvmap(envmarks, r, newenv);
// }
// 
// static void assignenv(
// 	Array *const U, const Ref r, const Array *const recode,
// 	Array *const defs, Array *const envdefs)
// {
// 	const List *const l = attrlist(r, U);
// 	const unsigned len = listlen(l);
// 	Ref R[len];
// 	assert(writerefs(l, R, len) == len);
// 
// 	DBG(DBGAE, "(len ((R 0).verb local) env) = (%u (\"%s\" %u) %p)",
// 		len,
// 		atombytes(atomat(U, nodeverb(R[0], NULL))),
// 		nodeverb(R[0], recode),
// 		(void *)envmap(envdefs, R[0]));
// 	
// 	DBG(DBGAE, "((R 1).verb def (R 1).isnode) = (\"%s\" %u %u)",
// 		atombytes(atomat(U, nodeverb(R[1], NULL))),
// 		setmap(defs, R[1]),
// 		isnode(R[1]));
// 
// 	// Проверим структуру аргументов и вычленим в случае успеха требуемое
// 	// окружение
// 
// 	Array *env = NULL;
// 
// 	if(len != 2
// 		|| !(nodeverb(R[0], recode) == ENODE
// 			&& (env = envmap(envdefs, R[0])))
// 		|| !(nodeverb(R[1], recode) == -1
// 			&& setmap(defs, R[1])))
// 	{
// 		ERR("line: %u. node: \"%s\": wrong attribute structure",
// 			nodeline(r),
// 			atombytes(atomat(U, nodeverb(r, NULL))));
// 
// 		return;
// 	}
// 
// 	// Если проверки пройдены, то регистрируем (напоминание: tuneenvmap
// 	// сведётся к refmap, которая для своих аргументов делает markext)
// 
// 	tuneenvmap(envdefs, R[1], env);
// }
// 
// static int makeassignone(List *const l, void *const ptr)
// {
// 	assert(l);
// 	assert(ptr);
// 
// 	const Ref r = l->ref;
// 	EState *const st = ptr;
// 
// 	if(r.code != NODE || r.external)
// 	{
// 		// Если это не определение узла, то нам не особо интересно.
// 		// Переходим дальше
// 
// 		return 0;
// 	}
// 
// 	// Узел определён. Отмечаем это
// 	tunesetmap(st->defs, r);
// 
// 	switch(nodeverb(r, st->recode))
// 	{
// 	case ENODE:
// 		mkenv(st->U, r, st->recode, st->env, st->envdefs, st->envmarks);
// 		break;
// 
// 	case ENV:
// 		assignenv(st->U, r, st->recode, st->defs, st->envdefs);
// 		break;
// 	}
// 
// 	return 0;
// }
// 
// static int markone(List *const l, void *const ptr)
// {
// 	assert(l);
// 	assert(ptr);
// 	EState *const st = ptr;
// 	const Ref r = l->ref;
// 
// 	if(r.code != NODE || r.external)
// 	{
// 		return 0;
// 	}
// 
// 	// Здесь мы имеем дело с определённым узлом. Но если нас не просили его
// 	// отмечать, то не отмечаем
// 
// 	if(!knownverb(r, st->markit))
// 	{
// 		return 0;
// 	}
// 
// 	// Узел надо отмечать. Возможны два варианта: для узла явно задано
// 	// окружение через envdefs (результат обработки Env на предыдущем
// 	// шаге), либо ничего в envdefs не сказано. В первом случае назначаем
// 	// окружение из envdefs, во втором - текущее.
// 
// 	Array *const env = envmap(st->envdefs, r);
// 
// 	if(env)
// 	{
// 		tuneenvmap(st->envmarks, r, env);
// 		return 0;
// 	}
// 
// 	tuneenvmap(st->envmarks, r, st->env);
// 	return 0;
// }
// 
// static int diveone(List *const l, void *const ptr)
// {
// 	assert(l);
// 	assert(ptr);
// 	EState *const st = ptr;
// 	const Ref r = l->ref;
// 
// 	switch(r.code)
// 	{
// 	case NUMBER:
// 	case ATOM:
// 	case TYPE:
// 		// Не интересные варианты
// 		break;
// 	
// 	case LIST:
// 		// Погружаемся этой же процедурой глубже
// 
// 		forlist(r.u.list, diveone, st, 0);
// 		break;
// 	
// 	case NODE:
// 		// Здесь нам интересны только определения узлов, в которые нам
// 		// можно входить, и которые отличаются от .E и .Env, которые уже
// 		// обработаны
// 
// 		if(!r.external
// 			&& !knownverb(r, st->escape)
// 			&& nodeverb(r, st->recode) == -1)
// 		{
// 			// Сначала надо выяснить, отнесён ли узел при помощи
// 			// .Env к какому-нибудь окружению, отличному от
// 			// текущего. Ну и вызвать enveval рекурсивно
// 
// 			Array *const t = envmap(st->envdefs, r);
// 
// 			enveval(st->U,
// 				t ? t : st->env, st->envmarks,
// 				nodeattribute(r), st->escape, st->markit);
// 		}
// 
// 		break;
// 	
// 	default:
// 		assert(0);
// 	}
// 
// 
// 	return 0;
// }
// 
// static void evalcore(
// 	Array *const U,
// 	Array *const env,
// 	Array *const envmarks,
// 	List *const l, const Array *const escape, const Array *const markit)
// {
// 	assert(U);
// 	assert(env && env->code == MAP);
// 	assert(envmarks && env->code == MAP);
// 
// 	EState st =
// 	{
// 		.U = U,
// 		.env = env,
// 		.envmarks = envmarks,
// 		.escape = escape,
// 		.markit = markit,
// 		.defs = newkeymap(),
// 		.recode = newverbmap(U, 0, enverbs),
// 		.envdefs = newkeymap()
// 	};
// 
// 	// Первая стадия оценки окружений. Проходим по текущим атрибутам,
// 	// формируем окружения и определяем окружения при помощи Env
// 
// 	forlist(l, makeassignone, &st, 0);
// 
// 	// Вторая стадия оценки окружений. Размечаем узлы
// 
// 	forlist(l, markone, &st, 0);
// 
// 	// Третья стадия оценки окружений. Проходим по текущим атрибутам и
// 	// рекурсивно вызываем enveval с учётом накопленной об окружениях
// 	// информации
// 
// 	forlist(l, diveone, &st, 0);
// 
// 	freekeymap(st.envdefs);
// 	freekeymap((Array *)st.recode);
// 	freekeymap(st.defs);
// }
// 
// void enveval(
// 	Array *const U,
// 	Array *const env,
// 	Array *const envmarks,
// 	const Ref r, const Array *const escape, const Array *const markit)
// {
// 	switch(r.code)
// 	{
// 	// Выражение может быть скучным и не давать никакой информации (в случае
// 	// с TYPE интереса больше, но раз там уже номер типа, то всё уже
// 	// рассчитано)
// 
// 	case NUMBER:
// 	case ATOM:
// 		return;
// 
// // 	case TYPE:
// // 		break;
// 
// 	// Интересные варианты развития событий. Текущие алгоритмы таковы, что
// 	// они не делают разницу между DAG и LIST
// 
// 	case LIST:
// 	case DAG:
// 		// FIXME: возможно, имеет смысл написать assert(!r.external)
// 
// 		evalcore(U, env, envmarks, r.u.list, escape, markit);	
// 		return;
// 
// 	case NODE:
// 	{
// 		DL(nl, RS(r));
// 		evalcore(U, env, envmarks, nl.u.list, escape, markit);	
// 		return;
// 	}
// 	
// 	default:
// 		assert(0);
// 	}
// }

static unsigned isenode(const Ref N, const Array *const limeverbs)
{
	return N.code == NODE && nodeverb(N, limeverbs) == ENODE;
}

static unsigned isvalidtarget(const Ref N, const Array *const limeverbs)
{
	if(N.code != NODE)
	{
		return 0;
	}

	switch(nodeverb(N, limeverbs))
	{
	case -1:
		return !0;

	case SNODE:
	case TENV:
	case FENV:
		return !0;
	
	default:
		return 0;
	}

	return 0;
}

void doedef(
	Array *const envdefs, Array *const envkeep,
	const Ref N, const Core *const C)
{
	const Array *const U = C->U;

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

	const unsigned eok = isenode(R[0], C->verbs.system);
	const unsigned tok = isvalidtarget(R[1], C->verbs.system);

	DBG(DBGDED, "len eok tok = %u (%u: %s) %u",
		len, eok, nodename(U, R[0]), tok);

	if(len != 2 || !eok || !tok)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	// Запоминаем, что для узла R[1] окружение переопределяется узлом R[0]
	tunerefmap(envdefs, R[1], R[0]);

	// Если Env помечал не системный узел, то его надо оставить в графе.
	// Если системный, то нужно стереть, так как и системный узел будет
	// стёрт

	if(!knownverb(R[1], C->verbs.system))
	{
		tunesetmap(envkeep, N);
	}
}

static unsigned maypass(Array *const U, const Array *const map);
static Array *nextpoint(Array *const U, const Array *const map);
static Array *newtarget(
	Array *const U, const Array *const map, const Ref id, void *const E);

void doenode(Core *const C, Array *const marks, const Ref N, const unsigned env)
{
	Array *const U = C->U;
	Array *const E = C->E;

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

	const Ref t = len == 2 ? refmap(marks, R[1]): reffree();

	if(len < 1 || len > 2 || (len == 2 && t.code != ENV))
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	const Ref K = simplerewrite(R[0], marks);
	if(K.code != LIST || !isbasickey(K))
	{
		freeref(K);

		item = nodeline(N);
		ERR("node \"%s\": can't rewrite 1st argument to basic key list",
			nodename(U, N));
		return;
	}

	const Ref tmap
		= t.code == ENV ? refkeymap(envkeymap(E, t)) : reffree();

	if(len == 2 && tmap.code == FREE)
	{
		freeref(K);

		item = nodeline(N);
		ERR("node \"%s\": environment isn't evaluated", nodename(U, N));
		return;
	}

	// Здесь у нас есть путь к окружению. Ref-а нового окружения (или
	// (Ref.code == FREE) в случае его отсутсвия). Нужно раздобыть ещё
	// текущий root. Все проверки внутри envkeymap

	Array *const root = envkeymap(E, refenv(env));

	// Остаётся запустить makepath

	const Array *const target
		= makepath(
			root, U, readtoken(U, "ENV"), K.u.list, tmap,
			maypass, newtarget, nextpoint, E);

	if(!target)
	{
		char *const kstr = strref(U, NULL, K);
		freeref(K);

		ERR("node \"%s\": can't %s environment with key: %s",
			nodename(U, N),
			len == 1 ? "trace" : "bind",
			kstr);

		free(kstr);
		return;
	}

	assert(tmap.code == FREE || target == tmap.u.array);

	freeref(K);

	tunerefmap(marks, N, envid(U, target));
}

static Array *nextpoint(Array *const U, const Array *const map)
{
	return (Array *)map;
}

static unsigned maypass(Array *const U, const Array *const map)
{
	return !0;
}

static Array *newtarget(
	Array *const U, const Array *const map, const Ref id, void *const state)
{
	Array *const T = newkeymap();
	Array *const E = state;

	const Ref mappath
		= forkref(cleanext(envrootpath(E, envid(U, map))), NULL);
	assert(mappath.code == LIST);

	const Ref path = reflist(append(mappath.u.list, RL(forkref(id, NULL))));

	const unsigned tid = mapreadin(E, path);
	assert(tid != -1);
	((Binding *)bindingat(E, tid))->ref = refkeymap(T);

	setenvid(U, T, tid);

	// FIXME: небольшой хак, чтобы не менять интерфейс makepath, но иметь
	// возможность обрабатывать специальное имя "/", указывающее на корневое
	// окружение

	Array *const R = envkeymap(E, refenv(0));
	assert(linkmap(U, T,
		readtoken(U, "ENV"), readtoken(U, "/"), refkeymap(R)) == R);

	return T;
}

void setenvid(Array *const U, Array *const env, const unsigned id)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "ENVID")));
	Binding *const b = (Binding *)bindingat(env, bindkey(env, key));
	assert(b && b->ref.code == FREE);

	b->ref = refenv(id);
}

Ref envid(Array *const U, const Array *const env)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "ENVID")));
	const Binding *const b = bindingat(env, maplookup(env, key));
	assert(b && b->ref.code == ENV);

	return b->ref;
}

static const Binding *idtobind(const Array *const E, const Ref id)
{
	assert(id.code == ENV && id.u.number < E->count);
	const Binding *const b = bindingat(E, id.u.number);
	assert(b);
	return b;
}

Array *envkeymap(const Array *const E, const Ref id)
{
	const Binding *const B = idtobind(E, id);
	assert(iskeymap(B->ref));

	return B->ref.u.array;
}

Ref envrootpath(const Array *const E, const Ref id)
{
	const Binding *const B = idtobind(E, id);
	assert(B->key.code == LIST);

	return markext(B->key);
}
