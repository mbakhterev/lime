#include "construct.h"

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
		break;
	
	default:
		assert(0);
	}

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

Ref refptr(void *const p)
{
	return (Ref) { .code = PTR, .u.pointer = p, .external = 0 };
}

Ref refnode(List *const exp)
{
	assert(isnode(exp));

	// Внимание на 1
	return (Ref) { .code = NODE, .u.list = exp, .external = 1 };
}

Ref reflist(List *const l)
{
	return (Ref) { .code = LIST, .u.list = l, .external = 0 };
}

Ref refform(Form *const f)
{
	return (Ref) { .code = FORM, .u.form = f, .external = 0 };
}

Ref reftab(Array *const a)
{
	assert(a && a->code == KEYTAB);
	return (Ref) { .code = KEYTAB, .u.array = a, .external = 0 };
}

Ref refenv(Environment *const e)
{
	return (Ref) { .code = ENV, .u.environment = e, .external = 0 };
}

Ref refctx(Context *const c)
{
	return (Ref) { .code = CTX, .u.context = c, .external = 0 };
}

static Ref setbit(const Ref r, const unsigned bit)
{
	// switch для аккуратности, потому что во многих случаях Ref не должна
	// быть внешней. Случая сейчас вообще всего два: форма и список (для
	// ключей и аргументов .FIn)

	switch(r.code)
	{
	case NODE:
	case LIST:
	case FORM:
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

Ref markext(const Ref r)
{
	return setbit(r, 1);
}

Ref cleanext(const Ref r)
{
	return setbit(r, 0);
}


