#include <lime/atomtab.h>
#include <stdio.h>

int main(int argc, char * argv[]) {
	AtomTable t = ZEROATOMTAB;
	rdatom(&t, stdin);
	return 0;
}
