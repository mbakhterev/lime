#include <lime/construct.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static int printer(List *const l, const unsigned isfinal, void *const ptr) {
// 	static int cnt = 0;
// 	if(cnt > 32) {
// 		exit(EXIT_FAILURE);
// 	}
// 
// 	cnt += 1;
	
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

static int checkfree(List *const l, const unsigned isfinal, void *const ptr) {
	assert(l->code == FREE);
	return 0;
}

int main(int argc, char * argv[]) {
	List * l = NULL;
	for(int i = 0; i < 20; i += 1) {
//		printf("%d\n", i);
		List *const k = newlist(NUMBER, 0);
		k->u.number = i;
		forlist(k, printer, NULL, 0);
		l = extend(l, k);
	}
	printf("---\n");

// 	List *const k = forklist(l);
// 	forlist(l, printer, NULL);
// 	printf("---\n");
// 	forlist(k, printer, NULL);
// 

	char *c = dumplist(l);
	printf("l: %s\n", c);
	free(c);

	List *const k = forklist(l);
	c = dumplist(k);
	printf("k: %s\n", c);
	free(c);

	freelist(k);
	forlist(k, checkfree, NULL, 0);

	return 0;
}
