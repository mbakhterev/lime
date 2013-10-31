#include <lime/construct.h>
#include <lime/util.h>

#include <stdio.h>

unsigned field = 1;
unsigned item = 1;
const char *unitname = "stdin";

int main(int argc, char *argv[])
{
	Array *const U = newatomtab();
	Array *const map = newverbmap(U, 0, ES("Z", "ZA", "ZB"));
	Array *const nogo = newverbmap(U, 0, ES("ZB"));
	Ref l = loaddag(stdin, U, map);

	printf("len(l): %u\n", listlen(l.u.list));
	dumpdag(1, stdout, 0, U, l, map);
	fputc('\n', stdout);

 	Array *const nonroots = newverbmap(U, 0, ES("X", "Y"));

	gcnodes(&l, nogo, nonroots, NULL);
	const Ref k = l;

	printf("len(l): %u; len(k): %u\n",
		listlen(l.u.list), listlen(k.u.list));

	size_t sz = 0;
	char *d = NULL;
	FILE *const f = newmemstream(&d, &sz);
	dumpdag(1, f, 0, U, k, map);
	fclose(f);

	printf("dag(k):%s\n", d);
	freeref(k);
	free((void *)d);

	return 0;
}
