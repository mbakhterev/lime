#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGAR	1
#define DBGAE	(1 << 1)
#define DBGOUT	(1 << 2)

// #define DBGFLAGS (DBGAR | DBGAE)
// #define DBGFLAGS (DBGOUT)

#define DBGFLAGS 0

unsigned isarea(const Ref r)
{
	return iskeymap(r);
}

static void initreactor(Array *const U, Array *const area, const unsigned id)
{
	Array *const R = newkeymap();
	DL(fkey, RS(decoatom(U, DAREA), readtoken(U, "FORMS")));

	Binding *const fb = (Binding *)bindingat(R, bindkey(R, fkey));
	if(!fb || fb->ref.code != FREE)
	{
		freekeymap(R);
		assert(0);
		return;
	}

	fb->ref = reflist(NULL);

	DL(rkey, RS(readtoken(U, "R"), refnum(id)));
	assert(linkmap(U, area, readtoken(U, "CTX"), rkey, refkeymap(R)) == R);
}

unsigned areforms(const Ref r)
{
	return r.code == LIST && (r.u.list == NULL || isform(r.u.list->ref));
}

Ref *reactorforms(Array *const U, const Array *const reactor)
{
	DL(fkey, RS(decoatom(U, DAREA), readtoken(U, "FORMS")));

	const Binding *const b = bindingat(reactor, maplookup(reactor, fkey));
	assert(b);
	assert(areforms(b->ref));
	
	return (Ref *)&b->ref;
}

static Array *areareactorcore(
	Array *const U, const Array *const area, const unsigned id)
{
	DL(rkey, RS(readtoken(U, "R"), refnum(id)));

	DBG(DBGAR, "%p", (void *)area);
	if(DBGFLAGS & DBGAR)
	{
		dumpkeymap(1, stderr, 0, U, area, NULL);
	}

	// Можно было бы создавать реакторы по запросу, но для лучшего контроля
	// за ошибками требуем, чтобы это вхождение уже было и было корректным
	
	Array *const R
		= linkmap(U, (Array *)area, 
			readtoken(U, "CTX"), rkey, reffree());
// 	assert(R);
	return R;
}

Array *areareactor(Array *const U, const Array *const area, const unsigned id)
{
	Array *const R = areareactorcore(U, area, id);
	assert(R);
	return R;
}

void unlinkareareactor(Array *const U, Array *const area, const unsigned id)
{
	DL(rkey, RS(readtoken(U, "R"), refnum(id)));
	assert(unlinkmap(U, area, readtoken(U, "CTX"), rkey));
}

static void initdag(Array *const U, Array *const area)
{
	Array *const R = areareactor(U, area, 1);
	DL(dkey, RS(decoatom(U, DAREA), readtoken(U, "DAG")));

	Binding *const b = (Binding *)bindingat(R, bindkey(R, dkey));
	assert(b && b->ref.code == FREE);
	b->ref = cleanext(refdag(NULL));
}

Ref *areadag(Array *const U, const Array *const area)
{
	Array *const R = areareactor(U, area, 1);
	DL(dkey, RS(decoatom(U, DAREA), readtoken(U, "DAG")));

	Binding *const b = (Binding *)bindingat(R, maplookup(R, dkey));
	assert(b);
	assert(isdag(b->ref));

	return &b->ref;
}

static void initlinks(Array *const U, Array *const area)
{
	Array *const R = newkeymap();
	assert(linkmap(U, area,
		readtoken(U, "CTX"), readtoken(U, "LINKS"), refkeymap(R)) == R);
}

Array *arealinks(Array *const U, const Array *const area)
{
	Array *const L
		= linkmap(U, (Array *)area,
			readtoken(U, "CTX"), readtoken(U, "LINKS"), reffree());
	assert(L);
	return L;
}

unsigned unlinkarealinks(Array *const U, Array *const area)
{
	return unlinkmap(U, area, readtoken(U, "CTX"), readtoken(U, "LINKS"));
}

static void initsyntax(Array *const U, Array *const area, const Ref syntax)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "SYNTAX")));
	Binding *const b = (Binding *)bindingat(area, bindkey(area, key));
	assert(b && b->ref.code == FREE);

	b->ref = forkref(syntax, NULL);
}

Ref areasyntax(Array *const U, const Array *const area)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "SYNTAX")));
	const Binding *const b = bindingat(area, maplookup(area, key));
	if(b && b->ref.code != FREE)
	{
		return markext(b->ref);
	}

	return reffree();
}

static void initenv(Array *const U, Array *const area, const Array *const env)
{
	const Ref r = refkeymap((Array *)env);
	const Ref path = readtoken(U, "ENV");
	assert(linkmap(U, area, path, readtoken(U, "this"), r) == env);
}

Array *areaenv(Array *const U, const Array *const area)
{
	DBG(DBGAE, "%p", (void *)area);

	const Ref path = readtoken(U, "ENV");
	Array *const env
		= linkmap(U, (Array *)area,
			path, readtoken(U, "this"), reffree());

	assert(env);
	return env;
}

void unlinkareaenv(Array *const U, Array *const area)
{
	const Ref path = readtoken(U, "ENV");
	assert(unlinkmap(U, area, path, readtoken(U, "this")));
}

Array *newarea(Array *const U, const Ref syntax, const Array *const env)
{
	Array *const area = newkeymap();
	markactive(U, area, 1);
	initlinks(U, area);
	initenv(U, area, env);
	initsyntax(U, area, syntax);

	for(unsigned i = 0; i < 2; i += 1)
	{
		initreactor(U, area, i);
	}
	initdag(U, area);

	return area;
}

