typedef struct S T;

void fnA()
{
	struct S { T *aptr; };
//	T a;
	T *b;
}

struct S { T *tptr; };
T a;
T *b;

void fnB()
{
	struct S { T *bptr; };

	T a;
	T *b;

	struct S x;

	x.bptr = b->tptr;
}

void fnC()
{
// 	struct S { T *cptr; };

	T a;
	T *b;

	struct S x;

	x.tptr = b->tptr;
}

void fnD()
{
//	typedef struct S T;
	struct S { T *dptr; };
	typedef struct S T;
	struct S { T *eptr; };

	T a;
	T *b;

	struct S x;

	x.dptr = b->dptr;
}
