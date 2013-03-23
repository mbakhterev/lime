#ifndef LISTHINCLUDED
#define LISTHINCLUDED

typedef struct ListStruct List;

typedef union {
	unsigned atom;
	unsigned number;
	List * list;
	struct {
		unsigned name;
		unsigned suffix;
		List * kids;
	} node;
} NodeOneof;


typedef struct ListStruct {
	ListOneof oneof;
	unsigned code;
	unsigned atline;
	struct ListStruct * next;
} List;

enum { ATOM, NUMBER, SUBLIST, NODE };

extern List * newlistnode(const unsigned code, const ListOneof);
extern List * expand(List *, List *);

#endif
