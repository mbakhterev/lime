#include "construct.h"
#include "util.h"
#include "heapsort.h"

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

static int cmplists(const List *const k, const List *const l) {
	return 0;
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

extern unsigned loadbinding(Environment *const env, const Binding *const b) {
	unsigned k = lookbinding(env, b->key); 
	if(k != -1) { return k; }

//	const Binding *const bptr = append(&env->bindings, b);

	k = env->bindings.count;
	append(&env->index, &k);

//	heapsort((const void **)env->index.buffer, env->index.count, cmplists);

	heapsort((const void *)env->bindings.buffer,
		(unsigned *)env->index.buffer, k, cmpbindings);

	return k;
}
