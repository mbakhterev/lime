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

// Типы индексируются теми же ключами, что и окружения. Поэтому функция не
// статическая, будет использоваться и в types.c

Array *newkeyedarray(const unsigned code)
{
	switch(code)
	{
	case TYPE:
	case ENV:
		break;
	
	default:
		assert(0);
	}

	return newarray(code, sizeof(Binding), icmp, kcmp);
}

static void unregister(Array *const U, const Ref E)
{
	// Список из окружения E
	DL(self, E);

	// Список из root-окружения, в котором живёт наше E. Плюс немного
	// паранойи

	DL(rootkey, readpack(U, strpack(0, "Root")));
	DL(root, *ref(atenvkey(self, rootkey)));

	assert(root->ref.external
		&& root->ref.code == ENV
		&& root->ref.u.array && root->ref.u.array == ENV);

	// Список, который идентифицирует текущее окружение. Плюс немного
	// паранойи

	DL(idkey, readpack(U, strpack(0, "ID")));
	const Ref id = *ref(atenvkey(self, idkey));

	assert(id.external && id->.code == LIST);
	
	// Ref-а, которая соответствует E в root-окружении

	Ref *const re = ref(atenvkey(root, id->u.list));
	assert(re->code == ENV && re->u.array == E.array);

	*re = reffree();
}

static void freeenv(const Ref ref)
{
	assert(r.code == ENV
		&& r.u.array && r.u.array->code == ENV);

	DBG(DBGFREE, "env->count: %u", env->count);

	Binding *const B = env->data;

	for(unsigned i = 0; i < env->count; i += 1)
	{
		freelist((List *)B[i].key);
		
		switch(B[i].ref.code)
		{
		case FORM:
			// external-формы из других окружений не должны быть
			// затронуты. Сотрётся только их структура, но не графы
			// или сигнатуры

			freeform(B[i].ref);
			break;

		case LIST:
			// Будем считать, что если в окружении список, то за
			// этот список отвечает окружение. Как с формами.
			// Поэтому, удаляем

			freelist(B[i].ref.u.list);
			break;
		}

		DBG(DBGFREE, "i: %u", i);
	}

	freearray(env);
	free(env);
}

extern List *pushenvironment(List *const env)
{
	assert(!env || env->ref.code == ENV);

	Array *const new = malloc(sizeof(Array));
	assert(new);

	{
		const Array fresh = makekeyedarray(ENV);
		memcpy(new, &fresh, sizeof(Array));
	}

	return append(RL(refenv(new)), env);
}

extern List *popenvironment(List **const penv)
{
	assert(*penv != NULL && (*penv)->ref.code == ENV);

	List *t = tipoff(penv);

	freeone(t->ref.u.environment);
	freelist(t);

	return *penv;
}

typedef struct
{
	const List *const key;
	const Array *env;
	unsigned pos;
	unsigned depth;
} LookingState;

enum { NOTFOUND, HITDEPTH, FOUND };

static int looker(List *const env, void *const ptr)
{
	assert(env && env->ref.code == ENV);
	assert(env->ref.code == ENV && env->ref.u.environment);

	LookingState *const s = ptr;
	const unsigned k = lookup(env->ref.u.environment, s->key);

	if(k != -1)
	{
		s->env = env->ref.u.environment;
		s->pos = k;
		return FOUND;
	}

	return (s->depth -= 1) ? NOTFOUND : HITDEPTH;
}

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
