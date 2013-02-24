#include <lime/rune.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(int argc, char * argv[]) {
	unsigned v;
	unsigned c;
	unsigned char in[8];
	unsigned char out[8];
	unsigned len;

	for(unsigned i = 0x7fffff00; i < 0x80000000; i += 1) {
		assert(wrrune(out, i) - out == 6);
		assert(rdrune(out, &v) - out == 6);
		assert(v == i);
	}

	while((c = fgetc(stdin)) != EOF) {
		memset(in, 0, sizeof(in));
		memset(out, 0, sizeof(out));

		len = runerun((const unsigned char *)&c);
		// printf("%u:", len);

		in[0] = c;
		for(int i = 1; i < len; i += 1) {
			in[i] = fgetc(stdin);
			assert(c != EOF);
		}

		assert(rdrune(in, &v) - in == len);

		// printf("%u:%x:", runelen(v), v);

		assert(len == runelen(v));
		assert(wrrune(out, v) - out == len);
		assert(runerun(out) == runelen(v));

		fwrite(out, 1, len, stdout);
		// printf("; ");
	}

	return 0;
}
