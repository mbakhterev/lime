#include "construct.h"
#include "util.h"
#include "heapsort.h"

#include <assert.h>

Environment mkenvironment() {
	return (Environment) {
		.index = mkarray(sizeof(Binding *)),
		.bindings = mkarray(sizeof(Binding))
	};
}

void freeenvironment(Environment *const env) {
	freearray(&env->bindings);
	freearray(&env->index);
}

static int cmplists(const List *const, const List *const);

// Сравнение элементов списка. Без лукавых мудростей: сравниваются только узлы с
// числовым содержимым (NUMBER, ATOM, TYPE) и подсписки похожей структуры.
// Попытка сравнить узлы другой строкутуры - баг. Порядок между числами из
// различных классов определяется порядком классов в соответствующем enum (см.
// lib/lime/construct.h

static int iscomparable(const List *const k) {
	return k->code <= LIST;
}

static int cmpitems(const List *const k, const List *const l) {
	assert(iscomparable(k));
	assert(iscomparable(l));

//	unsigned r = 1 - (k->code == l->code) - ((k->code - l->code) << 1);

	unsigned r = cmpui(k->code, l->code);
	if(r) { return r; }

	if(k->code < LIST) {
		return cmpui(k->u.number, l->u.number);
	}

	return cmplists(k->u.list, l->u.list);
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

static int cmpbindings(
	const void *const bindings, const unsigned i, const unsigned j) {
	const Binding *const b = bindings;
	return cmplists(b[i].key, b[j].key);
}

extern unsigned lookbinding(Environment *const env, const List *const key) {
	if(env->index.count) { } else {
		return -1;
	}

	const Binding *const *const B = env->index.buffer;
	unsigned r = env->index.count - 1;
	unsigned l = 0;

	while(l < r) {
		const unsigned m = middle(l, r);

		if(cmplists(B[m]->key, key) < 0) {
			l = m + 1;
		}
		else {
			r = m;
		
		}
	}

	if(l == r && cmplists(B[l]->key, key) == 0) {
		return B[l]->id;
	}

	return -1;
}

extern unsigned loadbinding(Environment *const env, Binding b) {
	unsigned k = lookbinding(env, b.key); 
	if(k != -1) { return k; }

	b.key = forklist(b.key);
	append(&env->bindings, &b);

	k = env->bindings.count;
	append(&env->index, &k);

//	heapsort((const void **)env->index.buffer, env->index.count, cmplists);

	heapsort((const void *)env->bindings.buffer,
		(unsigned *)env->index.buffer, k, cmpbindings);

	return k;
}
