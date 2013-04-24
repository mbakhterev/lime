#include "construct.h"
#include "util.h"

#include <assert.h>
#include <string.h>

Array keymap(Array *const U,
	const unsigned hint, const char *const A[], const unsigned N) {
	assert(U->code == ATOM);
	assert(N <= MAXNUM);
	assert(N <= MAXHINT);

	Array map = makeuimap();

	for(unsigned i = 0; i < N; i += 1) {
		const AtomPack ap = {
			.hint = hint,
			.bytes = (const unsigned char *)A[i],
			.length = strlen(A[i]) };

		uimap(&map, readpack(U, &ap));
	}

	return map;
}

// Состояния для автомата разбора k-выражения

enum {
	START, DONE };

List *loadrawdag(LoadContext *const ctx) {
	FILE *const f = ctx->file;
	int c;

	if((c = fgetc(f)) == '(') { } else {
		errexpect('(', c);
	}

	Array env = makeenvironment();
	List le;
	le.next = &le;
	le.code = ENV;
	le.u.environment = &env;

	ctx->env = append(&le, ctx->env);

	unsigned state = DONE;
	while(state != DONE) {
		
	}

	assert(tipoff(&ctx->env) == &le);

	return NULL;
}
