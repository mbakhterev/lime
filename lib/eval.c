#include "util.h"
#include "core.h"
#include "construct.h"

static const char *const limeverbs[] =
{
	[LNODE]	= "LNODE",
	[FIN]	= "FIn",
	[NTH]	= "Nth",
	[FNODE]	= "F",
	[FENV]	= "FEnv",
	[FOUT]	= "FOut",
	[TNODE]	= "T",
	[TENV]	= "TEnv",
	[ENODE]	= "E",
	[ENV]	= "Env",
	[SNODE]	= "S",
	[RNODE]	= "R",
	[RIP]	= "Rip",
	[DONE]	= "Done",
	[GO]	= "Go",
	NULL
};

static const char *const rvgeneric[] = 
{
	"L", "FIn", "Nth", "T", "TEnv", "E", "S", NULL
};

static const char *const escape[] =
{
	"F", NULL
};

static void initroot(Array *const U, Array *const E)
{
	Array *const R = newkeymap();

	assert(linkmap(U, R,
		readtoken(U, "ENV"), readtoken(U, "this"), refkeymap(R)) == R);
	
	DL(rootname, RS(readtoken(U, "/")));
	const unsigned bid = bindkey(E, rootname);
	assert(bid == 0);
	
	Binding *const B = (Binding *)bindingat(E, bid);
	assert(b->ref.code == FREE);
	B->ref = refkeymap(R);
}

Core *newcore(const unsigned dip)
{
	Array *const U = newatomtab();
	Array *const E = newkeymap();

	initroot(U, E);

	const Core C =
	{
		.U = U,
		.T = newkeymap(),
		.E = E,
		.S = newkeymap(),
		.areastack = NULL,
		.activity = newkeymap(),
		.envtogo = 0,
		.limeverbs = newverbmap(U, 0, limeverbs),
		.escape = newverbmap(U, 0, escape),
		.reverbs.generic = newverbmap(U, 0, rvgeneric),
		.dumpinfopath = dip
	};

	Core *const pc = malloc(sizeof(Core));
	assert(pc);
	memcpy(pc, &C, sizeof(Core));
	
	return pc;
}

void freecore(Core *const C)
{
	freeatomtab(C->U);
	freekeymap(C->T);
	freekeymap(C->S);

// FIXME: Необходимо почистить сами окружения
	freekeymap(C->E);

// FIXME: Нужно почистить области вывода
	freelist(C->areastack);

	freekeymap(C->activity);

	freekeymap((Array *)C->limeverbs);
	freekeymap((Array *)C->escape);
	freekeymap((Array *)C->reverbs.generic);
}

typedef struct
{
	Array *const envmarks;
	Array *const envtokeep;
	Array *const marks;
	
	Array *const env,
	Array *const area,

	Core *const C;

	List *L;

	const unsigned mode;
} EState;

static int stageone(List *const l, void *const ptr)
{
	assert(l && isnode(l->ref) && !l->ref.external);

	assert(ptr);
	EState *const E = ptr;
	const Core *const C = E->C;

	switch(nodeverb(l->ref, C->verbs.system))
	{
	case ENV:
		doenv(E->envmarks, E->envtokeep, l->ref, C);
		break;
	}

	return 0;
}

static Array *envfornode(
	const Ref N,
	const Array *const env,
	const Array *const marks, const Array *const envmarks,
	const Array *const environments)
{
	const Ref enode = refmap(envmarks, N);
	if(enode.code == FREE)
	{
		return (Array *)env;
	}

	assert(isnode(enode));

	const Ref eref = refmap(marks, enode);
	if(eref.code == FREE)
	{
		item = nodeline(N);
		ERR("node \"%s\": environment isn't evaluated", nodename(U, N));
		return NULL;
	}

	return envat(environments, eref);
}

static List *dogeneric(
	Array *const env, Array *const area, Core *const C,
	const Ref N, const Array *const marks)
{
	const unsigned verb = nodeverb(N);
	const unsigned line = nodeline(N);

	const Ref r = nodeattribute(N);
	if(r.code == DAG)
	{
		const Ref M = newnode(verb, eval(env, area, C, r, EMDAG), line);
		tunerefmap(marks, N, M);
		return RL(M);
	}

	const Ref attr = exprewrite(r, marks);
	if(attr.code == FREE)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return NULL;
	}

	const Ref M = newnode(verb, attr, line);
	tunerefmap(marks, N, M);
	return RL(M);
}

static int stagetwo(List *const l, void *const ptr)
{
	assert(l && isnode(l->ref) && !l->ref.external);

	assert(ptr);
	EState *const E = ptr;
	const Core *const C = E->C;

	switch(nodeverb(l->ref, C->verbs.system))
	{
	case ENV:
		if(setmap(E->envtokeep, l->ref))
		{
			List *const l
				= dogeneric(l->ref, C, E->marks, E->envmarks);

			E->L = append(E->L, l);
		}
	}
}
