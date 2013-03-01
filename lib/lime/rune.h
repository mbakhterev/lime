#ifndef RUNEHINCLUDED
#define RUNEHINCLUDED

extern unsigned char * writerune(unsigned char *const, unsigned);
extern const unsigned char * readrune(const unsigned char *, unsigned *const);

extern unsigned runerun(const unsigned char *const);
extern unsigned runelen(unsigned);

#endif
