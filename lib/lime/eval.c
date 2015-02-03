#include "util.h"
#include "construct.h"

#include <assert.h>
#include <string.h>

static const char *const limeverbs[] =
{
	[LNODE]	= "L",
	[FIN]	= "FIn",
	[NTH]	= "Nth",
	[FNODE]	= "F",
	[FENV]	= "FEnv",
	[FOUT]	= "FOut",
	[FPUT]	= "FPut",
	[TNODE]	= "T",
	[TENV]	= "TEnv",
	[TDEF]	= "TDef",
	[ENODE]	= "E",
	[EDEF]	= "EDef",
	[SNODE]	= "S",
	[EX]	= "Ex",
	[EQ]	= "Eq",
	[UNIQ]	= "Uniq",
	[RNODE]	= "R",
	[RIP]	= "Rip",
	[DONE]	= "Done",
	[GO]	= "Go",
	NULL
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
	
	// FIXME: небольшой хак, чтобы не менять интерфейс makepath, но иметь
	// возможность всегда обращаться к корневому окружению по имени "/"

	assert(linkmap(U, R,
		readtoken(U, "ENV"), readtoken(U, "/"), refkeymap(R)) == R);
	
	DL(rootname, RS(readtoken(U, "/")));
	const unsigned bid = bindkey(E, rootname);
	assert(bid == 0);
	
	Binding *const B = (Binding *)bindingat(E, bid);
	assert(B->ref.code == FREE);
	B->ref = refkeymap(R);

	setenvid(U, R, 0);
}

Core *newcore(
	Array *const U,
	Array *const envmarks, const Array *const tomark,
	const unsigned dip)
{
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
		.envmarks = envmarks,
		.verbs.system = newverbmap(U, 0, limeverbs),
		.verbs.envmarks = tomark,
		.verbs.escape = newverbmap(U, 0, escape),
		.dumpinfopath = dip
	};

	Core *const pc = malloc(sizeof(Core));
	assert(pc);
	memcpy(pc, &C, sizeof(Core));
	
	return pc;
}

void freecore(Core *const C)
{
// 	freeatomtab(C->U);
	freekeymap(C->T);
	freekeymap(C->S);

// FIXME: Необходимо почистить сами окружения
	freekeymap(C->E);

// FIXME: Нужно почистить области вывода
	freelist(C->areastack);

	freekeymap(C->activity);

	freekeymap((Array *)C->verbs.system);
	freekeymap((Array *)C->verbs.escape);
}

Marks makemarks(void)
{
	return (Marks)
	{
		.marks = newkeymap(),
		.areamarks = newkeymap()
	};
}

void dropmarks(Marks *const M)
{
	freekeymap(M->areamarks);
	freekeymap(M->marks);
}

typedef struct
{
	Array *const envdefs;
	Array *const envkeep;

// 	Array *const marks;
// 	Array *const formmarks;
	
	Array *const area;

	Core *const C;

	List *L;

	const List *const inputs;
	const unsigned env;
	const unsigned mode;

	Marks M;
} EState;

static int stageone(List *const l, void *const ptr)
{
	assert(l && isnode(l->ref) && !l->ref.external);

	assert(ptr);
	EState *const E = ptr;
	const Core *const C = E->C;

	switch(nodeverb(l->ref, C->verbs.system))
	{
	case EDEF:
		doedef(E->envdefs, E->envkeep, l->ref, C);
		break;
	}

	return 0;
}

static List *dogeneric(
	Core *const, Array *const area, Marks *const,
	const Ref, const unsigned env, const List *const inputs,
	const unsigned mode);

static unsigned envfornode(
	const Ref N, const Array *const U,
	const unsigned env,
	const Array *const marks, const Array *const envmarks);

static const char *const modenames[] =
{
	[EMGEN] = "codegen",
	[EMDAG] = "subdag",
	[EMINIT] = "init",
	[EMFULL] = "full"
};

