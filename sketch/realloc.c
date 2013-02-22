#include <stdio.h>
#include <stdlib.h>

int main(int argc, char * argv[]) {
	void * p = realloc(NULL, 1024);
	printf("%p\n", p);
	return 0;
}
