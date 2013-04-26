#include "construct.h"

#include <stdlib.h>
#include <assert.h>

static Node *freenodes = NULL;

// Откусывание первого элемента из списка узлов, связанных u.nextfree.
// cf. lib/lime/list.c

static Node *tipoffnode(Node **const lptr) {
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

Node *newnode(const unsigned verb, const List *const attributes) {
	assert(attributes);
	assert(verb != FREE);

	Node *n = NULL;

	if(freenodes) {
		n = tipoffnode(&freenodes);
		assert(n->verb = FREE && n->mark == 0);
	}
	else {
		n = malloc(sizeof(Node));
		assert(n);
		n->verb = FREE;
		n->mark = 0;
	}

	n->u.nextfree = n;

	n->verb = verb;
	n->u.attributes = (List *)attributes;

	return n;
}

void freenode(Node *const n) {
	assert(n && n->verb != FREE);

	n->verb = FREE;
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
