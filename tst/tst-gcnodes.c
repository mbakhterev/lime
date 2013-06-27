#include <lime/construct.h>
#include <lime/util.h>

#include <stdio.h>

unsigned field = 1;
unsigned item = 1;
const char *unitname = "stdin";

int main(int argc, char *argv[])
{
	Array U = makeatomtab();
	const Array DM = keymap(&U, 0, ES("Z", "ZA", "ZB"));

	List *l = loaddag(stdin, &U, &DM);

	printf("len(l): %u\n", listlen(l));

 	const Array nonroots = keymap(&U, 0, ES("X", "Y"));

	List *k = gcnodes(&l, &DM, &nonroots);

	printf("len(l): %u; len(k): %u\n", listlen(l), listlen(k));

	size_t sz = 0;
	char *d = NULL;
	FILE *const f = newmemstream(&d, &sz);
	dumpdag(f, 0, &U, k, &DM);
	fclose(f);

	printf("dag(k):%s\n", d);
	freedag(k, NULL);
	free((void *)d);

	return 0;
}
