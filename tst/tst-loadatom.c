#include <lime/atomtab.h>

#include <stdio.h>
#include <ctype.h>

unsigned item = 0;
unsigned field = 0;
const char *unitname = "test";

int main(int argc, char * argv[]) {
	AtomTable t = mkatomtab(); 

	while(!feof(stdin)) {
		int c;
		while(isspace(c = fgetc(stdin))) {
		}
		if(!feof(stdin)) {
			ungetc(c, stdin);
			loadatom(&t, stdin);
			item += 1;
		}
	}

	for(unsigned i = 0; i < t.count; i += 1) {
		unsigned char *const a = (unsigned char *)tabindex(&t, i);
		const unsigned hint = a[0];
		a[0] = 0;
		printf("%02x.%u.\"%s\"\n", hint, atomlen(a), atombytes(a));
	}

	return 0;
}
