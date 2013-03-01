#include <lime/atomtab.h>
#include <stdio.h>

unsigned item = 0;
unsigned field = 0;
const char *unitname = "test";

int main(int argc, char * argv[]) {
	AtomTable t = ATOMTABNULL;
	readatom(&t, stdin);
	fwrite(atombytes(t.atoms[0]), 1, atomlen(t.atoms[0]), stdout);
	printf("\n");
	return 0;
}
