#include <lime/construct.h>
#include <lime/util.h>

#include <stdio.h>

unsigned field = 1;
unsigned item = 1;
const char *unitname = "stdin";

int main(int argc, char *argv[])
{
	Array U = makeatomtab();

	const DagMap DM
		= makedagmap(&U, 0, ES("Z", "ZA", "ZB"), ES("Z", "ZA"));
	
	const Array nonroots
// 		= keymap(&U, 0, ES("L"));
		= keymap(&U, 0, ES("L", "LNth", "FIn"));

	List *l = loaddag(stdin, &U, &DM.map);

	evallists(&U, &l, &DM, RL(refnum(1), refnum(2), refnum(3)));
	gcnodes(&l, &DM, &nonroots);

	size_t sz = 0;
	char *d = NULL;
	FILE *const f = newmemstream(&d, &sz);
	dumpdag(f, 0, &U, l, &DM.map);
	fclose(f);

	printf("dag(l):%s\n", d);
	freedag(l, NULL);
	free((void *)d);

	return 0;
}