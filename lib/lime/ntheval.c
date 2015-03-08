#include "construct.h"
#include "util.h"
#include "nodeutil.h"

#include <assert.h>

#define DBGNTH 1
#define DBGIDX 2

// #define DBGFLAGS (DBGNTH | DBGIDX)

#define DBGFLAGS 0

typedef struct
{
	const unsigned from;
	const unsigned to;
	const unsigned valid;
} Index;

static unsigned reftoidx(
	const Ref r, const Array *const verbs, unsigned *const correct)
{
	switch(r.code)
	{
	case NUMBER:
		assert(r.u.number < MAXNUM);
		*correct = !0;
		return r.u.number;
	
	case ATOM:
		// Нам здесь везёт, что конец списка обозначается так же, как и
		// узел T

		*correct = verbmap(verbs, r.u.number) == TNODE;
		return -1;
	
	default:
		*correct = 0;
		return -1;
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
		const Ref R[2];
		if(!splitpair(r, (Ref *)R))
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
		const Ref r = l->ref;
		
		// Немного паранойи
		assert(r.code != NODE || r.external);

		l->ref = reffree();
		freelist(l);

// 		return l->ref;
		return r;
	}

	return reflist(l);
}

void dofin(
	Marks *const M,
	const Ref N, const Core *const C, const List *const inputs)
{
	const Array *const U = C->U;
	Array *const marks = M->marks;

	const Ref r = nodeattribute(N);
	if(r.code != LIST || listlen(r.u.list) != 0)
	{
		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	// tunrefmap запомнит ссылку на список как external. Освобождением
	// списка нужно управлять вне eval

	tunerefmap(marks, N, reflist((List *)inputs));
}

typedef struct
{
	const Array *const U;
	const Array *const verbs;
	const Array *const types;
	const Array *const symbols;
	const Array *const marks;
	Ref L;
} DCThread;

static int indexone(List *const, void *const);

void donth(Marks *const M, const Ref N, const Core *const C)
{
	Array *const marks = M->marks;
	const Array *const U = C->U;
	const Array *const V = C->verbs.system;

	const Ref r = nodeattribute(N);
	if(r.code != LIST)
	{
		item = nodeline(N);
		ERR("node \"%s\": expecting attribute list", nodename(U, N));
		return;
	}

	const unsigned len = listlen(r.u.list);
	const Ref R[len];
	assert(writerefs(r.u.list, (Ref *)R, len) == len);

	// Здесь надо учитывать тонкости. В src может получится либо одиночная
	// Ref-а, и тогда с освобождением th.L в indexone будет всё хорошо. В
	// том смысле, что никакого освобождения не будет. Либо может получится
	// список. Списки могут трёх видов: (1) Ref.external - такой список
	// получается после трансляции узла через marks (refmap всегда делает
	// markext); (2) !Ref.external - такой список получится, если в R[0] был
	// список. В обоих случаях в indexone можно безопасно вызывать freeref
	// после вырезания из src кусочка.

	const Ref src = len > 0 ? simplerewrite(R[0], marks, V) : reffree();

	if(len != 2 || src.code == FREE || R[1].code != LIST)
	{
		freeref(src);

		item = nodeline(N);
		ERR("node \"%s\": wrong attribute structure", nodename(U, N));
		return;
	}

	DCThread th =
	{
		.U = U,
		.verbs = C->verbs.system,
		.types = C->T,
		.symbols = C->S,
		.marks = marks,
		.L = src
	};

	if(forlist(R[1].u.list, indexone, &th, 0))
	{
		freeref(th.L);

// 		item  = nodeline(N);
// 		ERR("node \"%s\": value-index mismatch", nodename(U, N));

		ERRNODE(U, N, "%s", "value-index mismatch");

		return;
	}

	// Запоминаем в такой форме, чтобы передать ответственность за
	// освобождение полученного выражения в marks

	Binding *const b
		= (Binding *)bindingat(marks, mapreadin(marks, markext(N)));
	assert(b);

	b->ref = th.L;
}

static Ref reftolist(const Ref r, const DCThread *const th);

int indexone(List *const i, void *const ptr)
{
	assert(ptr);
	DCThread *const th = ptr;

	assert(i);
	const Index idx = index(i->ref, th->verbs);
	if(!idx.valid)
	{
		return !0;
	}

	DBG(DBGIDX, "from to: %u %u", idx.from, idx.to);

	const Ref src = reftolist(th->L, th);

	if(DBGFLAGS & DBGIDX)
	{
		char *const sstr = strref(th->U, NULL, src);
		DBG(DBGIDX, "src: %s", sstr);
		free(sstr);
	}

	if(src.code != LIST)
	{
		return !0;
	}

	unsigned valid = 0;
	List *const l = forklistcut(src.u.list, idx.from, idx.to, &valid);

	if(DBGFLAGS & DBGIDX)
	{
		char *const sstr = strref(th->U, NULL, src);
		char *const lstr = strref(th->U, NULL, reflist(l));
		DBG(DBGIDX, "valid: %u; src: %s -> l: %s", valid, sstr, lstr);
		free(sstr);
		free(lstr);
	}

	if(!valid)
	{
		freelist(l);
		return !0;
	}

	// Это корректное освобождение: cf. комментарий в ntheval и код
	// reftolist
	freeref(src);	

	th->L = reexpress(l);

	return 0;
}

Ref reftolist(const Ref r, const DCThread *const th)
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

		const Binding *const b = typeat(th->types, r);
		return reflist(RL(
			markext(b->key),
			b->ref.code != FREE ? markext(b->ref) : reflist(NULL)));
	}

	case SYM:
		return reflist(RL(
			symname(th->symbols, r),
			symtype(th->symbols, r)));
	}

	// Нечто иное в список для индексирования превратить нельзя
	return reffree();
}
