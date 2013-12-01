#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGNMT 1
#define DBGTENV 2
#define DBGGE 4

// #define DBGFLAGS (DBGNMT)

#define DBGFLAGS (DBGTENV | DBGGE)

// #define DBGFLAGS 0

typedef struct
{
	Array *const U;
	Array *const types;
	Array *const typemarks;

	Array *const escape;
	Array *const envmarks;

	Array *const verbs;
	Array *const envverbs;
	Array *const typeverbs;
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

	const Binding *const b = pathlookup(l, key, NULL);

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

	Binding *const b = keymap(env, key);

	if(!b || b->ref.code != FREE)
	{
		return -1;
	}

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
	writerefs(attr.u.list, R, len);
	
	if(!isbasickey(R[0]))
	{
		item = nodeline(r);
		ERR("node \"%s\": expecting 1st attribute to be basic key",
			atombytes(atomat(E->U, nodeverb(r, NULL))));
	}

	// По входным выясняем, на что отображён второй аргумент, если он есть.
	// Он должен задавать какой-нибудь тип, это будет проверено в setnew

	const Ref typeref = refmap(len == 2 ? E->typemarks : NULL, R[1]);

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

	// Уточняем, что работаем с ключом для типа. FIXME: необходимость явно
	// делать forkref, оправдана ли?

	const Ref key = decorate(forkref(R[0], NULL), E->U, TYPE);

	DBG(DBGTENV, "%s", "decorated");

	const unsigned typeid
		= (len == 1) ? getexisting(env, E->U, key)
		: (len == 2) ? setnew(env, E->U, key, typeref)
		: -1;

	DBG(DBGTENV, "typeid = %u", typeid);
	
	freeref(key);
	
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

static void nominatenode(const Ref r, EState *const st)
{
	if(r.external)
	{
		// Ссылками не интересуемся. Нам важны только определения
		return;
	}

	switch(nodeverb(r, st->verbs))
	{
	case TNODE:
		tnode(r, st);
		break;

	case TENV:
		tenv(r, st);
		break;
		
//	case TENV:
//	{
// 		// Нас интересуют те .TEnv-ы, которые задают TVAR-ы, то есть
// 		// .TEnv-ы со списками атрибутов единичной длины. Остальные
// 		// .TEnv описывают другие сущности и мы их здесь не трогаем
// 
// 		const Ref attr = nodeattribute(r);
// 		if(attr.code != LIST)
// 		{
// 			return;
// 		}
// 
// 		const unsigned len = listlen(attr.u.list);
// 		if(len != 1)
// 		{
// 			return;
// 		}
// 
// 		// Добываем этот единственный элемент из списка. Он будет
// 		// служить нам основным ключом. Нужно проверить, что он подходит
// 		// под критерии basic
// 
// 		const Ref key = attr.u.list->ref;
// 
// 		if(!isbasickey(key))
// 		{
// 			return;
// 		}
// 		
// 		// Если ключ подходящий, дополнительно его декорируем и
// 		// добавляем в него ссылку на окружение, в котором видим текущий
// 		// .TEnv
// 
// 		const Ref augkey 
// 			= reflist(
// 				append(
// 					RL(refkeymap(envmap(envmarks, r))),
// 					decorate(key, U, TYPE)));
// 
// 		// Теперь надо найти этому выражению место в таблице типов. И
// 		// соответствующим образом расширить varmap
// 
// 		const unsigned id = typeenummap(types, augkey);
// 		freeref(augkey);
// 
// 		tunerefmap(varmap, r, reftvar(id));
// 	}
	}
}

static void nominate(const Ref r, EState *const E)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
		// В этом нет никакой информации о типах
		return;

	case LIST:
		// В списке могут встретится определения узлов .T и .TEnv
		// проходим его

		forlist(r.u.list, nominateone, E, 0);
		return;

	
	case NODE:
		nominatenode(r, E);
		return;
	
	default:
		assert(0);
	}
}

void typeeval(
	Array *const U,
	Array *const types,
	Array *const typemarks,
	const Ref dag, Array *const escape, Array *const envmarks)
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

	freekeymap(st.typeverbs);
	freekeymap(st.envverbs);
	freekeymap(st.verbs);
}
