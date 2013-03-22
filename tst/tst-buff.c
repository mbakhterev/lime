#include <lime/array.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct {
	unsigned a;
	unsigned char b;
	unsigned c;
} Some;

int main(int argc, char * argv[]) {
	Array a = mkarray(sizeof(Some));
	for(int i = 0; i < 16; i += 1) {
		unsigned len = rand() & 0x00ffffff;
		printf("%08x :", len);
		exporesize(&a, len);
		printf(" %08x\n", a.capacity);
	}

	return 0;
}
