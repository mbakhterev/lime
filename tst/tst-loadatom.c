#include <lime/construct.h>

#include <stdio.h>
#include <ctype.h>

unsigned item = 0;
unsigned field = 0;
const char *unitname = "test";

int main(int argc, char * argv[])
{
	Array *const t = newatomtab(); 

	while(!feof(stdin))
	{
		int c;
		while(isspace(c = fgetc(stdin)))
		{
		}

		if(!feof(stdin))
		{
			ungetc(c, stdin);
			loadatom(t, stdin);
			item += 1;
		}
	}

	const unsigned *const I = t->index;
	for(unsigned i = 0; i < t->count; i += 1)
	{
		unsigned char *const a = *(unsigned char **)itemat(t, I[i]);
		const unsigned hint = a[0];
		printf("%02x.%u.\"%s\"\n", hint, atomlen(a), atombytes(a));
	}

	freeatomtab(t);

	return 0;
}
