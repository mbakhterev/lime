#include <lime/construct.h>
#include <lime/util.h>

#include <stdio.h>

unsigned field, item;
const char *unitname = "test";

int main(const int ac, const char *const av[])
{
	Array U = makeatomtab();
	List *const l = loaddag(stdin, &U, NULL);

	printf("loaded\n");

	char *const c = strlist(NULL, l);
	printf("l: %s\n", c);
	free(c);

	printf("dag:\n");
	dumpdag(1, stdout, 0, &U, l);

	return 0;
}
