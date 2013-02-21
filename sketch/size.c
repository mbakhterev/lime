#include <stdlib.h>
#include <stdio.h>

int main(int argc, char * argv[]) {
	srand(time(NULL));
	unsigned char a[rand() % 0x1000];
	printf("%zi\n", sizeof(a));
	return 0;
}
