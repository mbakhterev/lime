#include "construct.h"
#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DBGFE 1
#define DBGPOOL 2
#define DBGMF 4
#define DBGFL 8

// #define DBGFLAGS (DBGFE)
// #define DBGFLAGS (DBGPOOL)
// #define DBGFLAGS (DBGMF)
// #define DBGFLAGS (DBGFL)

#define DBGFLAGS 0

// Будем держать пулл звеньев для списков и пулл узлов. Немного улучшит
// эффективность. Списки будут кольцевые, поэтому храним только один указатель.
// Это указатель (ptr) на конец списка. На начало указывает ptr->next

static List * freeitems = NULL;

List *tip(const List *const l)
{
	assert(l);
	return l->next;
}

// Откусить первый элемент
List *tipoff(List **const lptr) {
	List *const l = *lptr;
	assert(l);

	// Первый элемент здесь
	List *const n = l->next;

	if(l != n) {
		// Список из >1 элементов
		l->next = n->next;
	}
	else {
		*lptr = NULL;
	}

	// Закольцовываем в одноэлементный список
	n->next = n;

	return n;
}

Ref refnat(const unsigned code, const unsigned n)
{
	switch(code) {
	case NUMBER:
	case ATOM:
	case TYPE:
		break;
	
	default:
		assert(0);
	}

	return (Ref) { .code = code, .u.number = n, .external = 0 };
}

Ref refnum(const unsigned n)
{
	return (Ref) { .code = NUMBER, .u.number = n, .external = 0 };
}

Ref refatom(const unsigned n)
{
	return (Ref) { .code = ATOM, .u.number = n, .external = 0 };
}

Ref refenv(Array *const e)
{
	assert(e && e->code == ENV);
	return (Ref) { .code = ENV, .u.environment = e, .external = 0 };
}

Ref refnode(Node *const n)
{
	return (Ref) { .code = NODE, .u.node = n, .external = 0 };
}

Ref reflist(List *const l)
{
	return (Ref) { .code = LIST, .u.list = l, .external = 0 };
}

Ref refform(Form *const f)
{
	return (Ref) { .code = FORM, .u.form = f, .external = 0 };
}

extern Ref refptr(void *const f)
{
	return (Ref) { .code = PTR, .u.pointer = f, .external = 0 };
}

extern Ref refctx(Context *const c)
{
	return (Ref) { .code = CTX, .u.context = c, .external = 0 };
}

extern Ref markext(const Ref r)
{
	// switch для аккуратности, потому что во многих случаях Ref не должна
	// быть внешней. Случая сейчас вообще всего два: форма и список (для
	// ключей и аргументов .FIn)

	switch(r.code)
	{
	case LIST:
	case FORM:
		return (Ref)
		{ 
			.code = r.code,
			.external = 1,
			.u.pointer = r.u.pointer
		};

	default:
		assert(0);
	}

	return (Ref) { .code = FREE, .u.pointer = NULL, .external = 0 };
}

static List *newlist(const Ref r)
{
	switch(r.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
	case NODE:
	case LIST:
	case ENV:
	case PTR:
	case CTX:
		break;
	
	default:
		assert(0);
	}

	List *l = NULL;

	if(freeitems)
	{
		DBG(DBGPOOL, "tipping off: %p", (void *)freeitems);
		
		l = tipoff(&freeitems);
		assert(l->ref.code == FREE && l->next == l);
	}
	else
	{
		l = malloc(sizeof(List));
		assert(l);
		l->next = l;
	}

	l->ref = r;
	return l;
}

List *readrefs(const Ref R[]) {
	List *l = NULL;
	for(unsigned i = 0; R[i].code != FREE; i += 1) {
		l = append(l, newlist(R[i]));
	}
	return l;
}

List * append(List *const k, List *const l)
{
	if(k == NULL)
	{
		return l;
	}

	if(l == NULL)
	{
		return k;
	}

	List *const h = k->next;

	// За последним элементом списка k должен следовать первый элемент l
	k->next = l->next;

	// А за последним элементом l должен идти первый элемент k;
	l->next = h;

	// конец нового списка - это l;
	return l;
}

int forlist(List *const k, Oneach fn, void *const ptr, const int key)
{
	if(k == NULL)
	{
		return key;
	}

	List *p = k;
	List *q = k->next;
	int r;
	do {
		DBG(DBGFE, "k: %p; p: %p", (void *)k, (void *)p);

		p = q;
		q = p->next;
		r = fn(p, ptr);
	} while(r == key && p != k);

	return r;
}

// FState - fork/free state

typedef struct 
{
	List *list;
	const Array *const nodemap;
	const Ref *const nodes;
	const unsigned bound;
	const unsigned from;
	const unsigned to;
	unsigned count;
} FState;

static void assertmap(
	const Array *const M, const Ref N[], const unsigned bnd)
{
	if(M)
	{
		assert(N);
	}
	else
	{
		assert(N == NULL && bnd == 0);
	}
}

