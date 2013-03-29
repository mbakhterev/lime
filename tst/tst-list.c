#include <lime/construct.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static void printer(List *const l, const unsigned isfinal, void *const ptr) {
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
		foreach(l->u.list, printer, NULL);

	default:
		assert(0);
	}
}

static void checkfree(List *const l, const unsigned isfinal, void *const ptr) {
	assert(l->code == FREE);
}

int main(int argc, char * argv[]) {
	List * l = NULL;
	for(int i = 0; i < 20; i += 1) {
//		printf("%d\n", i);
		List *const k = newlist(NUMBER, 0);
		k->u.number = i;
		foreach(k, printer, NULL);
		l = extend(l, k);
	}
	printf("---\n");

// 	List *const k = forklist(l);
// 	foreach(l, printer, NULL);
// 	printf("---\n");
// 	foreach(k, printer, NULL);
// 

	char *c = dumplist(l);
	printf("l: %s\n", c);
	free(c);

	List *const k = forklist(l);
	c = dumplist(k);
	printf("k: %s\n", c);
	free(c);

	releaselist(k);
	foreach(k, checkfree, NULL);

	return 0;
}
