#include "construct.h"
#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DBGFE 1
#define DBGPOOL 2

// #define DBGFLAGS (DBGFE)
// #define DBGFLAGS (DBGPOOL)

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

extern Ref refnum(const unsigned code, const unsigned n) {
	switch(code) {
	case NUMBER:
	case ATOM:
	case TYPE:
		break;
	
	default:
		assert(0);
	}

	return (Ref) { .code = code, .u.number = n };
}

extern Ref refenv(Array *const e) {
	return (Ref) { .code = ENV, .u.environment = e };
}

extern Ref refnode(Node *const n) {
	return (Ref) { .code = NODE, .u.node = n };
}

extern Ref reflist(List *const l) {
	return (Ref) { .code = LIST, .u.list = l };
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
	if(k) { } else
	{
		return l;
	}

	if(l) { } else
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
	if(k) { } else
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

typedef struct {
	List *list;
} FState;

static int forkitem(List *const k, void *const ptr) {
	FState *const fs = ptr;
	List *l = NULL;

	switch(k->ref.code) {
	case NUMBER:
	case ATOM:
	case TYPE:
		l = newlist(refnum(k->ref.code, k->ref.u.number));
		break;

	case NODE:
		assert(k->ref.u.node);
		l = newlist(refnode(k->ref.u.node));
		break;
	
	case LIST:
		assert(k->ref.u.list);
		l = newlist(reflist(forklist(k->ref.u.list)));
		break;
	
	default:
		assert(0);
	}

	fs->list = append(fs->list, l);

	return 0;
}

List *forklist(const List *const k) {
	FState fs = { .list = NULL };
	forlist((List *)k, forkitem, &fs, 0);
	return fs.list;
}

static int releaser(List *const l, void *const p) {
	l->ref.code = FREE;

	DBG(DBGPOOL, "pooling: %p", (void *)l);

	l->next = l;
	freeitems = append(freeitems, l);

	return 0;
}

void freelist(List *const l) {
	FState fs = { .list = NULL };
	forlist(l, releaser, &fs, 0);
}

static int dumper(List *const l, void *const file);

typedef struct {
	FILE *file;
	const List *first;
} DumpState;

static void dumptostream(List *const l, FILE *const f)
{
	assert(fputc('(', f) != EOF);

	DumpState s = { .file = f, .first = l != NULL ? l->next : NULL };
	forlist(l, dumper, &s, 0);

	assert(fputc(')', f) != EOF);
}

static int dumper(List *const l, void *const state)
{
	DumpState *const s = state;
	FILE *const f = s->file;

// 	unsigned isfinal;
// 
// 	if(s->first) { 	
// 		isfinal = l->next == s->first;
// 	}
// 	else {
// 		isfinal = 0;
// 		s->first = l;
// 	}

	const unsigned isfinal = l->next == s->first;

	switch(l->ref.code)
	{
	case NUMBER:
		assert(fprintf(f, "%u", l->ref.u.number) > 0);
		break;
	
	case ATOM:
		assert(fprintf(f, "A:%u", l->ref.u.number) > 0);
		break;
	
	case TYPE:
		assert(fprintf(f, "T:%u", l->ref.u.number) > 0);
		break;
	
	case NODE:
		assert(l->ref.u.node);

		assert(fprintf(f, "N:%p:%u",
			(void *)l->ref.u.node, l->ref.u.node->verb) > 0);

		break;
	
	case LIST:
		dumptostream(l->ref.u.list, f);
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

char *dumplist(const List *const l) {
	char *buff = NULL;
	size_t length = 0;
	FILE *f = newmemstream(&buff, &length);
	assert(f);
	dumptostream((List *)l, f);
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
