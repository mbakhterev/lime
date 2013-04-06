#include "construct.h"
#include "util.h"

#include <assert.h>
#include <stdlib.h>

#define DBGFE 1
#define DBGPOOL 2

// #define DBGFLAGS (DBGFE)
// #define DBGFLAGS (DBGPOOL)

#define DBGFLAGS 0

// Будем держать пулл звеньев для списков и пулл узлов. Немного улучшит
// эффективность. Списки будут кольцевые, поэтому храним только один указатель.
// Это указатель (ptr) на конец списка. На начало указывает ptr->next

static List * freeitems = NULL;

// Откусить первый элемент
static List * tipoff(List **const lptr) {
	List *const l = *lptr;
	assert(l);

	// Первый элемент здесь
	List *const n = l->next;

	if(l != n) {
		// Список из > 1 элемента.
		l->next = n->next;
	}
	else {
		*lptr = NULL;
	}

	return n;
}

List * newlist(const int code, const unsigned allocate) {
	List *l = NULL;

	if(freeitems) {
		DBG(DBGPOOL, "tipping off: %p", (void *)freeitems);

		l = tipoff(&freeitems);
		assert(l->code == FREE);
	}
	else {
		l = malloc(sizeof(List));
		assert(l);
	}

	l->code = code;
	l->next = l;

	switch(code) {
	case NUMBER:
	case ATOM:
	case TYPE:
		return l;

	case NODE:
		if(allocate) {
			l->u.node = newnode();
		}

		return l;
	}

	assert(0);

	return NULL;
}

List * extend(List *const k, List *const l) {
	assert(l);

	if(k) { } else {
		return l;
	}

	List *const h = k->next;

	// За последним элементом списка k должен следовать первый элемент l
	k->next = l->next;

	// А за последним элементом l должен идти первый элемент k;
	l->next = h;

	// конец нового списка - это l;

	return l;
}

int forlist(List *const k, Oneach fn, void *const ptr, const int key) {
	assert(k);
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

static int forkitem(List *const k, void *const ptr) {
	List **const lptr = ptr;
	List *l = newlist(k->code, 0);

	switch(l->code) {
	case NUMBER:
	case ATOM:
	case TYPE:
		l->u.number = k->u.number;
		break;

	case NODE:
		assert(k->u.node);
		l->u.node = k->u.node;
		break;
	
	case LIST:
		assert(k->u.list);
		l->u.list = forklist(k->u.list);
		break;
	
	default:
		assert(0);
	}

	*lptr = extend(*lptr, l);

	return 0;
}

List *forklist(const List *const k) {
	List *l = NULL;

	forlist((List *)k, forkitem, &l, 0);
	return l;
}

static int releaser(List *const l, void *const p) {
	l->code = FREE;

	DBG(DBGPOOL, "pooling: %p", l);

	l->next = l;
	freeitems = extend(freeitems, l);

	return 0;
}

void freelist(List *const l) {
	forlist(l, releaser, NULL, 0);
}

static int dumper(List *const l, void *const file);

typedef struct {
	FILE *file;
	const List *first;
} DumpState;

static void dumptostream(List *const l, FILE *const f) {
	assert(fputc('(', f) != EOF);

	DumpState s = { .file = f, .first = NULL };
	forlist(l, dumper, &s, 0);

	assert(fputc(')', f) != EOF);
}

static int dumper(List *const l, void *const state) {
	DumpState *const s = state;
	FILE *const f = s->file;
	unsigned isfinal;

	if(s->first) { 	
		isfinal = l->next == s->first;
	}
	else {
		isfinal = 0;
		s->first = l;
	}

	switch(l->code) {
	case NUMBER:
		assert(fprintf(f, "%u", l->u.number) > 0);
		break;
	
	case ATOM:
		assert(fprintf(f, "A:%u", l->u.number) > 0);
		break;
	
	case TYPE:
		assert(fprintf(f, "T:%u", l->u.number) > 0);
		break;
	
	case NODE:
		assert(l->u.node);
		assert(fprintf(f, "N:%p", (void *)l->u.node) > 0);
		break;
	
	case LIST:
		assert(l->u.list);
		dumptostream(l->u.list, f);
		break;

	default:
		assert(0);
	}

	if(!isfinal) {
		assert(fputc(' ', f) != EOF);
	}

	return 0;
}

char *dumplist(const List *const l) {
	char *buff = NULL;
	size_t length = 0;
	FILE *f = open_memstream(&buff, &length);
	assert(f);
	dumptostream((List *)l, f);
	assert(fputc(0, f) != EOF);
	fclose(f);
	return buff;
}
