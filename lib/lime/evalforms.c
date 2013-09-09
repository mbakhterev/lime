#include "construct.h"
#include "util.h"

#include <assert.h>
#include <string.h>

#define FNODE	0
#define FPUT	1
#define FGPUT	2
#define FEPUT	3
#define FLOOK	4

static const char *const formverbs[] =
{
	[FNODE] = "F",
	[FPUT] = "FPut",
	[FGPUT] = "FGPut",
	[FEPUT] = "FEPut",
	[FLOOK] = "FLook",
	NULL
};

// EState - Evaluation State

#define AKEY 0
#define AFORMREF 1
#define ASIGNATURE 0
#define ADAG 1

#define DBGITERR 1

#define DBGFLAGS (DBGITERR)

typedef struct
{
	const List *const dag;
	const List *const signature;
} FEssence;

static unsigned isvoidessence(const FEssence e)
{
	if(e.dag == NULL)
	{
		assert(e.signature == NULL);
		return 1;
	}

	return 0;
}

static FEssence voidessence(void)
{
	return (FEssence) { .dag = NULL, .signature = NULL };
}

static FEssence extractfromnode(const Node *const n, const Array *const V)
{
	assert(n);

	switch(uireverse(V, n->verb))
	{
	case FLOOK:
	{
		const List *const attr = n->u.attributes;
		assert(attr && attr->ref.code == FORM);

		const Form *const f = attr->ref.u.form;
		assert(f);

		return (FEssence)
		{
			.dag = f->u.dag,
			.signature = f->signature
		};
	}
	}

	return voidessence();
}

static int isvalidkey(const Ref ref)
{
	return ref.code == LIST && ref.u.list && iscomparable(ref.u.list);
}

static int isvalidone(List *const l, void *const ptr)
{
	assert(l);
	return isvalidkey(l->ref);
}

static unsigned isvalidsignature(const Ref r)
{
	return r.code == LIST
		&& r.u.list
		&& forlist((List *)r.u.list, isvalidone, NULL, !0) == !0;
}

static FEssence extractfromlist(const List *const l, const Array *const V)
{
	assert(l);

	const unsigned len = 2;
	const Ref R[len + 1];
	const unsigned refcnt = writerefs(l, (Ref *)R, len + 1);

	if(!(refcnt == len + 1 && R[len].code == FREE))
	{
		return voidessence();
	}

	if(!isvalidsignature(R[ASIGNATURE]))
	{
		return voidessence();
	}

	const Ref d = R[ADAG];

	if(!(d.code == NODE
		&& d.u.node && uireverse(V, d.u.node->verb) == FNODE
		&& d.u.node->u.attributes))
	{
		return voidessence();
	}

	return (FEssence)
	{
		.dag = d.u.node->u.attributes,
		.signature = R[ASIGNATURE].u.list
	};
}

// extractform должна получить описание формы: граф и сигнатуру - из списка
// аргументов узла и убедиться, в процессе убеждаясь, что всё корректно. Чтобы
// вывод информации об ошибках оставался на одном уровне графа вызовов, функция
// возвращает voidessence, когда обнаруживает нарушение в структуре аргумента.
// Этой же политике следуют используемые функции: extractfromnode и
// extractfromlist

static FEssence extractform(const Ref ref, const Array *const V)
{
	assert(V);

	// Бывает всего два варианта аттрибутов, описывающих форму: ссылка на
	// узел .Flook или пара (signature list; .F (...))

	switch(ref.code)
	{
	case NODE:
		return extractfromnode(ref.u.node, V);

	case LIST:
		return extractfromlist(ref.u.list, V);
	}

	return voidessence();
}

typedef struct 
{
	const List *const env;
	const List *const ctx;
	const Array *const map;
	const Array *const verbs;
	Array *const universe;
} EState;


