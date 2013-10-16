#include <lime/construct.h>

#include <stdio.h>

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
		keymatch(A, A), keymatch(reffree(), A), keymatch(A, C));

	printf("(p a) = %u\n(p b) = %u\n", keymatch(p, a), keymatch(p, b));

	const Ref al = reflist(RL(A));
	const Ref fl = reflist(RL(reffree()));

	printf("(al al) = %u\n(al fl) = %u\n",
		keymatch(al, al), keymatch(al, fl));

	freeref(p);
	freeref(a);
	freeref(b);

	freeref(al);
	freeref(fl);
	
	return 0;
}
