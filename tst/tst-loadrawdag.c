#include <lime/construct.h>
#include <lime/util.h>

#include <stdio.h>

unsigned field, item;
const char *unitname = "test";

int main(const int ac, const char *const av[]) {
	Array U = makeatomtab();

	LoadContext lc = 
	{
		.file = stdin,
		.state = NULL,
		.universe = &U,
		.keymap = NULL,
		.actions = NULL,
		.keyonly = 0
	};

	List *const l = loadrawdag(&lc, NULL, NULL);

	printf("loaded\n");

	char *const c = dumplist(l);
	printf("l: %s\n", c);

	return 0;
}
