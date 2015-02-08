#include "util.h"
#include "construct.h"

void doerror(const Ref N, const Marks *const M, const Core *const C)
{
	Array *const U = C->U;
	const Array *const V = C->verbs.system;
	Array *const marks = M->marks;

	const Ref val = simplerewrite(nodeattribute(N), marks, V);
	const char *const str = strref(U, NULL, val);
	freeref(val);

	fprintf(stderr, "%s:%u: ERR: %s\n",
		nodefilename(U, N), nodeline(N), str);

	fflush(stderr);
	free((char *)str);

	checkout(STRIKE);
}

void dodebug(const Ref N, const Marks *const M, const Core *const C)
{
	Array *const U = C->U;
	const Array *const V = C->verbs.system;
	Array *const marks = M->marks;

	const Ref val = simplerewrite(nodeattribute(N), marks, V);
	const char *const str = strref(U, NULL, val);
	freeref(val);

	fprintf(stderr, "%s.%u: DBG: %s\n",
		nodefilename(U, N), nodeline(N), str);

	free((char *)str);
}
