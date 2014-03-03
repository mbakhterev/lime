#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGNMT 1
#define DBGTENV 2
#define DBGGE 4

// #define DBGFLAGS (DBGNMT)
// #define DBGFLAGS (DBGTENV | DBGGE)

#define DBGFLAGS 0

typedef struct
{
	Array *const U;

	// Результаты: таблица типов и метки для T и TEnv узлов с номерами
	// типов, которые они задают

	Array *const types;
	Array *const typemarks;

	// Информация об узлах: что не надо разбирать, и в какие окружения они
	// приписаны

	const Array *const escape;
	const Array *const envmarks;

	// verbmap-ы на разные случаи

	const Array *const verbs;
	const Array *const envverbs;
	const Array *const typeverbs;
} EState;

// Nominate - это от номинальных типов. Мы должны сопоставить .T и .TEnv
// выражениям их уникальные (с учётом внутренней структуры T и окружений для
// .TEnv) имена (индексы в таблице)

static void nominate(const Ref, EState *const);

static int nominateone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);

	nominate(l->ref, ptr);

	return 0;
}

#define TNODE 0
#define TENV 1
#define TDEF 2

static const char *const verbs[] =
{
	[TNODE] = "T",
	[TENV] = "TEnv",
	[TDEF] = "TDef",
	NULL
};

static Ref totypekey(const Ref r, EState *const st)
{
	const Ref envtmp = exprewrite(r, st->envmarks, st->envverbs);
	const Ref key = exprewrite(envtmp, st->typemarks, st->typeverbs);
	freeref(envtmp);
	return key;
}

static void tnode(const Ref r, EState *const st)
{
	// Смотрим, во что превращается атрибут узла, в текущем рабочем
	// контексте

	const Ref key = totypekey(nodeattribute(r), st);

	if(DBGFLAGS & DBGNMT)
	{
		char *const astr
			= strref(st->U, NULL, nodeattribute(r));

		char *const kstr
			= strref(st->U, NULL, key);
		
		DBG(DBGNMT, "\n\tattr:\t%s\n\tkey:\t%s", astr, kstr);

		free(astr);
		free(kstr);
	}

	// Полученное выражение должно быть описанием типа

	if(!istypekey(key))
	{
		item = nodeline(r);
		ERR("node \"%s\": wrong attribute structure",
			atombytes(atomat(st->U, nodeverb(r, NULL))));
	}

	// Присваиваем .T-выражению номер типа, которое оно задаёт. И
	// запоминаем это в таблице соответствия узлов значениям

	const unsigned typeid = typeenummap(st->types, key);
	freeref(key);

	tunerefmap(st->typemarks, r, reftype(typeid));
}

static unsigned getexisting(Array *const env, Array *const U, const Ref key)
{
	DBG(DBGGE, "%s", "entering");

	List *const l
		= tracepath(
			env, U,
			readpack(U, strpack(0, "ENV")),
			readpack(U, strpack(0, "parent")));
	
	if(DBGFLAGS & DBGGE)
	{
		char *const strpath = strlist(U, l);
		DBG(DBGGE, "path trace = %s", strpath);
		free(strpath);
	}

	const Ref K = decorate(dynamark(key), U, TYPE); 
	const Binding *const b = pathlookup(l, K, NULL);

	freeref(K);
	freelist(l);

	if(b)
	{
		assert(b->ref.code == TYPE);
		return b->ref.u.number;
	}

	return -1;
}

static unsigned setnew(
	Array *const env, Array *const U, const Ref key, const Ref typeref)
{
	if(typeref.code != TYPE)
	{
		return -1;
	}

	const Ref K = decorate(forkref(key, NULL), U, TYPE);
	Binding *const b = mapreadin(env, K);

	if(!b)
	{
		freeref(K);
		return -1;
	}

	assert(b->ref.code == FREE);

	b->ref = typeref;
	return typeref.u.number;
}

