#include "heapsort.h"

// typedef int (*Cmp)(const void *const, const void *const);

static void swap(unsigned *const i, unsigned *const j) {
	unsigned t = *i;
	*i = *j;
	*j = t;
}

static void maxheapify(const void *const D, unsigned I[],
	const unsigned F, const unsigned T, Cmp cmp) { // From, To
	unsigned l = 2 * F + 1;
	unsigned r = 2 * F + 2;
	unsigned max = F;

	if(l <= T && cmp(D, I[l], I[F]) > 0) {
		max = l;
	}

	if(r <= T && cmp(D, I[r], I[max]) > 0) {
		max = r;
	}

	if(max != F) {
		swap(I + F, I + max);	
		maxheapify(D, I, max, T, cmp);
	}
}

static void buildheap(const void *const D, unsigned  I[],
	const unsigned N, Cmp cmp) {
	unsigned i = N / 2 + 1;
	do {
		i -= 1;
		maxheapify(D, I, i, N - 1, cmp);
	} while(i);
}

void heapsort(const void *const D, unsigned I[], const unsigned N, Cmp cmp) {
	if(N > 1) {
		buildheap(D, I, N, cmp);
		
		for(unsigned n = N - 1; n > 0; n -= 1) {
			swap(I + 0, I + n);
			maxheapify(D, I, 0, n - 1, cmp);
		}
	}
}
