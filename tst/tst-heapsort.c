#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <lime/heapsort.h>

static int cmp(const void *const D, const unsigned i, const unsigned j) {
	unsigned long x = ((unsigned long *)D)[i];
	unsigned long y = ((unsigned long *)D)[j];
	return 1 - (x == y) - ((x < y) << 1);
}

int main(int argc, char * argv[]) {
	const unsigned N = 65536;
	unsigned long D[N];
	unsigned I[N];

	srand(time(NULL));

	for(int i = 0; i < N; i += 1) {
		D[i] = (unsigned long)rand();
		I[i] = i;
	}

	heapsort(D, I, N, cmp);

	int p = 0;
	for(int i = 0; i < N; i += 1) {
		unsigned long q = D[I[i]];
		printf("%016lx\t%u\n", (unsigned long)q, cmp(D, p, i) > -1);
		p = i;
	}

	return 0;
}
