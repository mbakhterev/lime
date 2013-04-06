#include "construct.h"

#include <stdlib.h>

static Node *freenodes = NULL;

// Откусывание первого элемента из списка узлов, связанных u.nextfree.
// cf. lib/lime/list.c

static Node *tipoff(Node **const lptr) {
	Node *const l = *lptr;
	assert(l);

	Node *const n = l->u.nextfree;

	if(l != n) {
		l->u.nextfree = n->u.nextfree;
	}
	else {
		*lptr = NULL;
	}

	return n;

}

Node *newnode(void) {
	Node *n;

	if(freenodes) {
		n = tipoff(&freenodes);
		assert(n->code = FREE && n->mark == 0);
	}
	else {
		n = malloc(sizeof(Node));
		assert(n);
		n->code = FREE;
		n->mark = 0;
	}

	n->u.nextfree = n;

	return n;
}

void freenode(Node *const n) {
	assert(n && n->code != FREE);

	n->code = FREE;
	n->mark = 0;

	if(freenodes) { } else {
		n->u.nextfree = n;
		freenodes = n;
		return;
	}

	freenodes->u.nextfree = n;
	n->u.nextfree = freenodes;
	freenodes = n;
}
