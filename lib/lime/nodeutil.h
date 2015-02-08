#ifndef LIBLIMENODEUTIL
#define LIBLIMENODEUTIL

#include "construct.h"
#include "util.h"

#include <stdio.h>

#define ERRNODE(U, N, fmt, ...) \
do { \
	fprintf(stderr, "%s:%u: ERR: %s: " fmt "\n", \
		nodefile(U, N), nodeline(N), nodename(U, N), __VA_ARGS__); \
	fflush(stderr); \
	checkout(STRIKE); \
} while (0)

#endif
