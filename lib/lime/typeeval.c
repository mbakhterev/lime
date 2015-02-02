#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGNMT 1
#define DBGTENV 2
#define DBGGE 4

// #define DBGFLAGS (DBGNMT)
// #define DBGFLAGS (DBGTENV | DBGGE)

#define DBGFLAGS 0

void dotnode(
	Core *const C, Marks *const M, const Ref N)
{
	Array *const T = C->T;
	const Array *const U = C->U;
	const Array *const V = C->verbs.system;

	Array *const marks = M->marks;

	// Превращаем атрибуты узла в выражение в текущем контексте обработки
	const Ref key = simplerewrite(nodeattribute(N), marks, V);

	// Полученное выражение должно быть формулой для типа

	if(!istypekey(key))
	{
		freeref(key);

		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	// Присваиваем выражению номер типа, которое оно задаёт. В случае
	// необходимости typeenummap запишет новое выражение в таблицу типов

	const unsigned typeid = typeenummap(T, key);
	freeref(key);

	tunerefmap(marks, N, reftype(typeid));
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

	const Ref K = decorate(markext(key), U, DTYPE); 
	const Binding *const b = pathlookup(l, K, NULL);
	freeref(K);

	freelist(l);

	if(b && b->ref.code == TYPE)
	{
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
	Binding *const b = (Binding *)bindingat(env, mapreadin(env, K));

	if(!b)
	{
		freeref(K);
		return -1;
	}

	assert(b->ref.code == FREE);

	b->ref = typeref;
	return typeref.u.number;
}

void dotenv(
	Core *const C, Marks *const M, const Ref N, const unsigned envid)
{
	Array *const U = C->U;
	Array *const E = C->E;
	const Array *const V = C->verbs.system;

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

	DBG(DBGTENV, "len = %u", len);

	const Ref key = len > 0 ? simplerewrite(R[0], marks, V) : reffree();
	const Ref T = len > 1 ? refmap(marks, R[1]) : reffree();

	if(len < 1 || 2 < len
		|| !isbasickey(key)
		|| (len == 2 && T.code != TYPE))
	{
		freeref(key);

		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	Array *const env = envkeymap(E, refenv(envid));
	DBG(DBGTENV, "env = %p", (void *)env);

	// Считаем, что в случае успешного завершения процедур key будет
	// оприходован, и его можно будет не освобождать

	const unsigned typeid
		= len == 1 ? getexisting(env, U, key)
		: len == 2 ? setnew(env, U, key, T)
		: -1;
	
	if(typeid == -1)
	{
		char *const strkey = strref(U, NULL, key);
		freeref(key);

		item = nodeline(N);
		ERR("node \"%s\": can't %s type for key: %s",
			nodename(U, N),
			len == 1 ? "locate" : "allocate",
			strkey);

		free(strkey);

		return;
	}

	freeref(key);
	tunerefmap(marks, N, reftype(typeid));
}

// static void tdef(const Ref N, EState *const E)
// {
// 	const Ref r = nodeattribute(N);
// 	const unsigned len = r.code == LIST ? listlen(r.u.list) : -1;
// 	if(len != 2)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": wrong attribute structure",
// 			atombytes(atomat(E->U, nodeverb(N, NULL))));
// 
// 		return;
// 	}
// 
// 	const Ref R[len];
// 	assert(writerefs(r.u.list, (Ref *)R, len) == len);
// 
// 	const Ref type = refmap(E->typemarks, R[0]);
// 	if(type.code != TYPE)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": attribute 0 should evaluate to typeref",
// 			atombytes(atomat(E->U, nodeverb(N, NULL))));
// 
// 		return;
// 	}
// 
// 	const Ref def = exprewrite(R[1], E->typemarks, E->typeverbs);
// 	if(!issignaturekey(def))
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": attribute 1 should evaluate to signature",
// 			atombytes(atomat(E->U, nodeverb(N, NULL))));
// 
// 		freeref(def);
// 		return;
// 	}
// 
// 	Binding *const b = (Binding *)typeat(E->types, type.u.number);
// 	if(b->ref.code != FREE)
// 	{
// 		item = nodeline(N);
// 		ERR("node \"%s\": can't redefine type",
// 			atombytes(atomat(E->U, nodeverb(N, NULL))));
// 
// 		freeref(def);
// 		return;
// 	}
// 
// 	b->ref = def;
// }
// 

void dotdef(Core *const C, const Ref N, const Marks *const M)
{
	Array *const T = C->T;
	const Array *const U = C->U;
	const Array *const V = C->verbs.system;

	const Array *const marks = M->marks;

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

	const Ref type = len > 0 ? refmap(marks, R[0]) : reffree();
	const Ref def = len > 1 ? simplerewrite(R[1], marks, V) : reffree();

	if(len != 2 || type.code != TYPE || !issignaturekey(def))
	{
		freeref(def);

		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	Binding *const b = (Binding *)typeat(T, type);
	if(b->ref.code != FREE)
	{
		freeref(def);

		item = nodeline(N);
		ERR("node \"%s\": can't redefine type", nodename(U, N));
		return;
	}

	b->ref = def;
}

const Binding *typeat(const Array *const types, const Ref id)
{
	assert(id.code == TYPE && id.u.number < types->count);
	return (Binding *)types->u.data + id.u.number;
}
