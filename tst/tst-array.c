#include <lime/construct.h>
#include <lime/util.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

unsigned item = 1;
unsigned field = 1;
const char *unitname = NULL;

typedef struct
{
	unsigned key;
	unsigned char b;
	unsigned c;
} Some;

static int icmp(const void *const D, const unsigned i, const unsigned j)
{
	const Some *const d = D;
	return cmpui(d[i].key, d[j].key);
}

static int kcmp(const void *const D, const unsigned i, const void *const key)
{
	const Some *const d = D;
	const unsigned k = *(unsigned *)key;
	return cmpui(d[i].key, k);
}

int main(int argc, char * argv[])
{
	const unsigned N = 1 << 13;

	Array *const a = newarray(0, sizeof(Some), icmp, kcmp);
	Some s;

	s.key = (unsigned)-1;
	readin(a, &s);

	for(unsigned i = 1; i < N; i += 1)
	{
		s.key = rand() % 2345223234;
		readin(a, &s);
		if(i % 1024) { } else
		{
			printf("%u items attached\n", i);
		}
	}

	const Some *const S = a->u.data;
	const unsigned *I = a->index;
	assert(a->count == N);

	for(unsigned i = 0; i < N; i += 1)
	{
		printf("%08x\n", S[I[i]].key);
	}
	
	freearray(a);

	return 0;
}
