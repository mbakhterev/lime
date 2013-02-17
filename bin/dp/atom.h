#ifndef ATOMHINCLUDED
#define ATOMHINCLUDED

typedef const unsigned char * Atom;

// atomlen будет проверять, что значение вмещается в int
extern int atomlen(Atom);
extern int atomcmp(Atom);
extern int atomhint(Atom);

#endif
