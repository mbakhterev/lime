#include <stdio.h>

struct test { int a; int b; struct test *ptr; };

static void f1(const struct test *const ptr)
{
}

static struct test f2(void)
{
	return (struct test) { 1, 2, NULL };
}

int main(int argc, char *argv[])
{
	f1(&((struct test) { 1, 2, NULL }));
	f1(&(struct test)f2());
	f1(&f2());
	return 0;
}
