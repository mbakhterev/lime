#ifndef ATOMTABINCLUDED
#define ATOMTABINCLUDED

#include <stdio.h>

typedef struct {
	unsigned char ** atoms;
	unsigned * index;
	unsigned count;
	unsigned capacity;
} AtomTable;

#define ZEROATOMTAB = (AtomTable) { NULL, NULL, 0, 0 }

void resetatomtab(AtomTable *const);
unsigned rdatom(AtomTable *const, FILE *const);
unsigned rdtoken(AtomTable *const, FILE *const);

#endif
