#include <lime/construct.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

unsigned item = 1;
unsigned field = 1;
const char *unitname = NULL;

static int printer(List *const l, void *const ptr)
{
	switch(l->ref.code)
	{
	case NUMBER:
		printf("N: %u\n", l->ref.u.number);
		break; 

	case ATOM:
		printf("A: %u\n", l->ref.u.number);
		break; 

	case TYPE:
		printf("A: %u\n", l->ref.u.number);
		break; 

	case NODE:
		printf("NODE\n");
		break;

	case LIST:
		forlist(l->ref.u.list, printer, NULL, 0);

	default:
		assert(0);
	}

	return 0;
}

static int checkfree(List *const l, void *const ptr)
{
	assert(l->ref.code == FREE);
	return 0;
}

static void cutnprint(List *const k, const unsigned from, const unsigned to)
{
	const char *const C[2] = { "FAIL", "OK" };
	char *c = NULL;
	unsigned correct = 0;
	List *const l = forklistcut(k, from, to, &correct);

	printf("len(list; (%u; %u)): %u\n(list; (%u; %u)): %s\n%s\n",
		from, to, listlen(l),
		from, to, c = strlist(NULL, l), C[correct]);
	
	free(c);
	freelist(l);
}

int main(int argc, char * argv[])
{
	List * l = NULL;

	for(int i = 0; i < 20; i += 1)
	{
		List *const k = RL(refnum(i));

		forlist(k, printer, NULL, 0);
		l = append(l, k);
	}
	printf("---\n");

	char *c = strlist(NULL, l);
	printf("l: %s\n", c);
	free(c);

	List *const k = forklist(l);
	printf("k forked\n"); fflush(stdout);
	printf("---\n");
	forlist(k, printer, NULL, 0);
	printf("---\n");
	c = strlist(NULL, k);
	printf("k(%u): %s\n", listlen(k), c);
	free(c);

	freelist(k);
	forlist(k, checkfree, NULL, 0);

	List *const m = forklist(l);
	printf("m: %s\n", c = strlist(NULL, m));
	free(c);

	List *const n = forklist(m);
	printf("n: %s\n", c = strlist(NULL, n));
	free(c);

	Ref R[20];
	unsigned ncnt = writerefs(n, R, 20);
	assert(ncnt == 20);
	printf("R:");
	for(unsigned i = 0; i < ncnt; i += 1)
	{
		assert(R[i].code == NUMBER);
		printf(" %u", R[i].u.number);
	}
	printf("\n");

	List *const o = readrefs(R, sizeof(R)/sizeof(Ref));
	printf("o: %s\n", c = strlist(NULL, o));
	printf("len(o): %u\n", listlen(o));
	free(c);

	printf("len(NULL): %u; NULL: %s\n",
		listlen(NULL), c = strlist(NULL, NULL));
	free(c);

	cutnprint(o, 10, 14);
	cutnprint(o, 11, 15);
	cutnprint(o, 8, -1);
	cutnprint(o, 0, -1);
	cutnprint(o, -1, -1);

	DL(sl, RS(refnum(1), refnum(2), refnum(3), refnum(4)));
	DL(tl, RS(refnum(5), sl));

	printf("lists on stack done\n");

	char *d = NULL;
	printf("sl: %s\ntl: %s\n",
		c = strlist(NULL, sl.u.list), d = strlist(NULL, tl.u.list));
	free(c);
	free(d);

	return 0;
}
