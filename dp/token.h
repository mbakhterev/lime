#ifndef symbolhincluded
#define symbolhincluded

#include <stdio.h>

enum
{
	aid, adec, ahex, astr
};

typedef struct
{
	unsigned length;
	unsigned char bytes[1];
} string;

typedef struct
{
	unsigned string:23;
	unsigned code:6;
	unsigned hint:3;
} tokenvalue;

typedef struct
{	
	tokenvalue value;
	unsigned char position[1];	// 32-bit в формате utf-8
} Token;

extern void rdtoken(FILE *, Token *const *const);
extern void wrtoken(FILE *, const Token *const *const);

#endif
