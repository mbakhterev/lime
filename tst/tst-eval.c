#include <lime/construct.h>
#include <lime/util.h>

unsigned item = 1;
const char *unitname = "test";

static Array *newtarget(Array *const U, const Array *const map)
{
	return newkeymap();
}

static Array *nextpoint(Array *const U, const Array *const map)
{
	return (Array *)map;
}

static unsigned maypass(Array *const U, const Array *const map)
{
	return !0;
}

int main(int argc, char *argv[])
{
	Array *const U = newatomtab();
	Array *const lb = newverbmap(U, 0, ES("LB"));

	printf("loading\n");
	const Ref D = loaddag(stdin, U, lb);
	printf("loaded\n");

	dumpdag(1, stdout, 0, U, D);
// 	, lb);
	fputc('\n', stdout);
	fflush(stdout);

	printf("going to eval\n");

	Array *const root = newkeymap();

	DL(names, RS(readpack(U, strpack(0, "this"))));
	makepath(
		root, U, 
		readpack(U, strpack(0, "ENV")), names.u.list,
		markext(refkeymap(root)),
		maypass, newtarget, nextpoint);

	Array *const envmarks = newkeymap();
	Array *const types = newkeymap();
	Array *const typemarks = newkeymap();
	Array *const symbols = newkeymap();
	Array *const symmarks = newkeymap();

	Array *const node = newverbmap(U, 0, ES("S", "TEnv", "LB"));
	Array *const escape = newverbmap(U, 0, ES("F"));

	enveval(U, root, envmarks, D, escape, node);
	printf("environments done\n");
// 	dumpkeymap(stdout, 0, U, root);
// 	dumpkeymap(stdout, 0, U, envmarks);

	typeeval(U, types, typemarks, D, escape, envmarks);
	printf("types done\n");

	symeval(U, symbols, symmarks, D, escape, envmarks, typemarks);
	printf("symbols done\n");

	const Array *const stdesc = stdupstreams(U);
	dumpkeymap(1, stdout, 0, U, root, stdesc);
	freekeymap((Array *)stdesc);

	dumpkeymap(1, stdout, 0, U, envmarks, NULL);

	dumptable(stdout, 0, U, types);

	dumpkeymap(1, stdout, 0, U, typemarks, NULL);
	dumpkeymap(1, stdout, 0, U, symmarks, NULL);

	dumptable(stdout, 0, U, symbols);

	freekeymap(escape);
	freekeymap(node);

	freekeymap(symmarks);
	freekeymap(typemarks);
	freekeymap(types);
	freekeymap(envmarks);
	freekeymap(root);

	freekeymap(lb);
	freeatomtab(U);

	return 0;
}
