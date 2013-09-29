#include <stdio.h>

typedef struct { void *ptr; } Ref;

#define SIZE 11

int main(int argc, char *argv[])
{
	const Ref r = { .ptr = (int[SIZE]) { 10, 20, 30 } };

	for(unsigned i = 0; i < SIZE; i += 1)
	{
		printf("%d\n", ((int *)r.ptr)[i]);
	}

	return 0;
}
