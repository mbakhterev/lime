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
#define DBGKR	0x20

// #define DBGFLAGS (DBGLST | DBGLRD)
// #define DBGFLAGS (DBGLRD | DBGNODE)
// #define DBGFLAGS (DBGNODE)
// #define DBGFLAGS (DBGLRD)
// #define DBGFLAGS (DBGKR)

#define DBGFLAGS 0

// Подробности: txt/sketch.txt: Fri Apr 26 19:29:46 YEKT 2013

typedef struct
{
	FILE *const file;
	Array *const universe;
	const Array *const dagmap;
} LoadContext;

typedef struct
{
	List *nodes;
	List *refs;
} LoadCurrent;

static LoadCurrent LC(List *const nodes, List *const refs)
{
	return (LoadCurrent) { .nodes = nodes, .refs = refs };
}

static Ref loadnum(FILE *const f)
{
	unsigned n;
	if(fscanf(f, "%u", &n) != 1)
	{
		ERR("%s", "can't read number");
	}

	if(n > MAXNUM)
	{
		ERR("%u is > %u", n, MAXNUM);
	}

	return refnum(n);
}

static LoadCurrent core(
	const LoadContext *const ctx, List *const, List *const, List *const);

// Обработка узла не требует текущего накопленного сипска ссылок. Накопленные
// ссылки при обработке будут добавлены в node.u.attributes

static LoadCurrent node(const LoadContext *const, List *const, List *const);

static LoadCurrent ce(
	const LoadContext *const, List *const, List *const, List *const);

#ifndef isascii
static unsigned isascii(const int c)
{
	return (c & 0x7f) == c;
}
#endif

static unsigned isfirstid(const int c)
{
	return isascii(c) && isalpha(c);
}

static unsigned isfirstcore(const int c)
{
	switch(c)
	{
	case '\'':
	case '.':
	case '(':
		return 1;
	}
	
	return isascii(c) && isalnum(c);
}

static List *pushenv(List *const env)
{
	assert(env == NULL || iskeymap(env->ref));
	return append(RL(refkeymap(newkeymap())), env);
}

static List *popenv(List **const penv)
{
	assert(penv && *penv);
	List *const k = tipoff(penv);
	assert(k != NULL && iskeymap(k->ref));
	
	// Освобождение списка освободит и сохранённую Ref, а так как там
	// сброшен external-бит, то и keymap будет освобождена

	freelist(k);

	return *penv;
}

// Для list не нужен параметр refs. Список всегда начинается с пустого списка
// ссылок

static LoadCurrent list(
	const LoadContext *const ctx, List *const env, List *const nodes)
{
	DBG(DBGLST, "LIST: ctx: %p", (void *)ctx);

	assert(env == NULL || iskeymap(env->ref));
	assert(nodes == NULL || isnode(nodes->ref));

	FILE *const f = ctx->file;
	assert(f);

	DBG(DBGLST, "%s", "pre skipping");

	// Либо список пустой...

	const int c = skipspaces(f);
	switch(c)
	{
	case ')':
		DBG(DBGLST, "%s", "-> END");
		return LC(nodes, NULL);
	}

	DBG(DBGLST, "c: %d", c);

	// ... либо список содержит core-структуру (да-да, немного пафосное
	// название)

	if(isfirstcore(c))
	{
		assert(ungetc(c, f) == c);

		List *lenv = pushenv(env);

		DBG(DBGLST,
			"-> ENV(CORE). env: %p; lenv %p; E: %p",
			(void *)env,
			(void *)lenv,
			(void *)lenv->ref.u.array);

		const LoadCurrent lc = core(ctx, lenv, nodes, NULL);

		assert(popenv(&lenv) == env);

		return lc;
	}

	errexpect(c, ES("(", ".", "'", "[0-9]+", "[A-Za-z][0-9A-Za-z]+", ")"));

	return LC(NULL, NULL);
}

