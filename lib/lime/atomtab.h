#ifndef ATOMTABINCLUDED
#define ATOMTABINCLUDED

#include <stdio.h>

typedef struct {
	unsigned char ** atoms;
	unsigned * index;
	unsigned char * temp;
	unsigned tempsize;
	unsigned count;
	unsigned capacity;
} AtomTable;

#define ZEROATOMTAB (AtomTable) { NULL, NULL, NULL, 0, 0, 0 }

void resetatomtab(AtomTable *const);
unsigned rdatom(AtomTable *const, FILE *const);
unsigned rdtoken(AtomTable *const, FILE *const);

// Координаты при чтении, для формирования сообщения об ошибке. Устанавливаются
// извне
extern unsigned line;
extern unsigned lineoffset;
extern unsigned char * filename;

#endif
