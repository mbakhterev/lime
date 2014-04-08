#include "construct.h"
#include "util.h"

#include <assert.h>

#define DBGNTH 1
#define DBGIDX 2

// #define DBGFLAGS (DBGNTH | DBGIDX)

#define DBGFLAGS 0

#define FIN	0U
#define NTH	1U
#define SNODE	2U
#define TNODE	3U
#define TENV	4U

static const char *const verbs[] =
{
	[FIN] = "FIn",
	[NTH] = "Nth",
	[SNODE] = "S",
	[TNODE] = "T",
	[TENV] = "TEnv",
	NULL
};

typedef struct
{
	const Array *const U;
	const Array *const escape;
	const Array *const verbs;

	const List *const inlist;

	// Исходные данные для разбора. Символы и типы, из которых можно Nth-ами
	// извлечь компоненты

	const Array *const typemarks;
	const Array *const types;
	const Array *const symmarks;
	const Array *const symbols;
	
	// Соответствия узлов Nth и FIn выражениям

	Array *const evalmarks;

} NState;

typedef struct
{
	const unsigned from;
	const unsigned to;
	const unsigned valid;
} Index;

static void eval(const Ref, NState *const);

static int evalone(List *const l, void *const ptr)
{
	assert(l);
	assert(ptr);
	eval(l->ref, ptr);
	return 0;
}

static unsigned reftoidx(
	const Ref r, const Array *const verbs, unsigned *const correct)
{
	assert(r.code <= ATOM);
	*correct = 0;

	switch(r.code)
	{
	case NUMBER:
		assert(r.u.number < MAXNUM);
		*correct = 1;
		return r.u.number;
	
	case ATOM:
		// Нам здесь везёт, что конец списка обозначается так же, как и
		// узел T

		if(verbmap(verbs, r.u.number) == TNODE)
		{
			*correct = 1;
			return -1;
		}

		break;
	}

	assert(0);
	return -1;
}

static Index index(const Ref r, const Array *const verbs)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	{
		unsigned valid = 0;
		const unsigned idx = reftoidx(r, verbs, &valid);
		return (Index) { .from = idx, .to = idx, .valid = valid };
	}

	case LIST:
	{
		const unsigned len = listlen(r.u.list);
		const Ref R[len];
		writerefs(r.u.list, (Ref *)R, len);

		if(len != 2)
		{
			break;
		}

		unsigned valid[2];
		const unsigned from = reftoidx(R[0], verbs, valid);
		const unsigned to = reftoidx(R[1], verbs, valid + 1);

		return (Index)
		{
			.from = from,
			.to = to,
			.valid = valid[0] && valid[1]
		};
	}
	}

	return (Index) { .from = -1, .to = -1, .valid = 0 };
}

static Ref reexpress(List *const l)
{
	if(listlen(l) == 1)
	{
		return l->ref;
	}

	return reflist(l);
}

typedef struct
{
	Ref L;
	const Array *const verbs;
	const Array *const symmarks;
	const Array *const symbols;
	const Array *const types;
	const Array *const typemarks;
	const Array *const evalmarks;
} DCState;

static Ref reftolist(const Ref r, const DCState *const st)
{
	switch(r.code)
	{
	case LIST:
		// Если имеем дело со списком, то список и возвращаем. Флаг
		// external при этом можно сохранить (по идее)

		return r;
	
	case TYPE:
	{
		// В этом случае выдаём список из двух элементов: обозначения и
		// определения типа. Можно не беспокоится за external-бит,
		// потому что на каждом шаге indexone список копируется. Эти
		// биты полезны только для оптимизации

		const Binding *const b = typeat(st->types, r.u.number);
// 		return reflist(RL(markext(b->key), markext(b->ref)));
		return reflist(RL(markext(b->key),
			b->ref.code != FREE ? markext(b->ref) : reflist(NULL)));
	}
		
	case NODE:
	{
		// Когда мы имеем дело со ссылкой на узел, мы знаем, как
		// поступать с символами и типами

		const unsigned verb = nodeverb(r, st->verbs);

		if(verb == TNODE || verb == TENV)
		{
			// Убедимся, что для соответствующего узла уже вычислен
			// тип и сведём задачу к предыдущей

			const Ref t = refmap(st->typemarks, r);
			assert(t.code == TYPE);
			return reftolist(t, st);
		}

		if(verb == SNODE)
		{
// 			// Убедимся, что имеем дело с обработанным символом и
// 			// поступим с ним так же, как с типом
// 
// 			const Binding *const b = ptrmap(st->symmarks, r);
// 			assert(b);
// 			return reflist(RL(markext(b->key), markext(b->ref)));

			// Убедимся, что имеем дело с обработанным S-узлом. И
			// поступим с ним примерно так же, как с типом

			const Ref id = refmap(st->symmarks, r);
			if(id.code == FREE)
			{
				return reffree();
			}
			
			return reflist(RL(
				symname(st->symbols, id),
				symtype(st->symbols, id)));
		}

		// Мы можем так же иметь дело с FIn или Nth. Информация о них
		// уже должна быть в evalmarks. Нужно выдать копию списка из
		// этого отображения, потому что вызывающая indexone будет эту
		// информацию освобождать

		if(verb == FIN || verb == NTH)
		{
			return forkref(refmap(st->evalmarks, r), NULL);
		}
	}
	}

	// Нечто иное в список для индексирования превратить нельзя
	return reffree();
}

