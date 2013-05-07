#include "construct.h"
#include "util.h"

#include <assert.h>
#include <string.h>
#include <ctype.h>

#define DBGLST 1
#define DBGLRD 2

#define DBGFLAGS (DBGLST | DBGLRD)

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

// Обработка узла не требует текущего накопленного сипска ссылок. Накопленные
// ссылки при обработке будут добавлены в node.u.attributes

static LoadCurrent node(LoadContext *const, List *const, List *const);

static LoadCurrent ce(LoadContext *const,
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

// Для list не нужен параметр refs. Список всегда начинается с пустого списка
// ссылок

static LoadCurrent list(LoadContext *const ctx,
	List *const env, List *const nodes)
{
	DBG(DBGLST, "LIST: ctx: %p", (void *)ctx);

	assert(env == NULL || env->ref.code == ENV);
	assert(nodes == NULL || nodes->ref.code == NODE);

	FILE *const f = ctx->file;
	assert(f);

	DBG(DBGLST, "%s", "pre skipping");

	const int c = skipspaces(f);
	switch(c)
	{
	case ')':
		return LC(nodes, NULL);
	}

	DBG(DBGLST, "c: %d", c);

	if(isfirstcore(c))
	{
		ungetc(c, f);

		Array E = makeenvironment();

		List l = { .ref = refenv(&E), .next = &l };
		List *lenv = append(&l, env); // lenv - local env

		const LoadCurrent lc = core(ctx, lenv, nodes, NULL);

		assert(tipoff(&lenv) == &l && lenv == env);
		freeenvironment(&E);

		return lc;
	}

	errexpect(c, ES("(", "'", "[0-9]+", "[A-Za-z][0-9A-Za-z]+", ")"));

	return LC(NULL, NULL);
}

static LoadCurrent ce(LoadContext *const ctx,
	List *const env, List *const nodes, List *const refs)
{
	assert(env && env->ref.code == ENV);
	assert(nodes == NULL || nodes->ref.code == NODE);

	const int c = skipspaces(ctx->file);
	switch(c)
	{
	case ')':
		return (LoadCurrent) { .nodes = nodes, .refs = refs };
	
	case ';':
		return core(ctx, env, nodes, refs);
	}

	errexpect(c, ES(")", ";"));

	return (LoadCurrent) { .nodes = NULL, .refs = NULL };
}

static LoadCurrent core(LoadContext *const ctx,
	List *const env, List *const nodes, List *const refs)
{
	assert(env != NULL && env->ref.code == ENV);
	assert(nodes == NULL || nodes->ref.code == NODE);

	FILE *const f = ctx->file;
	assert(f);

	const int c = skipspaces(f);
	switch(c)
	{
	case '\'': {
		// Обработка узла может добавить только новый узел. Список
		// ссылок должен представлять собой один элемент со ссылокой на
		// узел.

		const LoadCurrent lc = node(ctx, env, nodes);

		List *const l = lc.refs;
		assert(l->ref.code == NODE && l->next == l);

		return ce(ctx, env, lc.nodes, append(refs, l));

//		return node(ctx, env, nodes, refs);

	}

	case '(':
	{
		const LoadCurrent lc = list(ctx, env, nodes);
		return ce(ctx, env, 
			lc.nodes, append(refs, RL(reflist(lc.refs))));
	}
	}

	if(isdigit(c))
	{
		assert(ungetc(c, ctx->file) == c);

		List *const lrefs
			= append(refs, RL(refnum(NUMBER, loadnum(f))));

		return ce(ctx, env, nodes, lrefs);
	}

	errexpect(c, ES("(", "'", "[0-9]+"));

	return (LoadCurrent) { .nodes = NULL, .refs = NULL };
}

static LoadCurrent node(
	LoadContext *const ctx, List *const env, List *const nodes)
{
	Array *const U = ctx->universe;
	FILE *const f = ctx->file;

	assert(f);
	assert(U);

	const int c = fgetc(f);
	if(isascii(c) && isalpha(c))
	{
		ungetc(c, f);
	}
	else
	{
		errexpect(c, ES("[A-Za-z]"));
	}

	List *const l
		= RL(refnode(newnode(loadtoken(U, f, 0, "[0-9A-Za-z]"), NULL)));
	
	return LC(append(nodes, RL(refnode(l->ref.u.node))), l);
}

List *loadrawdag(LoadContext *const ctx, List *const env, List *const nodes)
{
	FILE *const f = ctx->file;
	int c;

	if((c = fgetc(f)) == '(') { } else
	{
		errexpect(c, ES("("));
	}

	DBG(DBGLRD, "pre list; ctx: %p", (void *)ctx);

	const LoadCurrent lc = list(ctx, env, nodes);

	if(DBGFLAGS & DBGLRD)
	{
		char *const c = dumplist(lc.refs);
		DBG(DBGLRD, "refs(%u): %s", listlen(lc.refs), c);
		free(c);
	}

	// FIXME: Список в виде списка ссылок не нужен
	freelist(lc.refs);

	return lc.nodes;

	return NULL;
}
