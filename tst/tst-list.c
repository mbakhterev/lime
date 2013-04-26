#include <lime/construct.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static int printer(List *const l, void *const ptr) {
	switch(l->ref.code) {
	case NUMBER:
		printf("N: %u\n", l->ref.u.number);
		break; 

	case ATOM:
		printf("A: %u\n", l->ref.u.number);
		break; 

	case TYPE:
		printf("A: %u\n", l->ref.u.number);
		break; 

	case NODE:
		printf("NODE\n");
		break;

	case LIST:
		forlist(l->ref.u.list, printer, NULL, 0);

	default:
		assert(0);
	}

	return 0;
}

static int checkfree(List *const l, void *const ptr) {
	assert(l->ref.code == FREE);
	return 0;
}

int main(int argc, char * argv[]) {
	List * l = NULL;
	for(int i = 0; i < 20; i += 1) {
		List *const k = RL(refnum(NUMBER, i));

//		k->u.number = i;

		forlist(k, printer, NULL, 0);
		l = append(l, k);
	}
	printf("---\n");

	char *c = dumplist(l);
	printf("l: %s\n", c);
	free(c);

	List *const k = forklist(l);
	printf("k forked\n"); fflush(stdout);
	c = dumplist(k);
	printf("k: %s\n", c);
	free(c);

	freelist(k);
	forlist(k, checkfree, NULL, 0);

	List *const m = forklist(l);
	printf("m: %s\n", dumplist(m));

	List *const n = forklist(m);
	printf("n: %s\n", c = dumplist(n));
	free(c);

	Ref R[21];
	unsigned ncnt = writerefs(n, R, 21);
	assert(ncnt == 21);
	printf("R:");
	for(unsigned i = 0; R[i].code != FREE; i += 1) {
		assert(R[i].code == NUMBER);
		printf(" %u", R[i].u.number);
	}
	printf("\n");

	List *const o = readrefs(R);
	printf("o: %s\n", c = dumplist(o));

	printf("len(o): %u\n", listlen(o));

	return 0;
}