void markonstack(Array *const U, Array *const area, const unsigned on)
{
	if(on)
	{
		assert(linkmap(U, area,
			readtoken(U, "CTX"), readtoken(U, "STACK"),
				refkeymap(area)) == area);
		return;
	}

	assert(unlinkmap(U, area, readtoken(U, "CTX"), readtoken(U, "STACK")));
}

unsigned isonstack(Array *const U, const Array *const area)
{
	Array *const t
		= linkmap(U, (Array *)area,
			readtoken(U, "CTX"), readtoken(U, "STACK"), reffree());
	
	if(!t)
	{
		return 0;
	}

	assert(t == area);
	return !0;
}
void markontop(Array *const U, Array *const map, const unsigned on)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "TOP")));
	Binding *const b = (Binding *)bindingat(map, bindkey(map, key));
	assert(b);

	if(on)
	{
		// Нужно убедится, что установка флага не повторная
		assert(b->ref.code == FREE
			|| (b->ref.code == NUMBER && !b->ref.u.number));

		b->ref = refnum(!0);
		return;
	}

	// Сбросить флаг активности можно только с активной сущности
	assert(b->ref.code == NUMBER && b->ref.u.number != 0);
	b->ref = refnum(0);
}

unsigned isontop(Array *const U, const Array *const map)
{
	DL(key, RS(decoatom(U, DUTIL), readtoken(U, "TOP")));
	const Binding *const b = bindingat(map, maplookup(map, key));
	assert(!b || b->ref.code == NUMBER);

	return b != NULL && b->ref.u.number != 0;
}


static Ref ripdag(Array *const U, const Array *const area)
{
	Ref *const dptr = areadag(U, area);
	const Ref d = *dptr; 

	// У процедур rip разрушительная семантика. После их завершения R.0
	// будет уничтожен. Чтобы вместе с ним не был уничтожен граф, стираем в
	// R.0 ссылку на него
	*dptr = reffree();

	return d;
}

// Ref ripareadag(Array *const U, Array *const area)
// {
// 	assert(!isactive(U, area));
// 	const Ref d = ripdag(U, area);
// 	unlinkareareactor(U, area, 1);
// 
// 	assert(isdag(d));
// 	return d;
// }

typedef struct
{
	Array *const U;
	const Ref pattern;
	List *nodes;
} RState;

static int onout(const Binding *const b, void *const ptr)
{
	assert(b);
	assert(ptr);
	RState *const st = ptr;

	const Ref *const R[1];
	unsigned matched = 0;
	if(keymatch(st->pattern, &b->key, (const Ref **)R, 1, &matched))
	{
		assert(matched == 1);

		if(DBGFLAGS & DBGOUT)
		{
			char *const skey = strref(st->U, NULL, b->key);
			char *const sref = strref(st->U, NULL, b->ref);
			DBG(DBGOUT, "(key(%u): %s) (val(%u): %s)",
				b->key.external, skey,
				b->ref.external, sref);
			free(skey);
			free(sref);
		}

		assert(!b->key.external
			&& (b->ref.code == NODE || !b->ref.external));

		// Нашли выход. Нужно добавить в список узлов графа конструкцию
		// .FOut ((*R[0]; b->ref)). Семантика у процедур разрушительная,
		// но содержимое ключа может понадобится для реконструкции форм,
		// поэтому его копируем. Содержимое же выхода забираем и
		// вычёркиваем из R.0

		const unsigned verb = readtoken(st->U, "FOut").u.number;
		const Ref pair = reflist(RL(forkref(*R[0], NULL), b->ref));
		((Binding *)b)->ref = reffree();
		const Ref attr = reflist(RL(refnum(0), reflist(RL(pair))));

		// Имя исходного файла устанавливаем равным "FOut"
		st->nodes = append(st->nodes, RL(newnode(verb, attr, verb, 0)));
	}

	// Никуда глубже проходить не нужно
	return 0;
}

static Ref ripouts(Array *const U, Array *const area, const Ref dag)
{
	assert(isdag(dag));

	DL(outpattern, RS(decoatom(U, DOUT), reffree()));
	RState st =
	{
		.U = U,
		.pattern = markext(outpattern),
		.nodes = NULL
	};

	walkbindings(U, areareactor(U, area, 1), NULL, onout, &st);

	return refdag(append(dag.u.list, st.nodes));
}

static Ref ripforms(Array *const U, Array *const area, const Ref dag)
{
	assert(isdag(dag));
	return dag;
}

// Ref ripareaform(Array *const U, Array *const area)
// {
// 	assert(!isactive(U, area));
// 	const Ref f = ripforms(U, area, ripouts(U, area, ripdag(U, area)));
// 	unlinkareareactor(U, area, 1);
// 
// 	assert(isdag(f));
// 	return f;
// }

void riparea(
	Array *const U, Array *const area, Ref *const body, Ref *const trace)
{
	assert(!isactive(U, area) && !isareaconsumed(U, area));
	*body = ripforms(U, area, ripouts(U, area, refdag(NULL)));
	*trace = ripdag(U, area);
	unlinkareareactor(U, area, 1);
}

unsigned isareaconsumed(Array *const U, const Array *const area)
{
	return areareactorcore(U, area, 1) == NULL;
}
