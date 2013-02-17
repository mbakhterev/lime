#ifndef SYMBOLHINCLUDED
#define SYMBOLHINCLUDED

#include <stdio.h>

// Окраски (hint) атома
enum {
	HINTID, HINTDEC, HINTHEX, HINTSTR
};

typedef struct {
	unsigned length;
	unsigned char bytes[1];
} string;

typedef struct {
	unsigned identity;
	unsigned char code, hint;
} tokenvalue;

typedef struct
{
	unsigned line;
	unsigned byte;
} tokenposition;

typedef struct
{	
	tokenvalue value;

	// relative offstet; abstracted to change details if needed
	tokenposition offset;
} token;

// compiler with LTO should optimize this
extern token rdtoken(FILE *);

extern void wrtoken(FILE *, const token *const);

#endif
