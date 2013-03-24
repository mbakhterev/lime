#ifndef CONSTRUCTHINCLUDED
#define CONSTRUCTHINCLUDED

#include "atomtab.h"

#include <stdio.h>

typedef struct ListStruct List;

typedef struct {
	unsigned code;
	unsigned suffix;
	union {
		unsigned number;
		List * sources;
	} u;
	unsigned nrefs;
} Node;

enum { NUMBER, ATOM, TYPE, NODE, LIST, FREE = -1 };

struct ListStruct {
	List * next;
	union {
		List * list;
		Node * node;
		unsigned number;
	} u;
	int code;
};

typedef struct {
	union {
		Node * node;
	} u;
	unsigned code;
} Binding;

typedef struct {
	Array lists;
	Array index;
	Array bindings;
} Environment;

extern List * newlist(const unsigned code);
extern List * extend(List *const, List *const);

// Загрузить сырой список из файла (сырой - такой, в котором не подставлены типы
// и атомы) используя универсальную таблицу (надо куда-то складывать прочитанные
// "зюквочки".
extern List * loadrawlist(AtomTable * universe, List *const env, FILE *);

#endif
