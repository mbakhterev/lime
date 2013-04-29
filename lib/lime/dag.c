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

// Подробности: txt/sketch.txt: Fri Apr 26 19:29:46 YEKT 2013

static LoadCurrent core(LoadContext *const ctx, LoadCurrent lc) {
	const int c = skipspaces(ctx->file);

	switch(c) {
	case ')':
		return lc;

	case '\'':
		return node(ctx, lc);

	case '(':
		Array env = makeenvironment();
		List le;
		le.next = &le;
		le.ref = refenv(&env);

		ctx->env = append(ctx->env, &le);

		assert(tipoff(&ctx->env) == &le);
		freeenvironment(&env);

		break;
	}

	return l;
}

List *loadrawdag(LoadContext *const ctx) {
	FILE *const f = ctx->file;
	int c;

	if((c = fgetc(f)) == '(') { } else {
		errexpect(c, ES("("));
	}

	return NULL;
}
