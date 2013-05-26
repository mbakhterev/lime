#include "util.h"

#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <error.h>

#define DBGEG 1

// #define DBGFLAGS (DBGEG)

#define DBGFLAGS 0

unsigned middle(const unsigned a, const unsigned b) {
	return a + (b - a) / 2;
}

unsigned min(const unsigned a, const unsigned b) {
	return a < b ? a : b;
}

int cmpui(const unsigned a, const unsigned b) {
	return 1 - (a == b) - ((a < b) << 1);
}

int cmpptr(const void *const a, const void *const b)
{
	return 1 - (a == b) - ((a < b) << 1);
}

static unsigned clp2(unsigned n) {
	assert(sizeof(unsigned) == 4);

	n -= 1;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	return n + 1;
}

void *expogrow(void *const buf, const unsigned ilen, const unsigned cnt) {
	const unsigned curlen = clp2(ilen * cnt);
	assert(curlen < MAXNUM);
	const unsigned len = clp2(ilen * (cnt + 1));
	assert(len && len < MAXNUM);

	if(len <= curlen) {
		return buf;
	}

	DBG(DBGEG, "resizing for buf: %p; il: %u; count: %u; len: %u -> %u",
		buf, ilen, cnt, curlen, len);
	
	void *const p = realloc(buf, len);
	assert(p);

	return p;
}

static void skipcomment(FILE *const f)
{
	int c;
	while((c = fgetc(f)) != '\n' && c != EOF) {}
	item += c == '\n';
}

int skipspaces(FILE *const f)
{
	int c;
	unsigned notdone = 1;
	while(notdone)
	{
		while(isspace(c = fgetc(f)))
		{
			item += c == '\n';
		}

		switch(c)
		{
		case '/':
		{
			switch(fgetc(f))
			{
			case '/':
				skipcomment(f);
				break;

			case EOF:
				notdone = 0;
				break;

			default:
				assert(fseek(f, -1, SEEK_CUR) == 0);
				notdone = 0;
				
			}

			break;
		}
		default:
			notdone = 0;
		}
	}

	return c;
}

void errexpect(const int have, const char *const E[])
{
	char *buf;
	size_t len;
	FILE *const f = newmemstream(&buf, &len);

	for(unsigned i = 0; E[i]; i += 1)
	{
		assert(fprintf(f, " %s", E[i]) > 0);
	}

	assert(fclose(f) == 0);

	if(have != EOF)
	{
		ERR("got %c expecting:%s", have, buf);
	}
	else
	{
		ERR("got EOF expecting:%s", buf);
	}

	free(buf);
}

FILE *newmemstream(char **const ptr, size_t *const szptr)
{
	FILE *f = open_memstream(ptr, szptr);
	assert(f);

	return f;
}
