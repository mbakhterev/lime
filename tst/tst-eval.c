#include <lime/construct.h>
#include <lime/util.h>

unsigned item = 1;
const char *unitname = "test";

int main(int argc, char *argv[])
{
	Array *const U = newatomtab();
	Array *const lb = newverbmap(U, 0, ES("LB"));

	printf("loading\n");
	const Ref D = loaddag(stdin, "stdin", U, lb);
	printf("loaded\n");

	dumpdag(1, stdout, 0, U, D); // , NULL, NULL);
	fputc('\n', stdout);
	fflush(stdout);

	printf("going to eval\n");

	Array *const envmarks = newkeymap();
	const Array *const tomark = newverbmap(U, 0, ES("LB"));

	Core *const C = newcore(U, envmarks, tomark, 1);

	List *const inputs = RL(
		refnum(1),
		reflist(RL(refnum(2), refnum(3), readtoken(U, "hello"))),
		readtoken(U, "world"),
		refnum(4));

	const Ref d = eval(C, NULL, D, 0, inputs, EMDAG);
	freelist(inputs);

	const Ref rd = reconstruct(C->U, d, C->verbs.system, C->E, C->T, C->S);
	freeref(d);

	dumpdag(1, stdout, 0, U, rd); // , NULL, NULL);
	fputc('\n', stdout);
	fflush(stdout);

	freeref(rd);

	printf("envmarks ");
	dumpkeymap(1, stdout, 0, U, envmarks, NULL);
	fputc('\n', stdout);

	printf("environments ");
	dumptable(stdout, 0, U, C->E);
	fputc('\n', stdout);

	printf("types ");
	dumptable(stdout, 0, U, C->T);
	fputc('\n', stdout);

	printf("symbols ");
	dumptable(stdout, 0, U, C->S);
	fputc('\n', stdout);

	freecore(C);
	freekeymap((Array *)tomark);
	freekeymap(envmarks);
	freekeymap(lb);
	freeatomtab(U);

	return 0;
}
