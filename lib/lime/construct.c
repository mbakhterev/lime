#include "construct.h"

#include <assert.h>
#include <stdlib.h>

// Будем держать пулл звеньев для списков и пулл узлов. Немного улучшит
// эффективность. Списки будут кольцевые, поэтому храним только один указатель.
// Это указатель (ptr) на конец списка. На начало указывает ptr->next

static List * freelist = NULL;
static List * freenodes = NULL;

// Откусить первый элемент
static List * nipoff(List **const lptr) {
	List *const l = *lptr;
	assert(l);

	// Первый элемент здесь
	List *const n = l->next;

	if(l != n) {
		// Список из >1 элемента.
		l->next = n->next;
	}
	else {
		*lptr = NULL;
	}

	return 0;
}

static List * listalloc() {
	List *const p = malloc(sizeof(List));
	assert(p);
	p->code = FREE;
	return p;
}

static Node * nodealloc() {
	Node *const p = malloc(sizeof(Node));
	assert(p);
	p->code = (unsigned)FREE;
	return p;
}

List * newlist(unsigned code) {
	if(code != NODE) {
		List *const l
			= freelist ? nipoff(&freelist) : listalloc();
		assert(l->code == FREE);
		l->code = code;
		l->next = l;
		return l;
	}

	if(freenodes) {
		List *const l = nipoff(&freelist);
		assert(l->code == NODE && l->u.node->code == (unsigned)FREE);
		l->next = l;
		return l;
	}

	List *const l = listalloc();
	l->code = NODE;
	l->next = l;
	l->u.node = nodealloc();
	l->u.node->code = (unsigned)FREE;

	return l;
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


