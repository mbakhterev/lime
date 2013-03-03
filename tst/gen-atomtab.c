#include <stdio.h>
#include <stdlib.h>

#include <lime/rune.h>

static const unsigned N = 1024;
static const unsigned CNT = 72;

static unsigned genrune() {
	return rand() % (1024) + '0';
}

int main(int argc, char * argv[]) {
	unsigned char buf[CNT * 8];

	for(unsigned i = 0; i < N; i += 1) {
		const unsigned hint = rand() % 256;
		const unsigned count = rand() % CNT;

		unsigned char *p = buf;
		for(unsigned j = 0; j < count; j += 1) {
			p = writerune(p, genrune());
		}

		printf("%02x.%u.\"", hint, (unsigned)(p - buf));
		fwrite(buf, 1, p - buf, stdout);
		printf("\"\n");
	}

	return 0;
}
