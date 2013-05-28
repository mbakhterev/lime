#include <stdio.h>

int fn(struct { int a; int b; } *const ptr)
{
	return ptr->a + ptr->b;
}

int main(int argc, char *const argv[])
{
	struct { int a; int b; } z;
	z.a = 10;
	z.b = 20;

	printf("%d\n", fn(&z));

	return 0;
}
