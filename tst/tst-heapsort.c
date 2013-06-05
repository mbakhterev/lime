#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <lime/heap.h>
#include <lime/util.h>

unsigned item = 1;
unsigned field = 1;
const char *unitname = NULL;

static int cmp(const void *const D, const unsigned i, const unsigned j) {
	unsigned long x = ((unsigned long *)D)[i];
	unsigned long y = ((unsigned long *)D)[j];
	return 1 - (x == y) - ((x < y) << 1);
}

// static int cmp(const void *const D, const unsigned i, const unsigned j) {
// 	return cmpui(((unsigned long *)D)[i], ((unsigned long *)D)[j]);
// }

int main(int argc, char * argv[]) {
	const unsigned N = 65535;
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

		// Текущий элемент не должен быть меньше предыдущего
		printf("%016lx\t%u\n", (unsigned long)q, cmp(D, I[i], I[p]) > -1);
		p = i;
	}

	return 0;
}
