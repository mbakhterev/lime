#include <lime/construct.h>
#include <lime/util.h>

#include <stdio.h>

unsigned field = 1;
unsigned item = 1;
const char *unitname = "stdin";

int main(int argc, char *argv[])
{
	Array U = makeatomtab();
	const LDContext lc = gencontext(stdin, &U);

	List *l = loaddag(&lc, NULL, NULL);

	freeuimap((Array *)lc.keymap);
	free((void *)lc.keymap);

	printf("len(l): %u\n", listlen(l));

 	const Array nonroots = keymap(&U, 0, ES("X", "Y"));

	List *k = gcnodes(&l, NULL, &nonroots);

	printf("len(l): %u; len(k): %u\n", listlen(l), listlen(k));

	const char *const d = dumpdag(&lc, k, 0);
	printf("dag(k):%s\n", d);
	freedag(k, NULL);
	free((void *)d);

	return 0;
}
