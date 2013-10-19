#include <lime/construct.h>

#include <stdio.h>

const char *const unitname = "test";
unsigned item = 1;

int main(int argc, char *const argv[])
{
	Array *const U = newatomtab();

	const Ref A = readpack(U, strpack(0, "this"));
// 	const Ref B = readpack(U, strpack(0, "is"));
	const Ref C = readpack(U, strpack(0, "war"));

	const Ref p = decorate(reflist(RL(A, reffree())), U, ATOM);
	const Ref a = decorate(reflist(RL(A, C)), U, ATOM);

	const Ref b = decorate(reflist(RL(reffree(), C)), U, MAP);

	printf("(A A) = %u\n(F A) = %u\n(A C) = %u\n",
		keymatch(A, A, NULL, 0, NULL),
		keymatch(reffree(), A, NULL, 0, NULL),
		keymatch(A, C, NULL, 0, NULL));

	printf("(p a) = %u\n(p b) = %u\n",
		keymatch(p, a, NULL, 0, NULL),
		keymatch(p, b, NULL, 0, NULL));

	const Ref al = reflist(RL(A));
	const Ref fl = reflist(RL(reffree()));

	printf("(al al) = %u\n(al fl) = %u\n(fl al) = %u\n",
		keymatch(al, al, NULL, 0, NULL),
		keymatch(al, fl, NULL, 0, NULL),
		keymatch(fl, al, NULL, 0, NULL));

	freeref(p);
	freeref(a);
	freeref(b);

	freeref(al);
	freeref(fl);
	
	return 0;
}
