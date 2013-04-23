#include <lime/construct.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static int printer(List *const l, void *const ptr) {
	switch(l->code) {
	case NUMBER:
		printf("N: %u\n", l->u.number);
		break; 

	case ATOM:
		printf("A: %u\n", l->u.number);
		break; 

	case TYPE:
		printf("A: %u\n", l->u.number);
		break; 

	case NODE:
		printf("NODE\n");
		break;

	case LIST:
		forlist(l->u.list, printer, NULL, 0);

	default:
		assert(0);
	}

	return 0;
}

static int checkfree(List *const l, void *const ptr) {
	assert(l->code == FREE);
	return 0;
}

int main(int argc, char * argv[]) {
	List * l = NULL;
	for(int i = 0; i < 20; i += 1) {
		List *const k = newlist(NUMBER, refnum(i));

//		k->u.number = i;

		forlist(k, printer, NULL, 0);
		l = extend(l, k);
	}
	printf("---\n");

	char *c = dumplist(l);
	printf("l: %s\n", c);
	free(c);

	List *const k = forklist(l, 0);
	c = dumplist(k);
	printf("k: %s\n", c);
	free(c);

	freelist(k, 0);
	forlist(k, checkfree, NULL, 0);

	List *const m = forklist(l, 0);
	printf("m: %s\n", dumplist(m));

	List *const n = forklist(m, 0);
	printf("n: %s\n", dumplist(n));

	return 0;
}
