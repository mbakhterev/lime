#include "construct.h"
#include "util.h"

#include <assert.h>

#define RNODE	0
#define RIP	1
#define DONE	2
#define GO	3

static const char *const verbs[] =
{
	[RNODE]	= "R",
	[RIP]	= "Rip",
	[DONE]	= "Done",
	[GO]	= "Go",
	NULL
};

typedef struct
{
	Array *const U;
	Array *const area;
	Array *const areamarks;

	const Array *const escape;
	const Array *const verbs;
} REState;

static void revalcore(const Ref r, REState *const);

static int revalone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	revalcore(l->ref, ptr);
	return 0;
}

static unsigned maypass(Array *const U, const Array *const map)
{
	return isactive(U, map);
}

static Array *newtarget(Array *const U)
{
	return newarea(U);
}

static Array *nextpoint(Array *const U, const Array *const map)
{
	return arealinks(U, map);
}

static void mkarea(
	Array *const U, const Ref N, Array *const area, Array *const areamarks)
{
	const Ref r = nodeattribute(N);
	if(r.code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting attribute list", nodename(U, N));
	}

	const unsigned len = listlen(r.u.list);
	if(len < 1 || 2 < len)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting 1 or 2 attributes", nodename(U, N));
	}

	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len);

	Array *const target = len == 2 ? envmap(areamarks, R[1]) : NULL;

	// Пока предполагаем, что у окружений могут быть только простые имена.
	// Без типов. Второй аргумент должен быть ссылкой, которая оценивается в
	// область вывода

	if(R[0].code != LIST || !isbasickey(R[0])
		|| (len == 2 && (R[1].code != NODE || target == NULL)))
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
	}

// 	const Ref TR
// 		= len == 1 ? markext(refkeymap(newarea(U)))
// 		: len == 2 ? markext(refkeymap(target))
// 		: reffree();

// 	const Ref TR
// 		= len == 1 ? markext(refarea(newarea(U)))
// 		: len == 2 ? markext(refarea(target))
// 		: reffree();

	const Ref TR = target ? markext(refarea(target)) : reffree();

// 	assert(len == 1 || iskeymap(TR));
// 	assert(iskeymap(TR));
	
	Array *const T
		= makepath(area, U, readtoken(U, "CTX"), R[0].u.list, TR,
			maypass, newtarget, nextpoint);
	
	if(!T)
	{
		item = nodeline(N);
		ERR("node \"%s\": can't trace area path", nodename(U, N));
	}

	assert(len == 1 || T == target);
	
	tuneenvmap(areamarks, N, T);
}

static void revalcore(const Ref r, REState *const st)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		return;
	
	case LIST:
	case DAG:
		forlist(r.u.list, revalone, st, 0);
		return;
	
	case NODE:
		if(r.external)
		{
			return;
		}

		switch(nodeverb(r, st->verbs))
		{
		case RNODE:
			mkarea(st->U, r, st->area, st->areamarks);
			return;

		default:
			if(!knownverb(r, st->escape))
			{
				revalcore(nodeattribute(r), st);
			}
			return;
		}

	default:
		assert(0);
	}
}

extern void reval(
	Array *const U,
	Array *const area, Array *const areamarks,
	const Ref dag, const Array *const escape)
{
	REState st =
	{
		.U = U,
		.area = area,
		.areamarks = areamarks,
		.escape = escape,
		.verbs = newverbmap(U, 0, verbs),
	};

	revalcore(dag, &st);

	freekeymap((Array *)st.verbs);
}

typedef struct
{
	Array *const U;
	Array *const area;
	Array *const envtogo;
	const Array *const escape;
	const Array *const envmarks;
} GState;

static void goevalcore(const Ref r, GState *const);

static int goevalone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	goevalcore(l->ref, ptr);
	return 0;
}

static void done(Array *const U, const Ref N, Array *const area)
{
	// Надо проверить на активность, чтобы не допускать повторных Done-ов

	if(!isactive(area))
	{
		item = nodeline(N);
		ERR("node \"%s\": can't kill inactive area", nodename(U, N));
	}

	const Ref ctx = readtoken(U, "CTX");
	Array *const links = arealinks(U, area);
	assert(links);

	if(!unlinkmap(U, links, ctx, readtoken(U, "LINKS")))
	{
		item = nodeline(N);
		ERR("node \"%s\": can't kill interlinks", nodename(U, N));
	}

	DL(rkey, RS(readtoken(U, "R"), refnum(0)));
	assert(unlinkmap(U, links, ctx, rkey));

	markactive(U, area, 0);
}

static Array *go(
	Array *const U, const Ref N,
	const Array *const area,
	const Array *const envmarks, const Array *const envtogo)
{
	if(envtogo)
	{
		item = nodeline(N);
		ERR("node \"%s\": Go already specified", nodename(U, N));
		return;
	}

	if(!isontop(U, area))
	{
		item = nodeline(N);
		ERR("node \"%s\": can Go only from stack top", nodename(U, N));
		return;
	}

	const Ref r = nodeattribute(N);
	if(r.code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	const unsigned len = listlen(r.u.list);
	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len);
	if(len != 1 || R[0].code != NODE)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting 1st attribute to be node reference",
			nodename(U, N));
		return;
	}

	const Array *const target = envmap(st->envmarks, R[0]);
	if(!target)
	{
		item = nodeline(N);
		ERR("node \"%s\": no environment evaluation", nodename(U, N));
		return;
	}

	return target;
}

static void goevalcore(const Ref r, GState *const st)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		return;

	case LIST:
	case DAG:
		forlist(r.u.list, goevalone, st, 0);
	
	case NODE:
		if(r.external)
		{
			return;
		}

		switch(nodeverb(r, st->verbs))
		{
		case DONE:
			done(st->U, r, st->area);
			return;

		case GO:
			st->envtogo
				= go(st->U, r,
					st->area, st->envmarks, st->envtogo);
			return;

		default:
			if(!knownverb(r, st->escape))
			{
				goevalone(nodeattribute(r), st);
			}
			return;
		}

	default:
		assert(0);
	}
}
