#include <stdio.h>
#include <stdlib.h>

static unsigned clp2(unsigned n) {
	n -= 1;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return n + 1;
}

void clpprint(unsigned x) {
	printf("%08x : %08x\n", x, clp2(x));
}

int main(int argc, char * argv[]) {

	clpprint(0xffffffff);
	clpprint(0x0);

	printf("\n");

	for(int i = 0; i < 32; i += 1) {
		clpprint((1 << i) + rand() % (1 << i));
	}

	printf("\n");

	for(int i = 0; i < 32; i += 1) {
		clpprint(1 << i);
	}
	
	return 0;
}
