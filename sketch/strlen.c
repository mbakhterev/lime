#include <stdio.h>

int main(int argc, char * argv[])
{
	printf("%u\n", sizeof("hello")/sizeof(char));
	return 0;
}
