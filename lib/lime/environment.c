#include "construct.h"

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

static int cmplists(const Binding *const k, const Binding *const l) {
	
}

static int cmpbindings(const Binding *const a, const Binding *const b) {
	return cmplists(a->key, b->key);
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
	const unsigned k = lookbinding(env, b->key); 
	if(k != -1) { return k; }

	const Binding *const bptr = append(&env->bindings, b);
	append(&env->index, &bptr);
	heapsort((const void **)env->index.buffer, env->index.count, cmplists);
}
