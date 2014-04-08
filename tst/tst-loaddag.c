#include <lime/construct.h>
#include <lime/util.h>

#include <stdio.h>

unsigned field, item;
const char *unitname = "test";

int main(const int ac, const char *const av[])
{
	Array *const U = newatomtab();
	const Ref l = loaddag(stdin, U, NULL);

	printf("loaded\n");

	char *const c = strlist(NULL, l.u.list);
	printf("l: %s\n", c);
	free(c);

	printf("dag:\n");
	dumpdag(1, stdout, 0, U, l, NULL, NULL);

	return 0;
}
