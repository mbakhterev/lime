#include <lime/buffer.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct {
	unsigned a;
	unsigned char b;
	unsigned c;
} Some;

int main(int argc, char * argv[]) {
	void * p = NULL;
	for(int i = 0; i < 16; i += 1) {
		unsigned len = rand() & 0x00ffffff;
		printf("%08x :", len);
		p = exporesize(p, &len, sizeof(Some));
		printf(" %08x\n", len);
	}

	return 0;
}