static void tenv(const Ref r, EState *const E)
{
	const Ref attr = nodeattribute(r);
	if(attr.code != LIST)
	{
		item = nodeline(r);
		ERR("node \"%s\": expecting attribute list",
			atombytes(atomat(E->U, nodeverb(r, NULL))));
	}

	const unsigned len = listlen(attr.u.list);
	DBG(DBGTENV, "len = %u", len);

	if(len < 1 || 2 < len)
	{
		item = nodeline(r);
		ERR("node \"%s\": expecting 1 or 2 attributes",
			atombytes(atomat(E->U, nodeverb(r, NULL))));
	}

	// R, кстати, от rands - операнды по-модному

	Ref R[len];
	assert(writerefs(attr.u.list, R, len) == len);
	
	if(!isbasickey(R[0]))
	{
		item = nodeline(r);
		ERR("node \"%s\": expecting 1st attribute to be basic key",
			atombytes(atomat(E->U, nodeverb(r, NULL))));
	}

	// По входным выясняем, на что отображён второй аргумент, если он есть.
	// Он должен задавать какой-нибудь тип, это будет проверено в setnew

	const Ref typeref = len == 2 ? refmap(E->typemarks, R[1]) : reffree();

	// Наконец, по входным данным выясняем, в каком окружении встретился
	// текущий текущий TEnv (ага, название r очень информативное). Это
	// отображение уже должно быть определено

	Array *const env = envmap(E->envmarks, r);

	DBG(DBGTENV, "env = %p", (void *)env);

	if(!env)
	{
		item = nodeline(r);
		ERR("node \"%s\": no environment definition for node",
			atombytes(atomat(E->U, nodeverb(r, NULL))));
	}

	const unsigned typeid
		= (len == 1) ? getexisting(env, E->U, R[0])
		: (len == 2) ? setnew(env, E->U, R[0], typeref)
		: -1;

	DBG(DBGTENV, "typeid = %u", typeid);
	
	if(typeid == -1)
	{
		char *const strkey = strref(E->U, NULL, R[0]);

		item = nodeline(r);
		ERR("node \"%s\": can't %s type for key: %s",
			atombytes(atomat(E->U, nodeverb(r, NULL))),
			(len == 1) ? "locate" : "allocate",
			strkey);

		free(strkey);
	}

	tunerefmap(E->typemarks, r, reftype(typeid));
}

static void tdef(const Ref N, EState *const E)
{
	const Ref r = nodeattribute(N);
	const unsigned len = r.code == LIST ? listlen(r.u.list) : -1;
	if(len != 2)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure",
			atombytes(atomat(E->U, nodeverb(N, NULL))));

		return;
	}

	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len);

	const Ref type = refmap(E->typemarks, R[0]);
	if(type.code != TYPE)
	{
		item = nodeline(N);
		ERR("node \"%s\": attribute 0 should evaluate to typeref",
			atombytes(atomat(E->U, nodeverb(N, NULL))));

		return;
	}

	const Ref def = exprewrite(R[1], E->typemarks, E->typeverbs);
	if(!issignaturekey(def))
	{
		item = nodeline(N);
		ERR("node \"%s\": attribute 1 should evaluate to signature",
			atombytes(atomat(E->U, nodeverb(N, NULL))));

		freeref(def);
		return;
	}

	Binding *const b = (Binding *)typeat(E->types, type.u.number);
	if(b->ref.code != FREE)
	{
		item = nodeline(N);
		ERR("node \"%s\": can't redefine type",
			atombytes(atomat(E->U, nodeverb(N, NULL))));

		freeref(def);
		return;
	}

	b->ref = def;
}

static void nominatenode(const Ref r, EState *const st)
{
	switch(nodeverb(r, st->verbs))
	{
	case TNODE:
		tnode(r, st);
		break;

	case TENV:
		tenv(r, st);
		break;

	case TDEF:
		tdef(r, st);
		break;
	}
}

static void nominate(const Ref r, EState *const E)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		// В этом нет никакой информации о типах
		return;

	// Текущие алгоритмы таковы, что они не различают DAG и LIST

	case LIST:
	case DAG:
		// В списке могут встретится определения узлов .T и .TEnv
		// проходим его

		forlist(r.u.list, nominateone, E, 0);
		return;

	case NODE:
		if(r.external)
		{
			// Ссылками не интересуемся. Нам важны только
			// определения

			return;
		}

		if(knownverb(r, E->verbs))
		{
			// Если это один из обрабатываемых узлов, то занимаемся
			// анализом и обработкой его атрибутов по специальному
			// алгоритму

			nominatenode(r, E);
			return;
		}

		// Если это определение просто некоторого узла, надо разбирать
		// его атрибуты в поисках значимого 

		if(!knownverb(r, E->escape))
		{
			// Но только если нас не попросили не совать туда свой
			// нос

			nominate(nodeattribute(r), E);
		}

		return;
	
	default:
		assert(0);
	}
}

void typeeval(
	Array *const U,
	Array *const types,
	Array *const typemarks,
	const Ref dag, const Array *const escape, const Array *const envmarks)
{
	EState st =
	{
		.U = U,
		.types = types,
		.typemarks = typemarks,
		.escape = escape,
		.envmarks = envmarks,
		.verbs = newverbmap(U, 0, verbs),
		.envverbs = newverbmap(U, 0, ES("E")),
		.typeverbs = newverbmap(U, 0, ES("T", "TEnv"))
	};

	nominate(dag, &st);

	freekeymap((Array *)st.typeverbs);
	freekeymap((Array *)st.envverbs);
	freekeymap((Array *)st.verbs);
}

const Binding *typeat(const Array *const types, const unsigned n)
{
	assert(n < types->count);
	return (Binding *)types->u.data + n;
}
