#include "construct.h"
#include "util.h"

#include <assert.h>
#include <string.h>
#include <ctype.h>

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

static LoadCurrent LC(List *const nodes, List *const refs) {
	return (LoadCurrent) { .nodes = nodes, .refs = refs };
}

static unsigned loadnum(FILE *const f) {
	unsigned n;
	if(fscanf(f, "%u", &n) == 1) { } else {
		ERR("%s", "can't read number");
	}

	if(n <= MAXNUM) { } else {
		ERR("%u is > %u", n, MAXNUM);
	}

	return n;
}

static LoadCurrent core(LoadContext *const ctx,
	List *const, List *const, List *const);

static LoadCurrent node(LoadContext *const,
	List *const, List *const, List *const);

static unsigned isascii(const int c) {
	return (c & 0x7f) == c;
}

static unsigned isfirstcore(const int c) {
	switch(c) {
	case '\'':
	case '(':
		return 1;
	}
	
	return isascii(c) && isalnum(c);
}

static LoadCurrent list(LoadContext *const ctx,
	List *const env, List *const nodes, List *const refs) {
	assert(env->ref.code == ENV);
	assert(env->ref.code == NODE);

	FILE *const f = ctx->file;
	assert(f);

	const int c = skipspaces(f);
	switch(c) {
	case ')':
		return LC(nodes, refs);
	}

	if(isfirstcore(c)) {
		Array E = makeenvironment();

		List *const l = RL(refenv(&E));
		// lenv - local env
		List *lenv = append(l, env);

		LoadCurrent const lc = core(ctx, lenv, nodes, NULL);

		assert(tipoff(&lenv) == l && lenv == env);
		freelist(l);
		freeenvironment(&E);

		return LC(lc.nodes, append(refs, RL(reflist(lc.refs))));
	}

	errexpect(c, ES("(", "'", "[0-9]+", "[A-Za-z][0-9A-Za-z]+", ")"));

	return LC(NULL, NULL);
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

	return (LoadCurrent) { .nodes = NULL, .refs = NULL };
}

static LoadCurrent core(LoadContext *const ctx,
	List *const env, List *const nodes, List *const refs) {
	assert(env->ref.code == ENV);
	assert(nodes->ref.code == NODE);

	FILE *const f = ctx->file;
	assert(f);

	const int c = skipspaces(ctx->file);
	switch(c) {
	case ')':
		return (LoadCurrent) { .nodes = nodes, .refs = refs };

	case '\'':
		return node(ctx, env, nodes, refs);

	case '(': {
	}
	}

	if(isdigit(c)) {
		assert(ungetc(c, ctx->file) == c);

		List *const lrefs
			= append(refs, RL(refnum(NUMBER, loadnum(f))));

		return ce(ctx, env, nodes, lrefs);
	}

	errexpect(c, ES("(", "'", ")", "[0-9]+"));

	return (LoadCurrent) { .nodes = NULL, .refs = NULL };
}

static LoadCurrent node(LoadContext *const ctx,
	List *const env, List *const nodes, List *const refs) {

	return (LoadCurrent) { .nodes = NULL, .refs = NULL };
}

List *loadrawdag(LoadContext *const ctx, List *const env, List *const nodes) {
	FILE *const f = ctx->file;
	int c;

	if((c = fgetc(f)) == '(') { } else {
		errexpect(c, ES("("));
	}

	const LoadCurrent lc = list(ctx, env, nodes, NULL);

	// Список ссылок не нужен
	freelist(lc.refs);

	return lc.nodes;

	return NULL;
}
