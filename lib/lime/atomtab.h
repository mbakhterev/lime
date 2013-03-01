#ifndef ATOMTABINCLUDED
#define ATOMTABINCLUDED

#include <stdio.h>

typedef const unsigned char * Atom;

typedef struct {
	Atom * atoms;
	Atom * index;
	unsigned count;
	unsigned capacity;
	unsigned char * temp;
	unsigned templen;
} AtomTable;

#define ATOMTABNULL (AtomTable) { NULL, NULL, NULL, 0, 0, 0 }
void resetatomtab(AtomTable *const);

unsigned readatom(AtomTable *const t, FILE *const f);

unsigned readtoken(AtomTable *const t, FILE *const f,
	const unsigned char hint, const unsigned char *const fmt);

unsigned loadbuffer(AtomTable *const,
	const unsigned char hint, const unsigned len,
	const unsigned char *const buff);

// В таком виде, чтобы можно было искать по ещё не оформленному атому. load/look
// buffer тогда позволят использовать AtomTable как ассоциативный массив для
// любых строк
unsigned lookbuffer(AtomTable *const,
	const unsigned char hint, const unsigned len,
	const unsigned char *const buff);

unsigned atomid(const Atom);
unsigned atomlen(const Atom);
unsigned atomhint(const Atom);
const unsigned char * atombytes(const Atom);

// Координаты при чтении, для формирования сообщения об ошибке. Устанавливаются
// извне
extern unsigned item;
extern unsigned field;
extern const char * unitname;

#endif
