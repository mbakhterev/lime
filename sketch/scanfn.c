#include <stdio.h>

int main(int argc, char * argv[]) {
	unsigned k, l, n;
	printf("pos: %ld\n", ftell(stdin));
	int r = scanf("%02x.%u%n", &k, &l, &n);
	printf("k: %u, l: %u, n: %u, r: %u\n", k, l, n, r);
	return 0;
}
