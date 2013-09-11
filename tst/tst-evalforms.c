#include <lime/construct.h>
#include <lime/util.h>

#include <stdio.h>

unsigned field = 1;
unsigned item = 1;
const char *unitname = "stdin";

int main(int argc, char *argv[])
{
	Array U = makeatomtab();

// 	const DagMap DM
// 		= makedagmap(&U, 0, ES("F"), (const char *const[]) { NULL });

	const Array map = keymap(&U, 0, ES("F"));

	List *l = loaddag(stdin, &U, &map);

	List *env = pushenvironment(NULL);
//	List *ctx = pushcontext(NULL);
	List *ctx = NULL;

	evallists(&U, &l, &map, NULL, NULL);
	evalforms(&U, l, &map, NULL, env, ctx);

	size_t sz = 0;
	char *d = NULL;
	FILE *const f = newmemstream(&d, &sz);
// 	dumpdag(f, 0, &U, l, &DM.map);
	dumpdag(1, f, 0, &U, l, &map);
	fclose(f);

	printf("dag(l):%s\n", d);
	freedag(l, NULL);
	free((void *)d);

	dumpenvironment(stdout, 0, &U, env);

	return 0;
}
