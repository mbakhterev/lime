#include "construct.h"
#include "util.h"

#include <assert.h>
#include <string.h>

#define DBGFREE	1

//#define DBGFLAGS (DBGFREE)

#define DBGFLAGS 0


// Проверка компонент ключа на сравниваемость. Есть два типа ключей: базовые, в
// которых могут быть только ATOM, TYPE, NUMBER, и общие, в которых могут быть
// PTR и NODE. Эти все коды упорядочены так, что проверку на соответствие типа
// можно свести к сравнению. Базовые ключи - это Ref.code <= TYPE. Обобщённые -
// это Ref.code <= NODE. Всё это регулируется через limit, чтобы не плодить
// много разных функций. В LIST уходим рекурсивно

static unsigned iskey(const Ref, const unsigned limit);

static int iskeyitem(List *const k, void *const ptr)
{
	assert(k);
	assert(ptr);

	const unsigned limit = *(unsigned *)ptr;
	return iskey(k->ref, limit);	
}

unsigned iskey(const Ref k, const unsigned limit)
{
	switch(k.code)
	{
	case LIST:
		// !0 - это числовое значение для True. Оно, вроде, должно быть
		// равным 1. Но что-то я уже ни в чём не уверен, когда речь
		// заходит о GCC

		return forlist(k.u.list, iskeyitem, (void *)&limit, !0) == !0;
	}

	return k.code <= limit;
}

unsigned isbasickey(const Ref r)
{
	return iskey(r, TYPE);
}

static unsigned isgenerickey(const Ref r)
{
	return iskey(r, NODE);
}

static int cmplists(const List *const, const List *const);

static int cmpkeys(const Ref k, const Ref l)
{
	assert(k.code <= LIST);
	assert(l.code <= LIST);

	const unsigned r = cmpui(k.code, l.code);
	if(r)
	{
		return r;
	}

	if(k.code <= TYPE)
	{
		return cmpui(k.u.number, l.u.number);
	}

	if(k.code <= NODE)
	{
		return cmpptr(k.u.pointer, l.u.pointer);
	}

	return cmplists(k.u.list, l.u.list);
}

// Для поиска и сортировки важно, чтобы порядок был линейным. Построить его
// можно разными способами. Например, можно считать, что списки большей длины
// всегда > списков меньшей длинны, а внутри списков длины равной порядок
// устанавливается лексикографически. Но это не удобно для пользователя, ибо,
// скорее всего, близкие по смыслу списки будут и начинаться одинаково. В целях
// отладки полезно было бы ставить рядом друг с дружкой. Поэтому, порядок будет
// просто лексикографический. Это оправданное усложнение

static int cmplists(const List *const k, const List *const l)
{
	if(k == NULL)
	{
		return 1 - (l == NULL) - ((l != NULL) << 1);
	}

	if(l == NULL)
	{
		return -1 + (k == NULL) + ((k != NULL) << 1);
	}

	// current указатели
	const List *ck = k;
	const List *cl = l;

	unsigned r;

	do
	{
		ck = ck->next;
		cl = cl->next;
		r = cmpkeys(ck->ref, cl->ref);
	} while(!r && ck != k && cl != l);

	if(r) { return r; }

	// Если (r == 0), значит какой-то список закончился. Разбираем три
	// варианта:

	return 1 - (ck == k && cl == l) - ((ck == k && cl != l) << 1);
}

static int kcmp(const void *const D, const unsigned i, const void *const key)
{
	return cmpkeys(((const Binding *)D)[i].key, *(const Ref *)key);
}

static int icmp(const void *const D, const unsigned i, const unsigned j)
{
	return kcmp(D, i, &((const Binding *)D)[j].key);
}

Array *newkeymap(void)
{
	return newarray(MAP, sizeof(Binding), icmp, kcmp);
}

void freekeymap(Array *const env)
{
	assert(env && env->code == MAP);

	DBG(DBGFREE, "env->count: %u", env->count);

	Binding *const B = env->u.data;

	for(unsigned i = 0; i < env->count; i += 1)
	{
		freeref(B[i].key);
		freeref(B[i].ref);

		DBG(DBGFREE, "i: %u", i);
	}

	freearray(env);
}

static const char *codetostr(const unsigned code)
{
	switch(code)
	{
	case MAP:
		return "/";
	
	case TYPE:
		return "@";
	
	case ATOM:
		return "$";
	}

	assert(0);
	return NULL;
}

static Ref decoatom(Array *const U, const unsigned code)
{
	switch(code)
	{
	case MAP:
	case TYPE:
	case ATOM:
		return readpack(U, strpack(0, codetostr(code)));

	default:
		assert(0);
	}

	return reffree();
}

