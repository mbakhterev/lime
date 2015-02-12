#include "fs.h"

#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

char *joinpath(const char *const base, const char *const name)
{
	assert(name);

	if(name[0] == '/')
	{
		return strcpy(malloc(strlen(name) + 1), name);
	}

	assert(base);

	char dirbuf[strlen(base) + 1];
	char *const dir = dirname(strcpy(dirbuf, base));
	assert(dir);
	const unsigned dirlen = strlen(dir);

	char *const buffer = malloc(dirlen + strlen(name) + 1 + 1);
	assert(buffer);

	strcpy(buffer, dir)[dirlen] = '/';
	strcpy(buffer + dirlen + 1, name);

	return buffer;
}