static int forkitem(List *const k, void *const ptr)
{
	assert(k);

	// external-маркер только для особых ситуаций, с ручным контролем за
	// составлением списков. Поэтому проверяем, что пользователь не
	// разошёлся уж слишком

	assert(!k->ref.external);

	FState *const fs = ptr;
	assert(fs);

	// Текущая позиция в списке
	const unsigned cur = fs->count;

	if(cur < fs->from)
	{
		// Не добрались до начала отрезка списка, продолжаем
		fs->count += 1;
		return 0;
	}

	List *l = NULL;

	switch(k->ref.code)
	{
	case NUMBER:
	case ATOM:
	case TYPE:
		l = newlist(k->ref);
		break;

	case NODE:
	{
		const Array *const M = fs->nodemap;
		const Ref *const N = fs->nodes;
		const unsigned bnd = fs->bound;

		assertmap(M, N, bnd);

		const Node *const n = k->ref.u.node;
 		assert(n);
	
		if(M)
		{
			const unsigned i = ptrreverse(M, n);
			assert(i < bnd);
			assert(N[i].code == NODE && N[i].u.node);
			l = newlist(N[i]);
		}
		else
		{
			l = newlist(k->ref);
		}
		break;
	}

	case LIST:
	{
		const Array *const M = fs->nodemap;
		const Ref *const N = fs->nodes;
		const unsigned bnd = fs->bound;

		// from и to работают для списка верхнего уровня. Под-списки
		// надо копировать целиком

		l = newlist(reflist(transforklist(k->ref.u.list, M, N, bnd)));
		break;
	}
	
	default:
		assert(0);
	}

	fs->list = append(fs->list, l);

	if(cur < fs->to)
	{
		// Если не добрались до последнего элемента, переходим к
		// следующему

		fs->count += 1;
		return 0;
	}

	// Дошли до последнего
	return 1;
}

static List *megafork(
	const List *const k, const unsigned from, const unsigned to,
	const Array *const M, const Ref N[], const unsigned bnd,
	unsigned *const correct);

List *forklist(const List *const k)
{
	return transforklist(k, NULL, NULL, 0);
}

List *transforklist(
	const List *const k,
	const Array *const M, const Ref N[], const unsigned bnd)
{
	unsigned correct = 0;
	List *const l = megafork(k, 0, -1, M, N, bnd, &correct);
	assert(correct);

	return l;
}

List *forklistcut(
	const List *const k, const unsigned from, const unsigned to,
	unsigned *const correct)
{
	return megafork(k, from, to, NULL, NULL, 0, correct);
}

List *megafork(
	const List *const k, const unsigned from, const unsigned to,
	const Array *const M, const Ref N[], const unsigned bnd,
	unsigned *const correct)
{
	assertmap(M, N, bnd);

	// Диапазон должен быть задан корректно. -1 -- самый большой unsigned

	if(from <= to && (to < MAXLEN || to == -1))
	{
	}
	else
	{
		*correct = 0;
		return NULL;
	}

	DBG(DBGMF, "diapason OK: %u -> %u", from, to);

	// Если k == NULL, то надо вернуть NULL. Но при этом всё будет корректно
	// только если (from; to) == (0; -1) -- только весь NULL-евой список
	// можно взять.

	if(k == NULL)
	{
		*correct = (from == 0) && (to == -1);
		return NULL;
	}

	// Здесь уже некоторая активность

	DBG(DBGMF, "list is sound: %p", (void *)k);

	// Может понадобится вырезать последний элемент из списка. Диапазон уже
	// проверен и известно, что список не NULL

	if(from == -1)
	{
		*correct = 1;
		return newlist(k->ref);
	}

	FState fs =
	{
		.list = NULL,
		.nodemap = M,
		.nodes = N,
		.bound = bnd,
		.from = from,
		.to = to,
		.count = 0
	};

	const unsigned rv = forlist((List *)k, forkitem, &fs, 0);

	// Успешными нужно считать два варианта:
	// 1.	to == -1 и rv == 0 -- это означает, что пройден весь список.
	// 2.	count == to и rv == -1 -- это означает, что прошли отрезок.

	DBG(DBGMF, "from: %u; to: %u; count: %u; rv: %u",
		fs.from, fs.to, fs.count, rv);

	if((fs.to == -1 && rv == 0) || (fs.count == to && rv == 1))
	{
		*correct = 1;
		return fs.list;
	}
	else
	{
		*correct = 0;
		freelist(fs.list);
		return NULL;
	}

	assert(0);
}

static int releaser(List *const l, void *const p)
{
	assert(l);

	switch(l->ref.code)
	{
	case LIST:
		// Если ссылка не на внешний список, то чистим его рекурсивно

		if(!l->ref.external)
		{
			freelist(l->ref.u.list);
		}

		break;
	}

	l->ref.code = FREE;

	DBG(DBGPOOL, "pooling: %p", (void *)l);

	l->next = l;
	freeitems = append(freeitems, l);

	return 0;
}