static LoadCurrent ce(
	const LoadContext *const ctx,
	List *const env, List *const nodes, List *const refs)
{
	DBG(DBGCE, "%s", "");

	assert(env && iskeymap(env->ref));
	assert(nodes == NULL || isnode(nodes->ref));

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

static Ref *keytoref(List *const env, const Ref r)
{
	assert(env && iskeymap(env->ref));

	Binding *b = (Binding *)pathlookup(env, r, NULL);

	DBG(DBGKR, "%p", (void *)b);

	if(b)
	{
		return &b->ref;
	}

	b = mapreadin(tip(env)->ref.u.array, r);
	assert(b);

	return &b->ref;
}

static LoadCurrent core(
	const LoadContext *const ctx,
	List *const env, List *const nodes, List *const refs)
{
	DBG(DBGCORE, "%s", "");

	assert(env != NULL && iskeymap(env->ref));
	assert(nodes == NULL || isnode(nodes->ref));

	FILE *const f = ctx->file;
	Array *const U = ctx->universe;

	assert(f);
	assert(U);

	const int c = skipspaces(f);
	switch(c)
	{
	case '.':
	{
		// Обработка узла может добавить только новый узел. Список
		// ссылок должен представлять собой один элемент со ссылокой на
		// узел

		DBG(DBGCORE, "%s", "-> NODE");
		const LoadCurrent lc = node(ctx, env, nodes);

		List *const l = lc.refs;
		assert(l && isnode(l->ref) && l->next == l);

		DBG(DBGCORE, "%s", "-> CE");
		return ce(ctx, env, lc.nodes, append(refs, l));
	}

	case '(':
	{
		// Здесь загружается список

		DBG(DBGCORE, "%s", "-> LIST");
		const LoadCurrent lc = list(ctx, env, nodes);

		DBG(DBGCORE, "%s", "-> CE");
		return ce(
			ctx, env, lc.nodes, append(refs, RL(reflist(lc.refs))));
	}

	case '\'':
		// Загрузка атома
		return ce(ctx, env, nodes, append(refs, RL(loadatom(U, f))));
	}

	// Остаётся один возможный вариант: имя ссылки на узел

	if(isdigit(c))
	{
		assert(ungetc(c, f) == c);

		List *const lrefs = append(refs, RL(loadnum(f)));

		DBG(DBGCORE, "-> %u -> CE", lrefs->ref.u.number);
		return ce(ctx, env, nodes, lrefs);
	}

	if(isfirstid(c))
	{
		assert(ungetc(c, f) == c);

		const Ref l = loadtoken(U, f, 0, "[0-9A-Za-z]");
		const Ref *const r = keytoref(env, l);

		if(r->code == FREE)
		{
			assert(r->u.pointer == NULL);

			ERR("no label in scope: %s",
				atombytes(atomat(U, l.u.number)));
		}

		assert(r->code == NODE);

		if(r->u.list == NULL)
		{
			ERR("labeled node is not complete: %s",
				atombytes(atomat(U, l.u.number)));
		}

		return ce(ctx, env,
			nodes, append(refs, RL(markext(*r))));
		
	}

	errexpect(c, ES("(", ".", "'", "[0-9]+", "[A-Za-z][0-9A-Za-z]+"));

	return (LoadCurrent) { .nodes = NULL, .refs = NULL };
}

static LoadCurrent loadattr(
	const LoadContext *const ctx, List *const env, List *const nodes)
{
	FILE *const f = ctx->file;
	assert(f);

	int c;
	if((c = skipspaces(f)) != '(')
	{
		errexpect(c, ES("("));
	}

	return list(ctx, env, nodes);
}

static LoadCurrent loadsubdag(
	const LoadContext *const ctx, List *const env, List *const nodes)
{
	Array *const U = ctx->universe;
	FILE *const f = ctx->file;
	
	assert(U);
	assert(f);

	// Загрузка под-dag-а не влияет на список узлов dag-а текущего. А список
	// узлов под-dag-а идёт в LoadCurrent.refs, потому что именно именно
	// этим полем node (см. ниже NEWNODE) замыкает список атрибутов нового
	// узла

	const Ref r = loaddag(f, U, ctx->dagmap);
// 	assert(r.code == LIST);
	assert(r.code == DAG);

	return LC(nodes, r.u.list);
}

static LoadCurrent node(
	const LoadContext *const ctx, List *const env, List *const nodes)
{
	DBG(DBGNODE, "%s", "");

	assert(env && iskeymap(env->ref));

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

	const unsigned verb = loadtoken(U, f, 0, "[0-9A-Za-z]").u.number;
	const unsigned here = item;

	Ref *ref = NULL;

	// Надо получить следующий символ и, так получается, в любом случае
	// вернуть его обратно

	c = skipspaces(f);
	assert(ungetc(c, f) == c);

	DBG(DBGNODE, "cc: %c", c);
	
	// Если дальше следует метка узла

	if(isfirstid(c))
	{
		const Ref label = loadtoken(U, f, 0, "[0-9A-Za-z]");
		ref = keytoref(env, label);

		if(ref->code == FREE)
		{
			assert(ref->u.pointer == NULL);
		}
		else
		{
			ERR("node label is in scope: %s",
				atombytes(atomat(U, label.u.number)));
		}
	}

	// Загрузка атрибутов узла. Которые могут составлять либо список
	// атрибутов в текущем dag-е, либо под-dag

	const unsigned k = verbmap(ctx->dagmap, verb);
	const unsigned isdag = k != -1;

	DBG(DBGNODE, "(map verb -> k) -> isdag = (%p %s -> %u) -> %u",
		(void *)ctx->dagmap, atombytes(atomat(U, verb)), k, isdag);
	
	if(DBGFLAGS & DBGNODE)
	{
		dumpkeymap(1, stderr, 1, U, ctx->dagmap); 
	}

	const LoadCurrent lc
		= (!isdag ? loadattr : loadsubdag)(ctx, env, nodes);

	if(DBGFLAGS & DBGNODE)
	{
		char *const ns = strlist(NULL, lc.nodes);
		char *const rs = strlist(NULL, lc.refs);
		DBG(DBGNODE, "ns: %s", ns);
		DBG(DBGNODE, "rs: %s", rs);
		free(rs);
		free(ns);
	}

	const Ref n = newnode(verb, (isdag ? refdag : reflist)(lc.refs), here);
	List *const l = RL(markext(n));

	// Узел создан, и если под него зарезервирована метка в окружении, надо
	// бы его туда добавить

	if(ref)
	{
		*ref = markext(n);
	}
	
	return LC(append(lc.nodes, RL(n)), l);
}

Ref loaddag(
	FILE *const f, Array *const U, const Array *const dagmap)
{
	List *const env = NULL;
	List *const nodes = NULL;

	assert(f);
	assert(U);

	const LoadContext ctx =
	{
		.file = f,
		.universe = U,
		.dagmap = dagmap
	};

	int c;

	if((c = skipspaces(f)) != '(')
	{
		errexpect(c, ES("("));
	}

	DBG(DBGLRD, "pre list; ctx: %p", (void *)&ctx);

	DBG(DBGLRD, "%s", "-> LIST");
 	const LoadCurrent lc = list(&ctx, env, nodes);

	if(DBGFLAGS & DBGLRD)
	{
		DBG(DBGLRD, "%s", "dumping");

		char *const c = strlist(NULL, lc.refs);
		DBG(DBGLRD, "refs(%u): %s", listlen(lc.refs), c);
		free(c);
	}

	// FIXME: Список в виде списка ссылок не нужен
	freelist(lc.refs);

// 	return reflist(lc.nodes);
	return refdag(lc.nodes);
}
