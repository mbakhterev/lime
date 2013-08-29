#include <lime/construct.h>
#include <lime/util.h>

#include <stdio.h>

unsigned field = 1;
unsigned item = 1;
const char *unitname = "stdin";

#define DBGFLAGS 1

int main(int argc, char *argv[])
{
	Array U = makeatomtab();
	const Array map = keymap(&U, 0, ES("Z", "ZA", "ZB"));
	const Array go = keymap(&U, 0, ES("Z", "ZA"));
	
	const Array nonroots
		= keymap(&U, 0, ES("L", "LNth", "FIn"));
	
	List *l = NULL;

	unsigned ok = CKPT() == 0;
	if(ok)
	{
		l = loaddag(stdin, &U, &map);
	}
	else
	{
		DBG(DBGFLAGS, "%s", "FAIL");
		return EXIT_FAILURE;
	}

	evallists(&U, &l, &map, &go, RL(refnum(1), refnum(2), refnum(3)));
	gcnodes(&l, &map, &go, &nonroots);

	size_t sz = 0;
	char *d = NULL;
	FILE *const f = newmemstream(&d, &sz);
	dumpdag(f, 0, &U, l, &map);
	fclose(f);

	printf("dag(l):%s\n", d);
	freedag(l, NULL);
	free((void *)d);

	return 0;
}