static void feputeval(const Node *const n, const EState *const st)
{
	assert(n);

	assert(st);
	assert(st->env && st->env->ref.code == ENV);
	assert(st->verbs);
	assert(st->universe);

	const List *const attr = n->u.attributes;
	assert(attr);

	// Ожидаемое количество
	const unsigned len = 2;
	const Ref R[len + 1];

	// Загрузка в массив и проверка длины
	const unsigned refcnt = writerefs(attr, (Ref *)R, len + 1);

	if(!(refcnt == len + 1 && R[len].code == FREE))
	{
		item = 10;
		ERR("%s", ".FEPut attribute structure is broken");
	}

	if(!isvalidkey(R[AKEY]))
	{
		item = n->line;
		ERR("%s", ".FEPut keyspec attribute structure is broken");
	}

	const FEssence fe = extractform(R[AFORMREF], st->verbs);

	if(isvoidessence(fe))
	{
		item = n->line;
		ERR("%s", ".FEPut formspec attribute structure is broken");
	}

	Ref *const r = formkeytoref(st->universe, st->env, R[AKEY].u.list, -1);

	if(r->code != FREE)
	{
		item = n->line;
		ERR("%s", "Form with key exists");
	}

// 	*r = (Ref)
// 	{
// 		.code = FORM,
// 		.u.form = newform(fe.dag, st->map, fe.signature)
// 	};

	// fork-и, потому что отправляем в окружение, где нужна твёрдая копия
	*r = newform(forkdag(fe.dag, st->map), st->map, forklist(fe.signature));
}

static void evalone(List *const l, void *const ptr)
{
	assert(l && l->ref.code == NODE);

	const EState *const st = ptr;
	assert(st);
	const Array *const V = st->verbs;
	assert(V);
	const Node *const n = l->ref.u.node;
	assert(n);
	
	const unsigned key = uireverse(V, n->verb);

	switch(key)
	{
	case FEPUT:
		feputeval(n, st);
	}
}

void evalforms(
	Array *const U,
	const List *const dag, const Array *const map, const Array *const go,
	const List *const env, const List *const ctx)
{
// Это надо проверять по ходу дела. В ситуации больше динамики, чем
// предполагают эти проверки. Откладываем их до конкретных функций
// 
// 	assert(env && env->ref.code == ENV);
// 	assert(ctx && ctx->ref.code == CTX);

	const Array verbs = keymap(U, 0, formverbs);

	const EState st =
	{
		.env = env,
		.ctx = ctx,
		.map = map,
		.verbs = &verbs,
		.universe = U
	};
	
	walkdag(dag, map, go, evalone, (void *)&st);

	freeuimap((Array *)&verbs);
}

typedef struct
{
	const Array *const U;
	const List *const ins;

	// В случае !external-формы, в этот список накопятся ссылки на ключи в
	// ins

	List *L;

	// Флаг external
	const unsigned ext;
} IIState;

// static int intakesigone(List *const sig, void *const ptr)
// {
// 	IIState *const st = ptr;
// 	assert(st);
// 	assert(st->ins
// 		&& st->ins->ref.code == ENV && st->ins->ref.u.environment);
// 	assert(st->U);
// 	
// 	// Сигнатуру глубоко не assert-им по максимуму, потому что, есть assert
// 	// в keytoref, ну и она пришла извне, там должны быть проверки тоже
// 
// 	assert(sig && sig->ref.code == LIST);
// 	
// 	// FIXME: надо ещё раз разобраться с режимом поиска в keytox
// 
// 	Binding *const B = keytobinding(st->ins, sig->ref.u.list, -1);
// //	Ref *const r = keytoref(st->ins, sig->ref.u.list, -1);
// 	Ref *const r = &B->ref;
// 
// 	if(r->code != FREE)
// 	{
// 		char *const str = listtostr(st->U, sig->ref.u.list);
// 		DBG(DBGITERR, "signature: %s\n", str);
// 		free(str);
// 
// 		ERR("%s", "input signature conflict");
// 
// 		return 1;
// 	}
// 
// 	// Если место пустое, добавляем ссылку на форму... FIXME: МЛИН, а
// 	// формы-то тут и нет
// 
// 	return 0;
// }
// 
// static const List *intakesig(
// 	const Array *const U,
// 	const List *const ins, const List *const sig, const unsigned ext)
// {
// 	assert(U);
// 
// 	IIState st =
// 	{
// 		.ins =  ins, .ext = ext, .U = U, .L = NULL
// 	};
// 
// 	forlist((List *)sig, intakesigone, &st, 0);
// 
// 	return ext ? sig : st.L;
// }

