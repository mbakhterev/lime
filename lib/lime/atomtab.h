#ifndef ATOMTABINCLUDED
#define ATOMTABINCLUDED

#include "array.h"

#include <stdio.h>

typedef const unsigned char * Atom;

typedef struct {
	unsigned char * temp;
	unsigned templen;
//	unsigned count;
	Array atoms;
	Array index;
} AtomTable;

AtomTable mkatomtab(void);

unsigned loadatom(AtomTable *const t, FILE *const f);

unsigned loadtoken(AtomTable *const t, FILE *const f,
	const unsigned char hint, const char *const format);

unsigned readbuffer(AtomTable *const,
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

Atom tabatoms(const AtomTable *const, unsigned n);
Atom tabindex(const AtomTable *const, unsigned i);
unsigned tabcount(const AtomTable *const);

#endif