static int stagetwo(List *const l, void *const ptr)
{
	assert(l && isnode(l->ref) && !l->ref.external);
	const Ref N = l->ref;

	assert(ptr);
	EState *const E = ptr;

	Core *const C = E->C;
	Array *const U = C->U;
// 	Array *const types = C->T;
// 	Array *const environments = C->E;

	Array *const area = E->area;

// 	Array *const marks = E->marks;
// 	Array *const formmarks = E->formmarks;

	Marks *const M = &E->M;

	Array *const envdefs = E->envdefs;
	Array *const envkeep = E->envkeep;
	const unsigned env = E->env;
	const unsigned mode = E->mode;
	const List *const inputs = E->inputs;

	switch(nodeverb(N, C->verbs.system))
	{
	case EDEF:
		if(!setmap(envkeep, N))
		{
			break;
		}
		
		List *const l
			= dogeneric(C, area, M, N, env, inputs, mode);
		E->L = append(E->L, l);

		break;
	
	case ENODE:
		doenode(C, M, N, env);
		break;
	
	case TNODE:
		dotnode(C, M, N);
		break;
	
	case TDEF:
		dotdef(C, N, M);
		break;
	
	case TENV:
		dotenv(C, M, N, envfornode(N, U, env, M->marks, envdefs));
		break;
	
	case SNODE:
		dosnode(C, M, N, envfornode(N, U, env, M->marks, envdefs));
		break;
	
	case LNODE:
		dolnode(M, N, C);
		break;
	
	case FIN:
		if(mode == EMGEN || mode == EMINIT)
		{
			ERR("node \"%s\": can't eval in %s mode",
				nodename(U, N),
				modenames[mode]);

			return !0;
		}

		dofin(M, N, C, inputs);
		break;
	
	case NTH:
		donth(M, N, C);
		break;
	
	case UNIQ:
		douniq(C, M, N, envfornode(N, U, env, M->marks, envdefs));
		break;
	
	case EX:
		doex(C, M, N, envfornode(N, U, env, M->marks, envdefs));
		break;
	
	case EQ:
		doeq(M, N, C);
		break;
	
	case FNODE:
		break;

	case FENV:
		if(mode == EMGEN || mode == EMDAG)
		{
			ERR("node \"%s\": can't eval in %s mode",
				nodename(U, N), modenames[mode]);

			return !0;
		}

		dofenv(C, M, N, envfornode(N, U, env, M->marks, envdefs));
		break;
	
	case DONE:
		if(mode != EMFULL)
		{
			ERR("node \"%s\": can't eval in %s mode",
				nodename(U, N), modenames[mode]);

			return !0;
		}

		dodone(U, area, N);
		break;
	
	case GO:
		if(mode != EMFULL)
		{
			ERR("node \"%s\": can't eval in %s mode",
				nodename(U, N), modenames[mode]);

			return !0;
		}

		C->envtogo = dogo(U, N, area, M, C->envtogo);
		break;
	
	case RNODE:
		if(mode == EMGEN || mode == EMINIT)
		{
			ERR("node \"%s\": can't eval in %s mode",
				nodename(U, N), modenames[mode]);

			return !0;
		}

		dornode(C, area, M, N);
		break;
	
	case RIP:
		if(mode == EMGEN || mode == EMINIT)
		{
			ERR("node \"%s\": can't eval in %s mode",
				nodename(U, N), modenames[mode]);

			return !0;
		}

		E->L = append(E->L, dorip(U, N, M));
		break;
	
	case FOUT:
		if(mode == EMGEN || mode == EMINIT)
		{
			ERR("node \"%s\": can't eval in %s mode",
				nodename(U, N), modenames[mode]);

			return !0;
		}

		dofout(C, area, N, M);
		break;
	
	case FPUT:
		if(mode == EMGEN || mode == EMINIT)
		{
			ERR("node \"%s\": can't eval in %s mode",
				nodename(U, N), modenames[mode]);

			return !0;
		}

		dofput(C, area, N, M);
		break;

	default:
	{
		const unsigned nenv = envfornode(N, U, env, M->marks, envdefs);
		List *const l
			= dogeneric(C, area, M, N, nenv, inputs, mode);
		E->L = append(E->L, l);

		break;
	}
	}

	return 0;
}

static unsigned envfornode(
	const Ref N,
	const Array *const U,
	const unsigned env,
	const Array *const marks, const Array *const envdefs)
{
	const Ref enode = refmap(envdefs, N);
	if(enode.code == FREE)
	{
		return env;
	}

	assert(isnode(enode));

	const Ref eref = refmap(marks, enode);
	if(eref.code == FREE)
	{
		item = nodeline(N);
		ERR("node \"%s\": environment isn't evaluated", nodename(U, N));
		return -1;
	}

	assert(eref.code == ENV);
	return eref.u.number;
}

static List *dogeneric(
	Core *const C, Array *const area, Marks *const M,
	const Ref N, const unsigned env, const List *const inputs,
	const unsigned mode)
{
	const Array *const U = C->U;
	const Array *const V = C->verbs.system;

	Array *const marks = M->marks;

	const unsigned verb = nodeverb(N, NULL);
	const unsigned line = nodeline(N);

	const Ref r = nodeattribute(N);

	const unsigned dagmode = mode < EMDAG ? mode : EMDAG;

	const Ref attr
		= r.code == DAG ?
			  eval(C, area, r, env, inputs, dagmode)
			: simplerewrite(r, marks, V);

	if(attr.code == FREE)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return NULL;
	}

	const Ref n = newnode(verb, attr, line);
	tunerefmap(marks, N, n);

	if(knownverb(n, C->verbs.envmarks))
	{
		tunerefmap(C->envmarks, n, refenv(env));
	}

	return RL(n);
}

Ref eval(
	Core *const C, Array *const area,
	const Ref dag, const unsigned env, const List *const inputs,
	const unsigned mode)
{
	assert(isdag(dag));
	assert(env < C->E->count);
	assert(mode <= EMFULL);

	EState st =
	{
		.envdefs = newkeymap(),
		.envkeep = newkeymap(),
// 		.marks = newkeymap(),
// 		.formmarks = newkeymap(),
		.area = area,
		.C = C,
		.L = NULL,
		.inputs = inputs,
		.env = env,
		.mode = mode,
		.M = makemarks()
	};

	forlist(dag.u.list, stageone, &st, 0);
	forlist(dag.u.list, stagetwo, &st, 0);

// 	freekeymap(st.formmarks);
// 	freekeymap(st.marks);
	dropmarks(&st.M);
	freekeymap(st.envkeep);
	freekeymap(st.envdefs);

	return refdag(st.L);
}
