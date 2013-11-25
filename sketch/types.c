#include <stdio.h>

static int fnA(struct { int a; int b; } *const ptr)
{
	return ptr->a + ptr->b;
}

int main(int argc, char *const argv[])
{
	struct { int a; int b; } z;
	z.a = 10;
	z.b = 20;

//	printf("%d\n", fnA(&z));

	return 0;
}

typedef struct X SX;

static SX sxA;

struct X
{
	int y;
};

static void fnB(void)
{
	struct X { int x; };

	SX sx;
}

typedef struct Y TY;
struct Y { struct Z *ptr; };

static void fna()
{
	struct Z { int a; };

	TY a;
}

struct Z { float z; };

static void fnb()
{
	struct Z { float b; };
	TY b;

	printf("%f\n", b.ptr->z);
}
