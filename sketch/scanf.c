#include <stdio.h>

int main(int ac, char * av[])
{
	char buffer[1024];
	while(1)
	{
		scanf("%s.%s", buffer, buffer + 512);
		printf("%s %s\n", buffer, buffer + 512);
	}
	return 0;
}
