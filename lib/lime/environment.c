#include "construct.h"
#include "util.h"

#include <assert.h>

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

static int kcmp(const void *const D, const unsigned i, const void *const key) {
	return cmplists(((const Binding *)D)[i].key, key);
}

static int icmp(const void *const D, const unsigned i, const unsigned j) {
	return kcmp(D, i, ((const Binding *)D)[j].key);
}

Array makeenvironment(void) {
	return makearray(ENV, sizeof(Binding), icmp, kcmp);
}

void freeenvironment(Array *const env) {
	assert(env->code == ENV);
	freearray(env);
}

typedef struct {
	const List *key;
	const Binding *result;
} LookingState;

static int looker(List *const env, void *const ptr) {
	assert(env && env->ref.code == ENV);

	LookingState *const s = ptr;
	const unsigned k = lookup(env->ref.u.environment, s->key);

	if(k != -1) {
		s->result = (const Binding *)env->ref.u.environment->data + k;
		return 1;
	}

	return 0;
}

const Binding *lookbinding(const List *const env, const List *const key) {
	LookingState s;
	s.key = key;
	s.result = NULL;

	if(forlist((List *)env, looker, &s, 0) != 0) {
		return s.result;
	}

	assert(s.result == NULL);

	return NULL;
}

extern unsigned readbinding(Array *const env, const Binding *const b) {
	assert(env->code == ENV);

	const unsigned k = lookup(env, b->key); 
	if(k != -1) { return k; }

	return readin(env, b);
}
