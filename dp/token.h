#ifndef symbolhincluded
#define symbolhincluded

#include <stdio.h>

enum
{
	// atom hints
	hintid, hintdec, hinthex, hintstr
};

typedef struct
{
	unsigned length;
	unsigned char bytes[1];
} string;

// it may be overoptimization of memory usage
typedef struct
{
	// atom identity, i.e. offset of symbol string in some table
	unsigned identity:24;

	unsigned code:6;
	unsigned hint:2;
} tokenvalue;

typedef struct
{
	unsigned line;
	unsigned byte;
} tokenposition;

typedef struct
{	
	tokenvalue value;

	// relative offstet; abstracted to change details if it will be needed
	tokenposition offset;
} token;

// compiler with flto should optimize this
extern token rdtoken(FILE *);

extern void wrtoken(FILE *, const token *const);

#endif
