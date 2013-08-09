#include <stdlib.h>
#include <stdio.h>

int main(int argc, char * argv[]) {
	srand(time(NULL));
	unsigned char a[rand() % 0x1000];
	unsigned char (* buf)[rand() % 0x1000];
	unsigned char * x;
	printf("%zi %zi %zi\n", sizeof(a), sizeof(*buf), sizeof(*x));

	printf("%zi\n", sizeof((const unsigned[]) { [1] = 2, [3] = 4 }));

	return 0;
}
