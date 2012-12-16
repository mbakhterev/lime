#include <dp/token.h>
#include <dp/symbol.h>

#include <stdio.h>

#include <errno.h>
#include <error.h>

static unsigned lexcount = 1;

static const unsigned opsymcount = 10;

token rdtoken(FILE * f)
{
	unsigned line, byte, id;
	unsigned atomid = 0;

	if(fscanf(f, "%u.%u %u", &line, &byte, &id) == 3) {} else
	{
		error(1, errno, "lexem %u format is broken\n", lexcount);
	}

	if(id < opsymcount)
	{
	}

	lexcount += 1;

	return (token)
	{
		.offset = {.line = line, .byte = byte},
		.value = 
		{
			.identity = atomid,
			.code = id,
			.hint = 0
		}
	};
}
