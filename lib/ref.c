#include "construct.h"
#include "util.h"

#define DBGSB 1
#define DBGDM 2

// #define DBGFLAGS (DBGSB)

#define DBGFLAGS 0

#include <assert.h>

Ref reffree(void)
{
	return (Ref) { .code = FREE, .u.pointer = NULL, .external = 0 };
}

Ref refnat(const unsigned code, const unsigned n)
{
	switch(code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
	case SYM:
	case ENV:
		break;
	
	default:
		assert(0);
	}

	assert(n < MAXLEN);

	return (Ref) { .code = code, .u.number = n, .external = 0 };
}

Ref refnum(const unsigned n)
{
	return refnat(NUMBER, n);
}

Ref refatom(const unsigned n)
{
	return refnat(ATOM, n);
}

Ref reftype(const unsigned n)
{
	return refnat(TYPE, n);
}

Ref refsym(const unsigned n)
{
	return refnat(SYM, n);
}

Ref refenv(const unsigned n)
{
	return refnat(ENV, n);
}

Ref refptr(void *const p)
{
	// Внимание на 1
	return (Ref) { .code = PTR, .u.pointer = p, .external = 1 };
}

Ref refnode(List *const exp)
{
	assert(isnodelist(exp));

	// Внимание на 1
	return (Ref) { .code = NODE, .u.list = exp, .external = 1 };
}

Ref refdag(List *const exp)
{
	assert(isdaglist(exp));
	return (Ref) { .code = DAG, .u.list = exp, .external = 0 };
}

Ref reflist(List *const l)
{
	return (Ref) { .code = LIST, .u.list = l, .external = 0 };
}

Ref refform(List *const f)
{
	assert(isformlist(f));

	return (Ref) { .code = FORM, .u.list = f, .external = 0 };
}

Ref refkeymap(Array *const a)
{
	assert(a && a->code == MAP);
// 	return (Ref) { .code = MAP, .u.array = a, .external = 0 };

	// Внимание на 1
	return (Ref) { .code = MAP, .u.array = a, .external = 1 };
}

Ref refarea(Array *const a)
{
// 	assert(a && a->code == MAP);
// 	return (Ref) { .code = AREA, .u.array = a, .external = 0 };

	return refkeymap(a);
}

static Ref setbit(const Ref r, const unsigned bit)
{
	DBG(DBGSB, "(r.code r.u.pointer) = (%u %p)", r.code, r.u.pointer);

	// switch для аккуратности, потому что во многих случаях Ref не должна
	// быть внешней. Случая сейчас вообще всего два: форма и список (для
	// ключей и аргументов .FIn)

	switch(r.code)
	{
	case NODE:
	case LIST:
	case DAG:
	case FORM:
// 	case MAP:
// 	case AREA:
		break;
	
	default:
		assert(0);
	}

	return (Ref)
	{
		.code = r.code,
		.u.pointer = r.u.pointer,
		.external = bit != 0
	};
}

Ref cleanext(const Ref r)
{
	return setbit(r, 0);
}

static Ref skip(const Ref r)
{
	return r;
}

Ref markext(const Ref r)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
	case PTR:
// 	case FREE:
		return skip(r);

	case NODE:
	case LIST:
	case DAG:
	case FORM:
		return setbit(r, 1);
	
	case MAP:
// 	case AREA:
		assert(r.external);
		return skip(r);
	}

	DBG(DBGDM, "r.(code pointer) = (%u %p)", r.code, r.u.pointer);

	assert(0);
	return reffree();
}

Ref forkref(const Ref r, Array *const nodemap)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
	case SYM:
	case ENV:
		return r;
	
	case MAP:
		assert(r.external);
		return r;
	
	case NODE:
		return forknode(r, nodemap);

	case LIST:
	{
		Array *const M = nodemap;

		// Тонкости тут, действительно есть. Если (M != NULL), то список
		// надо в любом случае копировать. Потому что мы заранее не
		// знаем, есть ли там ссылки на узлы или нет. Если (M == NULL),
		// то можно действовать в зависимости от Ref.external

		if(M)
		{
			return reflist(transforklist(r.u.list, M));
		}

		if(!r.external)
		{
			return reflist(forklist(r.u.list));
		}

		return r;
	}

	case DAG:
		return forkdag(r);

	case FORM:
		return forkform(r);	

	default:
		assert(0);
	}

	return reffree();
}

void freeref(const Ref r)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
	case SYM:
	case ENV:
	case PTR:
	case FREE:
		break;

	case NODE:
		freenode(r);
		break;

	case FORM:
		freeform(r);
		break;

	case LIST:
	case DAG:
// 	case FORM:
		if(!r.external)
		{
			freelist(r.u.list);
		}

		break;
	
	case MAP:
// 		if(!r.external)
// 		{
// 			freekeymap(r.u.array);
// 		}
// 
// 		break;

		assert(r.external);
		break;
	
	default:
		assert(0);
	}
}
