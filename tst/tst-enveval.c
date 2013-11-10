#include <lime/construct.h>
#include <lime/util.h>

unsigned item = 1;
const char *unitname = "test";

int main(int argc, char *argv[])
{
	Array *const U = newatomtab();
	Array *const lb = newverbmap(U, 0, ES("LB"));
	const Ref D = loaddag(stdin, U, lb);

	dumpdag(1, stdout, 0, U, D, lb);

	Array *const root = newkeymap();
	Array *const marks = newkeymap();

	Array *const node = newverbmap(U, 0, ES("S", "TEnv"));
	Array *const escape = newverbmap(U, 0, ES("F"));

	enveval(U, root, marks, D, escape, node);

	dumpkeymap(stdout, 0, U, root);
	dumpkeymap(stdout, 0, U, marks);

	freekeymap(escape);
	freekeymap(node);
	freekeymap(marks);
	freekeymap(root);

	freekeymap(lb);
	freeatomtab(U);

	return 0;
}
