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
		= makedagmap(&U, 0, ES("F"), (const char *const[]) { NULL });

	List *l = loaddag(stdin, &U, &DM.map);

	List *env = pushenvironment(NULL);
	List *ctx = pushcontext(NULL, &DM);

	evallists(&U, &l, &DM, NULL);
	evalforms(&U, l, &DM, env, ctx);

	size_t sz = 0;
	char *d = NULL;
	FILE *const f = newmemstream(&d, &sz);
	dumpdag(f, 0, &U, l, &DM.map);
	fclose(f);

	printf("dag(l):%s\n", d);
	freedag(l, NULL);
	free((void *)d);

	dumpenvironment(stdout, &U, env);

	return 0;
}
