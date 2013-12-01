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
	fputc('\n', stdout);
	fflush(stdout);

	Array *const root = newkeymap();

	DL(names, RS(readpack(U, strpack(0, "this"))));
	makepath(
		root, U, 
		readpack(U, strpack(0, "ENV")), names.u.list,
		markext(refkeymap(root)));

	Array *const envmarks = newkeymap();
	Array *const types = newkeymap();
	Array *const typemarks = newkeymap();

	Array *const node = newverbmap(U, 0, ES("S", "TEnv"));
	Array *const escape = newverbmap(U, 0, ES("F"));

	enveval(U, root, envmarks, D, escape, node);
	typeeval(U, types, typemarks, D, escape, envmarks);

	dumpkeymap(stdout, 0, U, root);
	dumpkeymap(stdout, 0, U, envmarks);
	dumpkeymap(stdout, 0, U, types);
	dumpkeymap(stdout, 0, U, typemarks);

	freekeymap(escape);
	freekeymap(node);

	freekeymap(typemarks);
	freekeymap(types);
	freekeymap(envmarks);
	freekeymap(root);

	freekeymap(lb);
	freeatomtab(U);

	return 0;
}
