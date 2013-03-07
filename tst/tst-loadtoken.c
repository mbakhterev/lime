#include <lime/atomtab.h>

#include <stdio.h>
#include <ctype.h>

static char * mkfmt(char *const buf) {
	sprintf(buf, "[0-%c.\"]", '0' + 63);
	return buf;
}

unsigned item = 0;
unsigned field = 0;
const char *unitname = "test";

int main(int argc, char * argv[]) {
	AtomTable t = ATOMTABNULL;
	char fmt[64];
	mkfmt(fmt);

	while(!feof(stdin)) {
		int c;
		while(isspace(c = fgetc(stdin))) {
		}
		if(!feof(stdin)) {
			ungetc(c, stdin);
			loadtoken(&t, stdin, 0, fmt);
			item += 1;
		}
	}

	for(unsigned i = 0; i < t.count; i += 1) {
		unsigned char *const a = (unsigned char *)t.index[i];
		fwrite(atombytes(a), 1, atomlen(a), stdout);
		fputc('\n', stdout);
	}

	return 0;
}