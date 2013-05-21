#include "construct.h"
#include "util.h"

#include <assert.h>
#include <string.h>
#include <ctype.h>

#define DBGLST	0x01
#define DBGLRD	0x02
#define DBGNODE	0x04
#define DBGCORE	0x08
#define DBGCE	0x10

// #define DBGFLAGS (DBGLST | DBGLRD)
// #define DBGFLAGS (DBGLRD | DBGNODE)
// #define DBGFLAGS 0x1f
// #define DBGFLAGS (DBGNODE)
// #define DBGFLAGS (DBGLRD)

#define DBGFLAGS 0

Array keymap(
	Array *const U,
	const unsigned hint, const char *const A[])
{
	assert(U->code == ATOM);
	assert(hint <= MAXHINT);

	Array map = makeuimap();

	unsigned i;
	for(i = 0; i < MAXNUM && A[i] != NULL; i += 1)
	{
		uimap(&map, readpack(U, strpack(hint, A[i])));
	}
	assert(i < MAXNUM);

	return map;
}

// Подробности: txt/sketch.txt: Fri Apr 26 19:29:46 YEKT 2013

static LoadCurrent LC(List *const nodes, List *const refs) {
	return (LoadCurrent) { .nodes = nodes, .refs = refs };
}

static unsigned loadnum(FILE *const f) {
	unsigned n;
	if(fscanf(f, "%u", &n) == 1) {} else {
		ERR("%s", "can't read number");
	}

	if(n <= MAXNUM) {} else {
		ERR("%u is > %u", n, MAXNUM);
	}

	return n;
}

static LoadCurrent core(
	const LoadContext *const ctx, List *const, List *const, List *const);

// Обработка узла не требует текущего накопленного сипска ссылок. Накопленные
// ссылки при обработке будут добавлены в node.u.attributes

static LoadCurrent node(const LoadContext *const, List *const, List *const);

static LoadCurrent ce(
	const LoadContext *const, List *const, List *const, List *const);

static unsigned isascii(const int c)
{
	return (c & 0x7f) == c;
}

static unsigned isfirstid(const int c)
{
	return isascii(c) && isalpha(c);
}

static unsigned isfirstcore(const int c)
{
	switch(c)
	{
	case '\'':
	case '(':
		return 1;
	}
	
	return isascii(c) && isalnum(c);
}

// Для list не нужен параметр refs. Список всегда начинается с пустого списка
// ссылок

static LoadCurrent list(
	const LoadContext *const ctx, List *const env, List *const nodes)
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
		DBG(DBGLST, "%s", "-> END");
		return LC(nodes, NULL);
	}

	DBG(DBGLST, "c: %d", c);

	if(isfirstcore(c))
	{
		assert(ungetc(c, f) == c);

		Array E = makeenvironment();

		List l = { .ref = refenv(&E), .next = &l };
		List *lenv = append(&l, env); // lenv - local env

		DBG(DBGLST,
			"-> ENV(CORE). env: %p; lenv %p; E: %p",
			(void *)env, (void *)lenv, (void *)&E);

		const LoadCurrent lc = core(ctx, lenv, nodes, NULL);

		assert(tipoff(&lenv) == &l && lenv == env);

		DBG(DBGLST, "freeing E: %p", (void *)&E);

		freeenvironment(&E);

		DBG(DBGLST, "released E: %p", (void *)&E); 

		return lc;
	}

	errexpect(c, ES("(", "'", "[0-9]+", "[A-Za-z][0-9A-Za-z]+", ")"));

	return LC(NULL, NULL);
}

static LoadCurrent ce(
	const LoadContext *const ctx,
	List *const env, List *const nodes, List *const refs)
{
	DBG(DBGCE, "%s", "");

	assert(env && env->ref.code == ENV);
	assert(nodes == NULL || nodes->ref.code == NODE);

	DBG(DBGCE, "%s", "skipping");
	const int c = skipspaces(ctx->file);
	DBG(DBGCE, "%s", "skipped");

	switch(c)
	{
	case ')':
		DBG(DBGCE, "%s", "-> END");
		return (LoadCurrent) { .nodes = nodes, .refs = refs };
	
	case ';':
		DBG(DBGCE, "%s", "-> CORE");
		return core(ctx, env, nodes, refs);
	}

	errexpect(c, ES(")", ";"));

	return (LoadCurrent) { .nodes = NULL, .refs = NULL };
}

