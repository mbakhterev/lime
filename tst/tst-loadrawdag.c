#include <lime/construct.h>
#include <lime/util.h>

#include <stdio.h>

unsigned field, item;
const char *unitname = "test";

int main(const int ac, const char *const av[])
{
	Array U = makeatomtab();

	const LoadContext lc = gencontext(stdin, &U);

	List *const l = loadrawdag(&lc, NULL, NULL);

	freeuimap((Array *)lc.keymap);
	free((void *)lc.keymap);

	printf("loaded\n");

	char *const c = dumplist(l);
	printf("l: %s\n", c);

	return 0;
}
