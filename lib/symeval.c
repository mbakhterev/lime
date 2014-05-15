#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGS 1

// #define DBGFLAGS (DBGS)

#define DBGFLAGS (0)

// В основном, списано из typeeval. S ведут себя похожим на TEnv образом

// typedef struct
// {
// 	Array *const U;
// 	Array *const symbols;
// 	Array *const symmarks;
// 
// 	const Array *const escape;
// 	const Array *const envmarks;
// 	const Array *const typemarks;
// 
// 	const Array *const verbs;
// } EState;
// 
// static void eval(const Ref N, EState *const E);
// 
// static int evalone(List *const l, void *const ptr)
// {
// 	assert(l);
// 	assert(ptr);
// 
// 	eval(l->ref, ptr);
// 
// 	return 0;
// }
// 
// #define SNODE 0
// 
// static const char *const verbs[] =
// {
// 	[SNODE] = "S",
// 	NULL
// };
// 
// // static const Binding *getexisting(

static Ref getexisting(
	const Array *const env, Array *const U, const Ref key)
{
	List *const l
		= tracepath(
			env, U,
			readpack(U, strpack(0, "ENV")),
			readpack(U, strpack(0, "parent")));
	
	const Ref K = decorate(markext(key), U, DSYM);
	const Binding *const b = pathlookup(l, K, NULL);
	freeref(K);

	freelist(l);

	if(b && b->ref.code == SYM)
	{
// 		// Ключ больше не нужен, можно от него избавиться
// 		freeref(key);
		return b->ref;
	}

	return reffree();
}

static Ref setnew(
	Array *const env, Array *const U, Array *const symbols,
	const Ref key, const Ref typeref)
{
	if(typeref.code != TYPE)
	{
		return reffree();
	}

	const Ref name = forkref(key, NULL);
	const Ref id = reflist(RL(envid(U, env), name));

	const unsigned sid = mapreadin(symbols, id);
	Binding *const symb = (Binding *)bindingat(symbols, sid);

	if(!symb)
	{
		// name будет освобождена автоматически
		freeref(id);
		return reffree();
	}

	symb->ref = typeref;

// 	const unsigned symnum = symb - (Binding *)symbols->u.data;
// 	assert(symnum + 1 == symbols->count);

	const Ref K = decorate(markext(name), U, DSYM);
	Binding *const b = (Binding *)bindingat(env, mapreadin(env, K));

	if(!b)
	{
		// id и name освобождать не надо. Они сохренены в symbols.
		// Вместе с ней и будут освобождены

		freeref(K);
		return reffree();
	}

// 	b->ref = refnum(symnum);
// 	return b->ref;

	b->ref = refsym(sid);
	return b->ref;
}

// static void snode(const Ref N, EState *const E)
// {
// // 	// Если мы этот узел уже обработали, то не обращаем на него внимания.
// // 	// Так? В качестве эксперимента пробуем
// // 
// // 	if(refmap(E->symmarks, N).code == FREE)
// // 	{
// // 		return;
// // 	}
// 
// 	const Ref r = nodeattribute(N);
// 	if(r.code != LIST)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": expecting attribute list",
// 			atombytes(atomat(E->U, nodeverb(N, NULL))));
// 
// 		return;
// 	}
// 
// 	const unsigned len = listlen(r.u.list);
// 
// 	if(len < 1 || 2 < len)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": expecting 1 or 2 attributes",
// 			atombytes(atomat(E->U, nodeverb(N, NULL))));
// 
// 		// Это символично, конечно же. Вообще, надо возвращать именно
// 		// ошибку и делать что-то в стиле монад, но на Си это сложно.
// 		// Поэтому оставляем до следующей версии
// 
// 		return;
// 	}
// 
// 	Ref R[len];
// 	writerefs(r.u.list, R, len);
// 
// 	if(!isbasickey(R[0]))
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": expecting 1st attributes to be basic key",
// 			atombytes(atomat(E->U, nodeverb(N, NULL))));
// 
// 		return;
// 	}
// 
// 	const Ref typeref = len == 2 ? refmap(E->typemarks, R[1]) : reffree();
// 
// 	Array *const env = envmap(E->envmarks, N);
// 
// 	if(!env)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": no environment definition",
// 			atombytes(atomat(E->U, nodeverb(N, NULL))));
// 		
// 		return;
// 	}
// 
// 	const Ref id
// 		= (len == 1) ? getexisting(env, E->U, R[0])
// 		: (len == 2) ? setnew(env, E->U, E->symbols, R[0], typeref)
// 		: reffree();
// 
// 	if(DBGS & DBGFLAGS)
// 	{
// 		char *const name = strref(E->U, NULL, R[0]);
// 
// 		DBG(DBGS, "(len: %u) (name: %s) (env: %p) (id: %u %u)",
// 			len, name, (void *)env, id.code, id.u.number);
// 
// 
// 		free(name);
// 	}
// 
// 	if(id.code == FREE)
// 	{
// 		char *const strkey = strref(E->U, NULL, R[0]);
// 		
// 		item = nodeline(N);
// 		ERR("node \"%s\": can't %s symbol for key: %s",
// 			atombytes(atomat(E->U, nodeverb(N, NULL))),
// 			(len == 1) ? "locate" : "allocate",
// 			strkey);
// 
// 		free(strkey);
// 		return;
// 	}
// 
// 	tunerefmap(E->symmarks, N, id);
// }

