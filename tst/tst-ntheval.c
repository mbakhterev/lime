#include <lime/construct.h>
#include <lime/util.h>

#include <stdio.h>

unsigned field = 1;
unsigned item = 1;
const char *unitname = "stdin";

#define DBGFLAGS 1

int main(int argc, char *argv[])
{
	Array *const U = newatomtab();

	const Array *const map = newverbmap(U, 0, ES("Z", "ZA", "ZB"));

// 	const Array *const go = newverbmap(U, 0, ES("Z", "ZA"));
	const Array *const escape = newverbmap(U, 0, ES("ZB"));
 	const Array *const nonroots = newverbmap(U, 0, ES("Nth", "FIn"));
	
	const unsigned ok = CKPT() == 0;
	if(!ok)
	{
		DBG(DBGFLAGS, "%s", "FAIL");
		return EXIT_FAILURE;
	}

	const Ref l = loaddag(stdin, U, map);

	printf("original:\n");
	dumpdag(1, stdout, 0, U, l);
// 	, map);
	printf("\n");

	const Array *const symmarks = newkeymap();
	const Array *const typemarks = newkeymap();
	const Array *const types = newkeymap();
	const Array *const symbols = newkeymap();

	const List *const L = RL(refnum(1), refnum(2), refnum(3), refnum(4));

	Ref el = ntheval(U, l, escape, typemarks, types, symmarks, symbols, L);

	freeref(l);
	freelist((List *)L);

	printf("\nevaluated:\n");
	dumpdag(1, stdout, 0, U, el);
// 	, map);
	printf("\n");

	gcnodes(&el, escape, nonroots, NULL);

	printf("\ncleaned:\n");
	dumpdag(1, stdout, 0, U, el);
// 	, map);
	printf("\n");

	freeref(el);

	return 0;
}
