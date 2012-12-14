#ifndef symbolhincluded
#define symbolhincluded

typedef struct
{	
	
	char position[1];
} symbol;

extern symbol rdsymbol();
extern void wrsymbol(symbol *);

#endif
