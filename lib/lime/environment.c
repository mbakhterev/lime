#include "construct.h"

Environment mkenvironment() {
	return (Environment) {
		.lists = mkarray(sizeof(List *)),
		.index = mkarray(sizeof(List *)),
		.bindings = mkarray(sizeof(Binding))
	};
}

void freeenvironment(Environment *const env) {
	freearray(&env->bindings);
	freearray(&env->index);
	freearray(&env->lists);
}

extern unsigned lookbinding(Environment *const env, const List *const key) {
	
}

extern unsigned loadbinding(
	Environment *const env,
	const List *const key, const Binding value) {
	unsigned k = lookbinding(env, key); 
}
