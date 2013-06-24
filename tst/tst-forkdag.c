#include <lime/construct.h>
#include <lime/util.h>

#include <stdio.h>

unsigned field = 1;
unsigned item = 1;
const char *unitname = "stdin";

int main(int argc, char *argv[])
{
	Array U = makeatomtab();
	List *const l = loaddag(stdin, &U, NULL);
	List *const k = forkdag(l, NULL);

	printf("len(l): %u\n", listlen(l));

	size_t sz = 0;
	char *d = NULL;

	FILE *f = newmemstream(&d, &sz);
	dumpdag(f, 0, &U, l, NULL);
	fclose(f);

	printf("dag(l):%s\n", d);
	freedag(l, NULL);
	free((void *)d);

	f = newmemstream(&d, &sz);
	dumpdag(f, 0, &U, k, NULL);
	fclose(f);

	printf("dag(k):%s\n", d);
	freedag(k, NULL);
	free((void *)d);
	return 0;
}
