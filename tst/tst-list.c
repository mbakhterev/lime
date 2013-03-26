#include <lime/construct.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char * argv[]) {
	List * l = NULL;
	for(int i = 0; i < 20; i += 1) {
//		printf("%d\n", i);
		l = extend(l, newlist(NUMBER));
		l->u.number = i;
	}

	const List * k = l;
	do {
		k = k->next;
		printf("%u\n", k->u.number);
	} while(k != l);

	return 0;
}