static LoadCurrent core(
	const LoadContext *const ctx,
	List *const env, List *const nodes, List *const refs)
{
	DBG(DBGCORE, "%s", "");

	assert(env != NULL && env->ref.code == ENV);
	assert(nodes == NULL || nodes->ref.code == NODE);

	FILE *const f = ctx->file;
	Array *const U = ctx->universe;

	assert(f);
	assert(U);

	const int c = skipspaces(f);
	switch(c)
	{
	case '\'':
	{
		// Обработка узла может добавить только новый узел. Список
		// ссылок должен представлять собой один элемент со ссылокой на
		// узел.

		DBG(DBGCORE, "%s", "-> NODE");
		const LoadCurrent lc = node(ctx, env, nodes);

		List *const l = lc.refs;
		assert(l->ref.code == NODE && l->next == l);

		DBG(DBGCORE, "%s", "-> CE");
		return ce(ctx, env, lc.nodes, append(refs, l));
	}

	case '(':
	{
		DBG(DBGCORE, "%s", "-> LIST");
		const LoadCurrent lc = list(ctx, env, nodes);

		DBG(DBGCORE, "%s", "-> CE");
		return ce(
			ctx, env, lc.nodes, append(refs, RL(reflist(lc.refs))));
	}
	}

	if(isdigit(c))
	{
		assert(ungetc(c, f) == c);

		List *const lrefs
			= append(refs, RL(refnum(NUMBER, loadnum(f))));

		DBG(DBGCORE, "-> %u -> CE", lrefs->ref.u.number);
		return ce(ctx, env, nodes, lrefs);
	}

	if(isfirstid(c))
	{
		assert(ungetc(c, f) == c);

		const List l =
		{
			.ref = refnum(ATOM, loadtoken(U, f, 0, "[0-9A-Za-z]")),
			.next = (List *)&l
		};

		const GDI ref = lookbinding(env, &l);

		if(ref.array) {} else
		{
			assert(ref.position == -1);

			ERR("no label in the scope: %s",
				atombytes(atomat(U, l.ref.u.number)));
		}

		const Ref *const r = gditorefcell(ref);
		assert(r->code == NODE);

		if(r->u.node) {} else
		{
			ERR("labeled node is not complete: %s",
				atombytes(atomat(U, l.ref.u.number)));
		}

		return ce(ctx, env,
			nodes, append(refs, RL(refnode(r->u.node))));
		
	}

	errexpect(c, ES("(", "'", "[0-9]+"));

	return (LoadCurrent) { .nodes = NULL, .refs = NULL };
}

static LoadCurrent node(
	const LoadContext *const ctx, List *const env, List *const nodes)
{
	DBG(DBGNODE, "%s", "");

	assert(env && env->ref.code == ENV);

	Array *const U = ctx->universe;
	FILE *const f = ctx->file;

	assert(f);
	assert(U);

	int c = fgetc(f);

	if(isfirstid(c))
	{
		assert(ungetc(c, f) == c);
	}
	else
	{
		errexpect(c, ES("[A-Za-z]"));
	}

	const unsigned verb = loadtoken(U, f, 0, "[0-9A-Za-z]");
	const unsigned key = uireverse(ctx->keymap, verb);

	DBG(DBGNODE, "verb: %u; key: %u", verb, key);

	if(!(ctx->keyonly && key == -1)) {} else
	{
		ERR("non key atom in keyonly mode: %s",
			atombytes(atomat(U, verb)));
	}

	GDI ref = { .array = NULL, .position = -1 };

	// Надо получить следующий символ и, так получается, в любом случае
	// вернуть его обратно

	c = skipspaces(f);
	assert(ungetc(c, f) == c);

	DBG(DBGNODE, "cc: %c", c);
	
	// Если дальше следует метка узла
	if(isfirstid(c))
	{
		const List lid =
		{
			.ref = refnum(ATOM, loadtoken(U, f, 0, "[0-9A-Za-z]")),
			.next = (List *)&lid
		};

		ref = lookbinding(env, &lid);
		if(ref.array == NULL)
		{
			assert(ref.position == -1);
		}
		else
		{
			ERR("node label is in scope: %s",
				atombytes(atomat(U, lid.ref.u.number)));
		}

		Array *const E = tip(env)->ref.u.environment;

		// Резервирование места в области видимости
		ref = readbinding(E, refnode(NULL), &lid);

		// Удостоверяемся в =
		if((c = skipspaces(f)) == '=') {} else
		{
			errexpect(c, ES("="));
		}
	}

	// Загрузка атрибутов узла, которые могут быть в специальном формате
	// (для ключевых узлов)

	LoadCurrent lc = LC(NULL, NULL);

	if(key == -1)
	{
		// Стандартная загрузка списка аргументов

		if((c = skipspaces(f)) == '(') {} else
		{
			errexpect(c, ES("("));
		}

		lc = list(ctx, env, nodes);
	}
	else
	{
		lc = ctx->actions[key](ctx, env, nodes);
	}

	if(DBGFLAGS & DBGNODE)
	{
		char *const ns = dumplist(lc.nodes);
		char *const rs = dumplist(lc.refs);
		DBG(DBGNODE, "ns: %s", ns);
		DBG(DBGNODE, "rs: %s", rs);
		free(rs);
		free(ns);
	}

	List *const l = RL(refnode(newnode(verb, lc.refs)));

	// Узел создан, и если под него зарезервирована метка в окружении, надо
	// бы его туда добавить

	if(ref.array)
	{
		gditorefcell(ref)->u.node = l->ref.u.node;
	}
	
	return LC(append(lc.nodes, RL(refnode(l->ref.u.node))), l);
}

List *loadrawdag(
	const LoadContext *const ctx, List *const env, List *const nodes)
{
	FILE *const f = ctx->file;
	int c;

	if((c = skipspaces(f)) == '(') {} else
	{
		errexpect(c, ES("("));
	}

	DBG(DBGLRD, "pre list; ctx: %p", (void *)ctx);

	DBG(DBGLRD, "%s", "-> LIST");
	const LoadCurrent lc = list(ctx, env, nodes);

	if(DBGFLAGS & DBGLRD)
	{
		DBG(DBGLRD, "%s", "dumping");

		char *const c = dumplist(lc.refs);
		DBG(DBGLRD, "refs(%u): %s", listlen(lc.refs), c);
		free(c);
	}

	// FIXME: Список в виде списка ссылок не нужен
	freelist(lc.refs);

	return lc.nodes;

	return NULL;
}