void freelist(List *const l)
{
	FState fs = { .list = NULL };
	forlist(l, releaser, &fs, 0);
}

static int dumper(List *const l, void *const file);

typedef struct {
	FILE *const file;
	const List *const first;
	const Array *const universe;
} DumpState;

static int dumper(List *const l, void *const state)
{
	DumpState *const s = state;
	assert(s);

	FILE *const f = s->file;
	assert(f);

	const Array *const U = s->universe;

	const unsigned isfinal = l->next == s->first;

	switch(l->ref.code)
	{
	case NUMBER:
		assert(fprintf(f, "%u", l->ref.u.number) > 0);
		break;
	
	case ATOM:
		if(U)
		{
			const Atom a = atomat(U, l->ref.u.number);
			assert(0 < 
				fprintf(f, "%02x.\"%s\"",
					atomhint(a), atombytes(a)));
		}
		else
		{
			assert(fprintf(f, "A:%u", l->ref.u.number) > 0);
		}

		break;
	
	case TYPE:
		assert(fprintf(f, "T:%u", l->ref.u.number) > 0);
		break;
	
	case NODE:
	{
		const Node *const n = l->ref.u.node;
		assert(n);
//		assert(l->ref.u.node);
		
		if(U)
		{
			const char *const verb
				= (char *)atombytes(atomat(U, n->verb));

			assert(fprintf(f, "N:%p.%s", (void *)n, verb) > 0);
		}
		else
		{
			assert(fprintf(f, "N:%p.%u", (void *)n, n->verb) > 0);
		}

		break;
	}
	
	case LIST:
		unidumplist(f, U, l->ref.u.list);
		break;

	default:
		assert(0);
	}

	if(!isfinal)
	{
		assert(fputc(' ', f) != EOF);
	}

	return 0;
}

void unidumplist(
	FILE *const f, const Array *const U, const List *const list)
{
	assert(fputc('(', f) != EOF);

	DumpState s =
	{
		.file = f,
		.first = list != NULL ? list->next : NULL,
		.universe = U
	};

	forlist((List *)list, dumper, &s, 0);

	assert(fputc(')', f) != EOF);
}

char *dumplist(const List *const l)
{
	char *buff = NULL;
	size_t length = 0;
	FILE *f = newmemstream(&buff, &length);
	assert(f);

	unidumplist(f, NULL, l);

	assert(fputc(0, f) != EOF);
	fclose(f);

	return buff;
}

// Состояние для прохода со счётчиками

typedef struct {
	Ref *refs;

	// N - ограничение сверху; n - пройдено элементов
	unsigned N;
	unsigned n;
} CState;

static int writer(List *const k, void *const ptr) {
	CState *const st = ptr;

	if(st->n < st->N) {
		st->refs[st->n] = k->ref;
		st->n += 1;
		return 0;
	}
	else {
		return 1;
	}
}

unsigned writerefs(const List *const l, Ref refs[], const unsigned N) {
	CState st = { .refs = refs, .N = N, .n = 0 };
	forlist((List *)l, writer, &st, 0);

	assert(N == st.N);
	assert(N >= st.n);

	if(st.n < N) {
		refs[st.n].code = FREE;

	 	// Обнуляем весь union
		memset(&refs[st.n].u, 0, sizeof(refs->u));
		return st.n + 1;
	}

	return st.n;
}

static int counter(List *const l, void *const ptr) {
	CState *const st = ptr;
	assert(st->n < st->N);
	st->n += 1;
	return 0;
}

unsigned listlen(const List *const l) {
	CState st = { .refs = NULL, .N = MAXNUM, .n = 0 };
	forlist((List *)l, counter, &st, 0);
	assert(st.n < MAXNUM);
	return st.n;
}

void formlist(List L[], const Ref R[], const unsigned N)
{
	assert(L && R);
	assert(N < MAXNUM);
	assert(R[N].code == FREE);

	if(N > 0)
	{

		// Чтобы на создаваемый список можно было ссылаться по адресу
		// первого элемента массива L, этот элемент (по семантике)
		// должен быть последней ссылкой в списке ссылок

		for(unsigned i = 1; i <= N - 1; i += 1)
		{
			DBG(DBGFL, "linking: %u -> %u; ref: %u",
				i - 1, i, R[i - 1].u.number);

			L[i - 1].next = &L[i];
			L[i].ref = R[i - 1];
		}

		L[N - 1].next = &L[0];
		L[0].ref = R[N - 1];
	}
}

char *listtostr(const Array *const U, const List *const l)
{
	char *buf= NULL;
	size_t sz = 0;
	FILE *const f = newmemstream(&buf, &sz);
	unidumplist(f, U, l);
	fclose(f);

	return buf;
}
