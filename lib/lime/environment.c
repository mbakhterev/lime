#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGFREE	1

//#define DBGFLAGS (DBGFREE)

#define DBGFLAGS 0

static int cmplists(const List *const, const List *const);

// Сравнение элементов списка. Без лукавых мудростей: сравниваются только узлы с
// числовым содержимым (NUMBER, ATOM, TYPE) и подсписки похожей структуры.
// Попытка сравнить узлы другой строкутуры - баг. Порядок между числами из
// различных классов определяется порядком классов в соответствующем enum (cf.
// lib/lime/construct.h)

static int iscomparable(const List *const k) {
	return k->ref.code <= LIST;
}

static int cmpitems(const List *const k, const List *const l) {
	assert(iscomparable(k));
	assert(iscomparable(l));

	unsigned r = cmpui(k->ref.code, l->ref.code);
	if(r) { return r; }

	if(k->ref.code < LIST) {
		return cmpui(k->ref.u.number, l->ref.u.number);
	}

	return cmplists(k->ref.u.list, l->ref.u.list);
}

// Для поиска и сортировки важно, чтобы порядок был линейным. Построить его
// можно разными способами. Например, можно считать, что списки большей длины
// всегда > списков меньшей длинны, а внутри списков длины равной порядок
// устанавливается лексикографически. Но это не удобно для пользователя, ибо,
// скорее всего, близкие по смыслу списки будут и начинаться одинаково. В целях
// отладки полезно было бы ставить рядом друг с дружкой. Поэтому, порядок будет
// просто лексикографический. Это оправданное усложнение.

static int cmplists(const List *const k, const List *const l) {
	assert(k != NULL);
	assert(l != NULL);

	// current указатели
	const List *ck = k;
	const List *cl = l;

	unsigned r;

	do {
		ck = ck->next;
		cl = cl->next;
		r = cmpitems(ck, cl);
	} while(!r && ck != k && cl != l);

	if(r) { return r; }

	// Если (r == 0), значит какой-то список закончился. Разбираем три
	// варианта:
	return 1 - (ck == k && cl == l) - ((ck == k && cl != l) << 1);
}

typedef struct
{
	const List *key;
	Ref ref;
} Binding;

static int kcmp(const void *const D, const unsigned i, const void *const key) {
	return cmplists(((const Binding *)D)[i].key, key);
}

static int icmp(const void *const D, const unsigned i, const unsigned j) {
	return kcmp(D, i, ((const Binding *)D)[j].key);
}

Array makeenvironment(void) {
	return makearray(ENV, sizeof(Binding), icmp, kcmp);
}

void freeenvironment(Array *const env)
{
	assert(env->code == ENV);

	DBG(DBGFREE, "env->count: %u", env->count);

	Binding *const B = env->data;
	for(unsigned i = 0; i < env->count; i += 1)
	{
		freelist((List *)B[i].key);
		DBG(DBGFREE, "i: %u", i);
	}

	freearray(env);
}

typedef struct
{
	const List *key;
	const Array *env;
	unsigned pos;
} LookingState;

static int looker(List *const env, void *const ptr)
{
	assert(env && env->ref.code == ENV);

	LookingState *const s = ptr;
	const unsigned k = lookup(env->ref.u.environment, s->key);

	if(k != -1)
	{
//		s->result = (const Binding *)env->ref.u.environment->data + k;

		s->env = env->ref.u.environment;
		s->pos = k;
		return 1;
	}

	return 0;
}

GDI lookbinding(const List *const env, const List *const key) {
	LookingState s;
	s.key = key;
	s.env = NULL;
	s.pos = -1;

	if(forlist((List *)env, looker, &s, 0) != 0) {
		return (GDI) { .array = s.env, .position = s.pos };
	}

	assert(s.env == NULL && s.pos == -1);

	return (GDI) { .array = NULL, .position = -1 };
}

GDI readbinding(Array *const env, const Ref ref, const List *const key)
{
	assert(env->code == ENV);

	const unsigned k = lookup(env, key); 
	if(k != -1)
	{
		return (GDI) { .array = env, .position = k };
	}

	const Binding b = { .key = forklist(key), .ref = ref };
	return (GDI) { .array = env, .position = readin(env, &b) };
}

Ref gditoref(const GDI g)
{
	assert(g.array && g.array->code == ENV);
	assert(g.position != -1);
	const Binding *const B = g.array->data;
	return B[g.position].ref;
}

// LOG: Вместе с тем, как конструируются узлы (cf. lib/lime/loaddag.c:node),
// это важный момент вообще для процесса программирования (правда, не очень
// понятно, как именно важный). Важность в том, что мы должны иметь возможность
// передать обратно некие данные, записав их в заранее подготовленную ячейку в
// некоторой структуре данных. Позиция ячейки вычисляема по аргументам функции

Ref *gditorefcell(const GDI g)
{
	assert(g.array && g.array->code == ENV);
	assert(g.position != -1);
	Binding *const B = g.array->data;
	return &B[g.position].ref;
}
