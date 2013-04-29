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

static unsigned loadnum(FILE *const f) {
	unsigned n;
	if(fscanf(f, "%u", &n) == 1) { } else
		ERR("%s", "can't read number");
	}

	if(n <= MAXNUM) { } else {
		ERR("%u is > %u", n, MAXNUM);
	}

	return n;
}

static LoadCurrent ce(LoadContext *const ctx,
	List *const env, List *const nodes, List *const refs) {
	assert(env->ref.code == ENV);
	assert(nodes->ref.code == NODE);

	const int c = skipspaces(ctx->file);
	switch(c) {
	case ')':
		return (LoadCurrent) { .nodes = nodes, .refs = refs };
	
	case ';':
		return core(ctx, env, nodes, refs);
	}

	errexpect(c, ES(")", ";"));

	return (LoadContext) { .nodes = NULL, .refs = NULL };
}

static LoadCurrent core(LoadContext *const ctx,
	List *const env, List *const nodes, List *const refs) {
	assert(env->ref.code == ENV);
	assert(nodes->ref.code == NODE);

	const int c = skipspaces(ctx->file);
	switch(c) {
	case ')':
		return (LoadCurrent) { .nodes = nodes, .refs = refs };

	case '\'':
		return node(ctx, env, nodes, refs);

	case '(':
		Array E = makeenvironment();
		List le;
		le.next = &le;
		le.ref = refenv(&E);
		List *lenv = append(&le, env);

		LoadCurrent lc = core(ctx, lenv, nodes, NULL);
		lc.refs = append(refs, newlist(reflist(lc.refs)));
		
		assert(tipoff(&lenv) == &le && lenv == env);
		freeenvironment(&env);

		return lc;
	}

	if(isdigit(c)) {
		assert(ungetc(c, ctx->file) == c);
		List *const lrefs = append(refs, newlist(refnum(loadnum(f))));
		return ce(ctx, env, nodes, lrefs);
	}

	errexpect(c, ES("(", "'", ")", "[0-9]+"));

	return (LoadContext) { .nodes = NULL, .refs = NULL };
}

List *loadrawdag(LoadContext *const ctx) {
	FILE *const f = ctx->file;
	int c;

	if((c = fgetc(f)) == '(') { } else {
		errexpect(c, ES("("));
	}

	return NULL;
}