Ref decorate(const Ref key, Array *const U, const unsigned code)
{
	return reflist(RL(decoatom(U, code), key));
}

Binding *keymap(const Array *const map, const Ref key)
{
	assert(map && map->code == MAP);

	const unsigned k = lookup(map, &key);

	if(k != -1)
	{
		return (Binding *)map->u.data + k;
	}
	
	return k == -1 ? NULL : (Binding *)map->u.data + k;
}

Binding *tunekeymap(
	Array *const map,
	const Ref key, Array *const U, const unsigned code, const Ref ref)
{
	assert(map && keymap(map, key, U, code) == NULL);

	const Binding b =
	{
		.key = decorate(decorator(U, code), key),
		.ref = ref
	};

	const unsigned k = readin(map, &b);
	return (Binding *)map->u.data + k;
}

Array *makepath(
	const Array *const map,
	const Ref path, const List *const names, Array *const newmap)
{
	assert(map && map->code == MAP);
}

typedef struct
{
	const List *const key;
	const Array *env;
	unsigned pos;
	unsigned depth;
} LookingState;

static GDI lookbinding(
	const List *const env, const List *const key, const unsigned depth)
{
	assert(env && env->ref.code == ENV);
	assert(depth == 1 || depth == -1);

	LookingState s =
	{
		.key = key,
		.env = NULL,
		.pos = -1,
		.depth = depth
	};

	const int reason = forlist((List *)env, looker, &s, 0);

	switch(reason)
	{
	case FOUND:
		assert(s.depth);
		return (GDI) { .array = s.env, .position = s.pos };
	
	case NOTFOUND:
		assert(s.env == NULL && s.pos == -1);
		break;
	
	case HITDEPTH:
		assert(s.env == NULL && s.pos == -1 && s.depth == 0);
		break;
	
	default:
		assert(0);
	}

	return (GDI) { .array = NULL, .position = -1 };
}


// WARNING: allocate должна вызываться только после того, как lookbinding
// сообщит, что ничего не найдено.

static GDI allocate(
	Array *const env, const List *const key)
{
	assert(env->code == ENV);

	const Binding b =
	{
		.key = forklist(key),
		.ref = { .code = FREE, .u.pointer = NULL }
	};

	return (GDI) { .array = env, .position = readin(env, &b) };
}


// LOG: Вместе с тем, как конструируются узлы (cf. lib/lime/loaddag.c:node),
// это важный момент вообще для процесса программирования (правда, не очень
// понятно, как именно важный). Важность в том, что мы должны иметь возможность
// передать обратно некие данные, записав их в заранее подготовленную ячейку в
// некоторой структуре данных. Позиция ячейки вычисляема по аргументам функции

static Ref *gditorefcell(const GDI g)
{
	assert(g.array && g.array->code == ENV);
	assert(g.position != -1 && g.position < g.array->count);
	Binding *const B = g.array->data;
	assert(B);
	return &B[g.position].ref;
}

static Binding *gditobindcell(const GDI g)
{
	assert(g.array && g.array->code == ENV);
	assert(g.position != -1 && g.position < g.array->count);
	Binding *const B = g.array->data;
	assert(B);
	return &B[g.position];
}

Ref *keytoref(
	const List *const env, const List *const key, const unsigned depth)
{
	const GDI gdi = lookbinding(env, key, depth);

	if(gdi.position != -1)
	{
		return gditorefcell(gdi);
	}

	// После lookbinding уже известно, что в env находятся ENV
	return gditorefcell(allocate(tip(env)->ref.u.environment, key));
}

Binding *keytobinding(
	const List *const env, const List *const key, const unsigned depth)
{
	const GDI gdi = lookbinding(env, key, depth);

	if(gdi.position != -1)
	{
		return gditobindcell(gdi);
	}

	return gditobindcell(allocate(tip(env)->ref.u.environment, key));
}

Ref *formkeytoref(
	Array *const U,
	const List *const env, const List *const key, const unsigned depth)
{
	DL(fkey, RS(
		refatom(readpack(U, strpack(0, "#"))),
		reflist((List *)key)));
	
	return keytoref(env, fkey, depth);
}

const Binding *topbindings(const List *const env, unsigned *const count)
{
	assert(env && env->ref.code == ENV);

	const Array *const E = tip(env)->ref.u.environment;
	assert(E && E->code == ENV);

	*count = E->count;
	return (const Binding *)E->data;
}

void freeenvironment(List *env)
{
	while(env)
	{
		env = popenvironment(&env);
	}
}