// Перечислить все Binding-и в окружении env, которые соответствуют ключам в
// списке signature

typedef struct
{
	const List *const env;
	Binding **const B;
	const unsigned N;
	unsigned nth;
} LBState;

static int listonebinding(List *const k, void *const ptr)
{
	// Проверяем сигнатуру по-минимуму, потому что assert-ы есть и на уровне
	// выше, в evalforms, и ниже, в keytobinding.
	assert(k && k->ref.code == LIST);

	LBState *const st = ptr;
	assert(st);
	assert(st->B);

	// Потому что рассчитываем, что в последнем элементе должна быть
	// записана Binding-пустышка

	assert(st->N > st->nth + 1);
	
	st->B[st->nth]
		= keytobinding(st->env, k->ref.u.list, -1);

	st->nth += 1;

	return 0;
}

static void listbindings(
	const List *const env, const List *const sig,
	Binding *B[], const unsigned N)
{
	LBState st = { .env = env, .B = B, .N = N, .nth = 0 };
	forlist((List *)sig, listonebinding, &st, 0);
	assert(st.N == st.nth + 1);
	B[st.nth] = NULL;
}

static Ref refpass(const Ref r)
{
	return r;
}

static List *dagpass(const List *const dag, const Array *const map)
{
	return (List *)dag;
}

static const List *sigselect(
	const List *const sig, Binding *const B[], const unsigned external)
{
	// Если речь о сигнатуре для external-формы, её и возвращаем

	if(external)
	{
		return sig;
	}

	// В другом случае строим эту сигнатуру из ключей в Binding-ах из
	// окружения. Каждый ключ в ней надо пометить как external, чтобы потом
	// freeform освобождала сигнатуру корректно

	List *l = NULL;

	for(unsigned i = 0; B[i]; i += 1)
	{
		l = append(l, RL(markext(reflist((List *)B[i]->key))));
	}

	return l;
}

void intakeform(
	const Array *const U,
	Context *const ctx, const unsigned level,
	const List *const dag, const Array *const map,
	const List *const signature, const unsigned external)
{
	assert(ctx);
	assert(level < sizeof(ctx->R) / sizeof(Reactor));

	Reactor *const R = ctx->R + level;

// 	// Содзадём ссылку на форму, с учётом external. В предположении, что в
// 	// intakeform всё приходит уже проверенным из evalforms (проверки там
// 	// есть и перед размещением в окружении и проверка будет перед
// 	// непосредственным вызовом intakeform в обработке fput
// 
// 	const Ref f
// 		= (external ? markext : refpass)(
// 			newform(
// 				(external ? forkdag : dagpass)(dag, map), map,
// 				intakesig(U, R->ins, signature, external)));

	const unsigned N = listlen(signature) + 1;
	Binding *const B[N];
	listbindings(R->ins, signature, (Binding **)B, N);
	assert(B[N-1] == NULL);

	const List *const fsig 
		= sigselect(signature, B, external);
	
	// Ок. Есть нужная сигнатура. Она по-прежнему может быть не особо
	// корректной, потому что в Binding-и могут быть не свободными. Но
	// сперва сотворим форму, а потом разберёмся с этим

	const Ref f
		= (external ? markext : refpass)(newform(
			(external ? dagpass : forkdag)(dag, map), map, fsig));
	
	// Ок. Форма есть, сигнатура есть. Теперь надо пройтись по массиву и
	// записать на нужные места указатель на эту форму. В этом цикле
	// проверяем и свободу связок

	unsigned i;
	for(i = 0; B[i] && B[i]->ref.code == FREE; i += 1)
	{
		B[i]->ref = f;
	}

	// Если дошли до конца, то есть B[i] == NULL, то можно добавлять форму в
	// список форм и успешно завершать работу

	if(B[i] == NULL)
	{
		R->forms = append(R->forms, RL(f));
		return;
	}

	// Иначе, имеем дело с ошибкой. Надо почистить ресурсы и сообщить о
	// ней. Ресурсы у нас сохранены в указателях dag и signature формы,
	// поэтому надо только её чистить

	freeform(f);

	char *const sig = listtostr(U, B[i]->key);
	DBG(DBGITERR, "signature: %s", sig);
	free(sig);
	ERR("%s", "signature duplication detected");
}