static int indexone(List *const i, void *const ptr)
{
	assert(i);
	assert(ptr);
	DCState *const st = ptr;
	const Index idx = index(i->ref, st->verbs);

	if(!idx.valid)
	{
		DBG(DBGIDX, "%s", "!idx.valid");

		// Прекращаем этот проход, если получили не пару индексов
		return 1;
	}

	DBG(DBGIDX, "cut index: (%u %u)", idx.from, idx.to);

	// По текущей Ref-е st->L нам надо получить список, который будет
	// отправлен на индексирование

	const Ref src = reftolist(st->L, st);

	if(DBGFLAGS & DBGIDX)
	{
		char *const srcstr = strref(NULL, NULL, src);
		char *const lstr = strref(NULL, NULL, st->L);

		DBG(DBGIDX, "L.code = %u: %s -> src.code = %u: %s",
			st->L.code, lstr, src.code, srcstr);

		free(lstr);
		free(srcstr);
	}

	if(src.code != LIST)
	{
		DBG(DBGIDX, "%s", "invalid source");

		return 1;
	}

	// Список, с которым следует работать, получен. Можно его порезать

	unsigned valid = 0;
	List *const l = forklistcut(src.u.list, idx.from, idx.to, &valid);

	if(!valid)
	{
		DBG(DBGIDX, "%s", "cutting failed");

		freelist(l);
		return 1;
	}

	// Сюда мы можем попасть если: (1) в st->L была ссылка на список, и
	// тогда src будет указывать на него, и тогда этот список будет
	// корректно освобождён; (2) в st->L - ссылка на узел, тогда мы её и не
	// трогаем (см. reftolist)

	freeref(src);

	// Здесь нужен небольшой грязный хак отождествления списков из одного
	// элемента с самим этим элементом

	st->L = reexpress(l);

	return 0;
}


static Ref deconstruct(const Ref N, NState *const S)
{
	const Ref r = nodeattribute(N);
	if(r.code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting attribute list",
			atombytes(atomat(S->U, nodeverb(N, NULL))));

		return reffree();
	}

	const unsigned len = listlen(r.u.list);

	if(len != 2)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure",
			atombytes(atomat(S->U, nodeverb(N, NULL))));

		return reffree();
	}

	const Ref R[len];
	writerefs(r.u.list, (Ref *)R, len);

	if(!isnode(R[0]) || !knownverb(N, S->verbs) || R[1].code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure",
			atombytes(atomat(S->U, nodeverb(N, NULL))));

		return reffree();
	}

	// Превращать Ref-ы узлов в списки выгоднее не здесь, а на каждом шаге
	// разбора и индексирования

	DCState st =
	{
		.L = R[0],
		.verbs = S->verbs,
		.types = S->types,
		.symbols = S->symbols,
		.symmarks = S->symmarks,
		.typemarks = S->typemarks,
		.evalmarks = S->evalmarks
	};

	if(forlist(R[1].u.list, indexone, &st, 0))
	{
		// Если forlist вернул не 0, значит, нечто пошло не так

		item = nodeline(N);
		ERR("node \"%s\": value-index mismatch",
			atombytes(atomat(S->U, nodeverb(N, NULL))));

		freeref(st.L);
		return reffree();
	}

	return st.L;
}

static void eval(const Ref N, NState *const S)
{
	switch(N.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		return;

	// Пока алгоритмы таковы, что не делаем здесь различий

	case LIST:
	case DAG:
		forlist(N.u.list, evalone, S, 0);
		return;
	
	case NODE:
		if(N.external)
		{
			// Традиционно, нам интересны только определения узлов
			return;
		}

		switch(nodeverb(N, S->verbs))
		{
		case FIN:
			tunerefmap(S->evalmarks, N, reflist((List *)S->inlist));
			return;

		case NTH:
			tunerefmap(S->evalmarks, N, deconstruct(N, S));
			return;

		default:
			if(!knownverb(N, S->escape))
			{
				eval(nodeattribute(N), S);
			}
		}

		return;
	
	default:
		assert(0);
	}
}

Ref ntheval(
	Array *const U,
	const Ref dag, const Array *const escape,
	const Array *const typemarks, const Array *const types,
	const Array *const symmarks, const Array *const symbols,
	const List *const inlist)
{
	if(DBGFLAGS & DBGNTH)
	{
		char *const istr = strlist(U, inlist);
		DBG(DBGNTH, "inlist: %s", istr);
		free(istr);
	}

	NState st =
	{
		.U = U,
		.escape = escape,
		.symmarks = symmarks,
		.symbols = symbols,
		.typemarks = typemarks,
		.inlist = inlist,
		.verbs = newverbmap(U, 0, verbs),
		.types = types,
		.evalmarks = newkeymap()
	};

	eval(dag, &st);

	if(DBGFLAGS & DBGNTH)
	{
		DBG(DBGNTH, "%s", "evaluated");
		dumpkeymap(1, stderr, 0, U, st.evalmarks, NULL);
	}

	const Array *const torewrite = newverbmap(U, 0, ES("FIn", "Nth"));
	const Ref r = exprewrite(dag, st.evalmarks, torewrite);

	DBG(DBGNTH, "%s", "rewritten");

	freekeymap((Array *)torewrite);
	freekeymap(st.evalmarks);
	freekeymap((Array *)st.verbs);

	return r;
}
