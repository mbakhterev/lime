#include "construct.h"

#include <stdlib.h>

static Node *freelist = NULL;

// Откусывание первого элемента из списка узлов, связанных u.nextfree.
// cf. lib/lime/list.c

static Node *tipoff(Node **const lptr) {
	Node *const l = *lptr;
	assert(l);

	Node *const n = l->u.nextfree;

	if(l != n) {
		l->u.nextfree = n->u.nextfree;
		return n;
	}

}

Node *newnode(construct unsigned code) {
	
}
