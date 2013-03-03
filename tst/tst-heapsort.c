#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <lime/heapsort.h>

static int cmp(const void *const a, const void *const b) {
	unsigned long x = (unsigned long)a;
	unsigned long y = (unsigned long)b;
	return 1 - (x == y) - ((x < y) << 1);
}

int main(int argc, char * argv[]) {
	const unsigned N = 65536;
	const void * B[N];

	srand(time(NULL));

	for(int i = 0; i < N; i += 1) {
		B[i] = (void *)(unsigned long)rand();
	}

	heapsort(B, N, cmp);

	const void *p = 0;
	for(int i = 0; i < N; i += 1) {
		const void *q = B[i];
		printf("%016lx\t%u\n", (unsigned long)q, cmp(p, q) > 0);
		p = q;
	}

	return 0;
}
