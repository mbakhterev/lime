#include <stdio.h>

int main(int ac, char * av[])
{
	char buffer[1024];
	while(1)
	{
		scanf("%c.%c", buffer, buffer + 1);
		printf("%c %c\n", buffer[0], buffer[1]);
	}
	return 0;
}