void dosnode(
	Core *const C, Marks *const M, const Ref N, const unsigned envid)
{
	Array *const U = C->U;
	Array *const E = C->E;
	Array *const S = C->S;
	Array *const marks = M->marks;

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

	const Ref key = len > 0 ?
		  simplerewrite(R[0], marks, M->areamarks)
		: reffree();

	const Ref type = len > 1 ? refmap(marks, R[1]) : reffree();

	if(DBGFLAGS & DBGS)
	{
		char *const rstr = strref(U, NULL, R[0]);
		char *const kstr = strref(U, NULL, key);
		DBG(DBGS, "(R 0 = %s) (key = %s)", rstr, kstr);
		free(kstr);
		free(rstr);
	}

	if(len < 1 || 2 < len
		|| !isbasickey(key)
		|| (len == 2 && type.code != TYPE))
	{
		freeref(key);

		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	Array *const env = envkeymap(E, refenv(envid));
	const Ref id
		= len == 1 ? getexisting(env, U, key)
		: len == 2 ? setnew(env, U, S, key, type)
		: reffree();

	if(id.code == FREE)
	{
		char *const kstr = strref(U, NULL, key);
		freeref(key);

		item = nodeline(N);
		ERR("node \"%s\": can't %s symbol for key: %s",
			nodename(U, N),
			len == 1 ? "locate" : "allocate",
			kstr);

		free(kstr);
		return;
	}

	freeref(key);
	tunerefmap(marks, N, id);
}

// static void evalnode(const Ref N, EState *const E)
// {
// 	switch(nodeverb(N, E->verbs))
// 	{
// 	case SNODE:
// 		snode(N, E);
// 		break;
// 	}
// }
// 
// static void eval(const Ref N, EState *const E)
// {
// 	switch(N.code)
// 	{
// 	case NUMBER:
// 	case ATOM:
// 	case TYPE:
// 		return;
// 	
// 	// Для текущих алгоритмов нет разницы между DAG и LIST
// 
// 	case LIST:
// 	case DAG:
// 		forlist(N.u.list, evalone, E, 0);
// 		return;
// 	
// 	case NODE:
// 		if(N.external)
// 		{
// 			return;
// 		}
// 
// 		if(knownverb(N, E->verbs))
// 		{
// 			evalnode(N, E);
// 			return;
// 		}
// 
// 		if(!knownverb(N, E->escape))
// 		{
// 			eval(nodeattribute(N), E);
// 		}
// 
// 		return;
// 	
// 	default:
// 		assert(0);
// 		
// 	}
// }
// 
// void symeval(
// 	Array *const U,
// 	Array *const symbols,
// 	Array *const symmarks,
// 	const Ref dag, const Array *const escape,
// 	const Array *const envmarks, const Array *typemarks)
// {
// 	EState E =
// 	{
// 		.U = U,
// 		.symbols = symbols,
// 		.symmarks = symmarks,
// 
// 		.typemarks = typemarks,
// 		.envmarks = envmarks,
// 		.escape = escape,
// 
// 		.verbs = newverbmap(U, 0, verbs)
// 	};
// 
// 	eval(dag, &E);
// 
// 	freekeymap((Array *)E.verbs);
// }
// 
// Ref symid(const Array *const symmarks, const Ref N)
// {
// 	const Ref id = refmap(symmarks, N);
// 	assert(id.code == NUMBER);
// 	return id;
// }

Ref symname(const Array *const symbols, const Ref id)
{
	assert(symbols);
	assert(id.code == SYM && id.u.number < symbols->count);

	const Binding *const b = (Binding *)symbols->u.data + id.u.number;
	assert(b->key.code == LIST && b->ref.code == TYPE);

	const unsigned len = listlen(b->key.u.list);
	const Ref R[len];
	assert(len == 2
		&& writerefs(b->key.u.list, (Ref *)R, len) == len
		&& R[0].code == ENV);

	return markext(R[1]);
}

Ref symtype(const Array *const symbols, const Ref id)
{
	assert(symbols);
	assert(id.code == SYM && id.u.number < symbols->count);

	const Binding *const b = (Binding *)symbols->u.data + id.u.number;
	assert(b->ref.code == TYPE);
	return b->ref;
}
