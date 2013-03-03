#include "heapsort.h"

typedef int (*Cmp)(const void *const, const void *const);

static void swap(const void * *const a, const void * *const b) {
	const void *const t = *a;
	*a = *b;
	*b = t;
}

static void maxheapify(const void * A[],
	const unsigned F, const unsigned T, Cmp cmp) { // From, To
	unsigned l = 2 * F + 1;
	unsigned r = 2 * F + 2;
	unsigned max = F;

	if(l <= T && cmp(A[l], A[F]) > 0) {
		max = l;
	}

	if(r <= T && cmp(A[r], A[max]) > 0) {
		max = r;
	}

	if(max != F) {
		swap(A + F, A + max);	
		maxheapify(A, max, T, cmp);
	}
}

static void buildheap(const void * A[], const unsigned N, Cmp cmp) {
	unsigned i = N / 2 + 1;
	do {
		i -= 1;
		maxheapify(A, i, N - 1, cmp);
	} while(i);
}

void heapsort(const void * A[], const unsigned N, Cmp cmp) {
	if(N > 1) {
		buildheap(A, N, cmp);
		
		for(unsigned n = N - 1; n > 0; n -= 1) {
			swap(A + 0, A + n);
			maxheapify(A, 0, n - 1, cmp);
		}
	}
}
